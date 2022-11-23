/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "gegl/ligmaapplicator.h"

#include "ligmachannel.h"
#include "ligmadrawable-floating-selection.h"
#include "ligmadrawable-filters.h"
#include "ligmadrawable-private.h"
#include "ligmaimage.h"
#include "ligmalayer.h"

#include "ligma-log.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void    ligma_drawable_remove_fs_filter             (LigmaDrawable      *drawable);
static void    ligma_drawable_sync_fs_filter               (LigmaDrawable      *drawable);

static void    ligma_drawable_fs_notify                    (GObject           *object,
                                                           const GParamSpec  *pspec,
                                                           LigmaDrawable      *drawable);
static void    ligma_drawable_fs_lock_position_changed     (LigmaDrawable      *signal_drawable,
                                                           LigmaDrawable      *drawable);
static void    ligma_drawable_fs_format_changed            (LigmaDrawable      *signal_drawable,
                                                           LigmaDrawable      *drawable);
static void    ligma_drawable_fs_affect_changed            (LigmaImage         *image,
                                                           LigmaChannelType    channel,
                                                           LigmaDrawable      *drawable);
static void    ligma_drawable_fs_mask_changed              (LigmaImage         *image,
                                                           LigmaDrawable      *drawable);
static void    ligma_drawable_fs_visibility_changed        (LigmaLayer         *fs,
                                                           LigmaDrawable      *drawable);
static void    ligma_drawable_fs_excludes_backdrop_changed (LigmaLayer         *fs,
                                                           LigmaDrawable      *drawable);
static void    ligma_drawable_fs_bounding_box_changed      (LigmaLayer         *fs,
                                                           LigmaDrawable      *drawable);
static void    ligma_drawable_fs_update                    (LigmaLayer         *fs,
                                                           gint               x,
                                                           gint               y,
                                                           gint               width,
                                                           gint               height,
                                                           LigmaDrawable      *drawable);


/*  public functions  */

LigmaLayer *
ligma_drawable_get_floating_sel (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);

  return drawable->private->floating_selection;
}

void
ligma_drawable_attach_floating_sel (LigmaDrawable *drawable,
                                   LigmaLayer    *fs)
{
  LigmaImage *image;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (ligma_drawable_get_floating_sel (drawable) == NULL);
  g_return_if_fail (LIGMA_IS_LAYER (fs));

  LIGMA_LOG (FLOATING_SELECTION, "%s", G_STRFUNC);

  image = ligma_item_get_image (LIGMA_ITEM (drawable));

  drawable->private->floating_selection = fs;
  ligma_image_set_floating_selection (image, fs);

  /*  clear the selection  */
  ligma_drawable_invalidate_boundary (LIGMA_DRAWABLE (fs));

  ligma_item_bind_visible_to_active (LIGMA_ITEM (fs), FALSE);
  ligma_filter_set_active (LIGMA_FILTER (fs), FALSE);

  _ligma_drawable_add_floating_sel_filter (drawable);

  g_signal_connect (fs, "visibility-changed",
                    G_CALLBACK (ligma_drawable_fs_visibility_changed),
                    drawable);
  g_signal_connect (fs, "excludes-backdrop-changed",
                    G_CALLBACK (ligma_drawable_fs_excludes_backdrop_changed),
                    drawable);
  g_signal_connect (fs, "bounding-box-changed",
                    G_CALLBACK (ligma_drawable_fs_bounding_box_changed),
                    drawable);
  g_signal_connect (fs, "update",
                    G_CALLBACK (ligma_drawable_fs_update),
                    drawable);

  ligma_drawable_fs_update (fs,
                           0, 0,
                           ligma_item_get_width  (LIGMA_ITEM (fs)),
                           ligma_item_get_height (LIGMA_ITEM (fs)),
                           drawable);
}

void
ligma_drawable_detach_floating_sel (LigmaDrawable *drawable)
{
  LigmaImage *image;
  LigmaLayer *fs;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_drawable_get_floating_sel (drawable) != NULL);

  LIGMA_LOG (FLOATING_SELECTION, "%s", G_STRFUNC);

  image = ligma_item_get_image (LIGMA_ITEM (drawable));
  fs    = drawable->private->floating_selection;

  ligma_drawable_remove_fs_filter (drawable);

  g_signal_handlers_disconnect_by_func (fs,
                                        ligma_drawable_fs_visibility_changed,
                                        drawable);
  g_signal_handlers_disconnect_by_func (fs,
                                        ligma_drawable_fs_excludes_backdrop_changed,
                                        drawable);
  g_signal_handlers_disconnect_by_func (fs,
                                        ligma_drawable_fs_bounding_box_changed,
                                        drawable);
  g_signal_handlers_disconnect_by_func (fs,
                                        ligma_drawable_fs_update,
                                        drawable);

  ligma_drawable_fs_update (fs,
                           0, 0,
                           ligma_item_get_width  (LIGMA_ITEM (fs)),
                           ligma_item_get_height (LIGMA_ITEM (fs)),
                           drawable);

  ligma_item_bind_visible_to_active (LIGMA_ITEM (fs), TRUE);

  /*  clear the selection  */
  ligma_drawable_invalidate_boundary (LIGMA_DRAWABLE (fs));

  ligma_image_set_floating_selection (image, NULL);
  drawable->private->floating_selection = NULL;
}

LigmaFilter *
ligma_drawable_get_floating_sel_filter (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (ligma_drawable_get_floating_sel (drawable) != NULL, NULL);

  /* Ensure that the graph is constructed before the filter is used.
   * Otherwise, we rely on the projection to cause the graph to be
   * constructed, which fails for images that aren't displayed.
   */
  ligma_filter_get_node (LIGMA_FILTER (drawable));

  return drawable->private->fs_filter;
}


/*  private functions  */

void
_ligma_drawable_add_floating_sel_filter (LigmaDrawable *drawable)
{
  LigmaDrawablePrivate *private = drawable->private;
  LigmaImage           *image   = ligma_item_get_image (LIGMA_ITEM (drawable));
  LigmaLayer           *fs      = ligma_drawable_get_floating_sel (drawable);
  GeglNode            *node;
  GeglNode            *fs_source;

  if (! private->source_node)
    return;

  private->fs_filter = ligma_filter_new (_("Floating Selection"));
  ligma_viewable_set_icon_name (LIGMA_VIEWABLE (private->fs_filter),
                               "ligma-floating-selection");

  node = ligma_filter_get_node (private->fs_filter);

  fs_source = ligma_drawable_get_source_node (LIGMA_DRAWABLE (fs));

  /* rip the fs' source node out of its graph */
  if (fs->layer_offset_node)
    {
      gegl_node_disconnect (fs->layer_offset_node, "input");
      gegl_node_remove_child (ligma_filter_get_node (LIGMA_FILTER (fs)),
                              fs_source);
    }

  gegl_node_add_child (node, fs_source);

  private->fs_applicator = ligma_applicator_new (node);

  ligma_filter_set_applicator (private->fs_filter, private->fs_applicator);

  ligma_applicator_set_cache (private->fs_applicator, TRUE);

  private->fs_crop_node = gegl_node_new_child (node,
                                               "operation", "gegl:nop",
                                               NULL);

  gegl_node_connect_to (fs_source,             "output",
                        private->fs_crop_node, "input");
  gegl_node_connect_to (private->fs_crop_node, "output",
                        node,                  "aux");

  ligma_drawable_add_filter (drawable, private->fs_filter);

  g_signal_connect (fs, "notify",
                    G_CALLBACK (ligma_drawable_fs_notify),
                    drawable);
  g_signal_connect (drawable, "notify::offset-x",
                    G_CALLBACK (ligma_drawable_fs_notify),
                    drawable);
  g_signal_connect (drawable, "notify::offset-y",
                    G_CALLBACK (ligma_drawable_fs_notify),
                    drawable);
  g_signal_connect (drawable, "lock-position-changed",
                    G_CALLBACK (ligma_drawable_fs_lock_position_changed),
                    drawable);
  g_signal_connect (drawable, "format-changed",
                    G_CALLBACK (ligma_drawable_fs_format_changed),
                    drawable);
  g_signal_connect (image, "component-active-changed",
                    G_CALLBACK (ligma_drawable_fs_affect_changed),
                    drawable);
  g_signal_connect (image, "mask-changed",
                    G_CALLBACK (ligma_drawable_fs_mask_changed),
                    drawable);

  ligma_drawable_sync_fs_filter (drawable);
}


/*  private functions  */

static void
ligma_drawable_remove_fs_filter (LigmaDrawable *drawable)
{
  LigmaDrawablePrivate *private = drawable->private;
  LigmaImage           *image   = ligma_item_get_image (LIGMA_ITEM (drawable));
  LigmaLayer           *fs      = ligma_drawable_get_floating_sel (drawable);

  if (private->fs_filter)
    {
      GeglNode *node;
      GeglNode *fs_source;

      g_signal_handlers_disconnect_by_func (fs,
                                            ligma_drawable_fs_notify,
                                            drawable);
      g_signal_handlers_disconnect_by_func (drawable,
                                            ligma_drawable_fs_notify,
                                            drawable);
      g_signal_handlers_disconnect_by_func (drawable,
                                            ligma_drawable_fs_lock_position_changed,
                                            drawable);
      g_signal_handlers_disconnect_by_func (drawable,
                                            ligma_drawable_fs_format_changed,
                                            drawable);
      g_signal_handlers_disconnect_by_func (image,
                                            ligma_drawable_fs_affect_changed,
                                            drawable);
      g_signal_handlers_disconnect_by_func (image,
                                            ligma_drawable_fs_mask_changed,
                                            drawable);

      ligma_drawable_remove_filter (drawable, private->fs_filter);

      node = ligma_filter_get_node (private->fs_filter);

      fs_source = ligma_drawable_get_source_node (LIGMA_DRAWABLE (fs));

      gegl_node_remove_child (node, fs_source);

      /* plug the fs' source node back into its graph */
      if (fs->layer_offset_node)
        {
          gegl_node_add_child (ligma_filter_get_node (LIGMA_FILTER (fs)),
                               fs_source);
          gegl_node_connect_to (fs_source,             "output",
                                fs->layer_offset_node, "input");
        }

      g_clear_object (&private->fs_filter);
      g_clear_object (&private->fs_applicator);

      private->fs_crop_node = NULL;

      ligma_drawable_update_bounding_box (drawable);
    }
}

static void
ligma_drawable_sync_fs_filter (LigmaDrawable *drawable)
{
  LigmaDrawablePrivate *private = drawable->private;
  LigmaImage           *image   = ligma_item_get_image (LIGMA_ITEM (drawable));
  LigmaChannel         *mask    = ligma_image_get_mask (image);
  LigmaLayer           *fs      = ligma_drawable_get_floating_sel (drawable);
  gint                 off_x, off_y;
  gint                 fs_off_x, fs_off_y;

  ligma_filter_set_active (private->fs_filter,
                          ligma_item_get_visible (LIGMA_ITEM (fs)));

  ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);
  ligma_item_get_offset (LIGMA_ITEM (fs), &fs_off_x, &fs_off_y);

  if (ligma_item_get_clip (LIGMA_ITEM (drawable), LIGMA_TRANSFORM_RESIZE_ADJUST) ==
      LIGMA_TRANSFORM_RESIZE_CLIP ||
      ! ligma_drawable_has_alpha (drawable))
    {
      gegl_node_set (
        private->fs_crop_node,
        "operation", "gegl:crop",
        "x",         (gdouble) (off_x - fs_off_x),
        "y",         (gdouble) (off_y - fs_off_y),
        "width",     (gdouble) ligma_item_get_width  (LIGMA_ITEM (drawable)),
        "height",    (gdouble) ligma_item_get_height (LIGMA_ITEM (drawable)),
        NULL);
    }
  else
    {
      gegl_node_set (
        private->fs_crop_node,
        "operation", "gegl:nop",
        NULL);
    }

  ligma_applicator_set_apply_offset (private->fs_applicator,
                                    fs_off_x - off_x,
                                    fs_off_y - off_y);

  if (ligma_channel_is_empty (mask))
    {
      ligma_applicator_set_mask_buffer (private->fs_applicator, NULL);
    }
  else
    {
      GeglBuffer *buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask));

      ligma_applicator_set_mask_buffer (private->fs_applicator, buffer);
      ligma_applicator_set_mask_offset (private->fs_applicator,
                                       -off_x, -off_y);
    }

  ligma_applicator_set_opacity (private->fs_applicator,
                               ligma_layer_get_opacity (fs));
  ligma_applicator_set_mode (private->fs_applicator,
                            ligma_layer_get_mode (fs),
                            ligma_layer_get_blend_space (fs),
                            ligma_layer_get_composite_space (fs),
                            ligma_layer_get_composite_mode (fs));
  ligma_applicator_set_affect (private->fs_applicator,
                              ligma_drawable_get_active_mask (drawable));
  ligma_applicator_set_output_format (private->fs_applicator,
                                     ligma_drawable_get_format (drawable));

  ligma_drawable_update_bounding_box (drawable);
}

static void
ligma_drawable_fs_notify (GObject          *object,
                         const GParamSpec *pspec,
                         LigmaDrawable     *drawable)
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
      ligma_drawable_sync_fs_filter (drawable);
    }
}

static void
ligma_drawable_fs_lock_position_changed (LigmaDrawable *signal_drawable,
                                        LigmaDrawable *drawable)
{
  LigmaLayer *fs = ligma_drawable_get_floating_sel (drawable);

  ligma_drawable_sync_fs_filter (drawable);

  ligma_drawable_update (LIGMA_DRAWABLE (fs), 0, 0, -1, -1);
}

static void
ligma_drawable_fs_format_changed (LigmaDrawable *signal_drawable,
                                 LigmaDrawable *drawable)
{
  LigmaLayer *fs = ligma_drawable_get_floating_sel (drawable);

  ligma_drawable_sync_fs_filter (drawable);

  ligma_drawable_update (LIGMA_DRAWABLE (fs), 0, 0, -1, -1);
}

static void
ligma_drawable_fs_affect_changed (LigmaImage       *image,
                                 LigmaChannelType  channel,
                                 LigmaDrawable    *drawable)
{
  LigmaLayer *fs = ligma_drawable_get_floating_sel (drawable);

  ligma_drawable_sync_fs_filter (drawable);

  ligma_drawable_update (LIGMA_DRAWABLE (fs), 0, 0, -1, -1);
}

static void
ligma_drawable_fs_mask_changed (LigmaImage    *image,
                               LigmaDrawable *drawable)
{
  LigmaLayer *fs = ligma_drawable_get_floating_sel (drawable);

  ligma_drawable_sync_fs_filter (drawable);

  ligma_drawable_update (LIGMA_DRAWABLE (fs), 0, 0, -1, -1);
}

static void
ligma_drawable_fs_visibility_changed (LigmaLayer    *fs,
                                     LigmaDrawable *drawable)
{
  if (ligma_layer_get_excludes_backdrop (fs))
    ligma_drawable_update (drawable, 0, 0, -1, -1);
  else
    ligma_drawable_update (LIGMA_DRAWABLE (fs), 0, 0, -1, -1);
}

static void
ligma_drawable_fs_excludes_backdrop_changed (LigmaLayer    *fs,
                                            LigmaDrawable *drawable)
{
  if (ligma_item_get_visible (LIGMA_ITEM (fs)))
    ligma_drawable_update (drawable, 0, 0, -1, -1);
}

static void
ligma_drawable_fs_bounding_box_changed (LigmaLayer    *fs,
                                       LigmaDrawable *drawable)
{
  ligma_drawable_update_bounding_box (drawable);
}

static void
ligma_drawable_fs_update (LigmaLayer    *fs,
                         gint          x,
                         gint          y,
                         gint          width,
                         gint          height,
                         LigmaDrawable *drawable)
{
  GeglRectangle bounding_box;
  GeglRectangle rect;
  gint          fs_off_x, fs_off_y;
  gint          off_x, off_y;

  ligma_item_get_offset (LIGMA_ITEM (fs), &fs_off_x, &fs_off_y);
  ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

  bounding_box = ligma_drawable_get_bounding_box (drawable);

  bounding_box.x += off_x;
  bounding_box.y += off_y;

  rect.x      = x + fs_off_x;
  rect.y      = y + fs_off_y;
  rect.width  = width;
  rect.height = height;

  if (gegl_rectangle_intersect (&rect, &rect, &bounding_box))
    {
      ligma_drawable_update (drawable,
                            rect.x - off_x, rect.y - off_y,
                            rect.width, rect.height);
    }
}
