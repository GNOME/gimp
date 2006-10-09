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


static void    gimp_vectors_tree_view_view_iface_init (GimpContainerViewInterface *iface);

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


G_DEFINE_TYPE_WITH_CODE (GimpVectorsTreeView, gimp_vectors_tree_view,
                         GIMP_TYPE_ITEM_TREE_VIEW,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONTAINER_VIEW,
                                                gimp_vectors_tree_view_view_iface_init))

#define parent_class gimp_vectors_tree_view_parent_class

static GimpContainerViewInterface *parent_view_iface = NULL;


static void
gimp_vectors_tree_view_class_init (GimpVectorsTreeViewClass *klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  GimpContainerTreeViewClass *view_class   = GIMP_CONTAINER_TREE_VIEW_CLASS (klass);
  GimpItemTreeViewClass      *iv_class     = GIMP_ITEM_TREE_VIEW_CLASS (klass);

  object_class->constructor = gimp_vectors_tree_view_constructor;

  view_class->drop_svg      = gimp_vectors_tree_view_drop_svg;

  iv_class->item_type       = GIMP_TYPE_VECTORS;
  iv_class->signal_name     = "active-vectors-changed";

  iv_class->get_container   = gimp_image_get_vectors;
  iv_class->get_active_item = (GimpGetItemFunc) gimp_image_get_active_vectors;
  iv_class->set_active_item = (GimpSetItemFunc) gimp_image_set_active_vectors;
  iv_class->reorder_item    = (GimpReorderItemFunc) gimp_image_position_vectors;
  iv_class->add_item        = (GimpAddItemFunc) gimp_image_add_vectors;
  iv_class->remove_item     = (GimpRemoveItemFunc) gimp_image_remove_vectors;
  iv_class->new_item        = gimp_vectors_tree_view_item_new;

  iv_class->action_group        = "vectors";
  iv_class->activate_action     = "vectors-path-tool";
  iv_class->edit_action         = "vectors-edit-attributes";
  iv_class->new_action          = "vectors-new";
  iv_class->new_default_action  = "vectors-new-last-values";
  iv_class->raise_action        = "vectors-raise";
  iv_class->raise_top_action    = "vectors-raise-to-top";
  iv_class->lower_action        = "vectors-lower";
  iv_class->lower_bottom_action = "vectors-lower-to-bottom";
  iv_class->duplicate_action    = "vectors-duplicate";
  iv_class->delete_action       = "vectors-delete";
  iv_class->reorder_desc        = _("Reorder path");
}

static void
gimp_vectors_tree_view_view_iface_init (GimpContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->set_container = gimp_vectors_tree_view_set_container;
}

static void
gimp_vectors_tree_view_init (GimpVectorsTreeView *view)
{
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
  GimpItemTreeView *view  = GIMP_ITEM_TREE_VIEW (tree_view);
  GimpImage        *image = view->image;
  gint              index = -1;
  GError           *error = NULL;

  if (image->gimp->be_verbose)
    g_print ("%s: SVG dropped (len = %d)\n", G_STRFUNC, (gint) svg_data_len);

  if (dest_viewable)
    {
      index = gimp_image_get_vectors_index (image,
                                            GIMP_VECTORS (dest_viewable));

      if (drop_pos == GTK_TREE_VIEW_DROP_AFTER)
        index++;
    }

  if (! gimp_vectors_import_buffer (image, svg_data, svg_data_len,
                                    TRUE, TRUE, index, &error))
    {
      gimp_message (image->gimp, G_OBJECT (tree_view), GIMP_MESSAGE_ERROR,
                    error->message);
      g_clear_error (&error);
    }
  else
    {
      gimp_image_flush (image);
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
  GimpItemTreeView *view  = GIMP_ITEM_TREE_VIEW (data);
  GimpImage        *image = view->image;
  GimpItem         *item;
  gchar            *svg_data = NULL;

  item = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (image);

  *svg_data_len = 0;

  if (item)
    {
      svg_data = gimp_vectors_export_string (image, GIMP_VECTORS (item));

      if (svg_data)
        *svg_data_len = strlen (svg_data);
    }

  return (guchar *) svg_data;
}
