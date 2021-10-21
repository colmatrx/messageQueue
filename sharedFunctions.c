#include<stdio.h>
#include<stdlib.h>
#include <sys/ipc.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<strings.h>
#include<time.h>
#include<sys/wait.h>
#include<signal.h>
#include<sys/msg.h>
#include"config.h"

/*Author Idris Adeleke CS4760 Project 3 - Concurrent Linux Programming and Message Queues*/
//This file contains functions that are shared by both testsim and runsim

struct msglog{

    int msgtype;
    int msgcontent;
};

struct msglog testsimlog;

time_t  msgtime;

char logstring[2048] = "\0";

int logging_id;


void testsim(int sleepTime, int repeatFactor){      //testsim() sleeps for sleepTime seconds in a loop counted by repeatFactor
                                                    //gets called by testsim application.
    int count = 0; char *logtime;

    logging_id = msgget(logfile_queue_key, 0);

    printf("\nIn testsim, logging id is %d\n", logging_id);

    while (1){          //attempts to get access to write message to logfile

        msgrcv(logging_id, &testsimlog, sizeof(testsimlog), 0, 0);

        if (testsimlog.msgcontent == 1){

            printf("\nChild process %d gets file write access\n", getpid());
            break;  

        }      
    }

    for (count = 0; count < repeatFactor; count++){

        printf("\nSleeping for %d seconds\n", sleepTime);

        sleep(sleepTime);

        time(&msgtime);

        logtime = ctime(&msgtime);

        snprintf(logstring, sizeof(logstring), "PID %d\tIteration %d of %d\t%s", getpid(), count+1, repeatFactor, logtime); //concatenates the parameters as a single string
 
        logmsg(logstring);   

    } 

    testsimlog.msgtype = 200; testsimlog.msgcontent = 1; //saving 1 back to the msgcontent
    msgsnd(logging_id, &testsimlog, sizeof(testsimlog), 0); //writing 1 back to the message queue to indicate process is done writing to the log file.
    printf("\nChild process %d is done writing log to file\n", getpid());
}


void logmsg(const char *msg){       //this function is for writing to logfile

    FILE *filedescriptor;

    filedescriptor = fopen("logfile.log", "a"); //open logfile in append mode

    if (filedescriptor == NULL){
        perror("\nrunsim: Error: Log File cannot be opened\n");
        exit(1);
    }

    fputs(msg, filedescriptor);    //writes log message to file
    fclose(filedescriptor);
}


char *logeventtime(void){       //thi function is for retrieving the system time for logging purposes

    time_t msgtime; char *logtime;
    
    time(&msgtime);

    logtime = ctime(&msgtime);

    return logtime;

}

