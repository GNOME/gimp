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

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimpbrush.h"
#include "gimpbrush-load.h"
#include "gimpbrush-scale.h"
#include "gimpbrushgenerated.h"
#include "gimpmarshal.h"

#include "gimp-intl.h"


enum
{
  SPACING_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SPACING
};


static void        gimp_brush_set_property          (GObject       *object,
                                                     guint          property_id,
                                                     const GValue  *value,
                                                     GParamSpec    *pspec);
static void        gimp_brush_get_property          (GObject       *object,
                                                     guint          property_id,
                                                     GValue        *value,
                                                     GParamSpec    *pspec);
static void        gimp_brush_finalize              (GObject       *object);

static gint64      gimp_brush_get_memsize           (GimpObject    *object,
                                                     gint64        *gui_size);

static gboolean    gimp_brush_get_size              (GimpViewable  *viewable,
                                                     gint          *width,
                                                     gint          *height);
static TempBuf   * gimp_brush_get_new_preview       (GimpViewable  *viewable,
                                                     GimpContext   *context,
                                                     gint           width,
                                                     gint           height);
static gchar     * gimp_brush_get_description       (GimpViewable  *viewable,
                                                     gchar        **tooltip);
static gchar     * gimp_brush_get_extension         (GimpData      *data);

static GimpBrush * gimp_brush_real_select_brush     (GimpBrush     *brush,
                                                     GimpCoords    *last_coords,
                                                     GimpCoords    *cur_coords);
static gboolean    gimp_brush_real_want_null_motion (GimpBrush     *brush,
                                                     GimpCoords    *last_coords,
                                                     GimpCoords    *cur_coords);


G_DEFINE_TYPE (GimpBrush, gimp_brush, GIMP_TYPE_DATA)

#define parent_class gimp_brush_parent_class

static guint brush_signals[LAST_SIGNAL] = { 0 };


static void
gimp_brush_class_init (GimpBrushClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpDataClass     *data_class        = GIMP_DATA_CLASS (klass);

  brush_signals[SPACING_CHANGED] =
    g_signal_new ("spacing-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpBrushClass, spacing_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->get_property       = gimp_brush_get_property;
  object_class->set_property       = gimp_brush_set_property;
  object_class->finalize           = gimp_brush_finalize;

  gimp_object_class->get_memsize   = gimp_brush_get_memsize;

  viewable_class->default_stock_id = "gimp-tool-paintbrush";
  viewable_class->get_size         = gimp_brush_get_size;
  viewable_class->get_new_preview  = gimp_brush_get_new_preview;
  viewable_class->get_description  = gimp_brush_get_description;

  data_class->get_extension        = gimp_brush_get_extension;

  klass->select_brush              = gimp_brush_real_select_brush;
  klass->want_null_motion          = gimp_brush_real_want_null_motion;
  klass->scale_size                = gimp_brush_real_scale_size;
  klass->scale_mask                = gimp_brush_real_scale_mask;
  klass->scale_pixmap              = gimp_brush_real_scale_pixmap;
  klass->spacing_changed           = NULL;

  g_object_class_install_property (object_class, PROP_SPACING,
                                   g_param_spec_double ("spacing", NULL, NULL,
                                                        1.0, 5000.0, 20.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_brush_init (GimpBrush *brush)
{
  brush->mask     = NULL;
  brush->pixmap   = NULL;

  brush->spacing  = 20;
  brush->x_axis.x = 15.0;
  brush->x_axis.y =  0.0;
  brush->y_axis.x =  0.0;
  brush->y_axis.y = 15.0;
}

static void
gimp_brush_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  GimpBrush *brush = GIMP_BRUSH (object);

  switch (property_id)
    {
    case PROP_SPACING:
      gimp_brush_set_spacing (brush, g_value_get_double (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_brush_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  GimpBrush *brush = GIMP_BRUSH (object);

  switch (property_id)
    {
    case PROP_SPACING:
      g_value_set_double (value, brush->spacing);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_brush_finalize (GObject *object)
{
  GimpBrush *brush = GIMP_BRUSH (object);

  if (brush->mask)
    {
      temp_buf_free (brush->mask);
      brush->mask = NULL;
    }

  if (brush->pixmap)
    {
      temp_buf_free (brush->pixmap);
      brush->pixmap = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_brush_get_memsize (GimpObject *object,
                        gint64     *gui_size)
{
  GimpBrush *brush   = GIMP_BRUSH (object);
  gint64     memsize = 0;

  if (brush->mask)
    memsize += temp_buf_get_memsize (brush->mask);

  if (brush->pixmap)
    memsize += temp_buf_get_memsize (brush->pixmap);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_brush_get_size (GimpViewable *viewable,
                     gint         *width,
                     gint         *height)
{
  GimpBrush *brush = GIMP_BRUSH (viewable);

  *width  = brush->mask->width;
  *height = brush->mask->height;

  return TRUE;
}

static TempBuf *
gimp_brush_get_new_preview (GimpViewable *viewable,
                            GimpContext  *context,
                            gint          width,
                            gint          height)
{
  GimpBrush *brush      = GIMP_BRUSH (viewable);
  TempBuf   *mask_buf   = NULL;
  TempBuf   *pixmap_buf = NULL;
  TempBuf   *return_buf = NULL;
  gint       mask_width;
  gint       mask_height;
  guchar     transp[4]  = { 0, 0, 0, 0 };
  guchar    *mask;
  guchar    *buf;
  gint       x, y;
  gboolean   scaled = FALSE;

  mask_buf   = brush->mask;
  pixmap_buf = brush->pixmap;

  mask_width  = mask_buf->width;
  mask_height = mask_buf->height;

  if (mask_width > width || mask_height > height)
    {
      gdouble ratio_x = (gdouble) width  / (gdouble) mask_width;
      gdouble ratio_y = (gdouble) height / (gdouble) mask_height;
      gdouble scale   = MIN (ratio_x, ratio_y);

      if (scale != 1.0)
        {
          mask_buf = gimp_brush_scale_mask (brush, scale);

          if (! mask_buf)
            mask_buf = temp_buf_new (1, 1, 1, 0, 0, transp);

          if (pixmap_buf)
            pixmap_buf = gimp_brush_scale_pixmap (brush, scale);

          mask_width  = mask_buf->width;
          mask_height = mask_buf->height;

          scaled = TRUE;
        }
    }

  return_buf = temp_buf_new (mask_width, mask_height, 4, 0, 0, transp);

  mask = temp_buf_data (mask_buf);
  buf  = temp_buf_data (return_buf);

  if (pixmap_buf)
    {
      guchar *pixmap = temp_buf_data (pixmap_buf);

      for (y = 0; y < mask_height; y++)
        {
          for (x = 0; x < mask_width ; x++)
            {
              *buf++ = *pixmap++;
              *buf++ = *pixmap++;
              *buf++ = *pixmap++;
              *buf++ = *mask++;
            }
        }
    }
  else
    {
      for (y = 0; y < mask_height; y++)
        {
          for (x = 0; x < mask_width ; x++)
            {
              *buf++ = 0;
              *buf++ = 0;
              *buf++ = 0;
              *buf++ = *mask++;
            }
        }
    }

  if (scaled)
    {
      temp_buf_free (mask_buf);

      if (pixmap_buf)
        temp_buf_free (pixmap_buf);
    }

  return return_buf;
}

static gchar *
gimp_brush_get_description (GimpViewable  *viewable,
                            gchar        **tooltip)
{
  GimpBrush *brush = GIMP_BRUSH (viewable);

  return g_strdup_printf ("%s (%d × %d)",
                          GIMP_OBJECT (brush)->name,
                          brush->mask->width,
                          brush->mask->height);
}

static gchar *
gimp_brush_get_extension (GimpData *data)
{
  return GIMP_BRUSH_FILE_EXTENSION;
}

static GimpBrush *
gimp_brush_real_select_brush (GimpBrush  *brush,
                              GimpCoords *last_coords,
                              GimpCoords *cur_coords)
{
  return brush;
}

static gboolean
gimp_brush_real_want_null_motion (GimpBrush  *brush,
                                  GimpCoords *last_coords,
                                  GimpCoords *cur_coords)
{
  return TRUE;
}


/*  public functions  */

GimpData *
gimp_brush_new (const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);

  return gimp_brush_generated_new (name,
                                   GIMP_BRUSH_GENERATED_CIRCLE,
                                   5.0, 2, 0.5, 1.0, 0.0);
}

GimpData *
gimp_brush_get_standard (void)
{
  static GimpData *standard_brush = NULL;

  if (! standard_brush)
    {
      standard_brush = gimp_brush_new ("Standard");

      standard_brush->dirty = FALSE;
      gimp_data_make_internal (standard_brush);

      /*  set ref_count to 2 --> never swap the standard brush  */
      g_object_ref (standard_brush);
    }

  return standard_brush;
}

GimpBrush *
gimp_brush_select_brush (GimpBrush  *brush,
                         GimpCoords *last_coords,
                         GimpCoords *cur_coords)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (last_coords != NULL, NULL);
  g_return_val_if_fail (cur_coords != NULL, NULL);

  return GIMP_BRUSH_GET_CLASS (brush)->select_brush (brush,
                                                     last_coords,
                                                     cur_coords);
}

gboolean
gimp_brush_want_null_motion (GimpBrush  *brush,
                             GimpCoords *last_coords,
                             GimpCoords *cur_coords)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), FALSE);
  g_return_val_if_fail (last_coords != NULL, FALSE);
  g_return_val_if_fail (cur_coords != NULL, FALSE);

  return GIMP_BRUSH_GET_CLASS (brush)->want_null_motion (brush,
                                                         last_coords,
                                                         cur_coords);
}

void
gimp_brush_scale_size (GimpBrush     *brush,
                       gdouble        scale,
                       gint          *width,
                       gint          *height)
{
  g_return_if_fail (GIMP_IS_BRUSH (brush));
  g_return_if_fail (scale > 0.0);
  g_return_if_fail (width != NULL);
  g_return_if_fail (height != NULL);

  if (scale == 1.0)
    {
      *width  = brush->mask->width;
      *height = brush->mask->height;

      return;
    }

  GIMP_BRUSH_GET_CLASS (brush)->scale_size (brush, scale, width, height);
}

TempBuf *
gimp_brush_scale_mask (GimpBrush *brush,
                       gdouble    scale)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (scale > 0.0, NULL);

  if (scale == 1.0)
    return temp_buf_copy (brush->mask, NULL);

  return GIMP_BRUSH_GET_CLASS (brush)->scale_mask (brush, scale);
}

TempBuf *
gimp_brush_scale_pixmap (GimpBrush *brush,
                         gdouble    scale)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (brush->pixmap != NULL, NULL);
  g_return_val_if_fail (scale > 0.0, NULL);

  if (scale == 1.0)
    return temp_buf_copy (brush->pixmap, NULL);

  return GIMP_BRUSH_GET_CLASS (brush)->scale_pixmap (brush, scale);
}

TempBuf *
gimp_brush_get_mask (const GimpBrush *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);

  return brush->mask;
}

TempBuf *
gimp_brush_get_pixmap (const GimpBrush *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);

  return brush->pixmap;
}

gint
gimp_brush_get_spacing (const GimpBrush *brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), 0);

  return brush->spacing;
}

void
gimp_brush_set_spacing (GimpBrush *brush,
                        gint       spacing)
{
  g_return_if_fail (GIMP_IS_BRUSH (brush));

  if (brush->spacing != spacing)
    {
      brush->spacing = spacing;

      gimp_brush_spacing_changed (brush);
      g_object_notify (G_OBJECT (brush), "spacing");
    }
}

void
gimp_brush_spacing_changed (GimpBrush *brush)
{
  g_return_if_fail (GIMP_IS_BRUSH (brush));

  g_signal_emit (brush, brush_signals[SPACING_CHANGED], 0);
}
