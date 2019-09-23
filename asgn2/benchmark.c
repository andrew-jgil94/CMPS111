#include <stdio.h>  
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>`

int main()
{
pid_t child_pid, wpid;
int status = 0;


for (int id=0; id<12; id++) {
    if ((child_pid = fork()) == 0) {
	printf("in child with id=%d\n",id);
    }
}

while ((wpid = wait(&status)) > 0); 
exit(0);
}
