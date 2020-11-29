#include "defines.h"
#include "tsos.h"
#include "intr.h"
#include "interrupt.h"
#include "serial.h"
#include "lib.h"
#include "consdrv.h"

#define CONS_BUFFER_SIZE 24

static struct consreg {
	ts_thread_id_t id; // コンソールを利用するスレッドのID
	int index;	// 利用するシリアルの番号

	char * send_buf; // 送信バッファ
	char * recv_buf; // 受信バッファ
	int send_len;	// 送信バッファ中のデータサイズ
	int recv_len;	// 受信バッファ中のデータサイズ

	long dummy[3];
} consreg[CONSDRV_DEVICE_NUM];

// 送信バッファの先頭１文字を送信する
static void send_char(struct consreg * cons) {
	int i;
	serial_send_byte(cons->index, cons->send_buf[0]);
	cons->send_len--;
	// 先頭文字は制御文字なので省略してからスタートする
	for (i = 0; i < cons->send_len; i++) {
		cons->send_buf[i] = cons->send_buf[i+1];	
	}
}

// 文字列を送信バッファに書き込み送信開始する
static void send_string(struct consreg * cons, char * str, int len) {
	int i;
	for (i = 0; i < len; i++ ) {
		if (str[i] == '\n') {
			// \nを\n\rに変換する
			cons->send_buf[cons->send_len++] = '\r';
		}
		cons->send_buf[cons->send_len++] = str[i];
	}

	if (cons->send_len && !serial_intr_is_send_enable(cons->index)) {
		serial_intr_send_enable(cons->index); // 送信割込み有効化
		send_char(cons);	// 送信開始
	}
}

static int consdrv_intrproc(struct consreg * cons) {
	unsigned char c;
	char * p;

	// 受信割込み
	if (serial_is_recv_enable(cons->index)) {
		c = serial_recv_byte(cons->index);
		if (c == '\r') {
			c = '\n';
		}

		send_string(cons, &c, 1); // エコーバック処理

		if (cons->id) {
			if (c != '\n') {
				// 改行コードでない場合は受信バッファにバッファリングする
				cons->recv_buf[cons->recv_len++] = c;
			} else {
				// Enterが押された場合
				p = tx_tmalloc(CONS_BUFFER_SIZE);
				memcpy(p, cons->recv_buf, cons->recv_len);
				tx_send(MSGBOX_ID_CONSINPUT, cons->recv_len, p);
				cons->recv_len = 0;
			}
		}
	}

	// 送信割込み
	if (serial_is_send_enable(cons->index)) {
		if (!cons->id || !cons->send_len) {
			// 送信データがなければ終了
			serial_intr_send_disable(cons->index);
		} else {
			// 送信データがある場合は引き続き送信する
			send_char(cons);
		}
	}
	return 0;
}

// 割込みハンドラ
static void consdrv_intr(void) {
	int i;
	struct consreg * cons;

	for (i = 0; i < CONSDRV_DEVICE_NUM; i++) {
		cons = &consreg[i];
		if (cons->id) {
			if (serial_is_send_enable(cons->index) || serial_is_recv_enable(cons->index)) {
				//　送信または受信の割込みが発生している場合
				consdrv_intrproc(cons);
			}
		}
	}
}

static int consdrv_init(void) {
	memset(consreg, 0, sizeof(consreg));
	return 0;
}

// スレッドからの要求を処理する
static int consdrv_command(struct consreg * cons, ts_thread_id_t id, int index, int size, char * command) {
	switch(command[0]) {
		case CONSDRV_CMD_USE:
			cons->id = id;
			cons->index = command[1] - '0';
			cons->send_buf = ts_tmalloc(CONS_BUFFER_SIZE);
			cons->recv_buf = ts_tmalloc(CONS_BUFFER_SIZE);
			cons->send_len = 0;
			cons->recv_len = 0;
			serial_init(cons->index);
			serial_intr_recv_enable(cons->index);
			break;
		case CONSDRV_CMD_WRITE:
			// send_stringでは送信バッファを操作しているため割込み禁止にする
			INTR_DISABLE;
			send_string(cons, command + 1, size-1);
			INTR_ENABLE;
			break;
		default:
			break;
	}
	return 0;
}

int consdrv_main(int argc, char * argv[]) {
	int size, index;
	ts_thread_id_t id;
	char * p;

	consdrv_init();
	// 割込みハンドラの設定
	ts_setintr(SOFTVEC_TYPE_SERINTR, consdrv_intr);

	while (1) {
		id = ts_recv(MSGBOX_ID_CONSOUTPUT, &size, &p);
		index = p[0] - '0';
		consdrv_command(&consreg[index], id, index, size-1, p+1);
		ts_tmfree(p);
	}
	return 0;
}
