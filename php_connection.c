/**
 * worker入口文件
 */
#include "base.h"
#include "connection.h"

zend_class_entry workerman_connection_ce;
zend_class_entry *workerman_connection_ce_ptr;

//zend_object_handlers实际上就是我们在PHP脚本上面操作一个PHP对象的时候，底层会去调用的函数。
static zend_object_handlers workerman_connection_handlers;

/**
 * 通过这个PHP对象找到我们的wmConnectionObject对象的代码
 */
wmConnectionObject* wm_connection_fetch_object(zend_object *obj) {
	return (wmConnectionObject*) ((char*) obj - workerman_connection_handlers.offset);
}

/**
 * 创建一个php对象
 * zend_class_entry是一个php类
 */
zend_object* wm_connection_create_object(zend_class_entry *ce) {
	wmConnectionObject *sock = (wmConnectionObject*) ecalloc(1, sizeof(wmConnectionObject) + zend_object_properties_size(ce));
	zend_object_std_init(&sock->std, ce);
	object_properties_init(&sock->std, ce);
	sock->std.handlers = &workerman_connection_handlers;
	return &sock->std;
}

/**
 * 释放php对象的方法
 */
static void wm_connection_free_object(zend_object *object) {
	wmConnectionObject *sock = (wmConnectionObject*) wm_connection_fetch_object(object);
	//这里需要判断，这个connection是不是被人继续用了。
	if (sock->connection && sock->connection != NULL) {
		//现在有这个时候，把别的正常的fd关闭的情况
		wmConnection_free(sock->connection);
		sock->connection = NULL;
	}
	//free_obj
	zend_object_std_dtor(&sock->std);
}

//空的参数声明
ZEND_BEGIN_ARG_INFO_EX(arginfo_workerman_connection_void, 0, 0, 0) //
ZEND_END_ARG_INFO()

//发送数据方法
ZEND_BEGIN_ARG_INFO_EX(arginfo_workerman_connection_send, 0, 0, 2) //
ZEND_ARG_INFO(0, data)
ZEND_ARG_INFO(0, raw)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_workerman_connection_close, 0, 0, 1) //
ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_workerman_connection_set, 0, 0, 1) //
ZEND_ARG_INFO(0, options) //
ZEND_END_ARG_INFO()

//consumeRecvBuffer截取readbuffer
ZEND_BEGIN_ARG_INFO_EX(arginfo_workerman_connection_consumeRecvBuffer, 0, 0, 1) //
ZEND_ARG_INFO(0, length)
ZEND_END_ARG_INFO()

/**
 * 设置connection属性
 */
PHP_METHOD(workerman_connection, set) {
	zval *options = NULL;
	ZEND_PARSE_PARAMETERS_START(1, 1)
				Z_PARAM_ARRAY(options)
			ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);
	wmConnectionObject *connection_object = (wmConnectionObject*) wm_connection_fetch_object(Z_OBJ_P(getThis()));

	//解析options
	HashTable *vht = Z_ARRVAL_P(options);
	zval *ztmp = NULL;

	//maxSendBufferSize
	if (php_workerman_array_get_value(vht, "maxSendBufferSize", ztmp)) {
		zend_long v = zval_get_long(ztmp);
		connection_object->connection->maxSendBufferSize = v;
		connection_object->connection->socket->maxSendBufferSize = v;
		zend_update_property_long(workerman_connection_ce_ptr, getThis(), ZEND_STRL("maxSendBufferSize"), v);
	}

	//maxPackageSize
	if (php_workerman_array_get_value(vht, "maxPackageSize", ztmp)) {
		zend_long v = zval_get_long(ztmp);
		connection_object->connection->maxPackageSize = v;
		zend_update_property_long(workerman_connection_ce_ptr, getThis(), ZEND_STRL("maxPackageSize"), v);
	}
}

/**
 * 私有方法，只有扩展内部使用
 */
PHP_METHOD(workerman_connection, read) {
	wmConnectionObject *connection_object = (wmConnectionObject*) wm_connection_fetch_object(Z_OBJ_P(getThis()));
	wmConnection *conn = connection_object->connection;
	if (conn == NULL) {
		php_error_docref(NULL, E_WARNING, "send error");
		RETURN_FALSE
	}
	wmConnection_read(conn);
	RETURN_TRUE
}

//发送数据
PHP_METHOD(workerman_connection, send) {
	wmConnectionObject *connection_object;
	wmConnection *conn;

	zend_bool raw = 0;
	char *data;
	size_t length;

	ZEND_PARSE_PARAMETERS_START(1, 2)
				Z_PARAM_STRING(data, length)
				Z_PARAM_OPTIONAL
				Z_PARAM_BOOL(raw)
			ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

	connection_object = (wmConnectionObject*) wm_connection_fetch_object(Z_OBJ_P(getThis()));
	conn = connection_object->connection;
	if (conn == NULL) {
		php_error_docref(NULL, E_WARNING, "send error , conn=null");
		RETURN_FALSE
	}
	if (!wmConnection_send(conn, data, length, raw)) {
		if(!conn->socket->closed){
			php_error_docref(NULL, E_WARNING, "send error,errno=%d", errno);
		}
		RETURN_FALSE
	}
	RETURN_TRUE
}

PHP_METHOD(workerman_connection, close) {
	int ret = 0;
	char *data = NULL;
	size_t length = 0;

	ZEND_PARSE_PARAMETERS_START(0, 1)
				Z_PARAM_OPTIONAL
				Z_PARAM_STRING(data, length)
			ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

	wmConnectionObject *connection_object = (wmConnectionObject*) wm_connection_fetch_object(Z_OBJ_P(getThis()));
	//如果用户想发送，就让他们发送
	if (data != NULL && length > 0) {
		wmConnection_send(connection_object->connection, data, length, false);
	}
	ret = wmConnection_destroy(connection_object->connection);
	if (ret < 0) {
		php_error_docref(NULL, E_WARNING, "close error");
		RETURN_FALSE
	}
	RETURN_LONG(ret);
}

PHP_METHOD(workerman_connection, destroy) {
	wmConnectionObject *connection_object = (wmConnectionObject*) wm_connection_fetch_object(Z_OBJ_P(getThis()));
	int ret = wmConnection_destroy(connection_object->connection);
	if (ret < 0) {
		php_error_docref(NULL, E_WARNING, "close error");
		RETURN_FALSE
	}
	RETURN_LONG(ret);
}

/**
 * Remove $length of data from receive buffer.
 */
PHP_METHOD(workerman_connection, consumeRecvBuffer) {
	zend_long length = 0;

	ZEND_PARSE_PARAMETERS_START(1, 1)
				Z_PARAM_LONG(length)
			ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);
	wmConnectionObject *connection_object = (wmConnectionObject*) wm_connection_fetch_object(Z_OBJ_P(getThis()));

	wmConnection_consumeRecvBuffer(connection_object->connection, length);
}

PHP_METHOD(workerman_connection, getRemoteIp) {
	wmConnectionObject *connection_object = (wmConnectionObject*) wm_connection_fetch_object(Z_OBJ_P(getThis()));
	char *ip = wmConnection_getRemoteIp(connection_object->connection);
	RETURN_STRING(ip);
}

PHP_METHOD(workerman_connection, getRemotePort) {
	wmConnectionObject *connection_object = (wmConnectionObject*) wm_connection_fetch_object(Z_OBJ_P(getThis()));
	int port = wmConnection_getRemotePort(connection_object->connection);
	RETURN_LONG(port);
}

PHP_METHOD(workerman_connection, pauseRecv) {
	wmConnectionObject *connection_object = (wmConnectionObject*) wm_connection_fetch_object(Z_OBJ_P(getThis()));
	wmConnection_pauseRecv(connection_object->connection);
}

PHP_METHOD(workerman_connection, resumeRecv) {
	wmConnectionObject *connection_object = (wmConnectionObject*) wm_connection_fetch_object(Z_OBJ_P(getThis()));
	wmConnection_resumeRecv(connection_object->connection);
}

static const zend_function_entry workerman_connection_methods[] = { //
	PHP_ME(workerman_connection, set, arginfo_workerman_connection_set, ZEND_ACC_PUBLIC) //
		//公有
		PHP_ME(workerman_connection, send, arginfo_workerman_connection_send, ZEND_ACC_PUBLIC) //
		PHP_ME(workerman_connection, close, arginfo_workerman_connection_close, ZEND_ACC_PUBLIC) //
		PHP_ME(workerman_connection, destroy, arginfo_workerman_connection_void, ZEND_ACC_PUBLIC) //
		PHP_ME(workerman_connection, consumeRecvBuffer, arginfo_workerman_connection_consumeRecvBuffer, ZEND_ACC_PUBLIC) //
		PHP_ME(workerman_connection, getRemoteIp, arginfo_workerman_connection_void, ZEND_ACC_PUBLIC) //
		PHP_ME(workerman_connection, getRemotePort, arginfo_workerman_connection_void, ZEND_ACC_PUBLIC) //
		PHP_ME(workerman_connection, pauseRecv, arginfo_workerman_connection_void, ZEND_ACC_PUBLIC) //
		PHP_ME(workerman_connection, resumeRecv, arginfo_workerman_connection_void, ZEND_ACC_PUBLIC) //

		//私有
		PHP_ME(workerman_connection, read, arginfo_workerman_connection_void, ZEND_ACC_PRIVATE) //
		PHP_FE_END };

/**
 * 注册我们的WorkerMan\Server这个类
 */
void workerman_connection_init() {

	//定义好一个类
	INIT_NS_CLASS_ENTRY(workerman_connection_ce, "Warriorman", "Connection\\TcpConnection", workerman_connection_methods);
	//在zedn中注册类
	workerman_connection_ce_ptr = zend_register_internal_class(&workerman_connection_ce TSRMLS_CC); // 在 Zend Engine 中注册

	//替换掉PHP默认的handler
	memcpy(&workerman_connection_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	//php对象实例化已经由我们自己的代码接管了
	workerman_connection_ce_ptr->create_object = wm_connection_create_object;
	workerman_connection_handlers.free_obj = wm_connection_free_object;
	workerman_connection_handlers.offset = (zend_long) (((char*) (&(((wmConnectionObject*) NULL)->std))) - ((char*) NULL));

	//类进行初始化的时候设置变量
	zend_declare_property_long(workerman_connection_ce_ptr, ZEND_STRL("fd"), 0, ZEND_ACC_PUBLIC);
	//worker
	zend_declare_property_null(workerman_connection_ce_ptr, ZEND_STRL("worker"), ZEND_ACC_PUBLIC);

	//注册变量和初始值
	zend_declare_property_long(workerman_connection_ce_ptr, ZEND_STRL("errCode"), 0, ZEND_ACC_PUBLIC);
	zend_declare_property_string(workerman_connection_ce_ptr, ZEND_STRL("errMsg"), "", ZEND_ACC_PUBLIC);

	//初始化发送静态缓冲区大小
	zend_declare_property_long(workerman_connection_ce_ptr, ZEND_STRL("defaultMaxSendBufferSize"), WM_MAX_SEND_BUFFER_SIZE, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC);

	//初始化静态最大包包长
	zend_declare_property_long(workerman_connection_ce_ptr, ZEND_STRL("defaultMaxPackageSize"), WM_MAX_PACKAGE_SIZE, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC);

}
