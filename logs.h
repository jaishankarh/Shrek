#ifndef LOGS_H
#define LOGS_H

#define MY_INBOX_KEY 3455
#define LEVEL_HOST_ACCESS_LOG 1
#define LEVEL_HOST_ERROR_LOG 2
#define LEVEL_SERVER_ERROR_LOG 3
#define MSG_LEN 200

struct msg_value
{
	
	int id;
	time_t now;
	char msg[MSG_LEN];
};    // this structure holds the information required to log.
struct msg
{
	long mtype;
	struct msg_value msg_body;
}; // this is covering structure which is posix standard
int msqid;
key_t key;
#endif