#include "defines.h"
#include "tsos.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "lib.h"
#include "memory.h"

// TCBの個数
#define THREAD_NUM 6
// 優先度の個数
#define PRIORITY_NUM 16
// スレッド名の最大長
#define THREAD_NAME_SIZE 15

// スレッドコンテキスト
typedef struct _ts_context {
	// スタックポインタ
	uint32	sp;
} ts_context;

// タスクコントロールブロック
typedef struct _ts_thread {
	struct _ts_thread *next; //レディーキューへの接続に利用するnextポインタ
	char name[THREAD_NAME_SIZE + 1]; // スレッド名
	int priority; // 優先度
	char *stack; // スレッドのスタック
	uint32 flags; // 各種フラグ
#define TS_THREAD_FLAG_READY (1<<0)

	struct { // スレッドのスタートアップに渡すパラメータ
		ts_func_t func; // スレッドのメイン関数
		int argc; // メイン関数の引数
		char **argv; // メイン関数の引数
	} init;

	struct { // システムコール用バッファ
		// システムコールの発行時に利用するパラメタ領域
		ts_syscall_type_t type; 
		ts_syscall_param_t *param;
	} syscall;

	ts_context context; // コンテキスト情報
} ts_thread;

// メッセージバッファ
typedef struct _ts_msgbuf {
	struct _ts_msgbuf * next;
	ts_thread * sender; // メッセージを送信したスレッド
	struct {
		int size;
		char * p;
	} param;
} ts_msgbuf;

// メッセージボックス
typedef struct _ts_msgbox {
	ts_thread * receiver; // 受信待ち状態のスレッド
	ts_msgbuf * head;
	ts_msgbuf * tail;
	long dummy[1];
} ts_msgbox;

// スレッドのレディーキュー
static struct {
	ts_thread *head;
	ts_thread *tail;
} readyque[PRIORITY_NUM];

static ts_thread *current; // カレントスレッド
static ts_thread threads[THREAD_NUM]; // TCB
static ts_handler_t handlers[SOFTVEC_TYPE_NUM]; // 割込みハンドラ
static ts_msgbox msgboxes[MSGBOX_ID_NUM]; // メッセージボックス

// スレッドのディスパッチ用関数
void dispatch(ts_context *context);

/* カレントスレッドをレディーキューから抜き出す */
static int getcurrent(void) {
	if (current == NULL) {
		return -1;
	}
	if (!(current->flags & TS_THREAD_FLAG_READY)) {
		return 1;
	}

	/* カレントスレッドはレディーキューの先頭にある */
	readyque[current->priority].head = current->next; // カレントスレッドの次のノードを渡している?
	if (readyque[current->priority].head == NULL) {
		readyque[current->priority].tail = NULL;
	}
	current->flags &= ~TS_THREAD_FLAG_READY;
	current->next = NULL;
	return 0;
}

/* カレントスレッドをレディーキューに繋げる */
static int putcurrent(void) {
	if (current == NULL) {
		return -1;
	}  
	if (current->flags & TS_THREAD_FLAG_READY) {
		return 1;
	}

	/* レディーキューの末尾に繋げる */
	if (readyque[current->priority].tail) {
		readyque[current->priority].tail->next = current;
	} else {
		readyque[current->priority].head = current;
	}
	readyque[current->priority].tail = current;
	current->flags |= TS_THREAD_FLAG_READY;
	return 0;
}

/* スレッドの終了 */
static void thread_end(void){
	ts_exit();
}

/* スレッドのスタートアップ */
static void thread_init(ts_thread *thp) {
	/* スレッドのメイン関数を呼び出す */
	thp->init.func(thp->init.argc, thp->init.argv);
	thread_end();
}

/* システムコールの処理 */
static ts_thread_id_t thread_run(ts_func_t func, char *name, int priority, int stacksize, int argc, char *argv[]) {
	int i;
	ts_thread *thp;
	uint32 *sp;
	extern char userstack; // リンカスクリプトで定義されるスタック領域
	static char *thread_stack = &userstack; // ユーザスタックに利用される領域

	/* 空いているタスクコントロールブロックを検索 */
	for (i = 0; i < THREAD_NUM; i++) {
		thp = &threads[i];
		if (!thp->init.func) {
			// 空いているブロックが見つかった場合
			break;
		}
	}
	if (i == THREAD_NUM) {
		return -1;
	}

	memset(thp, 0, sizeof(*thp)); // TCBをゼロクリアする
	strcpy(thp->name, name);
	thp->next	= NULL;
	thp->priority	= priority;
	thp->flags 	= 0;

	thp->init.func = func;
	thp->init.argc = argc;
	thp->init.argv = argv;

	// スタック領域を獲得
	memset(thread_stack, 0, stacksize);
	thread_stack += stacksize;

	thp->stack = thread_stack; // スタックを設定

	/* スタックの初期化 */
	sp = (uint32 *)thp->stack;
	*(--sp) = (uint32)thread_end;

	/* プログラムカウンタを設定する */
	*(--sp) = (uint32)thread_init | ((uint32)(priority ? 0 : 0xc0 ) << 24);

	*(--sp) = 0; //ER6
	*(--sp) = 0; //ER5
	*(--sp) = 0; //ER4
	*(--sp) = 0; //ER3
	*(--sp) = 0; //ER2
	*(--sp) = 0; //ER1

	/* スレッドのスタートアップ */
	*(--sp) = (uint32)thp; // ER0, 引数

	/* スレッドのコンテキストを設定 */
	thp->context.sp = (uint32)sp;

	// システムコールを呼び出したスレッドをレディーキューに戻す
	putcurrent();

	// 新規作成したスレッドをレディーキューに接続する
	current = thp;
	putcurrent();

	return (ts_thread_id_t)current; // 新規作成したスレッドidを返す
}

/* システムコールの処理 */
static int thread_exit(void) {
	puts(current->name);
	puts(" EXIT.\n");
	memset(current, 0, sizeof(*current));
	return 0;
}

/* システムコールの処理:スレッドの実行権放棄 */
static int thread_wait(void) {
	putcurrent();
	return 0;
}

/* システムコールの処理:スレッドのスリープ */
static int thread_sleep(void) {
	return 0;
}

/* システムコールの処理:スレッドのウェイクアップ */
static int thread_wakeup(ts_thread_id_t id) {
	// ウェイクアップを呼び出したスレッドをレディーキューに戻す
	putcurrent();

	// 指定されたスレッドをレディーキューに戻す
	current = (ts_thread *)id;
	putcurrent();

	return 0;
} 

/* システムコールの処理:スレッドID取得 */
static ts_thread_id_t thread_getid(void) {
	putcurrent();
	return (ts_thread_id_t)current;
}

/* システムコールの処理:ts_chpri */
static int thread_chpri(int priority) {
	int old = current->priority;

	if (priority >= 0) {
		current->priority = priority; // 優先度変更
	}
	putcurrent();
	return old;
}

/* 動的メモリ確保 */
static void * thread_tmalloc(int size) {
	putcurrent();
	return tsmem_alloc(size);
}

/* メモリ解放 */
static int thread_tmfree(char * p) {
	tsmem_free(p);
	putcurrent();
	return 0;
}

// メッセージの送信処理
static void sendmsg(ts_msgbox * mboxp, ts_thread * thp, int size, char * p) {
	ts_msgbuf * mp;

	// メッセージバッファの作成
	mp = (ts_msgbuf *)tsmem_alloc(sizeof(*mp));
	if (mp == NULL) {
		ts_sysdown();
	}

	// 各種パラメータの設定
	mp->next 	= NULL;
	mp->sender	= thp; // 送信元スレッドのポインタ
	mp->param.size	= size;
	mp->param.p	= p;

	if (mboxp->tail) {
		mboxp->tail->next = mp;
	} else {
		mboxp->head = mp;
	}
	mboxp->tail = mp;
}

// メッセージの受信処理
static void recvmsg(ts_msgbox * mboxp) {
	ts_msgbuf * mp;
	ts_syscall_param_t * p;

	// メッセージボックスの先頭にあるメッセージを抜き出す
	mp = mboxp->head;
	mboxp->head = mp->next;
	if (mboxp->head == NULL) {
		mboxp->tail = NULL;
	}
	mp->next = NULL;

	// メッセージを受信するスレッドに返す値を設定する
	p = mboxp->receiver->syscall.param;
	p->un.recv.ret = (ts_thread_id_t)mp->sender;
	if (p->un.recv.sizep) {
		*(p->un.recv.sizep) = mp->param.size;
	}
	if (p->un.recv.pp) {
		*(p->un.recv.pp) = mp->param.p;
	}

	// 受信待ちスレッドはいなくなったのでNULLに戻す
	mboxp->receiver = NULL;

	// メッセージバッファの解放
	tsmem_free(mp);
}
/* メッセージ送信 */
static int thread_send(ts_msgbox_id_t id, int size, char * p) {
	ts_msgbox * mboxp = &msgboxes[id];

	putcurrent();
	sendmsg(mboxp, current, size, p);

	// 受信待ちスレッドが存在している場合には受信処理を行う
	if (mboxp->receiver) {
		current = mboxp->receiver; // 受信待ちスレッド
		recvmsg(mboxp);			//メッセージの受信処理
		putcurrent();
	}
	return size;
}

/* メッセージ受信 */
static ts_thread_id_t thread_recv(ts_msgbox_id_t id, int * sizep, char **p) {
	ts_msgbox * mboxp = &msgboxes[id];

	// 他のスレッドが受信待ちしている
	if (mboxp->receiver) {
		ts_sysdown();
	}

	mboxp->receiver = current; // 受信待ちスレッドに設定

	// メッセージボックスにメッセージがないためスリープさせる
	if (mboxp->head == NULL) {
		return -1;
	}

	recvmsg(mboxp);
	putcurrent();

	return current->syscall.param->un.recv.ret;
}


/* 割込みハンドラの登録 */
static int thread_setintr(softvec_type_t type, ts_handler_t handler) {
	static void thread_intr(softvec_type_t type, unsigned long sp);

	/* 割込みを受け付けるために，ソフトウェア・割込みベクタに
	 * OSの割込み処理の入り口となる関数を登録する*
	 */

	softvec_setintr(type, thread_intr);
	handlers[type] = handler; // OS側から呼び出すハンドラを登録
	putcurrent();
	return 0;
}

static void call_functions(ts_syscall_type_t type, ts_syscall_param_t *p) {
	/* システムコールの時効中にcurrentが書き換わるの注意 */
	switch (type) {
		case TS_SYSCALL_TYPE_RUN: // ts_run
			p->un.run.ret = thread_run(	p->un.run.func, p->un.run.name,
							p->un.run.priority, p->un.run.stacksize,
							p->un.run.argc, p->un.run.argv);
			break;

		case TS_SYSCALL_TYPE_EXIT: // ts_exit
			thread_exit();
			break;
		case TS_SYSCALL_TYPE_WAIT: // ts_wait()
			p->un.wait.ret = thread_wait();
			break;
		case TS_SYSCALL_TYPE_SLEEP: // ts_sleep()
			p->un.sleep.ret = thread_sleep();
			break;
		case TS_SYSCALL_TYPE_WAKEUP: // ts_wakeup();
			p->un.wakeup.ret = thread_wakeup(p->un.wakeup.id);
			break;
		case TS_SYSCALL_TYPE_GETID: // ts_getid()
			p->un.getid.ret = thread_getid();
			break;
		case TS_SYSCALL_TYPE_CHPRI: // ts_chpri()
			p->un.chpri.ret = thread_chpri(p->un.chpri.priority);
			break;
		case TS_SYSCALL_TYPE_TMALLOC: // ts_tmalloc()
			p->un.tmalloc.ret = thread_tmalloc(p->un.tmalloc.size);
			break;
		case TS_SYSCALL_TYPE_TMFREE: // ts_tmfree()
			p->un.tmfree.ret = thread_tmfree(p->un.tmfree.p);
			break;
		case TS_SYSCALL_TYPE_SEND: // ts_send()
			p->un.send.ret = thread_send(p->un.send.id, p->un.send.size, p->un.send.p);
			break;
		case TS_SYSCALL_TYPE_RECV: // ts_recv()
			p->un.recv.ret = thread_recv(p->un.recv.id, p->un.recv.sizep, p->un.recv.pp);
			break;
		default:
			break;
	}
}

/* システムコールの処理 */
static void syscall_proc(ts_syscall_type_t type, ts_syscall_param_t *p) {
	getcurrent();
	call_functions(type, p);
}

/* スレッドのスケジューリング */
static void schedule(void) {
	int i;

	/*
	 * 優先順位の高いスレッドから見ていく
	 * */
	for (i = 0 ; i < PRIORITY_NUM; i++) {
		if (readyque[i].head) {
			break;
		}
	}
	if (i == PRIORITY_NUM) {
		ts_sysdown();
	}
	current = readyque[i].head; // カレントスレッドに設定する
}

static void syscall_intr(void) {
	syscall_proc(current->syscall.type, current->syscall.param);
}

static void softerr_intr(void) {
	puts(current->name);
	puts(" DOWN\n");
	getcurrent();
	thread_exit();
}
	
static void thread_intr(softvec_type_t type, unsigned long sp) {
	// カレントスレッドのコンテキストを保存する
	current->context.sp = sp;

	/*
	 * 割込みごとの処理を実行する
	 * SOFTVEC_TYPE_SYSCALL, SOFTVEC_TYPE_SOFTERRの場合は
	 * syscall_intr(), softerr_intr()がハンドラに登録されているので
	 * それらが実行される
	 * */

	if (handlers[type]) {
		handlers[type]();
	}

	schedule();

	/*
	 * スレッドのディスパッチ
	 * 関数本体はstartup.s
	 * */
	dispatch(&current->context);
}

/* 初期スレッドを起動し, OSの動作を開始する */
void ts_start(ts_func_t func, char *name, int priority, int stacksize, int argc, char *argv[]) {
	tsmem_init();
	current = NULL;

	// 各種データの初期化
	memset(readyque, 0, sizeof(readyque));
	memset(threads, 0, sizeof(threads));
	memset(handlers, 0, sizeof(handlers));
	memset(msgboxes, 0, sizeof(msgboxes));

	// 割込みハンドラの登録
	setintr(SOFTVEC_TYPE_SYSCALL, syscall_intr); // システムコール割込み
	setintr(SOFTVEC_TYPE_SOFTERR, softerr_intr); // エラー発生時処理

	// 初期スレッドはシステムコールの発行が不可能なので直接関数を呼び出す
	current = (ts_thread *)thread_run(func, name, priority, stacksize, argc, argv);

	// 最初のスレッド起動
	dispatch(&current->context);
}

/* OS内部で致命的なエラーが発生した場合 */
void ts_sysdown(void) {
	puts("system error!\n");
	while(1) {
		;
	}
}

/* システムコール呼び出し用ライブラリ関数 */
void ts_syscall(ts_syscall_type_t type, ts_syscall_param_t *param) {
	current->syscall.type = type; // システムコール番号の設定
	current->syscall.param = param; // パラメータの設定

	asm volatile ("trapa #0"); // トラップ割込発生，トラップ命令により割込みを発生させる
}

