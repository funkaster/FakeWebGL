//
//  ViewController.m
//  webglshim
//
//  Created by Rolando Abarca on 10/5/12.
//  Copyright (c) 2012 Rolando Abarca. All rights reserved.
//

#include <sys/time.h>
#include <vector>
#import "ViewController.h"
#import "glue.h"

@interface ViewController () {
	struct timeval lastInfoDisplay;
}
@property (strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) GLKBaseEffect *effect;

- (void)setupGL;
- (void)tearDownGL;

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
	gettimeofday(&lastInfoDisplay, NULL);
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
	// the default is 30 - for games we want it to be as fast as possible
	self.preferredFramesPerSecond = 60;
    
	createJSEnvironment();
    [self setupGL];	
}

- (void)dealloc
{    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil)) {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context) {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }

    // Dispose of any resources that can be recreated.
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];
	// load js files
	NSString *glMatrixPath = [[NSBundle mainBundle] pathForResource:@"gl-matrix-min.js" ofType:nil inDirectory:@"js"];
	// run all the files
	runScript([glMatrixPath UTF8String]);
	NSString *mainPath = [[NSBundle mainBundle] pathForResource:@"chester-test-1.js" ofType:nil inDirectory:@"js"];
	runScript([mainPath UTF8String]);
	evalString("setupGame()", NULL, "native-setupGL");
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
        
    self.effect = nil;    
}

#pragma mark - Touches

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
	std::vector<webglTouch_t> injectedTouches;
	for (UITouch *t in touches) {
		CGPoint pt = [t locationInView:self.view];
		injectedTouches.push_back({pt.x, pt.y});
	}
	injectTouches(webglTouchesBegan, &injectedTouches[0], touches.count);
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
	std::vector<webglTouch_t> injectedTouches;
	for (UITouch *t in touches) {
		CGPoint pt = [t locationInView:self.view];
		injectedTouches.push_back({pt.x, pt.y});
	}
	injectTouches(webglTouchesMoved, &injectedTouches[0], touches.count);
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
	std::vector<webglTouch_t> injectedTouches;
	for (UITouch *t in touches) {
		CGPoint pt = [t locationInView:self.view];
		injectedTouches.push_back({pt.x, pt.y});
	}
	injectTouches(webglTouchesEnded, &injectedTouches[0], touches.count);
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
	std::vector<webglTouch_t> injectedTouches;
	for (UITouch *t in touches) {
		CGPoint pt = [t locationInView:self.view];
		injectedTouches.push_back({pt.x, pt.y});
	}
	injectTouches(webglTouchesCanceled, &injectedTouches[0], touches.count);
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
	JSObject* obj = getRequestAnimationFrameCallback();
	if (obj) {
		jsval fval = OBJECT_TO_JSVAL(obj);
		jsval outval;
		JS_CallFunctionValue(getGlobalContext(), NULL, fval, 0, NULL, &outval);
	}
//	struct timeval now;
//	gettimeofday(&now, NULL);
//	if (now.tv_sec - lastInfoDisplay.tv_sec >= 1) {
//		lastInfoDisplay.tv_sec = now.tv_sec;
//		NSLog(@"FPS: %d", self.framesPerSecond);
//	}
}

@end
