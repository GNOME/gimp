/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainereditor.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimpcontainer.h"

#include "gimpcontainereditor.h"
#include "gimpcontainergridview.h"
#include "gimpcontainerlistview.h"
#include "gimpdnd.h"


static void   gimp_container_editor_class_init (GimpContainerEditorClass *klass);
static void   gimp_container_editor_init       (GimpContainerEditor      *view);

static void   gimp_container_editor_select_item      (GtkWidget           *widget,
						      GimpViewable        *viewable,
						      gpointer             insert_data,
						      GimpContainerEditor *editor);
static void   gimp_container_editor_activate_item    (GtkWidget           *widget,
						      GimpViewable        *viewable,
						      gpointer             insert_data,
						      GimpContainerEditor *editor);
static void   gimp_container_editor_context_item     (GtkWidget           *widget,
						      GimpViewable        *viewable,
						      gpointer             insert_data,
						      GimpContainerEditor *editor);
static void   gimp_container_editor_real_context_item(GimpContainerEditor *editor,
						      GimpViewable        *viewable);


static GtkVBoxClass *parent_class = NULL;


GType
gimp_container_editor_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpContainerEditorClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_container_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpContainerEditor),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_container_editor_init,
      };

      view_type = g_type_register_static (GTK_TYPE_VBOX,
                                          "GimpContainerEditor",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_container_editor_class_init (GimpContainerEditorClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);

  klass->select_item    = NULL;
  klass->activate_item  = NULL;
  klass->context_item   = gimp_container_editor_real_context_item;
}

static void
gimp_container_editor_init (GimpContainerEditor *view)
{
  view->context_func = NULL;
  view->view         = NULL;
}

gboolean
gimp_container_editor_construct (GimpContainerEditor      *editor,
				 GimpViewType              view_type,
				 GimpContainer            *container,
				 GimpContext              *context,
				 gint                      preview_size,
				 gint                      min_items_x,
				 gint                      min_items_y,
				 GimpContainerContextFunc  context_func)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER_EDITOR (editor), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);

  g_return_val_if_fail (preview_size > 0 && preview_size <= 64, FALSE);
  g_return_val_if_fail (min_items_x > 0 && min_items_x <= 64, FALSE);
  g_return_val_if_fail (min_items_y > 0 && min_items_y <= 64, FALSE);

  editor->context_func = context_func;

  switch (view_type)
    {
    case GIMP_VIEW_TYPE_GRID:
      editor->view =
	GIMP_CONTAINER_VIEW (gimp_container_grid_view_new (container,
							   context,
							   preview_size,
							   min_items_x,
							   min_items_y));
      break;

    case GIMP_VIEW_TYPE_LIST:
      editor->view =
	GIMP_CONTAINER_VIEW (gimp_container_list_view_new (container,
							   context,
							   preview_size,
							   min_items_x,
							   min_items_y));
      break;

    default:
      g_warning ("%s(): unknown GimpViewType passed", G_GNUC_FUNCTION);
      return FALSE;
    }

  gtk_container_add (GTK_CONTAINER (editor),
		     GTK_WIDGET (editor->view));
  gtk_widget_show (GTK_WIDGET (editor->view));

  g_signal_connect_object (G_OBJECT (editor->view), "select_item",
			   G_CALLBACK (gimp_container_editor_select_item),
			   G_OBJECT (editor),
			   0);

  g_signal_connect_object (G_OBJECT (editor->view), "activate_item",
			   G_CALLBACK (gimp_container_editor_activate_item),
			   G_OBJECT (editor),
			   0);

  g_signal_connect_object (G_OBJECT (editor->view), "context_item",
			   G_CALLBACK (gimp_container_editor_context_item),
			   G_OBJECT (editor),
			   0);

  return TRUE;
}


/*  private functions  */

static void
gimp_container_editor_select_item (GtkWidget           *widget,
				   GimpViewable        *viewable,
				   gpointer             insert_data,
				   GimpContainerEditor *editor)
{
  GimpContainerEditorClass *klass;

  klass = GIMP_CONTAINER_EDITOR_GET_CLASS (editor);

  if (klass->select_item)
    klass->select_item (editor, viewable);
}

static void
gimp_container_editor_activate_item (GtkWidget           *widget,
				     GimpViewable        *viewable,
				     gpointer             insert_data,
				     GimpContainerEditor *editor)
{
  GimpContainerEditorClass *klass;

  klass = GIMP_CONTAINER_EDITOR_GET_CLASS (editor);

  if (klass->activate_item)
    klass->activate_item (editor, viewable);
}

static void
gimp_container_editor_context_item (GtkWidget           *widget,
				    GimpViewable        *viewable,
				    gpointer             insert_data,
				    GimpContainerEditor *editor)
{
  GimpContainerEditorClass *klass;

  klass = GIMP_CONTAINER_EDITOR_GET_CLASS (editor);

  if (klass->context_item)
    klass->context_item (editor, viewable);
}

static void
gimp_container_editor_real_context_item (GimpContainerEditor *editor,
					 GimpViewable        *viewable)
{
  if (viewable && gimp_container_have (editor->view->container,
				       GIMP_OBJECT (viewable)))
    {
      if (editor->context_func)
	editor->context_func (editor);
    }
}
