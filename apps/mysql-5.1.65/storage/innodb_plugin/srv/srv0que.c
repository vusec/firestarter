/*****************************************************************************

Copyright (c) 1996, 2009, Innobase Oy. All Rights Reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307 USA

*****************************************************************************/

/**************************************************//**
@file srv/srv0que.c
Server query execution

Created 6/5/1996 Heikki Tuuri
*******************************************************/

#include "srv0que.h"

#include "srv0srv.h"
#include "sync0sync.h"
#include "os0thread.h"
#include "usr0sess.h"
#include "que0que.h"

/**********************************************************************//**
Enqueues a task to server task queue and releases a worker thread, if there
is a suspended one. */
UNIV_INTERN
void
srv_que_task_enqueue_low(
/*=====================*/
	que_thr_t*	thr)	/*!< in: query thread */
{
	ut_ad(thr);
	ut_ad(mutex_own(&kernel_mutex));

	UT_LIST_ADD_LAST(queue, srv_sys->tasks, thr);

	srv_release_threads(SRV_WORKER, 1);
}
