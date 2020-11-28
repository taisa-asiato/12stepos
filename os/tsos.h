#ifndef _TSOS_H_INCLUDED_
#define _TSOS_H_INCLUDED_

#include "defines.h"
#include "syscall.h"
#include "interrupt.h"

/* システムコール */
// スレッド起動用のシステムコール
ts_thread_id_t ts_run(ts_func_t func, char * name, int priority, int stacksize, int argc, char * argv[] );
// スレッド終了のシステムコール
void ts_exit(void);
int ts_wait(void);
int ts_sleep(void);
int ts_wakeup(ts_thread_id_t id);
ts_thread_id_t ts_getid(void);
int ts_chpri(int priority);
void  * ts_tmalloc(int size);
int ts_tmfree(void *p);
int ts_send(ts_msgbox_id_t id, int size, char *pp);
ts_thread_id_t ts_recv(ts_msgbox_id_t id, int * sizep, char **pp);
int ts_setintr(softvec_type_t type, ts_handler_t handler);

// サービスコール
int tx_wakeup(ts_thread_id_t id);
void * tx_tmalloc(int size);
int tx_tmfree(void *p);
int tx_send(ts_msgbox_id_t id, int size, char * p);

/* ライブラリ関数 */
// 初期スレッドを起動しOSの動作を開始する
void ts_start(ts_func_t func, char * name, int priority, int stacksize, int argc, char * argv[]);
// 致命的エラーの際に実行する
void ts_sysdown(void);
// システムコールを実行する
void ts_syscall(ts_syscall_type_t type, ts_syscall_param_t * param);

void ts_srvcall(ts_syscall_type_t type, ts_syscall_param_t * param);

// システムタスク
int consdrv_main(int argc, char * argv[]);



/* ユーザースレッドのメイン関数 */
int test09_1_main(int argc, char * argv[]);
int test09_2_main(int argc, char * argv[]);
int test09_3_main(int argc, char * argv[]);
int test09_4_main(int argc, char * argv[]);
int test08_1_main(int argc, char * argv[]);
extern ts_thread_id_t test09_1_id;
extern ts_thread_id_t test09_2_id;
extern ts_thread_id_t test09_3_id;
extern ts_thread_id_t test09_4_id;
int test10_1_main(int argc, char * argv[]);
int test11_1_main(int argc, char * argv[]);
int test11_2_main(int argc, char * argv[]);
int command_main(int argc, char * argv[]);

#endif
