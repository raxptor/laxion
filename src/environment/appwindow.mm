#include "appwindow.h"

#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CVDisplayLink.h>

#include <vector>
#include <kosmos/glwrap/gl.h>

struct mouse
{
	int x, y;
};

struct update_info
{
	laxion::appwindow::input_batch input;
	laxion::appwindow::updatefunc f;
	laxion::appwindow::data *d;
};

@interface LaxionView : NSOpenGLView
{
	CVDisplayLinkRef displayLink; //display link for managing rendering thread
	@public update_info uinfo;
	@public int viewWidth;
	@public int viewHeight;
}

-(void)drawRect:(NSRect)rect;
-(CVReturn)getFrameForTime:(const CVTimeStamp*)outputTime;

@end

@implementation LaxionView

- (void)mouseMoved:(NSEvent *)theEvent
{
	NSPoint mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	NSRect r = [self bounds];
	uinfo.input.mouse_pos[0] = mouseLoc.x;
	uinfo.input.mouse_pos[1] = r.size.height - mouseLoc.y - 1;
}

- (void)mouseDragged:(NSEvent *)theEvent
{
	NSPoint mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	NSRect r = [self bounds];
	uinfo.input.mouse_pos[0] = mouseLoc.x;
	uinfo.input.mouse_pos[1] = r.size.height - mouseLoc.y - 1;
}

- (void)mouseDown:(NSEvent *)theEvent
{
	uinfo.input.mouse_wentdown[0]++;
	uinfo.input.mouse_isdown[0] = 1;
	printf("Mouse down!\n");
}

- (void)mouseUp:(NSEvent *)theEvent
{
	uinfo.input.mouse_wentup[0]++;
	uinfo.input.mouse_isdown[0] = 0;
	printf("Mouse up!\n");
}


- (id)initWithFrame:(NSRect)frame {
	self = [super initWithFrame:frame];
	if (self) {
	}
	return self;
}

- (void)resetCursorRects
{
	[super resetCursorRects];
	printf("reset cursor rects\n");
	[self addCursorRect:self.bounds cursor:[NSCursor crosshairCursor]];
}

-(void)drawRect:(NSRect)rect {
	[[NSColor blueColor] set];
	NSRectFill( [self bounds] );
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
	return YES;
}

- (void)prepareOpenGL
{
	// Synchronize buffer swaps with vertical refresh rate
	
	// (i.e. all openGL on this thread calls will go to this context)
	[[self openGLContext] makeCurrentContext];
	
	// Synchronize buffer swaps with vertical refresh rate
	GLint swapInt = 1;
	[[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
	
	// Create a display link capable of being used with all active displays
	CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
	
	// Set the renderer output callback function
	CVDisplayLinkSetOutputCallback(displayLink, &MyDisplayLinkCallback, self);

	NSOpenGLPixelFormatAttribute attrs[] = {
		NSOpenGLPFAAllRenderers,
		NSOpenGLPFADepthSize, 32,
        0
	};

	CGLPixelFormatObj cglPixelFormat  = (CGLPixelFormatObj) [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
	
	CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
	CVDisplayLinkSetOutputCallback(displayLink, MyDisplayLinkCallback, self);
	CGLContextObj cglContext = (CGLContextObj)[[self openGLContext] CGLContextObj];
	CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);
	
	// Activate the display link
	CVDisplayLinkStart(displayLink);

	viewWidth = 100;
	viewHeight = 100;

	printf("Starting with [%s]\n", glGetString(GL_VERSION));
}

static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
	LaxionView *p = (LaxionView*) displayLinkContext;

	NSSize viewBounds = [p bounds].size;
	p->viewWidth = viewBounds.width;
	p->viewHeight = viewBounds.height;
	
	NSOpenGLContext *currentContext = [p openGLContext];
	[currentContext makeCurrentContext];
	
	// must lock GL context because display link is threaded
	CGLLockContext((CGLContextObj)[currentContext CGLContextObj]);
	
	CVReturn result = [(LaxionView*)displayLinkContext getFrameForTime:outputTime];
	
	// draw here
	[currentContext flushBuffer];
	CGLUnlockContext((CGLContextObj)[currentContext CGLContextObj]);
	
	return result;
}

- (CVReturn)getFrameForTime:(const CVTimeStamp*)outputTime
{
	if (uinfo.f)
	{
		uinfo.f(&uinfo.input, 0.005f);
		for (int i=0;i<laxion::appwindow::input_batch::BUTTONS;i++)
		{
			uinfo.input.mouse_wentdown[i] = 0;
			uinfo.input.mouse_wentup[i] = 0;
		}
	}
	return kCVReturnSuccess;
}

- (void)dealloc
{
	// Release the display link
	CVDisplayLinkRelease(displayLink);
	
	[super dealloc];
}
@end




@interface AppDelegate : NSObject
{
@public NSView *view;
@public NSString *windowIcon;
}
@end

@implementation AppDelegate

- (void)applicationWillFinishLaunching:(NSNotification *)notification
{

}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{

	// NSImage *img = [[NSImage alloc] initWithContentsOfFile: windowIcon];
	// [NSApp setApplicationIconImage: img];
	
	id menubar = [[NSMenu new] autorelease];
	id appMenuItem = [[NSMenuItem new] autorelease];
	[menubar addItem:appMenuItem];
	[NSApp setMainMenu:menubar];
	
	NSMenu *appMenu = [[NSMenu new] autorelease];

	id appName = [[NSProcessInfo processInfo] processName];
	id quitTitle = [@"Quit " stringByAppendingString:appName];
	id quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
	
	[appMenu addItem:quitMenuItem];
	[appMenuItem setSubmenu:appMenu];

	[windowIcon release];
	[NSApp activateIgnoringOtherApps:YES];
}

- (void)windowWillClose:(NSNotification *)notification {
	printf("Window closing, terminating app!\n");
	[[NSApplication sharedApplication] terminate: self];
}

@end

namespace laxion
{
	namespace appwindow
	{
		struct data
		{
			NSAutoreleasePool *pool;
			NSApplication *app;
			NSWindow *window;
			LaxionView *view;
			AppDelegate *appdelegate;
		};
		
		data * create(const char *title, int width, int height, const char *iconfile)
		{
			data *d = new data();
			d->pool = [NSAutoreleasePool new];
			d->app = [NSApplication sharedApplication];
			d->appdelegate = [[AppDelegate alloc] autorelease];

//			d->appdelegate->windowIcon = [NSString stringWithCString:iconfile encoding:NSUTF8StringEncoding];

			NSRect frame = NSMakeRect( 100., 100., 100. + (float)width, 100. + (float)height );
			
			d->window = [[NSWindow alloc]
					 initWithContentRect:frame
					 styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask
					 backing:NSBackingStoreBuffered
					 defer:false];

			d->view = [[[LaxionView alloc] initWithFrame:frame] autorelease];
			
			NSOpenGLPixelFormatAttribute attrs[] = {
				NSOpenGLPFAAllRenderers,
				NSOpenGLPFADepthSize, 32,
                NSOpenGLPFAOpenGLProfile,
                NSOpenGLProfileVersion3_2Core,                
				0
			};

			NSOpenGLPixelFormat *fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
			[d->view setPixelFormat: fmt];

			memset(&d->view->uinfo.input, 0x00, sizeof(input_batch));

			[d->window setContentView:d->view];
			[d->window makeKeyAndOrderFront:nil];
			
			[d->app setActivationPolicy:NSApplicationActivationPolicyRegular];
			
			d->appdelegate->view = d->view;
			
			[NSApp setDelegate: (id)d->appdelegate];
			[d->window setDelegate: (id)d->appdelegate];
			
			[d->window setAcceptsMouseMovedEvents: true];
			[d->window makeFirstResponder: (id)d->view];
			
			int opts = (NSTrackingActiveAlways | NSTrackingInVisibleRect | NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved);
			NSTrackingArea *area = [[NSTrackingArea alloc] initWithRect:[d->view bounds]
											    options:opts
												owner:(id) d->view
											   userInfo:nil];
			[d->view addTrackingArea:(id)area];

			set_title(d, title);
			return d;
		}
		
		void destroy(data *d)
		{
			[d->pool release];
		}
		
		void set_title(data *d, const char *title)
		{
			id str = [[NSString alloc] initWithCString:title encoding: NSUTF8StringEncoding];
			[d->window setTitle: str];
			[str release];
		}
		
		void get_client_rect(data *d, int *x0, int *y0, int *x1, int *y1)
		{
			*x0 = 0;
			*y0 = 0;
			*x1 = d->view->viewWidth;
			*y1 = d->view->viewHeight;
			return;
		}
		
		void run_loop(data *d, updatefunc f)
		{
			d->view->uinfo.f = f;
			[d->app run];
		}
		
	}
}

