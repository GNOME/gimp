/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkWrapBox: Wrapping box widget
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

#ifndef __GTK_WRAP_BOX_H__
#define __GTK_WRAP_BOX_H__


#include <gtk/gtkcontainer.h>

G_BEGIN_DECLS


/* --- type macros --- */
#define GTK_TYPE_WRAP_BOX	     (gtk_wrap_box_get_type ())
#define GTK_WRAP_BOX(obj)	     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_WRAP_BOX, GtkWrapBox))
#define GTK_WRAP_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_WRAP_BOX, GtkWrapBoxClass))
#define GTK_IS_WRAP_BOX(obj)	     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_WRAP_BOX))
#define GTK_IS_WRAP_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_WRAP_BOX))
#define GTK_WRAP_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_WRAP_BOX, GtkWrapBoxClass))


/* --- typedefs --- */
typedef struct _GtkWrapBox      GtkWrapBox;
typedef struct _GtkWrapBoxClass GtkWrapBoxClass;
typedef struct _GtkWrapBoxChild GtkWrapBoxChild;

/* --- GtkWrapBox --- */
struct _GtkWrapBox
{
  GtkContainer     container;
  
  guint            homogeneous : 1;
  guint            justify : 4;
  guint            line_justify : 4;
  guint8           hspacing;
  guint8           vspacing;
  guint16          n_children;
  GtkWrapBoxChild *children;
  gfloat           aspect_ratio; /* 1/256..256 */
  guint            child_limit;
};
struct _GtkWrapBoxClass
{
  GtkContainerClass parent_class;

  GSList* (*rlist_line_children) (GtkWrapBox       *wbox,
				  GtkWrapBoxChild **child_p,
				  GtkAllocation    *area,
				  guint            *max_child_size,
				  gboolean         *expand_line);
};
struct _GtkWrapBoxChild
{
  GtkWidget *widget;
  guint      hexpand : 1;
  guint      hfill : 1;
  guint      vexpand : 1;
  guint      vfill : 1;
  guint      wrapped : 1;
  
  GtkWrapBoxChild *next;
};
#define GTK_JUSTIFY_TOP    GTK_JUSTIFY_LEFT
#define GTK_JUSTIFY_BOTTOM GTK_JUSTIFY_RIGHT


/* --- prototypes --- */
GType	   gtk_wrap_box_get_type            (void) G_GNUC_CONST;
void	   gtk_wrap_box_set_homogeneous     (GtkWrapBox      *wbox,
					     gboolean         homogeneous);
void	   gtk_wrap_box_set_hspacing        (GtkWrapBox      *wbox,
					     guint            hspacing);
void	   gtk_wrap_box_set_vspacing        (GtkWrapBox      *wbox,
					     guint            vspacing);
void	   gtk_wrap_box_set_justify         (GtkWrapBox      *wbox,
					     GtkJustification justify);
void	   gtk_wrap_box_set_line_justify    (GtkWrapBox      *wbox,
					     GtkJustification line_justify);
void	   gtk_wrap_box_set_aspect_ratio    (GtkWrapBox      *wbox,
					     gfloat           aspect_ratio);
void	   gtk_wrap_box_pack	            (GtkWrapBox      *wbox,
					     GtkWidget       *child,
					     gboolean         hexpand,
					     gboolean         hfill,
					     gboolean         vexpand,
					     gboolean         vfill);
void	   gtk_wrap_box_pack_wrapped        (GtkWrapBox      *wbox,
					     GtkWidget       *child,
					     gboolean         hexpand,
					     gboolean         hfill,
					     gboolean         vexpand,
					     gboolean         vfill,
					     gboolean         wrapped);
void       gtk_wrap_box_reorder_child       (GtkWrapBox      *wbox,
					     GtkWidget       *child,
					     gint             position);
void       gtk_wrap_box_query_child_packing (GtkWrapBox      *wbox,
					     GtkWidget       *child,
					     gboolean        *hexpand,
					     gboolean        *hfill,
					     gboolean        *vexpand,
					     gboolean        *vfill,
					     gboolean        *wrapped);
void       gtk_wrap_box_set_child_packing   (GtkWrapBox      *wbox,
					     GtkWidget       *child,
					     gboolean         hexpand,
					     gboolean         hfill,
					     gboolean         vexpand,
					     gboolean         vfill,
					     gboolean         wrapped);
guint*	   gtk_wrap_box_query_line_lengths  (GtkWrapBox	     *wbox,
					     guint           *n_lines);


G_END_DECLS

#endif /* __GTK_WRAP_BOX_H__ */
