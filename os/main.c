#include "defines.h"
#include "tsos.h"
#include "interrupt.h"
#include "lib.h"

/* システムタスクとユーザスレッドの起動 */
static int start_threads(int argc, char * argv[]) {
	ts_run(test08_1_main, "command", 0x100, 0, NULL);
	return 0;
}

int main(void) {
	INTR_DISABLE;

	puts("tsos boot succeeded\n");

	/* OS起動 */
	ts_start(start_threads, "start", 0x100, 0, NULL);

	return 0;
}
