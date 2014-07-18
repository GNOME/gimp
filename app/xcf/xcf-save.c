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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <cairo.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core/core-types.h"

#include "base/tile.h"
#include "base/tile-manager.h"
#include "base/tile-manager-private.h"

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
#include "core/gimpimage-private.h"
#include "core/gimpimage-sample-points.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimpparasitelist.h"
#include "core/gimpprogress.h"
#include "core/gimpsamplepoint.h"

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


/**
 * SECTION:xcf-save
 * @Short_description:XCF file saver functions
 *
 * XCF file saver
 */

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
static gboolean xcf_save_hierarchy     (XcfInfo           *info,
                                        TileManager       *tiles,
                                        GError           **error);
static gboolean xcf_save_level         (XcfInfo           *info,
                                        TileManager       *tiles,
                                        GError           **error);
static gboolean xcf_save_tile          (XcfInfo           *info,
                                        Tile              *tile,
                                        GError           **error);
static gboolean xcf_save_tile_rle      (XcfInfo           *info,
                                        Tile              *tile,
                                        guchar            *rlebuf,
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
  info->cp += xcf_write_int32 (info->fp, data, count, &tmp_error); \
  if (tmp_error)                                                   \
    {                                                              \
      g_propagate_error (error, tmp_error);                        \
      return FALSE;                                                \
    }                                                              \
  } G_STMT_END

#define xcf_write_int8_check_error(info, data, count) G_STMT_START { \
  info->cp += xcf_write_int8 (info->fp, data, count, &tmp_error); \
  if (tmp_error)                                                  \
    {                                                             \
      g_propagate_error (error, tmp_error);                       \
      return FALSE;                                               \
    }                                                             \
  } G_STMT_END

#define xcf_write_float_check_error(info, data, count) G_STMT_START { \
  info->cp += xcf_write_float (info->fp, data, count, &tmp_error); \
  if (tmp_error)                                                   \
    {                                                              \
      g_propagate_error (error, tmp_error);                        \
      return FALSE;                                                \
    }                                                              \
  } G_STMT_END

#define xcf_write_string_check_error(info, data, count) G_STMT_START { \
  info->cp += xcf_write_string (info->fp, data, count, &tmp_error); \
  if (tmp_error)                                                    \
    {                                                               \
      g_propagate_error (error, tmp_error);                         \
      return FALSE;                                                 \
    }                                                               \
  } G_STMT_END

#define xcf_write_prop_type_check_error(info, prop_type) G_STMT_START { \
  guint32 _prop_int32 = prop_type;                     \
  xcf_write_int32_check_error (info, &_prop_int32, 1); \
  } G_STMT_END

#define xcf_check_error(x) G_STMT_START { \
  if (! (x))                                                  \
    return FALSE;                                             \
  } G_STMT_END

#define xcf_progress_update(info) G_STMT_START  \
  {                                             \
    progress++;                                 \
    if (info->progress)                         \
      gimp_progress_set_value (info->progress,  \
                               (gdouble) progress / (gdouble) max_progress); \
  } G_STMT_END

static const guint32 zero = 0;

/**
 * xcf_save_choose_format:
 * @info:  #XcfInfo structure of the file to save
 * @image: Image to save
 *
 * Determines the format version for saving the image to an XCF file:
 *
 * 0: Nothing special is in the image.
 *
 * 1: Image has color map.
 *
 * 2: Image uses one of the layer modes "Soft light", "Grain extract",
 *    "Grain merge" or "Color erase".
 *
 * 3: Image contains a layer group.
 *
 */
void
xcf_save_choose_format (XcfInfo   *info,
                        GimpImage *image)
{
  GList *list;
  gint   save_version = 0;  /* default to oldest */

  /* need version 1 for color maps */
  if (gimp_image_get_colormap (image))
    save_version = 1;

  for (list = gimp_image_get_layer_iter (image);
       list && save_version < 3;
       list = g_list_next (list))
    {
      GimpLayer *layer = GIMP_LAYER (list->data);

      switch (gimp_layer_get_mode (layer))
        {
          /* new layer modes not supported by gimp-1.2 */
        case GIMP_SOFTLIGHT_MODE:
        case GIMP_GRAIN_EXTRACT_MODE:
        case GIMP_GRAIN_MERGE_MODE:
        case GIMP_COLOR_ERASE_MODE:
          save_version = MAX (2, save_version);
          break;

        default:
          break;
        }

      /* need version 3 for layer trees */
      if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
        save_version = MAX (3, save_version);
    }

  info->file_version = save_version;
}

/**
 * xcf_save_image:
 * @info:  #XcfInfo structure of the file to save
 * @image: Image to save
 * @error: Return location for errors
 *
 * Saves the image to an XCF file.
 *
 * Returns: 0 if file errors occurred
 */
gint
xcf_save_image (XcfInfo    *info,
                GimpImage  *image,
                GError    **error)
{
  GList   *all_layers;
  GList   *all_channels;
  GList   *list;
  guint32  saved_pos;
  guint32  offset;
  guint32  value;
  guint    n_layers;
  guint    n_channels;
  guint    progress = 0;
  guint    max_progress;
  gint     t1, t2, t3, t4;
  gchar    version_tag[16];
  GError  *tmp_error = NULL;

  /* write out the tag information for the image */
  if (info->file_version > 0)
    {
      sprintf (version_tag, "gimp xcf v%03d", info->file_version);
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

  value = gimp_image_base_type (image);
  xcf_write_int32_check_error (info, &value, 1);

  /* determine the number of layers and channels in the image */
  all_layers   = gimp_image_get_layer_list (image);
  all_channels = gimp_image_get_channel_list (image);

  /* check and see if we have to save out the selection */
  if (gimp_channel_bounds (gimp_image_get_mask (image),
                           &t1, &t2, &t3, &t4))
    {
      all_channels = g_list_append (all_channels, gimp_image_get_mask (image));
    }

  n_layers   = (guint) g_list_length (all_layers);
  n_channels = (guint) g_list_length (all_channels);

  max_progress = 1 + n_layers + n_channels;

  /* write the property information for the image. */
  xcf_check_error (xcf_save_image_props (info, image, error));

  xcf_progress_update (info);

  offset = info->cp + (n_layers + n_channels + 2) * 4;

  /* write the layers */
  for (list = all_layers; list; list = g_list_next (list))
    {
      GimpLayer *layer = list->data;

      /* write offset to layer pointers table */
      xcf_write_int32_check_error (info, &offset, 1);

      /* write layer at offset */
      saved_pos = info->cp;
      xcf_check_error (xcf_seek_pos (info, offset, error));
      xcf_check_error (xcf_save_layer (info, image, layer, error));

      /* increase offset */
      offset = info->cp;

      /* set file position to layer pointers table */
      xcf_check_error (xcf_seek_pos (info, saved_pos, error));

      /* indicate progress */
      xcf_progress_update (info);
    }

  /* write out a '0' offset position to indicate the end
   *  of the layer offsets.
   */
  xcf_write_int32_check_error (info, &zero, 1);

  /* write the channels */
  for (list = all_channels; list; list = g_list_next (list))
    {
      GimpChannel *channel = list->data;

      /* write offset to channel pointers table */
      xcf_write_int32_check_error (info, &offset, 1);

      /* write channel at offset */
      saved_pos = info->cp;
      xcf_check_error (xcf_seek_pos (info, offset, error));
      xcf_check_error (xcf_save_channel (info, image, channel, error));

      /* increase offset */
      offset = info->cp;

      /* set file position to channel pointers table */
      xcf_check_error (xcf_seek_pos (info, saved_pos, error));

      /* indicate progress */
      xcf_progress_update (info);
    }

  g_list_free (all_layers);
  g_list_free (all_channels);

  /* write out a '0' offset position to indicate the end
   *  of the channel offsets.
   */
  xcf_write_int32_check_error (info, &zero, 1);

  return !ferror (info->fp);
}

static gboolean
xcf_save_image_props (XcfInfo    *info,
                      GimpImage  *image,
                      GError    **error)
{
  GimpImagePrivate *private  = GIMP_IMAGE_GET_PRIVATE (image);
  GimpParasite     *parasite = NULL;
  GimpUnit          unit     = gimp_image_get_unit (image);
  gdouble           xres;
  gdouble           yres;

  gimp_image_get_resolution (image, &xres, &yres);

  /* check and see if we should save the color map property */
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
    xcf_check_error (xcf_save_prop (info, image, PROP_SAMPLE_POINTS, error,
                                    gimp_image_get_sample_points (image)));

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

      parasite = gimp_grid_to_parasite (grid);
      gimp_parasite_list_add (private->parasites, parasite);
    }

  if (gimp_parasite_list_length (private->parasites) > 0)
    {
      xcf_check_error (xcf_save_prop (info, image, PROP_PARASITES, error,
                                      private->parasites));
    }

  if (parasite)
    {
      gimp_parasite_list_remove (private->parasites,
                                 gimp_parasite_name (parasite));
      gimp_parasite_free (parasite);
    }

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
  xcf_check_error (xcf_save_prop (info, image, PROP_VISIBLE, error,
                                  gimp_item_get_visible (GIMP_ITEM (layer))));
  xcf_check_error (xcf_save_prop (info, image, PROP_LINKED, error,
                                  gimp_item_get_linked (GIMP_ITEM (layer))));
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_CONTENT, error,
                                  gimp_item_get_lock_content (GIMP_ITEM (layer))));
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_ALPHA, error,
                                  gimp_layer_get_lock_alpha (layer)));

  if (gimp_layer_get_mask (layer))
    {
      GimpLayerMask *mask = gimp_layer_get_mask (layer);

      xcf_check_error (xcf_save_prop (info, image, PROP_APPLY_MASK, error,
                                      gimp_layer_mask_get_apply (mask)));
      xcf_check_error (xcf_save_prop (info, image, PROP_EDIT_MASK, error,
                                      gimp_layer_mask_get_edit (mask)));
      xcf_check_error (xcf_save_prop (info, image, PROP_SHOW_MASK, error,
                                      gimp_layer_mask_get_show (mask)));
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
  guchar            col[3];

  if (channel == gimp_image_get_active_channel (image))
    xcf_check_error (xcf_save_prop (info, image, PROP_ACTIVE_CHANNEL, error));

  if (channel == gimp_image_get_mask (image))
    xcf_check_error (xcf_save_prop (info, image, PROP_SELECTION, error));

  xcf_check_error (xcf_save_prop (info, image, PROP_OPACITY, error,
                                  gimp_channel_get_opacity (channel)));
  xcf_check_error (xcf_save_prop (info, image, PROP_VISIBLE, error,
                                  gimp_item_get_visible (GIMP_ITEM (channel))));
  xcf_check_error (xcf_save_prop (info, image, PROP_LINKED, error,
                                  gimp_item_get_linked (GIMP_ITEM (channel))));
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_CONTENT, error,
                                  gimp_item_get_lock_content (GIMP_ITEM (channel))));
  xcf_check_error (xcf_save_prop (info, image, PROP_SHOW_MASKED, error,
                                  gimp_channel_get_show_masked (channel)));

  gimp_rgb_get_uchar (&channel->color, &col[0], &col[1], &col[2]);
  xcf_check_error (xcf_save_prop (info, image, PROP_COLOR, error, col));

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
  guint32 size;
  va_list args;

  GError *tmp_error = NULL;

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
        guint32  n_colors;
        guchar  *colors;

        n_colors = va_arg (args, guint32);
        colors = va_arg (args, guchar *);
        size = 4 + n_colors * 3;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int32_check_error (info, &n_colors, 1);
        xcf_write_int8_check_error  (info, colors, n_colors * 3);
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
      {
        guint32 dummy;

        dummy = 0;
        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        info->floating_sel_offset = info->cp;
        xcf_write_int32_check_error (info, &dummy, 1);
      }
      break;

    case PROP_OPACITY:
      {
        gdouble opacity;
        guint32 uint_opacity;

        opacity = va_arg (args, gdouble);

        uint_opacity = opacity * 255.999;

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int32_check_error (info, &uint_opacity, 1);
      }
      break;

    case PROP_MODE:
      {
        gint32 mode;

        mode = va_arg (args, gint32);
        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int32_check_error (info, (guint32 *) &mode, 1);
      }
      break;

    case PROP_VISIBLE:
      {
        guint32 visible;

        visible = va_arg (args, guint32);
        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int32_check_error (info, &visible, 1);
      }
      break;

    case PROP_LINKED:
      {
        guint32 linked;

        linked = va_arg (args, guint32);
        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int32_check_error (info, &linked, 1);
      }
      break;

    case PROP_LOCK_CONTENT:
      {
        guint32 lock_content;

        lock_content = va_arg (args, guint32);
        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int32_check_error (info, &lock_content, 1);
      }
      break;

    case PROP_LOCK_ALPHA:
      {
        guint32 lock_alpha;

        lock_alpha = va_arg (args, guint32);
        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int32_check_error (info, &lock_alpha, 1);
      }
      break;

    case PROP_APPLY_MASK:
      {
        guint32 apply_mask;

        apply_mask = va_arg (args, guint32);
        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int32_check_error (info, &apply_mask, 1);
      }
      break;

    case PROP_EDIT_MASK:
      {
        guint32 edit_mask;

        edit_mask = va_arg (args, guint32);
        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int32_check_error (info, &edit_mask, 1);
      }
      break;

    case PROP_SHOW_MASK:
      {
        guint32 show_mask;

        show_mask = va_arg (args, guint32);
        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int32_check_error (info, &show_mask, 1);
      }
      break;

    case PROP_SHOW_MASKED:
      {
        guint32 show_masked;

        show_masked = va_arg (args, guint32);
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
        guchar *color;

        color = va_arg (args, guchar*);
        size = 3;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int8_check_error  (info, color, 3);
      }
      break;

    case PROP_COMPRESSION:
      {
        guint8 compression;

        compression = (guint8) va_arg (args, guint32);
        size = 1;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int8_check_error  (info, &compression, 1);
      }
      break;

    case PROP_GUIDES:
      {
        GList *guides;
        gint   n_guides;

        guides = va_arg (args, GList *);
        n_guides = g_list_length (guides);

        size = n_guides * (4 + 1);

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        for (; guides; guides = g_list_next (guides))
          {
            GimpGuide *guide    = guides->data;
            gint32     position = gimp_guide_get_position (guide);
            gint8      orientation;

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
      break;

    case PROP_SAMPLE_POINTS:
      {
        GList *sample_points;
        gint   n_sample_points;

        sample_points = va_arg (args, GList *);
        n_sample_points = g_list_length (sample_points);

        size = n_sample_points * (4 + 4);

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        for (; sample_points; sample_points = g_list_next (sample_points))
          {
            GimpSamplePoint *sample_point = sample_points->data;
            gint32           x, y;

            x = sample_point->x;
            y = sample_point->y;

            xcf_write_int32_check_error (info, (guint32 *) &x, 1);
            xcf_write_int32_check_error (info, (guint32 *) &y, 1);
          }
      }
      break;

    case PROP_RESOLUTION:
      {
        gfloat xresolution, yresolution;

        /* we pass in floats,
           but they are promoted to double by the compiler */
        xresolution =  va_arg (args, double);
        yresolution =  va_arg (args, double);

        size = 4*2;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);

        xcf_write_float_check_error (info, &xresolution, 1);
        xcf_write_float_check_error (info, &yresolution, 1);
      }
      break;

    case PROP_TATTOO:
      {
        guint32 tattoo;

        tattoo =  va_arg (args, guint32);
        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int32_check_error (info, &tattoo, 1);
      }
      break;

    case PROP_PARASITES:
      {
        GimpParasiteList *list;

        list = va_arg (args, GimpParasiteList *);

        if (gimp_parasite_list_persistent_length (list) > 0)
          {
            guint32 base, length = 0;
            long    pos;

            xcf_write_prop_type_check_error (info, prop_type);

            /* because we don't know how much room the parasite list will take
             * we save the file position and write the length later
             */
            pos = info->cp;
            xcf_write_int32_check_error (info, &length, 1);
            base = info->cp;

            xcf_check_error (xcf_save_parasite_list (info, list, error));

            length = info->cp - base;
            /* go back to the saved position and write the length */
            xcf_check_error (xcf_seek_pos (info, pos, error));
            xcf_write_int32 (info->fp, &length, 1, &tmp_error);
            if (tmp_error)
              {
                g_propagate_error (error, tmp_error);
                return FALSE;
              }

            xcf_check_error (xcf_seek_pos (info, base + length, error));
          }
      }
      break;

    case PROP_UNIT:
      {
        guint32 unit;

        unit = va_arg (args, guint32);

        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int32_check_error (info, &unit, 1);
      }
      break;

    case PROP_PATHS:
      {
        guint32 base, length = 0;
        glong   pos;

        xcf_write_prop_type_check_error (info, prop_type);

        /* because we don't know how much room the paths list will take
         * we save the file position and write the length later
         */
        pos = info->cp;
        xcf_write_int32_check_error (info, &length, 1);

        base = info->cp;

        xcf_check_error (xcf_save_old_paths (info, image, error));

        length = info->cp - base;

        /* go back to the saved position and write the length */
        xcf_check_error (xcf_seek_pos (info, pos, error));
        xcf_write_int32 (info->fp, &length, 1, &tmp_error);
        if (tmp_error)
          {
            g_propagate_error (error, tmp_error);
            return FALSE;
          }

        xcf_check_error (xcf_seek_pos (info, base + length, error));
      }
      break;

    case PROP_USER_UNIT:
      {
        GimpUnit     unit;
        const gchar *unit_strings[5];
        gfloat       factor;
        guint32      digits;

        unit = va_arg (args, guint32);

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
        xcf_write_float_check_error (info, &factor, 1);
        xcf_write_int32_check_error (info, &digits, 1);
        xcf_write_string_check_error (info, (gchar **) unit_strings, 5);
      }
      break;

    case PROP_VECTORS:
      {
        guint32 base, length = 0;
        glong   pos;

        xcf_write_prop_type_check_error (info, prop_type);

        /* because we don't know how much room the paths list will take
         * we save the file position and write the length later
         */
        pos = info->cp;
        xcf_write_int32_check_error (info, &length, 1);

        base = info->cp;

        xcf_check_error (xcf_save_vectors (info, image, error));

        length = info->cp - base;

        /* go back to the saved position and write the length */
        xcf_check_error (xcf_seek_pos (info, pos, error));
        xcf_write_int32 (info->fp, &length, 1, &tmp_error);
        if (tmp_error)
          {
            g_propagate_error (error, tmp_error);
            return FALSE;
          }

        xcf_check_error (xcf_seek_pos (info, base + length, error));
      }
      break;

    case PROP_TEXT_LAYER_FLAGS:
      {
        guint32 flags;

        flags = va_arg (args, guint32);
        size = 4;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int32_check_error (info, &flags, 1);
      }
      break;

    case PROP_ITEM_PATH:
      {
        GList *path;

        path = va_arg (args, GList *);
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
        guint32 flags;

        flags = va_arg (args, guint32);
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
  guint32      saved_pos;
  guint32      offset;
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
      xcf_write_int32_check_error (info, &saved_pos, 1);
      xcf_check_error (xcf_seek_pos (info, saved_pos, error));
    }

  /* write out the width, height and image type information for the layer */
  value = gimp_item_get_width (GIMP_ITEM (layer));
  xcf_write_int32_check_error (info, &value, 1);

  value = gimp_item_get_height (GIMP_ITEM (layer));
  xcf_write_int32_check_error (info, &value, 1);

  value = gimp_drawable_type (GIMP_DRAWABLE (layer));
  xcf_write_int32_check_error (info, &value, 1);

  /* write out the layers name */
  string = gimp_object_get_name (layer);
  xcf_write_string_check_error (info, (gchar **) &string, 1);

  /* write out the layer properties */
  xcf_save_layer_props (info, image, layer, error);

  /*  save the current position which is where the hierarchy offset
   *  will be stored.
   */

  /*  write out the layer tile hierarchy  */
  offset = info->cp + 8;
  xcf_write_int32_check_error (info, &offset, 1);

  saved_pos = info->cp;
  xcf_check_error (xcf_seek_pos (info, offset, error));

  xcf_check_error (xcf_save_hierarchy (info,
                                       gimp_drawable_get_tiles (GIMP_DRAWABLE (layer)),
                                       error));

  offset = info->cp;
  xcf_check_error (xcf_seek_pos (info, saved_pos, error));

  /* write out the layer mask */
  if (gimp_layer_get_mask (layer))
    {
      GimpLayerMask *mask = gimp_layer_get_mask (layer);

      xcf_write_int32_check_error (info, &offset, 1);
      xcf_check_error (xcf_seek_pos (info, offset, error));

      xcf_check_error (xcf_save_channel (info, image, GIMP_CHANNEL (mask),
                                         error));
    }
  else
    {
      xcf_write_int32_check_error (info, &zero, 1);
      xcf_check_error (xcf_seek_pos (info, offset, error));
    }

  return TRUE;
}

static gboolean
xcf_save_channel (XcfInfo      *info,
                  GimpImage    *image,
                  GimpChannel  *channel,
                  GError      **error)
{
  guint32      saved_pos;
  guint32      offset;
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
      xcf_write_int32_check_error (info, &saved_pos, 1);
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

  offset = info->cp + 4;
  xcf_write_int32_check_error (info, &offset, 1);

  xcf_check_error (xcf_save_hierarchy (info,
                                       gimp_drawable_get_tiles (GIMP_DRAWABLE (channel)),
                                       error));

  return TRUE;
}

static gint
xcf_calc_levels (gint size,
                 gint tile_size)
{
  int levels;

  levels = 1;
  while (size > tile_size)
    {
      size /= 2;
      levels += 1;
    }

  return levels;
}


static gboolean
xcf_save_hierarchy (XcfInfo      *info,
                    TileManager  *tiles,
                    GError      **error)
{
  guint32 saved_pos;
  guint32 offset;
  guint32 width;
  guint32 height;
  guint32 bpp;
  gint    i;
  gint    nlevels;
  gint    tmp1, tmp2;

  GError *tmp_error = NULL;

  width  = tile_manager_width (tiles);
  height = tile_manager_height (tiles);
  bpp    = tile_manager_bpp (tiles);

  xcf_write_int32_check_error (info, (guint32 *) &width, 1);
  xcf_write_int32_check_error (info, (guint32 *) &height, 1);
  xcf_write_int32_check_error (info, (guint32 *) &bpp, 1);

  saved_pos = info->cp;

  tmp1 = xcf_calc_levels (width, TILE_WIDTH);
  tmp2 = xcf_calc_levels (height, TILE_HEIGHT);
  nlevels = MAX (tmp1, tmp2);

  offset = info->cp + (1 + nlevels) * 4;

  for (i = 0; i < nlevels; i++)
    {
      xcf_write_int32_check_error (info, &offset, 1);

      saved_pos = info->cp;
      xcf_check_error (xcf_seek_pos (info, offset, error));

      if (i == 0)
        {
          /* write out the level. */
          xcf_check_error (xcf_save_level (info, tiles, error));
        }
      else
        {
          /* fake an empty level */
          tmp1 = 0;
          width  /= 2;
          height /= 2;
          xcf_write_int32_check_error (info, (guint32 *) &width,  1);
          xcf_write_int32_check_error (info, (guint32 *) &height, 1);
          xcf_write_int32_check_error (info, (guint32 *) &tmp1,   1);
        }

      offset = info->cp;
      xcf_check_error (xcf_seek_pos (info, saved_pos, error));
    }

  /* write out a '0' offset position to indicate the end
   *  of the level offsets.
   */
  xcf_write_int32_check_error (info, &zero, 1);
  xcf_check_error (xcf_seek_pos (info, offset, error));

  return TRUE;
}

static gboolean
xcf_save_level (XcfInfo      *info,
                TileManager  *level,
                GError      **error)
{
  guint32  saved_pos;
  guint32  offset;
  guint32  width;
  guint32  height;
  guint    ntiles;
  gint     i;
  guchar  *rlebuf;

  GError *tmp_error = NULL;

  width  = tile_manager_width (level);
  height = tile_manager_height (level);

  xcf_write_int32_check_error (info, (guint32 *) &width, 1);
  xcf_write_int32_check_error (info, (guint32 *) &height, 1);

  /* allocate a temporary buffer to store the rle data before it is
     written to disk */
  rlebuf =
    g_malloc (TILE_WIDTH * TILE_HEIGHT * tile_manager_bpp (level) * 1.5);
  ntiles = level->ntile_rows * level->ntile_cols;
  offset = info->cp + (ntiles + 1) * 4;

  if (level->tiles)
    {
      for (i = 0; i < ntiles; i++)
        {
          xcf_write_int32_check_error (info, &offset, 1);

          saved_pos = info->cp;
          xcf_check_error (xcf_seek_pos (info, offset, error));

          /* write out the tile. */
          switch (info->compression)
            {
            case COMPRESS_NONE:
              xcf_check_error (xcf_save_tile (info, level->tiles[i], error));
              break;
            case COMPRESS_RLE:
              xcf_check_error (xcf_save_tile_rle (info, level->tiles[i],
                               rlebuf, error));
              break;
            case COMPRESS_ZLIB:
              g_error ("xcf: zlib compression unimplemented");
              break;
            case COMPRESS_FRACTAL:
              g_error ("xcf: fractal compression unimplemented");
              break;
            }

          offset = info->cp;
          xcf_check_error (xcf_seek_pos (info, saved_pos, error));
        }
    }

  g_free (rlebuf);

  /* write out a '0' offset position to indicate the end
   *  of the level offsets.
   */
  xcf_write_int32_check_error (info, &zero, 1);
  xcf_check_error (xcf_seek_pos (info, offset, error));

  return TRUE;
}

static gboolean
xcf_save_tile (XcfInfo  *info,
               Tile     *tile,
               GError  **error)
{
  GError *tmp_error = NULL;

  tile_lock (tile);
  xcf_write_int8_check_error (info, tile_data_pointer (tile, 0, 0),
                              tile_size (tile));
  tile_release (tile, FALSE);

  return TRUE;
}

static gboolean
xcf_save_tile_rle (XcfInfo  *info,
                   Tile     *tile,
                   guchar   *rlebuf,
                   GError  **error)
{
  GError *tmp_error = NULL;
  gint    len       = 0;
  gint    bpp;
  gint    i, j;

  tile_lock (tile);

  bpp = tile_bpp (tile);

  for (i = 0; i < bpp; i++)
    {
      const guchar *data = (const guchar *) tile_data_pointer (tile, 0, 0) + i;

      gint  state  = 0;
      gint  length = 0;
      gint  count  = 0;
      gint  size   = tile_ewidth (tile) * tile_eheight (tile);
      guint last   = -1;

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

      if (count != (tile_ewidth (tile) * tile_eheight (tile)))
        g_message ("xcf: uh oh! xcf rle tile saving error: %d", count);
    }

  xcf_write_int8_check_error (info, rlebuf, len);
  tile_release (tile, FALSE);

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
      guint32                 linked;
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
       * linked (gint)
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
       * we already saved the number of paths and I wont start seeking
       * around to fix that cruft  */

      name     = (gchar *) gimp_object_get_name (vectors);
      linked   = gimp_item_get_linked (GIMP_ITEM (vectors));
      state    = closed ? 4 : 2;  /* EDIT : ADD  (editing state, 1.2 compat) */
      version  = 3;
      pathtype = 1;  /* BEZIER  (1.2 compat) */
      tattoo   = gimp_item_get_tattoo (GIMP_ITEM (vectors));

      xcf_write_string_check_error (info, &name,       1);
      xcf_write_int32_check_error  (info, &linked,     1);
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
      num_strokes   = g_list_length (vectors->strokes);

      xcf_write_string_check_error (info, (gchar **) &name, 1);
      xcf_write_int32_check_error  (info, &tattoo,          1);
      xcf_write_int32_check_error  (info, &visible,         1);
      xcf_write_int32_check_error  (info, &linked,          1);
      xcf_write_int32_check_error  (info, &num_parasites,   1);
      xcf_write_int32_check_error  (info, &num_strokes,     1);

      xcf_check_error (xcf_save_parasite_list (info, parasites, error));

      for (stroke_list = g_list_first (vectors->strokes);
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

              xcf_write_int32_check_error (info, &type, 1);
              xcf_write_float_check_error (info, coords, num_axes);
            }

          g_array_free (control_points, TRUE);
        }
    }

  return TRUE;
}
