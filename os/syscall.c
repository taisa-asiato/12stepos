#include "defines.h"
#include "tsos.h"
#include "syscall.h"

/* システムコール */
ts_thread_id_t ts_run(ts_func_t func, char * name, int stacksize, int argc, char * argv[]) {
	ts_syscall_param_t param;
	param.un.run.func = func;
	param.un.run.name = name;
	param.un.run.stacksize = stacksize;
	param.un.run.argc = argc;
	param.un.run.argv = argv;
	ts_syscall(TS_SYSCALL_TYPE_RUN, &param);
	return param.un.run.ret;
}

void ts_exit(void) {
	ts_syscall(TS_SYSCALL_TYPE_EXIT, NULL);
}
