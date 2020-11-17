#include "defines.h"
#include "tsos.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "lib.h"

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

// スレッドのレディーキュー
static struct {
	ts_thread *head;
	ts_thread *tail;
} readyque[PRIORITY_NUM];

static ts_thread *current; // カレントスレッド
static ts_thread threads[THREAD_NUM]; // TCB
static ts_handler_t handlers[SOFTVEC_TYPE_NUM]; // 割込みハンドラ

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

/* 割込みハンドラの登録 */
static int setintr(softvec_type_t type, ts_handler_t handler) {
	static void thread_intr(softvec_type_t type, unsigned long sp);

	/* 割込みを受け付けるために，ソフトウェア・割込みベクタに
	 * OSの割込み処理の入り口となる関数を登録する*
	 */

	softvec_setintr(type, thread_intr);
	handlers[type] = handler; // OS側から呼び出すハンドラを登録
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
	current = NULL;

	// 各種データの初期化
	memset(readyque, 0, sizeof(readyque));
	memset(threads, 0, sizeof(threads));
	memset(handlers, 0, sizeof(handlers));

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

