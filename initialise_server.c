#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include "logs.h"
#define PATH_LEN 300

char DocumentRoot[MED_BUF_LEN];
char ServerName[MED_BUF_LEN];
char ServerRoot[MED_BUF_LEN];
int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
struct addrinfo hints, *servinfo, *p;
struct sockaddr_storage their_addr; // connector's address information
socklen_t sin_size;
struct sigaction sa;
int yes=1;
int rv;


void sigctrlc_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
	exit(0);
}
void childprocess_reap()
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int initialise()
{
	key = ftok(".", MY_INBOX_KEY); //generates the label for the mail box used in msgget
	msqid = msgget(key, IPC_CREAT|0600); //IPC_PRIVATE mentions that the mailbox is private to the process. and 0600 tells us that all the process with the same user id can read and write to this msg queue.
	if(msqid < 0)
	{
		//perror("msgget: ");
		char m[MSG_LEN];
		strcpy(m, "msgget: ");
		strcat(m, strerror(errno));
		writeLog(msqid, m, LEVEL_SERVER_ERROR_LOG);
		return 1;

	}
	signal(SIGCHLD, childprocess_reap);
	signal(SIGINT, sigctrlc_handler);

	FILE *conf_fp = fopen("server.conf", "r");
	if(conf_fp == NULL)
	{
		//perror("Configuration: ");
		char m[MSG_LEN];
		strcpy(m, "Configuration: ");
		strcat(m, strerror(errno));
		writeLog(msqid, m, LEVEL_SERVER_ERROR_LOG);
		return 1;
		//server error
	}

	char temp[PATH_LEN];
	// if(temp == NULL)
	// {
	// 	perror("malloc: ");

	// 	return 1;
	// }

	while(feof(conf_fp) == 0)
	{	
		fgets(temp, sizeof(temp), conf_fp);
		if(ferror(conf_fp) != 0)
		{
			char m[MSG_LEN];
			strcpy(m, "Configuration fgets: ");
			strcat(m, strerror(errno));
			writeLog(msqid, m, LEVEL_SERVER_ERROR_LOG);
		}
		if(temp[strlen(temp)-1] == '\n')
			temp[strlen(temp)-1] = temp[strlen(temp)]; //fgets returns the newline character as well so need to remove it..
		char *fieldname;
		fieldname = strtok(temp, ":");
		if(fieldname == NULL)
		{
			char m[MSG_LEN];
			strcpy(m, "Configuration fgets: ");
			strcat(m, strerror(errno));
			writeLog(msqid, m, LEVEL_SERVER_ERROR_LOG);
			break;
		}
		if(strncmp(fieldname, "DocumentRoot",strlen("DocumentRoot")) == 0)
		{
			printf("I was here..%s\n",fieldname);
			fflush(stdout);
			strcpy(DocumentRoot,strtok(NULL, ":")); //check for NULL before use;
		}
		
		else if(strncmp(fieldname, "ServerName",strlen("ServerName")) == 0)
		{
			printf("I was here..%s\n",fieldname);
			fflush(stdout);
			strcpy(ServerName,strtok(NULL, ":")); //check for NULL before use;
		}
		else if(strncmp(fieldname, "ServerRoot",strlen("ServerRoot")) == 0)
		{
			printf("I was here..%s\n",fieldname);
			fflush(stdout);
			strcpy(ServerRoot,strtok(NULL, ":")); //check for NULL before use;
		}
	}
	printf("DocumentRoot: %s\n ServerName: %s\n ServerRoot: %s\n", DocumentRoot, ServerName, ServerRoot);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if((rv = getaddrinfo(NULL, PORT,&hints, &servinfo)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	// loop through all the results and bind to the first wecan
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if(setsockopt(sockfd, SOL_SOCKET,SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if(bind(sockfd, p->ai_addr,p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}
	if(p == NULL) {
		fprintf(stderr,"server:failed to bind\n");
		char m[MSG_LEN];
		strcpy(m, "Server bind: ");
		strcat(m, strerror(errno));
		writeLog(msqid, m, LEVEL_SERVER_ERROR_LOG);
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure
	
	if (listen(sockfd, BACKLOG) == -1) {
		//perror("listen");
		char m[MSG_LEN];
		strcpy(m, "Server Listen: ");
		strcat(m, strerror(errno));
		writeLog(msqid, m, LEVEL_SERVER_ERROR_LOG);
		return 1;
	}
	
	
	printf("server: waiting for connections...\n");


	// printf("%s\n", fieldname);
	// printf("%s\n", DocumentRoot);
	return 0;
}
