//
//  WebGLRenderingContext.cpp
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

#include <OpenGLES/ES2/gl.h>
#include <iostream>
#include <iterator>
#include <assert.h>
#include "WebGLRenderingContext.h"
#include "jsapi.h"
#include "jsfriendapi.h"
#include "lodepng.h"


// WebGL specific defines
#define GL_UNPACK_FLIP_Y_WEBGL 0x9240
#define GL_UNPACK_PREMULTIPLY_ALPHA_WEBGL 0x9241
#define GL_CONTEXT_LOST_WEBGL 0x9242
#define GL_UNPACK_COLORSPACE_CONVERSION_WEBGL 0x9243
#define GL_BROWSER_DEFAULT_WEBGL 0x9244


#if DEBUG
#define BREAK_ON_GL_ERROR 1

#if BREAK_ON_GL_ERROR && TARGET_IPHONE_SIMULATOR
#define BREAK() { __asm__("int $3\n" : : ); }
#else
#define BREAK()
#endif

#define CHECK_GL_ERROR() ({												\
	GLenum __error = glGetError();										\
	if (__error) {														\
		printf("OpenGL error 0x%04X in %s\n", __error, __FUNCTION__);	\
		BREAK();														\
	}																	\
})
#else
#define CHECK_GL_ERROR()
#endif

using namespace std;

#pragma mark - ChesterCanvas

JS_BINDED_CLASS_GLUE_IMPL(ChesterCanvas);

JS_BINDED_CONSTRUCTOR_IMPL(ChesterCanvas)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		int w = JSVAL_TO_INT(argv[0]);
		int h = JSVAL_TO_INT(argv[1]);
		ChesterCanvas* cobj = new ChesterCanvas(w, h);
		jsval out;
		JS_WRAP_OBJECT_IN_VAL(ChesterCanvas, cobj, out);
		JS_SET_RVAL(cx, vp, out);
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_PROP_GET_IMPL(ChesterCanvas, width)
{
	vp.set(INT_TO_JSVAL(width));
	return JS_TRUE;
}

JS_BINDED_PROP_GET_IMPL(ChesterCanvas, height)
{
	vp.set(INT_TO_JSVAL(height));
	return JS_TRUE;
}

JS_BINDED_FUNC_IMPL(ChesterCanvas, getContext)
{
	if (argc >= 1) {
		jsval* argv = JS_ARGV(cx, vp);
		JSString* str = JS_ValueToString(cx, argv[0]);
		JSStringWrapper wrapper(str);
		JSBool ok = JS_FALSE;
		if (strncmp(wrapper, "experimental-webgl", 18) == 0) {
			WebGLRenderingContext* cobj = new WebGLRenderingContext(this);
			jsval out;
			JS_WRAP_OBJECT_IN_VAL(WebGLRenderingContext, cobj, out);
			JS_SET_RVAL(cx, vp, out);
			ok = JS_TRUE;
		}
		return ok;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

void ChesterCanvas::_js_register(JSContext* cx, JSObject* global)
{
	// create the class
	ChesterCanvas::js_class = {
		"ChesterCanvas", JSCLASS_HAS_PRIVATE,
		JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, basic_object_finalize,
		JSCLASS_NO_OPTIONAL_MEMBERS
	};
	static JSPropertySpec props[] = {
		JS_BINDED_PROP_DEF_GETTER(ChesterCanvas, width),
		JS_BINDED_PROP_DEF_GETTER(ChesterCanvas, height),
		{0, 0, 0, 0, 0}
	};
	static JSFunctionSpec funcs[] = {
		JS_BINDED_FUNC_FOR_DEF(ChesterCanvas, getContext),
		JS_FS_END
	};
	ChesterCanvas::js_parent = NULL;
	ChesterCanvas::js_proto = JS_InitClass(cx, global, NULL, &ChesterCanvas::js_class, ChesterCanvas::_js_constructor, 1, props, funcs, NULL, NULL);
}

#pragma mark - FakeXMLHTTPRequest

FakeXMLHTTPRequest::~FakeXMLHTTPRequest()
{
	if (onreadystateCallback) {
		JS_RemoveObjectRoot(getGlobalContext(), &onreadystateCallback);
	}
	if (data) {
		free(data);
	}
}

JS_BINDED_CLASS_GLUE_IMPL(FakeXMLHTTPRequest);

JS_BINDED_CONSTRUCTOR_IMPL(FakeXMLHTTPRequest)
{
	FakeXMLHTTPRequest* req = new FakeXMLHTTPRequest();
	jsval out;
	JS_WRAP_OBJECT_IN_VAL(FakeXMLHTTPRequest, req, out);
	JS_SET_RVAL(cx, vp, out);
	return JS_TRUE;
}

JS_BINDED_PROP_GET_IMPL(FakeXMLHTTPRequest, onreadystatechange)
{
	if (onreadystateCallback) {
		jsval out = OBJECT_TO_JSVAL(onreadystateCallback);
		vp.set(out);
	} else {
		vp.set(JSVAL_NULL);
	}
	return JS_TRUE;
}

JS_BINDED_PROP_SET_IMPL(FakeXMLHTTPRequest, onreadystatechange)
{
	jsval callback = vp.get();
	if (callback != JSVAL_NULL) {
		onreadystateCallback = JSVAL_TO_OBJECT(callback);
		JS_AddNamedObjectRoot(cx, &onreadystateCallback, "FakeXMLHttpRequest_callback");
	}
	return JS_TRUE;
}

JS_BINDED_PROP_GET_IMPL(FakeXMLHTTPRequest, responseType)
{
	JSString* str = JS_NewStringCopyN(cx, "", 0);
	vp.set(STRING_TO_JSVAL(str));
	return JS_TRUE;
}

JS_BINDED_PROP_SET_IMPL(FakeXMLHTTPRequest, responseType)
{
	jsval type = vp.get();
	if (type.isString()) {
		JSString* str = type.toString();
		JSBool equal;
		JS_StringEqualsAscii(cx, str, "text", &equal);
		if (equal) {
			responseType = kRequestResponseTypeString;
			return JS_TRUE;
		}
		JS_StringEqualsAscii(cx, str, "arraybuffer", &equal);
		if (equal) {
			responseType = kRequestResponseTypeArrayBuffer;
			return JS_TRUE;
		}
		JS_StringEqualsAscii(cx, str, "json", &equal);
		if (equal) {
			responseType = kRequestResponseTypeJSON;
			return JS_TRUE;
		}
		// ignore the rest of the response types for now
		return JS_TRUE;
	}
	JS_ReportError(cx, "Invalid response type");
	return JS_FALSE;
}

JS_BINDED_PROP_GET_IMPL(FakeXMLHTTPRequest, readyState)
{
	vp.set(INT_TO_JSVAL(readyState));
	return JS_TRUE;
}

JS_BINDED_PROP_GET_IMPL(FakeXMLHTTPRequest, status)
{
	vp.set(INT_TO_JSVAL(status));
	return JS_TRUE;
}

JS_BINDED_PROP_GET_IMPL(FakeXMLHTTPRequest, responseText)
{
	JSString* str = JS_NewStringCopyN(cx, (const char*)data, dataSize);
	if (str) {
		vp.set(STRING_TO_JSVAL(str));
		return JS_TRUE;
	} else {
		JS_ReportError(cx, "Error trying to create JSString from data");
		return JS_FALSE;
	}
}

JS_BINDED_PROP_GET_IMPL(FakeXMLHTTPRequest, response)
{
	if (responseType == kRequestResponseTypeJSON) {
		jsval outVal;
		JSString* str = JS_NewStringCopyN(cx, (const char*)data, dataSize);
		if (JS_ParseJSON(cx, JS_GetStringCharsZ(cx, str), dataSize, &outVal)) {
			vp.set(outVal);
			return JS_TRUE;
		}
	} else if (responseType == kRequestResponseTypeArrayBuffer) {
		JSObject* tmp = JS_NewArrayBuffer(cx, dataSize);
		uint8_t* tmpData = JS_GetArrayBufferData(tmp, cx);
		memcpy(tmpData, data, dataSize);
		jsval outVal = OBJECT_TO_JSVAL(tmp);
		vp.set(outVal);
		return JS_TRUE;
	}
	// by default, return text
	return _js_get_responseText(cx, id, vp);
}

JS_BINDED_FUNC_IMPL(FakeXMLHTTPRequest, open)
{
	if (argc >= 2) {
		jsval* argv = JS_ARGV(cx, vp);
		const char* method;
		const char* urlstr;
		JSBool async = true;
		JSString* jsMethod = JS_ValueToString(cx, argv[0]);
		JSString* jsURL = JS_ValueToString(cx, argv[1]);
		if (argc > 2) {
			JS_ValueToBoolean(cx, argv[2], &async);
		}
		JSStringWrapper w1(jsMethod);
		JSStringWrapper w2(jsURL);
		method = w1;
		urlstr = w2;

		url = urlstr;
		readyState = 1;
		isAsync = async;

		if (url.length() > 5 && url.compare(url.length() - 5, 5, ".json") == 0) {
			responseType = kRequestResponseTypeJSON;
		}
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(FakeXMLHTTPRequest, send)
{
	std::string path = getFullPathFromRelativePath(url.c_str());
	readyState = 4;
	if (path.empty()) {
		// file not found
		status = 404;
	} else {
		dataSize = readFileInMemory(path.c_str(), &data);
		if (dataSize > 0) {
			status = 200;
		} else {
			printf("Error trying to read '%s'\n", path.c_str());
			status = 404; // just issue any error
		}
	}
	if (onreadystateCallback) {
		jsval fval = OBJECT_TO_JSVAL(onreadystateCallback);
		jsval out;
		JS_CallFunctionValue(cx, NULL, fval, 0, NULL, &out);
	}
	return JS_TRUE;
}

void FakeXMLHTTPRequest::_js_register(JSContext *cx, JSObject *global)
{
	// create the class
	FakeXMLHTTPRequest::js_class = {
		"XMLHttpRequest", JSCLASS_HAS_PRIVATE,
		JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, basic_object_finalize,
		JSCLASS_NO_OPTIONAL_MEMBERS
	};
	static JSPropertySpec props[] = {
		JS_BINDED_PROP_DEF_ACCESSOR(FakeXMLHTTPRequest, onreadystatechange),
		JS_BINDED_PROP_DEF_ACCESSOR(FakeXMLHTTPRequest, responseType),
		JS_BINDED_PROP_DEF_GETTER(FakeXMLHTTPRequest, readyState),
		JS_BINDED_PROP_DEF_GETTER(FakeXMLHTTPRequest, status),
		JS_BINDED_PROP_DEF_GETTER(FakeXMLHTTPRequest, responseText),
		JS_BINDED_PROP_DEF_GETTER(FakeXMLHTTPRequest, response),
		{0, 0, 0, 0, 0}
	};
	static JSFunctionSpec funcs[] = {
		JS_BINDED_FUNC_FOR_DEF(FakeXMLHTTPRequest, open),
		JS_BINDED_FUNC_FOR_DEF(FakeXMLHTTPRequest, send),
		JS_FS_END
	};
	FakeXMLHTTPRequest::js_parent = NULL;
	FakeXMLHTTPRequest::js_proto = JS_InitClass(cx, global, NULL, &FakeXMLHTTPRequest::js_class, FakeXMLHTTPRequest::_js_constructor, 1, props, funcs, NULL, NULL);
}

#pragma mark - PNGImage

PNGImage::PNGImage() :
	onloadCallback(getGlobalContext()),
	onerrorCallback(getGlobalContext())
{
	src = "invalid";
}

void PNGImage::loadPNG()
{
	std::string fullPath = getFullPathFromRelativePath(src.c_str());
	if (fullPath.empty()) {
		printf("PNG: File not found: %s\n", src.c_str());
		if (onerrorCallback) {
			jsval funcval = OBJECT_TO_JSVAL(onerrorCallback);
			jsval out;
			JSContext* cx = getGlobalContext();
			// create error: object with a single property
			JSObject* err = JS_NewObject(cx, NULL, NULL, NULL);
			JSString* str = JS_NewStringCopyZ(cx, "error");
			jsval strVal = STRING_TO_JSVAL(str);
			JS_SetProperty(cx, err, "type", &strVal);
			jsval errVal = OBJECT_TO_JSVAL(err);
			// just execute the callback
			JS_CallFunctionValue(cx, NULL, funcval, 1, &errVal, &out);
		}
	} else {
		unsigned error = lodepng::decode(bytes, width, height, fullPath);
		if (error) {
			printf("PNG: error %u decoding png: %s\n", error, lodepng_error_text(error));
			return;
		}
		if (onloadCallback) {
			jsval funcval = OBJECT_TO_JSVAL(onloadCallback);
			jsval out;
			JS_CallFunctionValue(getGlobalContext(), NULL, funcval, 0, NULL, &out);
		}
	}
}

unsigned char* PNGImage::getBytes()
{
	return &bytes[0];
}

JS_BINDED_CLASS_GLUE_IMPL(PNGImage)

JS_BINDED_CONSTRUCTOR_IMPL(PNGImage)
{
	PNGImage* image = new PNGImage();
	jsval out;
	JS_WRAP_OBJECT_IN_VAL(PNGImage, image, out);
	JS_SET_RVAL(cx, vp, out);
	return JS_TRUE;
}

JS_BINDED_PROP_GET_IMPL(PNGImage, width)
{
	vp.set(INT_TO_JSVAL(width));
	return JS_TRUE;
}

JS_BINDED_PROP_GET_IMPL(PNGImage, height)
{
	vp.set(INT_TO_JSVAL(height));
	return JS_TRUE;
}

JS_BINDED_PROP_GET_IMPL(PNGImage, src)
{
	JSString* str = JS_NewStringCopyZ(cx, src.c_str());
	vp.set(STRING_TO_JSVAL(str));
	return JS_TRUE;
}

JS_BINDED_PROP_SET_IMPL(PNGImage, src)
{
	JSString* jsstr = JS_ValueToString(cx, vp.get());
	JSStringWrapper wrapper(jsstr);
	// copy the source
	if (strlen(wrapper) > 0) {
		src = (char *)wrapper;
		loadPNG();
	}
	return JS_TRUE;
}

JS_BINDED_PROP_GET_IMPL(PNGImage, onload)
{
	if (onloadCallback) {
		vp.set(OBJECT_TO_JSVAL(onloadCallback));
	} else {
		vp.set(JSVAL_NULL);
	}
	return JS_TRUE;
}

JS_BINDED_PROP_SET_IMPL(PNGImage, onload)
{
	jsval callback = vp.get();
	if (callback != JSVAL_NULL) {
		onloadCallback = JSVAL_TO_OBJECT(callback);
	}
	return JS_TRUE;
}

JS_BINDED_PROP_GET_IMPL(PNGImage, onerror)
{
	if (onerrorCallback) {
		vp.set(OBJECT_TO_JSVAL(onerrorCallback));
	} else {
		vp.set(JSVAL_NULL);
	}
	return JS_TRUE;
}

JS_BINDED_PROP_SET_IMPL(PNGImage, onerror)
{
	jsval callback = vp.get();
	if (callback != JSVAL_NULL && callback.isObject()) {
		onerrorCallback = JSVAL_TO_OBJECT(callback);
	}
	return JS_TRUE;
}

void PNGImage::_js_register(JSContext* cx, JSObject* global)
{
	// create the class
	PNGImage::js_class = {
		"PNGImage", JSCLASS_HAS_PRIVATE,
		JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, basic_object_finalize,
		JSCLASS_NO_OPTIONAL_MEMBERS
	};
	static JSPropertySpec props[] = {
		JS_BINDED_PROP_DEF_GETTER(PNGImage, width),
		JS_BINDED_PROP_DEF_GETTER(PNGImage, height),
		JS_BINDED_PROP_DEF_ACCESSOR(PNGImage, src),
		JS_BINDED_PROP_DEF_ACCESSOR(PNGImage, onload),
		JS_BINDED_PROP_DEF_ACCESSOR(PNGImage, onerror),
		{0, 0, 0, 0, 0}
	};
	static JSFunctionSpec funcs[] = {
		JS_FS_END
	};
	PNGImage::js_parent = NULL;
	PNGImage::js_proto = JS_InitClass(cx, global, NULL, &PNGImage::js_class, PNGImage::_js_constructor, 1, props, funcs, NULL, NULL);
}

#pragma mark - WebGLRenderingContext

JS_BINDED_CLASS_GLUE_IMPL(WebGLRenderingContext);

JS_BINDED_CONSTRUCTOR_IMPL(WebGLRenderingContext)
{
	// this constructor should never be called from js
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, activeTexture)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum texture;
		JS_ValueToECMAUint32(cx, argv[0], &texture);
		glActiveTexture(texture); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, attachShader)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint program;
		GLuint shader;
		JS_GET_UINT_WRAPPED(argv[0], "program", program);
		JS_ValueToECMAUint32(cx, argv[1], &shader);
		glAttachShader(program, shader); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, bindAttribLocation)
{
	if (argc == 3) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint program;
		GLuint index;
		JSString* str = JS_ValueToString(cx, argv[2]);
		JSStringWrapper name(str);
		JS_GET_UINT_WRAPPED(argv[0], "program", program);
		JS_ValueToECMAUint32(cx, argv[1], &index);
		glBindAttribLocation(program, index, name); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, bindBuffer)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum target;
		GLuint buffer;
		JS_ValueToECMAUint32(cx, argv[0], &target);
		JS_GET_UINT_WRAPPED(argv[1], "buffer", buffer);
		glBindBuffer(target, buffer); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, bindFramebuffer)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum target;
		GLuint framebuffer;
		JS_ValueToECMAUint32(cx, argv[0], &target);
		JS_GET_UINT_WRAPPED(argv[1], "buffer", framebuffer);
		glBindFramebuffer(target, framebuffer); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, bindRenderbuffer)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum target;
		GLuint renderbuffer;
		JS_ValueToECMAUint32(cx, argv[0], &target);
		JS_GET_UINT_WRAPPED(argv[1], "buffer", renderbuffer);
		glBindFramebuffer(target, renderbuffer); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, bindTexture)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum target;
		uint32_t texture;
		JS_ValueToECMAUint32(cx, argv[0], &target);
		JS_GET_UINT_WRAPPED(argv[1], "texture", texture);
		glBindTexture(target, texture); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

void WebGLRenderingContext::blendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	glBlendColor(red, green, blue, alpha); CHECK_GL_ERROR();
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, blendEquation)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum mode;
		JS_ValueToECMAUint32(cx, argv[0], &mode);
		glBlendEquation(mode); CHECK_GL_ERROR();
	}
	return JS_TRUE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, blendEquationSeparate)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum modeRGB;
		GLenum modeAlpha;
		JS_ValueToECMAUint32(cx, argv[0], &modeRGB);
		JS_ValueToECMAUint32(cx, argv[1], &modeAlpha);
		glBlendEquationSeparate(modeRGB, modeAlpha); CHECK_GL_ERROR();
	}
	return JS_TRUE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, blendFunc)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum sfactor;
		GLenum dfactor;
		JS_ValueToECMAUint32(cx, argv[0], &sfactor);
		JS_ValueToECMAUint32(cx, argv[1], &dfactor);
		glBlendFunc(sfactor, dfactor); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, blendFuncSeparate)
{
	if (argc == 4) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum srcRGB;
		GLenum dstRGB;
		GLenum srcAlpha;
		GLenum dstAlpha;
		JS_ValueToECMAUint32(cx, argv[0], &srcRGB);
		JS_ValueToECMAUint32(cx, argv[1], &dstRGB);
		JS_ValueToECMAUint32(cx, argv[2], &srcAlpha);
		JS_ValueToECMAUint32(cx, argv[3], &dstAlpha);
		glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, bufferData)
{
	if (argc == 3) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum target;
		JSObject *data;
		GLenum usage;
		JS_ValueToECMAUint32(cx, argv[0], &target);
		data = JSVAL_TO_OBJECT(argv[1]);
		JS_ValueToECMAUint32(cx, argv[2], &usage);
		if (JS_IsArrayBufferViewObject(data, cx)) {
			void* d = JS_GetArrayBufferViewData(data, cx);
			GLsizeiptr size = JS_GetArrayBufferViewByteLength(data, cx);
			glBufferData(target, size, d, usage); CHECK_GL_ERROR();
			return JS_TRUE;
		}
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, bufferSubData)
{
	if (argc == 3) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum target;
		GLintptr offset;
		JSObject *data;
		JS_ValueToECMAUint32(cx, argv[0], &target);
		offset = JSVAL_TO_INT(argv[1]);
		data = JSVAL_TO_OBJECT(argv[1]);
		if (JS_IsArrayBufferViewObject(data, cx)) {
			void* d = JS_GetArrayBufferViewData(data, cx);
			GLsizeiptr size = JS_GetArrayBufferViewByteLength(data, cx);
			glBufferSubData(target, offset, size, d); CHECK_GL_ERROR();
			return JS_TRUE;
		}
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

GLenum WebGLRenderingContext::checkFramebufferStatus(GLenum target)
{
	return glCheckFramebufferStatus(target); CHECK_GL_ERROR();
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, clear)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLbitfield mask;
		JS_ValueToECMAUint32(cx, argv[0], &mask);
		glClear(mask); CHECK_GL_ERROR();
	}
	return JS_TRUE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, clearColor)
{
	if (argc == 4) {
		GLclampf color[4];
		jsval* argv = JS_ARGV(cx, vp);
		for (int i=0; i < 4; i++) {
			double d;
			JS_ValueToNumber(cx, argv[i], &d);
			color[i] = d;
		}
		glClearColor(color[0], color[1], color[2], color[3]); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	return JS_TRUE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, clearDepth)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		double depth;
		JS_ValueToNumber(cx, argv[0], &depth);
		glClearDepthf(depth); CHECK_GL_ERROR();
	}
	return JS_TRUE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, clearStencil)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint s = JSVAL_TO_INT(argv[0]);
		glClearStencil(s); CHECK_GL_ERROR();
	}
	return JS_TRUE;
}

void WebGLRenderingContext::colorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	glColorMask(red, green, blue, alpha); CHECK_GL_ERROR();
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, compileShader)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint shader;
		JS_ValueToECMAUint32(cx, argv[0], &shader);
		glCompileShader(shader); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, createBuffer)
{
	if (argc == 0) {
		GLuint buffer;
		glGenBuffers(1, &buffer); CHECK_GL_ERROR();
		jsval out;
		JS_CREATE_UINT_WRAPPED(out, "buffer", buffer);
		JS_SET_RVAL(cx, vp, out);
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, createFramebuffer)
{
	if (argc == 0) {
		GLuint framebuffer;
		glGenFramebuffers(1, &framebuffer); CHECK_GL_ERROR();
		jsval out;
		JS_CREATE_UINT_WRAPPED(out, "buffer", framebuffer);
		JS_SET_RVAL(cx, vp, out);
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, createProgram)
{
	if (argc == 0) {
		GLuint program = glCreateProgram(); CHECK_GL_ERROR();
		jsval out;
		JS_CREATE_UINT_WRAPPED(out, "program", program);
		JS_SET_RVAL(cx, vp, out);
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, createRenderbuffer)
{
	if (argc == 0) {
		GLuint renderbuffer;
		glGenRenderbuffers(1, &renderbuffer); CHECK_GL_ERROR();
		jsval out;
		JS_CREATE_UINT_WRAPPED(out, "buffer", renderbuffer);
		JS_SET_RVAL(cx, vp, out);
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, createShader)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum type;
		JS_ValueToECMAUint32(cx, argv[0], &type);
		GLuint shader = glCreateShader(type); CHECK_GL_ERROR();
		jsval out = UINT_TO_JSVAL(shader);
		JS_SET_RVAL(cx, vp, out);
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, createTexture)
{
	if (argc == 0) {
		GLuint texture;
		glGenTextures(1, &texture); CHECK_GL_ERROR();
		jsval out;
		JS_CREATE_UINT_WRAPPED(out, "texture", texture);
		JS_SET_RVAL(cx, vp, out);
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, cullFace)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum mode;
		JS_ValueToECMAUint32(cx, argv[0], &mode);
		glCullFace(mode); CHECK_GL_ERROR();
	}
	return JS_TRUE;
}

void WebGLRenderingContext::deleteBuffer(WebGLBuffer buffer)
{
	glDeleteBuffers(1, &buffer); CHECK_GL_ERROR();
}

void WebGLRenderingContext::deleteFramebuffer(WebGLFramebuffer framebuffer)
{
	glDeleteFramebuffers(1, &framebuffer); CHECK_GL_ERROR();
}

void WebGLRenderingContext::deleteProgram(WebGLProgram program)
{
	glDeleteProgram(program); CHECK_GL_ERROR();
}

void WebGLRenderingContext::deleteRenderbuffer(WebGLRenderbuffer renderbuffer)
{
	glDeleteRenderbuffers(1, &renderbuffer); CHECK_GL_ERROR();
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, deleteShader)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint shader;
		JS_ValueToECMAUint32(cx, argv[0], &shader);
		glDeleteShader(shader); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	return JS_FALSE;
}

void WebGLRenderingContext::deleteTexture(WebGLTexture texture)
{
	glDeleteTextures(1, &texture); CHECK_GL_ERROR();
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, depthFunc)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum func;
		JS_ValueToECMAUint32(cx, argv[0], &func);
		glDepthFunc(func); CHECK_GL_ERROR();
	}
	return JS_TRUE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, depthMask)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		JSBool flag;
		JS_ValueToBoolean(cx, argv[0], &flag);
		glDepthMask(flag); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	return JS_FALSE;
}

void WebGLRenderingContext::depthRange(GLclampf zNear, GLclampf zFar)
{
	glDepthRangef(zNear, zFar); CHECK_GL_ERROR();
}

void WebGLRenderingContext::detachShader(WebGLProgram program, WebGLShader shader)
{
	glDetachShader(program, shader); CHECK_GL_ERROR();
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, disable)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum cap;
		JS_ValueToECMAUint32(cx, argv[0], &cap);
		glDisable(cap); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, disableVertexAttribArray)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint index;
		JS_ValueToECMAUint32(cx, argv[0], &index);
		glDisableVertexAttribArray(index); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid native call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, drawArrays)
{
	if (argc == 3) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum mode;
		GLint first;
		GLsizei count;
		JS_ValueToECMAUint32(cx, argv[0], &mode);
		first = JSVAL_TO_INT(argv[1]);
		count = JSVAL_TO_INT(argv[2]);
		glDrawArrays(mode, first, count); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, drawElements)
{
	if (argc == 4) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum mode;
		GLsizei count;
		GLenum type;
		GLintptr offset;
		JS_ValueToECMAUint32(cx, argv[0], &mode);
		count = JSVAL_TO_INT(argv[1]);
		JS_ValueToECMAUint32(cx, argv[2], &type);
		offset = JSVAL_TO_INT(argv[3]);
		glDrawElements(mode, count, type, ((char *)NULL + offset));
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, enable)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum cap;
		JS_ValueToECMAUint32(cx, argv[0], &cap);
		glEnable(cap); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, enableVertexAttribArray)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint index;
		JS_ValueToECMAUint32(cx, argv[0], &index);
		glEnableVertexAttribArray(index); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

void WebGLRenderingContext::finish()
{
	glFinish(); CHECK_GL_ERROR();
}

void WebGLRenderingContext::flush()
{
	glFlush(); CHECK_GL_ERROR();
}

void WebGLRenderingContext::framebufferRenderbuffer(GLenum target, GLenum attachment,
							 GLenum renderbuffertarget,
							 WebGLRenderbuffer renderbuffer)
{
	glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer); CHECK_GL_ERROR();
}

void WebGLRenderingContext::framebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
						  WebGLTexture texture, GLint level)
{
	glFramebufferTexture2D(target, attachment, textarget, texture, level); CHECK_GL_ERROR();
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, frontFace)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum mode;
		JS_ValueToECMAUint32(cx, argv[0], &mode);
		glFrontFace(mode); CHECK_GL_ERROR();
	}
	return JS_TRUE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, generateMipmap)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum target;
		if (argv[0].isNumber()) {
			JS_ValueToECMAUint32(cx, argv[0], &target);
			glGenerateMipmap(target); CHECK_GL_ERROR();
			return JS_TRUE;
		}
	}
	JS_ReportError(cx, "invalid call to generateMipmap");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, getAttribLocation)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint program;
		JS_GET_UINT_WRAPPED(argv[0], "program", program);
		JSString* jsname = JS_ValueToString(cx, argv[1]);
		JSStringWrapper name(jsname);
		int location = glGetAttribLocation(program, name); CHECK_GL_ERROR();
		JS_SET_RVAL(cx, vp, INT_TO_JSVAL(location));
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, getParameter)
{
	// TODO: FIX FOR THE REST OF THE NON-INTEGER PARAMS
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum pname;
		GLint ret;
		JS_ValueToECMAUint32(cx, argv[0], &pname);
		glGetIntegerv(pname, &ret); CHECK_GL_ERROR();
		JS_SET_RVAL(cx, vp, INT_TO_JSVAL(ret));
	}
	return JS_TRUE;
}

GLint WebGLRenderingContext::getBufferParameter(GLenum target, GLenum pname)
{
	GLint res;
	glGetBufferParameteriv(target, pname, &res); CHECK_GL_ERROR();
	return res;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, getError)
{
	GLenum error = glGetError();
	jsval out = UINT_TO_JSVAL(error);
	JS_SET_RVAL(cx, vp, out);
	return JS_TRUE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, getExtension)
{
	jsval out = BOOLEAN_TO_JSVAL(JS_FALSE);
	JS_SET_RVAL(cx, vp, out);
	return JS_TRUE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, getShaderInfoLog)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint program;
		JS_GET_UINT_WRAPPED(argv[0], "program", program);
		GLint len, outLen;
		glGetShaderiv(program, GL_INFO_LOG_LENGTH, &len); CHECK_GL_ERROR();
		char buff[len+1];
		glGetShaderInfoLog(program, len, &outLen, buff); CHECK_GL_ERROR();
		if (outLen > 0) {
			JSString* str = JS_NewStringCopyN(cx, buff, outLen);
			JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(str));
		}
	} else {
		JS_SET_RVAL(cx, vp, JSVAL_NULL);
	}
	return JS_TRUE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, getUniformLocation)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint program;
		JS_GET_UINT_WRAPPED(argv[0], "program", program);
		JSString* str = JS_ValueToString(cx, argv[1]);
		JSStringWrapper name(str);
		int location = glGetUniformLocation(program, name); CHECK_GL_ERROR();
		JS_SET_RVAL(cx, vp, INT_TO_JSVAL(location));
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

GLint WebGLRenderingContext::getVertexAttrib(GLuint index, GLenum pname)
{
	return 0;
}

GLsizeiptr WebGLRenderingContext::getVertexAttribOffset(GLuint index, GLenum pname)
{
	return 0;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, lineWidth)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		double width;
		JS_ValueToNumber(cx, argv[0], &width);
		glLineWidth(width); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, linkProgram)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint program;
		JS_GET_UINT_WRAPPED(argv[0], "program", program);
		glLinkProgram(program); CHECK_GL_ERROR();
		GLint res;
		glGetProgramiv(program, GL_LINK_STATUS, &res);
		if (res != GL_TRUE) {
			char buf[512];
			GLsizei length;
			glGetProgramInfoLog(program, 512, &length, buf);
			buf[length] = '\0';
			fprintf(stderr, "OpenGL - %s", buf);
		}
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, pixelStorei)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum pname;
		int32_t param;
		JS_ValueToECMAUint32(cx, argv[0], &pname);
		JS_ValueToInt32(cx, argv[1], &param);
		switch (pname) {
		case GL_UNPACK_FLIP_Y_WEBGL:
		case GL_UNPACK_PREMULTIPLY_ALPHA_WEBGL:
		case GL_UNPACK_COLORSPACE_CONVERSION_WEBGL:
			fprintf(stderr, "WARNING: flag 0x%04X not available (pixelStorei)\n", pname);
			break;
				
		default:
			glPixelStorei(pname, param); CHECK_GL_ERROR();
			break;
		}
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

void WebGLRenderingContext::polygonOffset(GLfloat factor, GLfloat units)
{
	glPolygonOffset(factor, units); CHECK_GL_ERROR();
}

void WebGLRenderingContext::readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
				GLenum format, GLenum type, JSObject* pixels)
{
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, shaderSource)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint shader;
		JS_ValueToECMAUint32(cx, argv[0], &shader);
		JSString* jsstr = JS_ValueToString(cx, argv[1]);
		JSStringWrapper wrapper(jsstr);
		const char* source = wrapper;
		GLint length = strlen(source);
		glShaderSource(shader, 1, &source, &length); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, getShaderParameter)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint shader;
		GLenum pname;
		JS_ValueToECMAUint32(cx, argv[0], &shader);
		JS_ValueToECMAUint32(cx, argv[1], &pname);
		jsval out = JSVAL_NULL;
		switch (pname) {
			case GL_SHADER_TYPE:
				GLint shaderType;
				glGetShaderiv(shader, GL_SHADER_TYPE, &shaderType); CHECK_GL_ERROR();
				out = INT_TO_JSVAL(shaderType);
				break;
			case GL_COMPILE_STATUS:
				GLint status;
				glGetShaderiv(shader, GL_COMPILE_STATUS, &status); CHECK_GL_ERROR();
				out = BOOLEAN_TO_JSVAL(status == GL_TRUE);
				break;
			default:
				break;
		}
		JS_SET_RVAL(cx, vp, out);
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, getProgramParameter)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint program;
		GLenum pname;
		JS_GET_UINT_WRAPPED(argv[0], "program", program);
		JS_ValueToECMAUint32(cx, argv[1], &pname);
		jsval out = JSVAL_NULL;
		GLint params;
		switch (pname) {
			case GL_DELETE_STATUS:
				glGetProgramiv(program, GL_DELETE_STATUS, &params); CHECK_GL_ERROR();
				out = BOOLEAN_TO_JSVAL(params == GL_TRUE);
				break;
			case GL_LINK_STATUS:
				glGetProgramiv(program, GL_LINK_STATUS, &params); CHECK_GL_ERROR();
				out = BOOLEAN_TO_JSVAL(params == GL_TRUE);
				break;
			case GL_VALIDATE_STATUS:
				glGetProgramiv(program, GL_VALIDATE_STATUS, &params); CHECK_GL_ERROR();
				out = BOOLEAN_TO_JSVAL(params == GL_TRUE);
				break;
			case GL_ATTACHED_SHADERS:
				glGetProgramiv(program, GL_ATTACHED_SHADERS, &params); CHECK_GL_ERROR();
				out = INT_TO_JSVAL(params);
				break;
			case GL_ACTIVE_ATTRIBUTES:
				glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &params); CHECK_GL_ERROR();
				out = INT_TO_JSVAL(params);
				break;
			case GL_ACTIVE_UNIFORMS:
				glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &params); CHECK_GL_ERROR();
				out = INT_TO_JSVAL(params);
				break;
			default:
				break;
		}
		JS_SET_RVAL(cx, vp, out);
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, texImage2D)
{
	if (argc >= 6) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum target;
		GLint level;
		GLenum internalformat;
		GLenum format;
		GLenum type;
		JSObject* jsimage;
		JS_ValueToECMAUint32(cx, argv[0], &target);
		level = JSVAL_TO_INT(argv[1]);
		JS_ValueToECMAUint32(cx, argv[2], &internalformat);
		if (argc == 6) {
			JS_ValueToECMAUint32(cx, argv[3], &format);
			JS_ValueToECMAUint32(cx, argv[4], &type);
			jsimage = JSVAL_TO_OBJECT(argv[5]);
			if (JS_InstanceOf(cx, jsimage, &PNGImage::js_class, NULL)) {
				PNGImage* image = (PNGImage*)JS_GetPrivate(jsimage);
				// NOTE: this is ignoring the format that getBytes() is returning...
				// we should convert the image to the proper `type` before sending it
				// to glTexImage2D
				glTexImage2D(target, level, internalformat, image->width, image->height, 0, format, type, image->getBytes());
				return JS_TRUE;
			}
		} else if (argc == 9) {
			// do the other call
			GLsizei width = JSVAL_TO_INT(argv[3]);
			GLsizei height = JSVAL_TO_INT(argv[4]);
			GLint border = JSVAL_TO_INT(argv[5]);
			JS_ValueToECMAUint32(cx, argv[6], &format);
			JS_ValueToECMAUint32(cx, argv[7], &type);
			jsimage = (argv[8].isNullOrUndefined() ? NULL : JSVAL_TO_OBJECT(argv[8]));
			const GLvoid* data = NULL;
			if (jsimage && JS_InstanceOf(cx, jsimage, &PNGImage::js_class, NULL)) {
				PNGImage* image = (PNGImage*)JS_GetPrivate(jsimage);
				data = image->getBytes();
			}
			glTexImage2D(target, level, internalformat, width, height, border, format, type, data); CHECK_GL_ERROR();
			return JS_TRUE;
		}
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, texParameterf)
{
	if (argc == 3) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum target;
		GLenum pname;
		GLfloat param;
		JS_ValueToECMAUint32(cx, argv[0], &target);
		JS_ValueToECMAUint32(cx, argv[1], &pname);
		param = JSVAL_TO_DOUBLE(argv[2]);
		glTexParameterf(target, pname, param); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, texParameteri)
{
	if (argc == 3) {
		jsval* argv = JS_ARGV(cx, vp);
		GLenum target;
		GLenum pname;
		GLint param;
		JS_ValueToECMAUint32(cx, argv[0], &target);
		JS_ValueToECMAUint32(cx, argv[1], &pname);
		param = JSVAL_TO_INT(argv[2]);
		glTexParameteri(target, pname, param); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform1f)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		if (location >= 0) {
			double x;
			JS_ValueToNumber(cx, argv[1], &x);
			glUniform1f(location, x); CHECK_GL_ERROR();
		}
		return JS_TRUE;
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform1fv)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		if (location < 0) {
			return JS_TRUE;
		}
		if (argv[1].isObject()) {
			JSObject* arr = JSVAL_TO_OBJECT(argv[1]);
			if (JS_IsFloat32Array(arr, cx)) {
				GLfloat* data = JS_GetFloat32ArrayData(arr, cx);
				GLsizei count = JS_GetTypedArrayLength(arr, cx);
				glUniform1fv(location, count, data); CHECK_GL_ERROR();
				return JS_TRUE;
			} else if (JS_IsArrayObject(cx, arr)) {
				// implement thing here getting the elements from the array
				unsigned length;
				JS_GetArrayLength(cx, arr, &length);
				GLfloat data[length];
				for (int i=0; i < length; i++) {
					jsval elt;
					double tmp;
					JS_GetElement(cx, arr, i, &elt);
					JS_ValueToNumber(cx, elt, &tmp);
					data[i] = tmp;
				}
				glUniform1fv(location, length, data); CHECK_GL_ERROR();
				return JS_TRUE;
			}
		}
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform1i)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		GLint x;
		if (location >= 0) {
			JS_ValueToInt32(cx, argv[1], &x);
			glUniform1i(location, x); CHECK_GL_ERROR();
		}
		return JS_TRUE;
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform1iv)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		if (location < 0) {
			return JS_TRUE;
		}
		if (argv[1].isObject()) {
			JSObject* arr = JSVAL_TO_OBJECT(argv[1]);
			if (JS_IsInt32Array(arr, cx)) {
				GLint* data = JS_GetInt32ArrayData(arr, cx);
				GLsizei count = JS_GetTypedArrayLength(arr, cx);
				glUniform1iv(location, count, data); CHECK_GL_ERROR();
				return JS_TRUE;
			} else if (JS_IsArrayObject(cx, arr)) {
				// implement thing here getting the elements from the array
				unsigned length;
				JS_GetArrayLength(cx, arr, &length);
				GLint data[length];
				for (int i=0; i < length; i++) {
					jsval elt;
					JS_GetElement(cx, arr, i, &elt);
					data[i] = JSVAL_TO_INT(elt);
				}
				glUniform1iv(location, length, data); CHECK_GL_ERROR();
				return JS_TRUE;
			}
		}
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform2f)
{
	if (argc == 3) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		if (location >= 0) {
			double x;
			double y;
			JS_ValueToNumber(cx, argv[1], &x);
			JS_ValueToNumber(cx, argv[2], &y);
			glUniform2f(location, x, y); CHECK_GL_ERROR();
		}
		return JS_TRUE;
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform2fv)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		if (location < 0) {
			return JS_TRUE;
		}
		if (argv[1].isObject()) {
			JSObject* arr = JSVAL_TO_OBJECT(argv[1]);
			if (JS_IsFloat32Array(arr, cx)) {
				GLfloat* data = JS_GetFloat32ArrayData(arr, cx);
				GLsizei count = JS_GetTypedArrayLength(arr, cx);
				glUniform2fv(location, count/2, data); CHECK_GL_ERROR();
				return JS_TRUE;
			} else if (JS_IsArrayObject(cx, arr)) {
				// implement thing here getting the elements from the array
				unsigned length;
				JS_GetArrayLength(cx, arr, &length);
				GLfloat data[length];
				for (int i=0; i < length; i++) {
					jsval elt;
					double tmp;
					JS_GetElement(cx, arr, i, &elt);
					JS_ValueToNumber(cx, elt, &tmp);
					data[i] = tmp;
				}
				glUniform2fv(location, length/2, data); CHECK_GL_ERROR();
				return JS_TRUE;
			}
		}
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform2i)
{
	if (argc == 3) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		if (location >= 0) {
			GLint x;
			GLint y;
			JS_ValueToInt32(cx, argv[1], &x);
			JS_ValueToInt32(cx, argv[2], &y);
			glUniform2i(location, x, y); CHECK_GL_ERROR();
		}
		return JS_TRUE;
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform2iv)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		if (location < 0) {
			return JS_TRUE;
		}
		if (argv[1].isObject()) {
			JSObject* arr = JSVAL_TO_OBJECT(argv[1]);
			if (JS_IsInt32Array(arr, cx)) {
				GLint* data = JS_GetInt32ArrayData(arr, cx);
				GLsizei count = JS_GetTypedArrayLength(arr, cx);
				glUniform2iv(location, count/2, data); CHECK_GL_ERROR();
				return JS_TRUE;
			} else if (JS_IsArrayObject(cx, arr)) {
				// implement thing here getting the elements from the array
				unsigned length;
				JS_GetArrayLength(cx, arr, &length);
				GLint data[length];
				for (int i=0; i < length; i++) {
					jsval elt;
					JS_GetElement(cx, arr, i, &elt);
					data[i] = JSVAL_TO_INT(elt);
				}
				glUniform2iv(location, length/2, data); CHECK_GL_ERROR();
				return JS_TRUE;
			}
		}
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform3f)
{
	if (argc == 4) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		if (location >= 0) {
			double x;
			double y;
			double z;
			JS_ValueToNumber(cx, argv[1], &x);
			JS_ValueToNumber(cx, argv[2], &y);
			JS_ValueToNumber(cx, argv[3], &z);
			glUniform3f(location, x, y, z); CHECK_GL_ERROR();
		}
		return JS_TRUE;
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform3fv)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		if (location < 0) {
			return JS_TRUE;
		}
		if (argv[1].isObject()) {
			JSObject* arr = JSVAL_TO_OBJECT(argv[1]);
			if (JS_IsFloat32Array(arr, cx)) {
				GLfloat* data = JS_GetFloat32ArrayData(arr, cx);
				GLsizei count = JS_GetTypedArrayLength(arr, cx);
				glUniform3fv(location, count/3, data); CHECK_GL_ERROR();
				return JS_TRUE;
			} else if (JS_IsArrayObject(cx, arr)) {
				unsigned length;
				JS_GetArrayLength(cx, arr, &length);
				GLfloat data[length];
				for (int i=0; i < length; i++) {
					jsval elt;
					JS_GetElement(cx, arr, i, &elt);
					data[i] = (GLfloat)elt.toNumber();
				}
				glUniform3fv(location, length/3, data); CHECK_GL_ERROR();
				return JS_TRUE;
			}
		}
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform3i)
{
	if (argc == 4) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		if (location >= 0) {
			GLint x;
			GLint y;
			GLint z;
			JS_ValueToInt32(cx, argv[1], &x);
			JS_ValueToInt32(cx, argv[2], &y);
			JS_ValueToInt32(cx, argv[3], &z);
			glUniform3i(location, x, y, z); CHECK_GL_ERROR();
		}
		return JS_TRUE;
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform3iv)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		if (location < 0) {
			return JS_TRUE;
		}
		if (argv[1].isObject()) {
			JSObject* arr = JSVAL_TO_OBJECT(argv[1]);
			if (JS_IsInt32Array(arr, cx)) {
				GLint* data = JS_GetInt32ArrayData(arr, cx);
				GLsizei count = JS_GetTypedArrayLength(arr, cx);
				glUniform3iv(location, count/3, data); CHECK_GL_ERROR();
				return JS_TRUE;
			} else if (JS_IsArrayObject(cx, arr)) {
				// implement thing here getting the elements from the array
				unsigned length;
				JS_GetArrayLength(cx, arr, &length);
				GLint data[length];
				for (int i=0; i < length; i++) {
					jsval elt;
					JS_GetElement(cx, arr, i, &elt);
					data[i] = JSVAL_TO_INT(elt);
				}
				glUniform3iv(location, length/3, data); CHECK_GL_ERROR();
				return JS_TRUE;
			}
		}
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform4f)
{
	if (argc == 5) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		if (location >= 0) {
			double x;
			double y;
			double z;
			double w;
			JS_ValueToNumber(cx, argv[1], &x);
			JS_ValueToNumber(cx, argv[2], &y);
			JS_ValueToNumber(cx, argv[3], &z);
			JS_ValueToNumber(cx, argv[4], &w);
			glUniform4f(location, x, y, z, w); CHECK_GL_ERROR();
		}
		return JS_TRUE;
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform4fv)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		if (location < 0) {
			return JS_TRUE;
		}
		if (argv[1].isObject()) {
			JSObject* arr = JSVAL_TO_OBJECT(argv[1]);
			if (JS_IsFloat32Array(arr, cx)) {
				GLfloat* data = JS_GetFloat32ArrayData(arr, cx);
				GLsizei count = JS_GetTypedArrayLength(arr, cx);
				glUniform4fv(location, count/4, data); CHECK_GL_ERROR();
				return JS_TRUE;
			} else if (JS_IsArrayObject(cx, arr)) {
				unsigned length;
				JS_GetArrayLength(cx, arr, &length);
				GLfloat data[length];
				for (int i=0; i < length; i++) {
					jsval elt;
					double tmp;
					JS_GetElement(cx, arr, i, &elt);
					JS_ValueToNumber(cx, elt, &tmp);
					data[i] = tmp;
				}
				glUniform4fv(location, length/4, data); CHECK_GL_ERROR();
				return JS_TRUE;
			}
		}
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform4i)
{
	if (argc == 5) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		if (location >= 0) {
			GLint x;
			GLint y;
			GLint z;
			GLint w;
			JS_ValueToInt32(cx, argv[1], &x);
			JS_ValueToInt32(cx, argv[2], &y);
			JS_ValueToInt32(cx, argv[3], &z);
			JS_ValueToInt32(cx, argv[4], &w);
			glUniform4i(location, x, y, z, w); CHECK_GL_ERROR();
		}
		return JS_TRUE;
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniform4iv)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint location = JSVAL_TO_INT(argv[0]);
		if (location < 0) {
			return JS_TRUE;
		}
		if (argv[1].isObject()) {
			JSObject* arr = JSVAL_TO_OBJECT(argv[1]);
			if (JS_IsInt32Array(arr, cx)) {
				GLint* data = JS_GetInt32ArrayData(arr, cx);
				GLsizei count = JS_GetTypedArrayLength(arr, cx);
				glUniform4iv(location, count/4, data); CHECK_GL_ERROR();
				return JS_TRUE;
			} else if (JS_IsArrayObject(cx, arr)) {
				// implement thing here getting the elements from the array
				unsigned length;
				JS_GetArrayLength(cx, arr, &length);
				GLint data[length];
				for (int i=0; i < length; i++) {
					jsval elt;
					JS_GetElement(cx, arr, i, &elt);
					data[i] = JSVAL_TO_INT(elt);
				}
				glUniform4iv(location, length/4, data); CHECK_GL_ERROR();
				return JS_TRUE;
			}
		}
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniformMatrix2fv)
{
	if (argc == 3) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint location;
		GLboolean transpose;
		JSObject* arr;
		JS_ValueToECMAUint32(cx, argv[0], &location);
		transpose = JSVAL_TO_BOOLEAN(argv[1]);
		arr = JSVAL_TO_OBJECT(argv[2]);
		if (JS_IsFloat32Array(arr, cx)) {
			GLfloat* data = JS_GetFloat32ArrayData(arr, cx);
			GLsizei count = JS_GetTypedArrayLength(arr, cx);
			glUniformMatrix2fv(location, count/4, transpose, data); CHECK_GL_ERROR();
			return JS_TRUE;
		} else if (JS_IsArrayObject(cx, arr)) {
			unsigned length;
			JS_GetArrayLength(cx, arr, &length);
			GLfloat data[length];
			for (int i=0; i < length; i++) {
				jsval elt;
				double tmp;
				JS_GetElement(cx, arr, i, &elt);
				JS_ValueToNumber(cx, elt, &tmp);
				data[i] = tmp;
			}
			glUniformMatrix2fv(location, length/4, transpose, data); CHECK_GL_ERROR();
			return JS_TRUE;
		}
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniformMatrix3fv)
{
	if (argc == 3) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint location;
		GLboolean transpose;
		JSObject* arr;
		JS_ValueToECMAUint32(cx, argv[0], &location);
		transpose = JSVAL_TO_BOOLEAN(argv[1]);
		arr = JSVAL_TO_OBJECT(argv[2]);
		if (JS_IsFloat32Array(arr, cx)) {
			GLfloat* data = JS_GetFloat32ArrayData(arr, cx);
			GLsizei count = JS_GetTypedArrayLength(arr, cx);
			glUniformMatrix3fv(location, count/9, transpose, data); CHECK_GL_ERROR();
			return JS_TRUE;
		} else if (JS_IsArrayObject(cx, arr)) {
			unsigned length;
			JS_GetArrayLength(cx, arr, &length);
			GLfloat data[length];
			for (int i=0; i < length; i++) {
				jsval elt;
				double tmp;
				JS_GetElement(cx, arr, i, &elt);
				JS_ValueToNumber(cx, elt, &tmp);
				data[i] = tmp;
			}
			glUniformMatrix3fv(location, length/9, transpose, data); CHECK_GL_ERROR();
			return JS_TRUE;
		}
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, uniformMatrix4fv)
{
	if (argc == 3) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint location;
		GLboolean transpose;
		JSObject* arr;
		JS_ValueToECMAUint32(cx, argv[0], &location);
		transpose = JSVAL_TO_BOOLEAN(argv[1]);
		arr = JSVAL_TO_OBJECT(argv[2]);
		if (JS_IsFloat32Array(arr, cx)) {
			GLfloat* data = JS_GetFloat32ArrayData(arr, cx);
			GLsizei count = JS_GetTypedArrayLength(arr, cx);
			glUniformMatrix4fv(location, count/16, transpose, data); CHECK_GL_ERROR();
			return JS_TRUE;
		} else if (JS_IsArrayObject(cx, arr)) {
			unsigned length;
			JS_GetArrayLength(cx, arr, &length);
			GLfloat data[length];
			for (int i=0; i < length; i++) {
				jsval elt;
				double tmp;
				JS_GetElement(cx, arr, i, &elt);
				JS_ValueToNumber(cx, elt, &tmp);
				data[i] = tmp;
			}
			glUniformMatrix4fv(location, length/16, transpose, data); CHECK_GL_ERROR();
			return JS_TRUE;
		}
	}
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, useProgram)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint program;
		JS_GET_UINT_WRAPPED(argv[0], "program", program);
		glUseProgram(program); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, validateProgram)
{
	if (argc == 1) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint program;
		JS_GET_UINT_WRAPPED(argv[0], "program", program);
		glValidateProgram(program); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

void WebGLRenderingContext::vertexAttrib1f(GLuint indx, GLfloat x)
{
	glVertexAttrib1f(indx, x); CHECK_GL_ERROR();
}

void WebGLRenderingContext::vertexAttrib1fv(GLuint indx, JSObject* values)
{
	JSContext* cx = getGlobalContext();
	if (JS_IsFloat32Array(values, cx)) {
		GLfloat* data = JS_GetFloat32ArrayData(values, cx);
		int count = JS_GetTypedArrayLength(values, cx);
		if (count >= 1) {
			glVertexAttrib1fv(indx, data); CHECK_GL_ERROR();
		}
	}
}

void WebGLRenderingContext::vertexAttrib2f(GLuint indx, GLfloat x, GLfloat y)
{
	glVertexAttrib2f(indx, x, y); CHECK_GL_ERROR();
}

void WebGLRenderingContext::vertexAttrib2fv(GLuint indx, JSObject* values)
{
	JSContext* cx = getGlobalContext();
	if (JS_IsFloat32Array(values, cx)) {
		GLfloat* data = JS_GetFloat32ArrayData(values, cx);
		int count = JS_GetTypedArrayLength(values, cx);
		if (count >= 2) {
			glVertexAttrib2fv(indx, data); CHECK_GL_ERROR();
		}
	}
}

void WebGLRenderingContext::vertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
	glVertexAttrib3f(indx, x, y, z); CHECK_GL_ERROR();
}

void WebGLRenderingContext::vertexAttrib3fv(GLuint indx, JSObject* values)
{
	JSContext* cx = getGlobalContext();
	if (JS_IsFloat32Array(values, cx)) {
		GLfloat* data = JS_GetFloat32ArrayData(values, cx);
		int count = JS_GetTypedArrayLength(values, cx);
		if (count >= 3) {
			glVertexAttrib3fv(indx, data); CHECK_GL_ERROR();
		}
	}
}

void WebGLRenderingContext::vertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	glVertexAttrib4f(indx, x, y, z, w); CHECK_GL_ERROR();
}

void WebGLRenderingContext::vertexAttrib4fv(GLuint indx, JSObject* values)
{
	JSContext* cx = getGlobalContext();
	if (JS_IsFloat32Array(values, cx)) {
		GLfloat* data = JS_GetFloat32ArrayData(values, cx);
		int count = JS_GetTypedArrayLength(values, cx);
		if (count >= 4) {
			glVertexAttrib4fv(indx, data); CHECK_GL_ERROR();
		}
	}
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, vertexAttribPointer)
{
	if (argc == 6) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint indx;
		GLint size;
		GLenum type;
		GLboolean normalized;
		GLsizei stride;
		GLintptr offset;
		JS_ValueToECMAUint32(cx, argv[0], &indx);
		size = JSVAL_TO_INT(argv[1]);
		JS_ValueToECMAUint32(cx, argv[2], &type);
		normalized = JSVAL_TO_BOOLEAN(argv[3]);
		stride = JSVAL_TO_INT(argv[4]);
		offset = JSVAL_TO_INT(argv[5]);
		glVertexAttribPointer(indx, size, type, normalized, stride, reinterpret_cast<GLvoid*>(offset));
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, viewport)
{
	if (argc == 4) {
		jsval* argv = JS_ARGV(cx, vp);
		GLint x, y;
		GLsizei width, height;
		x = JSVAL_TO_INT(argv[0]);
		y = JSVAL_TO_INT(argv[1]);
		width = JSVAL_TO_INT(argv[2]);
		height = JSVAL_TO_INT(argv[3]);
		glViewport(x, y, width, height); CHECK_GL_ERROR();
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

JS_BINDED_FUNC_IMPL(WebGLRenderingContext, getShaderPrecisionFormat)
{
	if (argc == 2) {
		jsval* argv = JS_ARGV(cx, vp);
		GLuint shaderType, precisiontype;
		JS_ValueToECMAUint32(cx, argv[0], &shaderType);
		JS_ValueToECMAUint32(cx, argv[1], &precisiontype);

		switch (shaderType) {
		case GL_FRAGMENT_SHADER:
		case GL_VERTEX_SHADER:
			break;
		default:
			JS_ReportError(cx, "invalid shader type: 0x%04X", shaderType);
			return JS_FALSE;
			break;
		}

		JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
		jsval precision = UINT_TO_JSVAL(precisiontype);
		JS_SetProperty(cx, obj, "precision", &precision);
		JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
		return JS_TRUE;
	}
	JS_ReportError(cx, "invalid call");
	return JS_FALSE;
}

void WebGLRenderingContext::_js_register(JSContext* cx, JSObject *global)
{
	// create the class
	WebGLRenderingContext::js_class = {
		"WebGLRenderingContext", JSCLASS_HAS_PRIVATE,
		JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, basic_object_finalize,
		JSCLASS_NO_OPTIONAL_MEMBERS
	};
	static JSPropertySpec props[] = {
		{0, 0, 0, 0, 0}
	};
	static JSFunctionSpec funcs[] = {
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, activeTexture),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, attachShader),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, bindAttribLocation),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, bindBuffer),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, bindFramebuffer),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, bindRenderbuffer),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, bindTexture),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, blendEquation),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, blendEquationSeparate),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, blendFunc),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, blendFuncSeparate),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, bufferData),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, bufferSubData),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, clear),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, clearColor),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, clearDepth),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, clearStencil),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, compileShader),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, createBuffer),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, createFramebuffer),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, createProgram),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, createRenderbuffer),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, createShader),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, createTexture),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, cullFace),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, deleteShader),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, depthFunc),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, depthMask),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, disable),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, disableVertexAttribArray),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, drawArrays),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, drawElements),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, enable),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, enableVertexAttribArray),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, frontFace),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, getError),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, getExtension),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, getShaderInfoLog),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, getUniformLocation),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, generateMipmap),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, getAttribLocation),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, getParameter),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, lineWidth),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, linkProgram),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, pixelStorei),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, shaderSource),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, getShaderParameter),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, getProgramParameter),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, texImage2D),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, texParameterf),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, texParameteri),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform1f),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform1fv),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform1i),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform1iv),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform2f),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform2fv),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform2i),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform2iv),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform3f),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform3fv),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform3i),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform3iv),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform4f),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform4fv),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform4i),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniform4iv),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniformMatrix2fv),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniformMatrix3fv),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, uniformMatrix4fv),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, useProgram),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, validateProgram),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, vertexAttribPointer),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, viewport),
		JS_BINDED_FUNC_FOR_DEF(WebGLRenderingContext, getShaderPrecisionFormat),
		JS_FS_END
	};
	WebGLRenderingContext::js_parent = NULL;
	WebGLRenderingContext::js_proto = JS_InitClass(cx, global, NULL, &WebGLRenderingContext::js_class, WebGLRenderingContext::_js_constructor, 1, props, funcs, NULL, NULL);
}
