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
  GimpDrawable *drawable;
  GimpDisplay  *gdisp;
  GimpLayer    *new_layer;
  gint          off_x, off_y;

  gdisp = GIMP_DISPLAY_SHELL (data)->gdisp;

  if (gdisp->gimage->gimp->busy)
    return;

  drawable = GIMP_DRAWABLE (viewable);

  undo_push_group_start (gdisp->gimage, EDIT_PASTE_UNDO_GROUP);

  new_layer = gimp_layer_new_from_drawable (drawable, gdisp->gimage);

  off_x = (gdisp->gimage->width  - gimp_drawable_width (drawable))  / 2;
  off_y = (gdisp->gimage->height - gimp_drawable_height (drawable)) / 2;

  gimp_layer_translate (new_layer, off_x, off_y);

  gimp_image_add_layer (gdisp->gimage, new_layer, -1);

  undo_push_group_end (gdisp->gimage);

  gdisplays_flush ();

  gimp_context_set_display (gimp_get_user_context (gdisp->gimage->gimp), gdisp);
}

static void
gimp_display_shell_bucket_fill (GimpImage          *gimage,
                                GimpBucketFillMode  fill_mode,
                                const GimpRGB      *color,
                                GimpPattern        *pattern)
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
                                  FALSE, 0.0, FALSE, 0.0, 0.0 /* fill params */);

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
                                      GIMP_PATTERN_BUCKET_FILL,
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
                                  GIMP_FG_BUCKET_FILL,
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
		   buffer,
		   FALSE);

  gdisplays_flush ();
}
