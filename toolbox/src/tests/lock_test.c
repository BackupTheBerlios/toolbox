
#include "pthread.h"
#include "Toolbox.h"

String_t S;

int wrk1() {
	while(1) {
		tb_warn("entering wrk1 (%d)\n", pthread_self());
		//		dump_share_list(
		tb_Rlock(S);
		tb_warn("wrk1 R_locked S\n");
		tb_Dump(S);
		sleep(2);
		tb_Unlock(S);
		tb_warn("wrk1 Un_R_locked S\n");
	}
}

int wrk2() {
	while(1) {
		tb_warn("entering wrk2 (%d)\n", pthread_self());
		tb_Rlock(S);
		tb_warn("wrk2 R_locked S\n");
		tb_Dump(S);
		sleep(2);
		tb_Unlock(S);
		tb_warn("wk2 Un_R_locked S\n");
	}
}


int wrk3() {
	while(1) {
		tb_warn("entering wrk3 (%d)\n", pthread_self());
		tb_Wlock(S);
		tb_warn("wk3 W_locked S\n");
		tb_Dump(S);
		sleep(5);
		tb_Unlock(S);
		tb_warn("wrk3 Un_W_locked S\n");
	}
}


int main(int argc, char **argv) {

	pthread_t pt1, pt2, pt3;

	S = tb_String("OBJ");

	pthread_create(&pt1,NULL, wrk1, NULL);
	sleep(1);
	pthread_create(&pt2,NULL, wrk2, NULL);
	sleep(1);
	pthread_create(&pt3,NULL, wrk3, NULL);

	while(1) sleep(100);
	return 0;
}
