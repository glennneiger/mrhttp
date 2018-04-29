#pragma once
#include "Python.h"
#include <stdbool.h>

#include "router.h"
#include "parser.h"
#include "request.h"
#include "response.h"
#include "memprotocol.h"

//MrMemcachedProtocol_CAPI

typedef struct {
  bool is_task;
  Request* request;
  PyObject* task; // Task if coroutine otherwise output
} PipelineRequest;

typedef struct {
  PyObject_HEAD
  PyObject* app;
  bool closed;

  PyObject* transport;
  PyObject* write;
  PyObject* writelines;
  double start_time;

  // Http 1.1 Pipeline 
  PyObject* create_task;
  PyObject* task_done; 
  PipelineRequest queue[10];
  size_t queue_start;
  size_t queue_end;

  Request *request;
  Response *response;
  Router router;
  Parser  parser;
  MemcachedProtocol *memprotocol;
} Protocol;

PyObject * Protocol_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
int Protocol_init(Protocol* self, PyObject *args, PyObject *kw);
void Protocol_dealloc(Protocol* self);


PyObject* Protocol_connection_made(Protocol* self, PyObject* transport);
void* Protocol_close(Protocol* self);
PyObject* Protocol_connection_lost(Protocol* self, PyObject* args);
PyObject* Protocol_data_received(Protocol* self, PyObject* data);
PyObject* Protocol_eof_received(Protocol* self);
PyObject* Protocol_updateDate(Protocol *self, PyObject *date);

Protocol* Protocol_on_incomplete(Protocol* self);
Protocol* Protocol_on_headers(Protocol* self, char* method, size_t method_len,
                              char* path, size_t path_len, int minor_version,
                              void* headers, size_t num_headers);
Protocol* Protocol_on_body(Protocol* self, char* body, size_t body_len);
Protocol* Protocol_on_error(Protocol* self, PyObject*);

static inline Protocol* protocol_write_response(Protocol* self, Request *req, PyObject* resp);
static inline Protocol* protocol_write_redirect_response(Protocol* self, int code, char *url);
static inline Protocol* protocol_write_error_response(   Protocol* self, int code, char *reason, char *msg);

static inline void reverse(char* begin, char* end)
{
  char t;
  while (end > begin) {
    t = *end;
    *end-- = *begin;
    *begin++ = t;
  }
}

#define PIPELINE_EMPTY(p) ((p)->queue_start == (p)->queue_end)

PyObject* protocol_task_done(Protocol* self, PyObject* task);
void* Protocol_pipeline_cancel(Protocol* self);

static inline bool pipeline_is_task(PipelineRequest r)
{
  return r.is_task;
}

static inline void pipeline_DECREF(PipelineRequest r)
{
    Py_DECREF(r.request);
    // if not real task this was response,
    // that was inside request that was already freed above
    if(r.is_task)
      Py_XDECREF(r.task);
}

static inline void pipeline_INCREF(PipelineRequest r)
{
    Py_INCREF(r.request);
    Py_XINCREF(r.task);
}

static inline PyObject* pipeline_get_task(PipelineRequest r)
{
  return r.task;
}
