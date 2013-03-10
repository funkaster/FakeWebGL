//
//  glue.h
//  webglshim
//
//  Created by Rolando Abarca on 10/5/12.
//  Copyright (c) 2012 Rolando Abarca. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#ifndef __GLUE_JS__
#define __GLUE_JS__

#include <typeinfo>
#include <string>
#include "jsapi.h"

void basic_object_finalize(JSFreeOp *freeOp, JSObject *obj);
JSBool jsNewGlobal(JSContext* cx, unsigned argc, jsval* vp);

typedef JSBool(*js_function)(JSContext* cx, unsigned argc, jsval* vp);

JSContext* getGlobalContext();
void createJSEnvironment();
JSObject* getGlobalObject(const char* name);
bool runScript(const char *path, JSObject* glob = NULL, JSContext* cx = NULL);
bool evalString(const char *string, jsval *outVal, const char *filename);

size_t readFileInMemory(const char *path, unsigned char **buff);
jsval getRequestAnimationFrameCallback();
void setRequestAnimationFrameCallback(jsval cb);
float getDevicePixelRatio();
void getDeviceWinSize(int* width, int* height);
void setInnerWidthAndHeight(JSContext* cx, JSObject* glob, int width, int height);

// native matrix ops

// multiply 2 4x4 matrices. Store result in res
void mat4mul(float* matA, float* matB, float* res);
// multiply a 4x4 matrix with a vec3, res should be a vec3
void mat4mulvec3(float* matA, float* vec, float* res);
// translate a 4x4 matrix
void mat4translate(float* matA, float x, float y, float z);
// rotate a 4x4 matrix
void mat4rotate(float* matA, float r, float x, float y, float z);
// scale a 4x4 matrix
void mat4scale(float* matA, float scaleX, float scaleY, float scaleZ);

#pragma mark - Platform Specific

typedef enum {
	webglTouchesBegan = 1,
	webglTouchesMoved,
	webglTouchesEnded,
	webglTouchesCanceled
} webglTouchEventType;

typedef struct {
	float x;
	float y;
} webglTouch_t;

const char* getFullPathFromRelativePath(const char* path);
void injectTouches(webglTouchEventType type, webglTouch_t* touches, int count);

#pragma mark - JSStringWrapper

// just a simple utility to avoid mem leaking when using JSString
class JSStringWrapper
{
	JSString*	string;
	const char*	buffer;
public:
	JSStringWrapper() {
		buffer = NULL;
	}
	JSStringWrapper(JSString* str, JSContext* cx = NULL) {
		set(str, cx);
	}
	JSStringWrapper(jsval val, JSContext* cx = NULL) {
		set(val, cx);
	}
	~JSStringWrapper() {
		if (buffer) {
			JS_free(getGlobalContext(), (void*)buffer);
		}
	}
	void set(jsval val, JSContext* cx) {
		if (val.isString()) {
			string = val.toString();
			if (!cx) {
				cx = getGlobalContext();
			}
			buffer = JS_EncodeString(cx, string);
		} else {
			buffer = NULL;
		}
	}
	void set(JSString* str, JSContext* cx) {
		string = str;
		if (!cx) {
			cx = getGlobalContext();
		}
		buffer = JS_EncodeString(cx, string);
	}
	operator std::string() {
		return std::string(buffer);
	}
	operator char*() {
		return (char*)buffer;
	}
};

class JSBindedObject
{	
};

#pragma mark - Helpful Macros

#define JS_BINDED_CLASS_GLUE(klass) \
static JSClass js_class; \
static JSObject* js_proto; \
static JSObject* js_parent; \
static void _js_register(JSContext* cx, JSObject* global);

#define JS_BINDED_CLASS_GLUE_IMPL(klass) \
JSClass klass::js_class = {}; \
JSObject* klass::js_proto = NULL; \
JSObject* klass::js_parent = NULL; \

#define JS_BINDED_FUNC(klass, name) \
JSBool name(JSContext *cx, unsigned argc, jsval *vp)

#define JS_BINDED_CONSTRUCTOR(klass) \
static JSBool _js_constructor(JSContext *cx, unsigned argc, jsval *vp)

#define JS_BINDED_CONSTRUCTOR_IMPL(klass) \
JSBool klass::_js_constructor(JSContext *cx, unsigned argc, jsval *vp)

#define JS_BINDED_FUNC_IMPL(klass, name) \
static JSBool klass##_func_##name(JSContext *cx, unsigned argc, jsval *vp) { \
	JSObject* thisObj = JS_THIS_OBJECT(cx, vp); \
	klass* obj = (klass*)JS_GetPrivate(thisObj); \
	if (obj) { \
		return obj->name(cx, argc, vp); \
	} \
	JS_ReportError(cx, "Invalid object call for function %s", #name); \
	return JS_FALSE; \
} \
JSBool klass::name(JSContext *cx, unsigned argc, jsval *vp)

#define JS_WRAP_OBJECT_IN_VAL(klass, cobj, out) \
do { \
	JSObject *obj = JS_NewObject(cx, &klass::js_class, klass::js_proto, klass::js_parent); \
	if (obj) { \
		JS_SetPrivate(obj, cobj); \
		out = OBJECT_TO_JSVAL(obj); \
	} \
} while(0) \

#define JS_BINDED_FUNC_FOR_DEF(klass, name) \
JS_FN(#name, klass##_func_##name, 0, JSPROP_ENUMERATE | JSPROP_PERMANENT)

#define JS_BINDED_PROP_GET(klass, propName) \
JSBool _js_get_##propName(JSContext *cx, JSHandleId id, JSMutableHandleValue vp)

#define JS_BINDED_PROP_GET_IMPL(klass, propName) \
static JSBool _js_get_##klass##_##propName(JSContext *cx, JSHandleObject obj, JSHandleId id, JSMutableHandleValue vp) { \
	klass* cobj = (klass*)JS_GetPrivate(obj); \
	if (cobj) { \
		return cobj->_js_get_##propName(cx, id, vp); \
	} \
	JS_ReportError(cx, "Invalid getter call for property %s", #propName); \
	return JS_FALSE; \
} \
JSBool klass::_js_get_##propName(JSContext *cx, JSHandleId id, JSMutableHandleValue vp)

#define JS_BINDED_PROP_SET(klass, propName) \
JSBool _js_set_##propName(JSContext *cx, JSHandleId id, JSBool strict, JSMutableHandleValue vp)

#define JS_BINDED_PROP_SET_IMPL(klass, propName) \
static JSBool _js_set_##klass##_##propName(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp) { \
	klass* cobj = (klass*)JS_GetPrivate(obj); \
	if (cobj) { \
		return cobj->_js_set_##propName(cx, id, strict, vp); \
	} \
	JS_ReportError(cx, "Invalid setter call for property %s", #propName); \
	return JS_FALSE; \
} \
JSBool klass::_js_set_##propName(JSContext *cx, JSHandleId id, JSBool strict, JSMutableHandleValue vp)

#define JS_BINDED_PROP_ACCESSOR(klass, propName) \
JS_BINDED_PROP_GET(klass, propName); \
JS_BINDED_PROP_SET(klass, propName);

#define JS_BINDED_PROP_DEF_GETTER(klass, propName) \
{#propName, 0, JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED, JSOP_WRAPPER(_js_get_##klass##_##propName), NULL}

#define JS_BINDED_PROP_DEF_ACCESSOR(klass, propName) \
{#propName, 0, JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED, JSOP_WRAPPER(_js_get_##klass##_##propName), JSOP_WRAPPER(_js_set_##klass##_##propName)}

#define JS_CREATE_UINT_WRAPPED(valOut, propName, val) \
do { \
	JSObject* jsobj = JS_NewObject(cx, NULL, NULL, NULL); \
	jsval propVal = UINT_TO_JSVAL(val); \
	JS_SetProperty(cx, jsobj, "__" propName, &propVal); \
	valOut = OBJECT_TO_JSVAL(jsobj); \
} while(0)

#define JS_GET_UINT_WRAPPED(inVal, propName, out) \
do { \
	if (inVal.isObject()) {\
		JSObject* jsobj = JSVAL_TO_OBJECT(inVal); \
		jsval outVal; \
		JS_GetProperty(cx, jsobj, "__" propName, &outVal); \
		JS_ValueToECMAUint32(cx, outVal, &out); \
	} else { \
		int32_t tmp; \
		JS_ValueToInt32(cx, inVal, &tmp); \
		out = (uint32_t)tmp; \
	} \
} while (0)

#if DEBUG
#define BREAK_ON_GL_ERROR 0

#if BREAK_ON_GL_ERROR && TARGET_IPHONE_SIMULATOR
#define BREAK() { __asm__("int $3\n" : : ); }
#else
#define BREAK()
#endif

#define CHECK_GL_ERROR() ({												\
	GLenum __error = glGetError();										\
	if (__error) {														\
		printf("OpenGL error 0x%04X in %s\n", __error, __FUNCTION__);	\
		BREAK()															\
	}																	\
})
#else
#define CHECK_GL_ERROR()
#endif

#endif
