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

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "gimpcursor.h"

#include "cursors/gimp-tool-cursors.h"


#define cursor_none_x_hot 10
#define cursor_none_y_hot 10
#define cursor_mouse_x_hot 3
#define cursor_mouse_y_hot 2
#define cursor_crosshair_x_hot 15
#define cursor_crosshair_y_hot 15
#define cursor_crosshair_small_x_hot 10
#define cursor_crosshair_small_y_hot 10
#define cursor_bad_x_hot 10
#define cursor_bad_y_hot 10
#define cursor_move_x_hot 10
#define cursor_move_y_hot 10
#define cursor_zoom_x_hot 8
#define cursor_zoom_y_hot 8
#define cursor_color_picker_x_hot 1
#define cursor_color_picker_y_hot 30
#define cursor_corner_top_left_x_hot 10
#define cursor_corner_top_left_y_hot 10
#define cursor_corner_top_right_x_hot 10
#define cursor_corner_top_right_y_hot 10
#define cursor_corner_bottom_left_x_hot 10
#define cursor_corner_bottom_left_y_hot 10
#define cursor_corner_bottom_right_x_hot 10
#define cursor_corner_bottom_right_y_hot 10
#define cursor_side_top_x_hot 10
#define cursor_side_top_y_hot 10
#define cursor_side_left_x_hot 10
#define cursor_side_left_y_hot 10
#define cursor_side_right_x_hot 10
#define cursor_side_right_y_hot 10
#define cursor_side_bottom_x_hot 10
#define cursor_side_bottom_y_hot 10


typedef struct _GimpCursor GimpCursor;

struct _GimpCursor
{
  const guint8 *pixbuf_data;
  const guint8 *pixbuf_data_bw;
  const gint    x_hot, y_hot;

  GdkPixbuf    *pixbuf;
  GdkPixbuf    *pixbuf_bw;
};


static GimpCursor gimp_cursors[] =
{
  /* these have to match up with enum GimpCursorType in widgets-enums.h */

  {
    cursor_none,
    cursor_none_bw,
    cursor_none_x_hot, cursor_none_y_hot
  },
  {
    cursor_mouse,
    cursor_mouse_bw,
    cursor_mouse_x_hot, cursor_mouse_y_hot
  },
  {
    cursor_crosshair,
    cursor_crosshair_bw,
    cursor_crosshair_x_hot, cursor_crosshair_y_hot
  },
  {
    cursor_crosshair_small,
    cursor_crosshair_small_bw,
    cursor_crosshair_small_x_hot, cursor_crosshair_small_y_hot
  },
  {
    cursor_bad,
    cursor_bad_bw,
    cursor_bad_x_hot, cursor_bad_y_hot
  },
  {
    cursor_move,
    cursor_move_bw,
    cursor_move_x_hot, cursor_move_y_hot
  },
  {
    cursor_zoom,
    cursor_zoom_bw,
    cursor_zoom_x_hot, cursor_zoom_y_hot
  },
  {
    cursor_color_picker,
    cursor_color_picker_bw,
    cursor_color_picker_x_hot, cursor_color_picker_y_hot
  },
  {
    cursor_corner_top_left,
    cursor_corner_top_left_bw,
    cursor_corner_top_left_x_hot, cursor_corner_top_left_y_hot
  },
  {
    cursor_corner_top_right,
    cursor_corner_top_right_bw,
    cursor_corner_top_right_x_hot, cursor_corner_top_right_y_hot
  },
  {
    cursor_corner_bottom_left,
    cursor_corner_bottom_left_bw,
    cursor_corner_bottom_left_x_hot, cursor_corner_bottom_left_y_hot
  },
  {
    cursor_corner_bottom_right,
    cursor_corner_bottom_right_bw,
    cursor_corner_bottom_right_x_hot, cursor_corner_bottom_right_y_hot
  },
  {
    cursor_side_top,
    cursor_side_top_bw,
    cursor_side_top_x_hot, cursor_side_top_y_hot
  },
  {
    cursor_side_left,
    cursor_side_left_bw,
    cursor_side_left_x_hot, cursor_side_left_y_hot
  },
  {
    cursor_side_right,
    cursor_side_right_bw,
    cursor_side_right_x_hot, cursor_side_right_y_hot
  },
  {
    cursor_side_bottom,
    cursor_side_bottom_bw,
    cursor_side_bottom_x_hot, cursor_side_bottom_y_hot
  }
};

static GimpCursor gimp_tool_cursors[] =
{
  /* these have to match up with enum GimpToolCursorType in widgets-enums.h */

  { NULL },
  { tool_rect_select, tool_rect_select_bw },
  { tool_ellipse_select, tool_ellipse_select_bw },
  { tool_free_select, tool_free_select_bw },
  { tool_polygon_select, tool_polygon_select_bw },
  { tool_fuzzy_select, tool_fuzzy_select_bw },
  { tool_paths, tool_paths_bw },
  { tool_paths_anchor, tool_paths_anchor_bw },
  { tool_paths_control, tool_paths_control_bw },
  { tool_paths_segment, tool_paths_segment_bw },
  { tool_iscissors, tool_iscissors_bw },
  { tool_move, tool_move_bw },
  { tool_zoom, tool_zoom_bw },
  { tool_crop, tool_crop_bw },
  { tool_resize, tool_resize_bw },
  { tool_rotate, tool_rotate_bw },
  { tool_shear, tool_shear_bw },
  { tool_perspective, tool_perspective_bw },
  { tool_flip_horizontal, tool_flip_horizontal_bw },
  { tool_flip_vertical, tool_flip_vertical_bw },
  { tool_text, tool_text_bw },
  { tool_color_picker, tool_color_picker_bw },
  { tool_bucket_fill, tool_bucket_fill_bw },
  { tool_blend, tool_blend_bw },
  { tool_pencil, tool_pencil_bw },
  { tool_paintbrush, tool_paintbrush_bw },
  { tool_airbrush, tool_airbrush_bw },
  { tool_ink, tool_ink_bw },
  { tool_clone, tool_clone_bw },
  { tool_heal, tool_heal_bw },
  { tool_eraser, tool_eraser_bw },
  { tool_smudge, tool_smudge_bw },
  { tool_blur, tool_blur_bw },
  { tool_dodge, tool_dodge_bw },
  { tool_burn, tool_burn_bw },
  { tool_measure, tool_measure_bw },
  { tool_hand, tool_hand_bw }
};

static GimpCursor gimp_cursor_modifiers[] =
{
  /* these have to match up with enum GimpCursorModifier in widgets-enums.h */

  { NULL },
  { modifier_bad, modifier_bad },
  { modifier_plus, modifier_plus },
  { modifier_minus, modifier_minus },
  { modifier_intersect, modifier_intersect },
  { modifier_move, modifier_move },
  { modifier_resize, modifier_resize },
  { modifier_control, modifier_control },
  { modifier_anchor, modifier_anchor },
  { modifier_foreground, modifier_foreground },
  { modifier_background, modifier_background },
  { modifier_pattern, modifier_pattern },
  { modifier_join, modifier_join },
  { modifier_select, modifier_select }
};

static const GdkPixbuf *
get_cursor_pixbuf (GimpCursor *cursor,
                   gboolean    bw)
{
  GdkPixbuf **pixbuf;

  if (bw)
    pixbuf = &cursor->pixbuf_bw;
  else
    pixbuf = &cursor->pixbuf;

  if (! *pixbuf)
    *pixbuf = gdk_pixbuf_new_from_inline (-1,
                                          bw ?
                                          cursor->pixbuf_data_bw :
                                          cursor->pixbuf_data,
                                          FALSE, NULL);
  g_return_val_if_fail (*pixbuf != NULL, NULL);

  return *pixbuf;
}

GdkCursor *
gimp_cursor_new (GdkDisplay         *display,
                 GimpCursorFormat    cursor_format,
                 GimpHandedness      cursor_handedness,
                 GimpCursorType      cursor_type,
                 GimpToolCursorType  tool_cursor,
                 GimpCursorModifier  modifier)
{
  GimpCursor *bmcursor   = NULL;
  GimpCursor *bmmodifier = NULL;
  GimpCursor *bmtool     = NULL;
  GdkCursor  *cursor;
  GdkPixbuf  *pixbuf;
  gboolean    bw;

  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (cursor_type < GIMP_CURSOR_LAST, NULL);

  if (cursor_type <= (GimpCursorType) GDK_LAST_CURSOR)
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

  /*  when cursor is "corner" or "side" sides must be exchanged for
   *  left-hand-mice-flipping of pixbuf below
   */

  if (cursor_handedness == GIMP_HANDEDNESS_LEFT)
    {
      if (cursor_type == GIMP_CURSOR_CORNER_TOP_LEFT)
        {
          cursor_type = GIMP_CURSOR_CORNER_TOP_RIGHT;
        }
      else if (cursor_type == GIMP_CURSOR_CORNER_TOP_RIGHT)
        {
          cursor_type = GIMP_CURSOR_CORNER_TOP_LEFT;
        }
      else if (cursor_type == GIMP_CURSOR_CORNER_BOTTOM_LEFT)
        {
          cursor_type = GIMP_CURSOR_CORNER_BOTTOM_RIGHT;
        }
      else if (cursor_type == GIMP_CURSOR_CORNER_BOTTOM_RIGHT)
        {
          cursor_type = GIMP_CURSOR_CORNER_BOTTOM_LEFT;
        }
      else if (cursor_type == GIMP_CURSOR_SIDE_LEFT)
        {
          cursor_type = GIMP_CURSOR_SIDE_RIGHT;
        }
      else if (cursor_type == GIMP_CURSOR_SIDE_RIGHT)
        {
          cursor_type = GIMP_CURSOR_SIDE_LEFT;
        }
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
      bw = FALSE;
    }
  else
    {
      bw = TRUE;
    }

  pixbuf = gdk_pixbuf_copy (get_cursor_pixbuf (bmcursor, bw));

  if (bmmodifier || bmtool)
    {
      gint width  = gdk_pixbuf_get_width  (pixbuf);
      gint height = gdk_pixbuf_get_height (pixbuf);

      if (bmmodifier)
        gdk_pixbuf_composite (get_cursor_pixbuf (bmmodifier, bw), pixbuf,
                              0, 0, width, height,
                              0.0, 0.0, 1.0, 1.0,
                              GDK_INTERP_NEAREST, bw ? 255 : 200);

      if (bmtool)
        gdk_pixbuf_composite (get_cursor_pixbuf (bmtool, bw), pixbuf,
                              0, 0, width, height,
                              0.0, 0.0, 1.0, 1.0,
                              GDK_INTERP_NEAREST, bw ? 255 : 200);
    }

  /*  flip the cursor if mouse setting is left-handed  */

  if (cursor_handedness == GIMP_HANDEDNESS_LEFT)
    {
      GdkPixbuf *flipped = gdk_pixbuf_flip (pixbuf, TRUE);
      gint       width   = gdk_pixbuf_get_width (flipped);

      cursor = gdk_cursor_new_from_pixbuf (display, flipped,
                                           (width - 1) - bmcursor->x_hot,
                                           bmcursor->y_hot);
      g_object_unref (flipped);
    }
  else
    {
      cursor = gdk_cursor_new_from_pixbuf (display, pixbuf,
                                           bmcursor->x_hot,
                                           bmcursor->y_hot);
    }

  g_object_unref (pixbuf);

  return cursor;
}

void
gimp_cursor_set (GtkWidget          *widget,
                 GimpCursorFormat    cursor_format,
                 GimpHandedness      cursor_handedness,
                 GimpCursorType      cursor_type,
                 GimpToolCursorType  tool_cursor,
                 GimpCursorModifier  modifier)
{
  GdkCursor *cursor;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (gtk_widget_get_realized (widget));

  cursor = gimp_cursor_new (gtk_widget_get_display (widget),
                            cursor_format,
                            cursor_handedness,
                            cursor_type,
                            tool_cursor,
                            modifier);
  gdk_window_set_cursor (gtk_widget_get_window (widget), cursor);
  gdk_cursor_unref (cursor);
}
