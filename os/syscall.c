#include "defines.h"
#include "tsos.h"
#include "syscall.h"
#include "interrupt.h"

/* システムコール */
ts_thread_id_t ts_run(ts_func_t func, char *name, int priority, int stacksize, int argc, char *argv[]) {
	ts_syscall_param_t param;
	param.un.run.func = func;
	param.un.run.name = name;
	param.un.run.priority = priority;
	param.un.run.stacksize = stacksize;
	param.un.run.argc = argc;
	param.un.run.argv = argv;
	ts_syscall(TS_SYSCALL_TYPE_RUN, &param);
	return param.un.run.ret;
}

void ts_exit(void) {
	ts_syscall(TS_SYSCALL_TYPE_EXIT, NULL);
}

int ts_wait(void) {
	ts_syscall_param_t param;
	ts_syscall(TS_SYSCALL_TYPE_WAIT, &param);
	return param.un.wait.ret;
}

int ts_sleep(void) {
	ts_syscall_param_t param;
	ts_syscall(TS_SYSCALL_TYPE_SLEEP, &param);
	return param.un.sleep.ret;
}

int ts_wakeup(ts_thread_id_t id) {
	ts_syscall_param_t param;
	param.un.wakeup.id = id;
	ts_syscall(TS_SYSCALL_TYPE_WAKEUP, &param);
	return param.un.wakeup.ret;
}

ts_thread_id_t ts_getid(void) {
	ts_syscall_param_t param;
	ts_syscall(TS_SYSCALL_TYPE_GETID, &param);
	return param.un.getid.ret;
}

int ts_chpri(int priority) {
	ts_syscall_param_t param;
	param.un.chpri.priority = priority;
	ts_syscall(TS_SYSCALL_TYPE_CHPRI, &param);
	return param.un.chpri.ret;
}

void * ts_tmalloc(int size) {
	ts_syscall_param_t param;
	param.un.tmalloc.size = size;
	ts_syscall(TS_SYSCALL_TYPE_TMALLOC, &param);
	return param.un.tmalloc.ret;
}

int ts_tmfree(void * p) {
	ts_syscall_param_t param;
	param.un.tmfree.p = p;
	ts_syscall(TS_SYSCALL_TYPE_TMFREE, &param);
	return param.un.tmfree.ret;
}

int ts_send(ts_msgbox_id_t id, int size, char *p) {
	ts_syscall_param_t param;
	param.un.send.id = id;
	param.un.send.size = size;
	param.un.send.p = p;
	ts_syscall(TS_SYSCALL_TYPE_SEND, &param);
	return param.un.send.ret;
}

ts_thread_id_t ts_recv(ts_msgbox_id_t id, int * sizep, char **pp) {
	ts_syscall_param_t param;
	param.un.recv.id = id;
	param.un.recv.sizep = sizep;
	param.un.recv.pp = pp;
	ts_syscall(TS_SYSCALL_TYPE_RECV, &param);
	return param.un.recv.ret;
}

int ts_setintr(softvec_type_t type, ts_handler_t handler) {
	ts_syscall_param_t param;
	param.un.setintr.type = type;
	param.un.setintr.handler = handler;
	ts_syscall(TS_SYSCALL_TYPE_SETINTR, &param);
	return param.un.setintr.ret;
}

// サービスコール
int tx_wakeup(ts_thread_id_t id) {
	ts_syscall_param_t param;
	param.un.wakeup.id = id;
	ts_srvcall(TS_SYSCALL_TYPE_WAKEUP, &param);
	return param.un.wakeup.ret;
}

void *tx_tmalloc(int size) {
	ts_syscall_param_t param;
	param.un.tmalloc.size = size;
	ts_srvcall(TS_SYSCALL_TYPE_TMALLOC, &param);
	return param.un.tmalloc.ret;
}

int tx_tmfree(void * p) {
	ts_syscall_param_t param;
	param.un.tmfree.p = p;
	ts_srvcall(TS_SYSCALL_TYPE_TMFREE, &param);
	return param.un.tmfree.ret;
}

int tx_send(ts_msgbox_id_t id, int size, char * p) {
	ts_syscall_param_t param;
	param.un.send.id = id;
	param.un.send.size = size;
	param.un.send.p = p;
	ts_srvcall(TS_SYSCALL_TYPE_SEND, &param);
	return param.un.send.ret;
}
