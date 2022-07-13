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

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput2.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>


#define IMPL_KEY(key) msg = strdup(key); \
                      val->min_width = tab_width; \
                      break;

void change_opacity(Display* display, Window* window, double alpha) {
    unsigned long opacity = (unsigned long)(0xFFFFFFFFul * alpha);
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

    Display* real_display; real_display = XOpenDisplay(NULL);

    if(!real_display) {
        fprintf(stderr, "cannot open display\n");
        return 1;
    }
    
    Window window,
           real_window;

    XColor text_color,
           background_color;

    XFontStruct* font_structure;

    char* msg = (char*)malloc(64); msg = (char*)"hi";
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
        } else */if((strcmp(argv[i], "-r") == 0) && i + 1 < argc) {
            text_color.red = atoi(argv[++i]);
        } if((strcmp(argv[i], "-g")) && i + 1 < argc) {
            text_color.green = atoi(argv[++i]);
        } if((strcmp(argv[i], "-b") == 0) && i + 1 < argc) {
            text_color.blue = atoi(argv[++i]);
        } if((strcmp(argv[i], "-br") == 0) && i + 1 < argc) {
            background_color.red = atoi(argv[++i]);
        } if((strcmp(argv[i], "-bg") == 0) && i + 1 < argc) {
            background_color.green = atoi(argv[++i]);
        } if((strcmp(argv[i], "-bb") == 0) && i + 1 < argc) {
            background_color.blue = atoi(argv[++i]);
        } if((strcmp(argv[i], "-o") == 0) && i + 1 < argc) {
            // TODO: Check given argument is valid
            alpha = atof(argv[++i]);
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
    XMapWindow(real_display, real_window);

    font_structure = XLoadQueryFont(real_display, font);

    if(!font) { 
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

    XTextExtents(font_structure, msg, strlen(msg), 
                &direction, &ascent, &descent, &overall);
    
    XWindowAttributes ra;
    Atom window_type = XInternAtom(real_display, "_NET_WM_WINDOW_TYPE", False);
    long value = XInternAtom(real_display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    
    unsigned long x_axis = XWidthOfScreen(DefaultScreenOfDisplay(real_display)) / 3,
                  y_axis = 3 * XHeightOfScreen(DefaultScreenOfDisplay(real_display)) / 4;

    XChangeProperty(real_display, real_window, window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *) &value, 1);
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

        XNextEvent(real_display, &event);
        if (XGetEventData(real_display, cookie)) {
            // Handle key press and release events
            if (cookie->type == GenericEvent
                    && cookie->extension == xiOpcode) {
                if (cookie->evtype == XI_RawKeyRelease
                        || cookie->evtype == XI_RawKeyPress) {
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
                    char *str = XKeysymToString(s);          
                    if (NULL == str) continue;
                    change_opacity(real_display, &real_window, alpha);

                    switch(ev->detail) {
                        default: {
                            size_t len = strlen(str);

                            if(len == 1) {
                                ret: font = "-misc-fixed-bold-r-normal--135-0-1300-1300-c-0-iso8859-13";
                                font_structure = XLoadQueryFont(real_display, font);
                                
                                XTextExtents(font_structure, str, len, 
                                                            &direction, 
                                                            &ascent, 
                                                            &descent, 
                                                            &overall);

                                XSetFont(real_display, DefaultGC(real_display, real_s), font_structure->fid);    

                                XClearWindow(real_display, real_window);
                                XDrawString(real_display, real_window, DefaultGC(real_display, real_s), ra.width / 7, 
                                                            ra.height / 8, 
                                                            str, len);

                                break;
                            } else if(len == 2) {
                                font = "-misc-fixed-bold-r-normal--130-0-1300-1300-c-0-iso8859-13";
                                font_structure = XLoadQueryFont(real_display, font);
                                
                                XTextExtents(font_structure, str, len, 
                                                            &direction, 
                                                            &ascent, 
                                                            &descent, 
                                                            &overall);

                                XSetFont(real_display, DefaultGC(real_display, real_s), font_structure->fid);    


                                XClearWindow(real_display, real_window);
                                XDrawString(real_display, real_window, DefaultGC(real_display, real_s), ra.width / 8, 
                                                            ra.height / 8, 
                                                            str, len);

                                break;
                            } else if(len == 3) {
                                font = "-misc-fixed-bold-r-normal--125-0-1300-1300-c-0-iso8859-13";
                                font_structure = XLoadQueryFont(real_display, font);

                                XTextExtents(font_structure, str, len, 
                                                            &direction, 
                                                            &ascent, 
                                                            &descent, 
                                                            &overall);
                                XSetFont(real_display, DefaultGC(real_display, real_s), font_structure->fid);    

                                XClearWindow(real_display, real_window);
                                XDrawString(real_display, real_window, DefaultGC(real_display, real_s), ra.width / 12, 
                                                            ra.height / 8,
                                                              str, len);
                                break;
                            } else if(len == 4) {
                                font = "-misc-fixed-bold-r-normal--120-0-950-950-c-0-iso8859-13";
                                font_structure = XLoadQueryFont(real_display, font);
                                
                                XTextExtents(font_structure, str, len, 
                                                            &direction, 
                                                            &ascent, 
                                                            &descent, 
                                                            &overall);
                                XSetFont(real_display, DefaultGC(real_display, real_s), font_structure->fid);    

                                XClearWindow(real_display, real_window);
                                XDrawString(real_display, real_window, DefaultGC(real_display, real_s), ra.width / 20,
                                                            ra.height / 8, 
                                                            str, len); 
                                break;
                            } else if(len == 5) {
                                if(strcmp(str, "grave") == 0) {
                                    str = strdup("`"); len = 1; goto ret;    
                                } else if(strcmp(str, "comma") == 0) {
                                    str = strdup(","); len = 1; goto ret;
                                } else if(strcmp(str, "slash") == 0) {
                                    str = strdup("/"); len = 1; goto ret;
                                } else if(strcmp(str, "equal") == 0) {
                                    str = strdup("="); len = 1; goto ret;  
                                } else if(strcmp(str, "minus") == 0) {
                                    str = strdup("-"); len = 1; goto ret;    
                                } else if(strcmp(str, "space") == 0) {
                                    str = strdup("Space");
                                } else if(strcmp(str, "KP_Up") == 0) {
                                    XMoveWindow(real_display, real_window, x_axis, y_axis - 10);
                                    y_axis -= 10;
                                }

                                font = "-misc-fixed-bold-r-normal--115-0-950-950-c-0-iso8859-13";
                                font_structure = XLoadQueryFont(real_display, font);
                                
                                XTextExtents(font_structure, str, len, 
                                                            &direction, 
                                                            &ascent, 
                                                            &descent, 
                                                            &overall);

                                XSetFont(real_display, DefaultGC(real_display, real_s), font_structure->fid);    

                                XClearWindow(real_display, real_window);
                                XDrawString(real_display, real_window, DefaultGC(real_display, real_s), ra.width / 25, 
                                                            ra.height / 8, 
                                                            str, len);
                                break;
                            } else if(len == 6) {
                                if(strcmp(str, "period") == 0) {
                                    str = strdup("."); len = 1; goto ret;    
                                }

                                font = "-misc-fixed-bold-r-normal--110-0-950-950-c-0-iso8859-13";
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
                                XDrawString(real_display, real_window, DefaultGC(real_display, real_s), ra.width / 50, 
                                                            ra.height / 8, 
                                                            str, len);
                                break;
                            } else if(len == 7) {
                                if(strcmp(str, "KP_Left") == 0) {
                                    XMoveWindow(real_display, real_window, x_axis - 10, y_axis);
                                    x_axis -= 10;
                                } else if(strcmp(str, "KP_Down") == 0) {
                                    XMoveWindow(real_display, real_window, x_axis, y_axis + 10);
                                    y_axis += 10;
                                }

                                font = "-misc-fixed-bold-r-normal--95-0-950-950-c-0-iso8859-13";
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
                                XDrawString(real_display, real_window, DefaultGC(real_display, real_s), ra.width / 50, 
                                                            ra.height / 8, 
                                                            str, len);
                                break;
                            } else if(len == 8) {
                                if(strcmp(str, "KP_Right") == 0) {
                                    XMoveWindow(real_display, real_window, x_axis + 10, y_axis);
                                    x_axis += 10;
                                }
                                
                                font = "-misc-fixed-bold-r-normal--85-0-950-950-c-0-iso8859-13";
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
                                XDrawString(real_display, real_window, DefaultGC(real_display, real_s), ra.width / 50, 
                                                            ra.height / 8, 
                                                            str, len);
                                break;
                            } else if(len == 9) {
                                if(strcmp(str, "backslash") == 0) {
                                    str = strdup("\\"); len = 1; goto ret;    
                                } else if(strcmp(str, "semicolon") == 0) {
                                    str = strdup(";"); len = 1; goto ret;
                                }

                                font = "-misc-fixed-bold-r-normal--75-0-950-950-c-0-iso8859-13";
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
                                XDrawString(real_display, real_window, DefaultGC(real_display, real_s), ra.width / 55, 
                                                            ra.height / 8, 
                                                            str, len);
                                break;  
                            } else if(len == 10) {
                                if(strcmp(str, "apostrophe") == 0) {
                                    str = strdup("\'"); len = 1; goto ret;
                                }

                                font = "-misc-fixed-bold-r-normal--65-0-950-950-c-0-iso8859-13";
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
                                XDrawString(real_display, real_window, DefaultGC(real_display, real_s), ra.width / 55, 
                                                            ra.height / 8, 
                                                            str, len);
                                break;  
                            } else if(len == 11) {
                                if(strcmp(str, "bracketleft") == 0) {
                                    str = strdup("["); len = 1; goto ret;
                                }
                                
                                font = "-misc-fixed-bold-r-normal--60-0-950-950-c-0-iso8859-13";
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
                                XDrawString(real_display, real_window, DefaultGC(real_display, real_s), ra.width / 55, 
                                                            ra.height / 8, 
                                                            str, len);
                                break;
                            } else if(strcmp(str, "bracketright") == 0) {
                                str = strdup("]"); len = 1; goto ret;
                            } else {
                                font = "-misc-fixed-bold-r-normal--130-0-950-950-c-0-iso8859-13";
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
                            XDrawString(real_display, real_window, DefaultGC(real_display, real_s), ra.width / 20, 
                                                            ra.height / 8, 
                                                            str, len);
                            break;
                        }
                    }
                }
            }

            XFreeEventData(real_display, cookie);
        } else {
            if(event.type == xkbEventCode) {
                XkbEvent *xkbEvent = (XkbEvent*)&event;
                if (xkbEvent->any.xkb_type == XkbStateNotify) {
                    group = xkbEvent->state.group;
                }
            }
        }
    }

    free(msg);
    XCloseDisplay(real_display);
}