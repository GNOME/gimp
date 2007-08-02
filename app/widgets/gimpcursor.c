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

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "gimpcursor.h"

#include "cursors/gimp-tool-cursors.h"

/*  standard gimp cursors  */
#include "cursors/xbm/cursor-none.xbm"
#include "cursors/xbm/cursor-mouse.xbm"
#include "cursors/xbm/cursor-mouse-mask.xbm"
#include "cursors/xbm/cursor-move.xbm"
#include "cursors/xbm/cursor-move-mask.xbm"
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
#include "cursors/xbm/cursor-corner-top-left.xbm"
#include "cursors/xbm/cursor-corner-top-left-mask.xbm"
#include "cursors/xbm/cursor-corner-top-right.xbm"
#include "cursors/xbm/cursor-corner-top-right-mask.xbm"
#include "cursors/xbm/cursor-corner-bottom-left.xbm"
#include "cursors/xbm/cursor-corner-bottom-left-mask.xbm"
#include "cursors/xbm/cursor-corner-bottom-right.xbm"
#include "cursors/xbm/cursor-corner-bottom-right-mask.xbm"
#include "cursors/xbm/cursor-side-top.xbm"
#include "cursors/xbm/cursor-side-top-mask.xbm"
#include "cursors/xbm/cursor-side-left.xbm"
#include "cursors/xbm/cursor-side-left-mask.xbm"
#include "cursors/xbm/cursor-side-right.xbm"
#include "cursors/xbm/cursor-side-right-mask.xbm"
#include "cursors/xbm/cursor-side-bottom.xbm"
#include "cursors/xbm/cursor-side-bottom-mask.xbm"

/*  tool cursors  */
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
#include "cursors/xbm/tool-paths-anchor.xbm"
#include "cursors/xbm/tool-paths-anchor-mask.xbm"
#include "cursors/xbm/tool-paths-control.xbm"
#include "cursors/xbm/tool-paths-control-mask.xbm"
#include "cursors/xbm/tool-paths-segment.xbm"
#include "cursors/xbm/tool-paths-segment-mask.xbm"
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
#include "cursors/xbm/tool-heal.xbm"
#include "cursors/xbm/tool-heal-mask.xbm"
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

/*  cursor modifiers  */
#include "cursors/xbm/modifier-bad.xbm"
#include "cursors/xbm/modifier-bad-mask.xbm"
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
#include "cursors/xbm/modifier-join.xbm"
#include "cursors/xbm/modifier-join-mask.xbm"


typedef struct _GimpCursor GimpCursor;

struct _GimpCursor
{
  const guchar *bits;
  const guchar *mask_bits;
  const gint    width, height;
  const gint    x_hot, y_hot;
  const guint8 *pixbuf_data;

  GdkBitmap    *bitmap;
  GdkBitmap    *mask;
  GdkPixbuf    *pixbuf;
};


static GimpCursor gimp_cursors[] =
{
  /* these have to match up with enum GimpCursorType in widgets-enums.h */

  {
    cursor_none_bits, cursor_none_bits,
    cursor_none_width, cursor_none_height,
    cursor_none_x_hot, cursor_none_y_hot,
    cursor_none, NULL, NULL, NULL
  },
  {
    cursor_mouse_bits, cursor_mouse_mask_bits,
    cursor_mouse_width, cursor_mouse_height,
    cursor_mouse_x_hot, cursor_mouse_y_hot,
    cursor_mouse, NULL, NULL, NULL
  },
  {
    cursor_crosshair_bits, cursor_crosshair_mask_bits,
    cursor_crosshair_width, cursor_crosshair_height,
    cursor_crosshair_x_hot, cursor_crosshair_y_hot,
    cursor_crosshair, NULL, NULL, NULL
  },
  {
    cursor_crosshair_small_bits, cursor_crosshair_small_mask_bits,
    cursor_crosshair_small_width, cursor_crosshair_small_height,
    cursor_crosshair_small_x_hot, cursor_crosshair_small_y_hot,
    cursor_crosshair_small, NULL, NULL, NULL
  },
  {
    cursor_bad_bits, cursor_bad_mask_bits,
    cursor_bad_width, cursor_bad_height,
    cursor_bad_x_hot, cursor_bad_y_hot,
    cursor_bad, NULL, NULL, NULL
  },
  {
    cursor_move_bits, cursor_move_mask_bits,
    cursor_move_width, cursor_move_height,
    cursor_move_x_hot, cursor_move_y_hot,
    cursor_move, NULL, NULL, NULL
  },
  {
    cursor_zoom_bits, cursor_zoom_mask_bits,
    cursor_zoom_width, cursor_zoom_height,
    cursor_zoom_x_hot, cursor_zoom_y_hot,
    cursor_zoom, NULL, NULL, NULL
  },
  {
    cursor_color_picker_bits, cursor_color_picker_mask_bits,
    cursor_color_picker_width, cursor_color_picker_height,
    cursor_color_picker_x_hot, cursor_color_picker_y_hot,
    cursor_color_picker, NULL, NULL, NULL
  },
  {
    cursor_corner_top_left_bits, cursor_corner_top_left_mask_bits,
    cursor_corner_top_left_width, cursor_corner_top_left_height,
    cursor_corner_top_left_x_hot, cursor_corner_top_left_y_hot,
    cursor_corner_top_left, NULL, NULL, NULL
  },
  {
    cursor_corner_top_right_bits, cursor_corner_top_right_mask_bits,
    cursor_corner_top_right_width, cursor_corner_top_right_height,
    cursor_corner_top_right_x_hot, cursor_corner_top_right_y_hot,
    cursor_corner_top_right, NULL, NULL, NULL
  },
  {
    cursor_corner_bottom_left_bits, cursor_corner_bottom_left_mask_bits,
    cursor_corner_bottom_left_width, cursor_corner_bottom_left_height,
    cursor_corner_bottom_left_x_hot, cursor_corner_bottom_left_y_hot,
    cursor_corner_bottom_left, NULL, NULL, NULL
  },
  {
    cursor_corner_bottom_right_bits, cursor_corner_bottom_right_mask_bits,
    cursor_corner_bottom_right_width, cursor_corner_bottom_right_height,
    cursor_corner_bottom_right_x_hot, cursor_corner_bottom_right_y_hot,
    cursor_corner_bottom_right, NULL, NULL, NULL
  },
  {
    cursor_side_top_bits, cursor_side_top_mask_bits,
    cursor_side_top_width, cursor_side_top_height,
    cursor_side_top_x_hot, cursor_side_top_y_hot,
    cursor_side_top, NULL, NULL, NULL
  },
  {
    cursor_side_left_bits, cursor_side_left_mask_bits,
    cursor_side_left_width, cursor_side_left_height,
    cursor_side_left_x_hot, cursor_side_left_y_hot,
    cursor_side_left, NULL, NULL, NULL
  },
  {
    cursor_side_right_bits, cursor_side_right_mask_bits,
    cursor_side_right_width, cursor_side_right_height,
    cursor_side_right_x_hot, cursor_side_right_y_hot,
    cursor_side_right, NULL, NULL, NULL
  },
  {
    cursor_side_bottom_bits, cursor_side_bottom_mask_bits,
    cursor_side_bottom_width, cursor_side_bottom_height,
    cursor_side_bottom_x_hot, cursor_side_bottom_y_hot,
    cursor_side_bottom, NULL, NULL, NULL
  }
};

static GimpCursor gimp_tool_cursors[] =
{
  /* these have to match up with enum GimpToolCursorType in widgets-enums.h */

  {
    NULL, NULL,
    0, 0,
    0, 0,
    NULL, NULL, NULL, NULL
  },
  {
    tool_rect_select_bits, tool_rect_select_mask_bits,
    tool_rect_select_width, tool_rect_select_height,
    0, 0,
    tool_rect_select, NULL, NULL, NULL
  },
  {
    tool_ellipse_select_bits, tool_ellipse_select_mask_bits,
    tool_ellipse_select_width, tool_ellipse_select_height,
    0, 0,
    tool_ellipse_select, NULL, NULL, NULL
  },
  {
    tool_free_select_bits, tool_free_select_mask_bits,
    tool_free_select_width, tool_free_select_height,
    0, 0,
    tool_free_select, NULL, NULL, NULL
  },
  {
    tool_fuzzy_select_bits, tool_fuzzy_select_mask_bits,
    tool_fuzzy_select_width, tool_fuzzy_select_height,
    0, 0,
    tool_fuzzy_select, NULL, NULL, NULL
  },
  {
    tool_paths_bits, tool_paths_mask_bits,
    tool_paths_width, tool_paths_height,
    0, 0,
    tool_paths, NULL, NULL, NULL
  },
  {
    tool_paths_anchor_bits, tool_paths_anchor_mask_bits,
    tool_paths_anchor_width, tool_paths_anchor_height,
    0, 0,
    tool_paths_anchor, NULL, NULL, NULL
  },
  {
    tool_paths_control_bits, tool_paths_control_mask_bits,
    tool_paths_control_width, tool_paths_control_height,
    0, 0,
    tool_paths_control, NULL, NULL, NULL
  },
  {
    tool_paths_segment_bits, tool_paths_segment_mask_bits,
    tool_paths_segment_width, tool_paths_segment_height,
    0, 0,
    tool_paths_segment, NULL, NULL, NULL
  },
  {
    tool_iscissors_bits, tool_iscissors_mask_bits,
    tool_iscissors_width, tool_iscissors_height,
    0, 0,
    tool_iscissors, NULL, NULL, NULL
  },
  {
    tool_move_bits, tool_move_mask_bits,
    tool_move_width, tool_move_height,
    0, 0,
    tool_move, NULL, NULL, NULL
  },
  {
    tool_zoom_bits, tool_zoom_mask_bits,
    tool_zoom_width, tool_zoom_height,
    0, 0,
    tool_zoom, NULL, NULL, NULL
  },
  {
    tool_crop_bits, tool_crop_mask_bits,
    tool_crop_width, tool_crop_height,
    0, 0,
    tool_crop, NULL, NULL, NULL
  },
  {
    tool_resize_bits, tool_resize_mask_bits,
    tool_resize_width, tool_resize_height,
    0, 0,
    tool_resize, NULL, NULL, NULL
  },
  {
    tool_rotate_bits, tool_rotate_mask_bits,
    tool_rotate_width, tool_rotate_height,
    0, 0,
    tool_rotate, NULL, NULL, NULL
  },
  {
    tool_shear_bits, tool_shear_mask_bits,
    tool_shear_width, tool_shear_height,
    0, 0,
    tool_shear, NULL, NULL, NULL
  },
  {
    tool_perspective_bits, tool_perspective_mask_bits,
    tool_perspective_width, tool_perspective_height,
    0, 0,
    tool_perspective, NULL, NULL, NULL
  },
  {
    tool_flip_horizontal_bits, tool_flip_horizontal_mask_bits,
    tool_flip_horizontal_width, tool_flip_horizontal_height,
    0, 0,
    tool_flip_horizontal, NULL, NULL, NULL
  },
  {
    tool_flip_vertical_bits, tool_flip_vertical_mask_bits,
    tool_flip_vertical_width, tool_flip_vertical_height,
    0, 0,
    tool_flip_vertical, NULL, NULL, NULL
  },
  {
    tool_text_bits, tool_text_mask_bits,
    tool_text_width, tool_text_height,
    0, 0,
    tool_text, NULL, NULL, NULL
  },
  {
    tool_color_picker_bits, tool_color_picker_mask_bits,
    tool_color_picker_width, tool_color_picker_height,
    0, 0,
    tool_color_picker, NULL, NULL, NULL
  },
  {
    tool_bucket_fill_bits, tool_bucket_fill_mask_bits,
    tool_bucket_fill_width, tool_bucket_fill_height,
    0, 0,
    tool_bucket_fill, NULL, NULL, NULL
  },
  {
    tool_blend_bits, tool_blend_mask_bits,
    tool_blend_width, tool_blend_height,
    0, 0,
    tool_blend, NULL, NULL, NULL
  },
  {
    tool_pencil_bits, tool_pencil_mask_bits,
    tool_pencil_width, tool_pencil_height,
    0, 0,
    tool_pencil, NULL, NULL, NULL
  },
  {
    tool_paintbrush_bits, tool_paintbrush_mask_bits,
    tool_paintbrush_width, tool_paintbrush_height,
    0, 0,
    tool_paintbrush, NULL, NULL, NULL
  },
  {
    tool_airbrush_bits, tool_airbrush_mask_bits,
    tool_airbrush_width, tool_airbrush_height,
    0, 0,
    tool_airbrush, NULL, NULL, NULL
  },
  {
    tool_ink_bits, tool_ink_mask_bits,
    tool_ink_width, tool_ink_height,
    0, 0,
    tool_ink, NULL, NULL, NULL
  },
  {
    tool_clone_bits, tool_clone_mask_bits,
    tool_clone_width, tool_clone_height,
    0, 0,
    tool_clone, NULL, NULL, NULL
  },
  {
    tool_heal_bits, tool_heal_mask_bits,
    tool_heal_width, tool_heal_height,
    0, 0,
    tool_heal, NULL, NULL, NULL
  },
  {
    tool_eraser_bits, tool_eraser_mask_bits,
    tool_eraser_width, tool_eraser_height,
    0, 0,
    tool_eraser, NULL, NULL, NULL
  },
  {
    tool_smudge_bits, tool_smudge_mask_bits,
    tool_smudge_width, tool_smudge_height,
    0, 0,
    tool_smudge, NULL, NULL, NULL
  },
  {
    tool_blur_bits, tool_blur_mask_bits,
    tool_blur_width, tool_blur_height,
    0, 0,
    tool_blur, NULL, NULL, NULL
  },
  {
    tool_dodge_bits, tool_dodge_mask_bits,
    tool_dodge_width, tool_dodge_height,
    0, 0,
    tool_dodge, NULL, NULL, NULL
  },
  {
    tool_burn_bits, tool_burn_mask_bits,
    tool_burn_width, tool_burn_height,
    0, 0,
    tool_burn, NULL, NULL, NULL
  },
  {
    tool_measure_bits, tool_measure_mask_bits,
    tool_measure_width, tool_measure_height,
    0, 0,
    tool_measure, NULL, NULL, NULL
  },
  {
    tool_hand_bits, tool_hand_mask_bits,
    tool_hand_width, tool_hand_height,
    0, 0,
    tool_hand, NULL, NULL, NULL
  }
};

static GimpCursor gimp_cursor_modifiers[] =
{
  /* these have to match up with enum GimpCursorModifier in widgets-enums.h */

  {
    NULL, NULL,
    0, 0,
    0, 0,
    NULL, NULL, NULL, NULL
  },
  {
    modifier_bad_bits, modifier_bad_mask_bits,
    modifier_bad_width, modifier_bad_height,
    0, 0,
    modifier_bad, NULL, NULL, NULL
  },
  {
    modifier_plus_bits, modifier_plus_mask_bits,
    modifier_plus_width, modifier_plus_height,
    0, 0,
    modifier_plus, NULL, NULL, NULL
  },
  {
    modifier_minus_bits, modifier_minus_mask_bits,
    modifier_minus_width, modifier_minus_height,
    0, 0,
    modifier_minus, NULL, NULL, NULL
  },
  {
    modifier_intersect_bits, modifier_intersect_mask_bits,
    modifier_intersect_width, modifier_intersect_height,
    0, 0,
    modifier_intersect, NULL, NULL, NULL
  },
  {
    modifier_move_bits, modifier_move_mask_bits,
    modifier_move_width, modifier_move_height,
    0, 0,
    modifier_move, NULL, NULL, NULL
  },
  {
    modifier_resize_bits, modifier_resize_mask_bits,
    modifier_resize_width, modifier_resize_height,
    0, 0,
    modifier_resize, NULL, NULL, NULL
  },
  {
    modifier_control_bits, modifier_control_mask_bits,
    modifier_control_width, modifier_control_height,
    0, 0,
    modifier_control, NULL, NULL, NULL
  },
  {
    modifier_anchor_bits, modifier_anchor_mask_bits,
    modifier_anchor_width, modifier_anchor_height,
    0, 0,
    modifier_anchor, NULL, NULL, NULL
  },
  {
    modifier_foreground_bits, modifier_foreground_mask_bits,
    modifier_foreground_width, modifier_foreground_height,
    0, 0,
    modifier_foreground, NULL, NULL, NULL
  },
  {
    modifier_background_bits, modifier_background_mask_bits,
    modifier_background_width, modifier_background_height,
    0, 0,
    modifier_background, NULL, NULL, NULL
  },
  {
    modifier_pattern_bits, modifier_pattern_mask_bits,
    modifier_pattern_width, modifier_pattern_height,
    0, 0,
    modifier_pattern, NULL, NULL, NULL
  },
  {
    modifier_join_bits, modifier_join_mask_bits,
    modifier_join_width, modifier_join_height,
    0, 0,
    modifier_join, NULL, NULL, NULL
  }
};

static GdkBitmap *
get_cursor_bitmap (GimpCursor *cursor)
{
  if (! cursor->bitmap)
    cursor->bitmap = gdk_bitmap_create_from_data (NULL,
                                                  (const gchar *) cursor->bits,
                                                  cursor->width,
                                                  cursor->height);
  g_return_val_if_fail (cursor->bitmap != NULL, NULL);

  return cursor->bitmap;
}

static GdkBitmap *
get_cursor_mask (GimpCursor *cursor)
{
  if (! cursor->mask)
    cursor->mask = gdk_bitmap_create_from_data (NULL,
                                                (const gchar *) cursor->mask_bits,
                                                cursor->width,
                                                cursor->height);
  g_return_val_if_fail (cursor->mask != NULL, NULL);

  return cursor->mask;
}

static const GdkPixbuf *
get_cursor_pixbuf (GimpCursor *cursor)
{
  if (! cursor->pixbuf)
    cursor->pixbuf = gdk_pixbuf_new_from_inline (-1, cursor->pixbuf_data,
                                                 FALSE, NULL);
  g_return_val_if_fail (cursor->pixbuf != NULL, NULL);

  return cursor->pixbuf;
}

GdkCursor *
gimp_cursor_new (GdkDisplay         *display,
                 GimpCursorFormat    cursor_format,
                 GimpCursorType      cursor_type,
                 GimpToolCursorType  tool_cursor,
                 GimpCursorModifier  modifier)
{
  GdkCursor  *cursor;
  gint        width;
  gint        height;
  GimpCursor *bmcursor   = NULL;
  GimpCursor *bmmodifier = NULL;
  GimpCursor *bmtool     = NULL;

  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (cursor_type < GIMP_CURSOR_LAST, NULL);

  if (cursor_type <= GDK_LAST_CURSOR)
    return gdk_cursor_new_for_display (display, cursor_type);

  g_return_val_if_fail (cursor_type >= GIMP_CURSOR_NONE, NULL);

  /*  disallow the small tool cursor with some cursors
   */
  if (cursor_type <= GIMP_CURSOR_NONE         ||
      cursor_type == GIMP_CURSOR_CROSSHAIR    ||
      cursor_type == GIMP_CURSOR_ZOOM         ||
      cursor_type == GIMP_CURSOR_COLOR_PICKER ||
      cursor_type >= GIMP_CURSOR_LAST)
    {
      tool_cursor = GIMP_TOOL_CURSOR_NONE;
    }

  /*  don't allow anything with the empty cursor
   */
  if (cursor_type == GIMP_CURSOR_NONE)
    {
      tool_cursor = GIMP_TOOL_CURSOR_NONE;
      modifier    = GIMP_CURSOR_MODIFIER_NONE;
    }

  /*  some more sanity checks
   */
  if (cursor_type == GIMP_CURSOR_MOVE &&
      modifier    == GIMP_CURSOR_MODIFIER_MOVE)
    {
      modifier = GIMP_CURSOR_MODIFIER_NONE;
    }

  /*  prepare the main cursor  */

  cursor_type -= GIMP_CURSOR_NONE;
  bmcursor = &gimp_cursors[cursor_type];

  /*  prepare the tool cursor  */

  if (tool_cursor > GIMP_TOOL_CURSOR_NONE &&
      tool_cursor < GIMP_TOOL_CURSOR_LAST)
    {
      bmtool = &gimp_tool_cursors[tool_cursor];
    }

  /*  prepare the cursor modifier  */

  if (modifier > GIMP_CURSOR_MODIFIER_NONE &&
      modifier < GIMP_CURSOR_MODIFIER_LAST)
    {
      bmmodifier = &gimp_cursor_modifiers[modifier];
    }

  if (cursor_format != GIMP_CURSOR_FORMAT_BITMAP  &&
      gdk_display_supports_cursor_alpha (display) &&
      gdk_display_supports_cursor_color (display))
    {
      GdkPixbuf *pixbuf;

      pixbuf = gdk_pixbuf_copy (get_cursor_pixbuf (bmcursor));

      width  = gdk_pixbuf_get_width  (pixbuf);
      height = gdk_pixbuf_get_height (pixbuf);

      if (bmmodifier)
        gdk_pixbuf_composite (get_cursor_pixbuf (bmmodifier), pixbuf,
                              0, 0, width, height,
                              0.0, 0.0, 1.0, 1.0,
                              GDK_INTERP_NEAREST, 200);

      if (bmtool)
        gdk_pixbuf_composite (get_cursor_pixbuf (bmtool), pixbuf,
                              0, 0, width, height,
                              0.0, 0.0, 1.0, 1.0,
                              GDK_INTERP_NEAREST, 200);

      cursor = gdk_cursor_new_from_pixbuf (display, pixbuf,
                                           bmcursor->x_hot,
                                           bmcursor->y_hot);
      g_object_unref (pixbuf);
    }
  else
    {
      GdkBitmap       *bitmap;
      GdkBitmap       *mask;
      static GdkGC    *gc = NULL;
      static GdkColor  fg, bg;

      if (! gc)
        {
          GdkColor color;

          gc = gdk_gc_new (get_cursor_bitmap (bmcursor));

          color.pixel = 1;
          gdk_gc_set_foreground (gc, &color);

          /* should have a way to configure the mouse colors */
          gdk_color_parse ("#FFFFFF", &bg);
          gdk_color_parse ("#000000", &fg);
        }

      gdk_drawable_get_size (get_cursor_bitmap (bmcursor), &width, &height);

      bitmap = gdk_pixmap_new (NULL, width, height, 1);
      mask   = gdk_pixmap_new (NULL, width, height, 1);

      gdk_draw_drawable (bitmap, gc, get_cursor_bitmap (bmcursor),
                         0, 0, 0, 0, width, height);
      gdk_draw_drawable (mask, gc, get_cursor_mask (bmcursor),
                         0, 0, 0, 0, width, height);

      if (bmmodifier)
        {
          gdk_gc_set_clip_mask (gc, get_cursor_bitmap (bmmodifier));
          gdk_draw_drawable (bitmap, gc, get_cursor_bitmap (bmmodifier),
                             0, 0, 0, 0, width, height);

          gdk_gc_set_clip_mask (gc, get_cursor_mask (bmmodifier));
          gdk_draw_drawable (mask, gc, get_cursor_mask (bmmodifier),
                             0, 0, 0, 0, width, height);
        }

      if (bmtool)
        {
          gdk_gc_set_clip_mask (gc, get_cursor_bitmap (bmtool));
          gdk_draw_drawable (bitmap, gc, get_cursor_bitmap (bmtool),
                             0, 0, 0, 0, width, height);

          gdk_gc_set_clip_mask (gc, get_cursor_mask (bmtool));
          gdk_draw_drawable (mask, gc, get_cursor_mask (bmtool),
                             0, 0, 0, 0, width, height);
        }

      if (bmmodifier || bmtool)
        gdk_gc_set_clip_mask (gc, NULL);

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
                 GimpCursorFormat    cursor_format,
                 GimpCursorType      cursor_type,
                 GimpToolCursorType  tool_cursor,
                 GimpCursorModifier  modifier)
{
  GdkCursor *cursor;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (GTK_WIDGET_REALIZED (widget));

  cursor = gimp_cursor_new (gtk_widget_get_display (widget),
                            cursor_format,
                            cursor_type,
                            tool_cursor,
                            modifier);
  gdk_window_set_cursor (widget->window, cursor);
  gdk_cursor_unref (cursor);
}
