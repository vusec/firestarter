/**********************************************************************
Data dictionary memory object creation

(c) 1996 Innobase Oy

Created 1/8/1996 Heikki Tuuri
***********************************************************************/

#include "dict0mem.h"

#ifdef UNIV_NONINL
#include "dict0mem.ic"
#endif

#include "rem0rec.h"
#include "data0type.h"
#include "mach0data.h"
#include "dict0dict.h"
#include "que0que.h"
#include "pars0pars.h"
#include "lock0lock.h"

#define	DICT_HEAP_SIZE		100	/* initial memory heap size when
					creating a table or index object */

/**************************************************************************
Creates a table memory object. */

dict_table_t*
dict_mem_table_create(
/*==================*/
				/* out, own: table object */
	const char*	name,	/* in: table name */
	ulint		space,	/* in: space where the clustered index of
				the table is placed; this parameter is
				ignored if the table is made a member of
				a cluster */
	ulint		n_cols,	/* in: number of columns */
	ulint		flags)	/* in: table flags */
{
	dict_table_t*	table;
	mem_heap_t*	heap;

	ut_ad(name);
	ut_ad(!(flags & ~DICT_TF_COMPACT));

	heap = mem_heap_create(DICT_HEAP_SIZE);

	table = mem_heap_alloc(heap, sizeof(dict_table_t));

	table->heap = heap;

	table->flags = (unsigned int) flags;
	table->name = mem_heap_strdup(heap, name);
	table->dir_path_of_temp_table = NULL;
	table->space = (unsigned int) space;
	table->ibd_file_missing = FALSE;
	table->tablespace_discarded = FALSE;
	table->n_def = 0;
	table->n_cols = (unsigned int) (n_cols + DATA_N_SYS_COLS);

	table->n_mysql_handles_opened = 0;
	table->n_foreign_key_checks_running = 0;

	table->cached = FALSE;

	table->cols = mem_heap_alloc(heap, (n_cols + DATA_N_SYS_COLS)
				     * sizeof(dict_col_t));
	table->col_names = NULL;
	UT_LIST_INIT(table->indexes);

	table->auto_inc_lock = mem_heap_alloc(heap, lock_get_size());

	table->query_cache_inv_trx_id = ut_dulint_zero;

	UT_LIST_INIT(table->locks);
	UT_LIST_INIT(table->foreign_list);
	UT_LIST_INIT(table->referenced_list);

#ifdef UNIV_DEBUG
	table->does_not_fit_in_memory = FALSE;
#endif /* UNIV_DEBUG */

	table->stat_initialized = FALSE;

	table->stat_modified_counter = 0;

	table->big_rows = 0;

	table->fk_max_recusive_level = 0;

	mutex_create(&table->autoinc_mutex, SYNC_DICT_AUTOINC_MUTEX);

	table->autoinc = 0;

	/* The number of transactions that are either waiting on the
	AUTOINC lock or have been granted the lock. */
	table->n_waiting_or_granted_auto_inc_locks = 0;

#ifdef UNIV_DEBUG
	table->magic_n = DICT_TABLE_MAGIC_N;
#endif /* UNIV_DEBUG */
	return(table);
}

/********************************************************************
Free a table memory object. */

void
dict_mem_table_free(
/*================*/
	dict_table_t*	table)		/* in: table */
{
	ut_ad(table);
	ut_ad(table->magic_n == DICT_TABLE_MAGIC_N);

	mutex_free(&(table->autoinc_mutex));
	mem_heap_free(table->heap);
}

/********************************************************************
Append 'name' to 'col_names' (@see dict_table_t::col_names). */
static
const char*
dict_add_col_name(
/*==============*/
					/* out: new column names array */
	const char*	col_names,	/* in: existing column names, or
					NULL */
	ulint		cols,		/* in: number of existing columns */
	const char*	name,		/* in: new column name */
	mem_heap_t*	heap)		/* in: heap */
{
	ulint	old_len;
	ulint	new_len;
	ulint	total_len;
	char*	res;

	ut_ad(!cols == !col_names);

	/* Find out length of existing array. */
	if (col_names) {
		const char*	s = col_names;
		ulint		i;

		for (i = 0; i < cols; i++) {
			s += strlen(s) + 1;
		}

		old_len = s - col_names;
	} else {
		old_len = 0;
	}

	new_len = strlen(name) + 1;
	total_len = old_len + new_len;

	res = mem_heap_alloc(heap, total_len);

	if (old_len > 0) {
		memcpy(res, col_names, old_len);
	}

	memcpy(res + old_len, name, new_len);

	return(res);
}

/**************************************************************************
Adds a column definition to a table. */

void
dict_mem_table_add_col(
/*===================*/
	dict_table_t*	table,	/* in: table */
	mem_heap_t*	heap,	/* in: temporary memory heap, or NULL */
	const char*	name,	/* in: column name, or NULL */
	ulint		mtype,	/* in: main datatype */
	ulint		prtype,	/* in: precise type */
	ulint		len)	/* in: precision */
{
	dict_col_t*	col;
	ulint		mbminlen;
	ulint		mbmaxlen;
	ulint		i;

	ut_ad(table);
	ut_ad(table->magic_n == DICT_TABLE_MAGIC_N);
	ut_ad(!heap == !name);

	i = table->n_def++;

	if (name) {
		if (UNIV_UNLIKELY(table->n_def == table->n_cols)) {
			heap = table->heap;
		}
		if (UNIV_LIKELY(i) && UNIV_UNLIKELY(!table->col_names)) {
			/* All preceding column names are empty. */
			char* s = mem_heap_alloc(heap, table->n_def);
			memset(s, 0, table->n_def);
			table->col_names = s;
		}

		table->col_names = dict_add_col_name(table->col_names,
						     i, name, heap);
	}

	col = (dict_col_t*) dict_table_get_nth_col(table, i);

	col->ind = (unsigned int) i;
	col->ord_part = 0;

	col->mtype = (unsigned int) mtype;
	col->prtype = (unsigned int) prtype;
	col->len = (unsigned int) len;

	dtype_get_mblen(mtype, prtype, &mbminlen, &mbmaxlen);

	col->mbminlen = (unsigned int) mbminlen;
	col->mbmaxlen = (unsigned int) mbmaxlen;
}

/**************************************************************************
Creates an index memory object. */

dict_index_t*
dict_mem_index_create(
/*==================*/
					/* out, own: index object */
	const char*	table_name,	/* in: table name */
	const char*	index_name,	/* in: index name */
	ulint		space,		/* in: space where the index tree is
					placed, ignored if the index is of
					the clustered type */
	ulint		type,		/* in: DICT_UNIQUE,
					DICT_CLUSTERED, ... ORed */
	ulint		n_fields)	/* in: number of fields */
{
	dict_index_t*	index;
	mem_heap_t*	heap;

	ut_ad(table_name && index_name);

	heap = mem_heap_create(DICT_HEAP_SIZE);
	index = mem_heap_alloc(heap, sizeof(dict_index_t));

	index->heap = heap;

	index->type = type;
	index->space = (unsigned int) space;
	index->page = 0;
	index->name = mem_heap_strdup(heap, index_name);
	index->table_name = table_name;
	index->table = NULL;
	index->n_def = index->n_nullable = 0;
	index->n_fields = (unsigned int) n_fields;
	index->fields = mem_heap_alloc(heap, 1 + n_fields
				       * sizeof(dict_field_t));
	/* The '1 +' above prevents allocation
	of an empty mem block */
	index->stat_n_diff_key_vals = NULL;

	index->cached = FALSE;
	memset(&index->lock, 0, sizeof index->lock);
#ifdef UNIV_DEBUG
	index->magic_n = DICT_INDEX_MAGIC_N;
#endif /* UNIV_DEBUG */
	return(index);
}

/**************************************************************************
Creates and initializes a foreign constraint memory object. */

dict_foreign_t*
dict_mem_foreign_create(void)
/*=========================*/
				/* out, own: foreign constraint struct */
{
	dict_foreign_t*	foreign;
	mem_heap_t*	heap;

	heap = mem_heap_create(100);

	foreign = mem_heap_alloc(heap, sizeof(dict_foreign_t));

	foreign->heap = heap;

	foreign->id = NULL;

	foreign->type = 0;
	foreign->foreign_table_name = NULL;
	foreign->foreign_table = NULL;
	foreign->foreign_col_names = NULL;

	foreign->referenced_table_name = NULL;
	foreign->referenced_table = NULL;
	foreign->referenced_col_names = NULL;

	foreign->n_fields = 0;

	foreign->foreign_index = NULL;
	foreign->referenced_index = NULL;

	return(foreign);
}

/**************************************************************************
Adds a field definition to an index. NOTE: does not take a copy
of the column name if the field is a column. The memory occupied
by the column name may be released only after publishing the index. */

void
dict_mem_index_add_field(
/*=====================*/
	dict_index_t*	index,		/* in: index */
	const char*	name,		/* in: column name */
	ulint		prefix_len)	/* in: 0 or the column prefix length
					in a MySQL index like
					INDEX (textcol(25)) */
{
	dict_field_t*	field;

	ut_ad(index);
	ut_ad(index->magic_n == DICT_INDEX_MAGIC_N);

	index->n_def++;

	field = dict_index_get_nth_field(index, index->n_def - 1);

	field->name = name;
	field->prefix_len = (unsigned int) prefix_len;
}

/**************************************************************************
Frees an index memory object. */

void
dict_mem_index_free(
/*================*/
	dict_index_t*	index)	/* in: index */
{
	ut_ad(index);
	ut_ad(index->magic_n == DICT_INDEX_MAGIC_N);

	mem_heap_free(index->heap);
}
