/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainereditor.h
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

#ifndef __GIMP_CONTAINER_EDITOR_H__
#define __GIMP_CONTAINER_EDITOR_H__


#include <gtk/gtkvbox.h>


typedef void (* GimpContainerContextFunc) (GimpContainerEditor *editor);


typedef enum
{
  GIMP_VIEW_TYPE_GRID,
  GIMP_VIEW_TYPE_LIST
} GimpViewType;


#define GIMP_TYPE_CONTAINER_EDITOR            (gimp_container_editor_get_type ())
#define GIMP_CONTAINER_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER_EDITOR, GimpContainerEditor))
#define GIMP_CONTAINER_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_EDITOR, GimpContainerEditorClass))
#define GIMP_IS_CONTAINER_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER_EDITOR))
#define GIMP_IS_CONTAINER_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_EDITOR))
#define GIMP_CONTAINER_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER_EDITOR, GimpContainerEditorClass))


typedef struct _GimpContainerEditorClass  GimpContainerEditorClass;

struct _GimpContainerEditor
{
  GtkVBox                   parent_instance;

  GimpContainerContextFunc  context_func;

  GimpContainerView        *view;
  GtkWidget                *button_box;
};

struct _GimpContainerEditorClass
{
  GtkVBoxClass  parent_class;

  void (* select_item)   (GimpContainerEditor *editor,
			  GimpViewable        *object);
  void (* activate_item) (GimpContainerEditor *editor,
			  GimpViewable        *object);
  void (* context_item)  (GimpContainerEditor *editor,
			  GimpViewable        *object);
};


GType       gimp_container_editor_get_type   (void);


/*  protected  */

gboolean    gimp_container_editor_construct  (GimpContainerEditor  *editor,
					      GimpViewType          view_type,
					      GimpContainer        *container,
					      GimpContext          *context,
					      gint                  preview_size,
					      gint                  min_items_x,
					      gint                  min_items_y,
					      GimpContainerContextFunc  context_func);

GtkWidget * gimp_container_editor_add_button (GimpContainerEditor  *editor,
					      const gchar          *stock_id,
					      const gchar          *tooltip,
					      const gchar          *help_data,
					      GCallback             callback);
void        gimp_container_editor_enable_dnd (GimpContainerEditor  *editor,
					      GtkButton            *button);


#endif  /*  __GIMP_CONTAINER_EDITOR_H__  */
