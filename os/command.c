#include "defines.h"
#include "tsos.h"
#include "consdrv.h"
#include "lib.h"

// コンソールドライバの使用開始を依頼する
static void send_use(int index) {
	char * p;
	p = ts_tmalloc(3);
	p[0] = '0';
	p[1] = CONSDRV_CMD_USE;
	p[2] = '0' + index;
	ts_send(MSGBOX_ID_CONSOUTPUT, 3, p);
}

// コンソールへの文字出力をドライバに依頼する
static void send_write(char * str) {
	char * p;
	int len;
	len = strlen(str);
	p = ts_tmalloc(len+2);
	p[0] = '0';
	p[1] = CONSDRV_CMD_WRITE;
	memcpy(&p[2], str, len);
	ts_send(MSGBOX_ID_CONSOUTPUT, len+2, p);
}

int command_main(int argc, char * argv[]) {
	char * p;
	int size;

	send_use(SERIAL_DEFAULT_DEVICE);

	while (1) {
		send_write("command> ");

		ts_recv(MSGBOX_ID_CONSINPUT, &size, &p);
		p[size] = '\0';


		if (!strncmp(p, "echo", 4)) {
			send_write(p + 4);
			send_write("\n");
		} else {
			send_write("Unknow command.\n");
		}
		ts_tmfree(p);
	}
	return 0;
}
