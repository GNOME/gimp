/* The GIMP -- an image manipulation program
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

#include "core/gimpchannel.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimplayer.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimplayermask.h"
#include "core/gimplist.h"
#include "core/gimpparasitelist.h"
#include "core/gimpunit.h"

#include "xcf-private.h"
#include "xcf-read.h"
#include "xcf-seek.h"
#include "xcf-write.h"

#include "path.h"
#include "pathP.h"

#include "gimp-intl.h"

static gboolean xcf_save_image_props   (XcfInfo     *info,
				        GimpImage   *gimage,
				        GError     **error);
static gboolean xcf_save_layer_props   (XcfInfo     *info,
				        GimpImage   *gimage,
				        GimpLayer   *layer,
				        GError     **error);
static gboolean xcf_save_channel_props (XcfInfo     *info,
				        GimpImage   *gimage,
				        GimpChannel *channel,
				        GError     **error);
static gboolean xcf_save_prop          (XcfInfo     *info,
				        GimpImage   *gimage,
				        PropType     prop_type,
				        GError     **error,
				        ...);
static gboolean xcf_save_layer         (XcfInfo     *info,
				        GimpImage   *gimage,
				        GimpLayer   *layer,
				        GError     **error);
static gboolean xcf_save_channel       (XcfInfo     *info,
				        GimpImage   *gimage,
				        GimpChannel *channel,
				        GError     **error);
static gboolean xcf_save_hierarchy     (XcfInfo     *info,
				        TileManager *tiles,
				        GError     **error);
static gboolean xcf_save_level         (XcfInfo     *info,
				        TileManager *tiles,
				        GError     **error);
static gboolean xcf_save_tile          (XcfInfo     *info,
				        Tile        *tile,
				        GError     **error);
static gboolean xcf_save_tile_rle      (XcfInfo     *info,
				        Tile        *tile,
				        guchar      *rlebuf,
				        GError     **error);

/* private convieniece macros */
#define xcf_write_int32_check_error(fp, data, count) G_STMT_START { \
  info->cp += xcf_write_int32 (fp, data, count, &tmp_error);  \
  if (tmp_error)                                              \
    {                                                         \
      g_propagate_error (error, tmp_error);                   \
      return FALSE;                                           \
    }                                                         \
  } G_STMT_END

#define xcf_write_int8_check_error(fp, data, count) G_STMT_START { \
  info->cp += xcf_write_int8 (fp, data, count, &tmp_error);   \
  if (tmp_error)                                              \
    {                                                         \
      g_propagate_error (error, tmp_error);                   \
      return FALSE;                                           \
    }                                                         \
  } G_STMT_END

#define xcf_write_float_check_error(fp, data, count) G_STMT_START { \
  info->cp += xcf_write_float (fp, data, count, &tmp_error);  \
  if (tmp_error)                                              \
    {                                                         \
      g_propagate_error (error, tmp_error);                   \
      return FALSE;                                           \
    }                                                         \
  } G_STMT_END

#define xcf_write_string_check_error(fp, data, count) G_STMT_START { \
  info->cp += xcf_write_string (fp, data, count, &tmp_error); \
  if (tmp_error)                                              \
    {                                                         \
      g_propagate_error (error, tmp_error);                   \
      return FALSE;                                           \
    }                                                         \
  } G_STMT_END

#define xcf_write_int32_print_error(fp, data, count) G_STMT_START { \
  info->cp += xcf_write_int32 (fp, data, count, &error);      \
  if (error)                                                  \
    {                                                         \
      g_message (_("Error saving XCF file: %s"),              \
                 error->message);                             \
      return FALSE;                                           \
    }                                                         \
  } G_STMT_END

#define xcf_write_int8_print_error(fp, data, count) G_STMT_START { \
  info->cp += xcf_write_int8 (fp, data, count, &error);       \
  if (error)                                                  \
    {                                                         \
      g_message (_("Error saving XCF file: %s"),              \
                 error->message);                             \
      return FALSE;                                           \
    }                                                         \
  } G_STMT_END

#define xcf_write_float_print_error(fp, data, count) G_STMT_START { \
  info->cp += xcf_write_float (fp, data, count, &error);      \
  if (error)                                                  \
    {                                                         \
      g_message (_("Error saving XCF file: %s"),              \
                 error->message);                             \
      return FALSE;                                           \
    }                                                         \
  } G_STMT_END

#define xcf_write_string_print_error(fp, data, count) G_STMT_START { \
  info->cp += xcf_write_string (fp, data, count, &error);     \
  if (error)                                                  \
    {                                                         \
      g_message (_("Error saving XCF file: %s"),              \
                 error->message);                             \
      return FALSE;                                           \
    }                                                         \
  } G_STMT_END

#define xcf_check_error(x) G_STMT_START { \
  if (! (x))                                                  \
    return FALSE;                                             \
  } G_STMT_END
          
#define xcf_print_error(x) G_STMT_START { \
  if (! (x))                                                  \
    {                                                         \
      g_message (_("Error saving XCF file: %s"),              \
                 error->message);                             \
      return FALSE;                                           \
    }                                                         \
  } G_STMT_END


void
xcf_save_choose_format (XcfInfo   *info,
                        GimpImage *gimage)
{
  GimpLayer *layer;
  GList     *list;

  gint save_version = 0;                /* default to oldest */

  if (gimage->cmap) 
    save_version = 1;                   /* need version 1 for colormaps */

  for (list = GIMP_LIST (gimage->layers)->list;
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
		GimpImage *gimage)
{
  GimpLayer   *layer;
  GimpLayer   *floating_layer;
  GimpChannel *channel;
  guint32      saved_pos;
  guint32      offset;
  guint        nlayers;
  guint        nchannels;
  GList       *list;
  gboolean     have_selection;
  gint         t1, t2, t3, t4;
  gchar        version_tag[14];
  GError      *error = NULL;

  floating_layer = gimp_image_floating_sel (gimage);
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
  xcf_write_int8_print_error  (info->fp, (guint8 *) version_tag, 14);

  /* write out the width, height and image type information for the image */
  xcf_write_int32_print_error (info->fp, (guint32 *) &gimage->width, 1);
  xcf_write_int32_print_error (info->fp, (guint32 *) &gimage->height, 1);
  xcf_write_int32_print_error (info->fp, (guint32 *) &gimage->base_type, 1);

  /* determine the number of layers and channels in the image */
  nlayers   = (guint) gimp_container_num_children (gimage->layers);
  nchannels = (guint) gimp_container_num_children (gimage->channels);

  /* check and see if we have to save out the selection */
  have_selection = gimp_image_mask_bounds (gimage, &t1, &t2, &t3, &t4);
  if (have_selection)
    nchannels += 1;

  /* write the property information for the image.
   */

  xcf_print_error (xcf_save_image_props (info, gimage, &error));

  /* save the current file position as it is the start of where
   *  we place the layer offset information.
   */
  saved_pos = info->cp;

  /* seek to after the offset lists */
  xcf_print_error (xcf_seek_pos (info,
                                 info->cp + (nlayers + nchannels + 2) * 4,
                                 &error));

  for (list = GIMP_LIST (gimage->layers)->list;
       list;
       list = g_list_next (list))
    {
      layer = (GimpLayer *) list->data;

      /* save the start offset of where we are writing
       *  out the next layer.
       */
      offset = info->cp;

      /* write out the layer. */
      xcf_print_error (xcf_save_layer (info, gimage, layer, &error));

      /* seek back to where we are to write out the next
       *  layer offset and write it out.
       */
      xcf_print_error (xcf_seek_pos (info, saved_pos, &error));
      xcf_write_int32_print_error (info->fp, &offset, 1);

      /* increment the location we are to write out the
       *  next offset.
       */
      saved_pos = info->cp;

      /* seek to the end of the file which is where
       *  we will write out the next layer.
       */
      xcf_print_error (xcf_seek_end (info, &error));
    }

  /* write out a '0' offset position to indicate the end
   *  of the layer offsets.
   */
  offset = 0;
  xcf_print_error (xcf_seek_pos (info, saved_pos, &error));
  xcf_write_int32_print_error (info->fp, &offset, 1);
  saved_pos = info->cp;
  xcf_print_error (xcf_seek_end (info, &error));

  list = GIMP_LIST (gimage->channels)->list;

  while (list || have_selection)
    {
      if (list)
	{
	  channel = (GimpChannel *) list->data;

	  list = g_list_next (list);
	}
      else
	{
	  channel = gimage->selection_mask;
	  have_selection = FALSE;
	}

      /* save the start offset of where we are writing
       *  out the next channel.
       */
      offset = info->cp;

      /* write out the layer. */
      xcf_print_error (xcf_save_channel (info, gimage, channel, &error));

      /* seek back to where we are to write out the next
       *  channel offset and write it out.
       */
      xcf_print_error (xcf_seek_pos (info, saved_pos, &error));
      xcf_write_int32_print_error (info->fp, &offset, 1);

      /* increment the location we are to write out the
       *  next offset.
       */
      saved_pos = info->cp;

      /* seek to the end of the file which is where
       *  we will write out the next channel.
       */
      xcf_print_error (xcf_seek_end (info, &error));
    }

  /* write out a '0' offset position to indicate the end
   *  of the channel offsets.
   */
  offset = 0;
  xcf_print_error (xcf_seek_pos (info, saved_pos, &error));
  xcf_write_int32_print_error (info->fp, &offset, 1);
  saved_pos = info->cp;

  if (floating_layer)
    floating_sel_rigor (floating_layer, FALSE);

  return !ferror(info->fp);
}

static gboolean
xcf_save_image_props (XcfInfo   *info,
		      GimpImage *gimage,
		      GError   **error)
{
  /* check and see if we should save the colormap property */
  if (gimage->cmap)
    xcf_check_error (xcf_save_prop (info, gimage, PROP_COLORMAP, error, 
                                    gimage->num_cols, gimage->cmap)); 

  if (info->compression != COMPRESS_NONE)
    xcf_check_error (xcf_save_prop (info, gimage, PROP_COMPRESSION,
                                    error, info->compression)); 

  if (gimage->guides)
    xcf_check_error (xcf_save_prop (info, gimage, PROP_GUIDES, 
                                    error, gimage->guides));

  xcf_check_error (xcf_save_prop (info, gimage, PROP_RESOLUTION, error,
		                  gimage->xresolution, gimage->yresolution));

  xcf_check_error (xcf_save_prop (info, gimage, PROP_TATTOO, error, 
                                  gimage->tattoo_state));

  if (gimp_parasite_list_length(gimage->parasites) > 0)
    xcf_check_error (xcf_save_prop (info, gimage, PROP_PARASITES, 
                                    error, gimage->parasites));

  if (gimage->unit < _gimp_unit_get_number_of_built_in_units (gimage->gimp))
    xcf_check_error (xcf_save_prop (info, gimage, PROP_UNIT, 
                                    error, gimage->unit));

  xcf_check_error (xcf_save_prop (info, gimage, PROP_PATHS, 
                                  error, gimage->paths));

  if (gimage->unit >= _gimp_unit_get_number_of_built_in_units (gimage->gimp))
    xcf_check_error (xcf_save_prop (info, gimage, PROP_USER_UNIT, 
                                    error, gimage->unit));

  xcf_check_error (xcf_save_prop (info, gimage, PROP_END, error));

  return TRUE;
}

static gboolean 
xcf_save_layer_props (XcfInfo   *info,
		      GimpImage *gimage,
		      GimpLayer *layer,
		      GError   **error)
{
  if (layer == gimp_image_get_active_layer (gimage))
    xcf_check_error (xcf_save_prop (info, gimage, PROP_ACTIVE_LAYER, error));

  if (layer == gimp_image_floating_sel (gimage))
    {
      info->floating_sel_drawable = layer->fs.drawable;
      xcf_check_error (xcf_save_prop (info, gimage, PROP_FLOATING_SELECTION,
                                      error));
    }

  xcf_check_error (xcf_save_prop (info, gimage, PROP_OPACITY, error, 
                                  layer->opacity));
  xcf_check_error (xcf_save_prop (info, gimage, PROP_VISIBLE, error,
		                  gimp_drawable_get_visible (GIMP_DRAWABLE (layer))));
  xcf_check_error (xcf_save_prop (info, gimage, PROP_LINKED, 
                                  error, layer->linked));
  xcf_check_error (xcf_save_prop (info, gimage, PROP_PRESERVE_TRANSPARENCY, 
                                  error, layer->preserve_trans));

  if (layer->mask)
    {
      xcf_check_error (xcf_save_prop (info, gimage, PROP_APPLY_MASK, 
                                      error, layer->mask->apply_mask));
      xcf_check_error (xcf_save_prop (info, gimage, PROP_EDIT_MASK,
                                      error, layer->mask->edit_mask));
      xcf_check_error (xcf_save_prop (info, gimage, PROP_SHOW_MASK,
                                      error, layer->mask->show_mask));
    }
  else
    {
      xcf_check_error (xcf_save_prop (info, gimage, PROP_APPLY_MASK, 
                                      error, FALSE));
      xcf_check_error (xcf_save_prop (info, gimage, PROP_EDIT_MASK,
                                      error, FALSE));
      xcf_check_error (xcf_save_prop (info, gimage, PROP_SHOW_MASK,
                                      error, FALSE));
    }

  xcf_check_error (xcf_save_prop (info, gimage, PROP_OFFSETS, error,
		                  GIMP_ITEM (layer)->offset_x,
		                  GIMP_ITEM (layer)->offset_y));
  xcf_check_error (xcf_save_prop (info, gimage, PROP_MODE, error, 
                                  layer->mode));
  xcf_check_error (xcf_save_prop (info, gimage, PROP_TATTOO, error, 
                                  GIMP_ITEM (layer)->tattoo));

  if (gimp_parasite_list_length (GIMP_ITEM (layer)->parasites) > 0)
    xcf_check_error (xcf_save_prop (info, gimage, PROP_PARASITES, error,
		                    GIMP_ITEM (layer)->parasites));

  xcf_check_error (xcf_save_prop (info, gimage, PROP_END, error));

  return TRUE;
}

static gboolean
xcf_save_channel_props (XcfInfo     *info,
			GimpImage   *gimage,
			GimpChannel *channel,
			GError     **error)
{
  guchar col[3];

  if (channel == gimp_image_get_active_channel (gimage))
    xcf_check_error (xcf_save_prop (info, gimage, PROP_ACTIVE_CHANNEL, error));

  if (channel == gimage->selection_mask)
    xcf_check_error (xcf_save_prop (info, gimage, PROP_SELECTION, error));

  xcf_check_error (xcf_save_prop (info, gimage, PROP_OPACITY, error, 
                                  channel->color.a));
  xcf_check_error (xcf_save_prop (info, gimage, PROP_VISIBLE, error,
		                  gimp_drawable_get_visible (GIMP_DRAWABLE (channel))));
  xcf_check_error (xcf_save_prop (info, gimage, PROP_SHOW_MASKED, error, 
                                  channel->show_masked));

  gimp_rgb_get_uchar (&channel->color, &col[0], &col[1], &col[2]);
  xcf_check_error (xcf_save_prop (info, gimage, PROP_COLOR, error, col));

  xcf_check_error (xcf_save_prop (info, gimage, PROP_TATTOO, error, 
                   GIMP_ITEM (channel)->tattoo));

  if (gimp_parasite_list_length (GIMP_ITEM (channel)->parasites) > 0)
    xcf_check_error (xcf_save_prop (info, gimage, PROP_PARASITES, error,
		     GIMP_ITEM (channel)->parasites));

  xcf_check_error (xcf_save_prop (info, gimage, PROP_END, error));

  return TRUE;
}

static void 
xcf_save_parasite (gchar        *key, 
                   GimpParasite *parasite, 
                   XcfInfo      *info)
{
  /* can't fail fast because there is no way to exit g_slist_foreach */

  if (! gimp_parasite_is_persistent (parasite))
    return;

  info->cp += xcf_write_string (info->fp, &parasite->name,  1, NULL);
  info->cp += xcf_write_int32  (info->fp, &parasite->flags, 1, NULL);
  info->cp += xcf_write_int32  (info->fp, &parasite->size,  1, NULL);
  info->cp += xcf_write_int8   (info->fp, parasite->data, parasite->size, NULL);
}

static void 
xcf_save_bz_point (PathPoint *bpt, 
                   XcfInfo   *info)
{
  gfloat xfloat = bpt->x;
  gfloat yfloat = bpt->y;

  gboolean errorure = FALSE, *error;
  
  error = &errorure;
  
  /* type (gint32)
   * x (float)
   * y (float)
   */

  /* can't fail fast because there is no way to exit g_slist_foreach */

  info->cp += xcf_write_int32 (info->fp, &bpt->type, 1, NULL);
  info->cp += xcf_write_float (info->fp, &xfloat,    1, NULL);
  info->cp += xcf_write_float (info->fp, &yfloat,    1, NULL);
}

static void 
xcf_save_path (Path    *bzp, 
               XcfInfo *info)
{
  guint8 state = bzp->state;
  guint32 num_points;
  guint32 closed;
  guint32 version;

  /*
   * name (string)
   * locked (gint)
   * state (gchar)
   * closed (gint)
   * number points (gint)
   * number paths (gint unused)
   * then each point.
   */
 
  /* can't fail fast because there is no way to exit g_slist_foreach */
  
  info->cp += xcf_write_string (info->fp, &bzp->name,    1, NULL);
  info->cp += xcf_write_int32  (info->fp, &bzp->locked,  1, NULL);
  info->cp += xcf_write_int8   (info->fp, &state,        1, NULL);
  
  closed = bzp->closed;
  info->cp += xcf_write_int32 (info->fp, &closed,        1, NULL);
  
  num_points = g_slist_length (bzp->path_details);
  info->cp += xcf_write_int32 (info->fp, &num_points,    1, NULL);
  
  version = 3;
  info->cp += xcf_write_int32 (info->fp, &version,       1, NULL);
  info->cp += xcf_write_int32 (info->fp, &bzp->pathtype, 1, NULL);
  info->cp += xcf_write_int32 (info->fp, &bzp->tattoo,   1, NULL); 
  g_slist_foreach (bzp->path_details, (GFunc) xcf_save_bz_point, info);
}

static gboolean
xcf_save_bzpaths (PathList *paths, 
                  XcfInfo  *info,
	          GError  **error)
{
  guint32 num_paths;

  GError *tmp_error = NULL;
  /* Write out the following:-
   *
   * last_selected_row (gint)
   * number_of_paths (gint)
   *
   * then each path:-
   */
  
  xcf_write_int32_check_error (info->fp,
                               (guint32 *) &paths->last_selected_row, 1);
  
  num_paths = g_slist_length (paths->bz_paths);
  xcf_write_int32_check_error (info->fp, &num_paths, 1);
  g_slist_foreach (paths->bz_paths, (GFunc) xcf_save_path, info);

  return TRUE;
}

static gboolean
xcf_save_prop (XcfInfo   *info,
	       GimpImage *gimage,
	       PropType   prop_type,
	       GError   **error,
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

      xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
      xcf_write_int32_check_error (info->fp, &size, 1);
      break;
    case PROP_COLORMAP:
      {
	guint32 ncolors;
	guchar *colors;

	ncolors = va_arg (args, guint32);
	colors = va_arg (args, guchar*);
	size = 4 + ncolors;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);
	xcf_write_int32_check_error (info->fp, &ncolors, 1);
	xcf_write_int8_check_error  (info->fp, colors, ncolors * 3); 
      }
      break;
    case PROP_ACTIVE_LAYER:
    case PROP_ACTIVE_CHANNEL:
    case PROP_SELECTION:
      size = 0;

      xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1); 
      xcf_write_int32_check_error (info->fp, &size, 1);
      break;
    case PROP_FLOATING_SELECTION:
      {
	guint32 dummy;

	dummy = 0;
	size = 4;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);
	info->floating_sel_offset = info->cp;
	xcf_write_int32_check_error (info->fp, &dummy, 1);
      }
      break;
    case PROP_OPACITY:
      {
	gdouble opacity;
        guint32 uint_opacity;

	opacity = va_arg (args, gdouble);

        uint_opacity = opacity * 255.999;

	size = 4;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1); 
	xcf_write_int32_check_error (info->fp, &uint_opacity, 1); 
      }
      break;
    case PROP_MODE:
      {
	gint32 mode;

	mode = va_arg (args, gint32);
	size = 4;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);
	xcf_write_int32_check_error (info->fp, (guint32 *) &mode, 1);
      }
      break;
    case PROP_VISIBLE:
      {
	guint32 visible;

	visible = va_arg (args, guint32);
	size = 4;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);
	xcf_write_int32_check_error (info->fp, &visible, 1);
      }
      break;
    case PROP_LINKED:
      {
	guint32 linked;

	linked = va_arg (args, guint32);
	size = 4;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);
	xcf_write_int32_check_error (info->fp, &linked, 1);
      }
      break;
    case PROP_PRESERVE_TRANSPARENCY:
      {
	guint32 preserve_trans;

	preserve_trans = va_arg (args, guint32);
	size = 4;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);
	xcf_write_int32_check_error (info->fp, &preserve_trans, 1);
      }
      break;
    case PROP_APPLY_MASK:
      {
	guint32 apply_mask;

	apply_mask = va_arg (args, guint32);
	size = 4;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);
	xcf_write_int32_check_error (info->fp, &apply_mask, 1);
      }
      break;
    case PROP_EDIT_MASK:
      {
	guint32 edit_mask;

	edit_mask = va_arg (args, guint32);
	size = 4;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);
	xcf_write_int32_check_error (info->fp, &edit_mask, 1);
      }
      break;
    case PROP_SHOW_MASK:
      {
	guint32 show_mask;

	show_mask = va_arg (args, guint32);
	size = 4;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);
	xcf_write_int32_check_error (info->fp, &show_mask, 1);
      }
      break;
    case PROP_SHOW_MASKED:
      {
	guint32 show_masked;

	show_masked = va_arg (args, guint32);
	size = 4;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);
	xcf_write_int32_check_error (info->fp, &show_masked, 1);
      }
      break;
    case PROP_OFFSETS:
      {
	gint32 offsets[2];

	offsets[0] = va_arg (args, gint32);
	offsets[1] = va_arg (args, gint32);
	size = 8;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);
	xcf_write_int32_check_error (info->fp, (guint32 *) offsets, 2);
      }
      break;
    case PROP_COLOR:
      {
	guchar *color;

	color = va_arg (args, guchar*);
	size = 3;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);
	xcf_write_int8_check_error  (info->fp, color, 3);
      }
      break;
    case PROP_COMPRESSION:
      {
	guint8 compression;

	compression = (guint8) va_arg (args, guint32);
	size = 1;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1); 
	xcf_write_int32_check_error (info->fp, &size, 1); 
	xcf_write_int8_check_error  (info->fp, &compression, 1);
      }
      break;
    case PROP_GUIDES:
      {
	GList     *guides;
	GimpGuide *guide;
	gint32     position;
	gint8      orientation;
	gint       nguides;

	guides = va_arg (args, GList *);
	nguides = g_list_length (guides);

	size = nguides * (4 + 1);

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);

	for (; guides; guides = g_list_next (guides))
	  {
	    guide = (GimpGuide *) guides->data;

	    position = guide->position;

            switch (guide->orientation)
              {
              case GIMP_ORIENTATION_HORIZONTAL:
                orientation = XCF_ORIENTATION_HORIZONTAL;
                break;

              case GIMP_ORIENTATION_VERTICAL:
                orientation = XCF_ORIENTATION_VERTICAL;
                break;

              default:
                g_warning ("xcf_save_prop: skipping guide with bad orientation");
                continue;
              }

	    xcf_write_int32_check_error (info->fp, (guint32 *) &position,    1);
	    xcf_write_int8_check_error  (info->fp, (guint8 *)  &orientation, 1);
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

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1); 

	xcf_write_float_check_error (info->fp, &xresolution, 1);
	xcf_write_float_check_error (info->fp, &yresolution, 1);
      }
      break;
    case PROP_TATTOO:
      {
	guint32 tattoo;

	tattoo =  va_arg (args, guint32);
	size = 4;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);
	xcf_write_int32_check_error (info->fp, &tattoo, 1);
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
	    xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	    /* because we don't know how much room the parasite list will take
	     * we save the file position and write the length later
	     */
            pos = info->cp;
	    xcf_write_int32_check_error (info->fp, &length, 1);
	    base = info->cp;
	    gimp_parasite_list_foreach (list, 
                                        (GHFunc) xcf_save_parasite, info);
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

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);
	xcf_write_int32_check_error (info->fp, &unit, 1);
      }
      break;
    case PROP_PATHS:
      {
	PathList *paths_list;
	guint32 base, length;
	long pos;

	paths_list =  va_arg (args, PathList *);

	if (paths_list)
          {
            xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
            /* because we don't know how much room the paths list will take
               we save the file position and write the length later 
             */
            pos = info->cp;
            xcf_write_int32_check_error (info->fp, &length, 1);
            base = info->cp;
            xcf_check_error (xcf_save_bzpaths (paths_list, info, error));
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
    case PROP_USER_UNIT:
      {
	GimpUnit     unit;
	const gchar *unit_strings[5];
	gfloat       factor;
	guint32      digits;

	unit = va_arg (args, guint32);

	/* write the entire unit definition */
	unit_strings[0] = _gimp_unit_get_identifier (gimage->gimp, unit);
	factor          = _gimp_unit_get_factor (gimage->gimp, unit);
	digits          = _gimp_unit_get_digits (gimage->gimp, unit);
	unit_strings[1] = _gimp_unit_get_symbol (gimage->gimp, unit);
	unit_strings[2] = _gimp_unit_get_abbreviation (gimage->gimp, unit);
	unit_strings[3] = _gimp_unit_get_singular (gimage->gimp, unit);
	unit_strings[4] = _gimp_unit_get_plural (gimage->gimp, unit);

	size =
	  2 * 4 +
	  strlen (unit_strings[0]) ? strlen (unit_strings[0]) + 5 : 4 +
	  strlen (unit_strings[1]) ? strlen (unit_strings[1]) + 5 : 4 +
	  strlen (unit_strings[2]) ? strlen (unit_strings[2]) + 5 : 4 +
	  strlen (unit_strings[3]) ? strlen (unit_strings[3]) + 5 : 4 +
	  strlen (unit_strings[4]) ? strlen (unit_strings[4]) + 5 : 4;

	xcf_write_int32_check_error (info->fp, (guint32 *) &prop_type, 1);
	xcf_write_int32_check_error (info->fp, &size, 1);
	xcf_write_float_check_error (info->fp, &factor, 1);
	xcf_write_int32_check_error (info->fp, &digits, 1);
	xcf_write_string_check_error (info->fp, (gchar **) unit_strings, 5);
      }
      break;
    }

  va_end (args);

  return TRUE;
}

static gboolean
xcf_save_layer (XcfInfo   *info,
		GimpImage *gimage,
		GimpLayer *layer,
		GError   **error)
{
  guint32 saved_pos;
  guint32 offset;

  GError *tmp_error = NULL;

  /* check and see if this is the drawable that the floating
   *  selection is attached to.
   */
  if (GIMP_DRAWABLE(layer) == info->floating_sel_drawable)
    {
      saved_pos = info->cp;
      xcf_check_error (xcf_seek_pos (info, info->floating_sel_offset, error));
      xcf_write_int32_check_error (info->fp, &saved_pos, 1);
      xcf_check_error (xcf_seek_pos (info, saved_pos, error));
    }

  /* write out the width, height and image type information for the layer */
  xcf_write_int32_check_error (info->fp,
                               (guint32 *) &GIMP_ITEM (layer)->width, 1);
  xcf_write_int32_check_error (info->fp,
                               (guint32 *) &GIMP_ITEM (layer)->height, 1);
  xcf_write_int32_check_error (info->fp,
                               (guint32 *) &GIMP_DRAWABLE (layer)->type, 1);

  /* write out the layers name */
  xcf_write_string_check_error (info->fp, &GIMP_OBJECT (layer)->name, 1);

  /* write out the layer properties */
  xcf_save_layer_props (info, gimage, layer, error);

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
  xcf_write_int32_check_error (info->fp, &offset, 1);
  saved_pos = info->cp;

  /* write out the layer mask */
  if (layer->mask)
    {
      xcf_check_error (xcf_seek_end (info, error));
      offset = info->cp;

      xcf_check_error (xcf_save_channel (info,
                                         gimage, GIMP_CHANNEL(layer->mask),
                                         error));
    }
  else
    offset = 0;

  xcf_check_error (xcf_seek_pos (info, saved_pos, error));
  xcf_write_int32_check_error (info->fp, &offset, 1);

  return TRUE;
}

static gboolean
xcf_save_channel (XcfInfo      *info,
		  GimpImage    *gimage,
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
      xcf_write_int32_check_error (info->fp, (guint32 *) &info->cp, 1);
      xcf_check_error (xcf_seek_pos (info, saved_pos, error));
    }

  /* write out the width and height information for the channel */
  xcf_write_int32_check_error (info->fp,
                               (guint32 *) &GIMP_ITEM (channel)->width, 1);
  xcf_write_int32_check_error (info->fp,
                               (guint32 *) &GIMP_ITEM (channel)->height, 1);

  /* write out the channels name */
  xcf_write_string_check_error (info->fp, &GIMP_OBJECT (channel)->name, 1); 

  /* write out the channel properties */
  xcf_save_channel_props (info, gimage, channel, error);

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
  xcf_write_int32_check_error (info->fp, &offset, 1);
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
xcf_save_hierarchy (XcfInfo     *info,
		    TileManager *tiles,
		    GError     **error)
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

  xcf_write_int32_check_error (info->fp, (guint32 *) &width, 1);
  xcf_write_int32_check_error (info->fp, (guint32 *) &height, 1); 
  xcf_write_int32_check_error (info->fp, (guint32 *) &bpp, 1);

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
	  xcf_write_int32_check_error (info->fp, (guint32 *) &width,  1);
	  xcf_write_int32_check_error (info->fp, (guint32 *) &height, 1);
	  xcf_write_int32_check_error (info->fp, (guint32 *) &tmp1,   1);
	}

      /* seek back to where we are to write out the next
       *  level offset and write it out.
       */
      xcf_check_error (xcf_seek_pos (info, saved_pos, error));
      xcf_write_int32_check_error (info->fp, &offset, 1);
      
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
  xcf_write_int32_check_error (info->fp, &offset, 1);

  return TRUE;
}

static gboolean
xcf_save_level (XcfInfo     *info,
		TileManager *level,
		GError     **error)
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

  xcf_write_int32_check_error (info->fp, (guint32 *) &width, 1);
  xcf_write_int32_check_error (info->fp, (guint32 *) &height, 1);

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
	  xcf_write_int32_check_error (info->fp, &offset, 1);

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
  xcf_write_int32_check_error (info->fp, &offset, 1);

  return TRUE;
  
}

static gboolean
xcf_save_tile (XcfInfo  *info,
	       Tile     *tile,
	       GError  **error)
{
  GError *tmp_error = NULL;

  tile_lock (tile);
  xcf_write_int8_check_error (info->fp, tile_data_pointer (tile, 0, 0), 
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
  guchar *data, *t;
  unsigned int last;
  gint state;
  gint length;
  gint count;
  gint size;
  gint bpp;
  gint i, j;
  gint len = 0;

  GError *tmp_error = NULL;

  tile_lock (tile);

  bpp = tile_bpp (tile);

  for (i = 0; i < bpp; i++)
    {
      data = (guchar*) tile_data_pointer (tile, 0, 0) + i;

      state = 0;
      length = 0;
      count = 0;
      size = tile_ewidth(tile) * tile_eheight(tile);
      last = -1;

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
		state = 1;
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

	  if (size > 0) {
	    length += 1;
	    last = *data;
	    data += bpp;
	  }
	}

      if (count != (tile_ewidth (tile) * tile_eheight (tile)))
	g_message ("xcf: uh oh! xcf rle tile saving error: %d", count);
    }
  xcf_write_int8_check_error (info->fp, rlebuf, len);
  tile_release (tile, FALSE);

  return TRUE;
}
