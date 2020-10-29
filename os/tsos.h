#ifndef _TSOS_H_INCLUEDED_
#define _TSOS_H_INCLUEDED_

#include "defines.h"
#include "syscall.h"

/* システムコール */
// スレッド起動用のシステムコール
ts_thread_id_t ts_run(ts_func_t func, char * name, int stacksize, int argc, char * argv[] );
// スレッド終了のシステムコール
void ts_exit(void);

/* ライブラリ関数 */
// 初期スレッドを起動しOSの動作を開始する
void ts_start(ts_func_t func, char * name, int stacksize, int argc, char * argv[]);
// 致命的エラーの際に実行する
void ts_sysdown(void);
// システムコールを実行する
void ts_syscall(ts_syscall_type_t type, ts_syscall_param_t * param);

/* ユーザースレッドのメイン関数 */
int test08_1_main(int argc, char * argv[]);

#endif
