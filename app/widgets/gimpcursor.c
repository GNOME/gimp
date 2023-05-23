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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_QUARTZ
#include <gdk/gdkquartz.h>
#endif
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#include "widgets-types.h"

#include "gimpcursor.h"


#define cursor_default_hot_x 10
#define cursor_default_hot_y 10

#define cursor_mouse_hot_x 3
#define cursor_mouse_hot_y 2
#define cursor_crosshair_hot_x 15
#define cursor_crosshair_hot_y 15
#define cursor_zoom_hot_x 8
#define cursor_zoom_hot_y 8
#define cursor_color_picker_hot_x 1
#define cursor_color_picker_hot_y 30


typedef struct _GimpCursor GimpCursor;

struct _GimpCursor
{
  const gchar *resource_name;
  const gint   hot_x;
  const gint   hot_y;

  GdkPixbuf   *pixbuf;
  GdkPixbuf   *pixbuf_x2;
};


static GimpCursor gimp_cursors[] =
{
  /* these have to match up with enum GimpCursorType in widgets-enums.h */

  {
    "cursor-none",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-mouse",
    cursor_mouse_hot_x, cursor_mouse_hot_y
  },
  {
    "cursor-crosshair",
    cursor_crosshair_hot_x, cursor_crosshair_hot_y
  },
  {
    "cursor-crosshair-small",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-bad",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-move",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-zoom",
    cursor_zoom_hot_x, cursor_zoom_hot_y
  },
  {
    "cursor-color-picker",
    cursor_color_picker_hot_x, cursor_color_picker_hot_y
  },
  {
    "cursor-corner-top",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-corner-top-right",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-corner-right",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-corner-bottom-right",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-corner-bottom",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-corner-bottom-left",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-corner-left",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-corner-top-left",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-side-top",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-side-top-right",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-side-right",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-side-bottom-right",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-side-bottom",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-side-bottom-left",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-side-left",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-side-top-left",
    cursor_default_hot_x, cursor_default_hot_y
  },
  {
    "cursor-single-dot",
    cursor_default_hot_x, cursor_default_hot_y
  }
};

static GimpCursor gimp_tool_cursors[] =
{
  /* these have to match up with enum GimpToolCursorType in widgets-enums.h */

  { NULL },
  { "tool-rect-select" },
  { "tool-ellipse-select" },
  { "tool-free-select" },
  { "tool-polygon-select" },
  { "tool-fuzzy-select" },
  { "tool-paths" },
  { "tool-paths-anchor" },
  { "tool-paths-control" },
  { "tool-paths-segment" },
  { "tool-iscissors" },
  { "tool-move" },
  { "tool-zoom" },
  { "tool-crop" },
  { "tool-resize" },
  { "tool-rotate" },
  { "tool-shear" },
  { "tool-perspective" },
  { "tool-transform-3d-camera" },
  { "tool-flip-horizontal" },
  { "tool-flip-vertical" },
  { "tool-text" },
  { "tool-color-picker" },
  { "tool-bucket-fill" },
  { "tool-gradient" },
  { "tool-pencil" },
  { "tool-paintbrush" },
  { "tool-airbrush" },
  { "tool-ink" },
  { "tool-clone" },
  { "tool-heal" },
  { "tool-eraser" },
  { "tool-smudge" },
  { "tool-blur" },
  { "tool-dodge" },
  { "tool-burn" },
  { "tool-measure" },
  { "tool-warp" },
  { "tool-hand" }
};

static GimpCursor gimp_cursor_modifiers[] =
{
  /* these have to match up with enum GimpCursorModifier in widgets-enums.h */

  { NULL },
  { "modifier-bad" },
  { "modifier-plus" },
  { "modifier-minus" },
  { "modifier-intersect" },
  { "modifier-move" },
  { "modifier-resize" },
  { "modifier-rotate" },
  { "modifier-zoom" },
  { "modifier-control" },
  { "modifier-anchor" },
  { "modifier-foreground" },
  { "modifier-background" },
  { "modifier-pattern" },
  { "modifier-join" },
  { "modifier-select" }
};


static const GdkPixbuf *
get_cursor_pixbuf (GimpCursor *cursor,
                   gint        scale_factor)
{
  gchar  *resource_path;
  GError *error = NULL;

  if (! cursor->pixbuf)
    {
      resource_path = g_strconcat ("/org/gimp/tool-cursors/",
                                   cursor->resource_name,
                                   ".png", NULL);

      cursor->pixbuf = gdk_pixbuf_new_from_resource (resource_path, &error);

      if (! cursor->pixbuf)
        {
          g_critical ("Failed to create cursor image '%s': %s",
                      resource_path, error->message);
          g_clear_error (&error);
        }

      g_free (resource_path);
    }

  if (scale_factor == 2 && ! cursor->pixbuf_x2)
    {
      resource_path = g_strconcat ("/org/gimp/tool-cursors/",
                                   cursor->resource_name,
                                   "-x2.png", NULL);

      cursor->pixbuf_x2 = gdk_pixbuf_new_from_resource (resource_path, &error);

      if (! cursor->pixbuf_x2)
        {
          /* no critical here until we actually have the cursor files */
          g_printerr ("Failed to create scaled cursor image '%s' "
                      "falling back to upscaling default cursor: %s\n",
                      resource_path, error->message);
          g_clear_error (&error);

          if (cursor->pixbuf)
            {
              gint width  = gdk_pixbuf_get_width  (cursor->pixbuf);
              gint height = gdk_pixbuf_get_height (cursor->pixbuf);

              cursor->pixbuf_x2 = gdk_pixbuf_scale_simple (cursor->pixbuf,
                                                           width  * 2,
                                                           height * 2,
                                                           GDK_INTERP_NEAREST);
            }
        }

      g_free (resource_path);
    }

  if (scale_factor == 2)
    return cursor->pixbuf_x2;
  else
    return cursor->pixbuf;
}

GdkCursor *
gimp_cursor_new (GdkWindow          *window,
                 GimpHandedness      cursor_handedness,
                 GimpCursorType      cursor_type,
                 GimpToolCursorType  tool_cursor,
                 GimpCursorModifier  modifier)
{
  GdkDisplay *display;
  GimpCursor *bmcursor   = NULL;
  GimpCursor *bmmodifier = NULL;
  GimpCursor *bmtool     = NULL;
  GdkCursor  *cursor;
  GdkPixbuf  *pixbuf;
  gint        scale_factor;
  gint        hot_x;
  gint        hot_y;

  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);
  g_return_val_if_fail (cursor_type < GIMP_CURSOR_LAST, NULL);

  display = gdk_window_get_display (window);

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

  scale_factor = gdk_window_get_scale_factor (window);

  /* we only support x2 scaling right now */
  scale_factor = CLAMP (scale_factor, 1, 2);

  pixbuf = gdk_pixbuf_copy (get_cursor_pixbuf (bmcursor, scale_factor));

  if (bmmodifier || bmtool)
    {
      gint width  = gdk_pixbuf_get_width  (pixbuf);
      gint height = gdk_pixbuf_get_height (pixbuf);

      if (bmmodifier)
        gdk_pixbuf_composite (get_cursor_pixbuf (bmmodifier, scale_factor),
                              pixbuf,
                              0, 0, width, height,
                              0.0, 0.0, 1.0, 1.0,
                              GDK_INTERP_NEAREST, 200);

      if (bmtool)
        gdk_pixbuf_composite (get_cursor_pixbuf (bmtool, scale_factor),
                              pixbuf,
                              0, 0, width, height,
                              0.0, 0.0, 1.0, 1.0,
                              GDK_INTERP_NEAREST, 200);
    }

  hot_x = bmcursor->hot_x;
  hot_y = bmcursor->hot_y;

  /*  flip the cursor if mouse setting is left-handed  */

  if (cursor_handedness == GIMP_HANDEDNESS_LEFT)
    {
      GdkPixbuf *flipped = gdk_pixbuf_flip (pixbuf, TRUE);
      gint       width   = gdk_pixbuf_get_width (flipped);

      g_object_unref (pixbuf);
      pixbuf = flipped;

      hot_x = (width - 1) - hot_x;
    }

  if (scale_factor > 1)
    {
      gint hot_x_scaled = hot_x;
      gint hot_y_scaled = hot_y;

      cairo_surface_t *surface =
        gdk_cairo_surface_create_from_pixbuf (pixbuf, scale_factor, NULL);

      /*
       * MacOS needs the hotspot in surface coordinates
       * X11 needs the hotspot in pixel coordinates (not scaled)
       * Windows doesn't handle scaled cursors at all
       * Broadway does not appear to support surface cursors at all,
       * let alone scaled surface cursors.
       * https://gitlab.gnome.org/GNOME/gimp/-/merge_requests/545#note_1388777
       */
      if (FALSE                            ||
#ifdef GDK_WINDOWING_QUARTZ
          GDK_IS_QUARTZ_DISPLAY (display)  ||
#endif
          FALSE)
        {
          hot_x_scaled *= scale_factor;
          hot_y_scaled *= scale_factor;
        }

      cursor = gdk_cursor_new_from_surface (display,
                                            surface,
                                            hot_x_scaled,
                                            hot_y_scaled);
      cairo_surface_destroy (surface);
    }
  else
    {
      cursor = gdk_cursor_new_from_pixbuf (display, pixbuf, hot_x, hot_y);
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
  GdkWindow *window;
  GdkCursor *cursor;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (gtk_widget_get_realized (widget));

  window = gtk_widget_get_window (widget);

  cursor = gimp_cursor_new (window,
                            cursor_handedness,
                            cursor_type,
                            tool_cursor,
                            modifier);
  gdk_window_set_cursor (window, cursor);
  g_object_unref (cursor);

  gdk_display_flush (gdk_window_get_display (window));
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
