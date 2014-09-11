#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include "logs.h"
#include "server_error_codes.h"


#define MAX_URI_LEN 2000

#define CR 13
#define LF 10

struct Header
{
	char name[MED_BUF_LEN];
	char value[MED_BUF_LEN];
	struct Header *next;
};

struct Request
{
	char method[SMALL_BUF_LEN];
	char path[MAX_URI_LEN];
	struct Header *headers;
	char protocol[SMALL_BUF_LEN];
	float protocolversion;
	int no_headers;
	int content_length;

};
struct Response
{
	char protocol[SMALL_BUF_LEN];
	struct Header* headers;
	float protocolversion;
	char status_code[SMALL_BUF_LEN];
	char status[MED_BUF_LEN];
	char server_name[MED_BUF_LEN];
	int no_headers;
	char date[SMALL_BUF_LEN+30];
	//int content_length;

};

struct Header* createHeader(char *name, char *value);


int writeLog(int msqid, char *msg, long level)
{
	struct msg* msgbuf;
	static int hostcount = 0;
	time_t now;
	msgbuf = (struct msg *)malloc(sizeof(struct msg));
	msgbuf->mtype = level;
	msgbuf->msg_body.id = hostcount++;
	time(&(msgbuf->msg_body.now));
	strcpy(msgbuf->msg_body.msg,msg);
	if((msgsnd(msqid,msgbuf,sizeof(msgbuf->msg_body),IPC_NOWAIT)) != 0)
	{
		perror("msgsnd: ");
		char m[MSG_LEN];
		strcpy(m, "msgsnd: ");
		strcat(m, strerror(errno));
		writeLog(msqid, m, LEVEL_SERVER_ERROR_LOG); // this could cause a recursive loop which has to be broken..
		return 1;
	} //actual sending of thee info to be logged to the message queue so the child process can read it from there.
	return 0;
}

int parseHeaderData(char* header, struct Request* req)
{
	if(header == NULL)
	{
		char m[MSG_LEN];
		strcpy(m, "Server Host header not found: ");
		strcat(m, strerror(errno));
		writeLog(msqid, m, LEVEL_HOST_ERROR_LOG);
		return BAD_REQUEST;
	}
	//get initial line
	//struct Request *req = malloc(sizeof(struct Request));
	// if(req == NULL)
	// {
	// 	char m[MSG_LEN];
	// 	strcpy(m, "Server parseHeaderData malloc error: ");
	// 	strcat(m, strerror(errno));
	// 	writeLog(msqid, m, LEVEL_SERVER_ERROR_LOG);
	// 	return OUT_OF_MEMORY;

	// }
	int i = 0;
	char *str = strstr(header, "\r\n ");

	char *method;
	char *path;
	char *protocol;
	char *token = strtok(header, "\r\n");
	char *firstline = malloc(strlen(token)*sizeof(char)+1);
	strcpy(firstline,token);
	char **headerlines = malloc(NO_HEADERS*sizeof(char *));
	

	while(1)
	{
		token = strtok(NULL, "\r\n");
		if(token == NULL)
			break;
		headerlines[i] = malloc(strlen(token)*sizeof(char)+1);
		strcpy(headerlines[i], token);
		i++;
	}
	int no_read_headers = i;
	printf("\nI was here...");
	fflush(stdout);
	method = strtok(firstline, " ");
	printf("%s\n", method);
	path = strtok(NULL," ");
	printf("%s\n", path);
	protocol = strtok(NULL, " ");
	char *protocolname = strtok(protocol, "/");
	printf("%s\n", protocolname);
	float protocolversion = atof(strtok(NULL, "/"));
	printf("%f\n", protocolversion);
	
	strcpy(req->method, method);
	strcpy(req->path, path);
	strcpy(req->protocol, protocol);
	req->protocolversion = protocolversion;
	req->headers = malloc(no_read_headers * sizeof(struct Header));
	req->content_length = 0;
	req->no_headers = no_read_headers;
	for(i = 0; i < no_read_headers; i++)
	{
		char* line = headerlines[i];
		char* header_name = strtok(line, ":");
		if(header_name == NULL)
		{
			return BAD_REQUEST;
		}
		char* header_value = strtok(NULL, ":"); // if a second ':' exists then the string will get truncated...
		strcpy(req->headers[i].name, header_name);
		strcpy(req->headers[i].value, header_value);
		//printf("%s: %s\n", header_name,header_value);
		if(strcmp(header_name, "Content-Length") == 0)
		{
			req->content_length = atoi(header_value); // if value is not integer could be a big problem....
		}

	}
	
	// if(no != 3)
	// {
	// 	char m[MSG_LEN];
	// 	strcpy(m, "Server parseHeaderData malloc error: ");
	// 	strcat(m, strerror(errno));
	// 	writeLog(msqid, m, LEVEL_HOST_ERROR_LOG);
	// 	return BAD_REQUEST;
	// }
	

}
char* unfold(char* buf)
{

	char *str = strstr(buf, "\r\n ");
	while(str != NULL)
	{
		char *sp_st = str;
		str += 2; // move the str pointer after the \r\n... 
		int count = 0;
		while(isspace(*(str)))
		{
			str++; //move the pointer str until a space is not found...
			count++;
		}
		if(str != sp_st)
		{
			memmove(sp_st, str, strlen(str)+1); // move all the contents after the spaces to before the spaces...
		}
		str = strstr(sp_st, "\r\n ");
	}
	return buf;
}
int createDefaultResponse(struct Response *res, struct Request *req)
{
	strcpy(res->protocol, req->protocol);
	res->protocolversion = req->protocolversion;
	strcpy(res->server_name, ServerName);
	time_t now;
	time(&now);
	struct tm *tm_now = gmtime(&now);
	char *now_str = asctime(tm_now);
	now_str[strlen(now_str)-1] = '\0'; //this is done because very strangely the time function asctime() adds a '\n' newline character before the '\0' null character...this hampers the formatting of the log...so this line removes that newline character.
	strcpy(res->date, now_str);


}

int doGet(struct Request *request, struct Response *response, int sock_fd)
{
	char *reqFile = strcat(DocumentRoot, request->path);
	int fd = open(reqFile, O_RDONLY);
	if(fd < 0)
	{
		strcpy(response->status_code,"404");
		strcpy(response->status, "Not Found");
		writeHeaders(response, sock_fd);
		char path[PATH_LEN];
		strcpy(path, ServerRoot);
		strcat(path, "error_pages/404.html");
		fd = open(path, O_RDONLY);
		if(fd < 0)
		{
			char m[MSG_LEN];
			strcpy(m, "Error page 404.html not found");
			
			writeLog(msqid, m, LEVEL_SERVER_ERROR_LOG);
		}

		//return NOT_FOUND;
	}
	else 
	{
		printf("%s %d\n",reqFile, fd );

		struct stat filestats;
		int s = fstat(fd, &filestats);
		if(s < 0)
		{
			//some errors could occur...manage them
		}
		strcpy(response->status_code,"200");
		strcpy(response->status, "OK");
		struct tm *lm_modi =  gmtime(&(filestats.st_mtime));
		char *last_modified = asctime(lm_modi);
		last_modified[strlen(last_modified)-1] = '\0'; ////this is done because very strangely the time function asctime() adds a '\n' newline character before the '\0' null character...this hampers the formatting of the log...so this line removes that newline character.
		struct Header* tail;
		response->headers = createHeader("Last-Modified", last_modified);
		tail = response->headers;
		char content_length[SMALL_BUF_LEN];
		sprintf(content_length, "%ld", filestats.st_size);
		tail->next = createHeader("Content-Length", content_length);
		tail = tail->next;
		writeHeaders(response, sock_fd);
	}
	int rd;
	char buf[BUFFER_LEN];
	while((rd = read(fd, buf, BUFFER_LEN)) > 0)
	{
		write(1, buf, rd);
		rd = write(sock_fd, buf, rd);

		if(rd < 0)
		{
			//manage errors...
		}
	}
	if(rd < 0)
	{
		//again manage local file read errors....
	}

}


int methodNotAllowed(struct Request *request, struct Response *response, int sock_fd)
{
	strcpy(response->status_code,"405");
	strcpy(response->status, "Method Not Allowed");
	writeHeaders(response, sock_fd);
	char path[PATH_LEN];
	strcpy(path, ServerRoot);
	strcat(path, "error_pages/405.html");
	int fd = open(path, O_RDONLY);
	if(fd < 0)
	{
		char m[MSG_LEN];
		strcpy(m, "Error page 405.html not found");
		
		writeLog(msqid, m, LEVEL_SERVER_ERROR_LOG);
	}
	int rd;
	char buf[BUFFER_LEN];
	while((rd = read(fd, buf, BUFFER_LEN)) > 0)
	{
		write(1, buf, rd);
		rd = write(sock_fd, buf, rd);

		if(rd < 0)
		{
			//manage errors...
		}
	}
	if(rd < 0)
	{
		//again manage local file read errors....
	}

}

struct Header* createHeader(char *name, char *value)
{
	struct Header *header  = malloc(sizeof(struct Header));
	strcpy(header->name, name);
	strcpy(header->value, value);
	header->next = NULL;
	return header;
}
int writeHeaders(struct Response *res, int fd)
{
	char buf[MED_BUF_LEN];
	sprintf(buf, "%s/%1.1f %s %s\r\n", res->protocol, res->protocolversion, res->status_code, res->status);
	write(fd, buf, strlen(buf));
	write(1, buf, strlen(buf));
	sprintf(buf, "Server: %s\r\n", res->server_name);
	write(fd, buf, strlen(buf));
	write(1, buf, strlen(buf));
	sprintf(buf, "Date: %s\r\n", res->date);
	write(fd, buf, strlen(buf));
	write(1, buf, strlen(buf));
	struct Header *curr = res->headers;
	while(curr != NULL)
	{
		sprintf(buf,"%s: %s\r\n", curr->name, curr->value);
		write(fd, buf, strlen(buf));
		write(1, buf, strlen(buf));
		curr = curr->next;

	}
	sprintf(buf,"\r\n");
	write(fd, buf, strlen(buf));
	write(1, buf, strlen(buf));

}