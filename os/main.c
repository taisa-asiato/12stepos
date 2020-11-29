#include "defines.h"
#include "tsos.h"
#include "interrupt.h"
#include "lib.h"


ts_thread_id_t test09_1_id;
ts_thread_id_t test09_2_id;
ts_thread_id_t test09_3_id;
ts_thread_id_t test09_4_id;

/* システムタスクとユーザスレッドの起動 */
static int start_threads(int argc, char * argv[]) {
	//ts_run(test08_1_main, "test08_1_main", 1, 0x100, 0, NULL);
	//test09_1_id = ts_run(test09_1_main, "test09_1", 1, 0x100, 0, NULL);
	//test09_2_id = ts_run(test09_2_main, "test09_2", 2, 0x100, 0, NULL);
	//test09_3_id = ts_run(test09_3_main, "test09_3", 3, 0x100, 0, NULL);
	//test09_4_id = ts_run(test09_4_main, "test09_4", 4, 0x100, 0, NULL);

	//ts_run(test10_1_main, "test10_1", 1, 0x100, 0, NULL);
	//
	//ts_run(test11_1_main, "test11_1", 1, 0x100, 0, NULL);
	//ts_run(test11_2_main, "test11_2", 2, 0x100, 0, NULL);
	//
	//
	//
	ts_run(consdrv_main, "consdrv", 1, 0x200, 0, NULL);
	ts_run(command_main, "comamnd", 8, 0x200, 0, NULL);
	ts_chpri(15);
	INTR_ENABLE;
	while (1) {
		asm volatile ("sleep");
	}
	return 0;
}

int main(void) {
	INTR_DISABLE;

	puts("tsos boot succeeded\n");

	/* OS起動 */
	ts_start(start_threads, "idle", 0, 0x100, 0, NULL);

	return 0;
}
