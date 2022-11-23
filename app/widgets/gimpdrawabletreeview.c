/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadrawabletreeview.c
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmadrawable.h"
#include "core/ligmadrawable-edit.h"
#include "core/ligmafilloptions.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmapattern.h"

#include "ligmacontainerview.h"
#include "ligmadnd.h"
#include "ligmadrawabletreeview.h"

#include "ligma-intl.h"


static void   ligma_drawable_tree_view_view_iface_init (LigmaContainerViewInterface *iface);

static void     ligma_drawable_tree_view_constructed   (GObject           *object);

static gboolean ligma_drawable_tree_view_select_items  (LigmaContainerView *view,
                                                       GList             *items,
                                                       GList             *paths);

static gboolean ligma_drawable_tree_view_drop_possible(LigmaContainerTreeView *view,
                                                      LigmaDndType          src_type,
                                                      GList               *src_viewables,
                                                      LigmaViewable        *dest_viewable,
                                                      GtkTreePath         *drop_path,
                                                      GtkTreeViewDropPosition  drop_pos,
                                                      GtkTreeViewDropPosition *return_drop_pos,
                                                      GdkDragAction       *return_drag_action);
static void   ligma_drawable_tree_view_drop_viewables (LigmaContainerTreeView   *view,
                                                      GList                   *src_viewables,
                                                      LigmaViewable            *dest_viewable,
                                                      GtkTreeViewDropPosition  drop_pos);
static void   ligma_drawable_tree_view_drop_color (LigmaContainerTreeView *view,
                                                  const LigmaRGB       *color,
                                                  LigmaViewable        *dest_viewable,
                                                  GtkTreeViewDropPosition  drop_pos);

static void   ligma_drawable_tree_view_set_image  (LigmaItemTreeView     *view,
                                                  LigmaImage            *image);

static void   ligma_drawable_tree_view_floating_selection_changed
                                                 (LigmaImage            *image,
                                                  LigmaDrawableTreeView *view);

static void   ligma_drawable_tree_view_new_pattern_dropped
                                                 (GtkWidget            *widget,
                                                  gint                  x,
                                                  gint                  y,
                                                  LigmaViewable         *viewable,
                                                  gpointer              data);
static void   ligma_drawable_tree_view_new_color_dropped
                                                 (GtkWidget            *widget,
                                                  gint                  x,
                                                  gint                  y,
                                                  const LigmaRGB        *color,
                                                  gpointer              data);


G_DEFINE_TYPE_WITH_CODE (LigmaDrawableTreeView, ligma_drawable_tree_view,
                         LIGMA_TYPE_ITEM_TREE_VIEW,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONTAINER_VIEW,
                                                ligma_drawable_tree_view_view_iface_init))

#define parent_class ligma_drawable_tree_view_parent_class

static LigmaContainerViewInterface *parent_view_iface = NULL;


static void
ligma_drawable_tree_view_class_init (LigmaDrawableTreeViewClass *klass)
{
  GObjectClass               *object_class;
  LigmaContainerTreeViewClass *tree_view_class;
  LigmaItemTreeViewClass      *item_view_class;

  object_class    = G_OBJECT_CLASS (klass);
  tree_view_class = LIGMA_CONTAINER_TREE_VIEW_CLASS (klass);
  item_view_class = LIGMA_ITEM_TREE_VIEW_CLASS (klass);

  object_class->constructed      = ligma_drawable_tree_view_constructed;

  tree_view_class->drop_possible  = ligma_drawable_tree_view_drop_possible;
  tree_view_class->drop_viewables = ligma_drawable_tree_view_drop_viewables;
  tree_view_class->drop_color     = ligma_drawable_tree_view_drop_color;

  item_view_class->set_image     = ligma_drawable_tree_view_set_image;

  item_view_class->lock_content_icon_name    = LIGMA_ICON_LOCK_CONTENT;
  item_view_class->lock_content_tooltip      = _("Lock pixels");
  item_view_class->lock_position_icon_name   = LIGMA_ICON_LOCK_POSITION;
  item_view_class->lock_position_tooltip     = _("Lock position and size");
  item_view_class->lock_visibility_icon_name = LIGMA_ICON_LOCK_VISIBILITY;
  item_view_class->lock_visibility_tooltip   = _("Lock visibility");
}

static void
ligma_drawable_tree_view_view_iface_init (LigmaContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->select_items = ligma_drawable_tree_view_select_items;
}

static void
ligma_drawable_tree_view_init (LigmaDrawableTreeView *view)
{
}

static void
ligma_drawable_tree_view_constructed (GObject *object)
{
  LigmaContainerTreeView *tree_view = LIGMA_CONTAINER_TREE_VIEW (object);
  LigmaItemTreeView      *item_view = LIGMA_ITEM_TREE_VIEW (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_dnd_viewable_dest_add (ligma_item_tree_view_get_new_button (item_view),
                              LIGMA_TYPE_PATTERN,
                              ligma_drawable_tree_view_new_pattern_dropped,
                              item_view);
  ligma_dnd_color_dest_add (ligma_item_tree_view_get_new_button (item_view),
                           ligma_drawable_tree_view_new_color_dropped,
                           item_view);

  ligma_dnd_color_dest_add    (GTK_WIDGET (tree_view->view),
                              NULL, tree_view);
  ligma_dnd_viewable_dest_add (GTK_WIDGET (tree_view->view), LIGMA_TYPE_PATTERN,
                              NULL, tree_view);
}


/*  LigmaContainerView methods  */

static gboolean
ligma_drawable_tree_view_select_items (LigmaContainerView *view,
                                      GList             *items,
                                      GList             *paths)
{
  LigmaItemTreeView *item_view = LIGMA_ITEM_TREE_VIEW (view);
  LigmaImage        *image     = ligma_item_tree_view_get_image (item_view);
  gboolean          success   = TRUE;

  if (image)
    {
      LigmaLayer *floating_sel = ligma_image_get_floating_selection (image);

      success = (items        == NULL ||
                 floating_sel == NULL ||
                 (g_list_length (items) == 1 &&
                  items->data           == LIGMA_VIEWABLE (floating_sel)));

      if (! success)
        {
          Ligma        *ligma    = image->ligma;
          LigmaContext *context = ligma_get_user_context (ligma);
          LigmaDisplay *display = ligma_context_get_display (context);

          ligma_message_literal (ligma, G_OBJECT (display), LIGMA_MESSAGE_WARNING,
                                _("Cannot select items while a floating "
                                  "selection is active."));
        }
    }

  if (success)
    success = parent_view_iface->select_items (view, items, paths);

  return success;
}

/*  LigmaContainerTreeView methods  */

static gboolean
ligma_drawable_tree_view_drop_possible (LigmaContainerTreeView   *tree_view,
                                       LigmaDndType              src_type,
                                       GList                   *src_viewables,
                                       LigmaViewable            *dest_viewable,
                                       GtkTreePath             *drop_path,
                                       GtkTreeViewDropPosition  drop_pos,
                                       GtkTreeViewDropPosition *return_drop_pos,
                                       GdkDragAction           *return_drag_action)
{
  if (LIGMA_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_possible (tree_view,
                                                                    src_type,
                                                                    src_viewables,
                                                                    dest_viewable,
                                                                    drop_path,
                                                                    drop_pos,
                                                                    return_drop_pos,
                                                                    return_drag_action))
    {
      if (src_type == LIGMA_DND_TYPE_COLOR ||
          src_type == LIGMA_DND_TYPE_PATTERN)
        {
          if (! dest_viewable ||
              ligma_item_is_content_locked (LIGMA_ITEM (dest_viewable), NULL) ||
              ligma_viewable_get_children (LIGMA_VIEWABLE (dest_viewable)))
            return FALSE;

          if (return_drop_pos)
            {
              *return_drop_pos = GTK_TREE_VIEW_DROP_INTO_OR_AFTER;
            }
        }

      return TRUE;
    }

  return FALSE;
}

static void
ligma_drawable_tree_view_drop_viewables (LigmaContainerTreeView   *view,
                                        GList                   *src_viewables,
                                        LigmaViewable            *dest_viewable,
                                        GtkTreeViewDropPosition  drop_pos)
{
  GList *iter;

  for (iter = src_viewables; iter; iter = iter->next)
    {
      LigmaViewable *src_viewable = iter->data;

      if (dest_viewable && LIGMA_IS_PATTERN (src_viewable))
        {
          LigmaImage       *image   = ligma_item_get_image (LIGMA_ITEM (dest_viewable));
          LigmaFillOptions *options = ligma_fill_options_new (image->ligma, NULL, FALSE);

          ligma_fill_options_set_style (options, LIGMA_FILL_STYLE_PATTERN);
          ligma_context_set_pattern (LIGMA_CONTEXT (options),
                                    LIGMA_PATTERN (src_viewable));

          ligma_drawable_edit_fill (LIGMA_DRAWABLE (dest_viewable),
                                   options,
                                   C_("undo-type", "Drop pattern to layer"));

          g_object_unref (options);

          ligma_image_flush (image);
          return;
        }
    }

  LIGMA_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_viewables (view,
                                                                 src_viewables,
                                                                 dest_viewable,
                                                                 drop_pos);
}

static void
ligma_drawable_tree_view_drop_color (LigmaContainerTreeView   *view,
                                    const LigmaRGB           *color,
                                    LigmaViewable            *dest_viewable,
                                    GtkTreeViewDropPosition  drop_pos)
{
  if (dest_viewable)
    {
      LigmaImage       *image   = ligma_item_get_image (LIGMA_ITEM (dest_viewable));
      LigmaFillOptions *options = ligma_fill_options_new (image->ligma, NULL, FALSE);

      ligma_fill_options_set_style (options, LIGMA_FILL_STYLE_SOLID);
      ligma_context_set_foreground (LIGMA_CONTEXT (options), color);

      ligma_drawable_edit_fill (LIGMA_DRAWABLE (dest_viewable),
                               options,
                               C_("undo-type", "Drop color to layer"));

      g_object_unref (options);

      ligma_image_flush (image);
    }
}


/*  LigmaItemTreeView methods  */

static void
ligma_drawable_tree_view_set_image (LigmaItemTreeView *view,
                                   LigmaImage        *image)
{
  if (ligma_item_tree_view_get_image (view))
    g_signal_handlers_disconnect_by_func (ligma_item_tree_view_get_image (view),
                                          ligma_drawable_tree_view_floating_selection_changed,
                                          view);

  LIGMA_ITEM_TREE_VIEW_CLASS (parent_class)->set_image (view, image);

  if (ligma_item_tree_view_get_image (view))
    g_signal_connect (ligma_item_tree_view_get_image (view),
                      "floating-selection-changed",
                      G_CALLBACK (ligma_drawable_tree_view_floating_selection_changed),
                      view);
}


/*  callbacks  */

static void
ligma_drawable_tree_view_floating_selection_changed (LigmaImage            *image,
                                                    LigmaDrawableTreeView *view)
{
  GList *items;

  items = LIGMA_ITEM_TREE_VIEW_GET_CLASS (view)->get_selected_items (image);
  items = g_list_copy (items);

  /*  update button states  */
  g_signal_handlers_block_by_func (ligma_item_tree_view_get_image (LIGMA_ITEM_TREE_VIEW (view)),
                                   ligma_drawable_tree_view_floating_selection_changed,
                                   view);
  ligma_container_view_select_items (LIGMA_CONTAINER_VIEW (view), items);
  g_signal_handlers_unblock_by_func (ligma_item_tree_view_get_image (LIGMA_ITEM_TREE_VIEW (view)),
                                     ligma_drawable_tree_view_floating_selection_changed,
                                     view);
  g_list_free (items);
}

static void
ligma_drawable_tree_view_new_dropped (LigmaItemTreeView *view,
                                     LigmaFillOptions  *options,
                                     const gchar      *undo_desc)
{
  LigmaImage *image = ligma_item_tree_view_get_image (view);
  LigmaItem  *item;

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_EDIT_PASTE,
                               _("New Layer"));

  item = LIGMA_ITEM_TREE_VIEW_GET_CLASS (view)->new_item (image);

  if (item)
    ligma_drawable_edit_fill (LIGMA_DRAWABLE (item), options, undo_desc);

  ligma_image_undo_group_end (image);

  ligma_image_flush (image);
}

static void
ligma_drawable_tree_view_new_pattern_dropped (GtkWidget    *widget,
                                             gint          x,
                                             gint          y,
                                             LigmaViewable *viewable,
                                             gpointer      data)
{
  LigmaItemTreeView *view    = LIGMA_ITEM_TREE_VIEW (data);
  LigmaImage        *image   = ligma_item_tree_view_get_image (view);
  LigmaFillOptions  *options = ligma_fill_options_new (image->ligma, NULL, FALSE);

  ligma_fill_options_set_style (options, LIGMA_FILL_STYLE_PATTERN);
  ligma_context_set_pattern (LIGMA_CONTEXT (options), LIGMA_PATTERN (viewable));

  ligma_drawable_tree_view_new_dropped (view, options,
                                       C_("undo-type", "Drop pattern to layer"));

  g_object_unref (options);
}

static void
ligma_drawable_tree_view_new_color_dropped (GtkWidget     *widget,
                                           gint           x,
                                           gint           y,
                                           const LigmaRGB *color,
                                           gpointer       data)
{
  LigmaItemTreeView *view    = LIGMA_ITEM_TREE_VIEW (data);
  LigmaImage        *image   = ligma_item_tree_view_get_image (view);
  LigmaFillOptions  *options = ligma_fill_options_new (image->ligma, NULL, FALSE);

  ligma_fill_options_set_style (options, LIGMA_FILL_STYLE_SOLID);
  ligma_context_set_foreground (LIGMA_CONTEXT (options), color);

  ligma_drawable_tree_view_new_dropped (view, options,
                                       C_("undo-type", "Drop color to layer"));

  g_object_unref (options);
}
