/*
 *  TOPPERS Automotive Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems
 *      Automotive Kernel
 * 
 *  Copyright (C) 2000-2003 by Embedded and Real-Time Systems Laboratory
 *                              Toyohashi Univ. of Technology, JAPAN
 *  Copyright (C) 2004 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *  Copyright (C) 2004-2006 by Witz Corporation, JAPAN
 * 
 *  上記著作権者は，以下の (1)〜(4) の条件か，Free Software Foundation 
 *  によって公表されている GNU General Public License の Version 2 に記
 *  述されている条件を満たす場合に限り，本ソフトウェア（本ソフトウェア
 *  を改変したものを含む．以下同じ）を使用・複製・改変・再配布（以下，
 *  利用と呼ぶ）することを無償で許諾する．
 *  (1) 本ソフトウェアをソースコードの形で利用する場合には，上記の著作
 *      権表示，この利用条件および下記の無保証規定が，そのままの形でソー
 *      スコード中に含まれていること．
 *  (2) 本ソフトウェアを，ライブラリ形式など，他のソフトウェア開発に使
 *      用できる形で再配布する場合には，再配布に伴うドキュメント（利用
 *      者マニュアルなど）に，上記の著作権表示，この利用条件および下記
 *      の無保証規定を掲載すること．
 *  (3) 本ソフトウェアを，機器に組み込むなど，他のソフトウェア開発に使
 *      用できない形で再配布する場合には，次のいずれかの条件を満たすこ
 *      と．
 *    (a) 再配布に伴うドキュメント（利用者マニュアルなど）に，上記の著
 *        作権表示，この利用条件および下記の無保証規定を掲載すること．
 *    (b) 再配布の形態を，別に定める方法によって，TOPPERSプロジェクトに
 *        報告すること．
 *  (4) 本ソフトウェアの利用により直接的または間接的に生じるいかなる損
 *      害からも，上記著作権者およびTOPPERSプロジェクトを免責すること．
 * 
 *  本ソフトウェアは，無保証で提供されているものである．上記著作権者お
 *  よびTOPPERSプロジェクトは，本ソフトウェアに関して，その適用可能性も
 *  含めて，いかなる保証も行わない．また，本ソフトウェアの利用により直
 *  接的または間接的に生じたいかなる損害に関しても，その責任を負わない．
 * 
 */

/*
 *	タスク管理モジュール(ECC2)
 */

#include "osek_kernel.h"
#include "check.h"
#include "task.h"
#include "resource.h"
#include "cpu_context.h"

#include "alarm.h"
#include "deadline.h"
#include "resource_info.h"
#include "rmcl_resource_manage.h"
#include "rmcl.h"
#include "lftime.h"
// #include "added_field.h"
extern int flg;
TaskType crtsk;

/*
 *  スタティック関数のプロトタイプ宣言
 */
void	ready_insert_first(Priority pri, TaskType tskid);
void	ready_insert_last(Priority pri, TaskType tskid);
TaskType	ready_delete_first(Priority pri);
Priority	bitmap_search(UINT16 bitmap);

/*
 *  レディキュー
 *
 *  レディキューには実行可能状態のタスクのみをつなぐこととし，実行状態
 *  のタスクはレディキューからは外す．厳密には，schedtsk をレディキュー
 *  から外す（割込み処理中には，runtsk がレディキューから外れているとは
 *  限らない）．
 *  レディキューは，優先度毎の単方向リンクキューで構成する．レディキュー
 *  の先頭のタスクIDを ready_queue_first に，末尾のタスクID を
 *  ready_queue_last に保持する．レディキューが空の時は，ready_queue_first 
 *  を TSKID_NULL とし，ready_queue_last は不定とする．
 */
TaskType ready_queue_first[TNUM_PRIORITY];
TaskType ready_queue_last[TNUM_PRIORITY];

/*
 *  レディキューの先頭への挿入
 */
void
ready_insert_first(Priority pri, TaskType tskid)
{
	TaskType	first;

	assert(pri <= TPRI_MAXTASK);
	first = ready_queue_first[pri];
	ready_queue_first[pri] = tskid;
	tcb_next[tskid] = first;
	if (first == TSKID_NULL) {
		ready_queue_last[pri] = tskid;
	}
}

/*
 *  レディキューの末尾への挿入
 */
void
ready_insert_last(Priority pri, TaskType tskid)
{
	assert(pri <= TPRI_MAXTASK);
	if (ready_queue_first[pri] == TSKID_NULL) {
		ready_queue_first[pri] = tskid;
	}
	else {
		tcb_next[ready_queue_last[pri]] = tskid;
	}
	ready_queue_last[pri] = tskid;
	tcb_next[tskid] = TSKID_NULL;
}

/*
 *  レディキューの先頭タスクの削除
 */
TaskType
ready_delete_first(Priority pri)
{
	TaskType	first;

	first = ready_queue_first[pri];
	assert(first != TSKID_NULL);
	ready_queue_first[pri] = tcb_next[first];
	return(first);
}

/*
 *  ビットマップサーチ関数
 *
 *  bitmap 内の 1 のビットの内，最も上記（左）のものをサーチし，そのビ
 *  ット番号を返す．ビット番号は，最下位ビットを 0 とする．bitmap に 0
 *  を指定してはならない．この関数では，優先度が16段階以下であることを
 *  仮定し，bitmap の下位16ビットのみをサーチする．
 *  ビットサーチ命令を持つプロセッサでは，ビットサーチ命令を使うように
 *  書き直した方が効率が良いだろう．このような場合には，cpu_insn.h で
 *  ビットサーチ命令を使った bitmap_search を定義し，CPU_BITMAP_SEARCH 
 *  をマクロ定義すればよい．また，ビットサーチ命令のサーチ方向が逆など
 *  の理由で優先度とビットとの対応を変更したい場合には，PRIMAP_BIT を
 *  マクロ定義すればよい．
 *  また，標準ライブラリに ffs があるなら，次のように定義して標準ライ
 *  ブラリを使った方が効率が良い可能性もある．
 *	#define PRIMAP_BIT(pri)	(0x8000u >> (pri))
 *	#define	bitmap_search(bitmap) (16 - ffs(bitmap))
 *  μITRON仕様とは優先度の意味が逆のため，サーチする方向が逆になって
 *  いる．bitmap_search を置き換える場合には，注意が必要である．
 */
#ifndef PRIMAP_BIT
#define	PRIMAP_BIT(pri)		(1u << (pri))
#endif /* PRIMAP_BIT */

#ifndef CPU_BITMAP_SEARCH

Priority
bitmap_search(UINT16 bitmap)
{
	static const UINT8 search_table[] = { 0, 1, 1, 2, 2, 2, 2,
						3, 3, 3, 3, 3, 3, 3, 3 };
	Priority	pri = 0;

	assert((bitmap & ((UINT16) 0xffffu)) != 0);
	if ((bitmap & ((UINT16) 0xff00u)) != 0) {
		bitmap >>= 8;
		pri += 8;
	}
	if ((bitmap & ((UINT16) 0xf0)) != 0) {
		bitmap >>= 4;
		pri += 4;
	}
	return(pri + (search_table[(bitmap & ((UINT16) 0x0f)) - 1]));
}

#endif /* CPU_BITMAP_SEARCH */

/*
 *  実行状態のタスク
 */
TaskType	runtsk;

/*
 *  最高優先順位タスク
 */
TaskType	schedtsk;

/*
 *  レディキュー中の最高優先度
 */
Priority	nextpri;

/*
 *  レディキューに入っているタスクの優先度のビットマップ
 *
 *  レディキューが空の時（実行可能状態のタスクが無い時）は 0 にする．
 */
UINT16	ready_primap;

/*
 *  タスク管理モジュールの初期化
 */
void
task_initialize(void)
{
	Priority	pri;
	TaskType	tskid;
	rescb_used = 0;
  crtsk = TSKID_NULL;
   for(tskid = 0; tskid < tnum_task; tskid++) {
    tcb_lftexetm[tskid] = (TickType)0x0000;
    tcb_exebgntm[tskid] = (TickType)0x0000;
  }
  for(tskid = 0; tskid < tnum_task; tskid++) {
    tcb_deadline[tskid] = (TickType)0x0000;
  }

	runtsk = TSKID_NULL;
	schedtsk = TSKID_NULL;
	for (pri = 0; pri < TNUM_PRIORITY; pri++) {
		ready_queue_first[pri] = TSKID_NULL;
	}
	nextpri = TPRI_MINTASK;
	ready_primap = 0u;

	for (tskid = 0; tskid < tnum_task; tskid++) {
		tcb_actcnt[tskid] = 0;
		if ((tinib_autoact[tskid] & appmode) != APPMODE_NONE) {
			(void)make_active(tskid);
			tcb_tstat[tskid] = TS_RUNNABLE;
		}
		else {
			tcb_tstat[tskid] = TS_DORMANT;
		}
	}
}

/*
 *  タスクの起動
 *
 *  TerminateTask や ChainTask の中で，自タスクに対して make_active を
 *  呼ぶ場合があるので注意する．
 */
BOOL
make_active(TaskType tskid)
{
  update_deadline(tskid);

	tcb_curpri[tskid] = tinib_inipri[tskid];
	if (tskid < tnum_exttask) {
		tcb_curevt[tskid] = EVTMASK_NONE;
		tcb_waievt[tskid] = EVTMASK_NONE;
	}
	tcb_lastres[tskid] = RESID_NULL;
	activate_context(tskid);
	return(make_runnable(tskid));
}

/*
 *  実行できる状態への移行
 */
BOOL
make_runnable(TaskType tskid)
{
  tcb_lftexetm[tskid] = tinib_wstexetm[tskid];

  return( make_runnable_rmcl(tskid) );
}

/*
 *  最高優先順位タスクのサーチ
 */
void
search_schedtsk(void)
{
  if(schedtsk != TSKID_NULL) {
    TickType exetm = diff_tick(cntcb_curval[alminib_cntid[tinib_almid[schedtsk]]], tcb_exebgntm[schedtsk],  cntinib_maxval2[alminib_cntid[tinib_almid[schedtsk]]]);
    if(exetm > tcb_lftexetm[schedtsk]) {
      tcb_lftexetm[schedtsk] = 0;
    } else { 
      tcb_lftexetm[schedtsk] -= exetm;
    }
  }

  search_schedtsk_rmcl();

  if(schedtsk != TSKID_NULL) {
    tcb_exebgntm[schedtsk] = cntcb_curval[alminib_cntid[  tinib_almid[schedtsk]]];
  }
}

/*
 *  タスクのプリエンプト
 */
void
preempt(void)
{
  if(nextpri < tcb_curpri[schedtsk]) {
    nextpri = tcb_curpri[schedtsk];
  }

	assert(runtsk == schedtsk);
	ready_insert_first(tcb_curpri[schedtsk], schedtsk);
	ready_primap |= PRIMAP_BIT(tcb_curpri[schedtsk]);
	search_schedtsk();
}
/********以下カスタマイズのための関数*************/
Inline TickType
add_tick(TickType almval, TickType incr, TickType maxval2)
{
  if (incr <= (maxval2 - almval)) {
    return(almval + incr);
  }
  else {
    return(almval + incr - (maxval2 + 1));
  }
}
/*
 *  タスクのデッドラインを更新する関数
 */
void
update_deadline(TaskType tskid)
{
  AlarmType almid = tinib_almid[tskid];
  tcb_deadline[tskid] = add_tick(almcb_almval[almid], tinib_deadline[tskid], cntinib_maxval2[alminib_cntid[tinib_almid[tskid]]]);
}
BOOL make_runnable_rmcl(TaskType tskid) {
  Priority pri;
  TaskType prevtsk = schedtsk;
  tcb_tstat[tskid] = TS_RUNNABLE;
  pri = tcb_curpri[tskid];
  if(schedtsk == TSKID_NULL) {
    schedtsk = tskid;
    return(TRUE);
  }
  ready_insert_last(pri, tskid);
  ready_primap |= PRIMAP_BIT(pri);
  if (nextpri < pri) {
    nextpri = pri;
  }
  if( crtsk == TSKID_NULL ) {
    preempt();
  }
  return prevtsk == schedtsk ? FALSE : TRUE;
}

void search_schedtsk_rmcl(void) {
  if( search_criticaltsk() ) {
    if( rescb_used > 0 ) {
      manage_resource();
    }
    schedtsk = ready_delete_first(TPRI_CRITICALTASK);
    if (ready_queue_first[TPRI_CRITICALTASK] == TSKID_NULL) {
      ready_primap &= ~PRIMAP_BIT(TPRI_CRITICALTASK);
      nextpri = (ready_primap == ((UINT16) 0)) ?
        TPRI_MINTASK : bitmap_search(ready_primap);
    }
  } else {
    if (ready_primap == ((UINT16) 0)) {
      schedtsk = TSKID_NULL;
    } else {
      schedtsk = ready_delete_first(nextpri);
      if (ready_queue_first[nextpri] == TSKID_NULL) {
        ready_primap &= ~PRIMAP_BIT(nextpri);
        nextpri = (ready_primap == ((UINT16) 0)) ?
          TPRI_MINTASK : bitmap_search(ready_primap);
      }
    }    
  }
}

BOOL is_critical(TaskType tskid) {
  return (tcb_lftexetm[ready_queue_first[nextpri]] + tcb_lftexetm[tskid] > tcb_deadline[tskid] &&
          tcb_lftexetm[ready_queue_first[nextpri]] + tcb_lftexetm[tskid] <= tcb_deadline[ready_queue_first[nextpri]]);
}

BOOL search_criticaltsk() {
  Priority pri;
  TaskType tsk;
  TaskType prev;
  for(pri = nextpri; pri != TPRI_MINTASK; --pri) {
    for(tsk = ready_queue_first[pri], prev = TSKID_NULL; tsk != TSKID_NULL; tsk = tcb_next[tsk]) {
      if(tsk == ready_queue_first[nextpri]) {
        prev = tsk;
        continue;
      }
      if( is_critical(tsk) ) {
      // if( tsk == (TaskType)0 ) {
      // if( tsk == (TaskType)1 ) {
      // if( tsk == (TaskType)2 ) {
      // if(0) {
        // #ifdef debug
        flg = 0; //デバッグ用
        // #endif

        if(prev == TSKID_NULL) {
          ready_queue_first[pri] = tcb_next[tsk];
          if(ready_queue_first[pri] == TSKID_NULL) {
            ready_primap &= ~PRIMAP_BIT(pri);
            nextpri = (ready_primap == ((UINT16)0)) ? TPRI_MINTASK : bitmap_search(ready_primap);
          }
        } else {
          tcb_next[prev] = tcb_next[tsk];
          if(tcb_next[tsk] == TSKID_NULL) {
            ready_queue_last[pri] = TSKID_NULL;
          }
        }
        tcb_curpri[tsk] = TPRI_CRITICALTASK;
        ready_insert_first(TPRI_CRITICALTASK, tsk);
        ready_primap |= PRIMAP_BIT(TPRI_CRITICALTASK);
        nextpri = tcb_curpri[schedtsk];
        crtsk = tsk;
        return TRUE;  //クリティカルタスクがある場合
      }
      prev = tsk;
    }
  }
  return FALSE; //クリティカルタスクがない場合
}

void
manage_resource()
{
  static const UINT16 search_table[] = { 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
                       0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000 };
  UINT16 bitmap = (UINT16)0x0000;
  UINT8 arr = (UINT8)0x00;
  TaskType usingtsk = TSKID_NULL;
  ResourceType resid;
  for( arr = 0; arr < res_arr; arr++) {
    bitmap = tinib_resource[crtsk][arr] & rescb_bitmap[arr];
    if( bitmap != (UINT16)0 ) {
      for( resid = 0; resid < 16; resid++ ){
        if( bitmap & search_table[resid] ) {
          usingtsk = rescb_usingtask[arr*16 + resid];
          if( usingtsk != crtsk ) {
            ready_insert_first(TPRI_CRITICALTASK, ready_delete_first(tcb_curpri[usingtsk]));
            tcb_curpri[usingtsk] = TPRI_CRITICALTASK;
            ready_primap |= PRIMAP_BIT(TPRI_CRITICALTASK);
            nextpri = TPRI_CRITICALTASK;
          }
        }
      }
    }
  }
}

Inline TickType
diff_tick(TickType val1, TickType val2, TickType maxval2)
{
  if (val1 >= val2) {
    return(val1 - val2);
  }
  else {
    return(val1 - val2 + (maxval2 + 1));
  }
}
