//
//  FakeAudio.h
//  webglshim
//
//  Created by Rolando Abarca on 11/2/12.
//  Copyright (c) 2012 Rolando Abarca. All rights reserved.
//

#ifndef __webglshim__FakeAudio__
#define __webglshim__FakeAudio__

#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include "WebGLRenderingContext.h"

class OpenALBuffer;

enum AudioCallbackType {
	AudioEnded,
	AudioStopped,
	AudioPaused
};

class OpenALCallback
{
	js::RootedValue jsfunction;
	js::RootedObject jsthis;
public:
	OpenALCallback(JSContext* cx, js::Value func, JSObject* thisObj);
	void call(JSContext* cx);
};

class OpenALBuffer
{
	ALuint bufferId;
	ALenum format;
	ALsizei size;
	ALsizei sampleRate;
	std::string src;
	std::vector<std::shared_ptr<OpenALCallback>> audioEndedCallbacks;
public:
	OpenALBuffer(std::string path);
	~OpenALBuffer();
	ALuint getBufferId();
	// this is platform dependent, for iOS this is defined in FakeAudio.ios.mm
	void registerCallback(AudioCallbackType eventType, std::shared_ptr<OpenALCallback> callback);
	void deregisterCallack(AudioCallbackType eventType, std::shared_ptr<OpenALCallback> callback);
	void notifyEvent(AudioCallbackType eventType);
	void* getData();
};

class FakeAudio : public JSBindedObject
{
	int readyState;
	std::string src;
	bool autoplay;
	bool loop;
	bool played;
	bool ended;
	bool preload;
	bool loaded;
	float volume;
	float duration;

	OpenALBuffer* buffer;
	ALuint sourceId;
public:
	FakeAudio();
	FakeAudio(std::string aPath);
	~FakeAudio();
	
	void loadAudio();
	
	JS_BINDED_CLASS_GLUE(FakeAudio);
	JS_BINDED_CONSTRUCTOR(FakeAudio);
	JS_BINDED_PROP_ACCESSOR(FakeAudio, autoplay);
	JS_BINDED_PROP_ACCESSOR(FakeAudio, loop);
	JS_BINDED_PROP_ACCESSOR(FakeAudio, preload);
	JS_BINDED_PROP_ACCESSOR(FakeAudio, muted);
	JS_BINDED_PROP_ACCESSOR(FakeAudio, src);
	JS_BINDED_PROP_ACCESSOR(FakeAudio, currentTime);
	JS_BINDED_PROP_ACCESSOR(FakeAudio, volume);
	JS_BINDED_PROP_GET(FakeAudio, played);
	JS_BINDED_PROP_GET(FakeAudio, ended);
	JS_BINDED_PROP_GET(FakeAudio, duration);
	JS_BINDED_PROP_GET(FakeAudio, currentSrc);
	JS_BINDED_PROP_GET(FakeAudio, readyState);
	
	JS_BINDED_FUNC(FakeAudio, canPlayType);
	JS_BINDED_FUNC(FakeAudio, load);
	JS_BINDED_FUNC(FakeAudio, play);
	JS_BINDED_FUNC(FakeAudio, pause);
	JS_BINDED_FUNC(FakeAudio, addEventListener);
	JS_BINDED_FUNC(FakeAudio, removeEventListener);
};

#endif /* defined(__webglshim__FakeAudio__) */
