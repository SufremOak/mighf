// Build instructions:
// To compile this program, use the following command:
// gcc -o mux-window mux-window.c -lX11

#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    Display *display;
    Window window;
    XEvent event;
    int screen;

    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    screen = DefaultScreen(display);
    window = XCreateSimpleWindow(display, RootWindow(display, screen), 
                                 100, 100, 400, 300, 1,
                                 BlackPixel(display, screen), 
                                 WhitePixel(display, screen));

    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XMapWindow(display, window);

    while (1) {
        XNextEvent(display, &event);
        if (event.type == Expose) {
            XFillRectangle(display, window, DefaultGC(display, screen), 20, 20, 10, 10);
            XDrawString(display, window, DefaultGC(display, screen), 50, 50, "Hello, X11!", 11);
        }
        if (event.type == KeyPress)
            break;
    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
}