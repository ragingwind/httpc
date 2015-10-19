#include <node.h>
#include <v8.h>
#include "http.h"

using namespace v8;

void get(const v8::FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
	v8::String::Utf8Value url(args[0]->ToString());
  HandleScope scope(isolate);

  http *h = (http*) malloc(sizeof(http));
  http_create(h, (const char*) *url, 80);

  http_add_header(&h->request.headers, "User-Agent", "httpc 0.1");
  http_add_header(&h->request.headers, "Connection", "keep-alive");
  http_add_header(&h->request.headers, "Range", "bytes=0-");
  http_add_header(&h->request.headers, "Accept", "*/*");
  http_add_header(&h->request.headers, "Host", h->url.host);
  http_add_header(&h->request.headers, "Accept", "*/*");

  http_write_request_header(h, "GET");

  http_socket_connect(h);
  http_socket_send(h);
	http_socket_recv(h);

  args.GetReturnValue().Set(String::NewFromUtf8(isolate, h->socket.recvbuf));

	http_destroy(h);
}

void init(Handle<Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();
  exports->Set(String::NewFromUtf8(isolate, "get"),
      FunctionTemplate::New(isolate, get)->GetFunction());
}

NODE_MODULE(binding, init);
