#include "defines.h"
#include "interrupt.h"
#include "serial.h"
#include "lib.h"
#include "elf.h"
#include "xmodem.h"

static int init(void) {
	/* 以下のシンボルはリンカスクリプトで定義済のもの */
	extern int erodata, data_start, edata, bss_start, ebss;

	/*
	 * データ領域とBSS領域を初期化
	 * */
	// なぜerodataの値をram上へ配置するのかわからない
	memcpy(&data_start, &erodata, (long)&edata - (long)&data_start);
	memset(&bss_start, 0, (long)&ebss - (long)&bss_start);

	// ソフトウェア・割込みベクタを初期化する
	softvec_init();

	/* シリアルの初期化 */
	serial_init(SERIAL_DEFAULT_DEVICE);

	return 0;
}

static int dump(char * buf, long size) {
	long i;
	if (size < 0) {
		puts("no data.\n");
		return -1;
	}
	for (i = 0 ; i < size ; i++) {
		putxval(buf[i], 2);
		if ((i & 0xf) == 15) {
			puts("\n");
		} else {
			if ((i & 0xf) == 7) {
				puts(" ");
			}
			puts(" ");
		}
	} 
	puts("\n");
	return 0;
}
static void wait() {
	volatile long i;
	for (i = 0; i < 300000; i++) {
		;
	}
}
int main(void) {
	static char buf[16];
	static long size = -1;
	static unsigned char * loadbuf = NULL;
	char * entry_point;
	void (*f) (void);
	/* リンカスクリプトで定義されているバッファ */
	extern int buffer_start;

	// 割込み無効化する
	INTR_DISABLE;

	init();
	puts("tzload (tzos boot loader) started.\n");

	while (1) {
		puts("tzload> ");
		gets(buf);

		if (!strcmp(buf, "load")) {
			/* XMODEMでのファイルのダウンロード */
			loadbuf = (char *)(&buffer_start);
			size = xmodem_recv(loadbuf);
			wait(); /* 転送が終了し端末に制御が戻るまで待つ */
			if (size < 0) {
				puts("\nXMODEM receive error!\n");
			} else {
				puts("\nXMODEM receive succeeded.\n");
			}

		} else if (!strcmp(buf, "dump")) {
			puts("size: ");
			putxval(size, 0);
			puts("\n");
			dump(loadbuf, size);
		} else if (!strcmp(buf, "run")) { // ELF形式ファイルの実行
			entry_point = elf_load(loadbuf); /* メモリ上に展開 */
			if (!entry_point) {
				puts("run error!\n");
			} else {
				/* エントリポイントが有効な場合 */
				puts("starting from entry point: ");
				putxval((unsigned long)entry_point, 0);
				puts("\n");
				f = (void (*)(void))entry_point;
				/* 関数fを実行する */
				f();
				/* 戻りアドレスを設定していないので，戻ってこない */
			}
		} else {
			puts("Unknown command.\n");
		}
	}
	return 0;
}
