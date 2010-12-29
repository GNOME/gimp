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

#include "cursors/gimp-tool-cursors.c"


#define cursor_default_x_hot 10
#define cursor_default_y_hot 10

#define cursor_mouse_x_hot 3
#define cursor_mouse_y_hot 2
#define cursor_crosshair_x_hot 15
#define cursor_crosshair_y_hot 15
#define cursor_zoom_x_hot 8
#define cursor_zoom_y_hot 8
#define cursor_color_picker_x_hot 1
#define cursor_color_picker_y_hot 30


typedef struct _GimpCursor GimpCursor;

struct _GimpCursor
{
  const gchar *resource_name;
  const gint   x_hot, y_hot;

  GdkPixbuf   *pixbuf;
};


static GimpCursor gimp_cursors[] =
{
  /* these have to match up with enum GimpCursorType in widgets-enums.h */

  {
    "cursor-none.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-mouse.png",
    cursor_mouse_x_hot, cursor_mouse_y_hot
  },
  {
    "cursor-crosshair.png",
    cursor_crosshair_x_hot, cursor_crosshair_y_hot
  },
  {
    "cursor-crosshair-small.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-bad.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-move.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-zoom.png",
    cursor_zoom_x_hot, cursor_zoom_y_hot
  },
  {
    "cursor-color-picker.png",
    cursor_color_picker_x_hot, cursor_color_picker_y_hot
  },
  {
    "cursor-corner-top.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-corner-top-right.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-corner-right.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-corner-bottom-right.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-corner-bottom.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-corner-bottom-left.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-corner-left.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-corner-top-left.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-side-top.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-side-top-right.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-side-right.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-side-bottom-right.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-side-bottom.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-side-bottom-left.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-side-left.png",
    cursor_default_x_hot, cursor_default_y_hot
  },
  {
    "cursor-side-top-left.png",
    cursor_default_x_hot, cursor_default_y_hot
  }
};

static GimpCursor gimp_tool_cursors[] =
{
  /* these have to match up with enum GimpToolCursorType in widgets-enums.h */

  { NULL },
  { "tool-rect-select.png" },
  { "tool-ellipse-select.png" },
  { "tool-free-select.png" },
  { "tool-polygon-select.png" },
  { "tool-fuzzy-select.png" },
  { "tool-paths.png" },
  { "tool-paths-anchor.png" },
  { "tool-paths-control.png" },
  { "tool-paths-segment.png" },
  { "tool-iscissors.png" },
  { "tool-move.png" },
  { "tool-zoom.png" },
  { "tool-crop.png" },
  { "tool-resize.png" },
  { "tool-rotate.png" },
  { "tool-shear.png" },
  { "tool-perspective.png" },
  { "tool-flip-horizontal.png" },
  { "tool-flip-vertical.png" },
  { "tool-text.png" },
  { "tool-color-picker.png" },
  { "tool-bucket-fill.png" },
  { "tool-blend.png" },
  { "tool-pencil.png" },
  { "tool-paintbrush.png" },
  { "tool-airbrush.png" },
  { "tool-ink.png" },
  { "tool-clone.png" },
  { "tool-heal.png" },
  { "tool-eraser.png" },
  { "tool-smudge.png" },
  { "tool-blur.png" },
  { "tool-dodge.png" },
  { "tool-burn.png" },
  { "tool-measure.png" },
  { "tool-hand.png" }
};

static GimpCursor gimp_cursor_modifiers[] =
{
  /* these have to match up with enum GimpCursorModifier in widgets-enums.h */

  { NULL },
  { "modifier-bad.png" },
  { "modifier-plus.png" },
  { "modifier-minus.png" },
  { "modifier-intersect.png" },
  { "modifier-move.png" },
  { "modifier-resize.png" },
  { "modifier-control.png" },
  { "modifier-anchor.png" },
  { "modifier-foreground.png" },
  { "modifier-background.png" },
  { "modifier-pattern.png" },
  { "modifier-join.png" },
  { "modifier-select.png" }
};


static const GdkPixbuf *
get_cursor_pixbuf (GimpCursor *cursor)
{
  if (! cursor->pixbuf)
    {
      gchar  *resource_path;
      GError *error = NULL;

      resource_path = g_strconcat ("/org/gimp/tool-cursors/",
                                   cursor->resource_name, NULL);

      cursor->pixbuf = gdk_pixbuf_new_from_resource (resource_path, &error);

      if (! cursor->pixbuf)
        {
          g_critical ("Failed to create cursor image: %s", error->message);
          g_clear_error (&error);
        }

      g_free (resource_path);
    }

  return cursor->pixbuf;
}

GdkCursor *
gimp_cursor_new (GdkDisplay         *display,
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

  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (cursor_type < GIMP_CURSOR_LAST, NULL);

  if (cursor_type <= (GimpCursorType) GDK_LAST_CURSOR)
    return gdk_cursor_new_for_display (display, (GdkCursorType) cursor_type);

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
      switch (cursor_type)
        {
        case GIMP_CURSOR_CORNER_TOP_LEFT:
          cursor_type = GIMP_CURSOR_CORNER_TOP_RIGHT; break;

        case GIMP_CURSOR_CORNER_TOP_RIGHT:
          cursor_type = GIMP_CURSOR_CORNER_TOP_LEFT; break;

        case GIMP_CURSOR_CORNER_LEFT:
          cursor_type = GIMP_CURSOR_CORNER_RIGHT; break;

        case GIMP_CURSOR_CORNER_RIGHT:
          cursor_type = GIMP_CURSOR_CORNER_LEFT; break;

        case GIMP_CURSOR_CORNER_BOTTOM_LEFT:
          cursor_type = GIMP_CURSOR_CORNER_BOTTOM_RIGHT; break;

        case GIMP_CURSOR_CORNER_BOTTOM_RIGHT:
          cursor_type = GIMP_CURSOR_CORNER_BOTTOM_LEFT; break;

        case GIMP_CURSOR_SIDE_TOP_LEFT:
          cursor_type = GIMP_CURSOR_SIDE_TOP_RIGHT; break;

        case GIMP_CURSOR_SIDE_TOP_RIGHT:
          cursor_type = GIMP_CURSOR_SIDE_TOP_LEFT; break;

        case GIMP_CURSOR_SIDE_LEFT:
          cursor_type = GIMP_CURSOR_SIDE_RIGHT; break;

        case GIMP_CURSOR_SIDE_RIGHT:
          cursor_type = GIMP_CURSOR_SIDE_LEFT; break;

        case GIMP_CURSOR_SIDE_BOTTOM_LEFT:
          cursor_type = GIMP_CURSOR_SIDE_BOTTOM_RIGHT; break;

        case GIMP_CURSOR_SIDE_BOTTOM_RIGHT:
          cursor_type = GIMP_CURSOR_SIDE_BOTTOM_LEFT; break;

        default:
          break;
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

  pixbuf = gdk_pixbuf_copy (get_cursor_pixbuf (bmcursor));

  if (bmmodifier || bmtool)
    {
      gint width  = gdk_pixbuf_get_width  (pixbuf);
      gint height = gdk_pixbuf_get_height (pixbuf);

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
                 GimpHandedness      cursor_handedness,
                 GimpCursorType      cursor_type,
                 GimpToolCursorType  tool_cursor,
                 GimpCursorModifier  modifier)
{
  GdkCursor *cursor;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (gtk_widget_get_realized (widget));

  cursor = gimp_cursor_new (gtk_widget_get_display (widget),
                            cursor_handedness,
                            cursor_type,
                            tool_cursor,
                            modifier);
  gdk_window_set_cursor (gtk_widget_get_window (widget), cursor);
  g_object_unref (cursor);

  gdk_display_flush (gtk_widget_get_display (widget));
}

GimpCursorType
gimp_cursor_rotate (GimpCursorType  cursor,
                    gdouble         angle)
{
  if (cursor >= GIMP_CURSOR_CORNER_TOP &&
      cursor <= GIMP_CURSOR_SIDE_TOP_LEFT)
    {
      gint offset = (gint) (angle / 45 + 0.5);

      if (cursor < GIMP_CURSOR_SIDE_TOP)
        {
          cursor += offset;

          if (cursor > GIMP_CURSOR_CORNER_TOP_LEFT)
            cursor -= 8;
        }
      else
        {
          cursor += offset;

          if (cursor > GIMP_CURSOR_SIDE_TOP_LEFT)
            cursor -= 8;
        }
   }

  return cursor;
}
