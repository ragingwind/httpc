#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "http.h"

void http_url_init(http_url* u) { memset(u, 0, sizeof(http_url)); }

int http_url_set(http_url *u, const char *ustr) {
	char *ptr = strstr(ustr, "://");
	http_url_init(u);

	if (ptr) {
		strncpy(u->protocol, ustr, ptr - ustr);
		ptr += 3;
	}
	else {
		strncpy(u->protocol, "http", strlen("http"));
		ptr = (char*) ustr;
	}

	// find / to get last-pos of hostname
	if (strstr(ptr, "/")) {
		char *path = strstr(ptr, "/");
		size_t path_len = strlen(path);
		strncpy(u->host, ptr, path - ptr);
		ptr = strstr(ptr, "/");

		if (path_len > 0)
			strncpy(u->path, path, path_len);
	}
	else {
		// url-string has a only hostname
		strncpy(u->host, ptr, strlen(ptr));
		strncpy(u->path, "/", strlen("/"));
	}

	return 0;
}


void http_socket_printaddress(struct hostent *h) {
	unsigned int i = 0;
	printf("%s:\n", h->h_name);
	while (h->h_addr_list[i] != NULL) {
		printf("\t%s\n", inet_ntoa( *(struct in_addr*)(h->h_addr_list[i]) ));
		i++;
	}
}

int http_socket_connect(http *h) {
	http_socket* sock = &h->socket;
	sock->socket = socket(AF_INET, SOCK_STREAM, 0);
	if (sock->socket < 0)
		return -1;

	struct hostent *host = gethostbyname(h->url.host);
	if (!host)
		return -1;

	memset(&sock->serveraddr, 0, sizeof(sock->serveraddr));
	sock->serveraddr.sin_family = AF_INET;
	strncpy((char*) &sock->serveraddr.sin_addr.s_addr, (char*) host->h_addr, host->h_length);
	sock->serveraddr.sin_port = htons(h->port);

	if (connect(sock->socket, (struct sockaddr*) &sock->serveraddr, sizeof(sock->serveraddr)) < 0)
		return -1;

	return 0;
}

int http_socket_send(http *h) {
	return send(h->socket.socket, h->socket.sendbuf, strlen(h->socket.sendbuf), 0);
}

int http_socket_recv(http *h) {
	memset(h->socket.recvbuf, 0, sizeof(h->socket.recvbuf));
	return recv(h->socket.socket, h->socket.recvbuf, sizeof(h->socket.recvbuf), 0);
}

void http_socket_close(http *h) {
	shutdown(h->socket.socket, 0);
	h->socket.socket = 0;
}

int http_create(http *h, const char *url, int port) {
	memset(h, 0, sizeof(http));
	if (http_url_set(&h->url, url) < 0)
		return -1;

	h->port = port;

	return 0;
}

void http_release_headers(http_header** hh) {
	http_header* header = *hh;
	while (header) {
		http_header* rh = header;
		header = header->sibling;
		free(rh);
		rh = null;
	}
}

void http_destroy(http* h) {
	// remove all headers
	http_release_headers(&h->request.headers);
	http_release_headers(&h->response.headers);
	memset(h, 0, sizeof(http));
}

void http_destory_and_release(http** h) {
	http_destroy(*h);
	free(*h);
	*h = null;
}

http_header* http_header_create(const char* field, const char* value) {
	http_header* p = (http_header*) malloc(sizeof(http_header));
	memset(p, 0, sizeof(http_header));
	strcpy(p->field, field);
	strcpy(p->value, value);
	return p;
}

int http_add_header(http_header** headers, const char* field, const char* val) {
	http_header* header = *headers;

	if (header) {
		while (header) {
			if (strcmp(header->field, field) == 0) {
				strcpy(header->value, val);
				break;
			}
			else if (!header->sibling) {
				header->sibling = http_header_create(field, val);
				break;
			}
				header = header->sibling;
			}
		}
		else {
			*headers = http_header_create(field, val);
		}

	return 0;
}

http_header* http_get_header(http_header* hh, const char* field) {
	http_header* header = hh;
	while (header) {
		if (strcasecmp(header->field, field) == 0)
			break;
		header = header->sibling;
	}
	return header;
}

int http_write_request_header(http* h, const char* method) {
	char* sendbuf = h->socket.sendbuf;
	http_header* header = h->request.headers;

	// write method
	strcat(sendbuf, method);
	strcat(sendbuf, " ");
	strcat(sendbuf, h->url.path);
	strcat(sendbuf, " HTTP/1.1\n");

	// write headers
	while (header) {
		strcat(sendbuf, header->field);
		strcat(sendbuf, ": ");
		strcat(sendbuf, header->value);
		strcat(sendbuf, "\n");
		header = header->sibling;
	}

	strcat(sendbuf, "\n\n");
	return 0;
}

int http_set_response_header(http* h) {
	char line[2048];
	char* buf = h->socket.recvbuf;
	int inc = 0;
	int pos = 0;

	pos = strcspn(buf, "\n");
	while (pos > 0) {
		strncpy(line, buf + inc , pos - 1);
		line[pos - 1] = '\0';
		inc += pos + 1;

		printf("'%s'\n", line);

		unsigned int delimiter = strcspn(line, ":");

		char field[http_header_filed_maxsize + 1];
		char value[http_header_value_maxsize + 1];

		if (delimiter > 0 && delimiter < strlen(line)) {
			memset(field, 0, sizeof(field));
			memset(value, 0, sizeof(value));

			strncpy(field, line, delimiter);
			strcpy(value, line + delimiter + 2);


			printf("header added: %s, %s\n", field, value);
			http_add_header(&h->response.headers, field, value);
		}
		else {
			if (strstr(line, "HTTP") == line) {
				char status[4];
				memset(status, 0, sizeof(status));
				strncpy(status, line + strcspn(line, " "), sizeof(status));
				h->response.status = atoi(status);

				printf("\nHTTP status code: %d", h->response.status);
			}
		}

		pos = strcspn(buf + inc, "\n");
	}

	return 0;
}
