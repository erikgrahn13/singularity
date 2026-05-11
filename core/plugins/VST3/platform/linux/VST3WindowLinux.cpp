#include "VST3Window.h"
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h> // For GtkPlug (GTK3 X11-specific)
#include <iostream>
#include <webkit2/webkit2.h>

void *VST3Window::createPlatformWindow(void *parent)
{
    // PARENT: return something WebView can use
    return parent;
}