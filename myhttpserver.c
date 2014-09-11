/*
until now no http work is done...only core access logs created
to compile..should not compile the different includes separetely..should only compile this file and serverlogger.c ...

should also implement error codes..each module should return predefined errors which should be correctly handled...


*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define PORT "3490"
 // the port users will be connecting to
#define BACKLOG 10
#define NAME_LEN 256
#define BUFFER_LEN 500
#define NO_HEADERS 50
#define SMALL_BUF_LEN 20
#define MED_BUF_LEN 100

#include "initialise_server.c"
#include "httpUtilities.c"

char **header_data;
 

int main(void)
{
	header_data = malloc(NO_HEADERS * sizeof(char*));
	int status = initialise();
	if(status != 0)
	{
		perror("Server, failed to initialise: ");
		exit(1);
	}
	pid_t logger_id = fork();
	if(logger_id == 0)
	{
		execl("serverlogger.out", "serverlogger.out", NULL);
	}
	else
	{
		while(1) { // main accept() loop
			sin_size = sizeof their_addr;
			new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
			if (new_fd == -1) {
				//perror("accept");
				char m[MSG_LEN];
				strcpy(m, "Server Accept: ");
				strcat(m, strerror(errno));
				writeLog(msqid, m, LEVEL_SERVER_ERROR_LOG);
				continue;
			}
			char client_ip[INET6_ADDRSTRLEN];
			inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), client_ip, sizeof client_ip);
			printf("server: got connection from %s\n", client_ip);
			pid_t child = fork();
			if (child == 0) { // this is the child process
				close(sockfd); // child doesn't need the listener
				char *headerbuffer = malloc(BUFFER_LEN*sizeof(char));
				//memset(name,0,NAME_LEN);
				memset(headerbuffer,0,BUFFER_LEN);
				int rd;
				int check = writeLog(msqid, client_ip, LEVEL_HOST_ACCESS_LOG); //msqid defined in logs.h //initialised in initialise_server.c //writeLog function defined in httpUtilities.c
				

				FILE *c_fp = fopen("asciiheadervalues.txt", "w+");
				if(c_fp == NULL)
				{
					perror("open c_fd error: ");
					exit(1);
				}
				char **headerlines = malloc(NO_HEADERS * sizeof(char*));
				//printf("Sending file to %s\n",client_ip);
				char tempbuffer[BUFFER_LEN];
				char *pos;
				int header_length = 0;
				char *found;
				int newline_len;
				while(1)
				{

					rd = read(new_fd,tempbuffer, BUFFER_LEN-1);

					if(rd < 0)
					{
						fprintf(stderr, "Error in read() socket read error");
						char m[MSG_LEN];
						strcpy(m, "Server Socket Read: ");
						strcat(m, strerror(errno));
						writeLog(msqid, m, LEVEL_SERVER_ERROR_LOG);
					}
					tempbuffer[rd+1] = '\0';
					memcpy(headerbuffer, tempbuffer, rd);
					pos += rd;
					header_length += rd;
					headerbuffer[header_length+1] = '\0';
					found = strstr(headerbuffer, "\r\n\r\n");
					newline_len = 4;
					if(found == NULL)
					{
						printf("I was here \n");
						found = strstr(headerbuffer, "\n\n");
						newline_len = 2;

					}
					
					
					if(found != NULL)
						break;
				}
					char *buf = malloc(header_length*sizeof(char) + 1);
					strcpy(buf, headerbuffer);
					//char *end_headers = strstr(buf, "\r\n\r\n");
					char *end_headers = found;
					char *start_content = end_headers + newline_len*sizeof(char);
					*end_headers = '\0';
					char *headerdata = malloc((strlen(buf)+1)*sizeof(char));
					strcpy(headerdata,buf);
					char *content_buf = malloc((strlen(start_content)+1)*sizeof(char));
					strcpy(content_buf, start_content);
					struct Request request;
					int status = parseHeaderData(headerdata,&request); // check for errors that this function returns and log them..
					int content_length = request.content_length;
					printf("======== %d\n",content_length);
					printf("Read Headers\n");
					int i;
					for(i = 0; i < request.no_headers; i++)
					{
						printf("%s: %s\n", request.headers[i].name, request.headers[i].value);
					}
					int read_content_len = strlen(content_buf);
					printf("Content_length %d\n",content_length);
					if(content_length > 0)
					{
						//read_content_len -= 1;//remove '\0' which is put to calculate the length using strlen
						char *temp;
						printf("Read content length %d", read_content_len);
						fflush(stdout);
						
						while((read_content_len < content_length))
						{
							rd = read(new_fd,headerbuffer, BUFFER_LEN);
							printf("I was here...%d\n",read_content_len);
							fflush(stdout);
							if(rd < 0)
							{
								fprintf(stderr, "Error in read() socket read error");
								char m[MSG_LEN];
								strcpy(m, "Server Socket Read: ");
								strcat(m, strerror(errno));
								writeLog(msqid, m, LEVEL_SERVER_ERROR_LOG);
							}
							
							temp = malloc((read_content_len+rd)*sizeof(char));
							char* s_con = temp;
							memcpy(temp, content_buf, read_content_len);
							free(content_buf);
							temp += read_content_len;
							memcpy(temp, headerbuffer, rd);
							read_content_len += rd;
							content_buf = s_con;
						
						}
						
					}
					printf("Read Entire content\n");
					struct Response response;
					createDefaultResponse(&response, &request);
					if(strcmp(request.method,"GET") == 0)
					{
						doGet(&request, &response, new_fd);
					}
					else
					{
						methodNotAllowed(&request, &response, new_fd);	
					}
					// for(i=0; i < read_content_len; i++)
					// {
					// 	putchar(content_buf[i]);
					// }
					// fflush(stdout);
					// int i = 0;
					
					// for(; i < rd; i++)
					// {
							
					// }
					//write(1, headerbuffer,rd);
					// i = 0;
					// for(; i< ind; i++)
					// {
					// 	printf("%s\n", headerlines[i]);
					// }
					if(rd < BUFFER_LEN)
						break;
					//have to figure out when to stop reading from the user...aparently if there is a msg body then the content-length or transfer-encoding header fields will be set...

					// write(new_fd, buffer, rd);
					// //printf("Hello");
				
				
				//parseHeaderData(buffer);
				fflush(stdout);
				free(headerbuffer);
				for(i = 0; i < NO_HEADERS; i++)
				{
					free(headerlines[i]);
				}
				free(headerlines);
				free(header_data);
				//fclose(fp);
				//close(file_fd);
				close(new_fd);
				fclose(c_fp);
				exit(0);
			}
			close(new_fd); // parent doesn't need this
		}
	}
	return 0;	
	
}
