/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkVWrapBox: Vertical wrapping box widget
 * Copyright (C) 1999 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GTK_VWRAP_BOX_H__
#define __GTK_VWRAP_BOX_H__


#include "gtkwrapbox.h"

G_BEGIN_DECLS


/* --- type macros --- */
#define GTK_TYPE_VWRAP_BOX	      (gtk_vwrap_box_get_type ())
#define GTK_VWRAP_BOX(obj)	      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_VWRAP_BOX, GtkVWrapBox))
#define GTK_VWRAP_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_VWRAP_BOX, GtkVWrapBoxClass))
#define GTK_IS_VWRAP_BOX(obj)	      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_VWRAP_BOX))
#define GTK_IS_VWRAP_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_VWRAP_BOX))
#define GTK_VWRAP_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_VWRAP_BOX, GtkVWrapBoxClass))


/* --- typedefs --- */
typedef struct _GtkVWrapBox      GtkVWrapBox;
typedef struct _GtkVWrapBoxClass GtkVWrapBoxClass;


/* --- GtkVWrapBox --- */
struct _GtkVWrapBox
{
  GtkWrapBox parent_widget;
  
  /*<h2v-off>*/
  guint      max_child_width;
  guint      max_child_height;
  /*<h2v-on>*/
};

struct _GtkVWrapBoxClass
{
  GtkWrapBoxClass parent_class;
};


/* --- prototypes --- */
GType	    gtk_vwrap_box_get_type  (void) G_GNUC_CONST;
GtkWidget * gtk_vwrap_box_new       (gboolean homogeneous);


G_END_DECLS

#endif /* __GTK_VWRAP_BOX_H__ */
