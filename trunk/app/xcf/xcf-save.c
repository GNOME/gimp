/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdio.h>
#include <string.h> /* strcpy, strlen */

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core/core-types.h"

#include "base/tile.h"
#include "base/tile-manager.h"
#include "base/tile-manager-private.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpdrawable.h"
#include "core/gimpgrid.h"
#include "core/gimpguide.h"
#include "core/gimpimage.h"
#include "core/gimpimage-grid.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-sample-points.h"
#include "core/gimplayer.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimplayermask.h"
#include "core/gimplist.h"
#include "core/gimpparasitelist.h"
#include "core/gimpprogress.h"
#include "core/gimpsamplepoint.h"
#include "core/gimpunit.h"

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

#define xcf_write_int32_print_error(info, data, count) G_STMT_START { \
  info->cp += xcf_write_int32 (info->fp, data, count, &error); \
  if (error)                                                   \
    {                                                          \
      gimp_message (info->gimp, G_OBJECT (info->progress),     \
                    GIMP_MESSAGE_ERROR,                        \
                    _("Error saving XCF file: %s"),            \
                    error->message);                           \
      return FALSE;                                            \
    }                                                          \
  } G_STMT_END

#define xcf_write_int8_print_error(info, data, count) G_STMT_START { \
  info->cp += xcf_write_int8 (info->fp, data, count, &error); \
  if (error)                                                  \
    {                                                         \
      gimp_message (info->gimp, G_OBJECT (info->progress),    \
                    GIMP_MESSAGE_ERROR,                       \
                    _("Error saving XCF file: %s"),           \
                    error->message);                          \
      return FALSE;                                           \
    }                                                         \
  } G_STMT_END

#define xcf_write_float_print_error(info, data, count) G_STMT_START { \
  info->cp += xcf_write_float (info->fp, data, count, &error); \
  if (error)                                                   \
    {                                                          \
      gimp_message (info->gimp, G_OBJECT (info->progress),     \
                    GIMP_MESSAGE_ERROR,                        \
                    _("Error saving XCF file: %s"),            \
                    error->message);                           \
      return FALSE;                                            \
    }                                                          \
  } G_STMT_END

#define xcf_write_string_print_error(info, data, count) G_STMT_START { \
  info->cp += xcf_write_string (info->fp, data, count, &error); \
  if (error)                                                    \
    {                                                           \
      gimp_message (info->gimp, G_OBJECT (info->progress),      \
                    GIMP_MESSAGE_ERROR,                         \
                    _("Error saving XCF file: %s"),             \
                    error->message);                            \
      return FALSE;                                             \
    }                                                           \
  } G_STMT_END

#define xcf_write_prop_type_check_error(info, prop_type) G_STMT_START { \
  guint32 _prop_int32 = prop_type;                     \
  xcf_write_int32_check_error (info, &_prop_int32, 1); \
  } G_STMT_END

#define xcf_write_prop_type_print_error(info, prop_type) G_STMT_START { \
  guint32 _prop_int32 = prop_type;                     \
  xcf_write_int32_print_error (info, &_prop_int32, 1); \
  } G_STMT_END

#define xcf_check_error(x) G_STMT_START { \
  if (! (x))                                                  \
    return FALSE;                                             \
  } G_STMT_END

#define xcf_print_error(info, x) G_STMT_START {               \
  if (! (x))                                                  \
    {                                                         \
      gimp_message (info->gimp, G_OBJECT (info->progress),    \
                    GIMP_MESSAGE_ERROR,                       \
                    _("Error saving XCF file: %s"),           \
                    error->message);                          \
      return FALSE;                                           \
    }                                                         \
  } G_STMT_END

#define xcf_progress_update(info) G_STMT_START  \
  {                                             \
    progress++;                                 \
    if (info->progress)                         \
      gimp_progress_set_value (info->progress,  \
                               (gdouble) progress / (gdouble) max_progress); \
  } G_STMT_END


void
xcf_save_choose_format (XcfInfo   *info,
                        GimpImage *image)
{
  GimpLayer *layer;
  GList     *list;

  gint save_version = 0;                /* default to oldest */

  if (image->cmap)
    save_version = 1;                   /* need version 1 for colormaps */

  for (list = GIMP_LIST (image->layers)->list;
       list && save_version < 2;
       list = g_list_next (list))
    {
      layer = GIMP_LAYER (list->data);

      switch (layer->mode)
        {
          /* new layer modes not supported by gimp-1.2 */
        case GIMP_SOFTLIGHT_MODE:
        case GIMP_GRAIN_EXTRACT_MODE:
        case GIMP_GRAIN_MERGE_MODE:
        case GIMP_COLOR_ERASE_MODE:
          save_version = 2;
          break;

        default:
          break;
        }
    }

  info->file_version = save_version;
}

gint
xcf_save_image (XcfInfo   *info,
                GimpImage *image)
{
  GimpLayer   *layer;
  GimpLayer   *floating_layer;
  GimpChannel *channel;
  GList       *list;
  guint32      saved_pos;
  guint32      offset;
  guint32      value;
  guint        nlayers;
  guint        nchannels;
  guint        progress = 0;
  guint        max_progress;
  gboolean     have_selection;
  gint         t1, t2, t3, t4;
  gchar        version_tag[16];
  GError      *error = NULL;

  floating_layer = gimp_image_floating_sel (image);
  if (floating_layer)
    floating_sel_relax (floating_layer, FALSE);

  /* write out the tag information for the image */
  if (info->file_version > 0)
    {
      sprintf (version_tag, "gimp xcf v%03d", info->file_version);
    }
  else
    {
      strcpy (version_tag, "gimp xcf file");
    }

  xcf_write_int8_print_error  (info, (guint8 *) version_tag, 14);

  /* write out the width, height and image type information for the image */
  xcf_write_int32_print_error (info, (guint32 *) &image->width, 1);
  xcf_write_int32_print_error (info, (guint32 *) &image->height, 1);

  value = image->base_type;
  xcf_write_int32_print_error (info, &value, 1);

  /* determine the number of layers and channels in the image */
  nlayers   = (guint) gimp_container_num_children (image->layers);
  nchannels = (guint) gimp_container_num_children (image->channels);

  max_progress = 1 + nlayers + nchannels;

  /* check and see if we have to save out the selection */
  have_selection = gimp_channel_bounds (gimp_image_get_mask (image),
                                        &t1, &t2, &t3, &t4);
  if (have_selection)
    nchannels += 1;

  /* write the property information for the image.
   */

  xcf_print_error (info, xcf_save_image_props (info, image, &error));

  xcf_progress_update (info);

  /* save the current file position as it is the start of where
   *  we place the layer offset information.
   */
  saved_pos = info->cp;

  /* seek to after the offset lists */
  xcf_print_error (info, xcf_seek_pos (info,
                                       info->cp + (nlayers + nchannels + 2) * 4,
                                       &error));

  for (list = GIMP_LIST (image->layers)->list;
       list;
       list = g_list_next (list))
    {
      layer = list->data;

      /* save the start offset of where we are writing
       *  out the next layer.
       */
      offset = info->cp;

      /* write out the layer. */
      xcf_print_error (info, xcf_save_layer (info, image, layer, &error));

      xcf_progress_update (info);

      /* seek back to where we are to write out the next
       *  layer offset and write it out.
       */
      xcf_print_error (info, xcf_seek_pos (info, saved_pos, &error));
      xcf_write_int32_print_error (info, &offset, 1);

      /* increment the location we are to write out the
       *  next offset.
       */
      saved_pos = info->cp;

      /* seek to the end of the file which is where
       *  we will write out the next layer.
       */
      xcf_print_error (info, xcf_seek_end (info, &error));
    }

  /* write out a '0' offset position to indicate the end
   *  of the layer offsets.
   */
  offset = 0;
  xcf_print_error (info, xcf_seek_pos (info, saved_pos, &error));
  xcf_write_int32_print_error (info, &offset, 1);
  saved_pos = info->cp;
  xcf_print_error (info, xcf_seek_end (info, &error));

  list = GIMP_LIST (image->channels)->list;

  while (list || have_selection)
    {
      if (list)
        {
          channel = list->data;

          list = g_list_next (list);
        }
      else
        {
          channel = image->selection_mask;
          have_selection = FALSE;
        }

      /* save the start offset of where we are writing
       *  out the next channel.
       */
      offset = info->cp;

      /* write out the layer. */
      xcf_print_error (info, xcf_save_channel (info, image, channel, &error));

      xcf_progress_update (info);

      /* seek back to where we are to write out the next
       *  channel offset and write it out.
       */
      xcf_print_error (info, xcf_seek_pos (info, saved_pos, &error));
      xcf_write_int32_print_error (info, &offset, 1);

      /* increment the location we are to write out the
       *  next offset.
       */
      saved_pos = info->cp;

      /* seek to the end of the file which is where
       *  we will write out the next channel.
       */
      xcf_print_error (info, xcf_seek_end (info, &error));
    }

  /* write out a '0' offset position to indicate the end
   *  of the channel offsets.
   */
  offset = 0;
  xcf_print_error (info, xcf_seek_pos (info, saved_pos, &error));
  xcf_write_int32_print_error (info, &offset, 1);
  saved_pos = info->cp;

  if (floating_layer)
    floating_sel_rigor (floating_layer, FALSE);

  return !ferror(info->fp);
}

static gboolean
xcf_save_image_props (XcfInfo    *info,
                      GimpImage  *image,
                      GError    **error)
{
  GimpParasite *parasite = NULL;
  GimpUnit      unit     = gimp_image_get_unit (image);

  /* check and see if we should save the colormap property */
  if (image->cmap)
    xcf_check_error (xcf_save_prop (info, image, PROP_COLORMAP, error,
                                    image->num_cols, image->cmap));

  if (info->compression != COMPRESS_NONE)
    xcf_check_error (xcf_save_prop (info, image, PROP_COMPRESSION,
                                    error, info->compression));

  if (image->guides)
    xcf_check_error (xcf_save_prop (info, image, PROP_GUIDES,
                                    error, image->guides));

  if (image->sample_points)
    xcf_check_error (xcf_save_prop (info, image, PROP_SAMPLE_POINTS,
                                    error, image->sample_points));

  xcf_check_error (xcf_save_prop (info, image, PROP_RESOLUTION, error,
                                  image->xresolution, image->yresolution));

  xcf_check_error (xcf_save_prop (info, image, PROP_TATTOO, error,
                                  image->tattoo_state));

  if (unit < _gimp_unit_get_number_of_built_in_units (image->gimp))
    xcf_check_error (xcf_save_prop (info, image, PROP_UNIT, error, unit));

  if (gimp_container_num_children (image->vectors) > 0)
    {
      if (gimp_vectors_compat_is_compatible (image))
        xcf_check_error (xcf_save_prop (info, image, PROP_PATHS, error));
      else
        xcf_check_error (xcf_save_prop (info, image, PROP_VECTORS, error));
    }

  if (unit >= _gimp_unit_get_number_of_built_in_units (image->gimp))
    xcf_check_error (xcf_save_prop (info, image, PROP_USER_UNIT, error, unit));

  if (GIMP_IS_GRID (image->grid))
    {
      GimpGrid *grid = gimp_image_get_grid (image);

      parasite = gimp_grid_to_parasite (grid);
      gimp_parasite_list_add (GIMP_IMAGE (image)->parasites, parasite);
    }

  if (gimp_parasite_list_length (GIMP_IMAGE (image)->parasites) > 0)
    {
      xcf_check_error (xcf_save_prop (info, image, PROP_PARASITES, error,
                                      GIMP_IMAGE (image)->parasites));
    }

  if (parasite)
    {
      gimp_parasite_list_remove (GIMP_IMAGE (image)->parasites,
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
  GimpParasite *parasite = NULL;

  if (layer == gimp_image_get_active_layer (image))
    xcf_check_error (xcf_save_prop (info, image, PROP_ACTIVE_LAYER, error));

  if (layer == gimp_image_floating_sel (image))
    {
      info->floating_sel_drawable = layer->fs.drawable;
      xcf_check_error (xcf_save_prop (info, image, PROP_FLOATING_SELECTION,
                                      error));
    }

  xcf_check_error (xcf_save_prop (info, image, PROP_OPACITY, error,
                                  layer->opacity));
  xcf_check_error (xcf_save_prop (info, image, PROP_VISIBLE, error,
                                  gimp_item_get_visible (GIMP_ITEM (layer))));
  xcf_check_error (xcf_save_prop (info, image, PROP_LINKED, error,
                                  gimp_item_get_linked (GIMP_ITEM (layer))));
  xcf_check_error (xcf_save_prop (info, image, PROP_LOCK_ALPHA,
                                  error, layer->lock_alpha));

  if (layer->mask)
    {
      xcf_check_error (xcf_save_prop (info, image, PROP_APPLY_MASK,
                                      error, layer->mask->apply_mask));
      xcf_check_error (xcf_save_prop (info, image, PROP_EDIT_MASK,
                                      error, layer->mask->edit_mask));
      xcf_check_error (xcf_save_prop (info, image, PROP_SHOW_MASK,
                                      error, layer->mask->show_mask));
    }
  else
    {
      xcf_check_error (xcf_save_prop (info, image, PROP_APPLY_MASK,
                                      error, FALSE));
      xcf_check_error (xcf_save_prop (info, image, PROP_EDIT_MASK,
                                      error, FALSE));
      xcf_check_error (xcf_save_prop (info, image, PROP_SHOW_MASK,
                                      error, FALSE));
    }

  xcf_check_error (xcf_save_prop (info, image, PROP_OFFSETS, error,
                                  GIMP_ITEM (layer)->offset_x,
                                  GIMP_ITEM (layer)->offset_y));
  xcf_check_error (xcf_save_prop (info, image, PROP_MODE, error,
                                  layer->mode));
  xcf_check_error (xcf_save_prop (info, image, PROP_TATTOO, error,
                                  GIMP_ITEM (layer)->tattoo));

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

  if (gimp_parasite_list_length (GIMP_ITEM (layer)->parasites) > 0)
    {
      xcf_check_error (xcf_save_prop (info, image, PROP_PARASITES, error,
                                      GIMP_ITEM (layer)->parasites));
    }

  if (parasite)
    {
      gimp_parasite_list_remove (GIMP_ITEM (layer)->parasites,
                                 gimp_parasite_name (parasite));
      gimp_parasite_free (parasite);
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
  guchar col[3];

  if (channel == gimp_image_get_active_channel (image))
    xcf_check_error (xcf_save_prop (info, image, PROP_ACTIVE_CHANNEL, error));

  if (channel == image->selection_mask)
    xcf_check_error (xcf_save_prop (info, image, PROP_SELECTION, error));

  xcf_check_error (xcf_save_prop (info, image, PROP_OPACITY, error,
                                  channel->color.a));
  xcf_check_error (xcf_save_prop (info, image, PROP_VISIBLE, error,
                                  gimp_item_get_visible (GIMP_ITEM (channel))));
  xcf_check_error (xcf_save_prop (info, image, PROP_LINKED, error,
                                  gimp_item_get_linked (GIMP_ITEM (channel))));
  xcf_check_error (xcf_save_prop (info, image, PROP_SHOW_MASKED, error,
                                  channel->show_masked));

  gimp_rgb_get_uchar (&channel->color, &col[0], &col[1], &col[2]);
  xcf_check_error (xcf_save_prop (info, image, PROP_COLOR, error, col));

  xcf_check_error (xcf_save_prop (info, image, PROP_TATTOO, error,
                   GIMP_ITEM (channel)->tattoo));

  if (gimp_parasite_list_length (GIMP_ITEM (channel)->parasites) > 0)
    xcf_check_error (xcf_save_prop (info, image, PROP_PARASITES, error,
                     GIMP_ITEM (channel)->parasites));

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
        guint32  ncolors;
        guchar  *colors;

        ncolors = va_arg (args, guint32);
        colors = va_arg (args, guchar *);
        size = 4 + ncolors * 3;

        xcf_write_prop_type_check_error (info, prop_type);
        xcf_write_int32_check_error (info, &size, 1);
        xcf_write_int32_check_error (info, &ncolors, 1);
        xcf_write_int8_check_error  (info, colors, ncolors * 3);
      }
      break;

    case PROP_ACTIVE_LAYER:
    case PROP_ACTIVE_CHANNEL:
    case PROP_SELECTION:
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

            xcf_write_int32_check_error (info, (guint32 *) &x,    1);
            xcf_write_int32_check_error (info, (guint32 *) &y,    1);
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
        guint32           base, length;
        long              pos;

        list = va_arg (args, GimpParasiteList *);

        if (gimp_parasite_list_persistent_length (list) > 0)
          {
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

            xcf_check_error (xcf_seek_end (info, error));
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
        guint32 base, length;
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

        xcf_check_error (xcf_seek_end (info, error));
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
        unit_strings[0] = _gimp_unit_get_identifier (image->gimp, unit);
        factor          = _gimp_unit_get_factor (image->gimp, unit);
        digits          = _gimp_unit_get_digits (image->gimp, unit);
        unit_strings[1] = _gimp_unit_get_symbol (image->gimp, unit);
        unit_strings[2] = _gimp_unit_get_abbreviation (image->gimp, unit);
        unit_strings[3] = _gimp_unit_get_singular (image->gimp, unit);
        unit_strings[4] = _gimp_unit_get_plural (image->gimp, unit);

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
        guint32 base, length;
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

        xcf_check_error (xcf_seek_end (info, error));
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
  guint32  saved_pos;
  guint32  offset;
  guint32  value;
  GError  *tmp_error = NULL;

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
  xcf_write_int32_check_error (info,
                               (guint32 *) &GIMP_ITEM (layer)->width, 1);
  xcf_write_int32_check_error (info,
                               (guint32 *) &GIMP_ITEM (layer)->height, 1);

  value = gimp_drawable_type (GIMP_DRAWABLE (layer));
  xcf_write_int32_check_error (info, &value, 1);

  /* write out the layers name */
  xcf_write_string_check_error (info, &GIMP_OBJECT (layer)->name, 1);

  /* write out the layer properties */
  xcf_save_layer_props (info, image, layer, error);

  /* save the current position which is where the hierarchy offset
   *  will be stored.
   */
  saved_pos = info->cp;

  /* write out the layer tile hierarchy */
  xcf_check_error (xcf_seek_pos (info, info->cp + 8, error));
  offset = info->cp;

  xcf_check_error (xcf_save_hierarchy (info,
                                       GIMP_DRAWABLE(layer)->tiles, error));

  xcf_check_error (xcf_seek_pos (info, saved_pos, error));
  xcf_write_int32_check_error (info, &offset, 1);
  saved_pos = info->cp;

  /* write out the layer mask */
  if (layer->mask)
    {
      xcf_check_error (xcf_seek_end (info, error));
      offset = info->cp;

      xcf_check_error (xcf_save_channel (info,
                                         image, GIMP_CHANNEL(layer->mask),
                                         error));
    }
  else
    offset = 0;

  xcf_check_error (xcf_seek_pos (info, saved_pos, error));
  xcf_write_int32_check_error (info, &offset, 1);

  return TRUE;
}

static gboolean
xcf_save_channel (XcfInfo      *info,
                  GimpImage    *image,
                  GimpChannel  *channel,
                  GError      **error)
{
  guint32 saved_pos;
  guint32 offset;

  GError *tmp_error = NULL;

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
  xcf_write_int32_check_error (info,
                               (guint32 *) &GIMP_ITEM (channel)->width, 1);
  xcf_write_int32_check_error (info,
                               (guint32 *) &GIMP_ITEM (channel)->height, 1);

  /* write out the channels name */
  xcf_write_string_check_error (info, &GIMP_OBJECT (channel)->name, 1);

  /* write out the channel properties */
  xcf_save_channel_props (info, image, channel, error);

  /* save the current position which is where the hierarchy offset
   *  will be stored.
   */
  saved_pos = info->cp;

  /* write out the channel tile hierarchy */
  xcf_check_error (xcf_seek_pos (info, info->cp + 4, error));
  offset = info->cp;

  xcf_check_error (xcf_save_hierarchy (info,
                                       GIMP_DRAWABLE (channel)->tiles, error));

  xcf_check_error (xcf_seek_pos (info, saved_pos, error));
  xcf_write_int32_check_error (info, &offset, 1);
  saved_pos = info->cp;

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

  xcf_check_error (xcf_seek_pos (info, info->cp + (1 + nlevels) * 4, error));

  for (i = 0; i < nlevels; i++)
    {
      offset = info->cp;

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

      /* seek back to where we are to write out the next
       *  level offset and write it out.
       */
      xcf_check_error (xcf_seek_pos (info, saved_pos, error));
      xcf_write_int32_check_error (info, &offset, 1);

      /* increment the location we are to write out the
       *  next offset.
       */
      saved_pos = info->cp;

      /* seek to the end of the file which is where
       *  we will write out the next level.
       */
      xcf_check_error (xcf_seek_end (info, error));
    }

  /* write out a '0' offset position to indicate the end
   *  of the level offsets.
   */
  offset = 0;
  xcf_check_error (xcf_seek_pos (info, saved_pos, error));
  xcf_write_int32_check_error (info, &offset, 1);

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

  saved_pos = info->cp;

  /* allocate a temporary buffer to store the rle data before it is
     written to disk */
  rlebuf =
    g_malloc (TILE_WIDTH * TILE_HEIGHT * tile_manager_bpp (level) * 1.5);

  if (level->tiles)
    {
      ntiles = level->ntile_rows * level->ntile_cols;
      xcf_check_error (xcf_seek_pos (info, info->cp + (ntiles + 1) * 4, error));

      for (i = 0; i < ntiles; i++)
        {
          /* save the start offset of where we are writing
           *  out the next tile.
           */
          offset = info->cp;

          /* write out the tile. */
          switch (info->compression)
            {
            case COMPRESS_NONE:
              xcf_check_error(xcf_save_tile (info, level->tiles[i], error));
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

          /* seek back to where we are to write out the next
           *  tile offset and write it out.
           */
          xcf_check_error (xcf_seek_pos (info, saved_pos, error));
          xcf_write_int32_check_error (info, &offset, 1);

          /* increment the location we are to write out the
           *  next offset.
           */
          saved_pos = info->cp;

          /* seek to the end of the file which is where
           *  we will write out the next tile.
           */
          xcf_check_error (xcf_seek_end (info, error));
        }
    }

  g_free (rlebuf);

  /* write out a '0' offset position to indicate the end
   *  of the level offsets.
   */
  offset = 0;
  xcf_check_error (xcf_seek_pos (info, saved_pos, error));
  xcf_write_int32_check_error (info, &offset, 1);

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
      GError *tmp_error = NULL;

      xcf_write_string_check_error (info, &parasite->name,  1);
      xcf_write_int32_check_error  (info, &parasite->flags, 1);
      xcf_write_int32_check_error  (info, &parasite->size,  1);
      xcf_write_int8_check_error   (info, parasite->data, parasite->size);
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

  num_paths = gimp_container_num_children (image->vectors);

  active_vectors = gimp_image_get_active_vectors (image);

  if (active_vectors)
    active_index = gimp_container_get_child_index (image->vectors,
                                                   GIMP_OBJECT (active_vectors));

  xcf_write_int32_check_error (info, &active_index, 1);
  xcf_write_int32_check_error (info, &num_paths,    1);

  for (list = GIMP_LIST (image->vectors)->list;
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
       * we already saved the number of paths and I wont start seeking
       * around to fix that cruft  */

      name     = (gchar *) gimp_object_get_name (GIMP_OBJECT (vectors));
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
    active_index = gimp_container_get_child_index (image->vectors,
                                                   GIMP_OBJECT (active_vectors));

  num_paths = gimp_container_num_children (image->vectors);

  xcf_write_int32_check_error (info, &version,      1);
  xcf_write_int32_check_error (info, &active_index, 1);
  xcf_write_int32_check_error (info, &num_paths,    1);

  for (list = GIMP_LIST (image->vectors)->list;
       list;
       list = g_list_next (list))
    {
      GimpVectors      *vectors = list->data;
      GimpParasiteList *parasites;
      gchar            *name;
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

      parasites = GIMP_ITEM (vectors)->parasites;

      name          = (gchar *) gimp_object_get_name (GIMP_OBJECT (vectors));
      visible       = gimp_item_get_visible (GIMP_ITEM (vectors));
      linked        = gimp_item_get_linked (GIMP_ITEM (vectors));
      tattoo        = gimp_item_get_tattoo (GIMP_ITEM (vectors));
      num_parasites = gimp_parasite_list_persistent_length (parasites);
      num_strokes   = g_list_length (vectors->strokes);

      xcf_write_string_check_error (info, &name,          1);
      xcf_write_int32_check_error  (info, &tattoo,        1);
      xcf_write_int32_check_error  (info, &visible,       1);
      xcf_write_int32_check_error  (info, &linked,        1);
      xcf_write_int32_check_error  (info, &num_parasites, 1);
      xcf_write_int32_check_error  (info, &num_strokes,   1);

      xcf_check_error (xcf_save_parasite_list (info, parasites, error));

      for (stroke_list = g_list_first (vectors->strokes);
           stroke_list;
           stroke_list = g_list_next (stroke_list))
        {
          GimpStroke *stroke;
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

          stroke = GIMP_STROKE (stroke_list->data);

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
