#include "defines.h"
#include "tsos.h"
#include "interrupt.h"
#include "lib.h"


ts_thread_id_t test09_1_id;
ts_thread_id_t test09_2_id;
ts_thread_id_t test09_3_id;

/* システムタスクとユーザスレッドの起動 */
static int start_threads(int argc, char * argv[]) {
	// ts_run(test08_1_main, "command", 0x100, 0, NULL);
	test09_1_id = ts_run(test09_1_main, "test09_1", 1, 0x100, 0, NULL);
	test09_2_id = ts_run(test09_2_main, "test09_2", 2, 0x100, 0, NULL);
	test09_3_id = ts_run(test09_3_main, "test09_3", 3, 0x100, 0, NULL);

	ts_chpri(15);
	INTR_ENABLE;
	while (1) {
		asm volatile("sleep");
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
