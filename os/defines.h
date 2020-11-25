#ifndef _DEFINES_H_INCLDUE_
#define _DEFINES_H_INCLDUE_

#define NULL ((void *)0)	/* NULLポインタの定義 */
#define SERIAL_DEFAULT_DEVICE 1	/* 標準のシリアルデバイス */

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;

typedef uint32 ts_thread_id_t;
typedef int (*ts_func_t)(int argc, char * argv[]);
typedef void (*ts_handler_t)(void);

typedef enum {
	MSGBOX_ID_CONSINPUT = 0, // コンソールからの入力用
	MSGBOX_ID_CONSOUTPU,	// コンソールへの出力用
	MSGBOX_ID_NUM
} ts_msgbox_id_t;

#endif
