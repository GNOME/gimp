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

#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvas.h"


/*  local function prototypes  */

static void     gimp_canvas_class_init (GimpCanvasClass  *klass);
static void     gimp_canvas_init       (GimpCanvas       *gdisp);
static void     gimp_canvas_realize    (GtkWidget        *widget);
static void     gimp_canvas_unrealize  (GtkWidget        *widget);
static GdkGC  * gimp_canvas_gc_new     (GimpCanvas       *canvas,
                                        GimpCanvasStyle   style);


static GtkDrawingAreaClass *parent_class = NULL;

static const guchar stipples[GIMP_CANVAS_NUM_STIPPLES][8] =
{
  {
    0xF0,    /*  ####----  */
    0xE1,    /*  ###----#  */
    0xC3,    /*  ##----##  */
    0x87,    /*  #----###  */
    0x0F,    /*  ----####  */
    0x1E,    /*  ---####-  */
    0x3C,    /*  --####--  */
    0x78,    /*  -####---  */
  },
  {
    0xE1,    /*  ###----#  */
    0xC3,    /*  ##----##  */
    0x87,    /*  #----###  */
    0x0F,    /*  ----####  */
    0x1E,    /*  ---####-  */
    0x3C,    /*  --####--  */
    0x78,    /*  -####---  */
    0xF0,    /*  ####----  */
  },
  {
    0xC3,    /*  ##----##  */
    0x87,    /*  #----###  */
    0x0F,    /*  ----####  */
    0x1E,    /*  ---####-  */
    0x3C,    /*  --####--  */
    0x78,    /*  -####---  */
    0xF0,    /*  ####----  */
    0xE1,    /*  ###----#  */
  },
  {
    0x87,    /*  #----###  */
    0x0F,    /*  ----####  */
    0x1E,    /*  ---####-  */
    0x3C,    /*  --####--  */
    0x78,    /*  -####---  */
    0xF0,    /*  ####----  */
    0xE1,    /*  ###----#  */
    0xC3,    /*  ##----##  */
  },
  {
    0x0F,    /*  ----####  */
    0x1E,    /*  ---####-  */
    0x3C,    /*  --####--  */
    0x78,    /*  -####---  */
    0xF0,    /*  ####----  */
    0xE1,    /*  ###----#  */
    0xC3,    /*  ##----##  */
    0x87,    /*  #----###  */
  },
  {
    0x1E,    /*  ---####-  */
    0x3C,    /*  --####--  */
    0x78,    /*  -####---  */
    0xF0,    /*  ####----  */
    0xE1,    /*  ###----#  */
    0xC3,    /*  ##----##  */
    0x87,    /*  #----###  */
    0x0F,    /*  ----####  */
  },
  {
    0x3C,    /*  --####--  */
    0x78,    /*  -####---  */
    0xF0,    /*  ####----  */
    0xE1,    /*  ###----#  */
    0xC3,    /*  ##----##  */
    0x87,    /*  #----###  */
    0x0F,    /*  ----####  */
    0x1E,    /*  ---####-  */
  },
  {
    0x78,    /*  -####---  */
    0xF0,    /*  ####----  */
    0xE1,    /*  ###----#  */
    0xC3,    /*  ##----##  */
    0x87,    /*  #----###  */
    0x0F,    /*  ----####  */
    0x1E,    /*  ---####-  */
    0x3C,    /*  --####--  */
  },
};


GType
gimp_canvas_get_type (void)
{
  static GType canvas_type = 0;

  if (! canvas_type)
    {
      static const GTypeInfo canvas_info =
      {
        sizeof (GimpCanvasClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_canvas_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpCanvas),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_canvas_init,
      };

      canvas_type = g_type_register_static (GTK_TYPE_DRAWING_AREA,
                                            "GimpCanvas",
                                            &canvas_info, 0);
    }

  return canvas_type;
}

static void
gimp_canvas_class_init (GimpCanvasClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->realize   = gimp_canvas_realize;
  widget_class->unrealize = gimp_canvas_unrealize;
}

static void
gimp_canvas_init (GimpCanvas *canvas)
{
  gint i;

  for (i = 0; i < GIMP_CANVAS_NUM_STYLES; i++)
    canvas->gc[i] = NULL;

  for (i = 0; i < GIMP_CANVAS_NUM_STIPPLES; i++)
    canvas->stipple[i] = NULL;
}

static void
gimp_canvas_realize (GtkWidget *widget)
{
  GimpCanvas *canvas = GIMP_CANVAS (widget);

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  canvas->stipple[0] =
    gdk_bitmap_create_from_data (widget->window,
                                 (const gchar *) stipples[0], 8, 8);
}

static void
gimp_canvas_unrealize (GtkWidget *widget)
{
  GimpCanvas *canvas = GIMP_CANVAS (widget);
  gint        i;

  for (i = 0; i < GIMP_CANVAS_NUM_STYLES; i++)
    {
      if (canvas->gc[i])
        {
          g_object_unref (canvas->gc[i]);
          canvas->gc[i] = NULL;
        }
    }

  for (i = 0; i < GIMP_CANVAS_NUM_STIPPLES; i++)
    {
      if (canvas->stipple[i])
        {
          g_object_unref (canvas->stipple[i]);
          canvas->stipple[i] = NULL;
        }
    }

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static GdkGC *
gimp_canvas_gc_new (GimpCanvas      *canvas,
                    GimpCanvasStyle  style)
{
  GdkGC           *gc;
  GdkGCValues      values;
  GdkGCValuesMask  mask = 0;
  GdkColor         fg;
  GdkColor         bg;

  if (! GTK_WIDGET_REALIZED (canvas))
    return NULL;

  switch (style)
    {
    case GIMP_CANVAS_STYLE_BLACK:
    case GIMP_CANVAS_STYLE_WHITE:
      break;

    case GIMP_CANVAS_STYLE_RENDER:
      mask |= GDK_GC_EXPOSURES;
      values.graphics_exposures = TRUE;
      break;

    case GIMP_CANVAS_STYLE_XOR_DASHED:
      mask |= GDK_GC_LINE_STYLE;
      values.line_style = GDK_LINE_ON_OFF_DASH;
      /*  fallthrough  */

    case GIMP_CANVAS_STYLE_XOR:
      mask |= GDK_GC_FUNCTION;
      values.function = GDK_INVERT;
      break;

    case GIMP_CANVAS_STYLE_SELECTION_IN:
    case GIMP_CANVAS_STYLE_SELECTION_OUT:
    case GIMP_CANVAS_STYLE_LAYER_BOUNDARY:
      mask |= GDK_GC_CAP_STYLE;
      values.cap_style = GDK_CAP_BUTT;
      /*  fallthrough  */

    case GIMP_CANVAS_STYLE_GUIDE_NORMAL:
    case GIMP_CANVAS_STYLE_GUIDE_ACTIVE:
      mask |= GDK_GC_FILL | GDK_GC_STIPPLE;
      values.fill    = GDK_OPAQUE_STIPPLED;
      values.stipple = canvas->stipple[0];
      break;

    case GIMP_CANVAS_STYLE_CUSTOM:
    default:
      return NULL;
    }

  gc = gdk_gc_new_with_values (GTK_WIDGET (canvas)->window, &values, mask);

  switch (style)
    {
    default:
      return gc;

    case GIMP_CANVAS_STYLE_WHITE:
      fg.red   = 0xffff;
      fg.green = 0xffff;
      fg.blue  = 0xffff;

      bg.red   = 0x0;
      bg.green = 0x0;
      bg.blue  = 0x0;
      break;

    case GIMP_CANVAS_STYLE_BLACK:
    case GIMP_CANVAS_STYLE_SELECTION_IN:
      fg.red   = 0x0;
      fg.green = 0x0;
      fg.blue  = 0x0;

      bg.red   = 0xffff;
      bg.green = 0xffff;
      bg.blue  = 0xffff;
      break;

    case GIMP_CANVAS_STYLE_SELECTION_OUT:
      fg.red   = 0xffff;
      fg.green = 0xffff;
      fg.blue  = 0xffff;

      bg.red   = 0x7f7f;
      bg.green = 0x7f7f;
      bg.blue  = 0x7f7f;
      break;

    case GIMP_CANVAS_STYLE_LAYER_BOUNDARY:
      fg.red   = 0x0;
      fg.green = 0x0;
      fg.blue  = 0x0;

      bg.red   = 0xffff;
      bg.green = 0xffff;
      bg.blue  = 0x0;
      break;

    case GIMP_CANVAS_STYLE_GUIDE_NORMAL:
      fg.red   = 0x0;
      fg.green = 0x0;
      fg.blue  = 0x0;

      bg.red   = 0x0;
      bg.green = 0x7f7f;
      bg.blue  = 0xffff;
      break;

    case GIMP_CANVAS_STYLE_GUIDE_ACTIVE:
      fg.red   = 0x0;
      fg.green = 0x0;
      fg.blue  = 0x0;

      bg.red   = 0xffff;
      bg.green = 0x0;
      bg.blue  = 0x0;
      break;
    }

  gdk_gc_set_rgb_fg_color (gc, &fg);
  gdk_gc_set_rgb_bg_color (gc, &bg);

  return gc;
}

static inline gboolean
gimp_canvas_ensure_style (GimpCanvas      *canvas,
                          GimpCanvasStyle  style)
{
  return (canvas->gc[style] != NULL ||
          (canvas->gc[style] = gimp_canvas_gc_new (canvas, style)) != NULL);
}


/*  public functions  */

/**
 * gimp_canvas_new:
 *
 * Creates a new #GimpCanvas widget and sets it's widget name to
 * "gimp-canvas".
 *
 * The #GimpCanvas widget is a #GdkDrawingArea abstraction. It manages
 * a set of graphic contexts for drawing on a GIMP display. If you
 * draw using a #GimpCanvasStyle, #GimpCanvas makes sure that the
 * associated #GdkGC is created. All drawing on the canvas needs to
 * happen by means of the #GimpCanvas drawing functions. Besides from
 * not needing a #GdkGC pointer, the #GimpCanvas drawing functions
 * look and work like their #GdkDrawable counterparts. #GimpCanvas
 * gracefully handles attempts to draw on the unrealized widget.
 *
 * Return value: a new #GimpCanvas widget
 **/
GtkWidget *
gimp_canvas_new (void)
{
  return g_object_new (GIMP_TYPE_CANVAS,
                       "name", "gimp-canvas",
                       NULL);
}

void
gimp_canvas_draw_cursor (GimpCanvas *canvas,
                         gint        x,
                         gint        y)
{
  GtkWidget *widget = GTK_WIDGET (canvas);

  if (! (gimp_canvas_ensure_style (canvas, GIMP_CANVAS_STYLE_BLACK) &&
         gimp_canvas_ensure_style (canvas, GIMP_CANVAS_STYLE_WHITE)) )
    return;

  gdk_draw_line (widget->window, canvas->gc[GIMP_CANVAS_STYLE_WHITE],
                 x - 7, y - 1, x + 7, y - 1);
  gdk_draw_line (widget->window, canvas->gc[GIMP_CANVAS_STYLE_BLACK],
                 x - 7, y,     x + 7, y    );
  gdk_draw_line (widget->window, canvas->gc[GIMP_CANVAS_STYLE_WHITE],
                 x - 7, y + 1, x + 7, y + 1);
  gdk_draw_line (widget->window, canvas->gc[GIMP_CANVAS_STYLE_WHITE],
                 x - 1, y - 7, x - 1, y + 7);
  gdk_draw_line (widget->window, canvas->gc[GIMP_CANVAS_STYLE_BLACK],
                 x,     y - 7, x,     y + 7);
  gdk_draw_line (widget->window, canvas->gc[GIMP_CANVAS_STYLE_WHITE],
                 x + 1, y - 7, x + 1, y + 7);
}

void
gimp_canvas_draw_point (GimpCanvas      *canvas,
                        GimpCanvasStyle  style,
                        gint             x,
                        gint             y)
{
  if (! gimp_canvas_ensure_style (canvas, style))
    return;

  gdk_draw_point (GTK_WIDGET (canvas)->window, canvas->gc[style],
                  x, y);
}

void
gimp_canvas_draw_points (GimpCanvas      *canvas,
                         GimpCanvasStyle  style,
                         GdkPoint        *points,
                         gint             num_points)
{
  if (! gimp_canvas_ensure_style (canvas, style))
    return;

  gdk_draw_points (GTK_WIDGET (canvas)->window, canvas->gc[style],
                   points, num_points);
}

void
gimp_canvas_draw_line (GimpCanvas      *canvas,
                       GimpCanvasStyle  style,
                       gint             x1,
                       gint             y1,
                       gint             x2,
                       gint             y2)
{
  if (! gimp_canvas_ensure_style (canvas, style))
    return;

  gdk_draw_line (GTK_WIDGET (canvas)->window, canvas->gc[style],
                 x1, y1, x2, y2);
}

void
gimp_canvas_draw_lines (GimpCanvas      *canvas,
                        GimpCanvasStyle  style,
                        GdkPoint        *points,
                        gint             num_points)
{
  if (! gimp_canvas_ensure_style (canvas, style))
    return;

  gdk_draw_lines (GTK_WIDGET (canvas)->window, canvas->gc[style],
                  points, num_points);
}

void
gimp_canvas_draw_rectangle (GimpCanvas      *canvas,
                            GimpCanvasStyle  style,
                            gboolean         filled,
                            gint             x,
                            gint             y,
                            gint             width,
                            gint             height)
{
  if (! gimp_canvas_ensure_style (canvas, style))
    return;

  gdk_draw_rectangle (GTK_WIDGET (canvas)->window, canvas->gc[style],
                      filled, x, y, width, height);
}

void
gimp_canvas_draw_arc (GimpCanvas      *canvas,
                      GimpCanvasStyle  style,
                      gboolean         filled,
                      gint             x,
                      gint             y,
                      gint             width,
                      gint             height,
                      gint             angle1,
                      gint             angle2)
{
  if (! gimp_canvas_ensure_style (canvas, style))
    return;

  gdk_draw_arc (GTK_WIDGET (canvas)->window, canvas->gc[style],
                filled, x, y, width, height, angle1, angle2);
}

void
gimp_canvas_draw_polygon (GimpCanvas      *canvas,
                          GimpCanvasStyle  style,
                          gboolean         filled,
                          GdkPoint        *points,
                          gint             num_points)
{
  if (! gimp_canvas_ensure_style (canvas, style))
    return;

  gdk_draw_polygon (GTK_WIDGET (canvas)->window, canvas->gc[style],
                    filled, points, num_points);
}

void
gimp_canvas_draw_segments (GimpCanvas      *canvas,
                           GimpCanvasStyle  style,
                           GdkSegment      *segments,
                           gint             num_segments)
{
  if (! gimp_canvas_ensure_style (canvas, style))
    return;

  gdk_draw_segments (GTK_WIDGET (canvas)->window, canvas->gc[style],
                     segments, num_segments);
}

void
gimp_canvas_draw_rgb (GimpCanvas      *canvas,
                      GimpCanvasStyle  style,
                      gint             x,
                      gint             y,
                      gint             width,
                      gint             height,
                      guchar          *rgb_buf,
                      gint             rowstride,
                      gint             xdith,
                      gint             ydith)
{
  if (! gimp_canvas_ensure_style (canvas, style))
    return;

  gdk_draw_rgb_image_dithalign (GTK_WIDGET (canvas)->window, canvas->gc[style],
                                x, y, width, height,
                                GDK_RGB_DITHER_MAX,
                                rgb_buf, rowstride, xdith, ydith);
}

void
gimp_canvas_set_clip_rect (GimpCanvas      *canvas,
                           GimpCanvasStyle  style,
                           GdkRectangle    *rect)
{
  if (! canvas->gc[style])
    {
      if (! rect)
        return;

      canvas->gc[style] = gimp_canvas_gc_new (canvas, style);
    }

  gdk_gc_set_clip_rectangle (canvas->gc[style], rect);
}

void
gimp_canvas_set_clip_region (GimpCanvas      *canvas,
                             GimpCanvasStyle  style,
                             GdkRegion       *region)
{
  if (! canvas->gc[style])
    {
      if (! region)
        return;

      canvas->gc[style] = gimp_canvas_gc_new (canvas, style);
    }

  gdk_gc_set_clip_region (canvas->gc[style], region);
}

/**
 * gimp_canvas_set_stipple_index:
 * @canvas: a #GimpCanvas widget
 * @style:  the #GimpCanvasStyle to alter
 * @index:  the new stipple index
 *
 * Some styles of the #GimpCanvas do a stipple fill. #GimpCanvas has a
 * set of %GIMP_CANVAS_NUM_STIPPLES stipple bitmaps. This functions
 * allows you to change the bitmap being used. This can be used to
 * implement a marching ants effect. An older implementation used to
 * use this feature and so it is included since it might be useful in
 * the future. All stipple bitmaps but the default one are created on
 * the fly.
 */
void
gimp_canvas_set_stipple_index (GimpCanvas      *canvas,
                               GimpCanvasStyle  style,
                               guint            index)
{
  if (! gimp_canvas_ensure_style (canvas, style))
    return;

  index = index % GIMP_CANVAS_NUM_STIPPLES;

  if (! canvas->stipple[index])
    {
      canvas->stipple[index] =
        gdk_bitmap_create_from_data (GTK_WIDGET (canvas)->window,
                                     (const gchar *) stipples[index], 8, 8);
    }

  gdk_gc_set_stipple (canvas->gc[style], canvas->stipple[index]);
}

/**
 * gimp_canvas_set_custom_gc:
 * @canvas: a #GimpCanvas widget
 * @gc:     a #GdkGC;
 *
 * The #GimpCanvas widget has an extra style for a custom #GdkGC. This
 * function allows you to set the @gc for the %GIMP_CANVAS_STYLE_CUSTOM.
 * Drawing with the custom style only works if you set a #GdkGC
 * earlier.  Since the custom #GdkGC can under certain circumstances
 * be destroyed by #GimpCanvas, you should always set the custom gc
 * before calling a #GimpCanvas drawing function with
 * %GIMP_CANVAS_STYLE_CUSTOM.
 **/
void
gimp_canvas_set_custom_gc (GimpCanvas *canvas,
                           GdkGC      *gc)
{
  if (canvas->gc[GIMP_CANVAS_STYLE_CUSTOM])
    g_object_unref (canvas->gc[GIMP_CANVAS_STYLE_CUSTOM]);

  canvas->gc[GIMP_CANVAS_STYLE_CUSTOM] = gc;

  if (gc)
    g_object_ref (gc);
}

void
gimp_canvas_set_bg_color (GimpCanvas *canvas,
                          GimpRGB    *color)
{
  GtkWidget   *widget = GTK_WIDGET (canvas);
  GdkColormap *colormap;
  GdkColor     gdk_color;

  if (! GTK_WIDGET_REALIZED (widget))
    return;

  gimp_rgb_get_gdk_color (color, &gdk_color);

  colormap = gdk_drawable_get_colormap (widget->window);
  g_return_if_fail (colormap != NULL);
  gdk_colormap_alloc_color (colormap, &gdk_color, FALSE, TRUE);

  gdk_window_set_background (widget->window, &gdk_color);
}
