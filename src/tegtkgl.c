/**
* Copyright (c) 2013 André Diego Piske
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
* of the Software, and to permit persons to whom the Software is furnished to do
* so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
**/

#include "tegtkgl.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <X11/extensions/xf86vmode.h>

#include <GL/glx.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <glib-object.h>

struct _TeGtkgl_Priv {
    GdkDisplay *disp;
    GdkWindow *win;
    Display *xdis;
    Window xwin;
    GLXContext glc;
};

typedef struct _TeGtkgl_Priv TeGtkgl_Priv;

#define TE_GTKGL_GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), TE_TYPE_GTKGL, TeGtkgl_Priv))

G_DEFINE_TYPE(TeGtkgl, te_gtkgl, GTK_TYPE_WIDGET)

static void te_gtkgl_realize(GtkWidget *wid);
static void te_gtkgl_send_configure(GtkWidget *wid);
static void te_gtkgl_size_allocate(GtkWidget *wid, GtkAllocation *allocation);
static gboolean te_gtkgl_draw(GtkWidget *wid, cairo_t *cr);

static gboolean
te_gtkgl_draw(GtkWidget *wid, cairo_t *cr)
{
    /*
    TeGtkgl *gtkgl = TE_GTKGL(wid);
    TeGtkgl_Priv *priv = TE_GTKGL_GET_PRIV(gtkgl);
    */
    return FALSE;
}

static void
te_gtkgl_realize(GtkWidget *wid)
{
    TeGtkgl_Priv *priv;
    TeGtkgl *gtkgl = TE_GTKGL(wid);
    GdkWindowAttr attributes;
    GtkAllocation allocation;
    gint attributes_mask;
    XVisualInfo *vi;
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };

    priv = TE_GTKGL_GET_PRIV(gtkgl);

    gtk_widget_set_realized(wid, TRUE);
    gtk_widget_get_allocation(wid, &allocation);

    priv->disp = gdk_display_get_default();
    priv->xdis = gdk_x11_display_get_xdisplay(priv->disp);
    vi = glXChooseVisual(priv->xdis, 0, att);
    priv->glc = glXCreateContext(priv->xdis, vi, 0, GL_TRUE);

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gdk_visual_get_best_with_both(vi->depth,
        GDK_VISUAL_DIRECT_COLOR);
    attributes.event_mask = gtk_widget_get_events(wid);
    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

    priv->win = gdk_window_new(gtk_widget_get_parent_window(wid),
            &attributes, attributes_mask);

    priv->xwin = gdk_x11_window_get_xid(priv->win);

    gdk_window_set_user_data(priv->win, wid);
    gtk_widget_set_window(wid, priv->win);
    g_object_ref(wid);

    te_gtkgl_send_configure(wid);
}

void *te_gtkgl_get_x11_display(GtkWidget*wid)
{
    TeGtkgl *gtkgl = TE_GTKGL(wid);
    TeGtkgl_Priv *priv = TE_GTKGL_GET_PRIV(gtkgl);
    return priv->xdis;
}

static void
te_gtkgl_unrealize(GtkWidget *wid)
{
    TeGtkgl *gtkgl = TE_GTKGL(wid);
    TeGtkgl_Priv *priv = TE_GTKGL_GET_PRIV(gtkgl);

    if (priv->xdis) {
        glXDestroyContext(priv->xdis, priv->glc);
        // g_clear_object((volatile GObject**)&priv->win);

        priv->xdis = NULL;
    }

    GTK_WIDGET_CLASS(te_gtkgl_parent_class)->unrealize(wid);
}

/*
static void
te_gtkgl_finalize(GObject *obj)
{
    TeGtkgl *gtkgl = TE_GTKGL(obj);
    TeGtkgl_Priv *priv = TE_GTKGL_GET_PRIV(gtkgl);

    // glXDestroyContext(priv->xdis, priv->glc);
    // g_clear_object((volatile GObject**)&priv->win);

    G_OBJECT_CLASS(te_gtkgl_parent_class)->finalize(obj);
}
*/

static void
te_gtkgl_class_init(TeGtkglClass *klass)
{
    GtkWidgetClass *wklass;
    GObjectClass *oklass;

    g_type_class_add_private(klass, sizeof(TeGtkgl_Priv));
    wklass = GTK_WIDGET_CLASS(klass);

    wklass->realize = te_gtkgl_realize;
    wklass->unrealize = te_gtkgl_unrealize;
    wklass->size_allocate = te_gtkgl_size_allocate;
    wklass->draw = te_gtkgl_draw;

    /*
    oklass = (GObjectClass*)klass;
    oklass->finalize = te_gtkgl_finalize;
    */
}

static void
te_gtkgl_init(TeGtkgl *self)
{
    GtkWidget *wid = (GtkWidget*)self;
    TeGtkgl_Priv *priv = TE_GTKGL_GET_PRIV(wid);

    gtk_widget_set_can_focus(wid, TRUE);
    gtk_widget_set_receives_default(wid, TRUE);
    gtk_widget_set_has_window(wid, TRUE);
    gtk_widget_set_double_buffered(wid, FALSE);

    priv->xdis = NULL;
}

static void
te_gtkgl_send_configure(GtkWidget *wid)
{
    GtkAllocation allocation;
    GdkEvent *event = gdk_event_new(GDK_CONFIGURE);

    gtk_widget_get_allocation(wid, &allocation);

    event->configure.window = (GdkWindow *) g_object_ref(gtk_widget_get_window(wid));
    event->configure.send_event = TRUE;
    event->configure.x = allocation.x;
    event->configure.y = allocation.y;
    event->configure.width = allocation.width;
    event->configure.height = allocation.height;

    gtk_widget_event(wid, event);
    gdk_event_free(event);
}

static void
te_gtkgl_size_allocate(GtkWidget *wid, GtkAllocation *allocation)
{
    g_return_if_fail(TE_IS_GTKGL(wid));
    g_return_if_fail(allocation != NULL);

    gtk_widget_set_allocation(wid, allocation);

    if (gtk_widget_get_realized(wid)) {
        if (gtk_widget_get_has_window(wid)) {

            gdk_window_move_resize(gtk_widget_get_window(wid),
                    allocation->x, allocation->y,
                    allocation->width, allocation->height);
        }

        te_gtkgl_send_configure(wid);
    }
}

GtkWidget*
te_gtkgl_new()
{
    return (GtkWidget *) g_object_new(TE_TYPE_GTKGL, NULL);
}

void
te_gtkgl_make_current(TeGtkgl *wid)
{
    TeGtkgl_Priv *priv = TE_GTKGL_GET_PRIV(wid);
    glXMakeCurrent(priv->xdis, priv->xwin, priv->glc);
}

void
te_gtkgl_swap(TeGtkgl *wid)
{
    TeGtkgl_Priv *priv = TE_GTKGL_GET_PRIV(wid);
    glXSwapBuffers(priv->xdis, priv->xwin);
}

