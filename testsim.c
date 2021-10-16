#include<stdio.h>
#include <sys/ipc.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<strings.h>
#include<time.h>
#include<sys/wait.h>
#include<sys/msg.h>
#include<signal.h>
#include<stdlib.h>

/*Author Idris Adeleke CS4760 Project 2 - Concurrent Linux Programming and SHared Memory*/
//This is the testsim application that gets called by the execl command inside runsim

void testsim(int, int); //function declaration

int main(int argc, char *argv[]){

    long int arg1, arg2;

    arg1 = strtol(argv[1], NULL, 10); arg2 = strtol(argv[2], NULL, 10); //using strtol() to convert the string arg argv[2] to integer

    printf("\nexecuting testsim in process %d \n", getpid());

    testsim(arg1, arg2 ); //takes command line args sleeptime and repeatfactor as integers

    printf("\nProcess %d completed testsim execution\n", getpid());

    return 0;

}