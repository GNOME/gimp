/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimppreviewarea.h"


#define CHECK_COLOR(row,col) \
  ((((row) & GIMP_CHECK_SIZE) ^ ((col) & GIMP_CHECK_SIZE)) ? \
   (GIMP_CHECK_LIGHT * 255) :                                \
   (GIMP_CHECK_DARK  * 255))


static void      gimp_preview_area_class_init    (GimpPreviewAreaClass *klass);
static void      gimp_preview_area_init          (GimpPreviewArea *area);

static void      gimp_preview_area_finalize      (GObject         *object);

static void      gimp_preview_area_size_allocate (GtkWidget       *widget,
                                                  GtkAllocation   *allocation);
static gboolean  gimp_preview_area_expose        (GtkWidget       *widget,
                                                  GdkEventExpose  *event);


static GtkDrawingAreaClass *parent_class = NULL;


GType
gimp_preview_area_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpPreviewAreaClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_preview_area_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpPreviewArea),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_preview_area_init,
      };

      view_type = g_type_register_static (GTK_TYPE_DRAWING_AREA,
                                          "GimpPreviewArea",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_preview_area_class_init (GimpPreviewAreaClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize      = gimp_preview_area_finalize;

  widget_class->size_allocate = gimp_preview_area_size_allocate;
  widget_class->expose_event  = gimp_preview_area_expose;
}

static void
gimp_preview_area_init (GimpPreviewArea *area)
{
  area->buf       = NULL;
  area->cmap      = NULL;
  area->dither_x  = 0;
  area->dither_y  = 0;
  area->width     = 0;
  area->height    = 0;
  area->rowstride = 0;
}

static void
gimp_preview_area_finalize (GObject *object)
{
  GimpPreviewArea *area = GIMP_PREVIEW_AREA (object);

  if (area->buf)
    {
      g_free (area->buf);
      area->buf = NULL;
    }
  if (area->cmap)
    {
      g_free (area->cmap);
      area->cmap = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_preview_area_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  GimpPreviewArea *area = GIMP_PREVIEW_AREA (widget);

  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (widget->allocation.width  != area->width ||
      widget->allocation.height != area->height)
    {
      if (area->buf)
        {
          g_free (area->buf);

          area->buf       = NULL;
          area->rowstride = 0;
        }

      area->width  = widget->allocation.width;
      area->height = widget->allocation.height;
    }
}

static gboolean
gimp_preview_area_expose (GtkWidget      *widget,
                          GdkEventExpose *event)
{
  GimpPreviewArea *area;
  guchar          *buf;

  g_return_val_if_fail (GIMP_IS_PREVIEW_AREA (widget), FALSE);

  area = GIMP_PREVIEW_AREA (widget);

  if (! area->buf)
    return FALSE;

  buf = area->buf + event->area.x * 3 + event->area.y * area->rowstride;

  gdk_draw_rgb_image_dithalign (widget->window,
                                widget->style->fg_gc[widget->state],
                                event->area.x,
                                event->area.y,
                                event->area.width,
                                event->area.height,
                                GDK_RGB_DITHER_MAX,
                                buf,
                                area->rowstride,
                                area->dither_x - event->area.x,
                                area->dither_y - event->area.y);

  return FALSE;
}


/**
 * gimp_preview_area_new:
 *
 * Return value: a new #GimpPreviewArea widget.
 *
 * Since GIMP 2.2
 **/
GtkWidget *
gimp_preview_area_new (void)
{
  return g_object_new (GIMP_TYPE_PREVIEW_AREA, NULL);
}

/**
 * gimp_preview_area_draw:
 * @area:      a #GimpPreviewArea widget.
 * @x:         x offset in preview
 * @y:         y offset in preview
 * @width:     buffer width
 * @height:    buffer height
 * @type:      the #GimpImageType of @buf
 * @buf:       a #guchar buffer that contains the preview pixel data.
 * @rowstride: rowstride of @buf
 *
 * Draws @buf on @area and queues a redraw on the rectangle that
 * changed.
 *
 * Since GIMP 2.2
 **/
void
gimp_preview_area_draw (GimpPreviewArea *area,
                        gint             x,
                        gint             y,
                        gint             width,
                        gint             height,
                        GimpImageType    type,
                        const guchar    *buf,
                        gint             rowstride)
{
  const guchar    *src;
  guchar          *dest;
  gint             row;
  gint             col;

  g_return_if_fail (GIMP_IS_PREVIEW_AREA (area));
  g_return_if_fail (width > 0 && height > 0);
  g_return_if_fail (buf != NULL);
  g_return_if_fail (rowstride > 0);

  if (x + width < 0 || x >= area->width)
    return;

  if (y + height < 0 || y >= area->height)
    return;

  if (x < 0)
    {
      gint  bpp;

      switch (type)
        {
        case GIMP_GRAY_IMAGE:
        case GIMP_INDEXED_IMAGE:
          bpp = 1;
          break;
        case GIMP_GRAYA_IMAGE:
        case GIMP_INDEXEDA_IMAGE:
          bpp = 2;
          break;
        case GIMP_RGB_IMAGE:
          bpp = 3;
          break;
        case GIMP_RGBA_IMAGE:
          bpp = 4;
          break;
        default:
          g_return_if_reached ();
          break;
        }

      buf += x * bpp;
      width -= x;
      x = 0;
    }

  if (x + width > area->width)
    width = area->width - x;

  if (y < 0)
    {
      buf += y * rowstride;
      height -= y;
      y = 0;
    }

  if (y + height > area->height)
    height = area->height - y;

  if (! area->buf)
    {
      area->rowstride = ((area->width * 3) + 3) & ~3;
      area->buf = g_new (guchar, area->rowstride * area->height);
    }

  src  = buf;
  dest = area->buf + x * 3 + y * area->rowstride;

  switch (type)
    {
    case GIMP_RGB_IMAGE:
      for (row = 0; row < height; row++)
        {
          memcpy (dest, src, 3 * width);

          src  += rowstride;
          dest += area->rowstride;
        }
      break;

    case GIMP_RGBA_IMAGE:
       for (row = y; row < y + height; row++)
        {
          const guchar *s = src;
          guchar       *d = dest;

          for (col = x; col < x + width; col++, s += 4, d+= 3)
            {
              switch (s[3])
                {
                case 0:
                  d[0] = d[1] = d[2] = CHECK_COLOR (row, col);
                  break;

                case 255:
                  d[0] = s[0];
                  d[1] = s[1];
                  d[2] = s[2];
                  break;

                default:
                  {
                    register guint alpha = s[3] + 1;
                    register guint check = CHECK_COLOR (row, col);

                    d[0] = ((check << 8) + (s[0] - check) * alpha) >> 8;
                    d[1] = ((check << 8) + (s[1] - check) * alpha) >> 8;
                    d[2] = ((check << 8) + (s[2] - check) * alpha) >> 8;
                  }
                  break;
                }
            }

          src  += rowstride;
          dest += area->rowstride;
        }
       break;

    case GIMP_GRAY_IMAGE:
      for (row = 0; row < height; row++)
        {
          const guchar *s = src;
          guchar       *d = dest;

          for (col = 0; col < width; col++, s++, d += 3)
            {
              d[0] = d[1] = d[2] = s[0];
            }

          src  += rowstride;
          dest += area->rowstride;
        }
      break;

    case GIMP_GRAYA_IMAGE:
      for (row = y; row < y + height; row++)
        {
          const guchar *s = src;
          guchar       *d = dest;

          for (col = x; col < x + width; col++, s += 2, d+= 3)
            {
              switch (s[1])
                {
                case 0:
                  d[0] = d[1] = d[2] = CHECK_COLOR (row, col);
                  break;

                case 255:
                  d[0] = d[1] = d[2] = s[0];
                  break;

                default:
                  {
                    register guint alpha = s[1] + 1;
                    register guint check = CHECK_COLOR (row, col);

                    d[0] = d[1] = d[2] =
                      ((check << 8) + (s[0] - check) * alpha) >> 8;
                  }
                  break;
                }
            }

          src  += rowstride;
          dest += area->rowstride;
        }
      break;

    case GIMP_INDEXED_IMAGE:
      g_return_if_fail (area->cmap != NULL);
      for (row = 0; row < height; row++)
        {
          const guchar *s = src;
          guchar       *d = dest;

          for (col = 0; col < width; col++, s++, d += 3)
            {
              const guchar *cmap = area->cmap + 3 * s[0];

              d[0] = cmap[0];
              d[1] = cmap[1];
              d[2] = cmap[2];
            }

          src  += rowstride;
          dest += area->rowstride;
        }
      break;

    case GIMP_INDEXEDA_IMAGE:
      g_return_if_fail (area->cmap != NULL);
      for (row = y; row < y + height; row++)
        {
          const guchar *s = src;
          guchar       *d = dest;

          for (col = x; col < x + width; col++, s += 2, d += 3)
            {
              const guchar *cmap  = area->cmap + 3 * s[0];

              switch (s[1])
                {
                case 0:
                  d[0] = d[1] = d[2] = CHECK_COLOR (row, col);
                  break;

                case 255:
                  d[0] = cmap[0];
                  d[1] = cmap[1];
                  d[2] = cmap[2];
                  break;

                default:
                  {
                    register guint alpha = s[3] + 1;
                    register guint check = CHECK_COLOR (row, col);

                    d[0] = ((check << 8) + (cmap[0] - check) * alpha) >> 8;
                    d[1] = ((check << 8) + (cmap[1] - check) * alpha) >> 8;
                    d[2] = ((check << 8) + (cmap[2] - check) * alpha) >> 8;
                  }
                  break;
                }
            }

          src  += rowstride;
          dest += area->rowstride;
        }
      break;
    }

  gtk_widget_queue_draw_area (GTK_WIDGET (area), x, y, width, height);
}

/**
 * gimp_preview_area_fill:
 * @area:   a #GimpPreviewArea widget.
 * @x:      x offset in preview
 * @y:      y offset in preview
 * @width:  buffer width
 * @height: buffer height
 * @red:
 * @green:
 * @blue:
 *
 * Fills the @area in the given color.
 *
 * Since GIMP 2.2
 **/
void
gimp_preview_area_fill (GimpPreviewArea *area,
                        gint             x,
                        gint             y,
                        gint             width,
                        gint             height,
                        guchar           red,
                        guchar           green,
                        guchar           blue)
{
  guchar *dest;
  gint    row;
  gint    col;

  g_return_if_fail (GIMP_IS_PREVIEW_AREA (area));
  g_return_if_fail (width > 0 && height > 0);

  if (x + width < 0 || x >= area->width)
    return;

  if (y + height < 0 || y >= area->height)
    return;

  if (x < 0)
    {
      width -= x;
      x = 0;
    }

  if (x + width > area->width)
    width = area->width - x;

  if (y < 0)
    {
      height -= y;
      y = 0;
    }

  if (y + height > area->height)
    height = area->height - y;

  if (! area->buf)
    {
      area->rowstride = ((area->width * 3) + 3) & ~3;
      area->buf = g_new (guchar, area->rowstride * area->height);
    }

  dest = area->buf + x * 3 + y * area->rowstride;

  for (row = 0; row < height; row++)
    {
      guchar *d = dest;

      for (col = 0; col < width; col++, d+= 3)
        {
          d[0] = red;
          d[1] = green;
          d[2] = blue;
        }

      dest += area->rowstride;
    }

  gtk_widget_queue_draw_area (GTK_WIDGET (area), x, y, width, height);
}


/**
 * gimp_preview_area_set_cmap:
 * @area:       a #GimpPreviewArea
 * @cmap:       a #guchar buffer that contains the colormap
 * @num_colors: the number of colors in the colormap
 *
 * Sets the colormap for the #GimpPreviewArea widget. You need to
 * call this function before you use gimp_preview_area_draw() with
 * an image type of %GIMP_INDEXED_IMAGE or %GIMP_INDEXEDA_IMAGE.
 *
 * Since GIMP 2.2
 **/
void
gimp_preview_area_set_cmap (GimpPreviewArea *area,
                            const guchar    *cmap,
                            gint             num_colors)
{
  g_return_if_fail (GIMP_IS_PREVIEW_AREA (area));
  g_return_if_fail (cmap != NULL || num_colors == 0);
  g_return_if_fail (num_colors >= 0 && num_colors <= 256);

  if (num_colors > 0)
    {
      if (! area->cmap)
        area->cmap = g_new0 (guchar, 3 * 256);

      memcpy (area->cmap, cmap, 3 * num_colors);
    }
  else
    {
      g_free (area->cmap);
      area->cmap = NULL;
    }
}
