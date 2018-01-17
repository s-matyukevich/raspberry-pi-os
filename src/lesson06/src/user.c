#include "user_sys.h"
#include "user.h"
#include "printf.h"

void loop(char* str)
{
	char buf[2] = {""};
	while (1){
		for (int i = 0; i < 5; i++){
			buf[0] = str[i];
			call_sys_write(buf);
			user_delay(1000000);
		}
	}
}

void user_process() 
{
	call_sys_write("User process\n\r");
	int pid = call_sys_fork();
	if (pid < 0) {
		call_sys_write("Errord during fork\n\r");
		call_sys_exit();
		return;
	}
	if (pid == 0){
		loop("abcde");
	} else {
		loop("12345");
	}
}

