#include "defines.h"
#include "intr.h"
#include "interrupt.h"
#include "serial.h"
#include "lib.h"

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
