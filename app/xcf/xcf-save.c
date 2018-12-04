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

#include <string.h>
#include <zlib.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core/core-types.h"

#include "gegl/gimp-babl-compat.h"
#include "gegl/gimp-gegl-tile-compat.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpchannel.h"
#include "core/gimpdrawable.h"
#include "core/gimpgrid.h"
#include "core/gimpguide.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"
#include "core/gimpimage-grid.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-metadata.h"
#include "core/gimpimage-private.h"
#include "core/gimpimage-sample-points.h"
#include "core/gimpimage-symmetry.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimpparasitelist.h"
#include "core/gimpprogress.h"
#include "core/gimpsamplepoint.h"
#include "core/gimpsymmetry.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "text/gimptextlayer.h"
#include "text/gimptextlayer-xcf.h"

#include "vectors/gimpanchor.h"
#include "vectors/gimpstroke.h"
#include "vectors/gimpbezierstroke.h"
#include "vectors/gimpvectors.h"
#include "vectors/gimpvectors-compat.h"

#include "xcf-private.h"
#include "xcf-read.h"
#include "xcf-save.h"
#include "xcf-seek.h"
#include "xcf-write.h"

#include "gimp-intl.h"


static gboolean xcf_save_image_props   (XcfInfo           *info,
                                        GimpImage         *image,
                                        GError           **error);
static gboolean xcf_save_layer_props   (XcfInfo           *info,
                                        GimpImage         *image,
                                        GimpLayer         *layer,
                                        GError           **error);
static gboolean xcf_save_channel_props (XcfInfo           *info,
                                        GimpImage         *image,
                                        GimpChannel       *channel,
                                        GError           **error);
static gboolean xcf_save_prop          (XcfInfo           *info,
                                        GimpImage         *image,
                                        PropType           prop_type,
                                        GError           **error,
                                        ...);
static gboolean xcf_save_layer         (XcfInfo           *info,
                                        GimpImage         *image,
                                        GimpLayer         *layer,
                                        GError           **error);
static gboolean xcf_save_channel       (XcfInfo           *info,
                                        GimpImage         *image,
                                        GimpChannel       *channel,
                                        GError           **error);
static gboolean xcf_save_buffer        (XcfInfo           *info,
                                        GeglBuffer        *buffer,
                                        GError           **error);
static gboolean xcf_save_level         (XcfInfo           *info,
                                        GeglBuffer        *buffer,
                                        GError           **error);
static gboolean xcf_save_tile          (XcfInfo           *info,
                                        GeglBuffer        *buffer,
                                        GeglRectangle     *tile_rect,
                                        const Babl        *format,
                                        GError           **error);
static gboolean xcf_save_tile_rle      (XcfInfo           *info,
                                        GeglBuffer        *buffer,
                                        GeglRectangle     *tile_rect,
                                        const Babl        *format,
                                        guchar            *rlebuf,
                                        GError           **error);
static gboolean xcf_save_tile_zlib     (XcfInfo           *info,
                                        GeglBuffer        *buffer,
                                        GeglRectangle     *tile_rect,
                                        const Babl        *format,
                                        GError           **error);
static gboolean xcf_save_parasite      (XcfInfo           *info,
                                        GimpParasite      *parasite,
                                        GError           **error);
static gboolean xcf_save_parasite_list (XcfInfo           *info,
                                        GimpParasiteList  *parasite,
                                        GError           **error);
static gboolean xcf_save_old_paths     (XcfInfo           *info,
                                        GimpImage         *image,
                                        GError           **error);
static gboolean xcf_save_vectors       (XcfInfo           *info,
                                        GimpImage         *image,
                                        GError           **error);


/* private convenience macros */
#define xcf_write_int32_check_error(info, data, count) G_STMT_START { \
  xcf_write_int32 (info, data, count, &tmp_error);                    \
  if (tmp_error)                                                      \
    {                                                                 \
      g_propagate_error (error, tmp_error);                           \
      return FALSE;                                                   \
    }                                                                 \
  } G_STMT_END

#define xcf_write_offset_check_error(info, data, count) G_STMT_START { \
  xcf_write_offset (info, data, count, &tmp_error);                    \
  if (tmp_error)                                                       \
    {                                                                  \
      g_propagate_error (error, tmp_error);                            \
      return FALSE;                                                    \
    }                                                                  \
  } G_STMT_END

#define xcf_write_zero_offset_check_error(info, count) G_STMT_START { \
  xcf_write_zero_offset (info, count, &tmp_error);                    \
  if (tmp_error)                                                      \
    {                                                                 \
      g_propagate_error (error, tmp_error);                           \
      return FALSE;                                                   \
    }                                                                 \
  } G_STMT_END

#define xcf_write_int8_check_error(info, data, count) G_STMT_START { \
  xcf_write_int8 (info, data, count, &tmp_error);                    \
  if (tmp_error)                                                     \
    {                                                                \
      g_propagate_error (error, tmp_error);                          \
      return FALSE;                                                  \
    }                                                                \
  } G_STMT_END

#define xcf_write_float_check_error(info, data, count) G_STMT_START { \
  xcf_write_float (info, data, count, &tmp_error);                    \
  if (tmp_error)                                                      \
    {                                                                 \
      g_propagate_error (error, tmp_error);                           \
      return FALSE;                                                   \
    }                                                                 \
  } G_STMT_END

#define xcf_write_string_check_error(info, data, count) G_STMT_START { \
  xcf_write_string (info, data, count, &tmp_error);                    \
  if (tmp_error)                                                       \
    {                                                                  \
      g_propagate_error (error, tmp_error);                            \
      return FALSE;                                                    \
    }                                                                  \
  } G_STMT_END

#define xcf_write_component_check_error(info, bpc, data, count) G_STMT_START { \
  xcf_write_component (info, bpc, data, count, &tmp_error);          \
  if (tmp_error)                                                     \
    {                                                                \
      g_propagate_error (error, tmp_error);                          \
      return FALSE;                                                  \
    }                                                                \
  } G_STMT_END

#define xcf_write_prop_type_check_error(info, prop_type) G_STMT_START { \
  guint32 _prop_int32 = prop_type;                                      \
  xcf_write_int32_check_error (info, &_prop_int32, 1);                  \
  } G_STMT_END

#define xcf_check_error(x) G_STMT_START { \
  if (! (x))                              \
    return FALSE;                         \
  } G_STMT_END

#define xcf_progress_update(info) G_STMT_START  \
  {                                             \
    progress++;                                 \
    if (info->progress)                         \
      gimp_progress_set_value (info->progress,  \
                               (gdouble) progress / (gdouble) max_progress); \
  } G_STMT_END


gboolean
xcf_save_image (XcfInfo    *info,
                GimpImage  *image,
                GError    **error)
{
  GList   *all_layers;
  GList   *all_channels;
  GList   *list;
  goffset  saved_pos;
  goffset  offset;
  guint32  value;
  guint    n_layers;
  guint    n_channels;
  guint    progress = 0;
  guint    max_progress;
  gchar    version_tag[16];
  GError  *tmp_error = NULL;

  /* write out the tag information for the image */
  if (info->file_version > 0)
    {
      g_snprintf (version_tag, sizeof (version_tag),
                  "gimp xcf v%03d", info->file_version);
    }
  else
    {
      strcpy (version_tag, "gimp xcf file");
    }

  xcf_write_int8_check_error (info, (guint8 *) version_tag, 14);

  /* write out the width, height and image type information for the image */
  value = gimp_image_get_width (image);
  xcf_write_int32_check_error (info, (guint32 *) &value, 1);

  value = gimp_image_get_height (image);
  xcf_write_int32_check_error (info, (guint32 *) &value, 1);

  value = gimp_image_get_base_type (image);
  xcf_write_int32_check_error (info, &value, 1);

  if (info->file_version >= 4)
    {
      value = gimp_image_get_precision (image);
      xcf_write_int32_check_error (info, &value, 1);
    }

  /* determine the number of layers and channels in the image */
  all_layers   = gimp_image_get_layer_list (image);
  all_channels = gimp_image_get_channel_list (image);

  /* check and see if we have to save out the selection */
  if (! gimp_channel_is_empty (gimp_image_get_mask (image)))
    {
      all_channels = g_list_append (all_channels, gimp_image_get_mask (image));
    }

  n_layers   = (guint) g_list_length (all_layers);
  n_channels = (guint) g_list_length (all_channels);

  max_progress = 1 + n_layers + n_channels;

  /* write the property information for the image */
  xcf_check_error (xcf_save_image_props (info, image, error));

  xcf_progress_update (info);

  /* 'saved_pos' is the next slot in the offset table */
  saved_pos = info->cp;

  /* write an empty offset table */
  xcf_write_zero_offset_check_error (info, n_layers + n_channels + 2);

  /* 'offset' is where we will write the next layer or channel */
  offset = info->cp;

  for (list = all_layers; list; list = g_list_next (list))
    {
      GimpLayer *layer = list->data;

      /* seek back to the next slot in the offset table and write the
       * offset of the layer
       */
      xcf_check_error (xcf_seek_pos (info, saved_pos, error));
      xcf_write_offset_check_error (info, &offset, 1);

      /* remember the next slot in the offset table */
      saved_pos = info->cp;

      /* seek to the layer offset and save the layer */
      xcf_check_error (xcf_seek_pos (info, offset, error));
      xcf_check_error (xcf_save_layer (info, image, layer, error));

      /* the next layer's offset is after the layer we just wrote */
      offset = info->cp;

      xcf_progress_update (info);
    }

  /* skip a '0' in the offset table to indicate the end of the layer
   * offsets
   */
  saved_pos += info->bytes_per_offset;

  for (list = all_channels; list; list = g_list_next (list))
    {
      GimpChannel *channel = list->data;

      /* seek back to the next slot in the offset table and write the
       * offset of the channel
       */
      xcf_check_error (xcf_seek_pos (info, saved_pos, error));
      xcf_write_offset_check_error (info, &offset, 1);

      /* remember the next slot in the offset table */
      saved_pos = info->cp;

      /* seek to the channel offset and save the channel */
      xcf_check_error (xcf_seek_pos (info, offset, error));
      xcf_check_error (xcf_save_channel (info, image, channel, error));

      /* the next channels's offset is after the channel we just wrote */
      offset = info->cp;

      xcf_progress_update (info);
    }

  /* there is already a '0' at the end of the offset table to indicate
   * the end of the channel offsets
   */

  g_list_free (all_layers);
  g_list_free (all_channels);

  return ! g_output_stream_is_closed (info->output);
}

static gboolean
xcf_save_image_props (XcfInfo    *info,
                      GimpImage  *image,
                      GError    **error)
{
  GimpImagePrivate *private       = GIMP_IMAGE_GET_PRIVATE (image);
  GimpParasite     *grid_parasite = NULL;
  GimpParasite     *meta_parasite = NULL;
  GList            *symmetry_parasites = NULL;
  GList            *iter;
  GimpUnit          unit          = gimp_image_get_unit (image);
  gdouble           xres;
  gdouble           yres;

  gimp_image_get_resolution (image, &xres, &yres);

  /* check and see if we should save the colormap property */
  if (gimp_image_get_colormap (image))
    xcf_check_error (xcf_save_prop (info, image, PROP_COLORMAP, error,
                                    gimp_image_get_colormap_size (image),
                                    gimp_image_get_colormap (image)));

  if (info->compression != COMPRESS_NONE)
    xcf_check_error (xcf_save_prop (info, image, PROP_COMPRESSION, error,
                                    info->compression));

  if (gimp_image_get_guides (image))
    xcf_check_error (xcf_save_prop (info, image, PROP_GUIDES, error,
                                    gimp_image_get_guides (image)));

  if (gimp_image_get_sample_points (image))
    {
      /* save the new property before the old one, so loading can skip
       * the latter
       */
      xcf_check_error (xcf_save_prop (info, image, PROP_SAMPLE_POINTS, error,
                                      gimp_image_get_sample_points (image)));
      xcf_check_error (xcf_save_prop (info, image, PROP_OLD_SAMPLE_POINTS, error,
                                      gimp_image_get_sample_points (image)));
    }

  xcf_check_error (xcf_save_prop (info, image, PROP_RESOLUTION, error,
                                  xres, yres));

  xcf_check_error (xcf_save_prop (info, image, PROP_TATTOO, error,
                                  gimp_image_get_tattoo_state (image)));

  if (unit < gimp_unit_get_number_of_built_in_units ())
    xcf_check_error (xcf_save_prop (info, image, PROP_UNIT, error, unit));

  if (gimp_container_get_n_children (gimp_image_get_vectors (image)) > 0)
    {
      if (gimp_vectors_compat_is_compatible (image))
        xcf_check_error (xcf_save_prop (info, image, PROP_PATHS, error));
      else
        xcf_check_error (xcf_save_prop (info, image, PROP_VECTORS, error));
    }

  if (unit >= gimp_unit_get_number_of_built_in_units ())
    xcf_check_error (xcf_save_prop (info, image, PROP_USER_UNIT, error, unit));

  if (gimp_image_get_grid (image))
    {
      GimpGrid *grid = gimp_image_get_grid (image);

      grid_parasite = gimp_grid_to_parasite (grid);
      gimp_parasite_list_add (private->parasites, grid_parasite);
    }

  if (gimp_image_get_metadata (image))
    {
      GimpMetadata *metadata = gimp_image_get_metadata (image);
      gchar        *meta_string;

      meta_string = gimp_metadata_serialize (metadata);

      if (meta_string)
        {
          meta_parasite = gimp_parasite_new ("gimp-image-metadata",
                                             GIMP_PARASITE_PERSISTENT,
                                             strlen (meta_string) + 1,
                                             meta_string);
          gimp_parasite_list_add (private->parasites, meta_parasite);
          g_free (meta_string);
        }
    }

  if (g_list_length (gimp_image_symmetry_get (image)))
    {
      GimpParasite *parasite  = NULL;
      GimpSymmetry *symmetry;

      for (iter = gimp_image_symmetry_get (image); iter; iter = g_list_next (iter))
        {
          symmetry = GIMP_SYMMETRY (iter->data);
          if (G_TYPE_FROM_INSTANCE (symmetry) == GIMP_TYPE_SYMMETRY)
            /* Do not save the identity symmetry. */
            continue;
          parasite = gimp_symmetry_to_parasite (GIMP_SYMMETRY (iter->data));
          gimp_parasite_list_add (private->parasites, parasite);
          symmetry_parasites = g_list_prepend (symmetry_parasites, parasite);
        }
    }

  if (gimp_parasite_list_length (private->parasites) > 0)
    {
      xcf_check_error (xcf_save_prop (info, image, PROP_PARASITES, error,
                                      private->parasites));
    }

  if (grid_parasite)
    {
      gimp_parasite_list_remove (private->parasites,
                                 gimp_parasite_name (grid_parasite));
      gimp_parasite_free (grid_parasite);
    }

  if (meta_parasite)
    {
      gimp_parasite_list_remove (private->parasites,
                                 gimp_parasite_name (meta_parasite));
      gimp_parasite_free (meta_parasite);
    }

  for (iter = symmetry_parasites; iter; iter = g_list_next (iter))
    {
      GimpParasite *parasite = iter->data;

      gimp_parasite_list_remove (private->parasites,
                                 gimp_parasite_name (parasite));
    }
  g_list_free_full (symmetry_parasites,
                    (GDestroyNotify) gimp_parasite_free);

  xcf_check_error (xcf_save_prop (info, image, PROP_END, error));

  return TRUE;
}

static gboolean
xcf_save_layer_props (XcfInfo    *info,
                      GimpImage  *image,
                      GimpLayer  *layer,
                      GError    **error)
{
  GimpParasiteList *parasites;
  gint              offset_x;
  gint              offset_y;

  if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
    xcf_check_error (xcf_save_prop (info, image, PROP_GROUP_ITEM, error));

  if (gimp_viewable_get_parent (GIMP_VIEWABLE (layer)))
    {
      GList *path;

      path = gimp_item_get_path (GIMP_ITEM (layer));
      xcf_check_error (xcf_save_prop (info, image, PROP_ITEM_PATH, error,
                                      path));
      g_list_free (path);
    }

  if (layer == gimp_image_get_active_layer (image))
    xcf_check_error (xcf_save_prop (info, image, PROP_ACTIVE_LAYER, error));

  if (layer == gimp_image_get_floating_selection (image))
    {
      info->floating_sel_drawable = gimp_layer_get_floating_sel_drawable (layer);
      xcf_check_error (xcf_save_prop (info, image, PROP_FLOATING_SELECTION,
                                      error));
    }

  xcf_check_error (xcf_save_prop (info, image, PROP_OPACITY, error,
                                  gimp_layer_get_opacity (layer)));
  xcf_check_error (xcf_save_prop (info, image, PROP_FLOAT_OPACITY, error,
                                  gimp_layer_get_opacity (layer)));
  xcf_check_error (xcf_save_prop (info, image, PROP_VISIBLE, error,
                                  gimp_item_get_visible (GIMP_ITEM (layer))));
  xcf_check_error (xcf_save_prop (info, image, PROP_LINKED, error,
                                  gimp_item_get_linked (GIMP_ITEM (layer))));
  xcf_check_error (xcf_save_prop (info, image, PROP_COLOR_TAG, error,
                                  gimp_item_get_color_tag (GIMP_ITEM (layer))));
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_CONTENT, error,
                                  gimp_item_get_lock_content (GIMP_ITEM (layer))));
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_ALPHA, error,
                                  gimp_layer_get_lock_alpha (layer)));
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_POSITION, error,
                                  gimp_item_get_lock_position (GIMP_ITEM (layer))));

  if (gimp_layer_get_mask (layer))
    {
      xcf_check_error (xcf_save_prop (info, image, PROP_APPLY_MASK, error,
                                      gimp_layer_get_apply_mask (layer)));
      xcf_check_error (xcf_save_prop (info, image, PROP_EDIT_MASK, error,
                                      gimp_layer_get_edit_mask (layer)));
      xcf_check_error (xcf_save_prop (info, image, PROP_SHOW_MASK, error,
                                      gimp_layer_get_show_mask (layer)));
    }
  else
    {
      xcf_check_error (xcf_save_prop (info, image, PROP_APPLY_MASK, error,
                                      FALSE));
      xcf_check_error (xcf_save_prop (info, image, PROP_EDIT_MASK, error,
                                      FALSE));
      xcf_check_error (xcf_save_prop (info, image, PROP_SHOW_MASK, error,
                                      FALSE));
    }

  gimp_item_get_offset (GIMP_ITEM (layer), &offset_x, &offset_y);

  xcf_check_error (xcf_save_prop (info, image, PROP_OFFSETS, error,
                                  offset_x, offset_y));
  xcf_check_error (xcf_save_prop (info, image, PROP_MODE, error,
                                  gimp_layer_get_mode (layer)));
  xcf_check_error (xcf_save_prop (info, image, PROP_BLEND_SPACE, error,
                                  gimp_layer_get_mode (layer),
                                  gimp_layer_get_blend_space (layer)));
  xcf_check_error (xcf_save_prop (info, image, PROP_COMPOSITE_SPACE, error,
                                  gimp_layer_get_mode (layer),
                                  gimp_layer_get_composite_space (layer)));
  xcf_check_error (xcf_save_prop (info, image, PROP_COMPOSITE_MODE, error,
                                  gimp_layer_get_mode (layer),
                                  gimp_layer_get_composite_mode (layer)));
  xcf_check_error (xcf_save_prop (info, image, PROP_TATTOO, error,
                                  gimp_item_get_tattoo (GIMP_ITEM (layer))));

  if (GIMP_IS_TEXT_LAYER (layer) && GIMP_TEXT_LAYER (layer)->text)
    {
      GimpTextLayer *text_layer = GIMP_TEXT_LAYER (layer);
      guint32        flags      = gimp_text_layer_get_xcf_flags (text_layer);

      gimp_text_layer_xcf_save_prepare (text_layer);

      if (flags)
        xcf_check_error (xcf_save_prop (info,
                                        image, PROP_TEXT_LAYER_FLAGS, error,
                                        flags));
    }

  if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
    {
      gint32 flags = 0;

      if (gimp_viewable_get_expanded (GIMP_VIEWABLE (layer)))
        flags |= XCF_GROUP_ITEM_EXPANDED;

      xcf_check_error (xcf_save_prop (info,
                                      image, PROP_GROUP_ITEM_FLAGS, error,
                                      flags));
    }

  parasites = gimp_item_get_parasites (GIMP_ITEM (layer));

  if (gimp_parasite_list_length (parasites) > 0)
    {
      xcf_check_error (xcf_save_prop (info, image, PROP_PARASITES, error,
                                      parasites));
    }

  xcf_check_error (xcf_save_prop (info, image, PROP_END, error));

  return TRUE;
}

static gboolean
xcf_save_channel_props (XcfInfo      *info,
                        GimpImage    *image,
                        GimpChannel  *channel,
                        GError      **error)
{
  GimpParasiteList *parasites;

  if (channel == gimp_image_get_active_channel (image))
    xcf_check_error (xcf_save_prop (info, image, PROP_ACTIVE_CHANNEL, error));

  if (channel == gimp_image_get_mask (image))
    xcf_check_error (xcf_save_prop (info, image, PROP_SELECTION, error));

  xcf_check_error (xcf_save_prop (info, image, PROP_OPACITY, error,
                                  gimp_channel_get_opacity (channel)));
  xcf_check_error (xcf_save_prop (info, image, PROP_FLOAT_OPACITY, error,
                                  gimp_channel_get_opacity (channel)));
  xcf_check_error (xcf_save_prop (info, image, PROP_VISIBLE, error,
                                  gimp_item_get_visible (GIMP_ITEM (channel))));
  xcf_check_error (xcf_save_prop (info, image, PROP_LINKED, error,
                                  gimp_item_get_linked (GIMP_ITEM (channel))));
  xcf_check_error (xcf_save_prop (info, image, PROP_COLOR_TAG, error,
                                  gimp_item_get_color_tag (GIMP_ITEM (channel))));
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_CONTENT, error,
                                  gimp_item_get_lock_content (GIMP_ITEM (channel))));
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_POSITION, error,
                                  gimp_item_get_lock_position (GIMP_ITEM (channel))));
  xcf_check_error (xcf_save_prop (info, image, PROP_SHOW_MASKED, error,
                                  gimp_channel_get_show_masked (channel)));
  xcf_check_error (xcf_save_prop (info, image, PROP_COLOR, error,
                                  &channel->color));
  xcf_check_error (xcf_save_prop (info, image, PROP_FLOAT_COLOR, error,
                                  &channel->color));
  xcf_check_error (xcf_save_prop (info, image, PROP_TATTOO, error,
                                  gimp_item_get_tattoo (GIMP_ITEM (channel))));

  parasites = gimp_item_get_parasites (GIMP_ITEM (channel));

  if (gimp_parasite_list_length (parasites) > 0)
    {
      xcf_check_error (xcf_save_prop (info, image, PROP_PARASITES, error,
                                      parasites));
    }

  xcf_check_error (xcf_save_prop (info, image, PROP_END, error));

  return TRUE;
}

static gboolean
xcf_save_prop (XcfInfo    *info,
               GimpImage  *image,
               PropType    prop_type,
               GError    **error,
               ...)
{
  guint32  size;
  va_list  args;
  GError  *tmp_error = NULL;

  va_start (args, error);

  switch (prop_type)
    {
    case PROP_END:
      size = 0;

      xcf_write_prop_type_check_error (info, prop_type);
      xcf_write_int32_check_error (info, &size, 1);
      break;

    case PROP_COLORMAP:
      {
        guint32  n_colors = va_arg (args, guint32);
        guchar  *colors   = va_arg (args, guchar *);

        size = 4 + n_colors * 3;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, &n_colors, 1);
        xcf_write_int8_check_error  (info, colors,    n_colors * 3);
      }
      break;

    case PROP_ACTIVE_LAYER:
    case PROP_ACTIVE_CHANNEL:
    case PROP_SELECTION:
    case PROP_GROUP_ITEM:
      size = 0;

      xcf_write_prop_type_check_error (info, prop_type);
      xcf_write_int32_check_error (info, &size, 1);
      break;

    case PROP_FLOATING_SELECTION:
      size = info->bytes_per_offset;

      xcf_write_prop_type_check_error (info, prop_type);
      xcf_write_int32_check_error (info, &size, 1);

      info->floating_sel_offset = info->cp;
      xcf_write_zero_offset_check_error (info, 1);
      break;

    case PROP_OPACITY:
      {
        gdouble opacity      = va_arg (args, gdouble);
        guint32 uint_opacity = opacity * 255.999;

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, &uint_opacity, 1);
      }
      break;

    case PROP_FLOAT_OPACITY:
      {
        gfloat opacity = va_arg (args, gdouble);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_float_check_error (info, &opacity, 1);
      }
      break;

    case PROP_MODE:
      {
        gint32 mode = va_arg (args, gint32);

        size = 4;

        if (mode == GIMP_LAYER_MODE_OVERLAY_LEGACY)
          mode = GIMP_LAYER_MODE_SOFTLIGHT_LEGACY;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, (guint32 *) &mode, 1);
      }
      break;

    case PROP_BLEND_SPACE:
      {
        GimpLayerMode mode        = va_arg (args, GimpLayerMode);
        gint32        blend_space = va_arg (args, gint32);

        G_STATIC_ASSERT (GIMP_LAYER_COLOR_SPACE_AUTO == 0);

        /* if blend_space is AUTO, save the negative of the actual value AUTO
         * maps to for the given mode, so that we can correctly load it even if
         * the mapping changes in the future.
         */
        if (blend_space == GIMP_LAYER_COLOR_SPACE_AUTO)
          blend_space = -gimp_layer_mode_get_blend_space (mode);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, (guint32 *) &blend_space, 1);
      }
      break;

    case PROP_COMPOSITE_SPACE:
      {
        GimpLayerMode mode            = va_arg (args, GimpLayerMode);
        gint32        composite_space = va_arg (args, gint32);

        G_STATIC_ASSERT (GIMP_LAYER_COLOR_SPACE_AUTO == 0);

        /* if composite_space is AUTO, save the negative of the actual value
         * AUTO maps to for the given mode, so that we can correctly load it
         * even if the mapping changes in the future.
         */
        if (composite_space == GIMP_LAYER_COLOR_SPACE_AUTO)
          composite_space = -gimp_layer_mode_get_composite_space (mode);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, (guint32 *) &composite_space, 1);
      }
      break;

    case PROP_COMPOSITE_MODE:
      {
        GimpLayerMode mode           = va_arg (args, GimpLayerMode);
        gint32        composite_mode = va_arg (args, gint32);

        G_STATIC_ASSERT (GIMP_LAYER_COMPOSITE_AUTO == 0);

        /* if composite_mode is AUTO, save the negative of the actual value
         * AUTO maps to for the given mode, so that we can correctly load it
         * even if the mapping changes in the future.
         */
        if (composite_mode == GIMP_LAYER_COMPOSITE_AUTO)
          composite_mode = -gimp_layer_mode_get_composite_mode (mode);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, (guint32 *) &composite_mode, 1);
      }
      break;

    case PROP_VISIBLE:
      {
        guint32 visible = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, &visible, 1);
      }
      break;

    case PROP_LINKED:
      {
        guint32 linked = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, &linked, 1);
      }
      break;

    case PROP_COLOR_TAG:
      {
        guint32 color_tag = va_arg (args, gint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, &color_tag, 1);
      }
      break;

    case PROP_LOCK_CONTENT:
      {
        guint32 lock_content = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, &lock_content, 1);
      }
      break;

    case PROP_LOCK_ALPHA:
      {
        guint32 lock_alpha = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, &lock_alpha, 1);
      }
      break;

    case PROP_LOCK_POSITION:
      {
        guint32 lock_position = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, &lock_position, 1);
      }
      break;

    case PROP_APPLY_MASK:
      {
        guint32 apply_mask = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, &apply_mask, 1);
      }
      break;

    case PROP_EDIT_MASK:
      {
        guint32 edit_mask = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, &edit_mask, 1);
      }
      break;

    case PROP_SHOW_MASK:
      {
        guint32 show_mask = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, &show_mask, 1);
      }
      break;

    case PROP_SHOW_MASKED:
      {
        guint32 show_masked = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, &show_masked, 1);
      }
      break;

    case PROP_OFFSETS:
      {
        gint32 offsets[2];

        offsets[0] = va_arg (args, gint32);
        offsets[1] = va_arg (args, gint32);

        size = 8;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, (guint32 *) offsets, 2);
      }
      break;

    case PROP_COLOR:
      {
        GimpRGB *color = va_arg (args, GimpRGB *);
        guchar   col[3];

        gimp_rgb_get_uchar (color, &col[0], &col[1], &col[2]);

        size = 3;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int8_check_error (info, col, 3);
      }
      break;

    case PROP_FLOAT_COLOR:
      {
        GimpRGB *color = va_arg (args, GimpRGB *);
        gfloat   col[3];

        col[0] = color->r;
        col[1] = color->g;
        col[2] = color->b;

        size = 3 * 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_float_check_error (info, col, 3);
      }
      break;

    case PROP_COMPRESSION:
      {
        guint8 compression = (guint8) va_arg (args, guint32);

        size = 1;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int8_check_error (info, &compression, 1);
      }
      break;

    case PROP_GUIDES:
      {
        GList *guides   = va_arg (args, GList *);
        gint   n_guides = g_list_length (guides);
        GList *iter;

        for (iter = guides; iter; iter = g_list_next (iter))
          {
            /* Do not save custom guides. */
            if (gimp_guide_is_custom (GIMP_GUIDE (iter->data)))
              n_guides--;
          }

        if (n_guides > 0)
          {
            size = n_guides * (4 + 1);

            xcf_write_prop_type_check_error (info, prop_type);
            xcf_write_int32_check_error (info, &size, 1);

            for (; guides; guides = g_list_next (guides))
              {
                GimpGuide *guide    = guides->data;
                gint32     position = gimp_guide_get_position (guide);
                gint8      orientation;

                if (gimp_guide_is_custom (guide))
                  continue;

                switch (gimp_guide_get_orientation (guide))
                  {
                  case GIMP_ORIENTATION_HORIZONTAL:
                    orientation = XCF_ORIENTATION_HORIZONTAL;
                    break;

                  case GIMP_ORIENTATION_VERTICAL:
                    orientation = XCF_ORIENTATION_VERTICAL;
                    break;

                  default:
                    g_warning ("%s: skipping guide with bad orientation",
                               G_STRFUNC);
                    continue;
                  }

                xcf_write_int32_check_error (info, (guint32 *) &position,    1);
                xcf_write_int8_check_error  (info, (guint8 *)  &orientation, 1);
              }
          }
      }
      break;

    case PROP_SAMPLE_POINTS:
      {
        GList *sample_points   = va_arg (args, GList *);
        gint   n_sample_points = g_list_length (sample_points);

        size = n_sample_points * (5 * 4);

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        for (; sample_points; sample_points = g_list_next (sample_points))
          {
            GimpSamplePoint   *sample_point = sample_points->data;
            gint32             x, y;
            GimpColorPickMode  pick_mode;
            guint32            padding[2] = { 0, };

            gimp_sample_point_get_position (sample_point, &x, &y);
            pick_mode = gimp_sample_point_get_pick_mode (sample_point);

            xcf_write_int32_check_error (info, (guint32 *) &x,         1);
            xcf_write_int32_check_error (info, (guint32 *) &y,         1);
            xcf_write_int32_check_error (info, (guint32 *) &pick_mode, 1);
            xcf_write_int32_check_error (info, (guint32 *) padding,    2);
          }
      }
      break;

    case PROP_OLD_SAMPLE_POINTS:
      {
        GList *sample_points   = va_arg (args, GList *);
        gint   n_sample_points = g_list_length (sample_points);

        size = n_sample_points * (4 + 4);

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        for (; sample_points; sample_points = g_list_next (sample_points))
          {
            GimpSamplePoint *sample_point = sample_points->data;
            gint32           x, y;

            gimp_sample_point_get_position (sample_point, &x, &y);

            xcf_write_int32_check_error (info, (guint32 *) &x, 1);
            xcf_write_int32_check_error (info, (guint32 *) &y, 1);
          }
      }
      break;

    case PROP_RESOLUTION:
      {
        gfloat resolution[2];

        resolution[0] = va_arg (args, double);
        resolution[1] = va_arg (args, double);

        size = 2 * 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_float_check_error (info, resolution, 2);
      }
      break;

    case PROP_TATTOO:
      {
        guint32 tattoo = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, &tattoo, 1);
      }
      break;

    case PROP_PARASITES:
      {
        GimpParasiteList *list = va_arg (args, GimpParasiteList *);

        if (gimp_parasite_list_persistent_length (list) > 0)
          {
            goffset base;
            goffset pos;

            size = 0;

            xcf_write_prop_type_check_error (info, prop_type);

            /* because we don't know how much room the parasite list
             * will take we save the file position and write the
             * length later
             */
            pos = info->cp;
            xcf_write_int32_check_error (info, &size, 1);

            base = info->cp;

            xcf_check_error (xcf_save_parasite_list (info, list, error));

            size = info->cp - base;

            /* go back to the saved position and write the length */
            xcf_check_error (xcf_seek_pos (info, pos, error));
            xcf_write_int32_check_error (info, &size, 1);

            xcf_check_error (xcf_seek_pos (info, base + size, error));
          }
      }
      break;

    case PROP_UNIT:
      {
        guint32 unit = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, &unit, 1);
      }
      break;

    case PROP_PATHS:
      {
        goffset base;
        goffset pos;

        size = 0;

        xcf_write_prop_type_check_error (info, prop_type);

        /* because we don't know how much room the paths list will
         * take we save the file position and write the length later
         */
        pos = info->cp;
        xcf_write_int32_check_error (info, &size, 1);

        base = info->cp;

        xcf_check_error (xcf_save_old_paths (info, image, error));

        size = info->cp - base;

        /* go back to the saved position and write the length */
        xcf_check_error (xcf_seek_pos (info, pos, error));
        xcf_write_int32_check_error (info, &size, 1);

        xcf_check_error (xcf_seek_pos (info, base + size, error));
      }
      break;

    case PROP_USER_UNIT:
      {
        GimpUnit     unit = va_arg (args, guint32);
        const gchar *unit_strings[5];
        gfloat       factor;
        guint32      digits;

        /* write the entire unit definition */
        unit_strings[0] = gimp_unit_get_identifier (unit);
        factor          = gimp_unit_get_factor (unit);
        digits          = gimp_unit_get_digits (unit);
        unit_strings[1] = gimp_unit_get_symbol (unit);
        unit_strings[2] = gimp_unit_get_abbreviation (unit);
        unit_strings[3] = gimp_unit_get_singular (unit);
        unit_strings[4] = gimp_unit_get_plural (unit);

        size =
          2 * 4 +
          strlen (unit_strings[0]) ? strlen (unit_strings[0]) + 5 : 4 +
          strlen (unit_strings[1]) ? strlen (unit_strings[1]) + 5 : 4 +
          strlen (unit_strings[2]) ? strlen (unit_strings[2]) + 5 : 4 +
          strlen (unit_strings[3]) ? strlen (unit_strings[3]) + 5 : 4 +
          strlen (unit_strings[4]) ? strlen (unit_strings[4]) + 5 : 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_float_check_error  (info, &factor,                 1);
        xcf_write_int32_check_error  (info, &digits,                 1);
        xcf_write_string_check_error (info, (gchar **) unit_strings, 5);
      }
      break;

    case PROP_VECTORS:
      {
        goffset base;
        goffset pos;

        size = 0;

        xcf_write_prop_type_check_error (info, prop_type);

        /* because we don't know how much room the paths list will
         * take we save the file position and write the length later
         */
        pos = info->cp;
        xcf_write_int32_check_error (info, &size, 1);

        base = info->cp;

        xcf_check_error (xcf_save_vectors (info, image, error));

        size = info->cp - base;

        /* go back to the saved position and write the length */
        xcf_check_error (xcf_seek_pos (info, pos, error));
        xcf_write_int32_check_error (info, &size, 1);

        xcf_check_error (xcf_seek_pos (info, base + size, error));
      }
      break;

    case PROP_TEXT_LAYER_FLAGS:
      {
        guint32 flags = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_int32_check_error (info, &flags, 1);
      }
      break;

    case PROP_ITEM_PATH:
      {
        GList *path = va_arg (args, GList *);

        size = 4 * g_list_length (path);

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        while (path)
          {
            guint32 index = GPOINTER_TO_UINT (path->data);

            xcf_write_int32_check_error (info, &index, 1);

            path = g_list_next (path);
          }
      }
      break;

    case PROP_GROUP_ITEM_FLAGS:
      {
        guint32 flags = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int32_check_error (info, &flags, 1);
      }
      break;
    }

  va_end (args);

  return TRUE;
}

static gboolean
xcf_save_layer (XcfInfo    *info,
                GimpImage  *image,
                GimpLayer  *layer,
                GError    **error)
{
  goffset      saved_pos;
  goffset      offset;
  guint32      value;
  const gchar *string;
  GError      *tmp_error = NULL;

  /* check and see if this is the drawable that the floating
   *  selection is attached to.
   */
  if (GIMP_DRAWABLE (layer) == info->floating_sel_drawable)
    {
      saved_pos = info->cp;
      xcf_check_error (xcf_seek_pos (info, info->floating_sel_offset, error));
      xcf_write_offset_check_error (info, &saved_pos, 1);
      xcf_check_error (xcf_seek_pos (info, saved_pos, error));
    }

  /* write out the width, height and image type information for the layer */
  value = gimp_item_get_width (GIMP_ITEM (layer));
  xcf_write_int32_check_error (info, &value, 1);

  value = gimp_item_get_height (GIMP_ITEM (layer));
  xcf_write_int32_check_error (info, &value, 1);

  value = gimp_babl_format_get_image_type (gimp_drawable_get_format (GIMP_DRAWABLE (layer)));
  xcf_write_int32_check_error (info, &value, 1);

  /* write out the layers name */
  string = gimp_object_get_name (layer);
  xcf_write_string_check_error (info, (gchar **) &string, 1);

  /* write out the layer properties */
  xcf_save_layer_props (info, image, layer, error);

  /* write out the layer tile hierarchy */
  offset = info->cp + 2 * info->bytes_per_offset;
  xcf_write_offset_check_error (info, &offset, 1);

  saved_pos = info->cp;

  /* write a zero layer mask offset */
  xcf_write_zero_offset_check_error (info, 1);

  xcf_check_error (xcf_save_buffer (info,
                                    gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)),
                                    error));

  offset = info->cp;

  /* write out the layer mask */
  if (gimp_layer_get_mask (layer))
    {
      GimpLayerMask *mask = gimp_layer_get_mask (layer);

      xcf_check_error (xcf_seek_pos (info, saved_pos, error));
      xcf_write_offset_check_error (info, &offset, 1);

      xcf_check_error (xcf_seek_pos (info, offset, error));
      xcf_check_error (xcf_save_channel (info, image, GIMP_CHANNEL (mask),
                                         error));
    }

  return TRUE;
}

static gboolean
xcf_save_channel (XcfInfo      *info,
                  GimpImage    *image,
                  GimpChannel  *channel,
                  GError      **error)
{
  goffset      saved_pos;
  goffset      offset;
  guint32      value;
  const gchar *string;
  GError      *tmp_error = NULL;

  /* check and see if this is the drawable that the floating
   *  selection is attached to.
   */
  if (GIMP_DRAWABLE (channel) == info->floating_sel_drawable)
    {
      saved_pos = info->cp;
      xcf_check_error (xcf_seek_pos (info, info->floating_sel_offset, error));
      xcf_write_offset_check_error (info, &saved_pos, 1);
      xcf_check_error (xcf_seek_pos (info, saved_pos, error));
    }

  /* write out the width and height information for the channel */
  value = gimp_item_get_width (GIMP_ITEM (channel));
  xcf_write_int32_check_error (info, &value, 1);

  value = gimp_item_get_height (GIMP_ITEM (channel));
  xcf_write_int32_check_error (info, &value, 1);

  /* write out the channels name */
  string = gimp_object_get_name (channel);
  xcf_write_string_check_error (info, (gchar **) &string, 1);

  /* write out the channel properties */
  xcf_save_channel_props (info, image, channel, error);

  /* write out the channel tile hierarchy */
  offset = info->cp + info->bytes_per_offset;
  xcf_write_offset_check_error (info, &offset, 1);

  xcf_check_error (xcf_save_buffer (info,
                                    gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                                    error));

  return TRUE;
}

static gint
xcf_calc_levels (gint size,
                 gint tile_size)
{
  gint levels;

  levels = 1;
  while (size > tile_size)
    {
      size /= 2;
      levels += 1;
    }

  return levels;
}


static gboolean
xcf_save_buffer (XcfInfo     *info,
                 GeglBuffer  *buffer,
                 GError     **error)
{
  const Babl *format;
  goffset     saved_pos;
  goffset     offset;
  guint32     width;
  guint32     height;
  guint32     bpp;
  gint        i;
  gint        nlevels;
  gint        tmp1, tmp2;
  GError     *tmp_error = NULL;

  format = gegl_buffer_get_format (buffer);

  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);
  bpp    = babl_format_get_bytes_per_pixel (format);

  xcf_write_int32_check_error (info, (guint32 *) &width,  1);
  xcf_write_int32_check_error (info, (guint32 *) &height, 1);
  xcf_write_int32_check_error (info, (guint32 *) &bpp,    1);

  tmp1 = xcf_calc_levels (width,  XCF_TILE_WIDTH);
  tmp2 = xcf_calc_levels (height, XCF_TILE_HEIGHT);
  nlevels = MAX (tmp1, tmp2);

  /* 'saved_pos' is the next slot in the offset table */
  saved_pos = info->cp;

  /* write an empty offset table */
  xcf_write_zero_offset_check_error (info, nlevels + 1);

  /* 'offset' is where we will write the next level */
  offset = info->cp;

  for (i = 0; i < nlevels; i++)
    {
      /* seek back to the next slot in the offset table and write the
       * offset of the level
       */
      xcf_check_error (xcf_seek_pos (info, saved_pos, error));
      xcf_write_offset_check_error (info, &offset, 1);

      /* remember the next slot in the offset table */
      saved_pos = info->cp;

      /* seek to the level offset and save the level */
      xcf_check_error (xcf_seek_pos (info, offset, error));

      if (i == 0)
        {
          /* write out the level. */
          xcf_check_error (xcf_save_level (info, buffer, error));
        }
      else
        {
          /* fake an empty level */
          tmp1 = 0;
          width  /= 2;
          height /= 2;
          xcf_write_int32_check_error (info, (guint32 *) &width,  1);
          xcf_write_int32_check_error (info, (guint32 *) &height, 1);

          /* NOTE:  this should be an offset, not an int32!  however...
           * since there are already 64-bit-offsets XCFs out there in
           * which this field is 32-bit, and since it's not actually
           * being used, we're going to keep this field 32-bit for the
           * dummy levels, to remain consistent.  if we ever make use
           * of levels above the first, we should turn this field into
           * an offset, and bump the xcf version.
           */
          xcf_write_int32_check_error (info, (guint32 *) &tmp1,   1);
        }

      /* the next level's offset if after the level we just wrote */
      offset = info->cp;
    }

  /* there is already a '0' at the end of the offset table to indicate
   * the end of the level offsets
   */

  return TRUE;
}

static gboolean
xcf_save_level (XcfInfo     *info,
                GeglBuffer  *buffer,
                GError     **error)
{
  const Babl *format;
  goffset    *offset_table;
  goffset    *next_offset;
  goffset     saved_pos;
  goffset     offset;
  goffset     max_data_length;
  guint32     width;
  guint32     height;
  gint        bpp;
  gint        n_tile_rows;
  gint        n_tile_cols;
  guint       ntiles;
  gint        i;
  guchar     *rlebuf    = NULL;
  GError     *tmp_error = NULL;

  format = gegl_buffer_get_format (buffer);

  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);
  bpp    = babl_format_get_bytes_per_pixel (format);

  xcf_write_int32_check_error (info, (guint32 *) &width,  1);
  xcf_write_int32_check_error (info, (guint32 *) &height, 1);

  saved_pos = info->cp;

  /* maximal allowable size of on-disk tile data.  make it somewhat bigger than
   * the uncompressed tile size, to allow for the possibility of negative
   * compression.  xcf_load_level() enforces this limit.
   */
  max_data_length = XCF_TILE_WIDTH * XCF_TILE_HEIGHT * bpp *
                    XCF_TILE_MAX_DATA_LENGTH_FACTOR /* = 1.5, currently */;

  /* allocate a temporary buffer to store the rle data before it is
   * written to disk
   */
  if (info->compression == COMPRESS_RLE)
    rlebuf = g_alloca (max_data_length);

  n_tile_rows = gimp_gegl_buffer_get_n_tile_rows (buffer, XCF_TILE_HEIGHT);
  n_tile_cols = gimp_gegl_buffer_get_n_tile_cols (buffer, XCF_TILE_WIDTH);

  ntiles = n_tile_rows * n_tile_cols;

  /* allocate an offset table so we don't have to seek back after each
   * tile, see bug #686862. allocate ntiles + 1 slots because a zero
   * offset indicates the offset table's end.
   */
  offset_table = g_alloca ((ntiles + 1) * sizeof (goffset));
  memset (offset_table, 0, (ntiles + 1) * sizeof (goffset));
  next_offset = offset_table;

  /* 'saved_pos' is the offset of the tile offset table  */
  saved_pos = info->cp;

  /* write an empty offset table */
  xcf_write_zero_offset_check_error (info, ntiles + 1);

  /* 'offset' is where we will write the next tile */
  offset = info->cp;

  for (i = 0; i < ntiles; i++)
    {
      GeglRectangle rect;

      /* store the offset in the table and increment the next pointer */
      *next_offset++ = offset;

      gimp_gegl_buffer_get_tile_rect (buffer,
                                      XCF_TILE_WIDTH, XCF_TILE_HEIGHT,
                                      i, &rect);

      /* write out the tile. */
      switch (info->compression)
        {
        case COMPRESS_NONE:
          xcf_check_error (xcf_save_tile (info, buffer, &rect, format,
                                          error));
          break;
        case COMPRESS_RLE:
          xcf_check_error (xcf_save_tile_rle (info, buffer, &rect, format,
                                              rlebuf, error));
          break;
        case COMPRESS_ZLIB:
          xcf_check_error (xcf_save_tile_zlib (info, buffer, &rect, format,
                                               error));
          break;
        case COMPRESS_FRACTAL:
          g_warning ("xcf: fractal compression unimplemented");
          return FALSE;
        }

      /* make sure the on-disk tile data didn't end up being too big.
       * xcf_load_level() would refuse to load the file if it did.
       */
      if (info->cp < offset || info->cp - offset > max_data_length)
        {
          g_message ("xcf: invalid tile data length: %" G_GOFFSET_FORMAT,
                     info->cp - offset);
          return FALSE;
        }

      /* the next tile's offset is after the tile we just wrote */
      offset = info->cp;
    }

  /* seek back to the offset table and write it  */
  xcf_check_error (xcf_seek_pos (info, saved_pos, error));
  xcf_write_offset_check_error (info, offset_table, ntiles + 1);

  /* seek to the end of the file */
  xcf_check_error (xcf_seek_pos (info, offset, error));

  return TRUE;
}

static gboolean
xcf_save_tile (XcfInfo        *info,
               GeglBuffer     *buffer,
               GeglRectangle  *tile_rect,
               const Babl     *format,
               GError        **error)
{
  gint    bpp       = babl_format_get_bytes_per_pixel (format);
  gint    tile_size = bpp * tile_rect->width * tile_rect->height;
  guchar *tile_data = g_alloca (tile_size);
  GError *tmp_error = NULL;

  gegl_buffer_get (buffer, tile_rect, 1.0, format, tile_data,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if (info->file_version <= 11)
    {
      xcf_write_int8_check_error (info, tile_data, tile_size);
    }
  else
    {
      gint n_components = babl_format_get_n_components (format);

      xcf_write_component_check_error (info, bpp / n_components, tile_data,
                                       tile_size / bpp * n_components);
    }

  return TRUE;
}

static gboolean
xcf_save_tile_rle (XcfInfo        *info,
                   GeglBuffer     *buffer,
                   GeglRectangle  *tile_rect,
                   const Babl     *format,
                   guchar         *rlebuf,
                   GError        **error)
{
  gint    bpp       = babl_format_get_bytes_per_pixel (format);
  gint    tile_size = bpp * tile_rect->width * tile_rect->height;
  guchar *tile_data = g_alloca (tile_size);
  gint    len       = 0;
  gint    i, j;
  GError *tmp_error  = NULL;

  gegl_buffer_get (buffer, tile_rect, 1.0, format, tile_data,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if (info->file_version >= 12)
    {
      gint n_components = babl_format_get_n_components (format);

      xcf_write_to_be (bpp / n_components, tile_data,
                       tile_size / bpp * n_components);
    }

  for (i = 0; i < bpp; i++)
    {
      const guchar *data   = tile_data + i;
      gint          state  = 0;
      gint          length = 0;
      gint          count  = 0;
      gint          size   = tile_rect->width * tile_rect->height;
      guint         last   = -1;

      while (size > 0)
        {
          switch (state)
            {
            case 0:
              /* in state 0 we try to find a long sequence of
               *  matching values.
               */
              if ((length == 32768) ||
                  ((size - length) <= 0) ||
                  ((length > 1) && (last != *data)))
                {
                  count += length;

                  if (length >= 128)
                    {
                      rlebuf[len++] = 127;
                      rlebuf[len++] = (length >> 8);
                      rlebuf[len++] = length & 0x00FF;
                      rlebuf[len++] = last;
                    }
                  else
                    {
                      rlebuf[len++] = length - 1;
                      rlebuf[len++] = last;
                    }

                  size -= length;
                  length = 0;
                }
              else if ((length == 1) && (last != *data))
                {
                  state = 1;
                }
              break;

            case 1:
              /* in state 1 we try and find a long sequence of
               *  non-matching values.
               */
              if ((length == 32768) ||
                  ((size - length) == 0) ||
                  ((length > 0) && (last == *data) &&
                   ((size - length) == 1 || last == data[bpp])))
                {
                  const guchar *t;

                  /* if we came here because of a new run, backup one */
                  if (!((length == 32768) || ((size - length) == 0)))
                    {
                      length--;
                      data -= bpp;
                    }

                  count += length;
                  state = 0;

                  if (length >= 128)
                    {
                      rlebuf[len++] = 255 - 127;
                      rlebuf[len++] = (length >> 8);
                      rlebuf[len++] = length & 0x00FF;
                    }
                  else
                    {
                      rlebuf[len++] = 255 - (length - 1);
                    }

                  t = data - length * bpp;

                  for (j = 0; j < length; j++)
                    {
                      rlebuf[len++] = *t;
                      t += bpp;
                    }

                  size -= length;
                  length = 0;
                }
              break;
            }

          if (size > 0)
            {
              length += 1;
              last = *data;
              data += bpp;
            }
        }

      if (count != (tile_rect->width * tile_rect->height))
        g_message ("xcf: uh oh! xcf rle tile saving error: %d", count);
    }

  xcf_write_int8_check_error (info, rlebuf, len);

  return TRUE;
}

static gboolean
xcf_save_tile_zlib (XcfInfo        *info,
                    GeglBuffer     *buffer,
                    GeglRectangle  *tile_rect,
                    const Babl     *format,
                    GError        **error)
{
  gint      bpp       = babl_format_get_bytes_per_pixel (format);
  gint      tile_size = bpp * tile_rect->width * tile_rect->height;
  guchar   *tile_data = g_alloca (tile_size);
  /* The buffer for compressed data. */
  guchar   *buf       = g_alloca (tile_size);
  GError   *tmp_error = NULL;
  z_stream  strm;
  int       action;
  int       status;

  gegl_buffer_get (buffer, tile_rect, 1.0, format, tile_data,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if (info->file_version >= 12)
    {
      gint n_components = babl_format_get_n_components (format);

      xcf_write_to_be (bpp / n_components, tile_data,
                       tile_size / bpp * n_components);
    }

  /* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree  = Z_NULL;
  strm.opaque = Z_NULL;

  status = deflateInit (&strm, Z_DEFAULT_COMPRESSION);
  if (status != Z_OK)
    return FALSE;

  strm.next_in   = tile_data;
  strm.avail_in  = tile_size;
  strm.next_out  = buf;
  strm.avail_out = tile_size;

  action = Z_NO_FLUSH;

  while (status == Z_OK || status == Z_BUF_ERROR)
    {
      if (strm.avail_in == 0)
        {
          /* Finish the encoding. */
          action = Z_FINISH;
        }

      status = deflate (&strm, action);

      if (status == Z_STREAM_END || status == Z_BUF_ERROR)
        {
          size_t write_size = tile_size - strm.avail_out;

          xcf_write_int8_check_error (info, buf, write_size);

          /* Reset next_out and avail_out. */
          strm.next_out  = buf;
          strm.avail_out = tile_size;
        }
      else if (status != Z_OK)
        {
          g_printerr ("xcf: tile compression failed: %s", zError (status));
          deflateEnd (&strm);
          return FALSE;
        }
    }

  deflateEnd (&strm);
  return TRUE;
}

static gboolean
xcf_save_parasite (XcfInfo       *info,
                   GimpParasite  *parasite,
                   GError       **error)
{
  if (gimp_parasite_is_persistent (parasite))
    {
      guint32      value;
      const gchar *string;
      GError      *tmp_error = NULL;

      string = gimp_parasite_name (parasite);
      xcf_write_string_check_error (info, (gchar **) &string, 1);

      value = gimp_parasite_flags (parasite);
      xcf_write_int32_check_error (info, &value, 1);

      value = gimp_parasite_data_size (parasite);
      xcf_write_int32_check_error (info, &value, 1);

      xcf_write_int8_check_error (info,
                                  gimp_parasite_data (parasite),
                                  gimp_parasite_data_size (parasite));
    }

  return TRUE;
}

typedef struct
{
  XcfInfo *info;
  GError  *error;
} XcfParasiteData;

static void
xcf_save_parasite_func (gchar           *key,
                        GimpParasite    *parasite,
                        XcfParasiteData *data)
{
  if (! data->error)
    xcf_save_parasite (data->info, parasite, &data->error);
}

static gboolean
xcf_save_parasite_list (XcfInfo           *info,
                        GimpParasiteList  *list,
                        GError           **error)
{
  XcfParasiteData data;

  data.info  = info;
  data.error = NULL;

  gimp_parasite_list_foreach (list, (GHFunc) xcf_save_parasite_func, &data);

  if (data.error)
    {
      g_propagate_error (error, data.error);
      return FALSE;
    }

  return TRUE;
}

static gboolean
xcf_save_old_paths (XcfInfo    *info,
                    GimpImage  *image,
                    GError    **error)
{
  GimpVectors *active_vectors;
  guint32      num_paths;
  guint32      active_index = 0;
  GList       *list;
  GError      *tmp_error = NULL;

  /* Write out the following:-
   *
   * last_selected_row (gint)
   * number_of_paths (gint)
   *
   * then each path:-
   */

  num_paths = gimp_container_get_n_children (gimp_image_get_vectors (image));

  active_vectors = gimp_image_get_active_vectors (image);

  if (active_vectors)
    active_index = gimp_container_get_child_index (gimp_image_get_vectors (image),
                                                   GIMP_OBJECT (active_vectors));

  xcf_write_int32_check_error (info, &active_index, 1);
  xcf_write_int32_check_error (info, &num_paths,    1);

  for (list = gimp_image_get_vectors_iter (image);
       list;
       list = g_list_next (list))
    {
      GimpVectors            *vectors = list->data;
      gchar                  *name;
      guint32                 locked;
      guint8                  state;
      guint32                 version;
      guint32                 pathtype;
      guint32                 tattoo;
      GimpVectorsCompatPoint *points;
      guint32                 num_points;
      guint32                 closed;
      gint                    i;

      /*
       * name (string)
       * locked (gint)
       * state (gchar)
       * closed (gint)
       * number points (gint)
       * version (gint)
       * pathtype (gint)
       * tattoo (gint)
       * then each point.
       */

      points = gimp_vectors_compat_get_points (vectors,
                                               (gint32 *) &num_points,
                                               (gint32 *) &closed);

      /* if no points are generated because of a faulty path we should
       * skip saving the path - this is unfortunately impossible, because
       * we already saved the number of paths and I won't start seeking
       * around to fix that cruft  */

      name     = (gchar *) gimp_object_get_name (vectors);
      locked   = gimp_item_get_linked (GIMP_ITEM (vectors));
      state    = closed ? 4 : 2;  /* EDIT : ADD  (editing state, 1.2 compat) */
      version  = 3;
      pathtype = 1;  /* BEZIER  (1.2 compat) */
      tattoo   = gimp_item_get_tattoo (GIMP_ITEM (vectors));

      xcf_write_string_check_error (info, &name,       1);
      xcf_write_int32_check_error  (info, &locked,     1);
      xcf_write_int8_check_error   (info, &state,      1);
      xcf_write_int32_check_error  (info, &closed,     1);
      xcf_write_int32_check_error  (info, &num_points, 1);
      xcf_write_int32_check_error  (info, &version,    1);
      xcf_write_int32_check_error  (info, &pathtype,   1);
      xcf_write_int32_check_error  (info, &tattoo,     1);

      for (i = 0; i < num_points; i++)
        {
          gfloat x;
          gfloat y;

          x = points[i].x;
          y = points[i].y;

          /*
           * type (gint)
           * x (gfloat)
           * y (gfloat)
           */

          xcf_write_int32_check_error (info, &points[i].type, 1);
          xcf_write_float_check_error (info, &x,              1);
          xcf_write_float_check_error (info, &y,              1);
        }

      g_free (points);
    }

  return TRUE;
}

static gboolean
xcf_save_vectors (XcfInfo    *info,
                  GimpImage  *image,
                  GError    **error)
{
  GimpVectors *active_vectors;
  guint32      version      = 1;
  guint32      active_index = 0;
  guint32      num_paths;
  GList       *list;
  GList       *stroke_list;
  GError      *tmp_error = NULL;

  /* Write out the following:-
   *
   * version (gint)
   * active_index (gint)
   * num_paths (gint)
   *
   * then each path:-
   */

  active_vectors = gimp_image_get_active_vectors (image);

  if (active_vectors)
    active_index = gimp_container_get_child_index (gimp_image_get_vectors (image),
                                                   GIMP_OBJECT (active_vectors));

  num_paths = gimp_container_get_n_children (gimp_image_get_vectors (image));

  xcf_write_int32_check_error (info, &version,      1);
  xcf_write_int32_check_error (info, &active_index, 1);
  xcf_write_int32_check_error (info, &num_paths,    1);

  for (list = gimp_image_get_vectors_iter (image);
       list;
       list = g_list_next (list))
    {
      GimpVectors      *vectors = list->data;
      GimpParasiteList *parasites;
      const gchar      *name;
      guint32           tattoo;
      guint32           visible;
      guint32           linked;
      guint32           num_parasites;
      guint32           num_strokes;

      /*
       * name (string)
       * tattoo (gint)
       * visible (gint)
       * linked (gint)
       * num_parasites (gint)
       * num_strokes (gint)
       *
       * then each parasite
       * then each stroke
       */

      name          = gimp_object_get_name (vectors);
      visible       = gimp_item_get_visible (GIMP_ITEM (vectors));
      linked        = gimp_item_get_linked (GIMP_ITEM (vectors));
      tattoo        = gimp_item_get_tattoo (GIMP_ITEM (vectors));
      parasites     = gimp_item_get_parasites (GIMP_ITEM (vectors));
      num_parasites = gimp_parasite_list_persistent_length (parasites);
      num_strokes   = g_queue_get_length (vectors->strokes);

      xcf_write_string_check_error (info, (gchar **) &name, 1);
      xcf_write_int32_check_error  (info, &tattoo,          1);
      xcf_write_int32_check_error  (info, &visible,         1);
      xcf_write_int32_check_error  (info, &linked,          1);
      xcf_write_int32_check_error  (info, &num_parasites,   1);
      xcf_write_int32_check_error  (info, &num_strokes,     1);

      xcf_check_error (xcf_save_parasite_list (info, parasites, error));

      for (stroke_list = g_list_first (vectors->strokes->head);
           stroke_list;
           stroke_list = g_list_next (stroke_list))
        {
          GimpStroke *stroke = stroke_list->data;
          guint32     stroke_type;
          guint32     closed;
          guint32     num_axes;
          GArray     *control_points;
          gint        i;

          guint32     type;
          gfloat      coords[6];

          /*
           * stroke_type (gint)
           * closed (gint)
           * num_axes (gint)
           * num_control_points (gint)
           *
           * then each control point.
           */

          if (GIMP_IS_BEZIER_STROKE (stroke))
            {
              stroke_type = XCF_STROKETYPE_BEZIER_STROKE;
              num_axes = 2;   /* hardcoded, might be increased later */
            }
          else
            {
              g_printerr ("Skipping unknown stroke type!\n");
              continue;
            }

          control_points = gimp_stroke_control_points_get (stroke,
                                                           (gint32 *) &closed);

          xcf_write_int32_check_error (info, &stroke_type,         1);
          xcf_write_int32_check_error (info, &closed,              1);
          xcf_write_int32_check_error (info, &num_axes,            1);
          xcf_write_int32_check_error (info, &control_points->len, 1);

          for (i = 0; i < control_points->len; i++)
            {
              GimpAnchor *anchor;

              anchor = & (g_array_index (control_points, GimpAnchor, i));

              type      = anchor->type;
              coords[0] = anchor->position.x;
              coords[1] = anchor->position.y;
              coords[2] = anchor->position.pressure;
              coords[3] = anchor->position.xtilt;
              coords[4] = anchor->position.ytilt;
              coords[5] = anchor->position.wheel;

              /*
               * type (gint)
               *
               * the first num_axis elements of:
               * [0] x (gfloat)
               * [1] y (gfloat)
               * [2] pressure (gfloat)
               * [3] xtilt (gfloat)
               * [4] ytilt (gfloat)
               * [5] wheel (gfloat)
               */

              xcf_write_int32_check_error (info, &type,  1);
              xcf_write_float_check_error (info, coords, num_axes);
            }

          g_array_free (control_points, TRUE);
        }
    }

  return TRUE;
}
