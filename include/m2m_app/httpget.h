
#ifndef _HTTP_GET_H_H_
#define _HTTP_GET_H_H_

typedef void (*httpget_recv_callback)(char *data,int len,void *ctx);

int http_get2(const char *host_name,uint16_t port,const char *path,httpget_recv_callback frcallback,void *ctx);


#endif

