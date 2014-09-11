/*
	This program simply aims to write logs on to a file called log.txt. The main aim of the program was to do the logging of the server on a separate thread
	so that the main thread can focus on the main functionality of the server, rather than the logging which is not so important a job. To see what to do when errors happen in this program..then what to do??
	author -jaishankar
	date: 1.8.2014

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <sys/ipc.h>


#include "logs.h"

char* format_message(struct msg mbuf);
int main()
{
	key_t key = ftok(".", MY_INBOX_KEY); //generates the label for the mail box used in msgget
	int msqid = msgget(key, 0600); //IPC_PRIVATE mentions that the mailbox is private to the process. and 0600 tells us that all the process with the same user id can read and write to this msg queue.
	if(msqid < 0)
	{
		perror("msgget: ");

	}
	
	static int hostcount = 0;
	struct msg mbuf;
	int res, bytesread;
	int h_a_fd = open("host_access_log.txt", O_RDWR|O_CREAT|O_APPEND, 0644); //host access log fd
	if(h_a_fd < 0)
	{
		perror("open: ");
	}
	int s_e_fd = open("server_error_log.txt", O_RDWR|O_CREAT|O_APPEND, 0644); //server error log fd 
	if(s_e_fd < 0)
	{
		perror("open: ");
	}
	//child
	while(1) //child receives relevant info from the message queue and logs it into a file
	{
		
		if(msgrcv(msqid, &mbuf, sizeof(mbuf.msg_body), LEVEL_HOST_ACCESS_LOG, IPC_NOWAIT) > 0) //fetches a message from the queue.
		{
			char *line = format_message(mbuf);
			write(h_a_fd, line, strlen(line));	
		}
		else if(msgrcv(msqid, &mbuf, sizeof(mbuf.msg_body), LEVEL_SERVER_ERROR_LOG, IPC_NOWAIT) > 0) //fetches a message from the queue.
		{
			char *line = format_message(mbuf);
			write(s_e_fd, line, strlen(line));	
		}
	}
	
	
	//fflush(stdout);
}
	
char* format_message(struct msg mbuf)
{
	time_t now = mbuf.msg_body.now;
	char msg[MSG_LEN];
	strcpy(msg, mbuf.msg_body.msg);
	struct tm *tmptr;
	tmptr = localtime(&now);
	
	char *accesstime = asctime(tmptr);
	int len1 = strlen(accesstime);
	int len2 = strlen(msg);
	int total = len1 + len2 + 1; // extra one for the space to be added between the two strings.
	char *line = (char*) malloc(total * sizeof(char));
	accesstime[len1-1] = '\0'; //this is done because very strangely the time function asctime() adds a '\n' newline character before the '\0' null character...this hampers the formatting of the log...so this line removes that newline character.
	strcpy(line, accesstime);
	strcat(line, " ");
	strcat(line, msg);
	strcat(line, "\n"); //the line variable holds all the info (time and ip) concatenated into one single line for the log
	return line;
}
