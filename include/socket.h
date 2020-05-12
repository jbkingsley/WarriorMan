#ifndef _SOCKET_H
#define _SOCKET_H

#include "header.h"

enum wmSocket_type {
	WM_SOCK_TCP = 1, WM_SOCK_UDP = 2,
};

/**
 * 创建套接字
 */
int wmSocket_create(int type);

int wmSocket_listen(int sock);

/**
 * 对bind()函数进行了封装
 */
int wmSocket_bind(int sock, int type, char *host, int port);

/**
 * 获取客户端连接
 */
int wmSocket_accept(int sock);

/**
 * 获取客户端数据
 */
ssize_t wmSocket_recv(int sock, void *buf, size_t len, int flag);

/**
 * 发送数据
 */
ssize_t wmSocket_send(int sock, void *buf, size_t len, int flag);

#endif
