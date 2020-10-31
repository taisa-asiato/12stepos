#include "defines.h"
#include "tsos.h"
#include "intr.h"
#include "interrupt.h"
#include "serial.h"
#include "lib.h"

// TCBの個数
#define THREAD_NUM 6
// スレッド名の最大長
#define THREAD_NAME_SIZE 15

// スレッドコンテキスト
typedef struct _ts_context {
	// スタックポインタ
	uint32	sp;
} ts_context;


// タスクコントロールブロック
typedef struct _ts_thread {
	struct _ts_thread * next; //レディーキューへの接続に利用するnextポインタ
	char name[THREAD_NAME_SIZE + 1]; // スレッド名
	char * stack; // スレッドのスタック

	struct { // スレッドのスタートアップに渡すパラメータ
		ts_func_t func; // スレッドのメイン関数
		int argc; // メイン関数の引数
		char ** argv; // メイン関数の引数
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
	ts_thread * head;
	ts_thread * tail;
} readyque;

static ts_thread * current; // カレントスレッド
static ts_thread threads[THREAD_NUM]; // TCB
static ts_handler_t handlers[SOFTVEC_TYPE_NUM]; // 割込みハンドラ

// スレッドのディスパッチ用関数
void dispatch(ts_context * context);

/* カレントスレッドをレディーキューから抜き出す */
static int getcurrent(void) {
	if (current == NULL) {
		return -1;
	}

	/* カレントスレッドはレディーキューの先頭にある */
	readyque.head = current->next; // カレントスレッドの次のノードを渡している?
	if (readyque.head == NULL) {
		readyque.tail = NULL;
	}

	current->next = NULL;
	return 0;
}

/* カレントスレッドをレディーキューに繋げる */
static int putcurrent(void) {
	if (current == NULL) {
		return -1;
	}  

	/* レディーキューの末尾に繋げる */
	if (readyque.tail) {
		readyque.tail->next = current;
	} else {
		readyque.head = current;
	}
	readyque.tail = current;
	return 0;
}

/* スレッドの終了 */
static void thread_end(void){
	ts_exit();
}

/* スレッドのスタートアップ */
static void thread_init(ts_thread * thp) {
	/* スレッドのメイン関数を呼び出す */
	thp->init.func(thp->init.argc, thp->init.argv);
	thread_end();
}

/* システムコールの処理 */
static ts_thread_id_t thread_run(ts_func_t func, char * name, int stacksize, int argc, char * argv[]) {
	int i;
	ts_thread * thp;
	uint32 * sp;
	extern char userstack; // リンカスクリプトで定義されるスタック領域
	static char * thread_stack = &userstack; // ユーザスタックに利用される領域

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
	*(--sp) = (uint32)thread_init;

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
static int thread_ext(void) {
	puts(current->name);
	puts(" EXIT\n");
	memset(current, 0, sizeof(*current));
	return 0;
}

static void intr(softvec_type_t type, unsigned long sp) {
	int c;
	// 受信バッファの定義
	static char buf[32];
	static int len;

	// 受信割込みが入ったのでコンソールから１文字受信する
	c = getc();

	if (c != '\n') {
		// 受信したバイト列が改行でない場合はバッファに文字を保存する
		buf[len++] = c;
	} else {
		buf[len++] = '\0';
		if (!strncmp(buf, "echo", 4)) {
			// 受信バイト列がechoの場合 
			puts(buf + 4);
			puts("\n");
		} else {
			puts("Unknow Command. Please check your input.\n");
		}
		puts("> ");
		len = 0;
	}
}

int main(void) {
	INTR_DISABLE; // 割込みを無効化する

	puts("tsos boot suceed!\n");

	// ソフトウェア・割込みベクタにシリアル割込みハンドラを設定
	softvec_setintr(SOFTVEC_TYPE_SERINTR, intr);
	serial_intr_recv_enable(SERIAL_DEFAULT_DEVICE);

	puts("> ");

	// 割込み有効化
	INTR_ENABLE;

	while (1) {
		// 省電力モードに以降 
		asm volatile ("sleep");
	}

	return 0;
}
