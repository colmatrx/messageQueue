#define max_number_of_processes 20  //maximum number of processes/licenses allowed
#define appication_wait_timeout 120 //time for the parent process to wait before killing all child processes.
#define message_queue_key 140687
#define logfile_queue_key 140672


/*Author Idris Adeleke CS4760 Project 2 - Concurrent Linux Programming and Message Queue*/

void testsim(int sleepTime, int repeatFactor);

void logmsg(const char *msg);

char *logeventtime(void);

int getlicense(void);

int returnlicense(void);

int initlicense(void);

void addtolicenses(int n);

void removelicenses(int n);

