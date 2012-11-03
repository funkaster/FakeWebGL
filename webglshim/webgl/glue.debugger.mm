#include <pthread.h>
#include <vector>
#include <errno.h>
#include <string>
#include <map>
#include <sys/socket.h>
#include <netdb.h>
#include "glue.h"

static bool __pthread_initialized = false;
static pthread_attr_t __attr;

typedef std::vector<pthread_t*> pthread_vec;
pthread_vec threads;
// maps ports and sockets
std::map<int,int> ports_sockets;

// THIS IS NOT WORKING PROPERLY
void* bgTask(void* callback)
{
	@autoreleasepool {
		JSObject* obj = (JSObject*)callback;
		JSContext* cx = getGlobalContext();
		JS_BeginRequest(cx);
		JS_AddNamedObjectRoot(cx, &obj, "bg_callback");
		JS_EndRequest(cx);

		jsval fval = OBJECT_TO_JSVAL(obj);
		jsval out;
		JS_CallFunctionValue(cx, NULL, fval, 0, NULL, &out);

		JS_BeginRequest(cx);
		JS_RemoveObjectRoot(cx, &obj);
		JS_EndRequest(cx);
	}
	return NULL;
}

JSBool jsGetScript(JSContext* cx, unsigned argc, jsval* vp)
{
	jsval* argv = JS_ARGV(cx, vp);
	if (argc == 1 && argv[0].isString()) {
		JSString* str = JSVAL_TO_STRING(argv[0]);
		const char* cstr = JS_EncodeString(cx, str);
		JSScript* script = getScript(std::string(cstr));
		jsval out = OBJECT_TO_JSVAL((JSObject*)script);
		JS_SET_RVAL(cx, vp, out);
		JS_free(cx, (void*)cstr);
	}
	return JS_TRUE;
}

// open a socket, bind it to a port and start listening, all at once :)
JSBool jsSocketOpen(JSContext* cx, unsigned argc, jsval* vp)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		int port = JSVAL_TO_INT(argv[0]);
		JSObject* callback = JSVAL_TO_OBJECT(argv[1]);
		
		int s;
		s = ports_sockets[port];
		if (!s) {
			char myname[256];
			struct sockaddr_in sa;
			struct hostent *hp;
			memset(&sa, 0, sizeof(struct sockaddr_in));
			gethostname(myname, 256);
			hp = gethostbyname(myname);
			sa.sin_family = hp->h_addrtype;
			sa.sin_port = htons(port);
			if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
				JS_ReportError(cx, "error opening socket");
				return JS_FALSE;
			}
			int optval = 1;
			if ((setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) < 0) {
				close(s);
				JS_ReportError(cx, "error setting socket options");
				return JS_FALSE;
			}
			if ((bind(s, (const struct sockaddr *)&sa, sizeof(struct sockaddr_in))) < 0) {
				close(s);
				JS_ReportError(cx, "error binding socket");
				return JS_FALSE;
			}
			listen(s, 1);
			int clientSocket;
			if ((clientSocket = accept(s, NULL, NULL)) > 0) {
				ports_sockets[port] = clientSocket;
				jsval fval = OBJECT_TO_JSVAL(callback);
				jsval jsSocket = INT_TO_JSVAL(clientSocket);
				jsval outVal;
				JS_CallFunctionValue(cx, NULL, fval, 1, &jsSocket, &outVal);
			}
		} else {
			// just call the callback with the client socket
			jsval fval = OBJECT_TO_JSVAL(callback);
			jsval jsSocket = INT_TO_JSVAL(s);
			jsval outVal;
			JS_CallFunctionValue(cx, NULL, fval, 1, &jsSocket, &outVal);
		}
		JS_SET_RVAL(cx, vp, INT_TO_JSVAL(s));
	}
	return JS_TRUE;
}

JSBool jsSocketRead(JSContext* cx, unsigned argc, jsval* vp)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		int s = JSVAL_TO_INT(argv[0]);
		char buff[1024];
		JSString* outStr = JS_NewStringCopyZ(cx, "");

		size_t bytesRead;
		while ((bytesRead = read(s, buff, 1024)) > 0) {
			JSString* newStr = JS_NewStringCopyN(cx, buff, bytesRead);
			outStr = JS_ConcatStrings(cx, outStr, newStr);
			// break on new line
			if (buff[bytesRead-1] == '\n') {
				break;
			}
		}
		JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(outStr));
	} else {
		JS_SET_RVAL(cx, vp, JSVAL_NULL);
	}
	return JS_TRUE;
}

JSBool jsSocketWrite(JSContext* cx, unsigned argc, jsval* vp)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		int s;
		const char* str;
		
		s = JSVAL_TO_INT(argv[0]);
		JSString* jsstr = JS_ValueToString(cx, argv[1]);
		str = JS_EncodeString(cx, jsstr);
		
		write(s, str, strlen(str));
		
		JS_free(cx, (void*)str);
	}
	return JS_TRUE;
}

JSBool jsSocketClose(JSContext* cx, unsigned argc, jsval* vp)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		int s = JSVAL_TO_INT(argv[0]);
		close(s);
	}
	return JS_TRUE;
}
