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
