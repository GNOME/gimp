/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawablelistview.h
 * Copyright (C) 2001 Michael Natterer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_DRAWABLE_LIST_VIEW_H__
#define __GIMP_DRAWABLE_LIST_VIEW_H__


#include "gimpcontainerlistview.h"


typedef GimpContainer * (* GimpGetContainerFunc)    (const GimpImage *gimage);
typedef GimpDrawable  * (* GimpGetDrawableFunc)     (const GimpImage *gimage);
typedef void            (* GimpSetDrawableFunc)     (GimpImage       *gimage,
						     GimpDrawable    *drawable);
typedef void            (* GimpReorderDrawableFunc) (GimpImage       *gimage,
						     GimpDrawable    *drawable,
						     gint             new_index,
						     gboolean         push_undo);
typedef void            (* GimpAddDrawableFunc)     (GimpImage       *gimage,
						     GimpDrawable    *drawable,
						     gint             index);
typedef void            (* GimpRemoveDrawableFunc)  (GimpImage       *gimage,
						     GimpDrawable    *drawable);
typedef GimpDrawable  * (* GimpCopyDrawableFunc)    (GimpDrawable    *drawable,
						     gboolean         add_alpha);

typedef void            (* GimpNewDrawableFunc)     (GimpImage       *gimage);
typedef void            (* GimpEditDrawableFunc)    (GimpDrawable    *drawable);
typedef void            (* GimpDrawableContextFunc) (GimpImage       *gimage);


#define GIMP_TYPE_DRAWABLE_LIST_VIEW            (gimp_drawable_list_view_get_type ())
#define GIMP_DRAWABLE_LIST_VIEW(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_DRAWABLE_LIST_VIEW, GimpDrawableListView))
#define GIMP_DRAWABLE_LIST_VIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DRAWABLE_LIST_VIEW, GimpDrawableListViewClass))
#define GIMP_IS_DRAWABLE_LIST_VIEW(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_DRAWABLE_LIST_VIEW))
#define GIMP_IS_DRAWABLE_LIST_VIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DRAWABLE_LIST_VIEW))


typedef struct _GimpDrawableListViewClass  GimpDrawableListViewClass;

struct _GimpDrawableListView
{
  GimpContainerListView    parent_instance;

  GimpImage               *gimage;

  GtkType                  drawable_type;
  gchar                   *signal_name;

  GimpGetContainerFunc     get_container_func;
  GimpGetDrawableFunc      get_drawable_func;
  GimpSetDrawableFunc      set_drawable_func;
  GimpReorderDrawableFunc  reorder_drawable_func;
  GimpAddDrawableFunc      add_drawable_func;
  GimpRemoveDrawableFunc   remove_drawable_func;
  GimpCopyDrawableFunc     copy_drawable_func;

  GimpNewDrawableFunc      new_drawable_func;
  GimpEditDrawableFunc     edit_drawable_func;
  GimpDrawableContextFunc  drawable_context_func;

  GtkWidget               *button_box;

  GtkWidget               *new_button;
  GtkWidget               *raise_button;
  GtkWidget               *lower_button;
  GtkWidget               *duplicate_button;
  GtkWidget               *edit_button;
  GtkWidget               *delete_button;
};

struct _GimpDrawableListViewClass
{
  GimpContainerListViewClass  parent_class;

  void (* set_image) (GimpDrawableListView *view,
		      GimpImage            *gimage);
};


GtkType     gimp_drawable_list_view_get_type (void);

GtkWidget * gimp_drawable_list_view_new      (GimpImage               *gimage,
					      GtkType                  drawable_type,
					      const gchar             *signal_name,
					      GimpGetContainerFunc     get_container_func,
					      GimpGetDrawableFunc      get_drawable_func,
					      GimpSetDrawableFunc      set_drawable_func,
					      GimpReorderDrawableFunc  reorder_drawable_func,
					      GimpAddDrawableFunc      add_drawable_func,
					      GimpRemoveDrawableFunc   remove_drawable_func,
					      GimpCopyDrawableFunc     copy_drawable_func,
					      GimpNewDrawableFunc      new_drawable_func,
					      GimpEditDrawableFunc     edit_drawable_func,
					      GimpDrawableContextFunc  drawable_context_func);

void       gimp_drawable_list_view_set_image (GimpDrawableListView *view,
					      GimpImage            *gimage);


#endif  /*  __GIMP_DRAWABLE_LIST_VIEW_H__  */
