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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core/core-types.h"
#include "tools/tools-types.h" /* EEK */

#include "base/tile.h"
#include "base/tile-manager.h"
#include "base/tile-manager-private.h"

#include "core/gimpchannel.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimplist.h"
#include "core/gimpparasitelist.h"
#include "core/gimpunit.h"

#include "xcf-private.h"
#include "xcf-read.h"
#include "xcf-seek.h"
#include "xcf-write.h"

#include "floating_sel.h"
#include "path.h"
#include "pathP.h"


static void xcf_save_image_props   (XcfInfo     *info,
				    GimpImage   *gimage);
static void xcf_save_layer_props   (XcfInfo     *info,
				    GimpImage   *gimage,
				    GimpLayer   *layer);
static void xcf_save_channel_props (XcfInfo     *info,
				    GimpImage   *gimage,
				    GimpChannel *channel);
static void xcf_save_prop          (XcfInfo     *info,
				    GimpImage   *gimage,
				    PropType     prop_type,
				    ...);
static void xcf_save_layer         (XcfInfo     *info,
				    GimpImage   *gimage,
				    GimpLayer   *layer);
static void xcf_save_channel       (XcfInfo     *info,
				    GimpImage   *gimage,
				    GimpChannel *channel);
static void xcf_save_hierarchy     (XcfInfo     *info,
				    TileManager *tiles);
static void xcf_save_level         (XcfInfo     *info,
				    TileManager *tiles);
static void xcf_save_tile          (XcfInfo     *info,
				    Tile        *tile);
static void xcf_save_tile_rle      (XcfInfo     *info,
				    Tile        *tile,
				    guchar      *rlebuf);



void
xcf_save_choose_format (XcfInfo   *info,
			GimpImage *gimage)
{
  gint save_version = 0;                /* default to oldest */

  if (gimage->cmap) 
    save_version = 1;                   /* need version 1 for colormaps */

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
  info->cp += xcf_write_int8 (info->fp, (guint8*) version_tag, 14);

  /* write out the width, height and image type information for the image */
  info->cp += xcf_write_int32 (info->fp, (guint32*) &gimage->width, 1);
  info->cp += xcf_write_int32 (info->fp, (guint32*) &gimage->height, 1);
  info->cp += xcf_write_int32 (info->fp, (guint32*) &gimage->base_type, 1);

  /* determine the number of layers and channels in the image */
  nlayers   = (guint) gimp_container_num_children (gimage->layers);
  nchannels = (guint) gimp_container_num_children (gimage->channels);

  /* check and see if we have to save out the selection */
  have_selection = gimage_mask_bounds (gimage, &t1, &t2, &t3, &t4);
  if (have_selection)
    nchannels += 1;

  /* write the property information for the image.
   */
  xcf_save_image_props (info, gimage);

  /* save the current file position as it is the start of where
   *  we place the layer offset information.
   */
  saved_pos = info->cp;

  /* seek to after the offset lists */
  xcf_seek_pos (info, info->cp + (nlayers + nchannels + 2) * 4);

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
      xcf_save_layer (info, gimage, layer);

      /* seek back to where we are to write out the next
       *  layer offset and write it out.
       */
      xcf_seek_pos (info, saved_pos);
      info->cp += xcf_write_int32 (info->fp, &offset, 1);

      /* increment the location we are to write out the
       *  next offset.
       */
      saved_pos = info->cp;

      /* seek to the end of the file which is where
       *  we will write out the next layer.
       */
      xcf_seek_end (info);
    }

  /* write out a '0' offset position to indicate the end
   *  of the layer offsets.
   */
  offset = 0;
  xcf_seek_pos (info, saved_pos);
  info->cp += xcf_write_int32 (info->fp, &offset, 1);
  saved_pos = info->cp;
  xcf_seek_end (info);

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
      xcf_save_channel (info, gimage, channel);

      /* seek back to where we are to write out the next
       *  channel offset and write it out.
       */
      xcf_seek_pos (info, saved_pos);
      info->cp += xcf_write_int32 (info->fp, &offset, 1);

      /* increment the location we are to write out the
       *  next offset.
       */
      saved_pos = info->cp;

      /* seek to the end of the file which is where
       *  we will write out the next channel.
       */
      xcf_seek_end (info);
    }

  /* write out a '0' offset position to indicate the end
   *  of the channel offsets.
   */
  offset = 0;
  xcf_seek_pos (info, saved_pos);
  info->cp += xcf_write_int32 (info->fp, &offset, 1);
  saved_pos = info->cp;

  if (floating_layer)
    floating_sel_rigor (floating_layer, FALSE);

  return !ferror(info->fp);
}

static void
xcf_save_image_props (XcfInfo   *info,
		      GimpImage *gimage)
{
  /* check and see if we should save the colormap property */
  if (gimage->cmap)
    xcf_save_prop (info, gimage, 
                   PROP_COLORMAP, gimage->num_cols, gimage->cmap);

  if (info->compression != COMPRESS_NONE)
    xcf_save_prop (info, gimage, PROP_COMPRESSION, info->compression);

  if (gimage->guides)
    xcf_save_prop (info, gimage, PROP_GUIDES, gimage->guides);

  xcf_save_prop (info, gimage, PROP_RESOLUTION,
		 gimage->xresolution, gimage->yresolution);

  xcf_save_prop (info, gimage, PROP_TATTOO, gimage->tattoo_state);

  if (gimp_parasite_list_length (gimage->parasites) > 0)
    xcf_save_prop (info, gimage, PROP_PARASITES, gimage->parasites);

  if (gimage->unit < _gimp_unit_get_number_of_built_in_units (gimage->gimp))
    xcf_save_prop (info, gimage, PROP_UNIT, gimage->unit);

  xcf_save_prop (info, gimage, PROP_PATHS, gimage->paths);

  if (gimage->unit >= _gimp_unit_get_number_of_built_in_units (gimage->gimp))
    xcf_save_prop (info, gimage, PROP_USER_UNIT, gimage->unit);

  xcf_save_prop (info, gimage, PROP_END);
}

static void
xcf_save_layer_props (XcfInfo   *info,
		      GimpImage *gimage,
		      GimpLayer *layer)
{
  if (layer == gimp_image_get_active_layer (gimage))
    xcf_save_prop (info, gimage, PROP_ACTIVE_LAYER);

  if (layer == gimp_image_floating_sel (gimage))
    {
      info->floating_sel_drawable = layer->fs.drawable;
      xcf_save_prop (info, gimage, PROP_FLOATING_SELECTION);
    }

  xcf_save_prop (info, gimage, PROP_OPACITY, layer->opacity);
  xcf_save_prop (info, gimage, PROP_VISIBLE,
		 gimp_drawable_get_visible (GIMP_DRAWABLE (layer)));
  xcf_save_prop (info, gimage, PROP_LINKED, layer->linked);
  xcf_save_prop (info, gimage, PROP_PRESERVE_TRANSPARENCY,
		 layer->preserve_trans);

  if (layer->mask)
    {
      xcf_save_prop (info, gimage, PROP_APPLY_MASK, layer->mask->apply_mask);
      xcf_save_prop (info, gimage, PROP_EDIT_MASK,  layer->mask->edit_mask);
      xcf_save_prop (info, gimage, PROP_SHOW_MASK,  layer->mask->show_mask);
    }
  else
    {
      xcf_save_prop (info, gimage, PROP_APPLY_MASK, FALSE);
      xcf_save_prop (info, gimage, PROP_EDIT_MASK,  FALSE);
      xcf_save_prop (info, gimage, PROP_SHOW_MASK,  FALSE);
    }

  xcf_save_prop (info, gimage, PROP_OFFSETS,
		 GIMP_DRAWABLE (layer)->offset_x,
		 GIMP_DRAWABLE (layer)->offset_y);
  xcf_save_prop (info, gimage, PROP_MODE, layer->mode);
  xcf_save_prop (info, gimage, PROP_TATTOO, GIMP_DRAWABLE (layer)->tattoo);

  if (gimp_parasite_list_length (GIMP_DRAWABLE (layer)->parasites) > 0)
    xcf_save_prop (info, gimage, PROP_PARASITES,
		   GIMP_DRAWABLE (layer)->parasites);

  xcf_save_prop (info, gimage, PROP_END);
}

static void
xcf_save_channel_props (XcfInfo     *info,
			GimpImage   *gimage,
			GimpChannel *channel)
{
  guchar col[3];

  if (channel == gimp_image_get_active_channel (gimage))
    xcf_save_prop (info, gimage, PROP_ACTIVE_CHANNEL);

  if (channel == gimage->selection_mask)
    xcf_save_prop (info, gimage, PROP_SELECTION);

  xcf_save_prop (info, gimage, PROP_OPACITY, 
		 (gint) (channel->color.a * 255.999));
  xcf_save_prop (info, gimage, PROP_VISIBLE,
		 gimp_drawable_get_visible (GIMP_DRAWABLE (channel)));
  xcf_save_prop (info, gimage, PROP_SHOW_MASKED, channel->show_masked);

  gimp_rgb_get_uchar (&channel->color, &col[0], &col[1], &col[2]);
  xcf_save_prop (info, gimage, PROP_COLOR, col);

  xcf_save_prop (info, gimage, PROP_TATTOO, GIMP_DRAWABLE (channel)->tattoo);

  if (gimp_parasite_list_length (GIMP_DRAWABLE (channel)->parasites) > 0)
    xcf_save_prop (info, gimage, PROP_PARASITES,
		   GIMP_DRAWABLE (channel)->parasites);

  xcf_save_prop (info, gimage, PROP_END);
}

static void 
xcf_save_parasite (gchar        *key, 
                   GimpParasite *parasite, 
                   XcfInfo      *info)
{
  if (gimp_parasite_is_persistent (parasite))
    {
      info->cp += xcf_write_string (info->fp, &parasite->name, 1);
      info->cp += xcf_write_int32 (info->fp, &parasite->flags, 1);
      info->cp += xcf_write_int32 (info->fp, &parasite->size, 1);
      info->cp += xcf_write_int8 (info->fp, parasite->data, parasite->size);
    }
}

static void 
xcf_save_bz_point (PathPoint *bpt, 
                   XcfInfo   *info)
{
  gfloat xfloat = bpt->x;
  gfloat yfloat = bpt->y;

  /* (all gint)
   * type
   * x
   * y
   */

  info->cp += xcf_write_int32 (info->fp, &bpt->type, 1);
  info->cp += xcf_write_float (info->fp, &xfloat, 1);
  info->cp += xcf_write_float (info->fp, &yfloat, 1);
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
 
  info->cp += xcf_write_string (info->fp, &bzp->name, 1);
  info->cp += xcf_write_int32 (info->fp, &bzp->locked, 1);
  info->cp += xcf_write_int8 (info->fp, &state, 1);
  closed = bzp->closed;
  info->cp += xcf_write_int32 (info->fp, &closed, 1);
  num_points = g_slist_length (bzp->path_details);
  info->cp += xcf_write_int32 (info->fp, &num_points, 1);
  version = 3;
  info->cp += xcf_write_int32 (info->fp, &version, 1);
  info->cp += xcf_write_int32 (info->fp, &bzp->pathtype, 1);
  info->cp += xcf_write_int32 (info->fp, &bzp->tattoo, 1); 
  g_slist_foreach (bzp->path_details, (GFunc) xcf_save_bz_point, info);
}

static void
xcf_save_bzpaths (PathList *paths, 
                  XcfInfo  *info)
{
  guint32 num_paths;
  /* Write out the following:-
   *
   * last_selected_row (gint)
   * number_of_paths (gint)
   *
   * then each path:-
   */
  
  info->cp += 
    xcf_write_int32 (info->fp, (guint32*) &paths->last_selected_row, 1);
  num_paths = g_slist_length (paths->bz_paths);
  info->cp += xcf_write_int32 (info->fp, &num_paths, 1);
  g_slist_foreach (paths->bz_paths, (GFunc) xcf_save_path, info);  
}

static void
xcf_save_prop (XcfInfo   *info,
	       GimpImage *gimage,
	       PropType   prop_type,
	       ...)
{
  guint32 size;
  va_list args;

  va_start (args, prop_type);

  switch (prop_type)
    {
    case PROP_END:
      size = 0;

      info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
      info->cp += xcf_write_int32 (info->fp, &size, 1);
      break;
    case PROP_COLORMAP:
      {
	guint32 ncolors;
	guchar *colors;

	ncolors = va_arg (args, guint32);
	colors = va_arg (args, guchar*);
	size = 4 + ncolors;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &ncolors, 1);
	info->cp += xcf_write_int8 (info->fp, colors, ncolors * 3);
      }
      break;
    case PROP_ACTIVE_LAYER:
    case PROP_ACTIVE_CHANNEL:
    case PROP_SELECTION:
      size = 0;

      info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
      info->cp += xcf_write_int32 (info->fp, &size, 1);
      break;
    case PROP_FLOATING_SELECTION:
      {
	guint32 dummy;

	dummy = 0;
	size = 4;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->floating_sel_offset = info->cp;
	info->cp += xcf_write_int32 (info->fp, &dummy, 1);
      }
      break;
    case PROP_OPACITY:
      {
	gint32 opacity;

	opacity = va_arg (args, gint32);

	size = 4;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, (guint32*) &opacity, 1);
      }
      break;
    case PROP_MODE:
      {
	gint32 mode;

	mode = va_arg (args, gint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, (guint32*) &mode, 1);
      }
      break;
    case PROP_VISIBLE:
      {
	guint32 visible;

	visible = va_arg (args, guint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &visible, 1);
      }
      break;
    case PROP_LINKED:
      {
	guint32 linked;

	linked = va_arg (args, guint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &linked, 1);
      }
      break;
    case PROP_PRESERVE_TRANSPARENCY:
      {
	guint32 preserve_trans;

	preserve_trans = va_arg (args, guint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &preserve_trans, 1);
      }
      break;
    case PROP_APPLY_MASK:
      {
	guint32 apply_mask;

	apply_mask = va_arg (args, guint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &apply_mask, 1);
      }
      break;
    case PROP_EDIT_MASK:
      {
	guint32 edit_mask;

	edit_mask = va_arg (args, guint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &edit_mask, 1);
      }
      break;
    case PROP_SHOW_MASK:
      {
	guint32 show_mask;

	show_mask = va_arg (args, guint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &show_mask, 1);
      }
      break;
    case PROP_SHOW_MASKED:
      {
	guint32 show_masked;

	show_masked = va_arg (args, guint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &show_masked, 1);
      }
      break;
    case PROP_OFFSETS:
      {
	gint32 offsets[2];

	offsets[0] = va_arg (args, gint32);
	offsets[1] = va_arg (args, gint32);
	size = 8;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, (guint32*) offsets, 2);
      }
      break;
    case PROP_COLOR:
      {
	guchar *color;

	color = va_arg (args, guchar*);
	size = 3;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int8 (info->fp, color, 3);
      }
      break;
    case PROP_COMPRESSION:
      {
	guint8 compression;

	compression = (guint8) va_arg (args, guint32);
	size = 1;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int8 (info->fp, &compression, 1);
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

	info->cp += xcf_write_int32 (info->fp, (guint32 *) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);

	for (; guides; guides = g_list_next (guides))
	  {
	    guide = (GimpGuide *) guides->data;

	    position    = guide->position;
	    orientation = guide->orientation;

	    info->cp += xcf_write_int32 (info->fp, (guint32 *) &position, 1);
	    info->cp += xcf_write_int8 (info->fp, (guint8 *) &orientation, 1);
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

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);

	info->cp += xcf_write_float (info->fp, &xresolution, 1);
	info->cp += xcf_write_float (info->fp, &yresolution, 1);
      }
      break;
    case PROP_TATTOO:
      {
	guint32 tattoo;

	tattoo =  va_arg (args, guint32);
	size = 4;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &tattoo, 1);
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
	    info->cp += xcf_write_int32 (info->fp, (guint32 *) &prop_type, 1);
	    /* because we don't know how much room the parasite list will take
	     * we save the file position and write the length later
	     */
            pos = info->cp;
	    info->cp += xcf_write_int32 (info->fp, &length, 1);
	    base = info->cp;
	    gimp_parasite_list_foreach (list, 
                                        (GHFunc) xcf_save_parasite, info);
	    length = info->cp - base;
	    /* go back to the saved position and write the length */
            xcf_seek_pos (info, pos);
	    xcf_write_int32 (info->fp, &length, 1);
            xcf_seek_end (info);
	  }
      }
      break;
    case PROP_UNIT:
      {
	guint32 unit;

	unit = va_arg (args, guint32);

	size = 4;

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_int32 (info->fp, &unit, 1);
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
            info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
            /* because we don't know how much room the paths list will take
               we save the file position and write the length later 
             */
            pos = info->cp;
            info->cp += xcf_write_int32 (info->fp, &length, 1);
            base = info->cp;
            xcf_save_bzpaths (paths_list, info);
            length = info->cp - base;
            /* go back to the saved position and write the length */
            xcf_seek_pos (info, pos);
            xcf_write_int32 (info->fp, &length, 1);
            xcf_seek_end (info);
          }
      }
      break;
    case PROP_USER_UNIT:
      {
	GimpUnit  unit;
	gchar    *unit_strings[5];
	gfloat    factor;
	guint32   digits;

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

	info->cp += xcf_write_int32 (info->fp, (guint32*) &prop_type, 1);
	info->cp += xcf_write_int32 (info->fp, &size, 1);
	info->cp += xcf_write_float (info->fp, &factor, 1);
	info->cp += xcf_write_int32 (info->fp, &digits, 1);
	info->cp += xcf_write_string (info->fp, unit_strings, 5);
      }
      break;
    }

  va_end (args);
}

static void
xcf_save_layer (XcfInfo   *info,
		GimpImage *gimage,
		GimpLayer *layer)
{
  guint32 saved_pos;
  guint32 offset;

  /* check and see if this is the drawable that the floating
   *  selection is attached to.
   */
  if (GIMP_DRAWABLE(layer) == info->floating_sel_drawable)
    {
      saved_pos = info->cp;
      xcf_seek_pos (info, info->floating_sel_offset);
      info->cp += xcf_write_int32 (info->fp, &saved_pos, 1);
      xcf_seek_pos (info, saved_pos);
    }

  /* write out the width, height and image type information for the layer */
  info->cp += 
    xcf_write_int32 (info->fp, (guint32*) &GIMP_DRAWABLE(layer)->width, 1);
  info->cp += 
    xcf_write_int32 (info->fp, (guint32*) &GIMP_DRAWABLE(layer)->height, 1);
  info->cp += 
    xcf_write_int32 (info->fp, (guint32*) &GIMP_DRAWABLE(layer)->type, 1);

  /* write out the layers name */
  info->cp += xcf_write_string (info->fp, &GIMP_OBJECT (layer)->name, 1);

  /* write out the layer properties */
  xcf_save_layer_props (info, gimage, layer);

  /* save the current position which is where the hierarchy offset
   *  will be stored.
   */
  saved_pos = info->cp;

  /* write out the layer tile hierarchy */
  xcf_seek_pos (info, info->cp + 8);
  offset = info->cp;

  xcf_save_hierarchy (info, GIMP_DRAWABLE(layer)->tiles);

  xcf_seek_pos (info, saved_pos);
  info->cp += xcf_write_int32 (info->fp, &offset, 1);
  saved_pos = info->cp;

  /* write out the layer mask */
  if (layer->mask)
    {
      xcf_seek_end (info);
      offset = info->cp;

      xcf_save_channel (info, gimage, GIMP_CHANNEL(layer->mask));
    }
  else
    offset = 0;

  xcf_seek_pos (info, saved_pos);
  info->cp += xcf_write_int32 (info->fp, &offset, 1);
}

static void
xcf_save_channel (XcfInfo     *info,
		  GimpImage   *gimage,
		  GimpChannel *channel)
{
  guint32 saved_pos;
  guint32 offset;

  /* check and see if this is the drawable that the floating
   *  selection is attached to.
   */
  if (GIMP_DRAWABLE (channel) == info->floating_sel_drawable)
    {
      saved_pos = info->cp;
      xcf_seek_pos (info, info->floating_sel_offset);
      info->cp += xcf_write_int32 (info->fp, (guint32*) &info->cp, 1);
      xcf_seek_pos (info, saved_pos);
    }

  /* write out the width and height information for the channel */
  info->cp += 
    xcf_write_int32 (info->fp, (guint32*) &GIMP_DRAWABLE(channel)->width, 1);
  info->cp += 
    xcf_write_int32 (info->fp, (guint32*) &GIMP_DRAWABLE(channel)->height, 1);

  /* write out the channels name */
  info->cp += xcf_write_string (info->fp, &GIMP_OBJECT (channel)->name, 1);

  /* write out the channel properties */
  xcf_save_channel_props (info, gimage, channel);

  /* save the current position which is where the hierarchy offset
   *  will be stored.
   */
  saved_pos = info->cp;

  /* write out the channel tile hierarchy */
  xcf_seek_pos (info, info->cp + 4);
  offset = info->cp;

  xcf_save_hierarchy (info, GIMP_DRAWABLE (channel)->tiles);

  xcf_seek_pos (info, saved_pos);
  info->cp += xcf_write_int32 (info->fp, &offset, 1);
  saved_pos = info->cp;
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


static void
xcf_save_hierarchy (XcfInfo     *info,
		    TileManager *tiles)
{
  guint32 saved_pos;
  guint32 offset;
  guint32 width;
  guint32 height;
  guint32 bpp;
  gint    i;
  gint    nlevels;
  gint    tmp1, tmp2;

  width  = tile_manager_width (tiles);
  height = tile_manager_height (tiles);
  bpp    = tile_manager_bpp (tiles);

  info->cp += xcf_write_int32 (info->fp, (guint32*) &width, 1);
  info->cp += xcf_write_int32 (info->fp, (guint32*) &height, 1);
  info->cp += xcf_write_int32 (info->fp, (guint32*) &bpp, 1);

  saved_pos = info->cp;
  
  tmp1 = xcf_calc_levels (width, TILE_WIDTH);
  tmp2 = xcf_calc_levels (height, TILE_HEIGHT);
  nlevels = MAX (tmp1, tmp2);

  xcf_seek_pos (info, info->cp + (1 + nlevels) * 4);

  for (i = 0; i < nlevels; i++) 
    {
      offset = info->cp;

      if (i == 0) 
	{
	  /* write out the level. */
	  xcf_save_level (info, tiles);
	} 
      else 
	{
	  /* fake an empty level */
	  tmp1 = 0;
	  width  /= 2;
	  height /= 2;
	  info->cp += xcf_write_int32 (info->fp, (guint32*) &width, 1);
	  info->cp += xcf_write_int32 (info->fp, (guint32*) &height, 1);
	  info->cp += xcf_write_int32 (info->fp, (guint32*) &tmp1, 1);
	}

      /* seek back to where we are to write out the next
       *  level offset and write it out.
       */
      xcf_seek_pos (info, saved_pos);
      info->cp += xcf_write_int32 (info->fp, &offset, 1);
      
      /* increment the location we are to write out the
       *  next offset.
       */
      saved_pos = info->cp;
  
      /* seek to the end of the file which is where
       *  we will write out the next level.
       */
      xcf_seek_end (info);
    }

  /* write out a '0' offset position to indicate the end
   *  of the level offsets.
   */
  offset = 0;
  xcf_seek_pos (info, saved_pos);
  info->cp += xcf_write_int32 (info->fp, &offset, 1);
}

static void
xcf_save_level (XcfInfo     *info,
		TileManager *level)
{
  guint32  saved_pos;
  guint32  offset;
  guint32  width;
  guint32  height;
  guint    ntiles;
  gint     i;
  guchar  *rlebuf;

  width  = tile_manager_width (level);
  height = tile_manager_height (level);

  info->cp += xcf_write_int32 (info->fp, (guint32*) &width, 1);
  info->cp += xcf_write_int32 (info->fp, (guint32*) &height, 1);

  saved_pos = info->cp;

  /* allocate a temporary buffer to store the rle data before it is
     written to disk */
  rlebuf = 
    g_malloc (TILE_WIDTH * TILE_HEIGHT * tile_manager_bpp (level) * 1.5);

  if (level->tiles)
    {
      ntiles = level->ntile_rows * level->ntile_cols;
      xcf_seek_pos (info, info->cp + (ntiles + 1) * 4);

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
	      xcf_save_tile (info, level->tiles[i]);
	      break;
	    case COMPRESS_RLE:
	      xcf_save_tile_rle (info, level->tiles[i], rlebuf);
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
	  xcf_seek_pos (info, saved_pos);
	  info->cp += xcf_write_int32 (info->fp, &offset, 1);

	  /* increment the location we are to write out the
	   *  next offset.
	   */
	  saved_pos = info->cp;

	  /* seek to the end of the file which is where
	   *  we will write out the next tile.
	   */
	  xcf_seek_end (info);
	}
    }

  g_free (rlebuf);

  /* write out a '0' offset position to indicate the end
   *  of the level offsets.
   */
  offset = 0;
  xcf_seek_pos (info, saved_pos);
  info->cp += xcf_write_int32 (info->fp, &offset, 1);
}

static void
xcf_save_tile (XcfInfo *info,
	       Tile    *tile)
{
  tile_lock (tile);
  info->cp += xcf_write_int8 (info->fp, tile_data_pointer (tile, 0, 0), 
			      tile_size (tile));
  tile_release (tile, FALSE);
}

static void
xcf_save_tile_rle (XcfInfo *info,
		   Tile    *tile,
		   guchar  *rlebuf)
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
  info->cp += xcf_write_int8 (info->fp, rlebuf, len);
  tile_release (tile, FALSE);
}

