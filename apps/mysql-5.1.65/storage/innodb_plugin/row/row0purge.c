/*****************************************************************************

Copyright (c) 1997, 2011, Oracle and/or its affiliates. All Rights Reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA

*****************************************************************************/

/**************************************************//**
@file row/row0purge.c
Purge obsolete records

Created 3/14/1997 Heikki Tuuri
*******************************************************/

#include "row0purge.h"

#ifdef UNIV_NONINL
#include "row0purge.ic"
#endif

#include "fsp0fsp.h"
#include "mach0data.h"
#include "trx0rseg.h"
#include "trx0trx.h"
#include "trx0roll.h"
#include "trx0undo.h"
#include "trx0purge.h"
#include "trx0rec.h"
#include "que0que.h"
#include "row0row.h"
#include "row0upd.h"
#include "row0vers.h"
#include "row0mysql.h"
#include "log0log.h"

/*************************************************************************
IMPORTANT NOTE: Any operation that generates redo MUST check that there
is enough space in the redo log before for that operation. This is
done by calling log_free_check(). The reason for checking the
availability of the redo log space before the start of the operation is
that we MUST not hold any synchonization objects when performing the
check.
If you make a change in this module make sure that no codepath is
introduced where a call to log_free_check() is bypassed. */

/********************************************************************//**
Creates a purge node to a query graph.
@return	own: purge node */
UNIV_INTERN
purge_node_t*
row_purge_node_create(
/*==================*/
	que_thr_t*	parent,	/*!< in: parent node, i.e., a thr node */
	mem_heap_t*	heap)	/*!< in: memory heap where created */
{
	purge_node_t*	node;

	ut_ad(parent && heap);

	node = mem_heap_alloc(heap, sizeof(purge_node_t));

	node->common.type = QUE_NODE_PURGE;
	node->common.parent = parent;

	node->heap = mem_heap_create(256);

	return(node);
}

/***********************************************************//**
Repositions the pcur in the purge node on the clustered index record,
if found.
@return	TRUE if the record was found */
static
ibool
row_purge_reposition_pcur(
/*======================*/
	ulint		mode,	/*!< in: latching mode */
	purge_node_t*	node,	/*!< in: row purge node */
	mtr_t*		mtr)	/*!< in: mtr */
{
	ibool	found;

	if (node->found_clust) {
		found = btr_pcur_restore_position(mode, &(node->pcur), mtr);

		return(found);
	}

	found = row_search_on_row_ref(&(node->pcur), mode, node->table,
				      node->ref, mtr);
	node->found_clust = found;

	if (found) {
		btr_pcur_store_position(&(node->pcur), mtr);
	}

	return(found);
}

/***********************************************************//**
Removes a delete marked clustered index record if possible.
@return TRUE if success, or if not found, or if modified after the
delete marking */
static
ibool
row_purge_remove_clust_if_poss_low(
/*===============================*/
	purge_node_t*	node,	/*!< in: row purge node */
	ulint		mode)	/*!< in: BTR_MODIFY_LEAF or BTR_MODIFY_TREE */
{
	dict_index_t*	index;
	btr_pcur_t*	pcur;
	btr_cur_t*	btr_cur;
	ibool		success;
	ulint		err;
	mtr_t		mtr;
	rec_t*		rec;
	mem_heap_t*	heap		= NULL;
	ulint		offsets_[REC_OFFS_NORMAL_SIZE];
	rec_offs_init(offsets_);

	index = dict_table_get_first_index(node->table);

	pcur = &(node->pcur);
	btr_cur = btr_pcur_get_btr_cur(pcur);

	log_free_check();
	mtr_start(&mtr);

	success = row_purge_reposition_pcur(mode, node, &mtr);

	if (!success) {
		/* The record is already removed */

		btr_pcur_commit_specify_mtr(pcur, &mtr);

		return(TRUE);
	}

	rec = btr_pcur_get_rec(pcur);

	if (0 != ut_dulint_cmp(node->roll_ptr, row_get_rec_roll_ptr(
				       rec, index, rec_get_offsets(
					       rec, index, offsets_,
					       ULINT_UNDEFINED, &heap)))) {
		if (UNIV_LIKELY_NULL(heap)) {
			mem_heap_free(heap);
		}
		/* Someone else has modified the record later: do not remove */
		btr_pcur_commit_specify_mtr(pcur, &mtr);

		return(TRUE);
	}

	if (UNIV_LIKELY_NULL(heap)) {
		mem_heap_free(heap);
	}

	if (mode == BTR_MODIFY_LEAF) {
		success = btr_cur_optimistic_delete(btr_cur, &mtr);
	} else {
		ut_ad(mode == BTR_MODIFY_TREE);
		btr_cur_pessimistic_delete(&err, FALSE, btr_cur,
					   RB_NONE, &mtr);

		if (err == DB_SUCCESS) {
			success = TRUE;
		} else if (err == DB_OUT_OF_FILE_SPACE) {
			success = FALSE;
		} else {
			ut_error;
		}
	}

	btr_pcur_commit_specify_mtr(pcur, &mtr);

	return(success);
}

/***********************************************************//**
Removes a clustered index record if it has not been modified after the delete
marking. */
static
void
row_purge_remove_clust_if_poss(
/*===========================*/
	purge_node_t*	node)	/*!< in: row purge node */
{
	ibool	success;
	ulint	n_tries	= 0;

	/*	fputs("Purge: Removing clustered record\n", stderr); */

	success = row_purge_remove_clust_if_poss_low(node, BTR_MODIFY_LEAF);
	if (success) {

		return;
	}
retry:
	success = row_purge_remove_clust_if_poss_low(node, BTR_MODIFY_TREE);
	/* The delete operation may fail if we have little
	file space left: TODO: easiest to crash the database
	and restart with more file space */

	if (!success && n_tries < BTR_CUR_RETRY_DELETE_N_TIMES) {
		n_tries++;

		os_thread_sleep(BTR_CUR_RETRY_SLEEP_TIME);

		goto retry;
	}

	ut_a(success);
}

/***********************************************************//**
Removes a secondary index entry if possible.
@return	TRUE if success or if not found */
static
ibool
row_purge_remove_sec_if_poss_low(
/*=============================*/
	purge_node_t*	node,	/*!< in: row purge node */
	dict_index_t*	index,	/*!< in: index */
	const dtuple_t*	entry,	/*!< in: index entry */
	ulint		mode)	/*!< in: latch mode BTR_MODIFY_LEAF or
				BTR_MODIFY_TREE */
{
	btr_pcur_t	pcur;
	btr_cur_t*	btr_cur;
	ibool		success;
	ibool		old_has = 0; /* remove warning */
	ibool		found;
	ulint		err;
	mtr_t		mtr;
	mtr_t		mtr_vers;

	log_free_check();
	mtr_start(&mtr);

	found = row_search_index_entry(index, entry, mode, &pcur, &mtr);

	if (!found) {
		/* Not found.  This is a legitimate condition.  In a
		rollback, InnoDB will remove secondary recs that would
		be purged anyway.  Then the actual purge will not find
		the secondary index record.  Also, the purge itself is
		eager: if it comes to consider a secondary index
		record, and notices it does not need to exist in the
		index, it will remove it.  Then if/when the purge
		comes to consider the secondary index record a second
		time, it will not exist any more in the index. */

		/* fputs("PURGE:........sec entry not found\n", stderr); */
		/* dtuple_print(stderr, entry); */

		btr_pcur_close(&pcur);
		mtr_commit(&mtr);

		return(TRUE);
	}

	btr_cur = btr_pcur_get_btr_cur(&pcur);

	/* We should remove the index record if no later version of the row,
	which cannot be purged yet, requires its existence. If some requires,
	we should do nothing. */

	mtr_start(&mtr_vers);

	success = row_purge_reposition_pcur(BTR_SEARCH_LEAF, node, &mtr_vers);

	if (success) {
		old_has = row_vers_old_has_index_entry(
			TRUE, btr_pcur_get_rec(&(node->pcur)),
			&mtr_vers, index, entry);
	}

	btr_pcur_commit_specify_mtr(&(node->pcur), &mtr_vers);

	if (!success || !old_has) {
		/* Remove the index record */

		if (mode == BTR_MODIFY_LEAF) {
			success = btr_cur_optimistic_delete(btr_cur, &mtr);
		} else {
			ut_ad(mode == BTR_MODIFY_TREE);
			btr_cur_pessimistic_delete(&err, FALSE, btr_cur,
						   RB_NONE, &mtr);
			success = err == DB_SUCCESS;
			ut_a(success || err == DB_OUT_OF_FILE_SPACE);
		}
	}

	btr_pcur_close(&pcur);
	mtr_commit(&mtr);

	return(success);
}

/***********************************************************//**
Removes a secondary index entry if possible. */
UNIV_INLINE
void
row_purge_remove_sec_if_poss(
/*=========================*/
	purge_node_t*	node,	/*!< in: row purge node */
	dict_index_t*	index,	/*!< in: index */
	dtuple_t*	entry)	/*!< in: index entry */
{
	ibool	success;
	ulint	n_tries		= 0;

	/*	fputs("Purge: Removing secondary record\n", stderr); */

	success = row_purge_remove_sec_if_poss_low(node, index, entry,
						   BTR_MODIFY_LEAF);
	if (success) {

		return;
	}
retry:
	success = row_purge_remove_sec_if_poss_low(node, index, entry,
						   BTR_MODIFY_TREE);
	/* The delete operation may fail if we have little
	file space left: TODO: easiest to crash the database
	and restart with more file space */

	if (!success && n_tries < BTR_CUR_RETRY_DELETE_N_TIMES) {

		n_tries++;

		os_thread_sleep(BTR_CUR_RETRY_SLEEP_TIME);

		goto retry;
	}

	ut_a(success);
}

/***********************************************************//**
Purges a delete marking of a record. */
static
void
row_purge_del_mark(
/*===============*/
	purge_node_t*	node)	/*!< in: row purge node */
{
	mem_heap_t*	heap;
	dtuple_t*	entry;
	dict_index_t*	index;

	ut_ad(node);

	heap = mem_heap_create(1024);

	while (node->index != NULL) {
		index = node->index;

		/* Build the index entry */
		entry = row_build_index_entry(node->row, NULL, index, heap);
		ut_a(entry);
		row_purge_remove_sec_if_poss(node, index, entry);

		node->index = dict_table_get_next_index(node->index);
	}

	mem_heap_free(heap);

	row_purge_remove_clust_if_poss(node);
}

/***********************************************************//**
Purges an update of an existing record. Also purges an update of a delete
marked record if that record contained an externally stored field. */
static
void
row_purge_upd_exist_or_extern_func(
/*===============================*/
#ifdef UNIV_DEBUG
	const que_thr_t*thr,	/*!< in: query thread */
#endif /* UNIV_DEBUG */
	purge_node_t*	node)	/*!< in: row purge node */
{
	mem_heap_t*	heap;
	dtuple_t*	entry;
	dict_index_t*	index;
	ibool		is_insert;
	ulint		rseg_id;
	ulint		page_no;
	ulint		offset;
	ulint		i;
	mtr_t		mtr;

	ut_ad(node);

	if (node->rec_type == TRX_UNDO_UPD_DEL_REC
	    || (node->cmpl_info & UPD_NODE_NO_ORD_CHANGE)) {

		goto skip_secondaries;
	}

	heap = mem_heap_create(1024);

	while (node->index != NULL) {
		index = node->index;

		if (row_upd_changes_ord_field_binary(node->index, node->update,
						     thr, NULL, NULL)) {
			/* Build the older version of the index entry */
			entry = row_build_index_entry(node->row, NULL,
						      index, heap);
			ut_a(entry);
			row_purge_remove_sec_if_poss(node, index, entry);
		}

		node->index = dict_table_get_next_index(node->index);
	}

	mem_heap_free(heap);

skip_secondaries:
	/* Free possible externally stored fields */
	for (i = 0; i < upd_get_n_fields(node->update); i++) {

		const upd_field_t*	ufield
			= upd_get_nth_field(node->update, i);

		if (dfield_is_ext(&ufield->new_val)) {
			buf_block_t*	block;
			ulint		internal_offset;
			byte*		data_field;

			/* We use the fact that new_val points to
			node->undo_rec and get thus the offset of
			dfield data inside the undo record. Then we
			can calculate from node->roll_ptr the file
			address of the new_val data */

			internal_offset
				= ((const byte*)
				   dfield_get_data(&ufield->new_val))
				- node->undo_rec;

			ut_a(internal_offset < UNIV_PAGE_SIZE);

			trx_undo_decode_roll_ptr(node->roll_ptr,
						 &is_insert, &rseg_id,
						 &page_no, &offset);
			mtr_start(&mtr);

			/* We have to acquire an X-latch to the clustered
			index tree */

			index = dict_table_get_first_index(node->table);

			mtr_x_lock(dict_index_get_lock(index), &mtr);

			/* NOTE: we must also acquire an X-latch to the
			root page of the tree. We will need it when we
			free pages from the tree. If the tree is of height 1,
			the tree X-latch does NOT protect the root page,
			because it is also a leaf page. Since we will have a
			latch on an undo log page, we would break the
			latching order if we would only later latch the
			root page of such a tree! */

			btr_root_get(index, &mtr);

			/* We assume in purge of externally stored fields
			that the space id of the undo log record is 0! */

			block = buf_page_get(0, 0, page_no, RW_X_LATCH, &mtr);
			buf_block_dbg_add_level(block, SYNC_TRX_UNDO_PAGE);

			data_field = buf_block_get_frame(block)
				+ offset + internal_offset;

			ut_a(dfield_get_len(&ufield->new_val)
			     >= BTR_EXTERN_FIELD_REF_SIZE);
			btr_free_externally_stored_field(
				index,
				data_field + dfield_get_len(&ufield->new_val)
				- BTR_EXTERN_FIELD_REF_SIZE,
				NULL, NULL, NULL, 0, RB_NONE, &mtr);
			mtr_commit(&mtr);
		}
	}
}

#ifdef UNIV_DEBUG
# define row_purge_upd_exist_or_extern(thr,node)	\
	row_purge_upd_exist_or_extern_func(thr,node)
#else /* UNIV_DEBUG */
# define row_purge_upd_exist_or_extern(thr,node)	\
	row_purge_upd_exist_or_extern_func(node)
#endif /* UNIV_DEBUG */

/***********************************************************//**
Parses the row reference and other info in a modify undo log record.
@return TRUE if purge operation required: NOTE that then the CALLER
must unfreeze data dictionary! */
static
ibool
row_purge_parse_undo_rec(
/*=====================*/
	purge_node_t*	node,	/*!< in: row undo node */
	ibool*		updated_extern,
				/*!< out: TRUE if an externally stored field
				was updated */
	que_thr_t*	thr)	/*!< in: query thread */
{
	dict_index_t*	clust_index;
	byte*		ptr;
	trx_t*		trx;
	undo_no_t	undo_no;
	dulint		table_id;
	trx_id_t	trx_id;
	roll_ptr_t	roll_ptr;
	ulint		info_bits;
	ulint		type;

	ut_ad(node && thr);

	trx = thr_get_trx(thr);

	ptr = trx_undo_rec_get_pars(
		node->undo_rec, &type, &node->cmpl_info,
		updated_extern, &undo_no, &table_id);
	node->rec_type = type;

	if (type == TRX_UNDO_UPD_DEL_REC && !(*updated_extern)) {

		return(FALSE);
	}

	ptr = trx_undo_update_rec_get_sys_cols(ptr, &trx_id, &roll_ptr,
					       &info_bits);
	node->table = NULL;

	if (type == TRX_UNDO_UPD_EXIST_REC
	    && node->cmpl_info & UPD_NODE_NO_ORD_CHANGE
	    && !(*updated_extern)) {

		/* Purge requires no changes to indexes: we may return */

		return(FALSE);
	}

	/* Prevent DROP TABLE etc. from running when we are doing the purge
	for this row */

	row_mysql_freeze_data_dictionary(trx);

	mutex_enter(&(dict_sys->mutex));

	node->table = dict_table_get_on_id_low(table_id);

	mutex_exit(&(dict_sys->mutex));

	if (node->table == NULL) {
		/* The table has been dropped: no need to do purge */
err_exit:
		row_mysql_unfreeze_data_dictionary(trx);
		return(FALSE);
	}

	if (node->table->ibd_file_missing) {
		/* We skip purge of missing .ibd files */

		node->table = NULL;

		goto err_exit;
	}

	clust_index = dict_table_get_first_index(node->table);

	if (clust_index == NULL) {
		/* The table was corrupt in the data dictionary */

		goto err_exit;
	}

	ptr = trx_undo_rec_get_row_ref(ptr, clust_index, &(node->ref),
				       node->heap);

	ptr = trx_undo_update_rec_get_update(ptr, clust_index, type, trx_id,
					     roll_ptr, info_bits, trx,
					     node->heap, &(node->update));

	/* Read to the partial row the fields that occur in indexes */

	if (!(node->cmpl_info & UPD_NODE_NO_ORD_CHANGE)) {
		ptr = trx_undo_rec_get_partial_row(
			ptr, clust_index, &node->row,
			type == TRX_UNDO_UPD_DEL_REC,
			node->heap);
	}

	return(TRUE);
}

/***********************************************************//**
Fetches an undo log record and does the purge for the recorded operation.
If none left, or the current purge completed, returns the control to the
parent node, which is always a query thread node. */
static __attribute__((nonnull))
void
row_purge(
/*======*/
	purge_node_t*	node,	/*!< in: row purge node */
	que_thr_t*	thr)	/*!< in: query thread */
{
	ibool		updated_extern;

	ut_ad(node);
	ut_ad(thr);

	node->undo_rec = trx_purge_fetch_next_rec(&node->roll_ptr,
						  &node->reservation,
						  node->heap);
	if (!node->undo_rec) {
		/* Purge completed for this query thread */

		thr->run_node = que_node_get_parent(node);

		return;
	}

	if (node->undo_rec != &trx_purge_dummy_rec
	    && row_purge_parse_undo_rec(node, &updated_extern, thr)) {
		node->found_clust = FALSE;

		node->index = dict_table_get_next_index(
			dict_table_get_first_index(node->table));

		if (node->rec_type == TRX_UNDO_DEL_MARK_REC) {
			row_purge_del_mark(node);

		} else if (updated_extern
			   || node->rec_type == TRX_UNDO_UPD_EXIST_REC) {

			row_purge_upd_exist_or_extern(thr, node);
		}

		if (node->found_clust) {
			btr_pcur_close(&(node->pcur));
		}

		row_mysql_unfreeze_data_dictionary(thr_get_trx(thr));
	}

	/* Do some cleanup */
	trx_purge_rec_release(node->reservation);
	mem_heap_empty(node->heap);

	thr->run_node = node;
}

/***********************************************************//**
Does the purge operation for a single undo log record. This is a high-level
function used in an SQL execution graph.
@return	query thread to run next or NULL */
UNIV_INTERN
que_thr_t*
row_purge_step(
/*===========*/
	que_thr_t*	thr)	/*!< in: query thread */
{
	purge_node_t*	node;

	ut_ad(thr);

	node = thr->run_node;

	ut_ad(que_node_get_type(node) == QUE_NODE_PURGE);

	row_purge(node, thr);

	return(thr);
}
