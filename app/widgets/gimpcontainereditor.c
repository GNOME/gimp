/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainereditor.c
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

static void   gimp_container_editor_destroy    (GtkObject                *object);
static void   gimp_container_editor_style_set  (GtkWidget                *widget,
						GtkStyle                 *prev_style);

static void   gimp_container_editor_viewable_dropped (GtkWidget           *widget,
						      GimpViewable        *viewable,
						      gpointer             data);

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
      GtkTypeInfo view_info =
      {
	"GimpContainerEditor",
	sizeof (GimpContainerEditor),
	sizeof (GimpContainerEditorClass),
	(GtkClassInitFunc) gimp_container_editor_class_init,
	(GtkObjectInitFunc) gimp_container_editor_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      view_type = gtk_type_unique (GTK_TYPE_VBOX, &view_info);
    }

  return view_type;
}

static void
gimp_container_editor_class_init (GimpContainerEditorClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy   = gimp_container_editor_destroy;

  widget_class->style_set = gimp_container_editor_style_set;

  klass->select_item      = NULL;
  klass->activate_item    = NULL;
  klass->context_item     = gimp_container_editor_real_context_item;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content_spacing",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             G_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("button_spacing",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             G_PARAM_READABLE));
}

static void
gimp_container_editor_init (GimpContainerEditor *view)
{
  view->context_func = NULL;
  view->view         = NULL;

  gtk_box_set_spacing (GTK_BOX (view), 2);

  view->button_box = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_end (GTK_BOX (view), view->button_box, FALSE, FALSE, 0);
  gtk_widget_show (view->button_box);
}

static void
gimp_container_editor_destroy (GtkObject *object)
{
  GimpContainerEditor *editor;

  editor = GIMP_CONTAINER_EDITOR (object);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_container_editor_style_set (GtkWidget *widget,
				 GtkStyle  *prev_style)
{
  gint content_spacing;
  gint button_spacing;

  gtk_widget_style_get (widget,
                        "content_spacing",
                        &content_spacing,
			"button_spacing",
			&button_spacing,
			NULL);

  gtk_box_set_spacing (GTK_BOX (widget), content_spacing);
  gtk_box_set_spacing (GTK_BOX (GIMP_CONTAINER_EDITOR (widget)->button_box),
		       button_spacing);

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
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

  /*  select the active item  */
  gimp_container_editor_select_item
    (GTK_WIDGET (editor->view),
     (GimpViewable *)
     gimp_context_get_by_type (context, container->children_type),
     NULL,
     editor);

  return TRUE;
}

GtkWidget *
gimp_container_editor_add_button (GimpContainerEditor  *editor,
				  const gchar          *stock_id,
				  const gchar          *tooltip,
				  const gchar          *help_data,
				  GCallback             callback)
{
  GtkWidget *button;
  GtkWidget *image;

  g_return_val_if_fail (GIMP_IS_CONTAINER_EDITOR (editor), NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (editor->button_box), button,
		      TRUE, TRUE, 0);
  gtk_widget_show (button);

  if (tooltip || help_data)
    gimp_help_set_help_data (button, tooltip, help_data);

  if (callback)
    g_signal_connect (G_OBJECT (button), "clicked",
		      callback,
		      editor);

  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  return button;
}

void
gimp_container_editor_enable_dnd (GimpContainerEditor  *editor,
				  GtkButton            *button)
{
  g_return_if_fail (GIMP_IS_CONTAINER_EDITOR (editor));
  g_return_if_fail (GTK_IS_BUTTON (button));

  g_return_if_fail (editor->view != NULL);
  g_return_if_fail (editor->view->container != NULL);

  gimp_gtk_drag_dest_set_by_type (GTK_WIDGET (button),
				  GTK_DEST_DEFAULT_ALL,
				  editor->view->container->children_type,
				  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (button),
			      editor->view->container->children_type,
			      gimp_container_editor_viewable_dropped,
			      editor);
}


/*  private functions  */

static void
gimp_container_editor_viewable_dropped (GtkWidget    *widget,
					GimpViewable *viewable,
					gpointer      data)
{
  GimpContainerEditor *editor;

  editor = (GimpContainerEditor *) data;

  if (viewable && gimp_container_have (editor->view->container,
				       GIMP_OBJECT (viewable)))
    {
      gimp_context_set_by_type (editor->view->context,
				editor->view->container->children_type,
				GIMP_OBJECT (viewable));

      gtk_button_clicked (GTK_BUTTON (widget));
    }
}

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
