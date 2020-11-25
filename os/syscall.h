#ifndef _TSOS_SYSCALL_H_INCLUDED_
#define _TSOS_SYSCALL_H_INCLUDED_
#include "defines.h"
#include "interrupt.h"

/* システムコール番号の定義 */
typedef enum {
	TS_SYSCALL_TYPE_RUN = 0,
	TS_SYSCALL_TYPE_EXIT,
	TS_SYSCALL_TYPE_WAIT,
	TS_SYSCALL_TYPE_SLEEP,
	TS_SYSCALL_TYPE_WAKEUP,
	TS_SYSCALL_TYPE_GETID,
	TS_SYSCALL_TYPE_CHPRI,
	TS_SYSCALL_TYPE_TMALLOC,
	TS_SYSCALL_TYPE_TMFREE,
	TS_SYSCALL_TYPE_SEND,
	TS_SYSCALL_TYPE_RECV,
	TS_SYSCALL_TYPE_SETINTR,
} ts_syscall_type_t;

/* システムコール呼び出し時のパラメータ格納域の定義 */
typedef struct {
	union {
		struct {
			ts_func_t func; // メイン関数
			char * name; // スレッド名
			int priority; // 
			int stacksize; // スタックのサイズ
			int argc; // メイン関数に渡す引数の個数
			char ** argv; // メイン関数に渡す引数
			ts_thread_id_t ret; // ts_runの返り値
		} run;
		struct {
			int dummy; // ts_exit用のパラメータ
		} exit;
		struct {
			int ret; //
		} wait;
		struct {
			int ret;
		} sleep;
		struct {
			ts_thread_id_t id;
			int ret;
		} wakeup;
		struct {
			ts_thread_id_t ret;
		} getid;
		struct {
			int priority;
			int ret;
		} chpri;
		struct {
			int size;
			void * ret;
		} tmalloc;
		struct {
			char * p;
			int ret;
		} tmfree;
		struct {
			ts_msgbox_id_t id;
			int size;
			char *p;
			int ret;
		} send;
		struct {
			ts_msgbox_id_t id;
			int * sizep;
			char **pp;
			ts_thread_id_t ret;
		} recv;
		struct {
			softvec_type_t type;
			ts_handler_t handler;
			int ret;
		} setintr;
	} un;
} ts_syscall_param_t;

#endif
