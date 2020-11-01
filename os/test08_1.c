#include "defines.h"
#include "tsos.h"
#include "lib.h"

int test08_1_main(int argc, char * argv[]) {
	char buf[32];

	puts("test08_1 started\n");

	while (1) {
		puts("> ");
		gets(buf);

		if(!strncmp(buf, "echo", 4)) {
			puts(buf+4);
			puts("\n");
		} else if (!strncmp(buf, "exit", 4)) {
			break;
		} else {
			puts("Unknow command.\n");
		}
	}

	puts("test08_1 exit.\n");
	return 0;
}
