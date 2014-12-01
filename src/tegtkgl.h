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

#ifndef __TE_GTKGL_H__
#define __TE_GTKGL_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TE_TYPE_GTKGL (te_gtkgl_get_type())
#define TE_GTKGL(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), TE_TYPE_GTKGL, TeGtkgl))
#define TE_IS_GTKGL(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), TE_TYPE_GTKGL))
#define TE_GTKGL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), TE_TYPE_GTKGL, TeGtkglClass))
#define TE_IS_GTKGL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), TE_TYPE_GTKGL))
#define TE_GTKGL_GET_CLASS(obj) (G_TYPE_INSTANCE((obj), TE_TYPE_GTKGL, TeGtkglClass))

typedef struct _TeGtkgl TeGtkgl;
typedef struct _TeGtkglClass TeGtkglClass;

struct _TeGtkgl {
    GtkWidget parent_instance;
};

struct _TeGtkglClass {
    GtkWidgetClass parent_class;
};

GType te_gtkgl_get_type(void);
GtkWidget *te_gtkgl_new();

void te_gtkgl_make_current(TeGtkgl*);
void *te_gtkgl_get_x11_display(GtkWidget*wid);
void te_gtkgl_swap(TeGtkgl*);

G_END_DECLS

#endif

