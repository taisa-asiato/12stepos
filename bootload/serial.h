#ifndef _SERIAL_H_INCLUDE_
#define _SERIAL_H_INCLUDE_

int serial_init(int index);				/* デバイス初期化 */
int serial_is_send_enable(int index);			/* 送信可能かどうか*/
int serial_send_byte(int index, unsigned char b);	/* 1文字送信 */
int serial_is_recv_enable(int index);
unsigned char serial_recv_byte(int index);

#endif
