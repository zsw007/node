#include "node_dtrace.h"

#ifdef HAVE_DTRACE
#include "node_provider.h"
#elif HAVE_ETW
#include "node_win32_etw_provider-inl.h"
#else
#define NODE_HTTP_SERVER_REQUEST(arg0, arg1)
#define NODE_HTTP_SERVER_REQUEST_ENABLED() (0)
#define NODE_HTTP_SERVER_RESPONSE(arg0)
#define NODE_HTTP_SERVER_RESPONSE_ENABLED() (0)
#define NODE_HTTP_CLIENT_REQUEST(arg0, arg1)
#define NODE_HTTP_CLIENT_REQUEST_ENABLED() (0)
#define NODE_HTTP_CLIENT_RESPONSE(arg0)
#define NODE_HTTP_CLIENT_RESPONSE_ENABLED() (0)
#define NODE_NET_SERVER_CONNECTION(arg0)
#define NODE_NET_SERVER_CONNECTION_ENABLED() (0)
#define NODE_NET_STREAM_END(arg0)
#define NODE_NET_STREAM_END_ENABLED() (0)
#define NODE_GC_START(arg0, arg1, arg2)
#define NODE_GC_DONE(arg0, arg1, arg2)
#endif

#include "env-inl.h"

#include "util.h"

#include <string.h>

namespace node {

using v8::FunctionCallbackInfo;
using v8::GCCallbackFlags;
using v8::GCType;
using v8::HandleScope;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;

#define SLURP_STRING(obj, member, valp)                                    \
  if (!(obj)->IsObject()) {                                                \
    return env->ThrowError(                                                \
        "\x65\x78\x70\x65\x63\x74\x65\x64\x20\x6f\x62\x6a\x65\x63\x74\x20\x66\x6f\x72" USTR(#obj) "\x20\x74\x6f\x20\x63\x6f\x6e\x74\x61\x69\x6e\x20\x73\x74\x72\x69\x6e\x67\x20\x6d\x65\x6d\x62\x65\x72" USTR(#member) ); \
  }                                                                        \
  node::Utf8Value _##member(env->isolate(),                                \
      obj->Get(OneByteString(env->isolate(), #member)));                   \
  if ((*(const char **)valp = *_##member) == nullptr)                      \
    *(const char **)valp = "\x3c\x75\x6e\x6b\x6e\x6f\x77\x6e\x3e";

#define SLURP_INT(obj, member, valp)                                       \
  if (!(obj)->IsObject()) {                                                \
    return env->ThrowError(                                                \
      "\x65\x78\x70\x65\x63\x74\x65\x64\x20\x6f\x62\x6a\x65\x63\x74\x20\x66\x6f\x72" USTR(#obj) "\x20\x74\x6f\x20\x63\x6f\x6e\x74\x61\x69\x6e\x20\x69\x6e\x74\x65\x67\x65\x72\x20\x6d\x65\x6d\x62\x65\x72" USTR(#member) );  \
  }                                                                        \
  *valp = obj->Get(OneByteString(env->isolate(), #member))                 \
      ->Int32Value();

#define SLURP_OBJECT(obj, member, valp)                                    \
  if (!(obj)->IsObject()) {                                                \
    return env->ThrowError(                                                \
      "\x65\x78\x70\x65\x63\x74\x65\x64\x20\x6f\x62\x6a\x65\x63\x74\x20\x66\x6f\x72" USTR(#obj) "\x20\x74\x6f\x20\x63\x6f\x6e\x74\x61\x69\x6e\x20\x6f\x62\x6a\x65\x63\x74\x20\x6d\x65\x6d\x62\x65\x72\x20" USTR(#member) );   \
  }                                                                        \
  *valp = Local<Object>::Cast(obj->Get(OneByteString(env->isolate(), #member)));

#define SLURP_CONNECTION(arg, conn)                                        \
  if (!(arg)->IsObject()) {                                                \
    return env->ThrowError(                                                \
      "\x65\x78\x70\x65\x63\x74\x65\x64\x20\x61\x72\x67\x75\x6d\x65\x6e\x74\x20" USTR(#arg) "\x20\x74\x6f\x20\x62\x65\x20\x61\x20\x63\x6f\x6e\x6e\x65\x63\x74\x69\x6f\x6e\x20\x6f\x62\x6a\x65\x63\x74");             \
  }                                                                        \
  node_dtrace_connection_t conn;                                           \
  Local<Object> _##conn = Local<Object>::Cast(arg);                        \
  Local<Value> _handle =                                                   \
      (_##conn)->Get(FIXED_ONE_BYTE_STRING(env->isolate(), "\x5f\x68\x61\x6e\x64\x6c\x65"));    \
  if (_handle->IsObject()) {                                               \
    SLURP_INT(_handle.As<Object>(), fd, &conn.fd);                         \
  } else {                                                                 \
    conn.fd = -1;                                                          \
  }                                                                        \
  SLURP_STRING(_##conn, remoteAddress, &conn.remote);                      \
  SLURP_INT(_##conn, remotePort, &conn.port);                              \
  SLURP_INT(_##conn, bufferSize, &conn.buffered);

#define SLURP_CONNECTION_HTTP_CLIENT(arg, conn)                            \
  if (!(arg)->IsObject()) {                                                \
    return env->ThrowError(                                                \
      "\x65\x78\x70\x65\x63\x74\x65\x64\x20\x61\x72\x67\x75\x6d\x65\x6e\x74\x20" USTR(#arg) "\x20\x74\x6f\x20\x62\x65\x20\x61\x20\x63\x6f\x6e\x6e\x65\x63\x74\x69\x6f\x6e\x20\x6f\x62\x6a\x65\x63\x74");             \
  }                                                                        \
  node_dtrace_connection_t conn;                                           \
  Local<Object> _##conn = Local<Object>::Cast(arg);                        \
  SLURP_INT(_##conn, fd, &conn.fd);                                        \
  SLURP_STRING(_##conn, host, &conn.remote);                               \
  SLURP_INT(_##conn, port, &conn.port);                                    \
  SLURP_INT(_##conn, bufferSize, &conn.buffered);

#define SLURP_CONNECTION_HTTP_CLIENT_RESPONSE(arg0, arg1, conn)            \
  if (!(arg0)->IsObject()) {                                               \
    return env->ThrowError(                                                \
      "\x65\x78\x70\x65\x63\x74\x65\x64\x20\x61\x72\x67\x75\x6d\x65\x6e\x74\x20" USTR(#arg) "\x20\x74\x6f\x20\x62\x65\x20\x61\x20\x63\x6f\x6e\x6e\x65\x63\x74\x69\x6f\x6e\x20\x6f\x62\x6a\x65\x63\x74");             \
  }                                                                        \
  if (!(arg1)->IsObject()) {                                               \
    return env->ThrowError(                                                \
      "\x65\x78\x70\x65\x63\x74\x65\x64\x20\x61\x72\x67\x75\x6d\x65\x6e\x74\x20" USTR(#arg) "\x20\x74\x6f\x20\x62\x65\x20\x61\x20\x63\x6f\x6e\x6e\x65\x63\x74\x69\x6f\x6e\x20\x6f\x62\x6a\x65\x63\x74");             \
  }                                                                        \
  node_dtrace_connection_t conn;                                           \
  Local<Object> _##conn = Local<Object>::Cast(arg0);                       \
  SLURP_INT(_##conn, fd, &conn.fd);                                        \
  SLURP_INT(_##conn, bufferSize, &conn.buffered);                          \
  _##conn = Local<Object>::Cast(arg1);                                     \
  SLURP_STRING(_##conn, host, &conn.remote);                               \
  SLURP_INT(_##conn, port, &conn.port);


void DTRACE_NET_SERVER_CONNECTION(const FunctionCallbackInfo<Value>& args) {
  if (!NODE_NET_SERVER_CONNECTION_ENABLED())
    return;
  Environment* env = Environment::GetCurrent(args);
  SLURP_CONNECTION(args[0], conn);
  NODE_NET_SERVER_CONNECTION(&conn, conn.remote, conn.port, conn.fd);
}


void DTRACE_NET_STREAM_END(const FunctionCallbackInfo<Value>& args) {
  if (!NODE_NET_STREAM_END_ENABLED())
    return;
  Environment* env = Environment::GetCurrent(args);
  SLURP_CONNECTION(args[0], conn);
  NODE_NET_STREAM_END(&conn, conn.remote, conn.port, conn.fd);
}

void DTRACE_HTTP_SERVER_REQUEST(const FunctionCallbackInfo<Value>& args) {
  node_dtrace_http_server_request_t req;

  if (!NODE_HTTP_SERVER_REQUEST_ENABLED())
    return;

  Environment* env = Environment::GetCurrent(args);
  HandleScope scope(env->isolate());
  Local<Object> arg0 = Local<Object>::Cast(args[0]);
  Local<Object> headers;

  memset(&req, 0, sizeof(req));
  req._un.version = 1;
  SLURP_STRING(arg0, url, &req.url);
  SLURP_STRING(arg0, method, &req.method);
  SLURP_OBJECT(arg0, headers, &headers);

  if (!(headers)->IsObject()) {
    return env->ThrowError(
      "\x65\x78\x70\x65\x63\x74\x65\x64\x20\x6f\x62\x6a\x65\x63\x74\x20\x66\x6f\x72\x20\x72\x65\x71\x75\x65\x73\x74\x20\x74\x6f\x20\x63\x6f\x6e\x74\x61\x69\x6e\x20\x73\x74\x72\x69\x6e\x67\x20\x6d\x65\x6d\x62\x65\x72\x20\x68\x65\x61\x64\x65\x72\x73");
  }

  Local<Value> strfwdfor = headers->Get(env->x_forwarded_string());
  node::Utf8Value fwdfor(env->isolate(), strfwdfor);

  if (!strfwdfor->IsString() || (req.forwardedFor = *fwdfor) == nullptr)
    req.forwardedFor = const_cast<char*>("");

  SLURP_CONNECTION(args[1], conn);
  NODE_HTTP_SERVER_REQUEST(&req, &conn, conn.remote, conn.port, req.method, \
                           req.url, conn.fd);
}


void DTRACE_HTTP_SERVER_RESPONSE(const FunctionCallbackInfo<Value>& args) {
  if (!NODE_HTTP_SERVER_RESPONSE_ENABLED())
    return;
  Environment* env = Environment::GetCurrent(args);
  SLURP_CONNECTION(args[0], conn);
  NODE_HTTP_SERVER_RESPONSE(&conn, conn.remote, conn.port, conn.fd);
}


void DTRACE_HTTP_CLIENT_REQUEST(const FunctionCallbackInfo<Value>& args) {
  node_dtrace_http_client_request_t req;
  char *header;

  if (!NODE_HTTP_CLIENT_REQUEST_ENABLED())
    return;

  Environment* env = Environment::GetCurrent(args);
  HandleScope scope(env->isolate());

  /*
   * For the method and URL, we're going to dig them out of the header.  This
   * is not as efficient as it could be, but we would rather not force the
   * caller here to retain their method and URL until the time at which
   * DTRACE_HTTP_CLIENT_REQUEST can be called.
   */
  Local<Object> arg0 = Local<Object>::Cast(args[0]);
  SLURP_STRING(arg0, _header, &header);

  req.method = header;

  while (*header != '\x0' && *header != '\x20')
    header++;

  if (*header != '\x0')
    *header++ = '\x0';

  req.url = header;

  while (*header != '\x0' && *header != '\x20')
    header++;

  *header = '\x0';

  SLURP_CONNECTION_HTTP_CLIENT(args[1], conn);
  NODE_HTTP_CLIENT_REQUEST(&req, &conn, conn.remote, conn.port, req.method, \
                           req.url, conn.fd);
}


void DTRACE_HTTP_CLIENT_RESPONSE(const FunctionCallbackInfo<Value>& args) {
  if (!NODE_HTTP_CLIENT_RESPONSE_ENABLED())
    return;
  Environment* env = Environment::GetCurrent(args);
  SLURP_CONNECTION_HTTP_CLIENT_RESPONSE(args[0], args[1], conn);
  NODE_HTTP_CLIENT_RESPONSE(&conn, conn.remote, conn.port, conn.fd);
}


void dtrace_gc_start(Isolate* isolate, GCType type, GCCallbackFlags flags) {
  // Previous versions of this probe point only logged type and flags.
  // That's why for reasons of backwards compatibility the isolate goes last.
  NODE_GC_START(type, flags, isolate);
}


void dtrace_gc_done(Isolate* isolate, GCType type, GCCallbackFlags flags) {
  // Previous versions of this probe point only logged type and flags.
  // That's why for reasons of backwards compatibility the isolate goes last.
  NODE_GC_DONE(type, flags, isolate);
}


void InitDTrace(Environment* env, Local<Object> target) {
  HandleScope scope(env->isolate());

  static struct {
    const char *name;
    void (*func)(const FunctionCallbackInfo<Value>&);
  } tab[] = {
#define NODE_PROBE(name) USTR(#name), name
    { NODE_PROBE(DTRACE_NET_SERVER_CONNECTION) },
    { NODE_PROBE(DTRACE_NET_STREAM_END) },
    { NODE_PROBE(DTRACE_HTTP_SERVER_REQUEST) },
    { NODE_PROBE(DTRACE_HTTP_SERVER_RESPONSE) },
    { NODE_PROBE(DTRACE_HTTP_CLIENT_REQUEST) },
    { NODE_PROBE(DTRACE_HTTP_CLIENT_RESPONSE) }
#undef NODE_PROBE
  };

  for (size_t i = 0; i < arraysize(tab); i++) {
    Local<String> key = OneByteString(env->isolate(), tab[i].name);
    Local<Value> val = env->NewFunctionTemplate(tab[i].func)->GetFunction();
    target->Set(key, val);
  }

#ifdef HAVE_ETW
  init_etw();
#endif

#if defined HAVE_DTRACE || defined HAVE_ETW
  env->isolate()->AddGCPrologueCallback(dtrace_gc_start);
  env->isolate()->AddGCEpilogueCallback(dtrace_gc_done);
#endif
}

}  // namespace node
