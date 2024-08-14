/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppathstreeview.c
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

#include "vectors/gimppath.h"
#include "vectors/gimppath-export.h"
#include "vectors/gimppath-import.h"

#include "gimpactiongroup.h"
#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimpuimanager.h"
#include "gimppathtreeview.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void    gimp_path_tree_view_view_iface_init (GimpContainerViewInterface  *iface);

static void      gimp_path_tree_view_constructed   (GObject                     *object);

static void      gimp_path_tree_view_set_container (GimpContainerView           *view,
                                                    GimpContainer               *container);
static void      gimp_path_tree_view_drop_svg      (GimpContainerTreeView       *tree_view,
                                                    const gchar                 *svg_data,
                                                    gsize                        svg_data_len,
                                                    GimpViewable                *dest_viewable,
                                                    GtkTreeViewDropPosition      drop_pos);
static GimpItem * gimp_path_tree_view_item_new     (GimpImage                   *image);
static guchar   * gimp_path_tree_view_drag_svg     (GtkWidget                   *widget,
                                                    gsize                       *svg_data_len,
                                                    gpointer                     data);


G_DEFINE_TYPE_WITH_CODE (GimpPathTreeView, gimp_path_tree_view,
                         GIMP_TYPE_ITEM_TREE_VIEW,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONTAINER_VIEW,
                                                gimp_path_tree_view_view_iface_init))

#define parent_class gimp_path_tree_view_parent_class

static GimpContainerViewInterface *parent_view_iface = NULL;


static void
gimp_path_tree_view_class_init (GimpPathTreeViewClass *klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  GimpContainerTreeViewClass *view_class   = GIMP_CONTAINER_TREE_VIEW_CLASS (klass);
  GimpItemTreeViewClass      *iv_class     = GIMP_ITEM_TREE_VIEW_CLASS (klass);

  object_class->constructed = gimp_path_tree_view_constructed;

  view_class->drop_svg      = gimp_path_tree_view_drop_svg;

  iv_class->move_cursor_up_action    = "paths-select-previous";
  iv_class->move_cursor_down_action  = "paths-select-next";
  iv_class->move_cursor_start_action = "paths-select-top";
  iv_class->move_cursor_end_action   = "paths-select-bottom";

  iv_class->item_type       = GIMP_TYPE_PATH;
  iv_class->signal_name     = "selected-paths-changed";

  iv_class->get_container   = gimp_image_get_paths;
  iv_class->get_selected_items = (GimpGetItemsFunc) gimp_image_get_selected_paths;
  iv_class->set_selected_items = (GimpSetItemsFunc) gimp_image_set_selected_paths;
  iv_class->add_item        = (GimpAddItemFunc) gimp_image_add_path;
  iv_class->remove_item     = (GimpRemoveItemFunc) gimp_image_remove_path;
  iv_class->new_item        = gimp_path_tree_view_item_new;

  iv_class->action_group              = "paths";
  iv_class->activate_action           = "paths-edit";
  iv_class->new_action                = "paths-new";
  iv_class->new_default_action        = "paths-new-last-values";
  iv_class->raise_action              = "paths-raise";
  iv_class->raise_top_action          = "paths-raise-to-top";
  iv_class->lower_action              = "paths-lower";
  iv_class->lower_bottom_action       = "paths-lower-to-bottom";
  iv_class->duplicate_action          = "paths-duplicate";
  iv_class->delete_action             = "paths-delete";
  iv_class->lock_content_icon_name    = GIMP_ICON_LOCK_PATH;
  iv_class->lock_content_tooltip      = _("Lock path");
  iv_class->lock_content_help_id      = GIMP_HELP_PATH_LOCK_STROKES;
  iv_class->lock_position_icon_name   = GIMP_ICON_LOCK_POSITION;
  iv_class->lock_position_tooltip     = _("Lock path position");
  iv_class->lock_position_help_id     = GIMP_HELP_PATH_LOCK_POSITION;
  iv_class->lock_visibility_icon_name = GIMP_ICON_LOCK_VISIBILITY;
  iv_class->lock_visibility_tooltip   = _("Lock path visibility");
  iv_class->lock_position_help_id     = GIMP_HELP_PATH_LOCK_VISIBILITY;
}

static void
gimp_path_tree_view_view_iface_init (GimpContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->set_container = gimp_path_tree_view_set_container;
}

static void
gimp_path_tree_view_init (GimpPathTreeView *view)
{
}

static void
gimp_path_tree_view_constructed (GObject *object)
{
  GimpEditor            *editor    = GIMP_EDITOR (object);
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (object);
  GimpPathTreeView      *view      = GIMP_PATH_TREE_VIEW (object);
  GdkModifierType        extend_mask;
  GdkModifierType        modify_mask;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  extend_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_EXTEND_SELECTION);
  modify_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_MODIFY_SELECTION);

  view->toselection_button =
    gimp_editor_add_action_button (editor, "paths",
                                   "paths-selection-replace",
                                   "paths-selection-add",
                                   extend_mask,
                                   "paths-selection-subtract",
                                   modify_mask,
                                   "paths-selection-intersect",
                                   extend_mask | modify_mask,
                                   NULL);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (editor),
                                  GTK_BUTTON (view->toselection_button),
                                  GIMP_TYPE_PATH);
  gtk_box_reorder_child (gimp_editor_get_button_box (editor),
                         view->toselection_button, 4);

  view->tovectors_button =
    gimp_editor_add_action_button (editor, "paths",
                                   "paths-selection-to-path",
                                   "paths-selection-to-path-advanced",
                                   GDK_SHIFT_MASK,
                                   NULL);
  gtk_box_reorder_child (gimp_editor_get_button_box (editor),
                         view->tovectors_button, 5);

  view->stroke_button =
    gimp_editor_add_action_button (editor, "paths",
                                   "paths-stroke",
                                   "paths-stroke-last-values",
                                   GDK_SHIFT_MASK,
                                   NULL);
  gimp_container_view_enable_dnd (GIMP_CONTAINER_VIEW (editor),
                                  GTK_BUTTON (view->stroke_button),
                                  GIMP_TYPE_PATH);
  gtk_box_reorder_child (gimp_editor_get_button_box (editor),
                         view->stroke_button, 6);

  gimp_dnd_svg_dest_add (GTK_WIDGET (tree_view->view), NULL, view);
}

static void
gimp_path_tree_view_set_container (GimpContainerView *view,
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
                               gimp_path_tree_view_drag_svg,
                               tree_view);
    }
}

static void
gimp_path_tree_view_drop_svg (GimpContainerTreeView   *tree_view,
                              const gchar             *svg_data,
                              gsize                    svg_data_len,
                              GimpViewable            *dest_viewable,
                              GtkTreeViewDropPosition  drop_pos)
{
  GimpItemTreeView *item_view = GIMP_ITEM_TREE_VIEW (tree_view);
  GimpImage        *image     = gimp_item_tree_view_get_image (item_view);
  GimpPath         *parent;
  gint              index;
  GError           *error = NULL;

  if (image->gimp->be_verbose)
    g_print ("%s: SVG dropped (len = %d)\n", G_STRFUNC, (gint) svg_data_len);

  index = gimp_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (GimpViewable **) &parent);

  if (! gimp_path_import_buffer (image, svg_data, svg_data_len,
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
gimp_path_tree_view_item_new (GimpImage *image)
{
  GimpPath *new_path;

  new_path = gimp_path_new (image, _("Path"));

  gimp_image_add_path (image, new_path,
                       GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

  return GIMP_ITEM (new_path);
}

static guchar *
gimp_path_tree_view_drag_svg (GtkWidget *widget,
                              gsize     *svg_data_len,
                              gpointer   data)
{
  GimpItemTreeView *view  = GIMP_ITEM_TREE_VIEW (data);
  GimpImage        *image = gimp_item_tree_view_get_image (view);
  GList            *items;
  gchar            *svg_data = NULL;

  items = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_selected_items (image);

  *svg_data_len = 0;

  if (items)
    {
      svg_data = gimp_path_export_string (image, items);

      if (svg_data)
        *svg_data_len = strlen (svg_data);
    }

  return (guchar *) svg_data;
}
