/*
 * simple http client for streamming
 *
 * @create: ragingwind@gmail.com
 * @version: 0.0.1
 */

#ifndef _HTTP_H_BBLAB_
#define _HTTP_H_BBLAB_

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#ifndef null
#define null NULL
#endif

// http
#define http_protocol_maxsize 16
#define http_host_maxsize 255
#define http_path_maxsize 255

typedef struct _url {
	char protocol[http_protocol_maxsize + 1];
	char host[http_host_maxsize + 1];
	char path[http_path_maxsize + 1];
} http_url;

void http_url_init(http_url* u);
int http_url_set(http_url *u, const char *ustr);

#define http_socket_buf_maxsize 2048

typedef struct _http_socket {
	int socket;
	int domain;
	int type;
	int protocol;
	struct sockaddr_in serveraddr;
	char recvbuf[http_socket_buf_maxsize];
	char sendbuf[http_socket_buf_maxsize];
} http_socket;

#define http_header_filed_maxsize 64
#define http_header_value_maxsize 128
#define http_protocol_maxsize 16

typedef struct _http_header {
	char field[http_header_filed_maxsize + 1];
	char value[http_header_value_maxsize + 1];
	struct _http_header *sibling;
} http_header;

typedef struct _http_request {
	http_header* headers;
} http_request;

typedef struct _http_response {
	int status;
	http_header* headers;
} http_response;


typedef struct _http {
	http_socket socket;
	http_url url;
	http_request request;
	http_response response;
	int port;
} http;

#ifdef __cplusplus
extern "C" {
#endif

int http_socket_connect(http *h);
int http_socket_send(http *h);
int http_socket_recv(http *h);
void http_socket_close(http *h);
void http_socket_printaddress(struct hostent *h);

int http_create(http *h, const char *url, int port);
void http_release_headers(http_header** hh);
void http_destroy(http* h);
void http_destory_and_release(http** h);
http_header* http_header_create(const char* field, const char* value);
int http_add_header(http_header** headers, const char* field, const char* val);
http_header* http_get_header(http_header* hh, const char* field);
int http_write_request_header(http* h, const char* method);
int http_set_response_header(http* h);

#ifdef __cplusplus
}
#endif

#endif // _HTTP_H_BBLAB_
