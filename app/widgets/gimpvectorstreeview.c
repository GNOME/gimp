/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpvectorstreeview.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "vectors/gimpvectors.h"
#include "vectors/gimpvectors-export.h"
#include "vectors/gimpvectors-import.h"

#include "gimpcontainerview.h"
#include "gimpvectorstreeview.h"
#include "gimpdnd.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void    gimp_vectors_tree_view_class_init (GimpVectorsTreeViewClass *klass);
static void    gimp_vectors_tree_view_init       (GimpVectorsTreeView      *view);

static void    gimp_vectors_tree_view_view_iface_init (GimpContainerViewInterface *view_iface);

static GObject * gimp_vectors_tree_view_constructor   (GType                  type,
                                                       guint                  n_params,
                                                       GObjectConstructParam *params);
static void      gimp_vectors_tree_view_set_container (GimpContainerView     *view,
                                                       GimpContainer         *container);
static void      gimp_vectors_tree_view_drop_svg      (GimpContainerTreeView *tree_view,
                                                       const gchar           *svg_data,
                                                       gsize                  svg_data_len,
                                                       GimpViewable          *dest_viewable,
                                                       GtkTreeViewDropPosition  drop_pos);
static GimpItem * gimp_vectors_tree_view_item_new     (GimpImage             *image);
static guchar  * gimp_vectors_tree_view_drag_svg      (GtkWidget             *widget,
                                                       gsize                 *svg_data_len,
                                                       gpointer               data);


static GimpItemTreeViewClass      *parent_class      = NULL;
static GimpContainerViewInterface *parent_view_iface = NULL;


GType
gimp_vectors_tree_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpVectorsTreeViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_vectors_tree_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpVectorsTreeView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_vectors_tree_view_init,
      };

      static const GInterfaceInfo view_iface_info =
      {
        (GInterfaceInitFunc) gimp_vectors_tree_view_view_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      view_type = g_type_register_static (GIMP_TYPE_ITEM_TREE_VIEW,
                                          "GimpVectorsTreeView",
                                          &view_info, 0);

      g_type_add_interface_static (view_type, GIMP_TYPE_CONTAINER_VIEW,
                                   &view_iface_info);
    }

  return view_type;
}

static void
gimp_vectors_tree_view_class_init (GimpVectorsTreeViewClass *klass)
{
  GObjectClass               *object_class;
  GimpContainerTreeViewClass *view_class;
  GimpItemTreeViewClass      *item_view_class;

  object_class    = G_OBJECT_CLASS (klass);
  view_class      = GIMP_CONTAINER_TREE_VIEW_CLASS (klass);
  item_view_class = GIMP_ITEM_TREE_VIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor        = gimp_vectors_tree_view_constructor;

  view_class->drop_svg             = gimp_vectors_tree_view_drop_svg;

  item_view_class->item_type       = GIMP_TYPE_VECTORS;
  item_view_class->signal_name     = "active-vectors-changed";

  item_view_class->get_container   = gimp_image_get_vectors;
  item_view_class->get_active_item = (GimpGetItemFunc) gimp_image_get_active_vectors;
  item_view_class->set_active_item = (GimpSetItemFunc) gimp_image_set_active_vectors;
  item_view_class->reorder_item    = (GimpReorderItemFunc) gimp_image_position_vectors;
  item_view_class->add_item        = (GimpAddItemFunc) gimp_image_add_vectors;
  item_view_class->remove_item     = (GimpRemoveItemFunc) gimp_image_remove_vectors;
  item_view_class->new_item        = gimp_vectors_tree_view_item_new;

  item_view_class->action_group        = "vectors";
  item_view_class->activate_action     = "vectors-path-tool";
  item_view_class->edit_action         = "vectors-edit-attributes";
  item_view_class->new_action          = "vectors-new";
  item_view_class->new_default_action  = "vectors-new-last-values";
  item_view_class->raise_action        = "vectors-raise";
  item_view_class->raise_top_action    = "vectors-raise-to-top";
  item_view_class->lower_action        = "vectors-lower";
  item_view_class->lower_bottom_action = "vectors-lower-to-bottom";
  item_view_class->duplicate_action    = "vectors-duplicate";
  item_view_class->delete_action       = "vectors-delete";
  item_view_class->reorder_desc        = _("Reorder path");
}

static void
gimp_vectors_tree_view_init (GimpVectorsTreeView *view)
{
}

static void
gimp_vectors_tree_view_view_iface_init (GimpContainerViewInterface *view_iface)
{
  parent_view_iface = g_type_interface_peek_parent (view_iface);

  view_iface->set_container = gimp_vectors_tree_view_set_container;
}

static GObject *
gimp_vectors_tree_view_constructor (GType                  type,
                                    guint                  n_params,
                                    GObjectConstructParam *params)
{
  GObject               *object;
  GimpEditor            *editor;
  GimpContainerTreeView *tree_view;
  GimpVectorsTreeView   *view;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor    = GIMP_EDITOR (object);
  tree_view = GIMP_CONTAINER_TREE_VIEW (object);
  view      = GIMP_VECTORS_TREE_VIEW (object);

  /*  hide basically useless edit button  */
  gtk_widget_hide (GIMP_ITEM_TREE_VIEW (view)->edit_button);

  view->toselection_button =
    gimp_editor_add_action_button (editor, "vectors",
                                   "vectors-selection-replace",
                                   "vectors-selection-add",
                                   GDK_SHIFT_MASK,
                                   "vectors-selection-subtract",
                                   GDK_CONTROL_MASK,
                                   "vectors-selection-intersect",
                                   GDK_SHIFT_MASK | GDK_CONTROL_MASK,
                                   NULL);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (editor),
				  GTK_BUTTON (view->toselection_button),
				  GIMP_TYPE_VECTORS);
  gtk_box_reorder_child (GTK_BOX (editor->button_box),
			 view->toselection_button, 5);

  view->tovectors_button =
    gimp_editor_add_action_button (editor, "vectors",
                                   "vectors-selection-to-vectors",
                                   "vectors-selection-to-vectors-advanced",
                                   GDK_SHIFT_MASK,
                                   NULL);
  gtk_box_reorder_child (GTK_BOX (editor->button_box),
			 view->tovectors_button, 6);

  view->stroke_button =
    gimp_editor_add_action_button (editor, "vectors",
                                   "vectors-stroke",
                                   "vectors-stroke-last-values",
                                   GDK_SHIFT_MASK,
                                   NULL);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (editor),
				  GTK_BUTTON (view->stroke_button),
				  GIMP_TYPE_VECTORS);
  gtk_box_reorder_child (GTK_BOX (editor->button_box),
			 view->stroke_button, 7);

  gimp_dnd_svg_dest_add (GTK_WIDGET (tree_view->view), NULL, view);

  return object;
}

static void
gimp_vectors_tree_view_set_container (GimpContainerView *view,
                                      GimpContainer     *container)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GimpContainer         *old_container;

  old_container = gimp_container_view_get_container (GIMP_CONTAINER_VIEW (view));

  if (old_container && ! container)
    {
      gimp_dnd_svg_source_remove (GTK_WIDGET (tree_view->view));
    }

  parent_view_iface->set_container (view, container);

  if (! old_container && container)
    {
      gimp_dnd_svg_source_add (GTK_WIDGET (tree_view->view),
                               gimp_vectors_tree_view_drag_svg,
                               tree_view);
    }
}

static void
gimp_vectors_tree_view_drop_svg (GimpContainerTreeView   *tree_view,
                                 const gchar             *svg_data,
                                 gsize                    svg_data_len,
                                 GimpViewable            *dest_viewable,
                                 GtkTreeViewDropPosition  drop_pos)
{
  GimpItemTreeView *view   = GIMP_ITEM_TREE_VIEW (tree_view);
  GimpImage        *gimage = view->gimage;
  GError           *error  = NULL;
  gint              index;

  if (gimage->gimp->be_verbose)
    g_print ("%s: SVG dropped (len = %d)\n", G_STRFUNC, (gint) svg_data_len);

  index = gimp_image_get_vectors_index (gimage, GIMP_VECTORS (dest_viewable));

  if (drop_pos == GTK_TREE_VIEW_DROP_AFTER)
    index++;

  if (! gimp_vectors_import_buffer (gimage, svg_data, svg_data_len,
                                    TRUE, TRUE, index, &error))
    {
      g_message (error->message);
      g_clear_error (&error);
    }
  else
    {
      gimp_image_flush (gimage);
    }
}

static GimpItem *
gimp_vectors_tree_view_item_new (GimpImage *image)
{
  GimpVectors *new_vectors;

  new_vectors = gimp_vectors_new (image, _("Empty Path"));

  gimp_image_add_vectors (image, new_vectors, -1);

  return GIMP_ITEM (new_vectors);
}

static guchar *
gimp_vectors_tree_view_drag_svg (GtkWidget *widget,
                                 gsize     *svg_data_len,
                                 gpointer   data)
{
  GimpItemTreeView *view   = GIMP_ITEM_TREE_VIEW (data);
  GimpImage        *gimage = view->gimage;
  GimpItem         *item;
  gchar            *svg_data = NULL;

  item = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (gimage);

  *svg_data_len = 0;

  if (item)
    {
      svg_data = gimp_vectors_export_string (gimage, GIMP_VECTORS (item));

      if (svg_data)
        *svg_data_len = strlen (svg_data) + 1;
    }

  return (guchar *) svg_data;
}
