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

#include "cursors/gimp-tool-cursors.h"

/*  standard gimp cursors  */
#include "cursors/xbm/cursor-mouse.xbm"
#include "cursors/xbm/cursor-mouse-mask.xbm"
#include "cursors/xbm/cursor-crosshair.xbm"
#include "cursors/xbm/cursor-crosshair-mask.xbm"
#include "cursors/xbm/cursor-crosshair-small.xbm"
#include "cursors/xbm/cursor-crosshair-small-mask.xbm"
#include "cursors/xbm/cursor-bad.xbm"
#include "cursors/xbm/cursor-bad-mask.xbm"
#include "cursors/xbm/cursor-zoom.xbm"
#include "cursors/xbm/cursor-zoom-mask.xbm"
#include "cursors/xbm/cursor-color-picker.xbm"
#include "cursors/xbm/cursor-color-picker-mask.xbm"

/*  stock tool cursors  */
#include "cursors/xbm/tool-rect-select.xbm"
#include "cursors/xbm/tool-rect-select-mask.xbm"
#include "cursors/xbm/tool-ellipse-select.xbm"
#include "cursors/xbm/tool-ellipse-select-mask.xbm"
#include "cursors/xbm/tool-free-select.xbm"
#include "cursors/xbm/tool-free-select-mask.xbm"
#include "cursors/xbm/tool-fuzzy-select.xbm"
#include "cursors/xbm/tool-fuzzy-select-mask.xbm"
#include "cursors/xbm/tool-paths.xbm"
#include "cursors/xbm/tool-paths-mask.xbm"
#include "cursors/xbm/tool-iscissors.xbm"
#include "cursors/xbm/tool-iscissors-mask.xbm"
#include "cursors/xbm/tool-move.xbm"
#include "cursors/xbm/tool-move-mask.xbm"
#include "cursors/xbm/tool-zoom.xbm"
#include "cursors/xbm/tool-zoom-mask.xbm"
#include "cursors/xbm/tool-crop.xbm"
#include "cursors/xbm/tool-crop-mask.xbm"
#include "cursors/xbm/tool-resize.xbm"
#include "cursors/xbm/tool-resize-mask.xbm"
#include "cursors/xbm/tool-rotate.xbm"
#include "cursors/xbm/tool-rotate-mask.xbm"
#include "cursors/xbm/tool-shear.xbm"
#include "cursors/xbm/tool-shear-mask.xbm"
#include "cursors/xbm/tool-perspective.xbm"
#include "cursors/xbm/tool-perspective-mask.xbm"
#include "cursors/xbm/tool-flip-horizontal.xbm"
#include "cursors/xbm/tool-flip-horizontal-mask.xbm"
#include "cursors/xbm/tool-flip-vertical.xbm"
#include "cursors/xbm/tool-flip-vertical-mask.xbm"
#include "cursors/xbm/tool-text.xbm"
#include "cursors/xbm/tool-text-mask.xbm"
#include "cursors/xbm/tool-color-picker.xbm"
#include "cursors/xbm/tool-color-picker-mask.xbm"
#include "cursors/xbm/tool-bucket-fill.xbm"
#include "cursors/xbm/tool-bucket-fill-mask.xbm"
#include "cursors/xbm/tool-blend.xbm"
#include "cursors/xbm/tool-blend-mask.xbm"
#include "cursors/xbm/tool-pencil.xbm"
#include "cursors/xbm/tool-pencil-mask.xbm"
#include "cursors/xbm/tool-paintbrush.xbm"
#include "cursors/xbm/tool-paintbrush-mask.xbm"
#include "cursors/xbm/tool-eraser.xbm"
#include "cursors/xbm/tool-eraser-mask.xbm"
#include "cursors/xbm/tool-airbrush.xbm"
#include "cursors/xbm/tool-airbrush-mask.xbm"
#include "cursors/xbm/tool-clone.xbm"
#include "cursors/xbm/tool-clone-mask.xbm"
#include "cursors/xbm/tool-blur.xbm"
#include "cursors/xbm/tool-blur-mask.xbm"
#include "cursors/xbm/tool-ink.xbm"
#include "cursors/xbm/tool-ink-mask.xbm"
#include "cursors/xbm/tool-dodge.xbm"
#include "cursors/xbm/tool-dodge-mask.xbm"
#include "cursors/xbm/tool-burn.xbm"
#include "cursors/xbm/tool-burn-mask.xbm"
#include "cursors/xbm/tool-smudge.xbm"
#include "cursors/xbm/tool-smudge-mask.xbm"
#include "cursors/xbm/tool-measure.xbm"
#include "cursors/xbm/tool-measure-mask.xbm"
#include "cursors/xbm/tool-hand.xbm"
#include "cursors/xbm/tool-hand-mask.xbm"

/*  modifiers  */
#include "cursors/xbm/modifier-plus.xbm"
#include "cursors/xbm/modifier-plus-mask.xbm"
#include "cursors/xbm/modifier-minus.xbm"
#include "cursors/xbm/modifier-minus-mask.xbm"
#include "cursors/xbm/modifier-intersect.xbm"
#include "cursors/xbm/modifier-intersect-mask.xbm"
#include "cursors/xbm/modifier-move.xbm"
#include "cursors/xbm/modifier-move-mask.xbm"
#include "cursors/xbm/modifier-resize.xbm"
#include "cursors/xbm/modifier-resize-mask.xbm"
#include "cursors/xbm/modifier-control.xbm"
#include "cursors/xbm/modifier-control-mask.xbm"
#include "cursors/xbm/modifier-anchor.xbm"
#include "cursors/xbm/modifier-anchor-mask.xbm"
#include "cursors/xbm/modifier-foreground.xbm"
#include "cursors/xbm/modifier-foreground-mask.xbm"
#include "cursors/xbm/modifier-background.xbm"
#include "cursors/xbm/modifier-background-mask.xbm"
#include "cursors/xbm/modifier-pattern.xbm"
#include "cursors/xbm/modifier-pattern-mask.xbm"


typedef struct _GimpBitmapCursor GimpBitmapCursor;

struct _GimpBitmapCursor
{
  guchar    *bits;
  guchar    *mask_bits;
  gint       width, height;
  gint       x_hot, y_hot;

  GdkBitmap *bitmap;
  GdkBitmap *mask;

  const guint8 *pixbuf_data;
  GdkPixbuf    *pixbuf;
};


static GimpBitmapCursor gimp_cursors[] =
{
  /* these have to match up with enum GimpCursorType in widgets-enums.h */

  {
    cursor_mouse_bits, cursor_mouse_mask_bits,
    cursor_mouse_width, cursor_mouse_height,
    cursor_mouse_x_hot, cursor_mouse_y_hot, NULL, NULL,

    cursor_mouse, NULL
  },
  {
    cursor_crosshair_bits, cursor_crosshair_mask_bits,
    cursor_crosshair_width, cursor_crosshair_height,
    cursor_crosshair_x_hot, cursor_crosshair_y_hot, NULL, NULL,

    cursor_crosshair, NULL
  },
  {
    cursor_crosshair_small_bits, cursor_crosshair_small_mask_bits,
    cursor_crosshair_small_width, cursor_crosshair_small_height,
    cursor_crosshair_small_x_hot, cursor_crosshair_small_y_hot, NULL, NULL,

    cursor_crosshair_small, NULL
  },
  {
    cursor_bad_bits, cursor_bad_mask_bits,
    cursor_bad_width, cursor_bad_height,
    cursor_bad_x_hot, cursor_bad_y_hot, NULL, NULL,

    cursor_bad, NULL
  },
  {
    cursor_zoom_bits, cursor_zoom_mask_bits,
    cursor_zoom_width, cursor_zoom_height,
    cursor_zoom_x_hot, cursor_zoom_y_hot, NULL, NULL,

    cursor_zoom, NULL
  },
  {
    cursor_color_picker_bits, cursor_color_picker_mask_bits,
    cursor_color_picker_width, cursor_color_picker_height,
    cursor_color_picker_x_hot, cursor_color_picker_y_hot, NULL, NULL,

    cursor_color_picker, NULL
  }
};

static GimpBitmapCursor gimp_stock_tool_cursors[] =
{
  /* these have to match up with enum GimpToolCursorType in widgets-enums.h */

  {
    NULL, NULL,
    0, 0,
    0, 0, NULL, NULL,

    NULL, NULL
  },
  {
    tool_rect_select_bits, tool_rect_select_mask_bits,
    tool_rect_select_width, tool_rect_select_height,
    0, 0, NULL, NULL,

    tool_rect_select, NULL
  },
  {
    tool_ellipse_select_bits, tool_ellipse_select_mask_bits,
    tool_ellipse_select_width, tool_ellipse_select_height,
    0, 0, NULL, NULL,

    tool_ellipse_select, NULL
  },
  {
    tool_free_select_bits, tool_free_select_mask_bits,
    tool_free_select_width, tool_free_select_height,
    0, 0, NULL, NULL,

    tool_free_select, NULL
  },
  {
    tool_fuzzy_select_bits, tool_fuzzy_select_mask_bits,
    tool_fuzzy_select_width, tool_fuzzy_select_height,
    0, 0, NULL, NULL,

    tool_fuzzy_select, NULL
  },
  {
    tool_paths_bits, tool_paths_mask_bits,
    tool_paths_width, tool_paths_height,
    0, 0, NULL, NULL,

    tool_paths, NULL
  },
  {
    tool_iscissors_bits, tool_iscissors_mask_bits,
    tool_iscissors_width, tool_iscissors_height,
    0, 0, NULL, NULL,

    tool_iscissors, NULL
  },
  {
    tool_move_bits, tool_move_mask_bits,
    tool_move_width, tool_move_height,
    0, 0, NULL, NULL,

    tool_move, NULL
  },
  {
    tool_zoom_bits, tool_zoom_mask_bits,
    tool_zoom_width, tool_zoom_height,
    0, 0, NULL, NULL,

    tool_zoom, NULL
  },
  {
    tool_crop_bits, tool_crop_mask_bits,
    tool_crop_width, tool_crop_height,
    0, 0, NULL, NULL,

    tool_crop, NULL
  },
  {
    tool_resize_bits, tool_resize_mask_bits,
    tool_resize_width, tool_resize_height,
    0, 0, NULL, NULL,

    tool_resize, NULL
  },
  {
    tool_rotate_bits, tool_rotate_mask_bits,
    tool_rotate_width, tool_rotate_height,
    0, 0, NULL, NULL,

    tool_rotate, NULL
  },
  {
    tool_shear_bits, tool_shear_mask_bits,
    tool_shear_width, tool_shear_height,
    0, 0, NULL, NULL,

    tool_shear, NULL
  },
  {
    tool_perspective_bits, tool_perspective_mask_bits,
    tool_perspective_width, tool_perspective_height,
    0, 0, NULL, NULL,

    tool_perspective, NULL
  },
  {
    tool_flip_horizontal_bits, tool_flip_horizontal_mask_bits,
    tool_flip_horizontal_width, tool_flip_horizontal_height,
    0, 0, NULL, NULL,

    tool_flip_horizontal, NULL
  },
  {
    tool_flip_vertical_bits, tool_flip_vertical_mask_bits,
    tool_flip_vertical_width, tool_flip_vertical_height,
    0, 0, NULL, NULL,

    tool_flip_vertical, NULL
  },
  {
    tool_text_bits, tool_text_mask_bits,
    tool_text_width, tool_text_height,
    0, 0, NULL, NULL,

    tool_text, NULL
  },
  {
    tool_color_picker_bits, tool_color_picker_mask_bits,
    tool_color_picker_width, tool_color_picker_height,
    0, 0, NULL, NULL,

    tool_color_picker, NULL
  },
  {
    tool_bucket_fill_bits, tool_bucket_fill_mask_bits,
    tool_bucket_fill_width, tool_bucket_fill_height,
    0, 0, NULL, NULL,

    tool_bucket_fill, NULL
  },
  {
    tool_blend_bits, tool_blend_mask_bits,
    tool_blend_width, tool_blend_height,
    0, 0, NULL, NULL,

    tool_blend, NULL
  },
  {
    tool_pencil_bits, tool_pencil_mask_bits,
    tool_pencil_width, tool_pencil_height,
    0, 0, NULL, NULL,

    tool_pencil, NULL
  },
  {
    tool_paintbrush_bits, tool_paintbrush_mask_bits,
    tool_paintbrush_width, tool_paintbrush_height,
    0, 0, NULL, NULL,

    tool_paintbrush, NULL
  },
  {
    tool_airbrush_bits, tool_airbrush_mask_bits,
    tool_airbrush_width, tool_airbrush_height,
    0, 0, NULL, NULL,

    tool_airbrush, NULL
  },
  {
    tool_ink_bits, tool_ink_mask_bits,
    tool_ink_width, tool_ink_height,
    0, 0, NULL, NULL,

    tool_ink, NULL
  },
  {
    tool_clone_bits, tool_clone_mask_bits,
    tool_clone_width, tool_clone_height,
    0, 0, NULL, NULL,

    tool_clone, NULL
  },
  {
    tool_eraser_bits, tool_eraser_mask_bits,
    tool_eraser_width, tool_eraser_height,
    0, 0, NULL, NULL,

    tool_eraser, NULL
  },
  {
    tool_smudge_bits, tool_smudge_mask_bits,
    tool_smudge_width, tool_smudge_height,
    0, 0, NULL, NULL,

    tool_smudge, NULL
  },
  {
    tool_blur_bits, tool_blur_mask_bits,
    tool_blur_width, tool_blur_height,
    0, 0, NULL, NULL,

    tool_blur, NULL
  },
  {
    tool_dodge_bits, tool_dodge_mask_bits,
    tool_dodge_width, tool_dodge_height,
    0, 0, NULL, NULL,

    tool_dodge, NULL
  },
  {
    tool_burn_bits, tool_burn_mask_bits,
    tool_burn_width, tool_burn_height,
    0, 0, NULL, NULL,

    tool_burn, NULL
  },
  {
    tool_measure_bits, tool_measure_mask_bits,
    tool_measure_width, tool_measure_height,
    0, 0, NULL, NULL,

    tool_measure, NULL
  },
  {
    tool_hand_bits, tool_hand_mask_bits,
    tool_hand_width, tool_hand_height,
    0, 0, NULL, NULL,

    tool_hand, NULL
  }
};

static GimpBitmapCursor gimp_modifier_cursors[] =
{
  /* these have to match up with enum GimpCursorModifier in widgets-enums.h */

  {
    NULL, NULL,
    0, 0,
    0, 0, NULL, NULL,

    NULL, NULL
  },
  {
    modifier_plus_bits, modifier_plus_mask_bits,
    modifier_plus_width, modifier_plus_height,
    0, 0, NULL, NULL,

    modifier_plus, NULL
  },
  {
    modifier_minus_bits, modifier_minus_mask_bits,
    modifier_minus_width, modifier_minus_height,
    0, 0, NULL, NULL,

    modifier_minus, NULL
  },
  {
    modifier_intersect_bits, modifier_intersect_mask_bits,
    modifier_intersect_width, modifier_intersect_height,
    0, 0, NULL, NULL,

    modifier_intersect, NULL
  },
  {
    modifier_move_bits, modifier_move_mask_bits,
    modifier_move_width, modifier_move_height,
    0, 0, NULL, NULL,

    modifier_move, NULL
  },
  {
    modifier_resize_bits, modifier_resize_mask_bits,
    modifier_resize_width, modifier_resize_height,
    0, 0, NULL, NULL,

    modifier_resize, NULL
  },
  {
    modifier_control_bits, modifier_control_mask_bits,
    modifier_control_width, modifier_control_height,
    0, 0, NULL, NULL,

    modifier_control, NULL
  },
  {
    modifier_anchor_bits, modifier_anchor_mask_bits,
    modifier_anchor_width, modifier_anchor_height,
    0, 0, NULL, NULL,

    modifier_anchor, NULL
  },
  {
    modifier_foreground_bits, modifier_foreground_mask_bits,
    modifier_foreground_width, modifier_foreground_height,
    0, 0, NULL, NULL,

    modifier_foreground, NULL
  },
  {
    modifier_background_bits, modifier_background_mask_bits,
    modifier_background_width, modifier_background_height,
    0, 0, NULL, NULL,

    modifier_background, NULL
  },
  {
    modifier_pattern_bits, modifier_pattern_mask_bits,
    modifier_pattern_width, modifier_pattern_height,
    0, 0, NULL, NULL,

    modifier_pattern, NULL
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

static void
create_cursor_pixbuf (GimpBitmapCursor *bmcursor)
{
  if (bmcursor->pixbuf == NULL)
    bmcursor->pixbuf = gdk_pixbuf_new_from_inline (-1, bmcursor->pixbuf_data,
                                                   FALSE, NULL);

  g_return_if_fail (bmcursor->pixbuf != NULL);
}

GdkCursor *
gimp_cursor_new (GdkDisplay         *display,
                 GimpCursorType      cursor_type,
                 GimpToolCursorType  tool_cursor,
                 GimpCursorModifier  modifier)
{
  GdkCursor        *cursor;
  gint              width;
  gint              height;
  GimpBitmapCursor *bmcursor   = NULL;
  GimpBitmapCursor *bmmodifier = NULL;
  GimpBitmapCursor *bmtool     = NULL;

  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (cursor_type < GIMP_LAST_CURSOR_ENTRY, NULL);

  if (cursor_type <= GDK_LAST_CURSOR)
    return gdk_cursor_new_for_display (display, cursor_type);

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

  /*  prepare the main cursor  */

  cursor_type -= GIMP_MOUSE_CURSOR;
  bmcursor = &gimp_cursors[cursor_type];

  /*  prepare the tool cursor  */

  if (tool_cursor >= GIMP_LAST_STOCK_TOOL_CURSOR_ENTRY)
    tool_cursor = GIMP_TOOL_CURSOR_NONE;

  if (tool_cursor != GIMP_TOOL_CURSOR_NONE)
    bmtool = &gimp_stock_tool_cursors[tool_cursor];

  /*  prepare the cursor modifier  */

  if (modifier >= GIMP_LAST_CURSOR_MODIFIER_ENTRY)
    modifier = GIMP_CURSOR_MODIFIER_NONE;

  if (modifier != GIMP_CURSOR_MODIFIER_NONE)
    bmmodifier = &gimp_modifier_cursors[modifier];

  if (gdk_display_supports_cursor_alpha (display) &&
      gdk_display_supports_cursor_color (display))
    {
      GdkPixbuf *pixbuf;

      if (! bmcursor->pixbuf)
        create_cursor_pixbuf (bmcursor);

      width  = gdk_pixbuf_get_width  (bmcursor->pixbuf);
      height = gdk_pixbuf_get_height (bmcursor->pixbuf);

      pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
      gdk_pixbuf_fill (pixbuf, 0);

      gdk_pixbuf_composite (bmcursor->pixbuf, pixbuf,
                            0, 0, width, height,
                            0.0, 0.0, 1.0, 1.0,
                            GDK_INTERP_NEAREST, 255);

      if (bmmodifier)
        {
          if (bmmodifier->pixbuf)
            create_cursor_pixbuf (bmmodifier);

          gdk_pixbuf_composite (bmmodifier->pixbuf, pixbuf,
                                0, 0, width, height,
                                0.0, 0.0, 1.0, 1.0,
                                GDK_INTERP_NEAREST, 180);
        }

      if (bmtool)
        {
          if (! bmtool->pixbuf)
            create_cursor_pixbuf (bmtool);

          gdk_pixbuf_composite (bmtool->pixbuf, pixbuf,
                                0, 0, width, height,
                                0.0, 0.0, 1.0, 1.0,
                                GDK_INTERP_NEAREST, 180);
        }

      cursor = gdk_cursor_new_from_pixbuf (display, pixbuf,
                                           bmcursor->x_hot,
                                           bmcursor->y_hot);
      g_object_unref (pixbuf);
    }
  else
    {
      GdkBitmap       *bitmap;
      GdkBitmap       *mask;
      GdkColor         color;
      static GdkGC    *gc = NULL;
      static GdkColor  fg, bg;

      if (! bmcursor->bitmap || ! bmcursor->mask)
        create_cursor_bitmaps (bmcursor);

      if (gc == NULL)
        {
          gc = gdk_gc_new (bmcursor->bitmap);

          /* should have a way to configure the mouse colors */
          gdk_color_parse ("#FFFFFF", &bg);
          gdk_color_parse ("#000000", &fg);
        }

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
          if (! bmmodifier->bitmap || ! bmmodifier->mask)
            create_cursor_bitmaps (bmmodifier);

          gdk_gc_set_clip_mask (gc, bmmodifier->bitmap);
          gdk_draw_drawable (bitmap, gc, bmmodifier->bitmap,
                             0, 0, 0, 0, width, height);
          gdk_gc_set_clip_mask (gc, NULL);
        }

      if (bmtool)
        {
          if (! bmtool->bitmap || ! bmtool->mask)
            create_cursor_bitmaps (bmtool);

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

      cursor = gdk_cursor_new_from_pixmap (bitmap, mask,
                                           &fg, &bg,
                                           bmcursor->x_hot,
                                           bmcursor->y_hot);

      g_object_unref (bitmap);
      g_object_unref (mask);
    }

  return cursor;
}

void
gimp_cursor_set (GtkWidget          *widget,
                 GimpCursorType      cursor_type,
                 GimpToolCursorType  tool_cursor,
                 GimpCursorModifier  modifier)
{
  GdkCursor *cursor;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (GTK_WIDGET_REALIZED (widget));

  cursor = gimp_cursor_new (gtk_widget_get_display (widget),
                            cursor_type,
                            tool_cursor,
                            modifier);
  gdk_window_set_cursor (widget->window, cursor);
  gdk_cursor_unref (cursor);
}
