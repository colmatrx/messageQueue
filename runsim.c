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

/*Author Idris Adeleke CS4760 Project 2 - Concurrent Linux Programming and SHared Memory*/
//This is the main runsim application

//function declaration and global variable block

void siginthandler(int sig);

void timeouthandler(int sig);

int getlicense(void);

int initlicense(void);

int returnlicense(void);

void removelicenses(int n);

void addtolicenses(int n);

char successstring[1024] = "\0"; char *successtime;

int count, forkcount=0; int nlicense; int message_queue_id;

int pid[max_number_of_processes]; int processid;

struct msg {

    int msgtype;
    int msgcontent;
};

struct msg message; 


int main(int argc, char *argv[]){

    //In Parent process

    char filestore[2048]; char *execlargv1, *execlargv2, *execlarg; int getlicense_count;

    //signal handling block

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

    nlicense = strtol(argv[1], NULL, 10); //copies command line license argument into global variable nlicense. 

    printf("\nNumber of Licenses is(are) %d\n", nlicense);    //prints out the number of licenses provided

    if ((nlicense > max_number_of_processes) || (nlicense <= 0)){     //tests if number of licenses is less than or equal to zero, or is more than 20. 
        perror("\nrunsim: Error: Minimun number of licenses allowed is 1.\nMaximum of number of licenses/processes allowed is 20.");    //use of perror
        exit(1);
    }

    message.msgtype = 100;
    message.msgcontent = nlicense;

    message_queue_id = msgget(message_queue_key, IPC_CREAT|0766); //creates the message queue and gets the queue id

    if (message_queue_id == -1){

        perror("\nrunsim: Error: In Parent Process. Message queue creation failed\n");

        exit(1);
    }

    printf("\nInitializing Message Queue with number of licenses\n");

    initlicense();  //parent runsim process calls this function to initialize the message queue with the number of licenses

    //start reading standard input here and fork based on the number of testsim lines in the input file
   
    while (fgets(filestore, 2048, stdin) != NULL){ //reads from the stdin 1 line at a time

            printf("\nObtaining license before forking a child process...\n");

            sleep(3);

            while (1){
                
                getlicense_count = getlicense();  //requesting a license before proceeding to fork a child process

                if (getlicense_count == 1)
                    break;
            }

            printf("\nLicense obtained in Parent process %d\n", getpid());

            execlarg = strtok(filestore, "  "); //using strtok() to extract the testsim argumments separated by a tab character
            
            execlargv1 = strtok(NULL, " ");    //using strtok() to extract the testsim argumments separated by a tab character

            execlargv2 = strtok(NULL, " ");    //using strtok() to extract the testsim argumments separated by a tab character
            
            processid = fork();    //an array to store the process IDs

            if (processid > 0)
                pid[forkcount] = processid;


            if (processid < 0){
                perror("\nrunsim: Error: fork() failed!\n");
                exit(1);
            }

            if (processid == 0){       //This means a child process was created if true

                printf("\nThis is Child Process ID %d getting license\n", getpid()); 

                while (1){
                
                    getlicense_count = getlicense();  //requesting a license before proceeding to fork a child process

                    if (getlicense_count == 1)
                        break;
                }

                printf("\nThis is Child Process ID %d -> license obtained\n", getpid());

                execl("./testsim", "./testsim", execlargv1, execlargv2, NULL); //how to use execl to execute testsim. exec will not allow execution of codes after this line when it returns
            }

        returnlicense();    //returning the license after successfully forking a child process

        printf ("\nParent process returned license\n");
            
        forkcount++; printf("\nforkcount is %d \n", forkcount);
             
        wait(0);
        returnlicense(); printf("\nChild process returned a license\n");
                     
    }   

    //Back In Parent process
    //returnlicense(); 

    /*for (count = 0; count < forkcount; count++){
        wait(0);
        returnlicense(); printf("\nChild process returned a license\n");
        }*/

    printf("\nThis is parent process ID %d, number of child processes are = %d\n", getpid(), forkcount);

    printf("\nParent has stopped waiting because Child processes are now done\n");

    msgrcv(message_queue_id, &message, sizeof(message), 0, 0);

    printf("\nFinal license count is %d\n", message.msgcontent);

    successtime = logeventtime();   //getting the completion time

    snprintf(successstring, sizeof(successstring), "runsim successfully executed on\t%s ", successtime);

    logmsg(successstring); //logs successful execution message before ending the parent process 

    if ( msgctl(message_queue_id, IPC_RMID, 0) == 0)
        printf("\nMessage Queue ID %d has been removed.\n", message_queue_id);

    else{    
        printf("\nrunsim: Error: In Parent Process, Message Queue removal failed!\n\n");
        exit(1);
    }
       
    return 0;
}


//signal handler blocks

void siginthandler(int sig){    //function to handle Ctrl+C signal interrupt

    char *log_time; char errorstring[1024] = "\0";

    printf("\nCtrl+C received. Aborting Child and Parent Processes..\n");

    for (count = 0; count < forkcount; count++)
        kill(pid[count], SIGKILL); //sending a kill signal to the child processes to forcefully terminate them after Ctrl+C is received

    if ( msgctl(message_queue_id, IPC_RMID, 0) == 0)
        printf("\nMessage Queue ID %d has been removed.\n", message_queue_id);

    else{    
        printf("\nrunsim: Error: In Ctrl+C handler, Message Queue removal failed!\n");
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
        printf("\nMessage Queue ID %d has been removed.\n", message_queue_id);

    else{    
        printf("\nrunsim: Error: In timeout signal handler, Message Queue removal failed!\n");
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

   // message_queue_id = msgget(message_queue_key, 0);
    int msgbyte;

    printf("\nInside getlicense()\n");

    msgbyte = msgrcv(message_queue_id, &message, sizeof(message), 0, 0);

    printf("\nInside getlicense() after msgbyte\n");


    if (msgbyte == -1){

        perror("\nIn getlicense(). msgrcv call failed\n");

        exit(1);
    }

    printf("\nMessage content is %d \n", message.msgcontent);

    if (message.msgcontent > 0){

        message.msgcontent = message.msgcontent - 1; //decrement the licesnse by 1

        removelicenses(1);

        return 1;
    }

    else {

        return 0;

    }
    
}


int initlicense(void){      //this function initializes the message queue and copies the number of license in there

    int err;
    
      err = msgsnd(message_queue_id, &message, sizeof(message), IPC_NOWAIT);

      if (err == -1){

        perror("\nIn initlicense(). Message content cannot be written");

        exit(1);
    }

    printf("\nerr is %d\n", err);

    printf("\nIn initlicense(). License initialization complete\n");

    printf("\n%d licenses written in queue\n", message.msgcontent);
    
    return 0;
}


int returnlicense(void){    //increments the number of available licenses.
                            //called by parent to return the license after child process execution

    printf("\nIn returnlicense()\n");

    if ((msgrcv(message_queue_id, &message, sizeof(message), 0, 0)) == -1){

        perror("\nrunsim: Error: In returnlicense() -> msgrcv() call failed\n");
        exit(1);
    }

    message.msgcontent = message.msgcontent + 1;

    msgsnd(message_queue_id, &message, sizeof(message), IPC_NOWAIT);

    printf("\nlicense incrememnted by 1. %d licenses now wirtten back to queue", message.msgcontent);

    return 0;
}


void removelicenses(int n){ //removes n from number of license

        msgsnd(message_queue_id, &message, sizeof(message), IPC_NOWAIT);

        printf("\nIn removelicenses(). 1 license removed. %d licenses written back to queue", message.msgcontent);

}


void addtolicenses(int n){  //this function adds n to the number of licenses. It is never used in this program so it is not considered a critical section in this case


}

