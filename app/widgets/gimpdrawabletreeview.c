/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawabletreeview.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-bucket-fill.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "gimpcontainerview.h"
#include "gimpdnd.h"
#include "gimpdrawabletreeview.h"

#include "gimp-intl.h"


static void   gimp_drawable_tree_view_view_iface_init (GimpContainerViewInterface *iface);

static void     gimp_drawable_tree_view_constructed (GObject           *object);

static gboolean gimp_drawable_tree_view_select_item (GimpContainerView *view,
                                                     GimpViewable      *item,
                                                     gpointer           insert_data);

static gboolean gimp_drawable_tree_view_drop_possible(GimpContainerTreeView *view,
                                                      GimpDndType          src_type,
                                                      GimpViewable        *src_viewable,
                                                      GimpViewable        *dest_viewable,
                                                      GtkTreePath         *drop_path,
                                                      GtkTreeViewDropPosition  drop_pos,
                                                      GtkTreeViewDropPosition *return_drop_pos,
                                                      GdkDragAction       *return_drag_action);
static void   gimp_drawable_tree_view_drop_viewable (GimpContainerTreeView *view,
                                                     GimpViewable     *src_viewable,
                                                     GimpViewable     *dest_viewable,
                                                     GtkTreeViewDropPosition  drop_pos);
static void   gimp_drawable_tree_view_drop_color (GimpContainerTreeView *view,
                                                  const GimpRGB       *color,
                                                  GimpViewable        *dest_viewable,
                                                  GtkTreeViewDropPosition  drop_pos);

static void   gimp_drawable_tree_view_set_image  (GimpItemTreeView     *view,
                                                  GimpImage            *image);

static void   gimp_drawable_tree_view_floating_selection_changed
                                                 (GimpImage            *image,
                                                  GimpDrawableTreeView *view);

static void   gimp_drawable_tree_view_new_pattern_dropped
                                                 (GtkWidget            *widget,
                                                  gint                  x,
                                                  gint                  y,
                                                  GimpViewable         *viewable,
                                                  gpointer              data);
static void   gimp_drawable_tree_view_new_color_dropped
                                                 (GtkWidget            *widget,
                                                  gint                  x,
                                                  gint                  y,
                                                  const GimpRGB        *color,
                                                  gpointer              data);


G_DEFINE_TYPE_WITH_CODE (GimpDrawableTreeView, gimp_drawable_tree_view,
                         GIMP_TYPE_ITEM_TREE_VIEW,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONTAINER_VIEW,
                                                gimp_drawable_tree_view_view_iface_init))

#define parent_class gimp_drawable_tree_view_parent_class

static GimpContainerViewInterface *parent_view_iface = NULL;


static void
gimp_drawable_tree_view_class_init (GimpDrawableTreeViewClass *klass)
{
  GObjectClass               *object_class;
  GimpContainerTreeViewClass *tree_view_class;
  GimpItemTreeViewClass      *item_view_class;

  object_class    = G_OBJECT_CLASS (klass);
  tree_view_class = GIMP_CONTAINER_TREE_VIEW_CLASS (klass);
  item_view_class = GIMP_ITEM_TREE_VIEW_CLASS (klass);

  object_class->constructed      = gimp_drawable_tree_view_constructed;

  tree_view_class->drop_possible = gimp_drawable_tree_view_drop_possible;
  tree_view_class->drop_viewable = gimp_drawable_tree_view_drop_viewable;
  tree_view_class->drop_color    = gimp_drawable_tree_view_drop_color;

  item_view_class->set_image     = gimp_drawable_tree_view_set_image;

  item_view_class->lock_content_stock_id = GIMP_STOCK_TOOL_PAINTBRUSH;
  item_view_class->lock_content_tooltip  = _("Lock pixels");
}

static void
gimp_drawable_tree_view_view_iface_init (GimpContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->select_item = gimp_drawable_tree_view_select_item;
}

static void
gimp_drawable_tree_view_init (GimpDrawableTreeView *view)
{
}

static void
gimp_drawable_tree_view_constructed (GObject *object)
{
  GimpContainerTreeView *tree_view = GIMP_CONTAINER_TREE_VIEW (object);
  GimpItemTreeView      *item_view = GIMP_ITEM_TREE_VIEW (object);

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_dnd_viewable_dest_add (gimp_item_tree_view_get_new_button (item_view),
                              GIMP_TYPE_PATTERN,
                              gimp_drawable_tree_view_new_pattern_dropped,
                              item_view);
  gimp_dnd_color_dest_add (gimp_item_tree_view_get_new_button (item_view),
                           gimp_drawable_tree_view_new_color_dropped,
                           item_view);

  gimp_dnd_color_dest_add    (GTK_WIDGET (tree_view->view),
                              NULL, tree_view);
  gimp_dnd_viewable_dest_add (GTK_WIDGET (tree_view->view), GIMP_TYPE_PATTERN,
                              NULL, tree_view);
}


/*  GimpContainerView methods  */

static gboolean
gimp_drawable_tree_view_select_item (GimpContainerView *view,
                                     GimpViewable      *item,
                                     gpointer           insert_data)
{
  GimpItemTreeView *item_view = GIMP_ITEM_TREE_VIEW (view);
  gboolean          success   = TRUE;

  if (gimp_item_tree_view_get_image (item_view))
    {
      GimpLayer *floating_sel =
        gimp_image_get_floating_selection (gimp_item_tree_view_get_image (item_view));

      success = (item         == NULL ||
                 floating_sel == NULL ||
                 item         == GIMP_VIEWABLE (floating_sel));
    }

  if (success)
    success = parent_view_iface->select_item (view, item, insert_data);

  return success;
}


/*  GimpContainerTreeView methods  */

static gboolean
gimp_drawable_tree_view_drop_possible (GimpContainerTreeView   *tree_view,
                                       GimpDndType              src_type,
                                       GimpViewable            *src_viewable,
                                       GimpViewable            *dest_viewable,
                                       GtkTreePath             *drop_path,
                                       GtkTreeViewDropPosition  drop_pos,
                                       GtkTreeViewDropPosition *return_drop_pos,
                                       GdkDragAction           *return_drag_action)
{
  if (GIMP_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_possible (tree_view,
                                                                    src_type,
                                                                    src_viewable,
                                                                    dest_viewable,
                                                                    drop_path,
                                                                    drop_pos,
                                                                    return_drop_pos,
                                                                    return_drag_action))
    {
      if (src_type == GIMP_DND_TYPE_COLOR ||
          src_type == GIMP_DND_TYPE_PATTERN)
        {
          if (! dest_viewable ||
              gimp_item_is_content_locked (GIMP_ITEM (dest_viewable)) ||
              gimp_viewable_get_children (GIMP_VIEWABLE (dest_viewable)))
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
gimp_drawable_tree_view_drop_viewable (GimpContainerTreeView   *view,
                                       GimpViewable            *src_viewable,
                                       GimpViewable            *dest_viewable,
                                       GtkTreeViewDropPosition  drop_pos)
{
  if (dest_viewable && GIMP_IS_PATTERN (src_viewable))
    {
      gimp_drawable_bucket_fill_full (GIMP_DRAWABLE (dest_viewable),
                                      GIMP_PATTERN_BUCKET_FILL,
                                      GIMP_NORMAL_MODE, GIMP_OPACITY_OPAQUE,
                                      FALSE,             /* no seed fill */
                                      FALSE,             /* don't fill transp */
                                      GIMP_SELECT_CRITERION_COMPOSITE,
                                      0.0, FALSE,        /* fill params  */
                                      0.0, 0.0,          /* ignored      */
                                      NULL, GIMP_PATTERN (src_viewable));
      gimp_image_flush (gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view)));
      return;
    }

  GIMP_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_viewable (view,
                                                                src_viewable,
                                                                dest_viewable,
                                                                drop_pos);
}

static void
gimp_drawable_tree_view_drop_color (GimpContainerTreeView   *view,
                                    const GimpRGB           *color,
                                    GimpViewable            *dest_viewable,
                                    GtkTreeViewDropPosition  drop_pos)
{
  if (dest_viewable)
    {
      gimp_drawable_bucket_fill_full (GIMP_DRAWABLE (dest_viewable),
                                      GIMP_FG_BUCKET_FILL,
                                      GIMP_NORMAL_MODE, GIMP_OPACITY_OPAQUE,
                                      FALSE,             /* no seed fill */
                                      FALSE,             /* don't fill transp */
                                      GIMP_SELECT_CRITERION_COMPOSITE,
                                      0.0, FALSE,        /* fill params  */
                                      0.0, 0.0,          /* ignored      */
                                      color, NULL);
      gimp_image_flush (gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view)));
    }
}


/*  GimpItemTreeView methods  */

static void
gimp_drawable_tree_view_set_image (GimpItemTreeView *view,
                                   GimpImage        *image)
{
  if (gimp_item_tree_view_get_image (view))
    g_signal_handlers_disconnect_by_func (gimp_item_tree_view_get_image (view),
                                          gimp_drawable_tree_view_floating_selection_changed,
                                          view);

  GIMP_ITEM_TREE_VIEW_CLASS (parent_class)->set_image (view, image);

  if (gimp_item_tree_view_get_image (view))
    g_signal_connect (gimp_item_tree_view_get_image (view),
                      "floating-selection-changed",
                      G_CALLBACK (gimp_drawable_tree_view_floating_selection_changed),
                      view);
}


/*  callbacks  */

static void
gimp_drawable_tree_view_floating_selection_changed (GimpImage            *image,
                                                    GimpDrawableTreeView *view)
{
  GimpItem *item;

  item = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (image);

  /*  update button states  */
  gimp_container_view_select_item (GIMP_CONTAINER_VIEW (view),
                                   (GimpViewable *) item);
}

static void
gimp_drawable_tree_view_new_dropped (GimpItemTreeView   *view,
                                     gint                x,
                                     gint                y,
                                     GimpBucketFillMode  fill_mode,
                                     const GimpRGB      *color,
                                     GimpPattern        *pattern)
{
  GimpItem *item;

  gimp_image_undo_group_start (gimp_item_tree_view_get_image (view), GIMP_UNDO_GROUP_EDIT_PASTE,
                               _("New Layer"));

  item = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->new_item (gimp_item_tree_view_get_image (view));

  if (item)
    {
      /*  Get the bucket fill context  */
      GimpContext  *context;
      GimpToolInfo *tool_info = gimp_get_tool_info (gimp_item_tree_view_get_image (view)->gimp,
                                                    "gimp-bucket-fill-tool");

      if (tool_info && tool_info->tool_options)
        context = GIMP_CONTEXT (tool_info->tool_options);
      else
        context = gimp_container_view_get_context (GIMP_CONTAINER_VIEW (view));

      gimp_drawable_bucket_fill_full (GIMP_DRAWABLE (item),
                                      fill_mode,
                                      gimp_context_get_paint_mode (context),
                                      gimp_context_get_opacity (context),
                                      FALSE, /* no seed fill */
                                      FALSE, /* don't fill transp */
                                      GIMP_SELECT_CRITERION_COMPOSITE,
                                      0.0, FALSE, 0.0, 0.0 /* fill params */,
                                      color, pattern);
    }

  gimp_image_undo_group_end (gimp_item_tree_view_get_image (view));

  gimp_image_flush (gimp_item_tree_view_get_image (view));
}

static void
gimp_drawable_tree_view_new_pattern_dropped (GtkWidget    *widget,
                                             gint          x,
                                             gint          y,
                                             GimpViewable *viewable,
                                             gpointer      data)
{
  gimp_drawable_tree_view_new_dropped (GIMP_ITEM_TREE_VIEW (data), x, y,
                                       GIMP_PATTERN_BUCKET_FILL,
                                       NULL,
                                       GIMP_PATTERN (viewable));
}

static void
gimp_drawable_tree_view_new_color_dropped (GtkWidget     *widget,
                                           gint           x,
                                           gint           y,
                                           const GimpRGB *color,
                                           gpointer       data)
{
  gimp_drawable_tree_view_new_dropped (GIMP_ITEM_TREE_VIEW (data), x, y,
                                       GIMP_FG_BUCKET_FILL,
                                       color,
                                       NULL);
}
