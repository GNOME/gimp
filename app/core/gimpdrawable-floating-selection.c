/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gegl/gimpapplicator.h"

#include "gimpchannel.h"
#include "gimpdrawable-floating-selection.h"
#include "gimpdrawable-filters.h"
#include "gimpdrawable-private.h"
#include "gimpimage.h"
#include "gimplayer.h"

#include "gimp-log.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void    gimp_drawable_remove_fs_filter             (GimpDrawable      *drawable);
static void    gimp_drawable_sync_fs_filter               (GimpDrawable      *drawable);

static void    gimp_drawable_fs_notify                    (GObject           *object,
                                                           const GParamSpec  *pspec,
                                                           GimpDrawable      *drawable);
static void    gimp_drawable_fs_lock_position_changed     (GimpDrawable      *signal_drawable,
                                                           GimpDrawable      *drawable);
static void    gimp_drawable_fs_format_changed            (GimpDrawable      *signal_drawable,
                                                           GimpDrawable      *drawable);
static void    gimp_drawable_fs_affect_changed            (GimpImage         *image,
                                                           GimpChannelType    channel,
                                                           GimpDrawable      *drawable);
static void    gimp_drawable_fs_mask_changed              (GimpImage         *image,
                                                           GimpDrawable      *drawable);
static void    gimp_drawable_fs_visibility_changed        (GimpLayer         *fs,
                                                           GimpDrawable      *drawable);
static void    gimp_drawable_fs_excludes_backdrop_changed (GimpLayer         *fs,
                                                           GimpDrawable      *drawable);
static void    gimp_drawable_fs_bounding_box_changed      (GimpLayer         *fs,
                                                           GimpDrawable      *drawable);
static void    gimp_drawable_fs_update                    (GimpLayer         *fs,
                                                           gint               x,
                                                           gint               y,
                                                           gint               width,
                                                           gint               height,
                                                           GimpDrawable      *drawable);


/*  public functions  */

GimpLayer *
gimp_drawable_get_floating_sel (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return drawable->private->floating_selection;
}

void
gimp_drawable_attach_floating_sel (GimpDrawable *drawable,
                                   GimpLayer    *fs)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (gimp_drawable_get_floating_sel (drawable) == NULL);
  g_return_if_fail (GIMP_IS_LAYER (fs));

  GIMP_LOG (FLOATING_SELECTION, "%s", G_STRFUNC);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  drawable->private->floating_selection = fs;
  gimp_image_set_floating_selection (image, fs);

  /*  clear the selection  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (fs));

  gimp_item_bind_visible_to_active (GIMP_ITEM (fs), FALSE);
  gimp_filter_set_active (GIMP_FILTER (fs), FALSE);

  _gimp_drawable_add_floating_sel_filter (drawable);

  g_signal_connect (fs, "visibility-changed",
                    G_CALLBACK (gimp_drawable_fs_visibility_changed),
                    drawable);
  g_signal_connect (fs, "excludes-backdrop-changed",
                    G_CALLBACK (gimp_drawable_fs_excludes_backdrop_changed),
                    drawable);
  g_signal_connect (fs, "bounding-box-changed",
                    G_CALLBACK (gimp_drawable_fs_bounding_box_changed),
                    drawable);
  g_signal_connect (fs, "update",
                    G_CALLBACK (gimp_drawable_fs_update),
                    drawable);

  gimp_drawable_fs_update (fs,
                           0, 0,
                           gimp_item_get_width  (GIMP_ITEM (fs)),
                           gimp_item_get_height (GIMP_ITEM (fs)),
                           drawable);
}

void
gimp_drawable_detach_floating_sel (GimpDrawable *drawable)
{
  GimpImage *image;
  GimpLayer *fs;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_drawable_get_floating_sel (drawable) != NULL);

  GIMP_LOG (FLOATING_SELECTION, "%s", G_STRFUNC);

  image = gimp_item_get_image (GIMP_ITEM (drawable));
  fs    = drawable->private->floating_selection;

  gimp_drawable_remove_fs_filter (drawable);

  g_signal_handlers_disconnect_by_func (fs,
                                        gimp_drawable_fs_visibility_changed,
                                        drawable);
  g_signal_handlers_disconnect_by_func (fs,
                                        gimp_drawable_fs_excludes_backdrop_changed,
                                        drawable);
  g_signal_handlers_disconnect_by_func (fs,
                                        gimp_drawable_fs_bounding_box_changed,
                                        drawable);
  g_signal_handlers_disconnect_by_func (fs,
                                        gimp_drawable_fs_update,
                                        drawable);

  gimp_drawable_fs_update (fs,
                           0, 0,
                           gimp_item_get_width  (GIMP_ITEM (fs)),
                           gimp_item_get_height (GIMP_ITEM (fs)),
                           drawable);

  gimp_item_bind_visible_to_active (GIMP_ITEM (fs), TRUE);

  /*  clear the selection  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (fs));

  gimp_image_set_floating_selection (image, NULL);
  drawable->private->floating_selection = NULL;
}

GimpFilter *
gimp_drawable_get_floating_sel_filter (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_drawable_get_floating_sel (drawable) != NULL, NULL);

  /* Ensure that the graph is construced before the filter is used.
   * Otherwise, we rely on the projection to cause the graph to be
   * constructed, which fails for images that aren't displayed.
   */
  gimp_filter_get_node (GIMP_FILTER (drawable));

  return drawable->private->fs_filter;
}


/*  private functions  */

void
_gimp_drawable_add_floating_sel_filter (GimpDrawable *drawable)
{
  GimpDrawablePrivate *private = drawable->private;
  GimpImage           *image   = gimp_item_get_image (GIMP_ITEM (drawable));
  GimpLayer           *fs      = gimp_drawable_get_floating_sel (drawable);
  GeglNode            *node;
  GeglNode            *fs_source;

  if (! private->source_node)
    return;

  private->fs_filter = gimp_filter_new (_("Floating Selection"));
  gimp_viewable_set_icon_name (GIMP_VIEWABLE (private->fs_filter),
                               "gimp-floating-selection");

  node = gimp_filter_get_node (private->fs_filter);

  fs_source = gimp_drawable_get_source_node (GIMP_DRAWABLE (fs));

  /* rip the fs' source node out of its graph */
  if (fs->layer_offset_node)
    {
      gegl_node_disconnect (fs->layer_offset_node, "input");
      gegl_node_remove_child (gimp_filter_get_node (GIMP_FILTER (fs)),
                              fs_source);
    }

  gegl_node_add_child (node, fs_source);

  private->fs_applicator = gimp_applicator_new (node);

  gimp_filter_set_applicator (private->fs_filter, private->fs_applicator);

  gimp_applicator_set_cache (private->fs_applicator, TRUE);

  private->fs_crop_node = gegl_node_new_child (node,
                                               "operation", "gegl:nop",
                                               NULL);

  gegl_node_connect_to (fs_source,             "output",
                        private->fs_crop_node, "input");
  gegl_node_connect_to (private->fs_crop_node, "output",
                        node,                  "aux");

  gimp_drawable_add_filter (drawable, private->fs_filter);

  g_signal_connect (fs, "notify",
                    G_CALLBACK (gimp_drawable_fs_notify),
                    drawable);
  g_signal_connect (drawable, "notify::offset-x",
                    G_CALLBACK (gimp_drawable_fs_notify),
                    drawable);
  g_signal_connect (drawable, "notify::offset-y",
                    G_CALLBACK (gimp_drawable_fs_notify),
                    drawable);
  g_signal_connect (drawable, "lock-position-changed",
                    G_CALLBACK (gimp_drawable_fs_lock_position_changed),
                    drawable);
  g_signal_connect (drawable, "format-changed",
                    G_CALLBACK (gimp_drawable_fs_format_changed),
                    drawable);
  g_signal_connect (image, "component-active-changed",
                    G_CALLBACK (gimp_drawable_fs_affect_changed),
                    drawable);
  g_signal_connect (image, "mask-changed",
                    G_CALLBACK (gimp_drawable_fs_mask_changed),
                    drawable);

  gimp_drawable_sync_fs_filter (drawable);
}


/*  private functions  */

static void
gimp_drawable_remove_fs_filter (GimpDrawable *drawable)
{
  GimpDrawablePrivate *private = drawable->private;
  GimpImage           *image   = gimp_item_get_image (GIMP_ITEM (drawable));
  GimpLayer           *fs      = gimp_drawable_get_floating_sel (drawable);

  if (private->fs_filter)
    {
      GeglNode *node;
      GeglNode *fs_source;

      g_signal_handlers_disconnect_by_func (fs,
                                            gimp_drawable_fs_notify,
                                            drawable);
      g_signal_handlers_disconnect_by_func (drawable,
                                            gimp_drawable_fs_notify,
                                            drawable);
      g_signal_handlers_disconnect_by_func (drawable,
                                            gimp_drawable_fs_lock_position_changed,
                                            drawable);
      g_signal_handlers_disconnect_by_func (drawable,
                                            gimp_drawable_fs_format_changed,
                                            drawable);
      g_signal_handlers_disconnect_by_func (image,
                                            gimp_drawable_fs_affect_changed,
                                            drawable);
      g_signal_handlers_disconnect_by_func (image,
                                            gimp_drawable_fs_mask_changed,
                                            drawable);

      gimp_drawable_remove_filter (drawable, private->fs_filter);

      node = gimp_filter_get_node (private->fs_filter);

      fs_source = gimp_drawable_get_source_node (GIMP_DRAWABLE (fs));

      gegl_node_remove_child (node, fs_source);

      /* plug the fs' source node back into its graph */
      if (fs->layer_offset_node)
        {
          gegl_node_add_child (gimp_filter_get_node (GIMP_FILTER (fs)),
                               fs_source);
          gegl_node_connect_to (fs_source,             "output",
                                fs->layer_offset_node, "input");
        }

      g_clear_object (&private->fs_filter);
      g_clear_object (&private->fs_applicator);

      private->fs_crop_node = NULL;

      gimp_drawable_update_bounding_box (drawable);
    }
}

static void
gimp_drawable_sync_fs_filter (GimpDrawable *drawable)
{
  GimpDrawablePrivate *private = drawable->private;
  GimpImage           *image   = gimp_item_get_image (GIMP_ITEM (drawable));
  GimpChannel         *mask    = gimp_image_get_mask (image);
  GimpLayer           *fs      = gimp_drawable_get_floating_sel (drawable);
  gint                 off_x, off_y;
  gint                 fs_off_x, fs_off_y;

  gimp_filter_set_active (private->fs_filter,
                          gimp_item_get_visible (GIMP_ITEM (fs)));

  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
  gimp_item_get_offset (GIMP_ITEM (fs), &fs_off_x, &fs_off_y);

  if (gimp_item_get_clip (GIMP_ITEM (drawable), GIMP_TRANSFORM_RESIZE_ADJUST) ==
      GIMP_TRANSFORM_RESIZE_CLIP ||
      ! gimp_drawable_has_alpha (drawable))
    {
      gegl_node_set (
        private->fs_crop_node,
        "operation", "gegl:crop",
        "x",         (gdouble) (off_x - fs_off_x),
        "y",         (gdouble) (off_y - fs_off_y),
        "width",     (gdouble) gimp_item_get_width  (GIMP_ITEM (drawable)),
        "height",    (gdouble) gimp_item_get_height (GIMP_ITEM (drawable)),
        NULL);
    }
  else
    {
      gegl_node_set (
        private->fs_crop_node,
        "operation", "gegl:nop",
        NULL);
    }

  gimp_applicator_set_apply_offset (private->fs_applicator,
                                    fs_off_x - off_x,
                                    fs_off_y - off_y);

  if (gimp_channel_is_empty (mask))
    {
      gimp_applicator_set_mask_buffer (private->fs_applicator, NULL);
    }
  else
    {
      GeglBuffer *buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

      gimp_applicator_set_mask_buffer (private->fs_applicator, buffer);
      gimp_applicator_set_mask_offset (private->fs_applicator,
                                       -off_x, -off_y);
    }

  gimp_applicator_set_opacity (private->fs_applicator,
                               gimp_layer_get_opacity (fs));
  gimp_applicator_set_mode (private->fs_applicator,
                            gimp_layer_get_mode (fs),
                            gimp_layer_get_blend_space (fs),
                            gimp_layer_get_composite_space (fs),
                            gimp_layer_get_composite_mode (fs));
  gimp_applicator_set_affect (private->fs_applicator,
                              gimp_drawable_get_active_mask (drawable));
  gimp_applicator_set_output_format (private->fs_applicator,
                                     gimp_drawable_get_format (drawable));

  gimp_drawable_update_bounding_box (drawable);
}

static void
gimp_drawable_fs_notify (GObject          *object,
                         const GParamSpec *pspec,
                         GimpDrawable     *drawable)
{
  if (! strcmp (pspec->name, "offset-x")        ||
      ! strcmp (pspec->name, "offset-y")        ||
      ! strcmp (pspec->name, "visible")         ||
      ! strcmp (pspec->name, "mode")            ||
      ! strcmp (pspec->name, "blend-space")     ||
      ! strcmp (pspec->name, "composite-space") ||
      ! strcmp (pspec->name, "composite-mode")  ||
      ! strcmp (pspec->name, "opacity"))
    {
      gimp_drawable_sync_fs_filter (drawable);
    }
}

static void
gimp_drawable_fs_lock_position_changed (GimpDrawable *signal_drawable,
                                        GimpDrawable *drawable)
{
  GimpLayer *fs = gimp_drawable_get_floating_sel (drawable);

  gimp_drawable_sync_fs_filter (drawable);

  gimp_drawable_update (GIMP_DRAWABLE (fs), 0, 0, -1, -1);
}

static void
gimp_drawable_fs_format_changed (GimpDrawable *signal_drawable,
                                 GimpDrawable *drawable)
{
  GimpLayer *fs = gimp_drawable_get_floating_sel (drawable);

  gimp_drawable_sync_fs_filter (drawable);

  gimp_drawable_update (GIMP_DRAWABLE (fs), 0, 0, -1, -1);
}

static void
gimp_drawable_fs_affect_changed (GimpImage       *image,
                                 GimpChannelType  channel,
                                 GimpDrawable    *drawable)
{
  GimpLayer *fs = gimp_drawable_get_floating_sel (drawable);

  gimp_drawable_sync_fs_filter (drawable);

  gimp_drawable_update (GIMP_DRAWABLE (fs), 0, 0, -1, -1);
}

static void
gimp_drawable_fs_mask_changed (GimpImage    *image,
                               GimpDrawable *drawable)
{
  GimpLayer *fs = gimp_drawable_get_floating_sel (drawable);

  gimp_drawable_sync_fs_filter (drawable);

  gimp_drawable_update (GIMP_DRAWABLE (fs), 0, 0, -1, -1);
}

static void
gimp_drawable_fs_visibility_changed (GimpLayer    *fs,
                                     GimpDrawable *drawable)
{
  if (gimp_layer_get_excludes_backdrop (fs))
    gimp_drawable_update (drawable, 0, 0, -1, -1);
  else
    gimp_drawable_update (GIMP_DRAWABLE (fs), 0, 0, -1, -1);
}

static void
gimp_drawable_fs_excludes_backdrop_changed (GimpLayer    *fs,
                                            GimpDrawable *drawable)
{
  if (gimp_item_get_visible (GIMP_ITEM (fs)))
    gimp_drawable_update (drawable, 0, 0, -1, -1);
}

static void
gimp_drawable_fs_bounding_box_changed (GimpLayer    *fs,
                                       GimpDrawable *drawable)
{
  gimp_drawable_update_bounding_box (drawable);
}

static void
gimp_drawable_fs_update (GimpLayer    *fs,
                         gint          x,
                         gint          y,
                         gint          width,
                         gint          height,
                         GimpDrawable *drawable)
{
  GeglRectangle bounding_box;
  GeglRectangle rect;
  gint          fs_off_x, fs_off_y;
  gint          off_x, off_y;

  gimp_item_get_offset (GIMP_ITEM (fs), &fs_off_x, &fs_off_y);
  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

  bounding_box = gimp_drawable_get_bounding_box (drawable);

  bounding_box.x += off_x;
  bounding_box.y += off_y;

  rect.x      = x + fs_off_x;
  rect.y      = y + fs_off_y;
  rect.width  = width;
  rect.height = height;

  if (gegl_rectangle_intersect (&rect, &rect, &bounding_box))
    {
      gimp_drawable_update (drawable,
                            rect.x - off_x, rect.y - off_y,
                            rect.width, rect.height);
    }
}
