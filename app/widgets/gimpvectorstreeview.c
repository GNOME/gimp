/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmavectorstreeview.c
 * Copyright (C) 2001-2009 Michael Natterer <mitch@ligma.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"

#include "vectors/ligmavectors.h"
#include "vectors/ligmavectors-export.h"
#include "vectors/ligmavectors-import.h"

#include "ligmaactiongroup.h"
#include "ligmacontainerview.h"
#include "ligmadnd.h"
#include "ligmahelp-ids.h"
#include "ligmauimanager.h"
#include "ligmavectorstreeview.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


static void    ligma_vectors_tree_view_view_iface_init (LigmaContainerViewInterface *iface);

static void      ligma_vectors_tree_view_constructed   (GObject                  *object);

static void      ligma_vectors_tree_view_set_container (LigmaContainerView        *view,
                                                       LigmaContainer            *container);
static void      ligma_vectors_tree_view_drop_svg      (LigmaContainerTreeView    *tree_view,
                                                       const gchar              *svg_data,
                                                       gsize                     svg_data_len,
                                                       LigmaViewable             *dest_viewable,
                                                       GtkTreeViewDropPosition   drop_pos);
static LigmaItem * ligma_vectors_tree_view_item_new     (LigmaImage                *image);
static guchar   * ligma_vectors_tree_view_drag_svg     (GtkWidget                *widget,
                                                       gsize                    *svg_data_len,
                                                       gpointer                  data);


G_DEFINE_TYPE_WITH_CODE (LigmaVectorsTreeView, ligma_vectors_tree_view,
                         LIGMA_TYPE_ITEM_TREE_VIEW,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONTAINER_VIEW,
                                                ligma_vectors_tree_view_view_iface_init))

#define parent_class ligma_vectors_tree_view_parent_class

static LigmaContainerViewInterface *parent_view_iface = NULL;


static void
ligma_vectors_tree_view_class_init (LigmaVectorsTreeViewClass *klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  LigmaContainerTreeViewClass *view_class   = LIGMA_CONTAINER_TREE_VIEW_CLASS (klass);
  LigmaItemTreeViewClass      *iv_class     = LIGMA_ITEM_TREE_VIEW_CLASS (klass);

  object_class->constructed = ligma_vectors_tree_view_constructed;

  view_class->drop_svg      = ligma_vectors_tree_view_drop_svg;

  iv_class->item_type       = LIGMA_TYPE_VECTORS;
  iv_class->signal_name     = "selected-vectors-changed";

  iv_class->get_container   = ligma_image_get_vectors;
  iv_class->get_selected_items = (LigmaGetItemsFunc) ligma_image_get_selected_vectors;
  iv_class->set_selected_items = (LigmaSetItemsFunc) ligma_image_set_selected_vectors;
  iv_class->add_item        = (LigmaAddItemFunc) ligma_image_add_vectors;
  iv_class->remove_item     = (LigmaRemoveItemFunc) ligma_image_remove_vectors;
  iv_class->new_item        = ligma_vectors_tree_view_item_new;

  iv_class->action_group            = "vectors";
  iv_class->activate_action         = "vectors-edit";
  iv_class->new_action              = "vectors-new";
  iv_class->new_default_action      = "vectors-new-last-values";
  iv_class->raise_action            = "vectors-raise";
  iv_class->raise_top_action        = "vectors-raise-to-top";
  iv_class->lower_action            = "vectors-lower";
  iv_class->lower_bottom_action     = "vectors-lower-to-bottom";
  iv_class->duplicate_action        = "vectors-duplicate";
  iv_class->delete_action           = "vectors-delete";
  iv_class->lock_content_icon_name  = LIGMA_ICON_TOOL_PATH;
  iv_class->lock_content_tooltip    = _("Lock path strokes");
  iv_class->lock_content_help_id    = LIGMA_HELP_PATH_LOCK_STROKES;
  iv_class->lock_position_icon_name = LIGMA_ICON_TOOL_MOVE;
  iv_class->lock_position_tooltip   = _("Lock path position");
  iv_class->lock_position_help_id   = LIGMA_HELP_PATH_LOCK_POSITION;
}

static void
ligma_vectors_tree_view_view_iface_init (LigmaContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->set_container = ligma_vectors_tree_view_set_container;
}

static void
ligma_vectors_tree_view_init (LigmaVectorsTreeView *view)
{
}

static void
ligma_vectors_tree_view_constructed (GObject *object)
{
  LigmaEditor            *editor    = LIGMA_EDITOR (object);
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (object);
  LigmaVectorsTreeView   *view      = LIGMA_VECTORS_TREE_VIEW (object);
  GdkModifierType        extend_mask;
  GdkModifierType        modify_mask;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  extend_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_EXTEND_SELECTION);
  modify_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_MODIFY_SELECTION);

  view->toselection_button =
    ligma_editor_add_action_button (editor, "vectors",
                                   "vectors-selection-replace",
                                   "vectors-selection-add",
                                   extend_mask,
                                   "vectors-selection-subtract",
                                   modify_mask,
                                   "vectors-selection-intersect",
                                   extend_mask | modify_mask,
                                   NULL);
  ligma_container_view_enable_dnd (LIGMA_CONTAINER_VIEW (editor),
                                  GTK_BUTTON (view->toselection_button),
                                  LIGMA_TYPE_VECTORS);
  gtk_box_reorder_child (ligma_editor_get_button_box (editor),
                         view->toselection_button, 4);

  view->tovectors_button =
    ligma_editor_add_action_button (editor, "vectors",
                                   "vectors-selection-to-vectors",
                                   "vectors-selection-to-vectors-advanced",
                                   GDK_SHIFT_MASK,
                                   NULL);
  gtk_box_reorder_child (ligma_editor_get_button_box (editor),
                         view->tovectors_button, 5);

  view->stroke_button =
    ligma_editor_add_action_button (editor, "vectors",
                                   "vectors-stroke",
                                   "vectors-stroke-last-values",
                                   GDK_SHIFT_MASK,
                                   NULL);
  ligma_container_view_enable_dnd (LIGMA_CONTAINER_VIEW (editor),
                                  GTK_BUTTON (view->stroke_button),
                                  LIGMA_TYPE_VECTORS);
  gtk_box_reorder_child (ligma_editor_get_button_box (editor),
                         view->stroke_button, 6);

  ligma_dnd_svg_dest_add (GTK_WIDGET (tree_view->view), NULL, view);
}

static void
ligma_vectors_tree_view_set_container (LigmaContainerView *view,
                                      LigmaContainer     *container)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (view);
  LigmaContainer         *old_container;

  old_container = ligma_container_view_get_container (LIGMA_CONTAINER_VIEW (view));

  if (old_container && ! container)
    {
      ligma_dnd_svg_source_remove (GTK_WIDGET (tree_view->view));
    }

  parent_view_iface->set_container (view, container);

  if (! old_container && container)
    {
      ligma_dnd_svg_source_add (GTK_WIDGET (tree_view->view),
                               ligma_vectors_tree_view_drag_svg,
                               tree_view);
    }
}

static void
ligma_vectors_tree_view_drop_svg (LigmaContainerTreeView   *tree_view,
                                 const gchar             *svg_data,
                                 gsize                    svg_data_len,
                                 LigmaViewable            *dest_viewable,
                                 GtkTreeViewDropPosition  drop_pos)
{
  LigmaItemTreeView *item_view = LIGMA_ITEM_TREE_VIEW (tree_view);
  LigmaImage        *image     = ligma_item_tree_view_get_image (item_view);
  LigmaVectors      *parent;
  gint              index;
  GError           *error = NULL;

  if (image->ligma->be_verbose)
    g_print ("%s: SVG dropped (len = %d)\n", G_STRFUNC, (gint) svg_data_len);

  index = ligma_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (LigmaViewable **) &parent);

  if (! ligma_vectors_import_buffer (image, svg_data, svg_data_len,
                                    TRUE, FALSE, parent, index, NULL, &error))
    {
      ligma_message_literal (image->ligma,
                            G_OBJECT (tree_view), LIGMA_MESSAGE_ERROR,
                            error->message);
      g_clear_error (&error);
    }
  else
    {
      ligma_image_flush (image);
    }
}

static LigmaItem *
ligma_vectors_tree_view_item_new (LigmaImage *image)
{
  LigmaVectors *new_vectors;

  new_vectors = ligma_vectors_new (image, _("Path"));

  ligma_image_add_vectors (image, new_vectors,
                          LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);

  return LIGMA_ITEM (new_vectors);
}

static guchar *
ligma_vectors_tree_view_drag_svg (GtkWidget *widget,
                                 gsize     *svg_data_len,
                                 gpointer   data)
{
  LigmaItemTreeView *view  = LIGMA_ITEM_TREE_VIEW (data);
  LigmaImage        *image = ligma_item_tree_view_get_image (view);
  GList            *items;
  gchar            *svg_data = NULL;

  items = LIGMA_ITEM_TREE_VIEW_GET_CLASS (view)->get_selected_items (image);

  *svg_data_len = 0;

  if (items)
    {
      svg_data = ligma_vectors_export_string (image, items);

      if (svg_data)
        *svg_data_len = strlen (svg_data);
    }

  return (guchar *) svg_data;
}
