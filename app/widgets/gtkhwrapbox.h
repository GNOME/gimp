/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkHWrapBox: Horizontal wrapping box widget
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
#ifndef __GTK_HWRAP_BOX_H__
#define __GTK_HWRAP_BOX_H__


#include "gtkwrapbox.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* --- type macros --- */
#define GTK_TYPE_HWRAP_BOX	      (gtk_hwrap_box_get_type ())
#define GTK_HWRAP_BOX(obj)	      (GTK_CHECK_CAST ((obj), GTK_TYPE_HWRAP_BOX, GtkHWrapBox))
#define GTK_HWRAP_BOX_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_HWRAP_BOX, GtkHWrapBoxClass))
#define GTK_IS_HWRAP_BOX(obj)	      (GTK_CHECK_TYPE ((obj), GTK_TYPE_HWRAP_BOX))
#define GTK_IS_HWRAP_BOX_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_HWRAP_BOX))
#define GTK_HWRAP_BOX_GET_CLASS(obj)  (GTK_HWRAP_BOX_CLASS (((GtkObject*) (obj))->klass))


/* --- typedefs --- */
typedef struct _GtkHWrapBox      GtkHWrapBox;
typedef struct _GtkHWrapBoxClass GtkHWrapBoxClass;


/* --- GtkHWrapBox --- */
struct _GtkHWrapBox
{
  GtkWrapBox parent_widget;
  
  guint16    max_child_width;
  guint16    max_child_height;
};
struct _GtkHWrapBoxClass
{
  GtkWrapBoxClass parent_class;
};


/* --- prototypes --- */
GtkType	   gtk_hwrap_box_get_type           (void);



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_HWRAP_BOX_H__ */




