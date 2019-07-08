/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpvectorstreeview.c
 * Copyright (C) 2001-2009 Michael Natterer <mitch@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "vectors/gimpvectors.h"
#include "vectors/gimpvectors-export.h"
#include "vectors/gimpvectors-import.h"

#include "gimpactiongroup.h"
#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimpuimanager.h"
#include "gimpvectorstreeview.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void    gimp_vectors_tree_view_view_iface_init (GimpContainerViewInterface *iface);

static void      gimp_vectors_tree_view_constructed   (GObject                  *object);

static void      gimp_vectors_tree_view_set_container (GimpContainerView        *view,
                                                       GimpContainer            *container);
static void      gimp_vectors_tree_view_drop_svg      (GimpContainerTreeView    *tree_view,
                                                       const gchar              *svg_data,
                                                       gsize                     svg_data_len,
                                                       GimpViewable             *dest_viewable,
                                                       GtkTreeViewDropPosition   drop_pos);
static GimpItem * gimp_vectors_tree_view_item_new     (GimpImage                *image);
static guchar   * gimp_vectors_tree_view_drag_svg     (GtkWidget                *widget,
                                                       gsize                    *svg_data_len,
                                                       gpointer                  data);


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

  object_class->constructed = gimp_vectors_tree_view_constructed;

  view_class->drop_svg      = gimp_vectors_tree_view_drop_svg;

  iv_class->item_type       = GIMP_TYPE_VECTORS;
  iv_class->signal_name     = "active-vectors-changed";

  iv_class->get_container   = gimp_image_get_vectors;
  iv_class->get_active_item = (GimpGetItemFunc) gimp_image_get_active_vectors;
  iv_class->set_active_item = (GimpSetItemFunc) gimp_image_set_active_vectors;
  iv_class->add_item        = (GimpAddItemFunc) gimp_image_add_vectors;
  iv_class->remove_item     = (GimpRemoveItemFunc) gimp_image_remove_vectors;
  iv_class->new_item        = gimp_vectors_tree_view_item_new;

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
  iv_class->lock_content_icon_name  = GIMP_ICON_TOOL_PATH;
  iv_class->lock_content_tooltip    = _("Lock path strokes");
  iv_class->lock_content_help_id    = GIMP_HELP_PATH_LOCK_STROKES;
  iv_class->lock_position_icon_name = GIMP_ICON_TOOL_MOVE;
  iv_class->lock_position_tooltip   = _("Lock path position");
  iv_class->lock_position_help_id   = GIMP_HELP_PATH_LOCK_POSITION;
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

static void
gimp_vectors_tree_view_constructed (GObject *object)
{
  GimpEditor            *editor    = GIMP_EDITOR (object);
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (object);
  GimpVectorsTreeView   *view      = GIMP_VECTORS_TREE_VIEW (object);
  GdkModifierType        extend_mask;
  GdkModifierType        modify_mask;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  extend_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_EXTEND_SELECTION);
  modify_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_MODIFY_SELECTION);

  view->toselection_button =
    gimp_editor_add_action_button (editor, "vectors",
                                   "vectors-selection-replace",
                                   "vectors-selection-add",
                                   extend_mask,
                                   "vectors-selection-subtract",
                                   modify_mask,
                                   "vectors-selection-intersect",
                                   extend_mask | modify_mask,
                                   NULL);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (editor),
                                  GTK_BUTTON (view->toselection_button),
                                  GIMP_TYPE_VECTORS);
  gtk_box_reorder_child (gimp_editor_get_button_box (editor),
                         view->toselection_button, 4);

  view->tovectors_button =
    gimp_editor_add_action_button (editor, "vectors",
                                   "vectors-selection-to-vectors",
                                   "vectors-selection-to-vectors-advanced",
                                   GDK_SHIFT_MASK,
                                   NULL);
  gtk_box_reorder_child (gimp_editor_get_button_box (editor),
                         view->tovectors_button, 5);

  view->stroke_button =
    gimp_editor_add_action_button (editor, "vectors",
                                   "vectors-stroke",
                                   "vectors-stroke-last-values",
                                   GDK_SHIFT_MASK,
                                   NULL);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (editor),
                                  GTK_BUTTON (view->stroke_button),
                                  GIMP_TYPE_VECTORS);
  gtk_box_reorder_child (gimp_editor_get_button_box (editor),
                         view->stroke_button, 6);

  gimp_dnd_svg_dest_add (GTK_WIDGET (tree_view->view), NULL, view);
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
  GimpItemTreeView *item_view = GIMP_ITEM_TREE_VIEW (tree_view);
  GimpImage        *image     = gimp_item_tree_view_get_image (item_view);
  GimpVectors      *parent;
  gint              index;
  GError           *error = NULL;

  if (image->gimp->be_verbose)
    g_print ("%s: SVG dropped (len = %d)\n", G_STRFUNC, (gint) svg_data_len);

  index = gimp_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (GimpViewable **) &parent);

  if (! gimp_vectors_import_buffer (image, svg_data, svg_data_len,
                                    TRUE, FALSE, parent, index, NULL, &error))
    {
      gimp_message_literal (image->gimp,
                            G_OBJECT (tree_view), GIMP_MESSAGE_ERROR,
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

  new_vectors = gimp_vectors_new (image, _("Path"));

  gimp_image_add_vectors (image, new_vectors,
                          GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

  return GIMP_ITEM (new_vectors);
}

static guchar *
gimp_vectors_tree_view_drag_svg (GtkWidget *widget,
                                 gsize     *svg_data_len,
                                 gpointer   data)
{
  GimpItemTreeView *view  = GIMP_ITEM_TREE_VIEW (data);
  GimpImage        *image = gimp_item_tree_view_get_image (view);
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
