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
#include "libgimpconfig/gimpconfig.h"

#include "config/gimpgeglconfig.h"

#include "core/core-types.h"

#include "gegl/gimp-babl-compat.h"
#include "gegl/gimp-gegl-tile-compat.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpchannel.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-filters.h"
#include "core/gimpdrawablefilter.h"
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
#include "core/gimpitemlist.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimplist.h"
#include "core/gimpparasitelist.h"
#include "core/gimpprogress.h"
#include "core/gimpsamplepoint.h"
#include "core/gimpsymmetry.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "text/gimptextlayer.h"
#include "text/gimptextlayer-xcf.h"

#include "vectors/gimpanchor.h"
#include "vectors/gimpbezierstroke.h"
#include "vectors/gimppath.h"
#include "vectors/gimpstroke.h"
#include "vectors/gimppath-compat.h"

#include "xcf-private.h"
#include "xcf-read.h"
#include "xcf-save.h"
#include "xcf-seek.h"
#include "xcf-write.h"

#include "gimp-intl.h"

typedef void (* CompressTileFunc) (GeglRectangle  *tile_rect,
                                   guchar         *tile_data,
                                   const Babl     *format,
                                   guchar         *out_data,
                                   gint            out_data_max_len,
                                   gint           *lenptr);

/* Per thread data for xcf_save_tile_rle */
typedef struct
{
  /* Common to all jobs. */
  GeglBuffer       *buffer;
  gint              file_version;
  gint              max_out_data_len;
  CompressTileFunc  compress;

  /* Job specific. */
  gint              tile;
  gint              batch_size;

  /* Temp data to avoid too many allocations. */
  guchar           *tile_data;

  /* Return data. */
  guchar           *out_data;
  gint              out_data_len[XCF_TILE_SAVE_BATCH_SIZE];
} XcfJobData;

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
static gboolean xcf_save_effect_props  (XcfInfo           *info,
                                        GimpImage         *image,
                                        GimpFilter        *filter,
                                        GError           **error);
static gboolean xcf_save_path_props    (XcfInfo           *info,
                                        GimpImage         *image,
                                        GimpPath          *vectors,
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
static gboolean xcf_save_effect        (XcfInfo           *info,
                                        GimpImage         *image,
                                        GimpFilter        *filter,
                                        GError           **error);
static gboolean xcf_save_path          (XcfInfo           *info,
                                        GimpImage         *image,
                                        GimpPath          *vectors,
                                        GError           **error);
static gboolean xcf_save_buffer        (XcfInfo           *info,
                                        GimpImage         *image,
                                        GeglBuffer        *buffer,
                                        GError           **error);
static gboolean xcf_save_level         (XcfInfo           *info,
                                        GimpImage         *image,
                                        GeglBuffer        *buffer,
                                        GError           **error);
static gboolean xcf_save_tile          (XcfInfo           *info,
                                        GeglBuffer        *buffer,
                                        GeglRectangle     *tile_rect,
                                        const Babl        *format,
                                        GError           **error);
static void     xcf_save_free_job_data (XcfJobData        *data);
static gint     xcf_save_sort_job_data (XcfJobData        *data1,
                                        XcfJobData        *data2,
                                        gpointer           user_data);
static void     xcf_save_tile_parallel (XcfJobData        *job_data,
                                        GAsyncQueue       *queue);
static void     xcf_save_tile_rle      (GeglRectangle     *tile_rect,
                                        guchar            *tile_data,
                                        const Babl        *format,
                                        guchar            *rlebuf,
                                        gint               rlebuf_max_len,
                                        gint              *lenptr);
static void     xcf_save_tile_zlib     (GeglRectangle     *tile_rect,
                                        guchar            *tile_data,
                                        const Babl        *format,
                                        guchar            *zlib_data,
                                        gint               zlib_data_max_len,
                                        gint              *lenptr);
static gboolean xcf_save_parasite      (XcfInfo           *info,
                                        GimpParasite      *parasite,
                                        GError           **error);
static gboolean xcf_save_parasite_list (XcfInfo           *info,
                                        GimpParasiteList  *parasite,
                                        GError           **error);
static gboolean xcf_save_old_paths     (XcfInfo           *info,
                                        GimpImage         *image,
                                        GError           **error);
static gboolean xcf_save_old_vectors   (XcfInfo           *info,
                                        GimpImage         *image,
                                        GError           **error);


/* private convenience macros */
#define xcf_write_int32_check_error(info, data, count, cleanup_code) G_STMT_START { \
  xcf_write_int32 (info, data, count, &tmp_error);                    \
  if (tmp_error)                                                      \
    {                                                                 \
      g_propagate_error (error, tmp_error);                           \
      cleanup_code;                                                   \
      return FALSE;                                                   \
    }                                                                 \
  } G_STMT_END

#define xcf_write_offset_check_error(info, data, count, cleanup_code) G_STMT_START { \
  xcf_write_offset (info, data, count, &tmp_error);                    \
  if (tmp_error)                                                       \
    {                                                                  \
      g_propagate_error (error, tmp_error);                            \
      cleanup_code;                                                    \
      return FALSE;                                                    \
    }                                                                  \
  } G_STMT_END

#define xcf_write_zero_offset_check_error(info, count, cleanup_code) G_STMT_START { \
  xcf_write_zero_offset (info, count, &tmp_error);                    \
  if (tmp_error)                                                      \
    {                                                                 \
      g_propagate_error (error, tmp_error);                           \
      cleanup_code;                                                   \
      return FALSE;                                                   \
    }                                                                 \
  } G_STMT_END

#define xcf_write_int8_check_error(info, data, count, cleanup_code) G_STMT_START { \
  xcf_write_int8 (info, data, count, &tmp_error);                    \
  if (tmp_error)                                                     \
    {                                                                \
      g_propagate_error (error, tmp_error);                          \
      cleanup_code;                                                  \
      return FALSE;                                                  \
    }                                                                \
  } G_STMT_END

#define xcf_write_float_check_error(info, data, count, cleanup_code) G_STMT_START { \
  xcf_write_float (info, data, count, &tmp_error);                    \
  if (tmp_error)                                                      \
    {                                                                 \
      g_propagate_error (error, tmp_error);                           \
      cleanup_code;                                                   \
      return FALSE;                                                   \
    }                                                                 \
  } G_STMT_END

#define xcf_write_string_check_error(info, data, count, cleanup_code) G_STMT_START { \
  xcf_write_string (info, data, count, &tmp_error);                    \
  if (tmp_error)                                                       \
    {                                                                  \
      g_propagate_error (error, tmp_error);                            \
      cleanup_code;                                                    \
      return FALSE;                                                    \
    }                                                                  \
  } G_STMT_END

#define xcf_write_component_check_error(info, bpc, data, count, cleanup_code) G_STMT_START { \
  xcf_write_component (info, bpc, data, count, &tmp_error);          \
  if (tmp_error)                                                     \
    {                                                                \
      g_propagate_error (error, tmp_error);                          \
      cleanup_code;                                                  \
      return FALSE;                                                  \
    }                                                                \
  } G_STMT_END

#define xcf_write_prop_type_check_error(info, prop_type, cleanup_code) G_STMT_START { \
  guint32 _prop_int32 = prop_type;                                                    \
  xcf_write_int32_check_error (info, &_prop_int32, 1, cleanup_code);                  \
  } G_STMT_END

#define xcf_check_error(x, cleanup_code) G_STMT_START { \
  if (! (x))                              \
    {                                     \
      cleanup_code;                       \
      return FALSE;                       \
    }                                     \
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
  GList   *all_paths = NULL;
  GList   *list;
  goffset  saved_pos;
  goffset  offset;
  guint32  value;
  guint    n_layers;
  guint    n_channels;
  guint    n_paths  = 0;
  guint    progress = 0;
  guint    max_progress;
  gchar    version_tag[16];
  gboolean write_paths = FALSE;
  GError  *tmp_error   = NULL;

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

  xcf_write_int8_check_error (info, (guint8 *) version_tag, 14, ;);

  /* write out the width, height and image type information for the image */
  value = gimp_image_get_width (image);
  xcf_write_int32_check_error (info, (guint32 *) &value, 1, ;);

  value = gimp_image_get_height (image);
  xcf_write_int32_check_error (info, (guint32 *) &value, 1, ;);

  value = gimp_image_get_base_type (image);
  xcf_write_int32_check_error (info, &value, 1, ;);

  if (info->file_version >= 4)
    {
      value = gimp_image_get_precision (image);
      xcf_write_int32_check_error (info, &value, 1, ;);
    }

  if (info->file_version >= 18)
    write_paths = TRUE;

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

  if (write_paths)
    {
      all_paths = gimp_image_get_path_list (image);
      n_paths   = (guint) g_list_length (all_paths);
    }

  max_progress = 1 + n_layers + n_channels + n_paths;

  /* write the property information for the image */
  xcf_check_error (xcf_save_image_props (info, image, error), ;);

  xcf_progress_update (info);

  /* 'saved_pos' is the next slot in the offset table */
  saved_pos = info->cp;

  /* write an empty offset table */
  xcf_write_zero_offset_check_error (info,
                                     n_layers + n_channels + 2 +
                                     (write_paths ? n_paths + 1 : 0), ;);

  /* 'offset' is where we will write the next layer or channel */
  offset = info->cp;

  for (list = all_layers; list; list = g_list_next (list))
    {
      GimpLayer *layer = list->data;

      /* seek back to the next slot in the offset table and write the
       * offset of the layer
       */
      xcf_check_error (xcf_seek_pos (info, saved_pos, error), ;);
      xcf_write_offset_check_error (info, &offset, 1, ;);

      /* remember the next slot in the offset table */
      saved_pos = info->cp;

      /* seek to the layer offset and save the layer */
      xcf_check_error (xcf_seek_pos (info, offset, error), ;);
      xcf_check_error (xcf_save_layer (info, image, layer, error), ;);

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
      xcf_check_error (xcf_seek_pos (info, saved_pos, error), ;);
      xcf_write_offset_check_error (info, &offset, 1, ;);

      /* remember the next slot in the offset table */
      saved_pos = info->cp;

      /* seek to the channel offset and save the channel */
      xcf_check_error (xcf_seek_pos (info, offset, error), ;);
      xcf_check_error (xcf_save_channel (info, image, channel, error), ;);

      /* the next channels's offset is after the channel we just wrote */
      offset = info->cp;

      xcf_progress_update (info);
    }

  if (write_paths)
    {
      /* skip a '0' in the offset table to indicate the end of the channel
       * offsets
       */
      saved_pos += info->bytes_per_offset;

      for (list = all_paths; list; list = g_list_next (list))
        {
          GimpPath *vectors = list->data;

          /* seek back to the next slot in the offset table and write the
           * offset of the channel
           */
          xcf_check_error (xcf_seek_pos (info, saved_pos, error), ;);
          xcf_write_offset_check_error (info, &offset, 1, ;);

          /* remember the next slot in the offset table */
          saved_pos = info->cp;

          /* seek to the channel offset and save the channel */
          xcf_check_error (xcf_seek_pos (info, offset, error), ;);
          xcf_check_error (xcf_save_path (info, image, vectors, error), ;);

          /* the next channels's offset is after the channel we just wrote */
          offset = info->cp;

          xcf_progress_update (info);
        }
    }

  /* there is already a '0' at the end of the offset table to indicate
   * the end of the channel offsets
   */

  g_list_free (all_layers);
  g_list_free (all_channels);
  g_list_free (all_paths);

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
  GimpUnit         *unit          = gimp_image_get_unit (image);
  gdouble           xres;
  gdouble           yres;

  gimp_image_get_resolution (image, &xres, &yres);

  /* check and see if we should save the colormap property */
  if (gimp_image_get_colormap_palette (image))
    {
      gint    n_colors;
      guint8 *colormap = _gimp_image_get_colormap (image, &n_colors);
      xcf_check_error (xcf_save_prop (info, image, PROP_COLORMAP, error,
                                      n_colors, colormap), ;);
      g_free (colormap);
    }

  if (info->compression != COMPRESS_NONE)
    xcf_check_error (xcf_save_prop (info, image, PROP_COMPRESSION, error,
                                    info->compression), ;);

  if (gimp_image_get_guides (image))
    xcf_check_error (xcf_save_prop (info, image, PROP_GUIDES, error,
                                    gimp_image_get_guides (image)), ;);

  if (gimp_image_get_sample_points (image))
    {
      /* save the new property before the old one, so loading can skip
       * the latter
       */
      xcf_check_error (xcf_save_prop (info, image, PROP_SAMPLE_POINTS, error,
                                      gimp_image_get_sample_points (image)), ;);
      xcf_check_error (xcf_save_prop (info, image, PROP_OLD_SAMPLE_POINTS, error,
                                      gimp_image_get_sample_points (image)), ;);
    }

  xcf_check_error (xcf_save_prop (info, image, PROP_RESOLUTION, error,
                                  xres, yres), ;);

  xcf_check_error (xcf_save_prop (info, image, PROP_TATTOO, error,
                                  gimp_image_get_tattoo_state (image)), ;);

  if (gimp_unit_is_built_in (unit))
    xcf_check_error (xcf_save_prop (info, image, PROP_UNIT, error, unit), ;);

  if (gimp_container_get_n_children (gimp_image_get_paths (image)) > 0 &&
      info->file_version < 18)
    {
      if (gimp_path_compat_is_compatible (image))
        xcf_check_error (xcf_save_prop (info, image, PROP_PATHS, error), ;);
      else
        xcf_check_error (xcf_save_prop (info, image, PROP_VECTORS, error), ;);
    }

  if (! gimp_unit_is_built_in (unit))
    xcf_check_error (xcf_save_prop (info, image, PROP_USER_UNIT, error, unit), ;);

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
                                      private->parasites), ;);
    }

  if (grid_parasite)
    {
      gimp_parasite_list_remove (private->parasites,
                                 gimp_parasite_get_name (grid_parasite));
      gimp_parasite_free (grid_parasite);
    }

  if (meta_parasite)
    {
      gimp_parasite_list_remove (private->parasites,
                                 gimp_parasite_get_name (meta_parasite));
      gimp_parasite_free (meta_parasite);
    }

  for (iter = symmetry_parasites; iter; iter = g_list_next (iter))
    {
      GimpParasite *parasite = iter->data;

      gimp_parasite_list_remove (private->parasites,
                                 gimp_parasite_get_name (parasite));
    }
  g_list_free_full (symmetry_parasites,
                    (GDestroyNotify) gimp_parasite_free);

  info->layer_sets = gimp_image_get_stored_item_sets (image, GIMP_TYPE_LAYER);
  info->channel_sets = gimp_image_get_stored_item_sets (image, GIMP_TYPE_CHANNEL);

  for (iter = info->layer_sets; iter; iter = iter->next)
    xcf_check_error (xcf_save_prop (info, image, PROP_ITEM_SET, error, iter->data), ;);
  for (iter = info->channel_sets; iter; iter = iter->next)
    xcf_check_error (xcf_save_prop (info, image, PROP_ITEM_SET, error, iter->data), ;);

  xcf_check_error (xcf_save_prop (info, image, PROP_END, error), ;);

  return TRUE;
}

static gboolean
xcf_save_layer_props (XcfInfo    *info,
                      GimpImage  *image,
                      GimpLayer  *layer,
                      GError    **error)
{
  GimpParasiteList *parasites;
  GList            *iter;
  gint              offset_x;
  gint              offset_y;

  if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
    xcf_check_error (xcf_save_prop (info, image, PROP_GROUP_ITEM, error), ;);

  if (gimp_viewable_get_parent (GIMP_VIEWABLE (layer)))
    {
      GList *path;

      path = gimp_item_get_path (GIMP_ITEM (layer));
      xcf_check_error (xcf_save_prop (info, image, PROP_ITEM_PATH, error,
                                      path), ;);
      g_list_free (path);
    }

  if (g_list_find (gimp_image_get_selected_layers (image), layer))
    xcf_check_error (xcf_save_prop (info, image, PROP_ACTIVE_LAYER, error), ;);

  if (layer == gimp_image_get_floating_selection (image))
    {
      info->floating_sel_drawable = gimp_layer_get_floating_sel_drawable (layer);
      xcf_check_error (xcf_save_prop (info, image, PROP_FLOATING_SELECTION,
                                      error), ;);
    }

  xcf_check_error (xcf_save_prop (info, image, PROP_OPACITY, error,
                                  gimp_layer_get_opacity (layer)), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_FLOAT_OPACITY, error,
                                  gimp_layer_get_opacity (layer)), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_VISIBLE, error,
                                  gimp_item_get_visible (GIMP_ITEM (layer))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_COLOR_TAG, error,
                                  gimp_item_get_color_tag (GIMP_ITEM (layer))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_CONTENT, error,
                                  gimp_item_get_lock_content (GIMP_ITEM (layer))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_ALPHA, error,
                                  gimp_layer_get_lock_alpha (layer)), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_POSITION, error,
                                  gimp_item_get_lock_position (GIMP_ITEM (layer))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_VISIBILITY, error,
                                  gimp_item_get_lock_visibility (GIMP_ITEM (layer))), ;);

  if (gimp_layer_get_mask (layer))
    {
      xcf_check_error (xcf_save_prop (info, image, PROP_APPLY_MASK, error,
                                      gimp_layer_get_apply_mask (layer)), ;);
      xcf_check_error (xcf_save_prop (info, image, PROP_EDIT_MASK, error,
                                      gimp_layer_get_edit_mask (layer)), ;);
      xcf_check_error (xcf_save_prop (info, image, PROP_SHOW_MASK, error,
                                      gimp_layer_get_show_mask (layer)), ;);
    }
  else
    {
      xcf_check_error (xcf_save_prop (info, image, PROP_APPLY_MASK, error,
                                      FALSE), ;);
      xcf_check_error (xcf_save_prop (info, image, PROP_EDIT_MASK, error,
                                      FALSE), ;);
      xcf_check_error (xcf_save_prop (info, image, PROP_SHOW_MASK, error,
                                      FALSE), ;);
    }

  gimp_item_get_offset (GIMP_ITEM (layer), &offset_x, &offset_y);

  xcf_check_error (xcf_save_prop (info, image, PROP_OFFSETS, error,
                                  offset_x, offset_y), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_MODE, error,
                                  gimp_layer_get_mode (layer)), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_BLEND_SPACE, error,
                                  gimp_layer_get_mode (layer),
                                  gimp_layer_get_blend_space (layer)), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_COMPOSITE_SPACE, error,
                                  gimp_layer_get_mode (layer),
                                  gimp_layer_get_composite_space (layer)), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_COMPOSITE_MODE, error,
                                  gimp_layer_get_mode (layer),
                                  gimp_layer_get_composite_mode (layer)), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_TATTOO, error,
                                  gimp_item_get_tattoo (GIMP_ITEM (layer))), ;);

  if (GIMP_IS_TEXT_LAYER (layer) && GIMP_TEXT_LAYER (layer)->text)
    {
      GimpTextLayer *text_layer = GIMP_TEXT_LAYER (layer);
      guint32        flags      = gimp_text_layer_get_xcf_flags (text_layer);

      gimp_text_layer_xcf_save_prepare (text_layer);

      if (flags)
        xcf_check_error (xcf_save_prop (info,
                                        image, PROP_TEXT_LAYER_FLAGS, error,
                                        flags), ;);
    }

  if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
    {
      gint32 flags = 0;

      if (gimp_viewable_get_expanded (GIMP_VIEWABLE (layer)))
        flags |= XCF_GROUP_ITEM_EXPANDED;

      xcf_check_error (xcf_save_prop (info,
                                      image, PROP_GROUP_ITEM_FLAGS, error,
                                      flags), ;);
    }

  parasites = gimp_item_get_parasites (GIMP_ITEM (layer));

  if (gimp_parasite_list_length (parasites) > 0)
    {
      xcf_check_error (xcf_save_prop (info, image, PROP_PARASITES, error,
                                      parasites), ;);
    }

  for (iter = info->layer_sets; iter; iter = iter->next)
    {
      GimpItemList *set = iter->data;

      if (! gimp_item_list_is_pattern (set, NULL))
        {
          GList *items = gimp_item_list_get_items (set, NULL);

          if (g_list_find (items, GIMP_ITEM (layer)))
            xcf_check_error (xcf_save_prop (info, image, PROP_ITEM_SET_ITEM, error,
                                            g_list_position (info->layer_sets, iter)), ;);

          g_list_free (items);
        }
    }

  xcf_check_error (xcf_save_prop (info, image, PROP_END, error), ;);

  return TRUE;
}

static gboolean
xcf_save_channel_props (XcfInfo      *info,
                        GimpImage    *image,
                        GimpChannel  *channel,
                        GError      **error)
{
  GimpParasiteList *parasites;
  GList            *iter;

  if (g_list_find (gimp_image_get_selected_channels (image), channel))
    xcf_check_error (xcf_save_prop (info, image, PROP_ACTIVE_CHANNEL, error), ;);

  if (channel == gimp_image_get_mask (image))
    xcf_check_error (xcf_save_prop (info, image, PROP_SELECTION, error), ;);

  xcf_check_error (xcf_save_prop (info, image, PROP_OPACITY, error,
                                  gimp_channel_get_opacity (channel)), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_FLOAT_OPACITY, error,
                                  gimp_channel_get_opacity (channel)), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_VISIBLE, error,
                                  gimp_item_get_visible (GIMP_ITEM (channel))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_COLOR_TAG, error,
                                  gimp_item_get_color_tag (GIMP_ITEM (channel))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_CONTENT, error,
                                  gimp_item_get_lock_content (GIMP_ITEM (channel))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_POSITION, error,
                                  gimp_item_get_lock_position (GIMP_ITEM (channel))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_VISIBILITY, error,
                                  gimp_item_get_lock_visibility (GIMP_ITEM (channel))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_SHOW_MASKED, error,
                                  gimp_channel_get_show_masked (channel)), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_COLOR, error,
                                  channel->color), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_FLOAT_COLOR, error,
                                  channel->color), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_TATTOO, error,
                                  gimp_item_get_tattoo (GIMP_ITEM (channel))), ;);

  parasites = gimp_item_get_parasites (GIMP_ITEM (channel));

  if (gimp_parasite_list_length (parasites) > 0)
    {
      xcf_check_error (xcf_save_prop (info, image, PROP_PARASITES, error,
                                      parasites), ;);
    }

  for (iter = info->channel_sets; iter; iter = iter->next)
    {
      GimpItemList *set = iter->data;

      if (! gimp_item_list_is_pattern (set, NULL))
        {
          GList *items = gimp_item_list_get_items (set, NULL);

          if (g_list_find (items, GIMP_ITEM (channel)))
            xcf_check_error (xcf_save_prop (info, image, PROP_ITEM_SET_ITEM, error,
                                            g_list_position (info->channel_sets, iter)), ;);

          g_list_free (items);
        }
    }

  xcf_check_error (xcf_save_prop (info, image, PROP_END, error), ;);

  return TRUE;
}

static gboolean
xcf_save_effect_props (XcfInfo      *info,
                       GimpImage    *image,
                       GimpFilter   *filter,
                       GError      **error)
{
  GParamSpec **pspecs;
  guint        n_pspecs;
  GeglNode    *node;
  gchar       *operation;

  xcf_check_error (xcf_save_prop (info, image, PROP_VISIBLE, error,
                                  gimp_filter_get_active (filter)), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_FLOAT_OPACITY, error,
                                  gimp_drawable_filter_get_opacity (GIMP_DRAWABLE_FILTER (filter))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_MODE, error,
                                  gimp_drawable_filter_get_paint_mode (GIMP_DRAWABLE_FILTER (filter))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_BLEND_SPACE, error,
                                  gimp_drawable_filter_get_blend_space (GIMP_DRAWABLE_FILTER (filter))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_COMPOSITE_SPACE, error,
                                  gimp_drawable_filter_get_composite_space (GIMP_DRAWABLE_FILTER (filter))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_COMPOSITE_MODE, error,
                                  gimp_drawable_filter_get_composite_mode (GIMP_DRAWABLE_FILTER (filter))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_FILTER_CLIP, error,
                                  gimp_drawable_filter_get_clip (GIMP_DRAWABLE_FILTER (filter))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_FILTER_REGION, error,
                                  gimp_drawable_filter_get_region (GIMP_DRAWABLE_FILTER (filter))), ;);

  /* Save each GEGL property individually */
  node = gimp_drawable_filter_get_operation (GIMP_DRAWABLE_FILTER (filter));

  gegl_node_get (node,
                 "operation", &operation,
                 NULL);

  pspecs = gegl_operation_list_properties (operation, &n_pspecs);

  for (gint i = 0; i < n_pspecs; i++)
    {
      GParamSpec     *pspec       = pspecs[i];
      GValue          value       = G_VALUE_INIT;
      FilterPropType  filter_type = FILTER_PROP_UNKNOWN;

      g_value_init (&value, pspec->value_type);
      gegl_node_get_property (node, pspec->name,
                              &value);

      switch (G_VALUE_TYPE (&value))
        {
          case G_TYPE_INT:
            filter_type = FILTER_PROP_INT;
            break;

          case G_TYPE_UINT:
            filter_type = FILTER_PROP_UINT;
            break;

          case G_TYPE_BOOLEAN:
            filter_type = FILTER_PROP_BOOL;
            break;

          case G_TYPE_FLOAT:
          case G_TYPE_DOUBLE:
            filter_type = FILTER_PROP_FLOAT;
            break;

          case G_TYPE_STRING:
            filter_type = FILTER_PROP_STRING;
            break;

          default:
            if (g_type_is_a (G_VALUE_TYPE (&value), GIMP_TYPE_CONFIG))
              {
                filter_type = FILTER_PROP_CONFIG;
              }
            else if (g_type_is_a (G_VALUE_TYPE (&value), G_TYPE_ENUM))
              {
                filter_type = FILTER_PROP_ENUM;
              }
            else if (g_type_is_a (G_VALUE_TYPE (&value), GEGL_TYPE_COLOR))
              {
                filter_type = FILTER_PROP_COLOR;
              }
            else
              {
                gimp_message (info->gimp, G_OBJECT (info->progress),
                              GIMP_MESSAGE_WARNING,
                              "XCF Warning: argument \"%s\" of filter %s has "
                              "unsupported type %s. It was discarded.",
                              pspec->name, operation,
                              g_type_name (G_VALUE_TYPE (&value)));
              }
            break;
        }

      if (filter_type != FILTER_PROP_UNKNOWN)
        xcf_check_error (xcf_save_prop (info, image, PROP_FILTER_ARGUMENT, error,
                         pspec->name, filter_type, value), ;);

      g_value_unset (&value);
    }
  g_free (operation);
  g_free (pspecs);

  xcf_check_error (xcf_save_prop (info, image, PROP_END, error), ;);

  return TRUE;
}

static gboolean
xcf_save_path_props (XcfInfo      *info,
                     GimpImage    *image,
                     GimpPath     *vectors,
                     GError      **error)
{
  GimpParasiteList *parasites;

  if (g_list_find (gimp_image_get_selected_paths (image), vectors))
    xcf_check_error (xcf_save_prop (info, image, PROP_SELECTED_PATH, error), ;);

  xcf_check_error (xcf_save_prop (info, image, PROP_VISIBLE, error,
                                  gimp_item_get_visible (GIMP_ITEM (vectors))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_COLOR_TAG, error,
                                  gimp_item_get_color_tag (GIMP_ITEM (vectors))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_CONTENT, error,
                                  gimp_item_get_lock_content (GIMP_ITEM (vectors))), ;);
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_POSITION, error,
                                  gimp_item_get_lock_position (GIMP_ITEM (vectors))), ;);

  xcf_check_error (xcf_save_prop (info, image, PROP_TATTOO, error,
                                  gimp_item_get_tattoo (GIMP_ITEM (vectors))), ;);

  parasites = gimp_item_get_parasites (GIMP_ITEM (vectors));

  if (gimp_parasite_list_length (parasites) > 0)
    {
      xcf_check_error (xcf_save_prop (info, image, PROP_PARASITES, error,
                                      parasites), ;);
    }

#if 0
  for (iter = info->vectors_sets; iter; iter = iter->next)
    {
      GimpItemList *set = iter->data;

      if (! gimp_item_list_is_pattern (set, NULL))
        {
          GList *items = gimp_item_list_get_items (set, NULL);

          if (g_list_find (items, GIMP_ITEM (vectors)))
            xcf_check_error (xcf_save_prop (info, image, PROP_ITEM_SET_ITEM, error,
                                            g_list_position (info->layer_sets, iter)), ;);

          g_list_free (items);
        }
    }
#endif

  xcf_check_error (xcf_save_prop (info, image, PROP_END, error), ;);

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

      xcf_write_prop_type_check_error (info, prop_type, va_end (args));
      xcf_write_int32_check_error (info, &size, 1, va_end (args));
      break;

    case PROP_COLORMAP:
      {
        guint32  n_colors = va_arg (args, guint32);
        guchar  *colors   = va_arg (args, guchar *);

        size = 4 + n_colors * 3;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &n_colors, 1, va_end (args));
        xcf_write_int8_check_error  (info, colors,    n_colors * 3, va_end (args));
      }
      break;

    case PROP_ACTIVE_LAYER:
    case PROP_ACTIVE_CHANNEL:
    case PROP_SELECTED_PATH:
    case PROP_SELECTION:
    case PROP_GROUP_ITEM:
      size = 0;

      xcf_write_prop_type_check_error (info, prop_type, va_end (args));
      xcf_write_int32_check_error (info, &size, 1, va_end (args));
      break;

    case PROP_FLOATING_SELECTION:
      size = info->bytes_per_offset;

      xcf_write_prop_type_check_error (info, prop_type, va_end (args));
      xcf_write_int32_check_error (info, &size, 1, va_end (args));

      info->floating_sel_offset = info->cp;
      xcf_write_zero_offset_check_error (info, 1, va_end (args));
      break;

    case PROP_OPACITY:
      {
        gdouble opacity      = va_arg (args, gdouble);
        guint32 uint_opacity = opacity * 255.999;

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &uint_opacity, 1, va_end (args));
      }
      break;

    case PROP_FLOAT_OPACITY:
      {
        gfloat opacity = va_arg (args, gdouble);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_float_check_error (info, &opacity, 1, va_end (args));
      }
      break;

    case PROP_MODE:
      {
        gint32 mode = va_arg (args, gint32);

        size = 4;

        if (mode == GIMP_LAYER_MODE_OVERLAY_LEGACY)
          mode = GIMP_LAYER_MODE_SOFTLIGHT_LEGACY;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, (guint32 *) &mode, 1, va_end (args));
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

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, (guint32 *) &blend_space, 1, va_end (args));
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

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, (guint32 *) &composite_space, 1, va_end (args));
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

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, (guint32 *) &composite_mode, 1, va_end (args));
      }
      break;

    case PROP_VISIBLE:
      {
        guint32 visible = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &visible, 1, va_end (args));
      }
      break;

    case PROP_LINKED:
      /* This code should not be called any longer. */
      g_return_val_if_reached (FALSE);
#if 0
      {
        guint32 linked = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &linked, 1, va_end (args));
      }
#endif
      break;

    case PROP_COLOR_TAG:
      {
        guint32 color_tag = va_arg (args, gint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &color_tag, 1, va_end (args));
      }
      break;

    case PROP_LOCK_CONTENT:
      {
        guint32 lock_content = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &lock_content, 1, va_end (args));
      }
      break;

    case PROP_LOCK_ALPHA:
      {
        guint32 lock_alpha = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &lock_alpha, 1, va_end (args));
      }
      break;

    case PROP_LOCK_POSITION:
      {
        guint32 lock_position = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &lock_position, 1, va_end (args));
      }
      break;

    case PROP_LOCK_VISIBILITY:
      {
        guint32 lock_visibility = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &lock_visibility, 1, va_end (args));
      }
      break;

    case PROP_APPLY_MASK:
      {
        guint32 apply_mask = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &apply_mask, 1, va_end (args));
      }
      break;

    case PROP_EDIT_MASK:
      {
        guint32 edit_mask = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &edit_mask, 1, va_end (args));
      }
      break;

    case PROP_SHOW_MASK:
      {
        guint32 show_mask = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &show_mask, 1, va_end (args));
      }
      break;

    case PROP_SHOW_MASKED:
      {
        guint32 show_masked = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &show_masked, 1, va_end (args));
      }
      break;

    case PROP_OFFSETS:
      {
        gint32 offsets[2];

        offsets[0] = va_arg (args, gint32);
        offsets[1] = va_arg (args, gint32);

        size = 8;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, (guint32 *) offsets, 2, va_end (args));
      }
      break;

    case PROP_COLOR:
      {
        GeglColor *color = va_arg (args, GeglColor *);
        guchar     col[3];

        gegl_color_get_pixel (color, babl_format ("R'G'B' u8"), col);

        size = 3;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int8_check_error (info, col, 3, va_end (args));
      }
      break;

    case PROP_FLOAT_COLOR:
      {
        GeglColor *color = va_arg (args, GeglColor *);
        gfloat     col[3];

        gegl_color_get_pixel (color, babl_format ("R'G'B' float"), col);

        size = 3 * 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_float_check_error (info, col, 3, va_end (args));
      }
      break;

    case PROP_COMPRESSION:
      {
        guint8 compression = (guint8) va_arg (args, guint32);

        size = 1;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int8_check_error (info, &compression, 1, va_end (args));
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

            xcf_write_prop_type_check_error (info, prop_type, va_end (args));
            xcf_write_int32_check_error (info, &size, 1, va_end (args));

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

                xcf_write_int32_check_error (info, (guint32 *) &position,    1, va_end (args));
                xcf_write_int8_check_error  (info, (guint8 *)  &orientation, 1, va_end (args));
              }
          }
      }
      break;

    case PROP_SAMPLE_POINTS:
      {
        GList *sample_points   = va_arg (args, GList *);
        gint   n_sample_points = g_list_length (sample_points);

        size = n_sample_points * (5 * 4);

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        for (; sample_points; sample_points = g_list_next (sample_points))
          {
            GimpSamplePoint   *sample_point = sample_points->data;
            gint32             x, y;
            GimpColorPickMode  pick_mode;
            guint32            padding[2] = { 0, };

            gimp_sample_point_get_position (sample_point, &x, &y);
            pick_mode = gimp_sample_point_get_pick_mode (sample_point);

            xcf_write_int32_check_error (info, (guint32 *) &x,         1, va_end (args));
            xcf_write_int32_check_error (info, (guint32 *) &y,         1, va_end (args));
            xcf_write_int32_check_error (info, (guint32 *) &pick_mode, 1, va_end (args));
            xcf_write_int32_check_error (info, (guint32 *) padding,    2, va_end (args));
          }
      }
      break;

    case PROP_OLD_SAMPLE_POINTS:
      {
        GList *sample_points   = va_arg (args, GList *);
        gint   n_sample_points = g_list_length (sample_points);

        size = n_sample_points * (4 + 4);

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        for (; sample_points; sample_points = g_list_next (sample_points))
          {
            GimpSamplePoint *sample_point = sample_points->data;
            gint32           x, y;

            gimp_sample_point_get_position (sample_point, &x, &y);

            xcf_write_int32_check_error (info, (guint32 *) &x, 1, va_end (args));
            xcf_write_int32_check_error (info, (guint32 *) &y, 1, va_end (args));
          }
      }
      break;

    case PROP_RESOLUTION:
      {
        gfloat resolution[2];

        resolution[0] = va_arg (args, double);
        resolution[1] = va_arg (args, double);

        size = 2 * 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_float_check_error (info, resolution, 2, va_end (args));
      }
      break;

    case PROP_TATTOO:
      {
        guint32 tattoo = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &tattoo, 1, va_end (args));
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

            xcf_write_prop_type_check_error (info, prop_type, va_end (args));

            /* because we don't know how much room the parasite list
             * will take we save the file position and write the
             * length later
             */
            pos = info->cp;
            xcf_write_int32_check_error (info, &size, 1, va_end (args));

            base = info->cp;

            xcf_check_error (xcf_save_parasite_list (info, list, error), va_end (args));

            size = info->cp - base;

            /* go back to the saved position and write the length */
            xcf_check_error (xcf_seek_pos (info, pos, error), va_end (args));
            xcf_write_int32_check_error (info, &size, 1, va_end (args));

            xcf_check_error (xcf_seek_pos (info, base + size, error), va_end (args));
          }
      }
      break;

    case PROP_UNIT:
      {
        GimpUnit *unit       = va_arg (args, GimpUnit *);
        guint32   unit_index = gimp_unit_get_id (unit);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &unit_index, 1, va_end (args));
      }
      break;

    case PROP_PATHS:
      {
        goffset base;
        goffset pos;

        size = 0;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));

        /* because we don't know how much room the paths list will
         * take we save the file position and write the length later
         */
        pos = info->cp;
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        base = info->cp;

        xcf_check_error (xcf_save_old_paths (info, image, error), va_end (args));

        size = info->cp - base;

        /* go back to the saved position and write the length */
        xcf_check_error (xcf_seek_pos (info, pos, error), va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_check_error (xcf_seek_pos (info, base + size, error), va_end (args));
      }
      break;

    case PROP_USER_UNIT:
      {
        GimpUnit    *unit = va_arg (args, GimpUnit *);
        const gchar *unit_strings[5];
        gfloat       factor;
        guint32      digits;

        /* write the entire unit definition */
        unit_strings[0] = gimp_unit_get_name (unit);
        factor          = gimp_unit_get_factor (unit);
        digits          = gimp_unit_get_digits (unit);
        unit_strings[1] = gimp_unit_get_symbol (unit);
        unit_strings[2] = gimp_unit_get_abbreviation (unit);
        /* Singular and plural forms were deprecated in XCF 21. Just use
         * the unit name as bogus (yet reasonable) replacements.
         */
        unit_strings[3] = gimp_unit_get_name (unit);
        unit_strings[4] = gimp_unit_get_name (unit);

        size =
          2 * 4 +
          strlen (unit_strings[0]) ? strlen (unit_strings[0]) + 5 : 4 +
          strlen (unit_strings[1]) ? strlen (unit_strings[1]) + 5 : 4 +
          strlen (unit_strings[2]) ? strlen (unit_strings[2]) + 5 : 4;

        if (info->file_version < 21)
          size +=
            strlen (unit_strings[3]) ? strlen (unit_strings[3]) + 5 : 4 +
            strlen (unit_strings[4]) ? strlen (unit_strings[4]) + 5 : 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_float_check_error  (info, &factor,                 1, va_end (args));
        xcf_write_int32_check_error  (info, &digits,                 1, va_end (args));
        if (info->file_version < 21)
          xcf_write_string_check_error (info, (gchar **) unit_strings, 5, va_end (args));
        else
          xcf_write_string_check_error (info, (gchar **) unit_strings, 3, va_end (args));
      }
      break;

    case PROP_VECTORS:
      {
        goffset base;
        goffset pos;

        size = 0;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));

        /* because we don't know how much room the paths list will
         * take we save the file position and write the length later
         */
        pos = info->cp;
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        base = info->cp;

        xcf_check_error (xcf_save_old_vectors (info, image, error), va_end (args));

        size = info->cp - base;

        /* go back to the saved position and write the length */
        xcf_check_error (xcf_seek_pos (info, pos, error), va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_check_error (xcf_seek_pos (info, base + size, error), va_end (args));
      }
      break;

    case PROP_TEXT_LAYER_FLAGS:
      {
        guint32 flags = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &flags, 1, va_end (args));
      }
      break;

    case PROP_ITEM_PATH:
      {
        GList *path = va_arg (args, GList *);

        size = 4 * g_list_length (path);

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        while (path)
          {
            guint32 index = GPOINTER_TO_UINT (path->data);

            xcf_write_int32_check_error (info, &index, 1, va_end (args));

            path = g_list_next (path);
          }
      }
      break;

    case PROP_GROUP_ITEM_FLAGS:
      {
        guint32 flags = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));
        xcf_write_int32_check_error (info, &flags, 1, va_end (args));
      }
      break;

    case PROP_ITEM_SET:
      {
        GimpItemList *set = va_arg (args, GimpItemList *);
        const gchar  *string;
        guint32       method;
        guint32       item_type;
        goffset       base;
        goffset       pos;

        size = 0;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        pos = info->cp;
        xcf_write_int32_check_error (info, &size, 1, va_end (args));
        base = info->cp;

        if (gimp_item_list_get_item_type (set) == GIMP_TYPE_LAYER)
          item_type = 0;
        else if (gimp_item_list_get_item_type (set) == GIMP_TYPE_CHANNEL)
          item_type = 1;
        else if (gimp_item_list_get_item_type (set) == GIMP_TYPE_PATH)
          item_type = 2;
        else
          g_return_val_if_reached (FALSE);
        xcf_write_int32_check_error (info, &item_type, 1, va_end (args));

        if (! gimp_item_list_is_pattern (set, &method))
          method = G_MAXUINT32;
        xcf_write_int32_check_error (info, &method, 1, va_end (args));

        string = gimp_object_get_name (set);
        xcf_write_string_check_error (info, (gchar **) &string, 1, va_end (args));

        /* go back to the saved position and write the length */
        size = info->cp - base;
        xcf_check_error (xcf_seek_pos (info, pos, error), va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_check_error (xcf_seek_pos (info, base + size, error), va_end (args));
      }
      break;

    case PROP_ITEM_SET_ITEM:
      {
        guint32 set_n = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));
        xcf_write_int32_check_error (info, &set_n, 1, va_end (args));
      }
      break;

    case PROP_FILTER_REGION:
      {
        guint32 filter_region = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &filter_region, 1, va_end (args));
      }
      break;

    case PROP_FILTER_ARGUMENT:
      {
        const gchar *string      = va_arg (args, const gchar *);
        guint32      filter_type = va_arg (args, guint32);
        GValue       filter_value;
        goffset      pos;
        goffset      base;

        size = 0;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        pos = info->cp;
        xcf_write_int32_check_error (info, &size, 1, va_end (args));
        base = info->cp;
        filter_value = va_arg (args, GValue);

        xcf_write_string_check_error (info, (gchar **) &string, 1, va_end (args));
        xcf_write_int32_check_error (info, &filter_type, 1, va_end (args));

        switch (filter_type)
          {
            case FILTER_PROP_INT:
            case FILTER_PROP_UINT:
            case FILTER_PROP_ENUM:
              {
                guint32 value;

                if (filter_type == FILTER_PROP_INT)
                  value = g_value_get_int (&filter_value);
                else if (filter_type == FILTER_PROP_UINT)
                  value = g_value_get_uint (&filter_value);
                else
                  value = g_value_get_enum (&filter_value);

                xcf_write_int32_check_error (info, &value, 1, va_end (args));
              }
              break;

            case FILTER_PROP_BOOL:
              {
                gboolean value = g_value_get_boolean (&filter_value);

                xcf_write_int32_check_error (info, (guint32 *) &value, 1, va_end (args));
              }
              break;

            case FILTER_PROP_FLOAT:
              {
                gfloat value = g_value_get_double (&filter_value);

                xcf_write_float_check_error (info, &value, 1, va_end (args));
              }
              break;

            case FILTER_PROP_STRING:
              {
                const gchar *value = g_value_get_string (&filter_value);

                xcf_write_string_check_error (info, (gchar **) &value, 1, va_end (args));
              }
              break;

            case FILTER_PROP_COLOR:
              {
                GeglColor *color = g_value_get_object (&filter_value);

                if (color)
                  {
                    const gchar   *encoding;
                    const Babl    *format = gegl_color_get_format (color);
                    const Babl    *space;
                    GBytes        *bytes;
                    gconstpointer  data;
                    gsize          data_length;
                    int            profile_length = 0;

                    if (babl_format_is_palette (format))
                      {
                        guint8     pixel[40];
                        GeglColor *palette_color;

                        /* As a special case, we don't want to serialize
                         * palette colors, because they are just too much
                         * dependent on external data and cannot be
                         * deserialized back safely. So we convert them first.
                         */
                         palette_color = gegl_color_duplicate (color);

                         format = babl_format_with_space ("R'G'B'A u8", format);
                         gegl_color_get_pixel (palette_color, format, pixel);
                         gegl_color_set_pixel (color, format, pixel);

                         g_object_unref (palette_color);
                      }

                    encoding = babl_format_get_encoding (format);
                    xcf_write_string_check_error (info, (gchar **) &encoding, 1, va_end (args));

                    bytes = gegl_color_get_bytes (color, format);
                    data  = (guint8 *) g_bytes_get_data (bytes, &data_length);

                    xcf_write_int32_check_error (info, (guint32 *) &data_length, 1, ;);
                    xcf_write_int8_check_error (info, (const guint8 *) data, data_length, ;);
                    g_bytes_unref (bytes);

                    space = babl_format_get_space (format);
                    if (space != babl_space ("sRGB"))
                      {
                        guint8 *profile_data;

                        profile_data = (guint8 *) babl_space_get_icc (babl_format_get_space (format),
                                                                      &profile_length);
                        xcf_write_int32_check_error (info, (guint32 *) &profile_length, 1, ;);

                        if (profile_data)
                          xcf_write_int8_check_error (info, profile_data, profile_length, ;);
                      }
                    else
                      {
                        xcf_write_int32_check_error (info, (guint32 *) &profile_length, 1, ;);
                      }
                  }
              }
              break;

            case FILTER_PROP_CONFIG:
              {
                GimpConfig *config = g_value_get_object (&filter_value);
                gchar      *value  = gimp_config_serialize_to_string (config, NULL);

                xcf_write_string_check_error (info, (gchar **) &value, 1, va_end (args));
                g_free (value);
              }
              break;

            default:
              break;
          }

        /* go back to the saved position and write the length */
        size = info->cp - base;
        xcf_check_error (xcf_seek_pos (info, pos, error), va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_check_error (xcf_seek_pos (info, base + size, error), va_end (args));
      }
      break;

    case PROP_FILTER_CLIP:
      {
        guint32 visible = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type, va_end (args));
        xcf_write_int32_check_error (info, &size, 1, va_end (args));

        xcf_write_int32_check_error (info, &visible, 1, va_end (args));
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
  goffset        saved_pos;
  goffset        offset;
  guint32        value;
  const gchar   *string;
  GimpContainer *filters;
  GList         *filter_list;
  guint32        num_effects = 0;
  GList         *list;
  GError        *tmp_error   = NULL;

  /* check and see if this is the drawable that the floating
   *  selection is attached to.
   */
  if (GIMP_DRAWABLE (layer) == info->floating_sel_drawable)
    {
      saved_pos = info->cp;
      xcf_check_error (xcf_seek_pos (info, info->floating_sel_offset, error), ;);
      xcf_write_offset_check_error (info, &saved_pos, 1, ;);
      xcf_check_error (xcf_seek_pos (info, saved_pos, error), ;);
    }

  /* Get filter information */
  filters = gimp_drawable_get_filters (GIMP_DRAWABLE (layer));
  /* Since floating selections are also stored in the filter stack,
   * we need to verify what's in there to get the correct count */
  for (filter_list = GIMP_LIST (filters)->queue->tail; filter_list;
       filter_list = g_list_previous (filter_list))
    {
      if (GIMP_IS_DRAWABLE_FILTER (filter_list->data))
        {
          GimpDrawableFilter *filter  = filter_list->data;
          GimpChannel        *mask    = NULL;
          GeglNode           *op_node = NULL;

          mask    = gimp_drawable_filter_get_mask (filter);
          op_node = gimp_drawable_filter_get_operation (filter);

          /* For now, prevent tool-based filters from being saved */
          if (mask    != NULL &&
              op_node != NULL &&
              strcmp (gegl_node_get_operation (op_node), "GraphNode") != 0)
            num_effects++;
        }
    }

  /* write out the width, height and image type information for the layer */
  value = gimp_item_get_width (GIMP_ITEM (layer));
  xcf_write_int32_check_error (info, &value, 1, ;);

  value = gimp_item_get_height (GIMP_ITEM (layer));
  xcf_write_int32_check_error (info, &value, 1, ;);

  value = gimp_babl_format_get_image_type (gimp_drawable_get_format (GIMP_DRAWABLE (layer)));
  xcf_write_int32_check_error (info, &value, 1, ;);

  /* write out the layers name */
  string = gimp_object_get_name (layer);
  xcf_write_string_check_error (info, (gchar **) &string, 1, ;);

  /* write out the layer properties */
  xcf_save_layer_props (info, image, layer, error);

  /* write out the layer tile hierarchy and effects */
  offset = info->cp + (2 + num_effects + 1) * info->bytes_per_offset;
  xcf_write_offset_check_error (info, &offset, 1, ;);

  saved_pos = info->cp;

  /* write a zero layer mask offset */
  xcf_write_zero_offset_check_error (info, 1, ;);

  /* write out zero effect and effect mask offset(s) */
  for (gint i = 0; i < num_effects + 1; i++)
    xcf_write_zero_offset_check_error (info, 1, ;);

  xcf_check_error (xcf_save_buffer (info, image,
                                    gimp_drawable_get_buffer (GIMP_DRAWABLE (layer)),
                                    error), ;);

  offset = info->cp;

  /* write out the layer mask */
  if (gimp_layer_get_mask (layer))
    {
      GimpLayerMask *mask = gimp_layer_get_mask (layer);

      xcf_check_error (xcf_seek_pos (info, saved_pos, error), ;);
      xcf_write_offset_check_error (info, &offset, 1, ;);

      xcf_check_error (xcf_seek_pos (info, offset, error), ;);
      xcf_check_error (xcf_save_channel (info, image, GIMP_CHANNEL (mask),
                                         error), ;);
    }

  /* write out any layer effects and effect masks */
  if (num_effects > 0)
    {
      saved_pos += info->bytes_per_offset;

      for (list = GIMP_LIST (filters)->queue->head; list;
           list = g_list_next (list))
        {
          if (GIMP_IS_DRAWABLE_FILTER (list->data))
            {
              GimpDrawableFilter *filter  = list->data;
              GimpChannel        *mask    = NULL;
              GeglNode           *op_node = NULL;

              mask    = gimp_drawable_filter_get_mask (filter);
              op_node = gimp_drawable_filter_get_operation (filter);

               /* For now, prevent tool-based filters from being saved */
              if (mask    != NULL &&
                  op_node != NULL &&
                  strcmp (gegl_node_get_operation (op_node), "GraphNode") != 0)
                {
                  offset = info->cp;

                  xcf_check_error (xcf_seek_pos (info, saved_pos, error), ;);
                  xcf_write_offset_check_error (info, &offset, 1, ;);

                  saved_pos = info->cp;

                  xcf_check_error (xcf_seek_pos (info, offset, error), ;);
                  xcf_check_error (xcf_save_effect (info, image,
                                                    GIMP_FILTER (filter),
                                                    error), ;);
                }
            }
        }
      g_list_free (list);
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
      xcf_check_error (xcf_seek_pos (info, info->floating_sel_offset, error), ;);
      xcf_write_offset_check_error (info, &saved_pos, 1, ;);
      xcf_check_error (xcf_seek_pos (info, saved_pos, error), ;);
    }

  /* write out the width and height information for the channel */
  value = gimp_item_get_width (GIMP_ITEM (channel));
  xcf_write_int32_check_error (info, &value, 1, ;);

  value = gimp_item_get_height (GIMP_ITEM (channel));
  xcf_write_int32_check_error (info, &value, 1, ;);

  /* write out the channels name */
  string = gimp_object_get_name (channel);
  xcf_write_string_check_error (info, (gchar **) &string, 1, ;);

  /* write out the channel properties */
  xcf_save_channel_props (info, image, channel, error);

  /* write out the channel tile hierarchy */
  offset = info->cp + info->bytes_per_offset;
  xcf_write_offset_check_error (info, &offset, 1, ;);

  xcf_check_error (xcf_save_buffer (info, image,
                                    gimp_drawable_get_buffer (GIMP_DRAWABLE (channel)),
                                    error), ;);

  return TRUE;
}

static gboolean
xcf_save_effect (XcfInfo     *info,
                 GimpImage   *image,
                 GimpFilter  *filter,
                 GError     **error)
{
  gchar              *name;
  gchar              *icon;
  GimpDrawableFilter *filter_drawable;
  GeglNode           *node;
  gchar              *operation;
  const gchar        *op_version;
  GimpChannel        *effect_mask;
  goffset             offset;
  GError             *tmp_error = NULL;

  filter_drawable = GIMP_DRAWABLE_FILTER (filter);
  node            = gimp_drawable_filter_get_operation (filter_drawable);

  gegl_node_get (node,
                 "operation", &operation,
                 NULL);

  g_object_get (filter,
                "name",      &name,
                "icon-name", &icon,
                NULL);

  /* Write out effect name */
  xcf_write_string_check_error (info, (gchar **) &name, 1, ;);
  g_free (name);

  /* Write out effect icon */
  xcf_write_string_check_error (info, (gchar **) &icon, 1, ;);
  g_free (icon);

  /* Write out GEGL operation name */
  xcf_write_string_check_error (info, (gchar **) &operation, 1, ;);
  g_free (operation);

  if (info->file_version >= 22)
    {
      /* Write out GEGL operation version */
      op_version = gegl_operation_get_op_version (operation);
      xcf_write_string_check_error (info, (gchar **) &op_version, 1, ;);
    }

  /* write out the effect properties */
  xcf_save_effect_props (info, image, filter, error);

  /* write a zero effect mask offset */
  offset = info->cp + info->bytes_per_offset;
  xcf_write_offset_check_error (info, &offset, 1, ;);

  effect_mask = gimp_drawable_filter_get_mask (filter_drawable);
  xcf_check_error (xcf_save_channel (info, image, effect_mask,
                                     error), ;);

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
                 GimpImage   *image,
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

  xcf_write_int32_check_error (info, (guint32 *) &width,  1, ;);
  xcf_write_int32_check_error (info, (guint32 *) &height, 1, ;);
  xcf_write_int32_check_error (info, (guint32 *) &bpp,    1, ;);

  tmp1 = xcf_calc_levels (width,  XCF_TILE_WIDTH);
  tmp2 = xcf_calc_levels (height, XCF_TILE_HEIGHT);
  nlevels = MAX (tmp1, tmp2);

  /* 'saved_pos' is the next slot in the offset table */
  saved_pos = info->cp;

  /* write an empty offset table */
  xcf_write_zero_offset_check_error (info, nlevels + 1, ;);

  /* 'offset' is where we will write the next level */
  offset = info->cp;

  for (i = 0; i < nlevels; i++)
    {
      /* seek back to the next slot in the offset table and write the
       * offset of the level
       */
      xcf_check_error (xcf_seek_pos (info, saved_pos, error), ;);
      xcf_write_offset_check_error (info, &offset, 1, ;);

      /* remember the next slot in the offset table */
      saved_pos = info->cp;

      /* seek to the level offset and save the level */
      xcf_check_error (xcf_seek_pos (info, offset, error), ;);

      if (i == 0)
        {
          /* write out the level. */
          xcf_check_error (xcf_save_level (info, image, buffer, error), ;);
        }
      else
        {
          /* fake an empty level */
          tmp1 = 0;
          width  /= 2;
          height /= 2;
          xcf_write_int32_check_error (info, (guint32 *) &width,  1, ;);
          xcf_write_int32_check_error (info, (guint32 *) &height, 1, ;);

          /* NOTE:  this should be an offset, not an int32!  however...
           * since there are already 64-bit-offsets XCFs out there in
           * which this field is 32-bit, and since it's not actually
           * being used, we're going to keep this field 32-bit for the
           * dummy levels, to remain consistent.  if we ever make use
           * of levels above the first, we should turn this field into
           * an offset, and bump the xcf version.
           */
          xcf_write_int32_check_error (info, (guint32 *) &tmp1,   1, ;);
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
                GimpImage   *image,
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
  gint        num_processors;
  gint        i, j, k;
  GError     *tmp_error = NULL;

  num_processors = GIMP_GEGL_CONFIG (image->gimp->config)->num_processors;

  format = gegl_buffer_get_format (buffer);

  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);
  bpp    = babl_format_get_bytes_per_pixel (format);

  xcf_write_int32_check_error (info, (guint32 *) &width,  1, ;);
  xcf_write_int32_check_error (info, (guint32 *) &height, 1, ;);

  saved_pos = info->cp;

  /* maximal allowable size of on-disk tile data.  make it somewhat bigger than
   * the uncompressed tile size, to allow for the possibility of negative
   * compression.  xcf_load_level() enforces this limit.
   */
  max_data_length = XCF_TILE_WIDTH * XCF_TILE_HEIGHT * bpp *
                    XCF_TILE_MAX_DATA_LENGTH_FACTOR /* = 1.5, currently */;

  n_tile_rows = gimp_gegl_buffer_get_n_tile_rows (buffer, XCF_TILE_HEIGHT);
  n_tile_cols = gimp_gegl_buffer_get_n_tile_cols (buffer, XCF_TILE_WIDTH);

  ntiles = n_tile_rows * n_tile_cols;

  /* allocate an offset table so we don't have to seek back after each
   * tile, see bug #686862. allocate ntiles + 1 slots because a zero
   * offset indicates the offset table's end.
   * Do not use g_alloca since it may cause Stack Overflow on
   * large images, see issue #6138.
   */
  offset_table = g_malloc0 ((ntiles + 1) * sizeof (goffset));
  next_offset = offset_table;

  /* 'saved_pos' is the offset of the tile offset table  */
  saved_pos = info->cp;

  /* write an empty offset table */
  xcf_write_zero_offset_check_error (info, ntiles + 1, ;);

  /* 'offset' is where we will write the next tile */
  offset = info->cp;

  if (info->compression == COMPRESS_RLE ||
      info->compression == COMPRESS_ZLIB)
    {
      /* parallel implementation */
      XcfJobData  *job_data;
      guchar      *switch_out_data;
      gint         out_data_len[XCF_TILE_SAVE_BATCH_SIZE];

      GThreadPool *pool;
      GAsyncQueue *queue;
      gint         num_tasks = num_processors * 2;
      gint         tile_size = XCF_TILE_WIDTH * XCF_TILE_HEIGHT * bpp;
      gint         out_data_max_size;
      gint         next_tile = 0;

      out_data_max_size = tile_size * XCF_TILE_MAX_DATA_LENGTH_FACTOR;
      /* Prepare an additional out_data to quickly switch. */
      switch_out_data   = g_malloc (out_data_max_size * XCF_TILE_SAVE_BATCH_SIZE);

      /* The free function passed to the queue and thread pool will likely never
       * be used. It would mean the thread pool is unfinidhed or the result
       * queue still has data which would mean we had to interrupt the save,
       * i.e. there is a bug in our code.
       */
      queue = g_async_queue_new_full ((GDestroyNotify) xcf_save_free_job_data);
      pool  = g_thread_pool_new_full ((GFunc) xcf_save_tile_parallel,
                                      queue,
                                      (GDestroyNotify) xcf_save_free_job_data,
                                      num_processors, TRUE, NULL);

      i = 0;
      /* We push more tasks than there are threads, ensuring threads always have
       * something to do!
       */
      for (j = 0; j < num_tasks && i < ntiles; j++)
        {
          job_data = g_malloc (sizeof (XcfJobData ));
          job_data->buffer        = buffer;
          job_data->file_version  = info->file_version;
          job_data->max_out_data_len = out_data_max_size;
          job_data->compress      = (info->compression == COMPRESS_RLE) ?
                                      xcf_save_tile_rle : xcf_save_tile_zlib;
          job_data->tile_data     = g_malloc (tile_size);
          job_data->out_data      = g_malloc (out_data_max_size * XCF_TILE_SAVE_BATCH_SIZE);

          job_data->tile       = i;
          job_data->batch_size = MIN (XCF_TILE_SAVE_BATCH_SIZE, ntiles - i);
          i += job_data->batch_size;

          g_thread_pool_push (pool, job_data, NULL);
        }

      /* Continue pushing tasks and writing tasks as long as we have tiles to
       * process.
       */
      while (i < ntiles)
        {
          while ((job_data = g_async_queue_pop (queue)))
            {
              if (next_tile == job_data->tile)
                {
                  guchar *tmp_out_data;
                  gint    batch_size;

                  tmp_out_data = job_data->out_data;
                  job_data->out_data = switch_out_data;
                  switch_out_data = tmp_out_data;

                  batch_size = job_data->batch_size;

                  for (k = 0; k < batch_size; k++)
                    out_data_len[k] = job_data->out_data_len[k];

                  /* First immediately push a new task for the thread pool,
                   * ensuring it always has work to do.
                   */
                  job_data->tile       = i;
                  job_data->batch_size = MIN (XCF_TILE_SAVE_BATCH_SIZE, ntiles - i);
                  i += job_data->batch_size;

                  g_thread_pool_push (pool, job_data, NULL);

                  /* Now write the data. */
                  for (k = 0; k < batch_size; k++)
                    {
                      *next_offset++ = offset;
                      xcf_write_int8_check_error (info,
                                                  switch_out_data + out_data_max_size * k,
                                                  out_data_len[k], ;);
                      if (info->cp < offset || info->cp - offset > max_data_length)
                        {
                          g_message ("xcf: invalid tile data length: %" G_GOFFSET_FORMAT,
                                     info->cp - offset);
                          g_thread_pool_free (pool, TRUE, TRUE);
                          g_async_queue_unref (queue);
                          g_free (offset_table);
                          return FALSE;
                        }
                      offset = info->cp;
                    }
                  next_tile += batch_size;
                  break;
                }
              else
                {
                  g_async_queue_push_sorted (queue, job_data,
                                             (GCompareDataFunc) xcf_save_sort_job_data,
                                             NULL);
                }
            }
        }
      g_free (switch_out_data);

      /* Finally wait for all remaining tasks to write. */
      while ((job_data = g_async_queue_pop (queue)))
        {
          if (next_tile == job_data->tile)
            {
              gboolean done = FALSE;

              for (k = 0; k < job_data->batch_size; k++)
                {
                  *next_offset++ = offset;
                  xcf_write_int8_check_error (info,
                                              job_data->out_data + out_data_max_size * k,
                                              job_data->out_data_len[k], ;);
                  if (info->cp < offset || info->cp - offset > max_data_length)
                    {
                      g_message ("xcf: invalid tile data length: %" G_GOFFSET_FORMAT,
                                 info->cp - offset);
                      g_thread_pool_free (pool, TRUE, TRUE);
                      g_async_queue_unref (queue);
                      g_free (offset_table);
                      return FALSE;
                    }
                  offset = info->cp;
                }
              next_tile += job_data->batch_size;

              if (job_data->tile + job_data->batch_size >= ntiles)
                done = TRUE;

              xcf_save_free_job_data (job_data);

              if (done)
                break;
            }
          else
            {
              g_async_queue_push_sorted (queue, job_data,
                                         (GCompareDataFunc) xcf_save_sort_job_data,
                                         NULL);
            }
        }

      g_thread_pool_free (pool, FALSE, TRUE);
      g_async_queue_unref (queue);
    }
  else
    {
      /* non parallel implementation */
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
                                              error), ;);
              break;
            case COMPRESS_FRACTAL:
              g_warning ("xcf: fractal compression unimplemented");
              g_free (offset_table);
              return FALSE;
            default:
              g_warning ("xcf: unsupported compression algorithm");
              g_free (offset_table);
              return FALSE;
            }

          /* make sure the on-disk tile data didn't end up being too big.
           * xcf_load_level() would refuse to load the file if it did.
           */
          if (info->cp < offset || info->cp - offset > max_data_length)
            {
              g_message ("xcf: invalid tile data length: %" G_GOFFSET_FORMAT,
                         info->cp - offset);
              g_free (offset_table);
              return FALSE;
            }

          /* the next tile's offset is after the tile we just wrote */
          offset = info->cp;
        }
    }

  /* seek back to the offset table and write it  */
  xcf_check_error (xcf_seek_pos (info, saved_pos, error), ;);
  xcf_write_offset_check_error (info, offset_table, ntiles + 1, ;);

  /* seek to the end of the file */
  xcf_check_error (xcf_seek_pos (info, offset, error), ;);

  g_free (offset_table);

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
      xcf_write_int8_check_error (info, tile_data, tile_size, ;);
    }
  else
    {
      gint n_components = babl_format_get_n_components (format);

      xcf_write_component_check_error (info, bpp / n_components, tile_data,
                                       tile_size / bpp * n_components, ;);
    }

  return TRUE;
}

static void
xcf_save_free_job_data (XcfJobData *data)
{
  g_free (data->out_data);
  g_free (data->tile_data);
  g_free (data);
}

static gint
xcf_save_sort_job_data (XcfJobData *data1,
                        XcfJobData *data2,
                        gpointer    user_data)
{
  if (data1->tile < data2->tile)
    return -1;
  else if (data1->tile > data2->tile)
    return 1;
  else
    return 0;
}

static void
xcf_save_tile_parallel (XcfJobData  *job_data,
                        GAsyncQueue *queue)
{
  const Babl    *format;
  GeglRectangle  tile_rect;
  gint           bpp;

  format = gegl_buffer_get_format (job_data->buffer);
  bpp    = babl_format_get_bytes_per_pixel (format);

  for (gint i = 0; i < job_data->batch_size; ++i)
    {
      gimp_gegl_buffer_get_tile_rect (job_data->buffer,
                                      XCF_TILE_WIDTH,
                                      XCF_TILE_HEIGHT,
                                      job_data->tile + i,
                                      &tile_rect);
      /* only single thread can create tile data when cache miss */
      gegl_buffer_get (job_data->buffer, &tile_rect, 1.0, format,
                       job_data->tile_data,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (job_data->file_version >= 12)
        {
          gint n_components = babl_format_get_n_components (format);
          gint tile_size    = bpp * tile_rect.width * tile_rect.height;

          xcf_write_to_be (bpp / n_components, job_data->tile_data,
                           tile_size / bpp * n_components);
        }

      job_data->compress (&tile_rect, job_data->tile_data, format,
                          job_data->out_data + job_data->max_out_data_len * i,
                          job_data->max_out_data_len,
                          job_data->out_data_len + i);
    }

  g_async_queue_push_sorted (queue, job_data,
                             (GCompareDataFunc) xcf_save_sort_job_data,
                             NULL);
}

static void
xcf_save_tile_rle (GeglRectangle  *tile_rect,
                   guchar         *tile_data,
                   const Babl     *format,
                   guchar         *rlebuf,
                   gint            rlebuf_max_len,
                   gint           *lenptr)
{
  gint bpp = babl_format_get_bytes_per_pixel (format);
  gint len = 0;
  gint i, j;

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

  *lenptr = len;
}

static void
xcf_save_tile_zlib (GeglRectangle  *tile_rect,
                    guchar         *tile_data,
                    const Babl     *format,
                    guchar         *zlib_data,
                    gint            zlib_data_max_len,
                    gint           *lenptr)
{
  gint      bpp       = babl_format_get_bytes_per_pixel (format);
  gint      tile_size = bpp * tile_rect->width * tile_rect->height;
  /* The buffer for compressed data. */
  z_stream  strm;
  int       action;
  int       status;

  *lenptr = 0;

  /* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree  = Z_NULL;
  strm.opaque = Z_NULL;

  status = deflateInit (&strm, Z_DEFAULT_COMPRESSION);
  if (status != Z_OK)
    return;

  strm.next_in   = tile_data;
  strm.avail_in  = tile_size;
  strm.next_out  = zlib_data;
  strm.avail_out = zlib_data_max_len;

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
          size_t write_size = zlib_data_max_len - strm.avail_out;

          *lenptr = write_size;

          /* Reset next_out and avail_out. */
          strm.next_out  = zlib_data;
          strm.avail_out = zlib_data_max_len;
        }
      else if (status != Z_OK)
        {
          g_printerr ("xcf: tile compression failed: %s", zError (status));
          deflateEnd (&strm);
          return;
        }
    }

  deflateEnd (&strm);
}

static gboolean
xcf_save_parasite (XcfInfo       *info,
                   GimpParasite  *parasite,
                   GError       **error)
{
  if (gimp_parasite_is_persistent (parasite))
    {
      guint32        value;
      const gchar   *string;
      GError        *tmp_error = NULL;
      gconstpointer  parasite_data;

      string = gimp_parasite_get_name (parasite);
      xcf_write_string_check_error (info, (gchar **) &string, 1, ;);

      value = gimp_parasite_get_flags (parasite);
      xcf_write_int32_check_error (info, &value, 1, ;);

      parasite_data = gimp_parasite_get_data (parasite, &value);
      xcf_write_int32_check_error (info, &value, 1, ;);

      xcf_write_int8_check_error (info, parasite_data, value, ;);
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

/* This is the oldest way to save paths. */
static gboolean
xcf_save_old_paths (XcfInfo    *info,
                    GimpImage  *image,
                    GError    **error)
{
  GimpPath *active_path = NULL;
  guint32   num_paths;
  guint32   active_index = 0;
  GList    *list;
  GError   *tmp_error = NULL;

  /* Write out the following:-
   *
   * last_selected_row (gint)
   * number_of_paths (gint)
   *
   * then each path:-
   */

  num_paths = gimp_container_get_n_children (gimp_image_get_paths (image));

  if (gimp_image_get_selected_paths (image))
    {
      active_path = gimp_image_get_selected_paths (image)->data;
      /* Having more than 1 selected vectors should not have happened in this
       * code path but let's not break saving, only produce a critical.
       */
      if (g_list_length (gimp_image_get_selected_paths (image)) > 1)
        g_critical ("%s: this code path should not happen with multiple paths selected",
                    G_STRFUNC);
    }

  if (active_path)
    active_index = gimp_container_get_child_index (gimp_image_get_paths (image),
                                                   GIMP_OBJECT (active_path));

  xcf_write_int32_check_error (info, &active_index, 1, ;);
  xcf_write_int32_check_error (info, &num_paths,    1, ;);

  for (list = gimp_image_get_path_iter (image);
       list;
       list = g_list_next (list))
    {
      GimpPath               *vectors = list->data;
      gchar                  *name;
      guint32                 locked;
      guint8                  state;
      guint32                 version;
      guint32                 pathtype;
      guint32                 tattoo;
      GimpPathCompatPoint    *points;
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

      points = gimp_path_compat_get_points (vectors,
                                            (gint32 *) &num_points,
                                            (gint32 *) &closed);

      /* if no points are generated because of a faulty path we should
       * skip saving the path - this is unfortunately impossible, because
       * we already saved the number of paths and I won't start seeking
       * around to fix that cruft  */

      name     = (gchar *) gimp_object_get_name (vectors);
      /* The 'linked' concept does not exist anymore in GIMP 3.0 and over. */
      locked   = 0;
      state    = closed ? 4 : 2;  /* EDIT : ADD  (editing state, 1.2 compat) */
      version  = 3;
      pathtype = 1;  /* BEZIER  (1.2 compat) */
      tattoo   = gimp_item_get_tattoo (GIMP_ITEM (vectors));

      xcf_write_string_check_error (info, &name,       1, ;);
      xcf_write_int32_check_error  (info, &locked,     1, ;);
      xcf_write_int8_check_error   (info, &state,      1, ;);
      xcf_write_int32_check_error  (info, &closed,     1, ;);
      xcf_write_int32_check_error  (info, &num_points, 1, ;);
      xcf_write_int32_check_error  (info, &version,    1, ;);
      xcf_write_int32_check_error  (info, &pathtype,   1, ;);
      xcf_write_int32_check_error  (info, &tattoo,     1, ;);

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

          xcf_write_int32_check_error (info, &points[i].type, 1, ;);
          xcf_write_float_check_error (info, &x,              1, ;);
          xcf_write_float_check_error (info, &y,              1, ;);
        }

      g_free (points);
    }

  return TRUE;
}

/* This is an older way to save paths, though more recent than
 * xcf_save_old_paths(). It used to be the normal path storing format until all
 * 2.10 versions. It changed with GIMP 3.0.
 */
static gboolean
xcf_save_old_vectors (XcfInfo    *info,
                      GimpImage  *image,
                      GError    **error)
{
  GimpPath *active_path = NULL;
  guint32   version        = 1;
  guint32   active_index   = 0;
  guint32   num_paths;
  GList    *list;
  GList    *stroke_list;
  GError   *tmp_error = NULL;

  /* Write out the following:-
   *
   * version (gint)
   * active_index (gint)
   * num_paths (gint)
   *
   * then each path:-
   */

  if (gimp_image_get_selected_paths (image))
    {
      active_path = gimp_image_get_selected_paths (image)->data;
      /* Having more than 1 selected vectors should not have happened in this
       * code path but let's not break saving, only produce a critical.
       */
      if (g_list_length (gimp_image_get_selected_paths (image)) > 1)
        g_critical ("%s: this code path should not happen with multiple paths selected",
                    G_STRFUNC);
    }

  if (active_path)
    active_index = gimp_container_get_child_index (gimp_image_get_paths (image),
                                                   GIMP_OBJECT (active_path));

  num_paths = gimp_container_get_n_children (gimp_image_get_paths (image));

  xcf_write_int32_check_error (info, &version,      1, ;);
  xcf_write_int32_check_error (info, &active_index, 1, ;);
  xcf_write_int32_check_error (info, &num_paths,    1, ;);

  for (list = gimp_image_get_path_iter (image);
       list;
       list = g_list_next (list))
    {
      GimpPath         *vectors = list->data;
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
      /* The 'linked' concept does not exist anymore in GIMP 3.0 and over. */
      linked        = 0;
      tattoo        = gimp_item_get_tattoo (GIMP_ITEM (vectors));
      parasites     = gimp_item_get_parasites (GIMP_ITEM (vectors));
      num_parasites = gimp_parasite_list_persistent_length (parasites);
      num_strokes   = g_queue_get_length (vectors->strokes);

      xcf_write_string_check_error (info, (gchar **) &name, 1, ;);
      xcf_write_int32_check_error  (info, &tattoo,          1, ;);
      xcf_write_int32_check_error  (info, &visible,         1, ;);
      xcf_write_int32_check_error  (info, &linked,          1, ;);
      xcf_write_int32_check_error  (info, &num_parasites,   1, ;);
      xcf_write_int32_check_error  (info, &num_strokes,     1, ;);

      xcf_check_error (xcf_save_parasite_list (info, parasites, error), ;);

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

          xcf_write_int32_check_error (info, &stroke_type,         1, ;);
          xcf_write_int32_check_error (info, &closed,              1, ;);
          xcf_write_int32_check_error (info, &num_axes,            1, ;);
          xcf_write_int32_check_error (info, &control_points->len, 1, ;);

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

              xcf_write_int32_check_error (info, &type,  1, ;);
              xcf_write_float_check_error (info, coords, num_axes, ;);
            }

          g_array_free (control_points, TRUE);
        }
    }

  return TRUE;
}

static gboolean
xcf_save_path (XcfInfo      *info,
               GimpImage    *image,
               GimpPath     *vectors,
               GError      **error)
{
  const gchar *string;
  GList       *stroke_list;
  GError      *tmp_error = NULL;
  /* Version of the path format is always 1 for now. */
  guint32      version   = 1;
  guint32      num_strokes;
  guint32      size;
  goffset      base;
  goffset      pos;

  /* write out the path name */
  string = gimp_object_get_name (vectors);
  xcf_write_string_check_error (info, (gchar **) &string, 1, ;);

  /* Payload size */
  size = 0;
  pos = info->cp;
  xcf_write_int32_check_error (info, &size, 1, ;);
  base = info->cp;

  /* write out the path properties */
  xcf_save_path_props (info, image, vectors, error);

  /* Path version */
  xcf_write_int32_check_error (info, &version, 1, ;);

  /* Write out the number of strokes. */
  num_strokes = g_queue_get_length (vectors->strokes);
  xcf_write_int32_check_error  (info, &num_strokes, 1, ;);

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

      /* Stroke type. */
      xcf_write_int32_check_error (info, &stroke_type,         1, ;);
      /* close path or not? */
      xcf_write_int32_check_error (info, &closed,              1, ;);
      /* Number of floats given for each point. */
      xcf_write_int32_check_error (info, &num_axes,            1, ;);
      /* Number of control points. */
      xcf_write_int32_check_error (info, &control_points->len, 1, ;);

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

          xcf_write_int32_check_error (info, &type,  1, ;);
          xcf_write_float_check_error (info, coords, num_axes, ;);
        }

      g_array_free (control_points, TRUE);
    }

  /* go back to the saved position and write the length */
  size = info->cp - base;
  xcf_check_error (xcf_seek_pos (info, pos, error), ;);
  xcf_write_int32_check_error (info, &size, 1, ;);

  xcf_check_error (xcf_seek_pos (info, base + size, error), ;);

  return TRUE;
}
