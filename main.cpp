/*
 * Copyright (c) 2015, Cameron L. Beggs, James P. Beggs
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _GNU_SOURCE
#include <iostream>
#include <string>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2proto.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xutil.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <mqueue.h>
#include <pthread.h>
#define USE_CHOOSE_FBCONFIG
using namespace std;


struct touchdata
{
    int touchid;
    double x;
    double y;
    int finger;
    touchdata (double x, double y)
    {
        this->x = x;
        this->y = y;
    };
    touchdata ()
    {
        touchid = 0;
        x = 0;
        y = 0;
        finger = 0;
    };
    touchdata& operator=(volatile touchdata& rhs)
    {
        this->x = rhs.x;
        this->y = rhs.y;
        this->finger = rhs.finger;
        this->touchid = rhs.touchid;

        return *this;
    };
    volatile touchdata& operator=(const touchdata& rhz) volatile
    {
        this->x = rhz.x;
        this->y = rhz.y;
        this->finger = rhz.finger;
        this->touchid = rhz.touchid;

        return *this;
    };
};

void glDisplay(Display *dpy, int data);
static int isExtensionSupported(const char *extList, const char *extension);
static void fatalError(const char *why);
static Bool waitForMapNotify(Display *d, XEvent *e, char *arg);
static void describe_fbconfig(GLXFBConfig fbconfig);
static void createTheWindow();
static int ctxErrorHandler( Display *dpy, XErrorEvent *ev );
static void createTheRenderContext();
static int updateTheMessageQueue();
static void draw_cube(void);
static void draw_slant(void);
static void draw_rev_slant(void);
static void redrawTheWindow(volatile touchdata & point);
static void select_events(Display *dpy, Window win);
void cookieAndEvents(XEvent ev, int xi_opcode, touchdata & point, vector<touchdata> & touchdatareal, int data);
int touchdatabegin(int touchid, vector<touchdata> & ff, double x, double y);
int touchDataUpdate(int touchid, vector<touchdata> & ff, double x, double y);
void * glDrawingThreid(void *);
int touchDataEnd(int touchid, vector<touchdata> & ff);
/* it getts annoing after a while transfreing so why not make these varables local?*/
int deviceid;
Display *dpy;
//int readpipefd;
//int writepipefd;
volatile bool syncpt = false;
volatile bool moretouches = false;
#ifndef FINGER_COUNT
const int FINGER_COUNT = 20;
#endif
volatile touchdata shareddata[FINGER_COUNT]; //just guessing
//const string msgqueuename("/foo");

int main(int argc, char *argv[])
{
    int devicez;
    int run;
    /* Connect to the X server */
    dpy = XOpenDisplay(NULL);
    /* XInput Extension available? */
    int opcode, event, error;
    if (!XQueryExtension(dpy, "XInputExtension", &opcode, &event, &error))
    {
        cout << "X Input extension not available." << endl;
        return -1;
    }
    else
    {
        cout << "X Input extension available." << endl;
    }
    /* Which version of XI2? We support 2.0 */
    int major = 2, minor = 2;
    /* we acutly support 2.3... */
    XIQueryVersion(dpy, &major, &minor);
    if (major * 1000 + minor < 2002)
    {
        cout << "Server does not support XI 2.2" << endl;
    }
    else
    {
        cout << "Server supports XI 2.2" << endl;
    }
    int ndevices;

    XIDeviceInfo *info = XIQueryDevice(dpy, XIAllDevices, &ndevices);
    string devicename;
    vector<string> listz;
    vector<int> devidz;
    for (int i = 0; i < ndevices; i++)
    {
        XIDeviceInfo *dev = &info[i];
        cout << "Device name, " << dev->name << endl;
        for (int j = 0; j < dev->num_classes; j++)
        {
            XIAnyClassInfo *classs = dev->classes[j];
            XITouchClassInfo *t = (XITouchClassInfo*)classs;

            if (classs->type == XITouchClass)
            {
                listz.push_back(dev->name);
                devidz.push_back(dev->deviceid);
                cout << ((t->mode == XIDirectTouch) ?  "direct" : "dependent") << " touch device, supporting " <<
                		t->num_touches << " touches." << endl;
            }
        }
    }
    cout << "listing useable devices," << endl;
    for (vector<string>::iterator it = listz.begin(); it != listz.end(); ++it)
    {
        cout << *it << endl;
    }
    int data;
    cout << "pls select a device based on number: " << endl;
    cin >> devicez;
    devicename = listz[devicez - 1];
    deviceid = devidz[devicez - 1];
    cout << "you picked: " << devicename << " with id: " << deviceid << endl;
    cout << "enable touchdata?" << endl;
    cout << "use 1 for yes and 0 for no" << endl;
    cin >> data;
    cout << "going to run GL display now try stuff" << endl;
    cout << "use 1 to run and 0 to not" << endl;
    cin >> run;
    if (run == 1)
    {
        glDisplay(dpy, data);
    }
    return 0;
}


static void fatalError(const char *why)
{
	cerr << why;
	exit(0x777);
}

static Atom del_atom;
static Colormap cmap;
static Display *Xdisplay;
static XVisualInfo *visual;
static XRenderPictFormat *pict_format;
static GLXFBConfig *fbconfigs, fbconfig;
static int numfbconfigs;
static GLXContext render_context;
volatile static Window win, window_handle;
static GLXWindow glX_window_handle;
static int width, height;

static int VisData[] =
{
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_DOUBLEBUFFER, True,
    GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE, 8,
    GLX_ALPHA_SIZE, 8,
    GLX_DEPTH_SIZE, 16,
    None
};

void glDisplay(Display *dpy, int data)
{
    touchdata point;
    //int pipefd[2];
    //pipe2(pipefd, O_DIRECT);
    //pipe2(pipefd, O_DIRECT);
    /*int flags = fcntl(pipefd[1], F_GETFL, O_DIRECT);
    fcntl(pipefd[1], F_SETFL, flags | O_DIRECT);
    int pipe_sz = fcntl(pipefd[1], F_SETPIPE_SZ, sizeof(touchdata)*1337*10);
    int flags1 = fcntl(pipefd[0], F_GETFL, O_DIRECT);
    fcntl(pipefd[0], F_SETFL, flags1 | O_DIRECT);
    int pipe_sz1 = fcntl(pipefd[0], F_SETPIPE_SZ, sizeof(touchdata)*1337*10);
    cout << pipe_sz << endl << pipe_sz1 << endl; */
    //readpipefd = pipefd[0];
    //writepipefd = pipefd[1];
    /*mq_attr messattr;
    messattr.mq_msgsize = sizeof(point);
    messattr.mq_flags = O_NONBLOCK;
    messattr.mq_maxmsg = 9;
    messattr.mq_curmsgs = 0;*/
    pthread_t redrawglWindow;
    pthread_create(&redrawglWindow, NULL, glDrawingThreid, NULL);
    while (syncpt == false)
    {
        /* take a cat nap while stuff is being made MEOW :3 */
    }
    /*static mqd_t msgq = mq_open(msgqueuename.c_str(), O_WRONLY);
    cout << msgq << endl;
    if (msgq == (mqd_t) -1)
    {
        perror("d");
    }*/
    //createTheWindow();
	//createTheRenderContext();
    XEvent ev;
    int opcode;
    select_events(dpy, window_handle);
    vector<touchdata> touchdatareal;
	while (true)
	{
        cookieAndEvents(ev, opcode, point, touchdatareal, data);
	}
}

static int isExtensionSupported(const char *extList, const char *extension)
{

  const char *start;
  const char *where, *terminator;

  /* Extension names should not have spaces. */
  where = strchr(extension, ' ');
  if ( where || *extension == '\0' )
    return 0;

  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string. Don't be fooled by sub-strings,
     etc. */
  for ( start = extList; ; ) {
    where = strstr( start, extension );

    if ( !where )
      break;

    terminator = where + strlen( extension );

    if ( where == start || *(where - 1) == ' ' )
      if ( *terminator == ' ' || *terminator == '\0' )
        return 1;

    start = terminator;
  }
  return 0;
}

static Bool waitForMapNotify(Display *d, XEvent *e, char *arg)
{
	return d && e && arg && (e->type == MapNotify) && (e->xmap.window == *(Window*)arg);
}

static void describe_fbconfig(GLXFBConfig fbconfig)
{
	int doublebuffer;
	int red_bits, green_bits, blue_bits, alpha_bits, depth_bits;

	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_DOUBLEBUFFER, &doublebuffer);
	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_RED_SIZE, &red_bits);
	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_GREEN_SIZE, &green_bits);
	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_BLUE_SIZE, &blue_bits);
	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_ALPHA_SIZE, &alpha_bits);
	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_DEPTH_SIZE, &depth_bits);

	cerr << "FBConfig selected:" << endl;
	cerr << "Doublebuffer: " << ((doublebuffer == True) ? "Yes" : "No") << endl;
	cerr <<	"Red Bits: " << red_bits << ", Green Bits: " << green_bits << ", Blue Bits: " <<
			blue_bits << ", Alpha Bits: " << alpha_bits << ", Depth Bits: " << depth_bits << endl;
}

static void createTheWindow()
{
	XEvent event;
	static int Xscreen;
	int x,y, attr_mask;
	XSizeHints hints;
	XWMHints *startup_state;
	XTextProperty textprop;
	XSetWindowAttributes attr = {0,};
	static char *title = "JET FLUE CAN'T MELT TIME AND SPACE";

	Xdisplay = XOpenDisplay(NULL);
	if (!Xdisplay)
	{
		fatalError("Couldn't connect to X server\n");
	}

	Xscreen = DefaultScreen(Xdisplay);

	win = RootWindow(Xdisplay, Xscreen);

	fbconfigs = glXChooseFBConfig(Xdisplay, Xscreen, VisData, &numfbconfigs);
	fbconfig = 0;
	for(int i = 0; i<numfbconfigs; i++) {
		visual = (XVisualInfo*) glXGetVisualFromFBConfig(Xdisplay, fbconfigs[i]);
		if(!visual)
			continue;

		pict_format = XRenderFindVisualFormat(Xdisplay, visual->visual);
		if(!pict_format)
			continue;

		fbconfig = fbconfigs[i];
		if(pict_format->direct.alphaMask > 0) {
			break;
		}
	}

	if(!fbconfig) {
		fatalError("No matching FB config found");
	}

	describe_fbconfig(fbconfig);

	/* Create a colormap - only needed on some X clients, eg. IRIX */
	cmap = XCreateColormap(Xdisplay, win, visual->visual, AllocNone);

	attr.colormap = cmap;
	attr.background_pixmap = None;
	attr.border_pixmap = None;
	attr.border_pixel = 0;
	attr.event_mask =
		StructureNotifyMask |
		EnterWindowMask |
		LeaveWindowMask |
		ExposureMask |
		ButtonPressMask |
		ButtonReleaseMask |
		OwnerGrabButtonMask |
		KeyPressMask |
		KeyReleaseMask;

	attr_mask =
	//	CWBackPixmap|
		CWColormap|
		CWBorderPixel|
		CWEventMask;

	width = DisplayWidth(Xdisplay, DefaultScreen(Xdisplay))/5;
	height = DisplayHeight(Xdisplay, DefaultScreen(Xdisplay))/2;
	x=width/2, y=height/2;

	window_handle = XCreateWindow(	Xdisplay,
					win,
					x, y, width, height,
					0,
					visual->depth,
					InputOutput,
					visual->visual,
					attr_mask, &attr);

	if( !window_handle ) {
		fatalError("Couldn't create the window\n");
	}

#if USE_GLX_CREATE_WINDOW
	int glXattr[] = { None };
	glX_window_handle = glXCreateWindow(Xdisplay, fbconfig, window_handle, glXattr);
	if( !glX_window_handle ) {
		fatalError("Couldn't create the GLX window\n");
	}
#else
	glX_window_handle = window_handle;
#endif

	textprop.value = (unsigned char*)title;
	textprop.encoding = XA_STRING;
	textprop.format = 8;
	textprop.nitems = strlen(title);

	hints.x = x;
	hints.y = y;
	hints.width = width;
	hints.height = height;
	hints.flags = USPosition|USSize;

	startup_state = XAllocWMHints();
	startup_state->initial_state = NormalState;
	startup_state->flags = StateHint;

	XSetWMProperties(Xdisplay, window_handle,&textprop, &textprop, NULL, 0, &hints, startup_state, NULL);

	XFree(startup_state);

	XMapWindow(Xdisplay, window_handle);
	XIfEvent(Xdisplay, &event, waitForMapNotify, (char*)&window_handle);

	if ((del_atom = XInternAtom(Xdisplay, "WM_DELETE_WINDOW", 0)) != None) {
		XSetWMProtocols(Xdisplay, window_handle, &del_atom, 1);
	}
}

static int ctxErrorHandler( Display *dpy, XErrorEvent *ev )
{
    cerr << "Error at context creation";
    return 0;
}

static void createTheRenderContext()
{
	int dummy;
	if (!glXQueryExtension(Xdisplay, &dummy, &dummy)) {
		fatalError("OpenGL not supported by X server\n");
	}

#if USE_GLX_CREATE_CONTEXT_ATTRIB
	#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
	#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
	render_context = NULL;
	if( isExtensionSupported( glXQueryExtensionsString(Xdisplay, DefaultScreen(Xdisplay)), "GLX_ARB_create_context" ) ) {
		typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
		glXCreateContextAttribsARBProc glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );
		if( glXCreateContextAttribsARB ) {
			int context_attribs[] =
			{
				GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
				GLX_CONTEXT_MINOR_VERSION_ARB, 0,
				//GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
				None
			};

			int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);

			render_context = glXCreateContextAttribsARB( Xdisplay, fbconfig, 0, True, context_attribs );

			XSync( Xdisplay, False );
			XSetErrorHandler( oldHandler );

			fputs("glXCreateContextAttribsARB failed", stderr);
		} else {
			fputs("glXCreateContextAttribsARB could not be retrieved", stderr);
		}
	} else {
			fputs("glXCreateContextAttribsARB not supported", stderr);
	}

	if(!render_context)
	{
#else
	{
#endif
		render_context = glXCreateNewContext(Xdisplay, fbconfig, GLX_RGBA_TYPE, 0, True);
		if (!render_context) {
			fatalError("Failed to create a GL context\n");
		}
	}

	if (!glXMakeContextCurrent(Xdisplay, glX_window_handle, glX_window_handle, render_context)) {
		fatalError("glXMakeCurrent failed for window\n");
	}
}

static int updateTheMessageQueue()
{
	XEvent event;
	XConfigureEvent *xc;

	while (XPending(Xdisplay))
	{
		XNextEvent(Xdisplay, &event);
		switch (event.type)
		{
		case ClientMessage:
			if (event.xclient.data.l[0] == del_atom)
			{
				return 0;
			}
		break;

		case ConfigureNotify:
			xc = &(event.xconfigure);
			width = xc->width;
			height = xc->height;
			break;
		}
	}
	return 1;
}

/*  6----7
   /|   /|
  3----2 |
  | 5--|-4
  |/   |/
  0----1
*/

GLfloat cube_vertices[][8] =
{
	/*  X     Y     Z   Nx   Ny   Nz    S    T */
	{-1.0, -1.0,  1.0, 0.0, 0.0, 1.0, 0.0, 0.0}, // 0
	{ 1.0, -1.0,  1.0, 0.0, 0.0, 1.0, 1.0, 0.0}, // 1
	{ 1.0,  1.0,  1.0, 0.0, 0.0, 1.0, 1.0, 1.0}, // 2
	{-1.0,  1.0,  1.0, 0.0, 0.0, 1.0, 0.0, 1.0}, // 3

	{ 1.0, -1.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0}, // 4
	{-1.0, -1.0, -1.0, 0.0, 0.0, -1.0, 1.0, 0.0}, // 5
	{-1.0,  1.0, -1.0, 0.0, 0.0, -1.0, 1.0, 1.0}, // 6
	{ 1.0,  1.0, -1.0, 0.0, 0.0, -1.0, 0.0, 1.0}, // 7

	{-1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 0.0, 0.0}, // 5
	{-1.0, -1.0,  1.0, -1.0, 0.0, 0.0, 1.0, 0.0}, // 0
	{-1.0,  1.0,  1.0, -1.0, 0.0, 0.0, 1.0, 1.0}, // 3
	{-1.0,  1.0, -1.0, -1.0, 0.0, 0.0, 0.0, 1.0}, // 6

	{ 1.0, -1.0,  1.0,  1.0, 0.0, 0.0, 0.0, 0.0}, // 1
	{ 1.0, -1.0, -1.0,  1.0, 0.0, 0.0, 1.0, 0.0}, // 4
	{ 1.0,  1.0, -1.0,  1.0, 0.0, 0.0, 1.0, 1.0}, // 7
	{ 1.0,  1.0,  1.0,  1.0, 0.0, 0.0, 0.0, 1.0}, // 2

	{-1.0, -1.0, -1.0,  0.0, -1.0, 0.0, 0.0, 0.0}, // 5
	{ 1.0, -1.0, -1.0,  0.0, -1.0, 0.0, 1.0, 0.0}, // 4
	{ 1.0, -1.0,  1.0,  0.0, -1.0, 0.0, 1.0, 1.0}, // 1
	{-1.0, -1.0,  1.0,  0.0, -1.0, 0.0, 0.0, 1.0}, // 0

	{-1.0, 1.0,  1.0,  0.0,  1.0, 0.0, 0.0, 0.0}, // 3
	{ 1.0, 1.0,  1.0,  0.0,  1.0, 0.0, 1.0, 0.0}, // 2
	{ 1.0, 1.0, -1.0,  0.0,  1.0, 0.0, 1.0, 1.0}, // 7
	{-1.0, 1.0, -1.0,  0.0,  1.0, 0.0, 0.0, 1.0}, // 6
};

/*
 *   3\
 *  /| \
 * 2\|  \
 * | \   \
 * | 5\---4
 * |/  \ /
 * 0----1
 */
/* useing glqads for this and points points to the same area works */
GLfloat slant_vertices[][8] =
{
    /*  X     Y     Z   Nx   Ny   Nz    S    T */
    {-1.0, -1.0,  1.0, 0.0, 0.0, 1.0, 0.0, 0.0}, // 0
	{ 1.0, -1.0,  1.0, 0.0, 0.0, 1.0, 1.0, 0.0}, // 1
	{-1.0,  1.0,  1.0, 0.0, 0.0, 1.0, 0.0, 1.0}, // 2
	{-1.0,  1.0,  1.0, 0.0, 0.0, 1.0, 0.0, 1.0}, // 2

	{ 1.0, -1.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0}, // 4
	{-1.0, -1.0, -1.0, 0.0, 0.0, -1.0, 1.0, 0.0}, // 5
	{-1.0,  1.0, -1.0, 0.0, 0.0, -1.0, 1.0, 1.0}, // 3
	{-1.0,  1.0, -1.0, 0.0, 0.0, -1.0, 1.0, 1.0}, // 3

	{-1.0, -1.0, -1.0, -1.0, 0.0, 0.0, 0.0, 0.0}, // 5
	{-1.0, -1.0,  1.0, -1.0, 0.0, 0.0, 1.0, 0.0}, // 0
	{-1.0,  1.0,  1.0, -1.0, 0.0, 0.0, 1.0, 1.0}, // 2
	{-1.0,  1.0, -1.0, -1.0, 0.0, 0.0, 0.0, 1.0}, // 3

	{ 1.0, -1.0,  1.0,  1.0, 0.0, 0.0, 0.0, 0.0}, // 1
	{ 1.0, -1.0, -1.0,  1.0, 0.0, 0.0, 1.0, 0.0}, // 4
	{-1.0,  1.0, -1.0, 0.0, 0.0, -1.0, 1.0, 1.0}, // 3
	{-1.0,  1.0,  1.0, 0.0, 0.0, 1.0, 0.0, 1.0}, // 2

    {-1.0, -1.0, -1.0,  0.0, -1.0, 0.0, 0.0, 0.0}, // 5
	{ 1.0, -1.0, -1.0,  0.0, -1.0, 0.0, 1.0, 0.0}, // 4
	{ 1.0, -1.0,  1.0,  0.0, -1.0, 0.0, 1.0, 1.0}, // 1
	{-1.0, -1.0,  1.0,  0.0, -1.0, 0.0, 0.0, 1.0}, // 0
};

/* pretty much just it revered */

GLfloat rev_slant_vertices[][8] =
{
    /*  X     Y     Z   Nx   Ny   Nz    S    T */
    {-1.0, 1.0,  1.0,  0.0,  1.0, 0.0, 0.0, 0.0}, // 3
	{ 1.0, 1.0,  1.0,  0.0,  1.0, 0.0, 1.0, 0.0}, // 2
	{ 1.0, 1.0, -1.0,  0.0,  1.0, 0.0, 1.0, 1.0}, // 7
	{-1.0, 1.0, -1.0,  0.0,  1.0, 0.0, 0.0, 1.0}, // 6

	{-1.0,  1.0,  1.0, 0.0, 0.0, 1.0, 0.0, 1.0}, // 3
    { 1.0,  1.0,  1.0, 0.0, 0.0, 1.0, 1.0, 1.0}, // 2
	{ 1.0, -1.0,  1.0, 0.0, 0.0, 1.0, 1.0, 0.0}, // 1
	{ 1.0, -1.0,  1.0, 0.0, 0.0, 1.0, 1.0, 0.0}, // 1

	{-1.0,  1.0, -1.0, 0.0, 0.0, -1.0, 1.0, 1.0}, // 6
    { 1.0,  1.0, -1.0, 0.0, 0.0, -1.0, 0.0, 1.0}, // 7
    { 1.0, -1.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0}, // 4
    { 1.0, -1.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0}, // 4

	{-1.0,  1.0,  1.0, -1.0, 0.0, 0.0, 1.0, 1.0}, // 3
    { 1.0, -1.0,  1.0,  1.0, 0.0, 0.0, 0.0, 0.0}, // 1
	{ 1.0, -1.0, -1.0,  1.0, 0.0, 0.0, 1.0, 0.0}, // 4
	{-1.0,  1.0, -1.0, -1.0, 0.0, 0.0, 0.0, 1.0}, // 6

	{ 1.0, -1.0,  1.0,  1.0, 0.0, 0.0, 0.0, 0.0}, // 1
	{ 1.0, -1.0, -1.0,  1.0, 0.0, 0.0, 1.0, 0.0}, // 4
	{ 1.0,  1.0, -1.0,  1.0, 0.0, 0.0, 1.0, 1.0}, // 7
	{ 1.0,  1.0,  1.0,  1.0, 0.0, 0.0, 0.0, 1.0}, // 2
};

static void draw_cube(void)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(GLfloat) * 8, &cube_vertices[0][0]);
	glNormalPointer(GL_FLOAT, sizeof(GLfloat) * 8, &cube_vertices[0][3]);
	glTexCoordPointer(2, GL_FLOAT, sizeof(GLfloat) * 8, &cube_vertices[0][6]);

	glDrawArrays(GL_QUADS, 0, 24);
}

static void draw_slant(void)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(GLfloat) * 8, &slant_vertices[0][0]);
	glNormalPointer(GL_FLOAT, sizeof(GLfloat) * 8, &slant_vertices[0][3]);
	glTexCoordPointer(2, GL_FLOAT, sizeof(GLfloat) * 8, &slant_vertices[0][6]);

	glDrawArrays(GL_QUADS, 0, 20);
}

static void draw_rev_slant(void)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(GLfloat) * 8, &rev_slant_vertices[0][0]);
	glNormalPointer(GL_FLOAT, sizeof(GLfloat) * 8, &rev_slant_vertices[0][3]);
	glTexCoordPointer(2, GL_FLOAT, sizeof(GLfloat) * 8, &rev_slant_vertices[0][6]);

	glDrawArrays(GL_QUADS, 0, 20);
}

float const light0_dir[]={0,1,0,0};
float const light0_color[]={78./255., 80./255., 184./255.,1};

float const light1_dir[]={-1,1,1,0};
float const light1_color[]={255./255., 220./255., 97./255.,1};

float const light2_dir[]={0,-1,0,0};
float const light2_color[]={31./255., 75./255., 16./255.,1};

static void redrawTheWindow(volatile touchdata & point)
{
    if (point.finger == 10)
    {
        exit(0);
    }
	float const aspect = (float)width / (float)height;

	static float ax = 0;
	static float ay = 0;
	static float az = 0;
	static float ax1 = 0;
	static float ay1 = 0;
	static float az1 = 0;

	glDrawBuffer(GL_BACK);

	glViewport(0, 0, width, height);

	// Clear with alpha = 0.0, i.e. full transparency
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-aspect, aspect, -1, 1, 2.5, 10);

    if (moretouches == false)
    {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

    #if 0
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    #endif

        glLightfv(GL_LIGHT0, GL_POSITION, light0_dir);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_color);

        glLightfv(GL_LIGHT1, GL_POSITION, light1_dir);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_color);

        glLightfv(GL_LIGHT2, GL_POSITION, light2_dir);
        glLightfv(GL_LIGHT2, GL_DIFFUSE, light2_color);

        glTranslatef(0., 0., -5.);

        glRotatef(ax, 1, 0, 0);
        glRotatef(ay, 0, 1, 0);
        glRotatef(az, 0, 0, 1);

        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glEnable(GL_LIGHTING);

        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

        glColor4f(1., 1., 1., 0.5);

        glCullFace(GL_FRONT);
        draw_cube();
        glCullFace(GL_BACK);
        draw_cube();

        if (point.finger == 1)
        {
            ax = fmod(point.x + 0.01, 360.);
            ay = fmod(point.y + 0.10, 360.);
            az = fmod(point.x/2 + point.y/2 + 0.25, 360.);
        }
    }
    else
    {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

    #if 0
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    #endif

        glLightfv(GL_LIGHT0, GL_POSITION, light0_dir);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_color);

        glLightfv(GL_LIGHT1, GL_POSITION, light1_dir);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_color);

        glLightfv(GL_LIGHT2, GL_POSITION, light2_dir);
        glLightfv(GL_LIGHT2, GL_DIFFUSE, light2_color);

        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glEnable(GL_LIGHTING);

        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

        glColor4f(1., 1., 1., 0.5);

        glPushMatrix();

        glTranslatef(0., 0., -5.);
        glRotatef(ax, 1, 0, 0);
        glRotatef(ay, 0, 1, 0);
        glRotatef(az, 0, 0, 1);

        glCullFace(GL_FRONT);
        draw_slant();
        glCullFace(GL_BACK);
        draw_slant();

        glPopMatrix();

        if (point.finger == 1)
        {
            ax = fmod(point.x + 0.01, 360.);
            ay = fmod(point.y + 0.10, 360.);
            az = fmod(point.x/2 + point.y/2 + 0.25, 360.);
        }

        glPushMatrix();

        glTranslatef(0., 0., -5.);
        glRotatef(ax1, 1, 0, 0);
        glRotatef(ay1, 0, 1, 0);
        glRotatef(az1, 0, 0, 1);

        glPushMatrix();
        glCullFace(GL_FRONT);
        draw_rev_slant();
        glCullFace(GL_BACK);
        draw_rev_slant();

        glPopMatrix();

        if (point.finger == 2)
        {
            ax1 = fmod(point.x + 0.01, 360.);
            ay1 = fmod(point.y + 0.10, 360.);
            az1 = fmod(point.x/2 + point.y/2 + 0.25, 360.);
        }
    }
 	glXSwapBuffers(Xdisplay, glX_window_handle);
}

static void select_events(Display *dpy, Window win)
{
    XIEventMask eventmask;
    eventmask.deviceid = deviceid;
    eventmask.mask_len = XIMaskLen(XI_LASTEVENT);
    eventmask.mask = (unsigned char*) calloc(eventmask.mask_len, sizeof(char));
    XISetMask(eventmask.mask, XI_TouchBegin);
    XISetMask(eventmask.mask, XI_TouchUpdate);
    XISetMask(eventmask.mask, XI_TouchEnd);
    XISetMask(eventmask.mask, XI_TouchOwnership);
    XISelectEvents(dpy, win, &eventmask, 1);
    XFlush(dpy);
}

void cookieAndEvents(XEvent ev, int xi_opcode, touchdata & point, vector<touchdata> & touchdatareal, int data) //this has to be ran in a loop
{
    xi_opcode = 131;
    XGenericEventCookie *cookie = &ev.xcookie;
    XIDeviceEvent *eventnt;
    while (XNextEvent(dpy, &ev) >= 0)
    {
        if (XGetEventData(dpy, cookie) && cookie->type == GenericEvent && cookie->extension == xi_opcode)
        {
            eventnt = (XIDeviceEvent *) cookie->data;
            /* quick drop that event it's going to cause lag */
            if (eventnt->evtype != XI_TouchBegin && eventnt->evtype != XI_TouchUpdate && eventnt->evtype != XI_TouchEnd && eventnt->evtype != XI_TouchOwnership)
            {
                XPutBackEvent(dpy, &ev);
                cout << "NOT A TOUCH EVENT DROPING: " << eventnt->evtype << endl;
            }
            if (eventnt->evtype == XI_TouchBegin)
            {
                touchdatabegin(eventnt->detail, touchdatareal, eventnt->event_x, eventnt->event_y);
                if (data == 1)
                {
                    for (vector<touchdata>::iterator it = touchdatareal.begin(); it != touchdatareal.end(); ++it)
                    {
                        if ((*it).touchid == eventnt->detail)
                        {
                            cout << "=========================" << endl;
                            cout << "       touch data;       " << endl;
                            cout << "|| finger:         " << it->finger << endl;
                            cout << "|| x:              " << it->x << endl;
                            cout << "|| y:              " << it->y << endl;
                            cout << "|| touchid:        " << it->touchid << endl;
                            cout << "|| evtype = XI_TouchBegin" << endl;
                            cout << "|| !adding finger!       " << endl;
                            cout << "_________________________" << endl;
                            break;
                        }
                    }
                }
                for (vector<touchdata>::iterator it = touchdatareal.begin(); it != touchdatareal.end(); ++it)
                {
                    if ((*it).touchid == eventnt->detail)
                    {
                        if ((*it).finger > 1)
                        {
                            moretouches = true;
                            break;
                        }
                    }
                }
            }
            if (eventnt->evtype == XI_TouchUpdate && eventnt->evtype != XI_TouchEnd)
            {
                touchDataUpdate(eventnt->detail, touchdatareal, eventnt->event_x, eventnt->event_y);
                if (data == 1)
                {
                    for (vector<touchdata>::iterator it = touchdatareal.begin(); it != touchdatareal.end(); ++it)
                    {
                        if ((*it).touchid == eventnt->detail)
                        {
                            cout << "=========================" << endl;
                            cout << "       touch data;       " << endl;
                            cout << "|| finger:         " << it->finger << endl;
                            cout << "|| x:              " << it->x << endl;
                            cout << "|| y:              " << it->y << endl;
                            cout << "|| touchid:        " << it->touchid << endl;
                            cout << "|| evtype = XI_TouchUpdate" << endl;
                            cout << "|| !updateing data!      " << endl;
                            cout << "_________________________" << endl;
                            break;
                        }
                    }
                }
                Window temp0 = eventnt->root;
                Window temp1 = eventnt->child;
                int		x_return;
                int		y_return;
                unsigned int	width_return;
                unsigned int	height_return;
                unsigned int	border_width_return;
                unsigned int	depth_return;
                Window wins;
                XGetGeometry(dpy, window_handle, &wins, &x_return, &y_return, &width_return, &height_return, &border_width_return, &depth_return);
                if (win == temp0 && window_handle == temp1 && touchdatareal[0].x > 0 && touchdatareal[0].y > 0 && touchdatareal[0].x < width_return, touchdatareal[0].y < height_return)
                {
                    //cout << "running2" << endl;
                    /*if (mq_send(msgq, (char * ) &point, sizeof(point), 1) == (mqd_t) -1)
                    {
                        perror("wirght");
                    }*/
                    //cout << "running3" << endl;
                    /*if (write(writepipefd, &point, sizeof(point)) == -1)
                    {
                        perror("!!!WRITE getting blocked!!!");
                    }*/
                    for (vector<touchdata>::iterator it = touchdatareal.begin(); it != touchdatareal.end(); ++it)
                    {
                        if ((*it).touchid == eventnt->detail)
                        {
                            point = (*it); //all the values are updated
                            //shareddata.insert(it, point);
                            shareddata[point.finger - 1] = point;
                            break;
                        }
                    }
                }
            }
            if (eventnt->evtype == XI_TouchEnd)
            {
                if (data == 1)
                {
                    for (vector<touchdata>::iterator it = touchdatareal.begin(); it != touchdatareal.end(); ++it)
                    {
                        if ((*it).touchid == eventnt->detail)
                        {
                            cout << "=========================" << endl;
                            cout << "       touch data;       " << endl;
                            cout << "|| finger:         " << it->finger << endl;
                            cout << "|| x:              " << it->x << endl;
                            cout << "|| y:              " << it->y << endl;
                            cout << "|| touchid:        " << it->touchid << endl;
                            cout << "|| evtype = XI_TouchEnd  " << endl;
                            cout << "|| !removeing finger!    " << endl;
                            cout << "_________________________" << endl;
                            break;
                        }
                    }
                }
                for (vector<touchdata>::iterator it = touchdatareal.begin(); it != touchdatareal.end(); ++it)
                {
                    if ((*it).touchid == eventnt->detail)
                    {
                        //shareddata.erase(it);
                        /* erase it */
                        shareddata[(*it).finger - 1];
                        if ((*it).finger <= 2)
                        {
                            moretouches = false;
                            break;
                        }
                    }
                }
                touchDataEnd(eventnt->detail, touchdatareal);
            }
            /*for (vector<touchdata>::iterator it = touchdatareal.begin(); it != touchdatareal.end(); ++it)
            {
                cout << (*it).touchid << " = " << eventnt->detail << endl;
            } */
        }
        /* clean up */
        XFlush(dpy);
        XFreeEventData(dpy, cookie);
    }
}

int touchdatabegin(int touchid, vector<touchdata> & ff, double x, double y)
{
    touchdata ffaacc;
    ffaacc.touchid = touchid;
    ffaacc.x = x;
    ffaacc.y = y;
    if (ff.size() == 0)
    {
        ffaacc.finger = 1;
        ff.push_back(ffaacc);
        return ffaacc.finger;
    }
    ffaacc.finger = ff[ff.size() - 1].finger + 1;
    ff.push_back(ffaacc);
    return ffaacc.finger;
}

int touchDataUpdate(int touchid, vector<touchdata> & ff, double x, double y)
{
    if (ff.size() == 0)
    {
        return -1;
    }
    for(vector<touchdata>::iterator it = ff.begin(); it != ff.end(); ++it)
    {
        if(it->touchid == touchid)
        {
            it->x = x;
            it->y = y;
            return it->finger;
        }
    }
    return -1;
}

int touchDataEnd(int touchid, vector<touchdata> & ff)
{
    if (ff.size() == 0)
    {
        return -1;
    }
    for(vector<touchdata>::iterator it = ff.begin(); it != ff.end(); ++it)
    {
        if (it->touchid == touchid)
        {
            int temp = it->finger;
            for(vector<touchdata>::iterator itz = it + 1; itz <= ff.end(); ++itz)
            {
                itz->finger = itz->finger - 1;
            }
            ff.erase(it);
            return temp;
        }
    }
    return -1;
}

void * glDrawingThreid(void *)
{
    createTheWindow();
    createTheRenderContext();
    volatile touchdata ffa;
    redrawTheWindow(ffa);
    touchdata recevdata[FINGER_COUNT];
    /*mq_attr messattr;
    messattr.mq_msgsize = sizeof(point);
    messattr.mq_flags = O_NONBLOCK;
    messattr.mq_maxmsg = 9;
    messattr.mq_curmsgs = 0;
    static mqd_t msgq = mq_open(msgqueuename.c_str(), O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWUSR | S_IROTH | S_IWOTH, &messattr);
    //cout << msgq << endl;
    if (msgq == (mqd_t) -1)
    {
        perror("s");
    } */
    syncpt = true;
    while(updateTheMessageQueue())
    {
        /*if (read(readpipefd, &point, sizeof(point)) == -1);
        {
            perror("!!!READ getting blocked!!!");
        }*/
        //unsigned int ff;
        //cout << "running1" << endl;
        /*if (mq_receive(msgq, (char *) &point, sizeof(point), & ff) == (mqd_t) -1)
        {
            perror("read");
        }*/
        //cout << ff << endl;
        //vector<touchdata> recevdata(shareddata);
        //copy(shareddata, shareddata+(sizeof(shareddata)/sizeof(touchdata)) - 1, recevdata);
        for (int i = 0; i < sizeof(shareddata)/sizeof(touchdata); i++)
        {
            redrawTheWindow(shareddata[i]);
        }
    }

    return NULL;
}

void glDevied(int ownership)
{

}
