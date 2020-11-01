#ifndef _TSOS_SYSCALL_H_INCLUDED_
#define _TSOS_SYSCALL_H_INCLUDED_
#include "defines.h"

/* システムコール番号の定義 */
typedef enum {
	TS_SYSCALL_TYPE_RUN = 0,
	TS_SYSCALL_TYPE_EXIT,
} ts_syscall_type_t;

/* システムコール呼び出し時のパラメータ格納域の定義 */
typedef struct {
	union {
		struct {
			ts_func_t func; // メイン関数
			char * name; // スレッド名
			int stacksize; // スタックのサイズ
			int argc; // メイン関数に渡す引数の個数
			char ** argv; // メイン関数に渡す引数
			ts_thread_id_t ret; // ts_runの返り値
		} run;

		struct {
			int dummy; // ts_exit用のパラメータ
		} exit;
	} un;
} ts_syscall_param_t;

#endif
