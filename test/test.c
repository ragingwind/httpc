#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/http.h"

#define terror(x, e) {if ((x) < 0) {perror(e);}}
#define terror_break(x, e) {if ((x) < 0) {perror(e);break;}}
#define tassert(x, e) {if (!(x)) {perror(e);}}

void test_http() {
	char* url = "http://www.google.com";
	http h;

	// create http
	terror(http_create(&h, url, 80), "http request");

	// add headers with host
	terror(http_add_header(&h.request.headers, "User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_7_3) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.56 Safari/535.11"), "http request add header");
	terror(http_add_header(&h.request.headers, "Connection", "keep-alive"), "http request add header");
	terror(http_add_header(&h.request.headers, "Range", "bytes=0-"), "http request add header");
	terror(http_add_header(&h.request.headers, "Accept", "*/*"), "http request add header");
	terror(http_add_header(&h.request.headers, "Host", h.url.host), "http header host");
	terror(http_add_header(&h.request.headers, "Accept", "*/*"), "http request add header");

	printf("http headers\n----------------------------------------\n");
	size_t count = 0;
	http_header* header = h.request.headers;
	while (header) {
		printf("[%s]: %s\n", header->field, header->value);
		header = header->sibling;
		count++;
	}

	tassert(count == 5, "header count is invalid");

	// write headers
	terror(http_write_request_header(&h, "GET"), "header write");

	printf("\nhttp request header\n----------------------------------------\n");
	printf("%s", h.socket.sendbuf);

	// connect and get data
	terror(http_socket_connect(&h), "http connect");

	// send http header
	terror(http_socket_send(&h), "http send error");

	// recv http header
	terror(http_socket_recv(&h), "http response error");
	printf("\nhttp response header\n----------------------------------------\n");
	printf("%s", h.socket.recvbuf);


	// set and get response header
	http_set_response_header(&h);

	printf("http response headers list\n----------------------------------------\n");
	count = 0;
	header = h.response.headers;
	while (header) {
		printf("[%s]: %s\n", header->field, header->value);
		terror(strcasecmp(header->value, http_get_header(header, header->field)->value), "invalid response header");
		header = header->sibling;
		count++;
	}

	// recv other data from server
	int recv = 0;
	FILE *fp = fopen(".tmp/download.mp4", "wb");
	while ((recv = http_socket_recv(&h)) > 0) {
		printf("recv data size: %d\n", recv);
		fwrite(h.socket.recvbuf, recv, sizeof(char), fp);
	}
	fclose(fp);

	// close socket
	http_socket_close(&h);

	// double free for testing
	http_destroy(&h);
	http_destroy(&h);


	printf("%s completed", __FUNCTION__);
}


void test_url_set(char *ustr) {
	http_url u;
	http_url_set(&u, ustr);
	printf("o:%s, p:%s, h:%s, path:%s\n\n", ustr, u.protocol, u.host, u.path);
}

void test_urls() {
	char* url1 = "http://www.host.com/path";
	char* url2 = "www.host.com/path";
	char* url3 = "http://www.host.com/";
	char* url4 = "http://www.host.com";

	test_url_set(url1);
	test_url_set(url2);
	test_url_set(url3);
	test_url_set(url4);
}

int main (int argc, char const *argv[])
{
	test_urls();
	test_http();
	return 0;
}
