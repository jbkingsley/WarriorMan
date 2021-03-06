#ifndef WM_CONFIG_H_
#define WM_CONFIG_H_

#define PHP_WORKERMAN_VERSION "0.1.0"

enum wmSocket_type {
	WM_SOCK_TCP = 1, //
	WM_SOCK_UDP = 2, //
};

enum wmSocketError_type {
	WM_SOCKET_SUCCESS = 0, //
	WM_SOCKET_CLOSE = -1, //
	WM_SOCKET_ERROR = -2, //
};

enum wmLoop_type {
	WM_LOOP_AUTO = 1, // 默认是全自动resume和yield，每次都自动添加和删除事件
	WM_LOOP_SEMI_AUTO = 2, //  send的时候默认resume和yield，read的监听事件需要自己添加
};

enum wmChannel_opcode {
	CHANNEL_PUSH = 1, //
	CHANNEL_POP = 2,
};

/**
 * epoll各种监听事件
 */
enum wmEvent_type {
	WM_EVENT_NULL = 0, //
	WM_EVENT_READ = 1u << 9, //
	WM_EVENT_WRITE = 1u << 10, //
	WM_EVENT_EPOLLEXCLUSIVE = 1u << 11, //防止惊群效应, 不使用
};

/**
 * worker的状态
 */
enum wmWorker_status {
	WM_WORKER_STATUS_STARTING = 1, //
	WM_WORKER_STATUS_RUNNING = 2, //
	WM_WORKER_STATUS_SHUTDOWN = 4, //
	WM_WORKER_STATUS_RELOADING = 8, //
};

//worker_worker.h
#define WM_KILL_WORKER_TIMER_TIME 2000  //2000毫秒后杀死进程

/**
 * Connection的状态
 */
enum wmConnection_status {
	WM_CONNECTION_STATUS_INITIAL = 0, //没用上，作为客户端使用
	WM_CONNECTION_STATUS_CONNECTING = 1, //没用上，作为客户端使用
	WM_CONNECTION_STATUS_ESTABLISHED = 2, //刚建立
	WM_CONNECTION_STATUS_CLOSING = 4, //不用
	WM_CONNECTION_STATUS_CLOSED = 8 //已关闭
};

//worker connection
#define WM_MAX_SEND_BUFFER_SIZE 102400 //默认应用层发送缓冲区大小  1M
#define WM_MAX_PACKAGE_SIZE 1024000   //每个连接能够接收的最大包包长 10M

//coroutine.h 默认的PHP栈页大小
#define DEFAULT_PHP_STACK_PAGE_SIZE       8192
#define PHP_CORO_TASK_SLOT ((int)((ZEND_MM_ALIGNED_SIZE(sizeof(wmCoroutine)) + ZEND_MM_ALIGNED_SIZE(sizeof(zval)) - 1) / ZEND_MM_ALIGNED_SIZE(sizeof(zval))))
#define DEFAULT_C_STACK_SIZE          (2 *1024 * 1024)

#define WM_MAXEVENTS            1024   //每次epoll可以返回的事件数量上限
#define WM_BUFFER_SIZE_BIG         65536 //默认一次从管道中读字节长度
#define WM_BUFFER_SIZE_DEFAULT         512 //初始化的时候的长度
#define WM_DEFAULT_BACKLOG	102400	//默认listen的时候backlog最大长度，也就是等待accept的队列最大长度

//file
#define WM_MAX_FILE_CONTENT        (64*1024*1024) //文件最大字节数

//array
#define WM_ARRAY_PAGE_MAX  1024 //wmArray默认的page数是多少，每一次扩展都会申请一页的内存

//socket
#define WM_SOCKET_MAX_TIMEOUT 2147483647 //
#define WM_SOCKET_DEFAULT_CONNECT_TIMEOUT 1000 //

#define wm_malloc              malloc
#define wm_free                free
#define wm_calloc              calloc
#define wm_realloc             realloc

#endif /* WM_CONFIG_H_ */
