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

#include <gtk/gtk.h>

#include "display-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "core/gimp.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-bucket-fill.h"
#include "core/gimpedit.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "paint-funcs/paint-funcs.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-dnd.h"
#include "gimpdisplay-foreach.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


void
gimp_display_shell_drop_drawable (GtkWidget    *widget,
                                  GimpViewable *viewable,
                                  gpointer      data)
{
  GimpDrawable     *drawable;
  GimpDisplay      *gdisp;
  GimpImage        *src_gimage;
  GimpLayer        *new_layer;
  GimpImage        *dest_gimage;
  gint              src_width, src_height;
  gint              dest_width, dest_height;
  gint              off_x, off_y;
  TileManager      *tiles;
  PixelRegion       srcPR, destPR;
  guchar            bg[MAX_CHANNELS];
  gint              bytes; 
  GimpImageBaseType type;

  gdisp = GIMP_DISPLAY_SHELL (data)->gdisp;

  if (gdisp->gimage->gimp->busy)
    return;

  drawable = GIMP_DRAWABLE (viewable);

  src_gimage = gimp_drawable_gimage (drawable);
  src_width  = gimp_drawable_width  (drawable);
  src_height = gimp_drawable_height (drawable);

  switch (gimp_drawable_type (drawable))
    {
    case RGB_GIMAGE: case RGBA_GIMAGE:
      bytes = 4; type = RGB;
      break;
    case GRAY_GIMAGE: case GRAYA_GIMAGE:
      bytes = 2; type = GRAY;
      break;
    case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
      bytes = 4; type = INDEXED;
      break;
    default:
      bytes = 3; type = RGB;
      break;
    }

  gimp_image_get_background (src_gimage, drawable, bg);

  tiles = tile_manager_new (src_width, src_height, bytes);

  pixel_region_init (&srcPR, gimp_drawable_data (drawable),
		     0, 0, src_width, src_height, FALSE);
  pixel_region_init (&destPR, tiles,
		     0, 0, src_width, src_height, TRUE);

  if (type == INDEXED)
    {
      /*  If the layer is indexed...we need to extract pixels  */
      extract_from_region (&srcPR, &destPR, NULL,
			   gimp_drawable_cmap (drawable), bg, type,
			   gimp_drawable_has_alpha (drawable), FALSE);
    }
  else if (bytes > srcPR.bytes)
    {
      /*  If the layer doesn't have an alpha channel, add one  */
      add_alpha_region (&srcPR, &destPR);
    }
  else
    {
      /*  Otherwise, do a straight copy  */
      copy_region (&srcPR, &destPR);
    }

  dest_gimage = gdisp->gimage;
  dest_width  = dest_gimage->width;
  dest_height = dest_gimage->height;

  undo_push_group_start (dest_gimage, EDIT_PASTE_UNDO);

  new_layer =
    gimp_layer_new_from_tiles (dest_gimage,
			       gimp_image_base_type_with_alpha (dest_gimage),
			       tiles, 
			       _("Pasted Layer"),
			       OPAQUE_OPACITY, NORMAL_MODE);

  tile_manager_destroy (tiles);

  if (new_layer)
    {
      gimp_drawable_set_gimage (GIMP_DRAWABLE (new_layer), dest_gimage);

      off_x = (dest_gimage->width - src_width) / 2;
      off_y = (dest_gimage->height - src_height) / 2;

      gimp_layer_translate (new_layer, off_x, off_y);

      gimp_image_add_layer (dest_gimage, new_layer, -1);

      undo_push_group_end (dest_gimage);

      gdisplays_flush ();

      gimp_context_set_display (gimp_get_user_context (gdisp->gimage->gimp),
				gdisp);
    }
}

static void
gimp_display_shell_bucket_fill (GimpImage      *gimage,
                                BucketFillMode  fill_mode,
                                const GimpRGB  *color,
                                GimpPattern    *pattern)
{
  GimpDrawable *drawable;
  GimpToolInfo *tool_info;
  GimpContext  *context;

  if (gimage->gimp->busy)
    return;

  drawable = gimp_image_active_drawable (gimage);

  if (! drawable)
    return;

  /*  Get the bucket fill context  */
  tool_info = (GimpToolInfo *)
    gimp_container_get_child_by_name (gimage->gimp->tool_info_list,
                                      "gimp:bucket_fill_tool");

  if (tool_info && tool_info->context)
    {
      context = tool_info->context;
    }
  else
    {
      context = gimp_get_user_context (gimage->gimp);
    }

  gimp_drawable_bucket_fill_full (drawable,
                                  fill_mode,
                                  color, pattern,
                                  gimp_context_get_paint_mode (context),
                                  gimp_context_get_opacity (context),
                                  FALSE /* no seed fill */,
                                  0.0, FALSE, 0.0, 0.0 /* seed fill params */);

  gdisplays_flush ();
}

void
gimp_display_shell_drop_pattern (GtkWidget    *widget,
                                 GimpViewable *viewable,
                                 gpointer      data)
{
  GimpDisplay *gdisp;

  gdisp = GIMP_DISPLAY_SHELL (data)->gdisp;

  if (GIMP_IS_PATTERN (viewable))
    {
      gimp_display_shell_bucket_fill (gdisp->gimage,
                                      PATTERN_BUCKET_FILL,
                                      NULL,
                                      GIMP_PATTERN (viewable));
    }
}

void
gimp_display_shell_drop_color (GtkWidget     *widget,
                               const GimpRGB *color,
                               gpointer       data)
{
  GimpDisplay *gdisp;

  gdisp = GIMP_DISPLAY_SHELL (data)->gdisp;

  gimp_display_shell_bucket_fill (gdisp->gimage,
                                  FG_BUCKET_FILL,
                                  color,
                                  NULL);
}

void
gimp_display_shell_drop_buffer (GtkWidget    *widget,
                                GimpViewable *viewable,
                                gpointer      data)
{
  GimpBuffer  *buffer;
  GimpDisplay *gdisp;

  gdisp = GIMP_DISPLAY_SHELL (data)->gdisp;

  if (gdisp->gimage->gimp->busy)
    return;

  buffer = GIMP_BUFFER (viewable);

  /* FIXME: popup a menu for selecting "Paste Into" */

  gimp_edit_paste (gdisp->gimage,
		   gimp_image_active_drawable (gdisp->gimage),
		   buffer->tiles,
		   FALSE);

  gdisplays_flush ();
}
