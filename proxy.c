#include <stdio.h>
#include "csapp.h"

#define MAX_OBJECT_SIZE 7204056
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
/*Initialize all  the headers*/
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";
static const char *request_line_header = "GET %s HTTP/1.0\r\n";
static const char *host_header = "Host: %s\r\n";
static const char *connection_identifier = "Connection";
static const char *proxy_connection_identifier = "Proxy-Connection";
static const char *user_agent_identifier = "User-Agent";

/*Initialize the cache*/
typedef struct cache_block{
  char url[MAXLINE];
  char response_header[MAXLINE];
  char object[MAX_OBJECT_SIZE];
  ssize_t header_size;
  ssize_t object_size;
} cache_block;

struct cache_block cacheNode;

void doit(int fd);
void parse_uri(char *uri, char *hostname, char *path, char *port);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
    char *longmsg);
void create_header(char *server_header, char *hostname, char *path,
    char *port, rio_t *rio);
int connect_server(char *hostname, char *port);

/*Main function, nearly identical to the main function of tiny.c*/
int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  
  if(argc != 2){
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  signal(SIGPIPE, SIG_IGN);
  listenfd = open_listenfd(argv[1]);
  while(1){
    clientlen = sizeof(clientaddr);
    connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
    getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE,
        port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);
    close(connfd);
  }
}

/*
 * Performs the action of a proxy by first checking if the requested uri
 * is equal to the uri stored in the cache. If so, the file it pulled from
 * the cache and displayed to the user. Otherwise, the program parses the
 * uri, connects to the server, receives the header and file, and sends them
 * to the user. The attempted implemenation of a cache was commented out
 * as I was unable to successfully finish it in time.
 */
void doit(fd){
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char hostname[MAXLINE], path[MAXLINE], port[MAXLINE];
  char server_header[MAXLINE];
  int server_fd;
  rio_t rio, server_rio;
  ssize_t bytes;
  ssize_t file_size;

  rio_readinitb(&rio, fd);
  if(!rio_readlineb(&rio, buf, MAXLINE)){
    return;
  }

  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  if(strcasecmp(method, "GET")){
    clienterror(fd, method, "501", "Not Implemented",
        "Proxy does not implement this method");
    return;
  }
  if(!strcmp(cacheNode.url, uri)){
   // rio_writen(fd, cacheNode.object, cacheNode.object_size);
  }
  else{
    parse_uri(uri, hostname, path, port);
    create_header(server_header, hostname, path, port, &rio);
    server_fd = connect_server(hostname, port);
    if(server_fd < 0){
      printf("Failed to connect to server\n");
      return;
    }
    rio_readinitb(&server_rio, server_fd);
    rio_writen(server_fd, server_header, strlen(server_header));
    while((bytes = rio_readlineb(&server_rio, buf, MAXLINE)) != 0){
      rio_writen(fd, buf, bytes);
      file_size = file_size + bytes;
     // memcpy(cacheNode.object, buf, bytes);
    }
    //if(file_size <= MAX_OBJECT_SIZE){
      //  memcpy(cacheNode.url, uri, strlen(uri));
        //memcpy(cacheNode.response_header, server_header, strlen(server_header));
       // cacheNode.object_size = file_size;
        //cacheNode.header_size = strlen(server_header);
      //}
    close(server_fd);
  }
}

/*
 * Parses through the uri and returns the hostname, path, and port
 * entered by the user. It does so by setting to pointers to the
 * beginning and end of each of the hostname, path, and port. It
 * then changes the first character after that to a null terminator.
 * The string up to the null terminator is then copied to the proper
 * char *, and the null terminator is turned back into the character
 * is originally was.
 */
void parse_uri(char *uri, char *hostname, char *path, char *port){
  char *ptr;
  char *ptr_2;
  *port = 80;

  ptr = strstr(uri, "//");
  ptr = ptr + 2;
  ptr_2  = strstr(ptr, ":");
  if(ptr_2 != NULL){
    *ptr_2 = '\0';
    sscanf(ptr, "%s", hostname);
    *ptr_2 = ':';
    ptr_2 = ptr_2 + 1;
    ptr = strstr(ptr_2, "/");
    *ptr = '\0';
    sscanf(ptr_2, "%s", port);
    *ptr = '/';
    sscanf(ptr, "%s", path);
  }
  else{
    ptr_2 = strstr(ptr, "/");
    if(ptr_2 != NULL){
      ptr_2 = "\0";
      sscanf(ptr, "%s", hostname);
      ptr_2 = "/";
      sscanf(ptr_2, "%s", path);
    }
    else{
      sscanf(ptr, "%s", hostname);
    }
  }
  return;
}

/*
 * Connects to the server and returns the fd of 
 * the server to the doit function.
 */
int connect_server(char *hostname, char *port){
  int server_fd;
  server_fd = open_clientfd(hostname, port);
  return server_fd;
}

/*Constructs the server header to be returned to the client.*/
void create_header(char *server_header, char *hostname, char *path, char *port,
    rio_t *rio){
  char buf[MAXLINE], request_header[MAXLINE], header[MAXLINE],
    hostname_header[MAXLINE];
  sprintf(request_header, request_line_header, path);
  while(rio_readlineb(rio, buf, MAXLINE) > 0){
    if(strcmp(buf, "\r\n") == 0){
      break;
    }
    if(!strncasecmp(buf, "Host", strlen("Host"))){
      strcpy(hostname_header, buf);
      continue;
    }
    if(!strncasecmp(buf, connection_identifier, strlen(connection_identifier))
        && !strncasecmp(buf, proxy_connection_identifier,
            strlen(proxy_connection_identifier))
        && !strncasecmp(buf, user_agent_identifier,
            strlen(user_agent_identifier))){
      strcat(header, buf);
    }
  }
  if(strlen(hostname_header) == 0){
    sprintf(hostname_header, host_header, hostname);
  }
  sprintf(server_header, "%s%s%s%s%s%s%s", request_header, hostname_header,
      connection_hdr, proxy_connection_hdr, user_agent_hdr, header,
      "\r\n");
  return;
}

/*Client error implemenation from tiny.c*/
void clienterror(int fd, char *cause, char *errnum,
    char *shortmsg, char *longmsg){
  char buf[MAXLINE], body[MAXLINE];

  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  sprintf(buf, "%sContent-type: text/html\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n\r\n", buf, (int)strlen(body));
  rio_writen(fd, buf, strlen(buf));
  rio_writen(fd, body, strlen(body));
}
