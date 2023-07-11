// tush - simple x11 keycaster
// ---------------------------
// github.com/ferhatgec/tush
//
// Features:
// ---------
// * Custom text-background color support
//  - for text:
//     * -r rgb_red
//     * -g rgb_green
//     * -b rgb_blue
//  - for background:
//     * -rb rgb_red
//     * -gb rgb_green
//     * -bb rgb_blue
// * Custom opacity support
//  * -o opacity (between 0.0 to 1.0)
// * Axis move support.
//  * KP_UP: moves 10 pixel along +y
//  * KP_DOWN: moves 10 pixel along -y
//  * KP_LEFT: moves 10 pixel along +x
//  * KP_RIGHT: moves 10 pixel along -x
//  ! There's no any TKL keyboard equivalent
//    yet.
//
// MIT License
//
// Copyright (c) 2022 Ferhat Geçdoğan All Rights Reserved.
// Distributed under the terms of the MIT License.
//
//


/*
NOTE : STRDUP ALLOCATES a new string!!!

this means everytime you do strdup you should be freeing the previous string!

this also means it's slower and uses more ram if you use strdup

(this comment was just made after I noticed how much you use strdup)
(I will try to find a fix to this, probably by replacing strdup)


NOTE :

pretty sure there isn't any way to close the program, this means the window doesn't
properly close and this may lead to memory leaks
*/


#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput2.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
 
/* this is never used */
#define IMPL_KEY(key) msg = strdup(key); \
                      val->min_width = tab_width; \
                      break;

/* 
    not sure what actually to name this
    but it's so long and weird that it'd probably be better to give 
    make it a constant rather than just putting it in the code
*/

/* 
    enforcing, objectively, the best c string size check so it works as fast as it can 
    [
        note: most effective form of protection is abstinence, so 
        using a char* header method (or having a int with the char*)
        would be the best way
    ]
*/
size_t cstrlen(const char* cstr) {
	char* s;
	for (s = (char*)cstr; *s; s++);

	return (s - cstr);
}

#define BASE_ALPHA 0xFFFFFFFFul

void change_opacity(Display* display, Window* window, double alpha) {
    unsigned long opacity = (unsigned long)(BASE_ALPHA * alpha);
    Atom XA_NET_WM_WINDOW_OPACITY = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", False);

    XChangeProperty(display, *window, XA_NET_WM_WINDOW_OPACITY, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&opacity, 1L);
}

struct timespec _ts, _temp;

int main(int argc, char** argv) {
    if(argc > 1 && (strcmp(argv[1], "--help") == 0 
                || strcmp(argv[1], "-h") == 0)) {
        printf("tush - simple x11 keycaster\n"
               "usage: %s [args]\n"
               "args:\n"
               /*" -font .description. [within x logical font description style]\n"*/
               " -r .red.\n"
               " -g .green.\n"
               " -b .blue.\n"
               " -br .background red.\n"
               " -bg .background green.\n"
               " -bb .background blue.\n"
               " -o .opacity.\n", argv[0]);
        return 1;
    }
    
    /* not much reason to format your code like that */
    Display* real_display = XOpenDisplay(NULL);

    if(!real_display) {
        fprintf(stderr, "cannot open display\n");
        return 1;
    }
    
    Window window,
           real_window;

    XColor text_color,
           background_color;

    XFontStruct* font_structure;

    /* 
        since the standard string size is going to be 64, let's not allocate in the first place
        
        1) allocating is slower than this
        2) this just uses stack data so far less ram ussage
    */

    char msg[64] = "hi";

    char* font = "-misc-fixed-bold-r-normal--130-0-950-950-c-0-iso8859-13";
     
    int real_s;

    double alpha = 0.9;

    text_color.red = 65535;
    text_color.green = 65535;
    text_color.blue = 0;
    text_color.flags = DoRed | DoGreen | DoBlue;

    background_color.red = 2550;
    background_color.green = 2500;
    background_color.blue = 2500;
    background_color.flags = DoRed | DoGreen | DoBlue;
    
    XSizeHints* val;

    val = XAllocSizeHints();
    val->flags = PMinSize | PMaxSize;
    val->min_width = val->max_width = 450;
    val->min_height = val->max_height = 165;

    for(size_t i = 0; i < argc; ++i) {
        /*if((strcmp(argv[i], "-font") == 0) && (i + 1 < argc)) {
            char* token;
            unsigned count = 0;
            font = argv[++i];
            printf("aight\n");
            while((token = strsep(&font, "-"))) {
                if(count == 6) {
                    token = "150";
                } else if(count == 7) {
                    token = "0";
                } else if(count == 8 || count == 9) {
                    printf("aight\n");
                    token = "990";
                } ++count;
            }
        } else */
        
        /* 
            strcmp is kinda slow 
            plus we're only looking for a one char thing [in most cases]

            so, I'll replace this part just by checking if there is a 
            '-'
            then check what the next char is
        */

        /* cstrlen is still kinda slow [just because it's a for loop] so it's best to only check once */
        size_t argLen = cstrlen(argv[i]);

        if (argv[i][0] != '-' || i + 1 >= argc || 
            (argv[i][1] > 'b' && argLen > 2) || 
            (argv[i][1] == 'b' && cstrlen(argv[i]) > 3)
        )
            continue;

        switch (argv[i][1]) {
            case 'r':
                text_color.red = atoi(argv[++i]);
            case 'g':
                text_color.green = atoi(argv[++i]);
            case 'b':
                switch (argv[i][2]) {
                    case 'r':
                        background_color.red = atoi(argv[++i]);
                        break;
                    case 'g':
                        background_color.green = atoi(argv[++i]);
                        break;
                    case 'b':
                        background_color.blue = atoi(argv[++i]);
                        break;
                    default:
                        if (argLen == 2)
                            text_color.blue = atoi(argv[++i]);
                        break;
                }
                
            case 'o':
                // TODO: Check given argument is valid
                alpha = atof(argv[++i]);
                break;
            
            default: break;
        }
    }

    real_s = DefaultScreen(real_display);

    XAllocColor(real_display, 
                XDefaultColormap(real_display, real_s), 
                &background_color);
    
    XAllocColor(real_display, 
                XDefaultColormap(real_display, real_s), 
                &text_color);

    window = DefaultRootWindow(real_display);
    real_window = XCreateSimpleWindow(real_display, RootWindow(real_display, real_s), 10, 10, val->min_width, 
                                                                        val->min_height, 
                                                                        1, 
                                                                        1, 
                                                                        background_color.pixel);  
     
    int xiOpcode = 0;

    { 
        int queryEvent, queryError;
        
        if(!XQueryExtension(real_display, "XInputExtension", &xiOpcode,
                    &queryEvent, &queryError)) {
            fprintf(stderr, "XInputExtension not available\n");
            return 1;
        }
    
        int major = 2, minor = 0;
        int queryResult = XIQueryVersion(real_display, &major, &minor);

        if(queryResult == BadRequest) {
            fprintf(stderr, "Need XI 2.0 support (got %d.%d)\n", major, minor);
            return 1;
        } else if(queryResult != Success) {
            fprintf(stderr, "XIQueryVersion failed\n");
            return 1;
        }
    } 
    
    Bool f12_pressed = False;

    int xkbEventCode = 0;
    XIEventMask m;
    m.deviceid = XIAllMasterDevices;
    m.mask_len = XIMaskLen(XI_LASTEVENT);
    m.mask = calloc(m.mask_len, sizeof(char));
    
    XISetMask(m.mask, XI_RawKeyPress);
    XISetMask(m.mask, XI_RawKeyRelease);
    XISelectEvents(real_display, window, &m, 1 /*number of masks*/);
    XSync(real_display, False);
    free(m.mask);       
    
    XkbSelectEventDetails(real_display, XkbUseCoreKbd, XkbStateNotify,
            XkbGroupStateMask, XkbGroupStateMask);
    int group;
    
    {
        XkbStateRec state;
        XkbGetState(real_display, XkbUseCoreKbd, &state);
        group = state.group;
    }                                                              
    
    // XSetWMSizeHints(real_display, real_window, val, XA_WM_NORMAL_HINTS);
    XSelectInput(real_display, real_window, KeyReleaseMask | KeyPressMask);

    /* 
        this should be done before the window maps
        otherwise there is a bug inwhich 
        the window is rendered with a border still
    */

    Atom window_type = XInternAtom(real_display, "_NET_WM_WINDOW_TYPE", False);
    long value = XInternAtom(real_display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(real_display, real_window, window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *) &value, 1);

    XMapWindow(real_display, real_window);

    font_structure = XLoadQueryFont(real_display, font);

    /* 
        I got an seg fault because !font_stucture wasn't caught 
    
        also, I didn't have the font on my system and needed to install 
        a package for it, so I added a note on the README
    */

    if(!font || !font_structure) { 
        fprintf(stderr, "cannot load font: '%s'\n", font);
        return 1;
    }

    XSetFont(real_display, DefaultGC(real_display, real_s), font_structure->fid);
    
    int direction,
        ascent,
        descent;

    XResizeWindow(real_display, real_window, val->min_width, val->min_height);
    XSetForeground(real_display, DefaultGC(real_display, real_s), text_color.pixel);
    
    XCharStruct overall;

    /* 
        strlen(msg) is quite slow, it's better to use the size if we know what it is 
    
        we know it is 2.
    */
    
    XTextExtents(font_structure, msg, 2, 
                &direction, &ascent, &descent, &overall);

    XWindowAttributes ra;

    unsigned long x_axis = XWidthOfScreen(DefaultScreenOfDisplay(real_display)) / 3,
                  y_axis = 3 * XHeightOfScreen(DefaultScreenOfDisplay(real_display)) / 4;

    XMoveWindow(real_display, real_window, x_axis, y_axis);
    
    change_opacity(real_display, &real_window, 0.0);
    
    while(1) {
        XEvent event;
        XGenericEventCookie *cookie = (XGenericEventCookie*)&event.xcookie;
        XGetWindowAttributes(real_display, DefaultRootWindow(real_display), &ra);

        timespec_get(&_temp, TIME_UTC);

        if(((unsigned long)_temp.tv_sec - _ts.tv_sec) > 2) {
            change_opacity(real_display, &real_window, alpha);
        }

        /* 
            not actually sure why you use the Xi raw 
            event system here
            I'd think you could've used pure Xlib

            ¯\_(ツ)_/¯
        */

        XNextEvent(real_display, &event);

        /* 
            if we check !XGetEventData here instead of waiting for the else, we can just skip the nesting

            this makes the code a bit cleaner, not sure if it does much for the actual performance
        */

        if (!XGetEventData(real_display, cookie)) {
            if(event.type == xkbEventCode) {
                XkbEvent *xkbEvent = (XkbEvent*)&event;
                if (xkbEvent->any.xkb_type == XkbStateNotify) {
                    group = xkbEvent->state.group;
                }
            }

            continue;
        }

        // Handle key press and release events
        if (cookie->type == GenericEvent
                && cookie->extension == xiOpcode
                && (cookie->evtype == XI_RawKeyRelease /* not much reason to have this in it's own if statement */
            || cookie->evtype == XI_RawKeyPress)) {
                
                XIRawEvent *ev = cookie->data;

                KeySym s = XkbKeycodeToKeysym(
                        real_display, ev->detail, group, 0 /*shift level*/);

                if (NoSymbol == s) {
                    if (group == 0) continue;
                    else {
                        s = XkbKeycodeToKeysym(real_display, ev->detail,
                                0 /* base group */, 0 /*shift level*/);
                        if (NoSymbol == s) continue;
                    }
                }    

                change_opacity(real_display, &real_window, alpha);

                /* 
                    the switch is useless if there's only a default argument  
                
                    let's fix that!
                */

                /*
                    keycodes should be used here because
                    comparing the keycode is 100% faster
                    than comparing strings
                */

                char* str;
                size_t len = 1;

                switch(s) {
                    case XK_grave:
                        str = "`"; break;
                    case XK_comma:
                        str = ","; break;
                    case XK_slash:
                        str = "/"; break;
                    case XK_equal:
                        str = "="; break;
                    case XK_minus:
                        str = "-"; break;
                    case XK_space:
                        str[0] = 'S'; break;
                    case XK_period: str = "."; break;
                    case XK_backslash:
                        str = "\\";    
                        break;
                    case XK_semicolon:
                        str = ";";
                        break;
                    case XK_apostrophe:
                        str = "'";
                        break; 
                    case XK_bracketleft:
                        str = "[";
                        break; 
                    case XK_bracketright:
                        str = "]";

                    case XK_KP_Up:
                    case XK_KP_Left:
                    case XK_KP_Down:
                    case XK_KP_Right:
                        x_axis += (s == XK_KP_Left) ? -10 : (s == XK_KP_Right) ? 10 : 0;
                        y_axis += (s == XK_KP_Down) ? 10 : (s == XK_KP_Up) ? -10 : 0;
                        XMoveWindow(real_display, real_window, x_axis, y_axis);

                    default: 
                        str =  XKeysymToString(s);
                        len = cstrlen(str); 
                        break;
                }   

                /* 
                    switched to this array system
                    so you're not repeating the same block of code 100 times 
                */
               
                struct {const char* font; int num;} fonts[] = {
                                    {"-misc-fixed-bold-r-normal--135-0-1300-1300-c-0-iso8859-13", 7}, 
                                    {"-misc-fixed-bold-r-normal--130-0-1300-1300-c-0-iso8859-13", 8}, 
                                    {"-misc-fixed-bold-r-normal--125-0-1300-1300-c-0-iso8859-13", 12},
                                    {"-misc-fixed-bold-r-normal--120-0-950-950-c-0-iso8859-13", 20},
                                    {"-misc-fixed-bold-r-normal--115-0-950-950-c-0-iso8859-13", 25},
                                    {"-misc-fixed-bold-r-normal--110-0-950-950-c-0-iso8859-13", 50},
                                    {"-misc-fixed-bold-r-normal--95-0-950-950-c-0-iso8859-13", 50},
                                    {"-misc-fixed-bold-r-normal--85-0-950-950-c-0-iso8859-13", 50},
                                    {"-misc-fixed-bold-r-normal--75-0-950-950-c-0-iso8859-13", 55},
                                    {"-misc-fixed-bold-r-normal--65-0-950-950-c-0-iso8859-13", 55},
                                    {"-misc-fixed-bold-r-normal--60-0-950-950-c-0-iso8859-13", 55}
                                };

                int num = 20;
                font = "-misc-fixed-bold-r-normal--130-0-950-950-c-0-iso8859-13";

                if (len >= 1 && len <= 11) {
                    font = (char*)fonts[len].font;
                    num = fonts[len].num;
                }

                font_structure = XLoadQueryFont(real_display, font);
                    
                XTextExtents(font_structure, str, len, 
                                            &direction, 
                                            &ascent, 
                                            &descent, 
                                            &overall);

                XSetFont(real_display, DefaultGC(real_display, real_s), font_structure->fid);    

                XTextExtents(font_structure, str, len, 
                                                &direction, 
                                                &ascent, 
                                                &descent, 
                                                &overall);

                
                XClearWindow(real_display, real_window);
                XDrawString(real_display, real_window, DefaultGC(real_display, real_s), ra.width / num, 
                                                ra.height / 8, 
                                                str, len);

                XFreeEventData(real_display, cookie);
            }
    }
    
    XCloseDisplay(real_display);
}
