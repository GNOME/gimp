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

#include "widgets-types.h"

#include "gimpcursor.h"

/*  standard gimp cursors  */
#include "cursors/mouse.xbm"
#include "cursors/mouse_mask.xbm"
#include "cursors/crosshair.xbm"
#include "cursors/crosshair_mask.xbm"
#include "cursors/crosshair_small.xbm"
#include "cursors/crosshair_small_mask.xbm"
#include "cursors/bad.xbm"
#include "cursors/bad_mask.xbm"
#include "cursors/zoom.xbm"
#include "cursors/zoom_mask.xbm"
#include "cursors/dropper.xbm"
#include "cursors/dropper_mask.xbm"

/*  stock tool cursors  */
#include "cursors/rect_select_small.xbm"
#include "cursors/rect_select_small_mask.xbm"
#include "cursors/ellipse_select_small.xbm"
#include "cursors/ellipse_select_small_mask.xbm"
#include "cursors/free_select_small.xbm"
#include "cursors/free_select_small_mask.xbm"
#include "cursors/fuzzy_select_small.xbm"
#include "cursors/fuzzy_select_small_mask.xbm"
#include "cursors/bezier_select_small.xbm"
#include "cursors/bezier_select_small_mask.xbm"
#include "cursors/scissors_small.xbm"
#include "cursors/scissors_small_mask.xbm"
#include "cursors/move_small.xbm"
#include "cursors/move_small_mask.xbm"
#include "cursors/zoom_small.xbm"
#include "cursors/zoom_small_mask.xbm"
#include "cursors/crop_small.xbm"
#include "cursors/crop_small_mask.xbm"
#include "cursors/resize_small.xbm"
#include "cursors/resize_small_mask.xbm"
#include "cursors/rotate_small.xbm"
#include "cursors/rotate_small_mask.xbm"
#include "cursors/shear_small.xbm"
#include "cursors/shear_small_mask.xbm"
#include "cursors/perspective_small.xbm"
#include "cursors/perspective_small_mask.xbm"
#include "cursors/flip_horizontal_small.xbm"
#include "cursors/flip_horizontal_small_mask.xbm"
#include "cursors/flip_vertical_small.xbm"
#include "cursors/flip_vertical_small_mask.xbm"
#include "cursors/text_small.xbm"
#include "cursors/text_small_mask.xbm"
#include "cursors/dropper_small.xbm"
#include "cursors/dropper_small_mask.xbm"
#include "cursors/bucket_fill_small.xbm"
#include "cursors/bucket_fill_small_mask.xbm"
#include "cursors/blend_small.xbm"
#include "cursors/blend_small_mask.xbm"
#include "cursors/pencil_small.xbm"
#include "cursors/pencil_small_mask.xbm"
#include "cursors/paintbrush_small.xbm"
#include "cursors/paintbrush_small_mask.xbm"
#include "cursors/eraser_small.xbm"
#include "cursors/eraser_small_mask.xbm"
#include "cursors/airbrush_small.xbm"
#include "cursors/airbrush_small_mask.xbm"
#include "cursors/clone_small.xbm"
#include "cursors/clone_small_mask.xbm"
#include "cursors/blur_small.xbm"
#include "cursors/blur_small_mask.xbm"
#include "cursors/ink_small.xbm"
#include "cursors/ink_small_mask.xbm"
#include "cursors/dodge_small.xbm"
#include "cursors/dodge_small_mask.xbm"
#include "cursors/burn_small.xbm"
#include "cursors/burn_small_mask.xbm"
#include "cursors/smudge_small.xbm"
#include "cursors/smudge_small_mask.xbm"
#include "cursors/measure_small.xbm"
#include "cursors/measure_small_mask.xbm"

/*  modifiers  */
#include "cursors/plus.xbm"
#include "cursors/plus_mask.xbm"
#include "cursors/minus.xbm"
#include "cursors/minus_mask.xbm"
#include "cursors/intersect.xbm"
#include "cursors/intersect_mask.xbm"
#include "cursors/move.xbm"
#include "cursors/move_mask.xbm"
#include "cursors/resize.xbm"
#include "cursors/resize_mask.xbm"
#include "cursors/control.xbm"
#include "cursors/control_mask.xbm"
#include "cursors/anchor.xbm"
#include "cursors/anchor_mask.xbm"
#include "cursors/foreground.xbm"
#include "cursors/foreground_mask.xbm"
#include "cursors/background.xbm"
#include "cursors/background_mask.xbm"
#include "cursors/pattern.xbm"
#include "cursors/pattern_mask.xbm"
#include "cursors/hand.xbm"
#include "cursors/hand_mask.xbm"


typedef struct _GimpBitmapCursor GimpBitmapCursor;

struct _GimpBitmapCursor
{
  guchar    *bits;
  guchar    *mask_bits;
  gint       width, height;
  gint       x_hot, y_hot;
  GdkBitmap *bitmap;
  GdkBitmap *mask;
  GdkCursor *cursor;
};


static GimpBitmapCursor gimp_cursors[] =
/* these have to match up with the enum in cursorutil.h */
{
  {
    mouse_bits, mouse_mask_bits,
    mouse_width, mouse_height,
    mouse_x_hot, mouse_y_hot, NULL, NULL, NULL
  },
  {
    crosshair_bits, crosshair_mask_bits,
    crosshair_width, crosshair_height,
    crosshair_x_hot, crosshair_y_hot, NULL, NULL, NULL
  },
  {
    crosshair_small_bits, crosshair_small_mask_bits,
    crosshair_small_width, crosshair_small_height,
    crosshair_small_x_hot, crosshair_small_y_hot, NULL, NULL, NULL
  },
  {
    bad_bits, bad_mask_bits,
    bad_width, bad_height,
    bad_x_hot, bad_y_hot, NULL, NULL, NULL
  },
  {
    zoom_bits, zoom_mask_bits,
    zoom_width, zoom_height,
    zoom_x_hot, zoom_y_hot, NULL, NULL, NULL
  },
  {
    dropper_bits, dropper_mask_bits,
    dropper_width, dropper_height,
    dropper_x_hot, dropper_y_hot, NULL, NULL, NULL
  }
};

static GimpBitmapCursor gimp_stock_tool_cursors[] =
/* these have to match up with the enum in widgets-types.h */
{
  {
    NULL, NULL,
    0, 0,
    0, 0, NULL, NULL, NULL
  },
  {
    rect_select_small_bits, rect_select_small_mask_bits,
    rect_select_small_width, rect_select_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    ellipse_select_small_bits, ellipse_select_small_mask_bits,
    ellipse_select_small_width, ellipse_select_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    free_select_small_bits, free_select_small_mask_bits,
    free_select_small_width, free_select_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    fuzzy_select_small_bits, fuzzy_select_small_mask_bits,
    fuzzy_select_small_width, fuzzy_select_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    bezier_select_small_bits, bezier_select_small_mask_bits,
    bezier_select_small_width, bezier_select_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    scissors_small_bits, scissors_small_mask_bits,
    scissors_small_width, scissors_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    move_small_bits, move_small_mask_bits,
    move_small_width, move_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    zoom_small_bits, zoom_small_mask_bits,
    zoom_small_width, zoom_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    crop_small_bits, crop_small_mask_bits,
    crop_small_width, crop_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    resize_small_bits, resize_small_mask_bits,
    resize_small_width, resize_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    rotate_small_bits, rotate_small_mask_bits,
    rotate_small_width, rotate_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    shear_small_bits, shear_small_mask_bits,
    shear_small_width, shear_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    perspective_small_bits, perspective_small_mask_bits,
    perspective_small_width, perspective_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    flip_horizontal_small_bits, flip_horizontal_small_mask_bits,
    flip_horizontal_small_width, flip_horizontal_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    flip_vertical_small_bits, flip_vertical_small_mask_bits,
    flip_vertical_small_width, flip_vertical_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    text_small_bits, text_small_mask_bits,
    text_small_width, text_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    dropper_small_bits, dropper_small_mask_bits,
    dropper_small_width, dropper_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    bucket_fill_small_bits, bucket_fill_small_mask_bits,
    bucket_fill_small_width, bucket_fill_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    blend_small_bits, blend_small_mask_bits,
    blend_small_width, blend_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    pencil_small_bits, pencil_small_mask_bits,
    pencil_small_width, pencil_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    paintbrush_small_bits, paintbrush_small_mask_bits,
    paintbrush_small_width, paintbrush_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    airbrush_small_bits, airbrush_small_mask_bits,
    airbrush_small_width, airbrush_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    ink_small_bits, ink_small_mask_bits,
    ink_small_width, ink_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    clone_small_bits, clone_small_mask_bits,
    clone_small_width, clone_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    eraser_small_bits, eraser_small_mask_bits,
    eraser_small_width, eraser_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    smudge_small_bits, smudge_small_mask_bits,
    smudge_small_width, smudge_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    blur_small_bits, blur_small_mask_bits,
    blur_small_width, blur_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    dodge_small_bits, dodge_small_mask_bits,
    dodge_small_width, dodge_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    burn_small_bits, burn_small_mask_bits,
    burn_small_width, burn_small_height,
    0, 0, NULL, NULL, NULL
  },
  {
    measure_small_bits, measure_small_mask_bits,
    measure_small_width, measure_small_height,
    0, 0, NULL, NULL, NULL
  }
};

static GimpBitmapCursor gimp_modifier_cursors[] =
/* these have to match up with the enum in widgets-types.h */
{
  {
    NULL, NULL,
    0, 0,
    0, 0, NULL, NULL, NULL
  },
  {
    plus_bits, plus_mask_bits,
    plus_width, plus_height,
    plus_x_hot, plus_y_hot, NULL, NULL, NULL
  },
  {
    minus_bits, minus_mask_bits,
    minus_width, minus_height,
    minus_x_hot, minus_y_hot, NULL, NULL, NULL
  },
  {
    intersect_bits, intersect_mask_bits,
    intersect_width, intersect_height,
    intersect_x_hot, intersect_y_hot, NULL, NULL, NULL
  },
  {
    move_bits, move_mask_bits,
    move_width, move_height,
    move_x_hot, move_y_hot, NULL, NULL, NULL
  },
  {
    resize_bits, resize_mask_bits,
    resize_width, resize_height,
    resize_x_hot, resize_y_hot, NULL, NULL, NULL
  },
  {
    control_bits, control_mask_bits,
    control_width, control_height,
    control_x_hot, control_y_hot, NULL, NULL, NULL
  },
  {
    anchor_bits, anchor_mask_bits,
    anchor_width, anchor_height,
    anchor_x_hot, anchor_y_hot, NULL, NULL, NULL
  },
  {
    foreground_bits, foreground_mask_bits,
    foreground_width, foreground_height,
    foreground_x_hot, foreground_y_hot, NULL, NULL, NULL
  },
  {
    background_bits, background_mask_bits,
    background_width, background_height,
    background_x_hot, background_y_hot, NULL, NULL, NULL
  },
  {
    pattern_bits, pattern_mask_bits,
    pattern_width, pattern_height,
    pattern_x_hot, pattern_y_hot, NULL, NULL, NULL
  },
  {
    hand_bits, hand_mask_bits,
    hand_width, hand_height,
    hand_x_hot, hand_y_hot, NULL, NULL, NULL
  }
};


static void
create_cursor_bitmaps (GimpBitmapCursor *bmcursor)
{
  if (bmcursor->bitmap == NULL)
    bmcursor->bitmap = gdk_bitmap_create_from_data (NULL, bmcursor->bits,
						    bmcursor->width,
						    bmcursor->height);
  g_return_if_fail (bmcursor->bitmap != NULL);

  if (bmcursor->mask == NULL)
    bmcursor->mask = gdk_bitmap_create_from_data (NULL, bmcursor->mask_bits,
						  bmcursor->width,
						  bmcursor->height);
  g_return_if_fail (bmcursor->mask != NULL);
}

/*
static void
create_cursor (GimpBitmapCursor *bmcursor)
{
  if (bmcursor->bitmap == NULL ||
      bmcursor->mask == NULL)
    create_cursor_bitmaps (bmcursor);

  if (bmcursor->cursor == NULL)
    {
      GdkColor fg, bg;

      * should have a way to configure the mouse colors *
      gdk_color_parse ("#FFFFFF", &bg);
      gdk_color_parse ("#000000", &fg);

      bmcursor->cursor = gdk_cursor_new_from_pixmap (bmcursor->bitmap,
						     bmcursor->mask,
						     &fg, &bg,
						     bmcursor->x_hot,
						     bmcursor->y_hot);
    }

  g_return_if_fail (bmcursor->cursor != NULL);
}
*/

GdkCursor *
gimp_cursor_new (GimpCursorType      cursor_type,
		 GimpToolCursorType  tool_cursor,
		 GimpCursorModifier  modifier)
{
  GdkCursor *cursor;

  GdkBitmap *bitmap;
  GdkBitmap *mask;

  GdkColor   color;
  GdkColor   fg, bg;

  static GdkGC *gc = NULL;

  gint width;
  gint height;

  GimpBitmapCursor *bmcursor   = NULL;
  GimpBitmapCursor *bmmodifier = NULL;
  GimpBitmapCursor *bmtool     = NULL;

  g_return_val_if_fail (cursor_type < GIMP_LAST_CURSOR_ENTRY, NULL);

  if (cursor_type <= GDK_LAST_CURSOR)
    {
      return gdk_cursor_new (cursor_type);
    }

  g_return_val_if_fail (cursor_type >= GIMP_MOUSE_CURSOR, NULL);

  /*  allow the small tool cursor only with the standard mouse,
   *  the small crosshair and the bad cursor
   */
  if (cursor_type != GIMP_MOUSE_CURSOR &&
      cursor_type != GIMP_CROSSHAIR_SMALL_CURSOR &&
      cursor_type != GIMP_BAD_CURSOR)
    {
      tool_cursor = GIMP_TOOL_CURSOR_NONE;
    }

  cursor_type -= GIMP_MOUSE_CURSOR;
  bmcursor = &gimp_cursors[(gint) cursor_type];

  /*  if there are no modifiers, we can show the cursor immediately
   */

  /* FIXME: ref the cursor in gtk-2.0 before returning it
  if (modifier    == GIMP_CURSOR_MODIFIER_NONE &&
      tool_cursor == GIMP_TOOL_CURSOR_NONE)
    {
      if  (bmcursor->cursor == NULL)
	create_cursor (bmcursor);

      gdk_window_set_cursor (window, bmcursor->cursor);

      return;
    }
    else */

  if (bmcursor->bitmap == NULL ||
      bmcursor->mask == NULL)
    {
      create_cursor_bitmaps (bmcursor);
    }

  /*  prepare the tool cursor  */

  if (tool_cursor < GIMP_TOOL_CURSOR_NONE ||
      tool_cursor >= GIMP_LAST_STOCK_TOOL_CURSOR_ENTRY)
    {
      tool_cursor = GIMP_TOOL_CURSOR_NONE; 
    }

  if (tool_cursor != GIMP_TOOL_CURSOR_NONE)
    {
      bmtool = &gimp_stock_tool_cursors[(gint) tool_cursor];

      if (bmtool->bitmap == NULL ||
	  bmtool->mask == NULL)
	{
	  create_cursor_bitmaps (bmtool);
	}
    }

  /*  prepare the cursor modifier  */

  if (modifier < GIMP_CURSOR_MODIFIER_NONE ||
      modifier >= GIMP_LAST_CURSOR_MODIFIER_ENTRY)
    {
      modifier = GIMP_CURSOR_MODIFIER_NONE;
    }

  if (modifier != GIMP_CURSOR_MODIFIER_NONE)
    {
      bmmodifier = &gimp_modifier_cursors[(gint) modifier];

      if (bmmodifier->bitmap == NULL ||
	  bmmodifier->mask == NULL)
	{
	  create_cursor_bitmaps (bmmodifier);
	}
    }

  if (gc == NULL)
    gc = gdk_gc_new (bmcursor->bitmap);

  gdk_drawable_get_size (bmcursor->bitmap, &width, &height);

  /*  new bitmap and mask for on-the-fly cursor creation  */

  bitmap = gdk_pixmap_new (NULL, width, height, 1);
  mask   = gdk_pixmap_new (NULL, width, height, 1);

  color.pixel = 1;
  gdk_gc_set_foreground (gc, &color);

  /*  first draw the bitmap completely ... */

  gdk_draw_drawable (bitmap, gc, bmcursor->bitmap,
                     0, 0, 0, 0, width, height);

  if (bmmodifier)
    {
      gdk_gc_set_clip_mask (gc, bmmodifier->bitmap);
      gdk_draw_drawable (bitmap, gc, bmmodifier->bitmap,
                         0, 0, 0, 0, width, height);
      gdk_gc_set_clip_mask (gc, NULL);
    }

  if (bmtool)
    {
      gdk_gc_set_clip_mask (gc, bmtool->bitmap);
      gdk_draw_drawable (bitmap, gc, bmtool->bitmap,
                         0, 0, 0, 0, width, height);
      gdk_gc_set_clip_mask (gc, NULL);
    }

  /*  ... then the mask  */

  gdk_draw_drawable (mask, gc, bmcursor->mask,
                     0, 0, 0, 0, width, height);

  if (bmmodifier)
    {
      gdk_gc_set_clip_mask (gc, bmmodifier->mask);
      gdk_draw_drawable (mask, gc, bmmodifier->mask,
                         0, 0, 0, 0, width, height);
      gdk_gc_set_clip_mask (gc, NULL);
    }

  if (bmtool)
    {
      gdk_gc_set_clip_mask (gc, bmtool->mask);
      gdk_draw_drawable (mask, gc, bmtool->mask,
                         0, 0, 0, 0, width, height);
      gdk_gc_set_clip_mask (gc, NULL);
    }

  /* should have a way to configure the mouse colors */
  gdk_color_parse ("#FFFFFF", &bg);
  gdk_color_parse ("#000000", &fg);

  cursor = gdk_cursor_new_from_pixmap (bitmap, mask,
				       &fg, &bg,
				       bmcursor->x_hot,
				       bmcursor->y_hot);

  g_object_unref (bitmap);
  g_object_unref (mask);

  return cursor;
}
