#include "defines.h"
#include "tsos.h"
#include "syscall.h"

/* システムコール */
ts_thread_id_t ts_run(ts_func_t func, char * name, int priority, int stacksize, int argc, char * argv[]) {
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
