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
#include "config.h"

/*Author Idris Adeleke CS4760 Project 3 - Concurrent Linux Programming and Message Queues*/
//This is the main runsim application

//function declaration and global variable block

void timeouthandler(int sig);

void siginthandler(int sig);

int getlicense(void);

int initlicense(void);

int returnlicense(void);

void removelicenses(int n);

void addtolicenses(int n);

int initlogfile(void);

char successstring[1024] = "\0"; char *successtime;

int count, forkcount=0; int nlicense; int message_queue_id;

int pid[max_number_of_processes]; int processid; int pid_check; int childcount;

struct msg {

    int msgtype;
    int msgcontent;
};

struct msg message; 

struct msglog{

    int msgtype;
    int msgcontent;
};

struct msglog logfile;

int log_queue_id;


int main(int argc, char *argv[]){

    //In Parent process

    char filestore[2048]; char *execlargv1, *execlargv2, *execlarg; int getlicense_count;

    //signal handling block declaration

    signal(SIGINT, siginthandler);  //handles Ctrl+C signal inside the parent process

    signal(SIGALRM, timeouthandler); //handles the timeout signal

    alarm(appication_wait_timeout); //fires timeout alarm after 100 seconds defined in the config.h file
 
    if (argc == 1){     //testing if there is any command line argument
        perror("\nrunsim: Error: Missing command line argument. Provide number of licenses\nUsage -> runsim n; where n = number of licenses.");   //use of perror
        exit(1);
    }

    if (argc > 2){       //test if more than 2 command line arguments are provided
        perror("\nrunsim: Error: Too many command line arguments.\nUsage -> runsim n; where n = number of licenses.");      //use of perror
        exit(1);
    }

    nlicense = strtol(argv[1], NULL, 10); //copies command line license argument into global variable nlicense after converting it to an integer. 

    if ((nlicense > max_number_of_processes) || (nlicense <= 1)){     //tests if number of licenses is less than or equal to 1, or is more than 20. 
        perror("\nrunsim: Error: Minimun number of licenses allowed is 2: 1 license for parent and 1 license available to share among the child processes");
        perror("\nrunsim: Error: Maximum of number of licenses/processes allowed is 20.");    //use of perror
        exit(1);
    }

    printf("\nNumber of Licenses is %d\n", nlicense);    //prints out the number of licenses provided

    message.msgtype = 100;  //message type chosen for license message queue is 100
    message.msgcontent = nlicense;  //initializing message content with number of licenses

    message_queue_id = msgget(message_queue_key, IPC_CREAT|0766); //creates the message queue for the license object and gets the queue id

    if (message_queue_id == -1){

        perror("\nrunsim: Error: In Parent Process. Message queue creation failed\n");

        exit(1);
    }

    printf("\nInitializing Message Queue with number of licenses\n");

    initlicense();  //parent runsim process calls this function to initialize the message queue with the number of licenses

    log_queue_id = msgget(logfile_queue_key, IPC_CREAT|0766);

    initlogfile(); //initializes the message queue to control access to the log file

    //start reading standard input here and fork based on the number of testsim lines in the input file

    printf("\nParent process obtaining a license before forking child processes\n");

    while (1){
                
                getlicense_count = getlicense();  //requesting a license before proceeding to fork the child processes
                                                 //removelicense() is called inside getlicense() to decrement the number of licenses after getting one
                if (getlicense_count == 1)
                    break;
            }

    printf("\nLicense successfully obtained by the parent process\n");
   
    while (fgets(filestore, 2048, stdin) != NULL){ //reads from the stdin 1 line at a time

            sleep(5);

            execlarg = strtok(filestore, "  "); //using strtok() to extract the testsim argumments separated by a tab character
            
            execlargv1 = strtok(NULL, " ");    //using strtok() to extract the testsim argumments separated by a tab character

            execlargv2 = strtok(NULL, " ");    //using strtok() to extract the testsim argumments separated by a tab character
            
            processid = fork();    

            if (processid > 0)
                pid[forkcount] = processid;     //an array to store the child process IDs if successfully created


            if (processid < 0){
                perror("\nrunsim: Error: fork() failed!");
                exit(1);
            }

            if (processid == 0){       //This means a child process was created if true

                printf("\nThis is Child Process ID %d getting license\n", getpid()); 

                while (1){
                
                    getlicense_count = getlicense();  //requesting a license before proceeding to call execute testsim

                    if (getlicense_count == 1)
                        break;
                }

                printf("\nThis is Child Process ID %d -> license obtained\n", getpid());

                execl("./testsim", "./testsim", execlargv1, execlargv2, NULL); //how to use execl to execute testsim. exec will not allow execution of codes after this line when it returns
            }
            
            forkcount++;    //tracks the number of child processes created so far
            
            printf("\n%d child processes have been forked\n", forkcount);   //prints out the number of child processes

            for (count = 0; count < forkcount; count++){        //this for loop tracks the number of child processes done executing so far

                sleep(3);

                pid_check = waitpid(pid[count], NULL, WNOHANG);

                if (pid_check > 0){     //if a child process is done, return the license

                    printf("\nChild process %d returning license\n", pid[count]);
                    returnlicense();
                }
            }          
                     
    }         

    printf("\nParent process %d returning license\n", getpid());

    returnlicense();

    printf("\nParent process %d successfully returned license\n", getpid());
    

    //Back In cleanup part of the Parent process 

    printf("\nThis is parent process ID %d, number of child processes are = %d\n", getpid(), forkcount);

    printf("\nParent has stopped waiting because Child processes are now done\n");

    printf("\nAll licenses have been returned\n");

    msgrcv(message_queue_id, &message, sizeof(message), 0, 0);  //reads the final count of the licenses after every process is done executing.

    printf("\nFinal license count is %d\n", message.msgcontent);

    successtime = logeventtime();   //getting the completion time

    snprintf(successstring, sizeof(successstring), "runsim successfully executed on\t%s ", successtime);

    logmsg(successstring); //logs successful execution message before ending the parent process 

    printf("\nParent process is done writing log to file\n");

    if ( msgctl(message_queue_id, IPC_RMID, 0) == 0)
        printf("\nLicense Message Queue ID %d has been removed.\n", message_queue_id);

    else{    
        printf("\nrunsim: Error: In Parent Process, Message Queue removal failed!\n\n");
        exit(1);
    }

    if (msgctl(log_queue_id, IPC_RMID, 0) == 0)
        printf("\nLogging Message Queue ID %d has been removed\n", log_queue_id);
    
    else{
        printf("\nrunsim: Error: In Parent Process, Logging Message Queue removal failed!\n\n");
        exit(1);
    }
       
    return 0;
}


//signal handler definition block

void siginthandler(int sig){    //function to handle Ctrl+C signal interrupt

    char *log_time; char errorstring[1024] = "\0";

    printf("\nCtrl+C received. Aborting Child and Parent Processes..\n");

    for (count = 0; count < forkcount; count++)
        kill(pid[count], SIGKILL); //sending a kill signal to the child processes to forcefully terminate them after Ctrl+C is received

    if ( msgctl(message_queue_id, IPC_RMID, 0) == 0)
        printf("\nLicense Message Queue ID %d has been removed.\n", message_queue_id);

    else{    
        printf("\nrunsim: Error: In Ctrl+C handler, License Message Queue removal failed!\n");
        exit(1);
    }

    if (msgctl(log_queue_id, IPC_RMID, 0) == 0) //removes log file message queue
        printf("\nLogging Message Queue ID %d has been removed.\n", log_queue_id);

    else{    
        printf("\nrunsim: Error: In Ctrl+C handler, Logging Message Queue removal failed!\n");
        exit(1);
    }

    log_time = logeventtime();  //getting time of signal interruption

    snprintf(errorstring, sizeof(errorstring), "runsim terminated by Ctrl+C on\t%s", log_time); //concatenating the errorstring to be written to log file

    logmsg(errorstring); //logs error message before terminating parent process

    printf("\nChild processes terminated. Parent process terminating itself. See logfile.log\n");

    kill(getpid(), SIGTERM); //parent process terminating itself
    
    exit(1);

}

void timeouthandler(int sig){   //this function is called if the program times out after 100 seconds

    char *log_time; char errorstring[1024] = "\0";

    printf("\nrunsim: Error: In timeout handler. Aborting Child and Parent Processes..\n");

    for (count = 0; count < forkcount; count++)
        kill(pid[count], SIGKILL); //sending a kill signal to the child processes to forcefully terminate them after timeout

    if ( msgctl(message_queue_id, IPC_RMID, 0) == 0)
        printf("\nLicense Message Queue ID %d has been removed.\n", message_queue_id);

    else{    
        printf("\nrunsim: Error: In timeout signal handler, License Message Queue removal failed!\n");
        exit(1);
    }

    if (msgctl(log_queue_id, IPC_RMID, 0) == 0) //removes log file message queue
        printf("\nLogging Message Queue ID %d has been removed.\n", log_queue_id);

    else{    
        printf("\nrunsim: Error: In timeout handler, Logging Message Queue removal failed!\n");
        exit(1);
    }
    
    log_time = logeventtime();  //getting event time

    snprintf(errorstring, sizeof(errorstring), "runsim timed out on\t%s", log_time);

    logmsg(errorstring); //logs error message before terminating parent process

    printf("\nChild processes terminated. Parent process terminating itself. See logfile.log\n");

    kill(getpid(), SIGTERM); //parent process terminating itself
    
    exit(1);
}


int getlicense(void){       //returns 1 for license available, and 0 for no license available

    int msgbyte;

    msgbyte = msgrcv(message_queue_id, &message, sizeof(message), 0, 0);

    if (msgbyte == -1){

        perror("\nrunsim: Error: In getlicense(). msgrcv() call failed");

        exit(1);
    }

    printf("\nAvailable license is %d \n", message.msgcontent);

    if (message.msgcontent > 0){

        message.msgcontent = message.msgcontent - 1; //decrement the licesnse by 1

        removelicenses(1);

        return 1;
    }

    else {

        return 0;

    }
    
}


int initlicense(void){      //this function initializes the message queue and copies the number of license to the queue

    int err;
    
      err = msgsnd(message_queue_id, &message, sizeof(message), IPC_NOWAIT);

      if (err == -1){

        perror("\nrunsim: Error: In initlicense(). Message content cannot be written");

        exit(1);
    }

    printf("\nIn initlicense(). License initialization complete\n");

    printf("\n%d licenses written to queue\n", message.msgcontent);
    
    return 0;
}


int returnlicense(void){    //increments the number of available licenses by 1
                            //called by parent to return the license after child process execution
    int msgbyte;

    msgbyte = msgrcv(message_queue_id, &message, sizeof(message), 0, IPC_NOWAIT);

    if (msgbyte == -1){

        message.msgcontent = 1;

        msgsnd(message_queue_id, &message, sizeof(message), IPC_NOWAIT);

        printf("\nQueue is empty. 1 license written to queue\n");

        return 0;
    }

    if (msgbyte != -1){

        message.msgcontent = message.msgcontent + 1;

        msgsnd(message_queue_id, &message, sizeof(message), IPC_NOWAIT);

        printf("\n1 license returned. %d licenses now available.\n", message.msgcontent);

        return 0;
    }

}


void removelicenses(int n){ //removes n from number of license from the license object

        msgsnd(message_queue_id, &message, sizeof(message), IPC_NOWAIT);

        printf("\nIn removelicenses(). %d license taken. %d licenses available\n", n, message.msgcontent);

}


void addtolicenses(int n){  //this function adds n to the number of licenses. It is never used in this program.
        message.msgcontent = n;
        msgsnd(message_queue_id, &message, sizeof(message), IPC_NOWAIT);
}

int initlogfile(void){  //this function initializes the message queue to control logfile access

    logfile.msgtype = 200;
    logfile.msgcontent = 1;
    
    if (msgsnd(log_queue_id, &logfile, sizeof(logfile), IPC_NOWAIT) == -1){

        perror("\nIn initlogfile(). logfile queue cannot be written");

        exit(1);
    }

    printf("\nIn initlogfile(). Log file message queue initialization complete\n");
    
    return 0;
}


