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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpbezierdesc.h"
#include "gimpbrush.h"
#include "gimpbrush-boundary.h"
#include "gimpbrush-load.h"
#include "gimpbrush-mipmap.h"
#include "gimpbrush-private.h"
#include "gimpbrush-save.h"
#include "gimpbrush-transform.h"
#include "gimpbrushcache.h"
#include "gimpbrushgenerated.h"
#include "gimpbrushpipe.h"
#include "gimpcontext.h"
#include "gimptagged.h"
#include "gimptempbuf.h"

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


static void          gimp_brush_tagged_iface_init     (GimpTaggedInterface  *iface);

static void          gimp_brush_finalize              (GObject              *object);
static void          gimp_brush_set_property          (GObject              *object,
                                                       guint                 property_id,
                                                       const GValue         *value,
                                                       GParamSpec           *pspec);
static void          gimp_brush_get_property          (GObject              *object,
                                                       guint                 property_id,
                                                       GValue               *value,
                                                       GParamSpec           *pspec);

static gint64        gimp_brush_get_memsize           (GimpObject           *object,
                                                       gint64               *gui_size);

static gboolean      gimp_brush_get_size              (GimpViewable         *viewable,
                                                       gint                 *width,
                                                       gint                 *height);
static GimpTempBuf * gimp_brush_get_new_preview       (GimpViewable         *viewable,
                                                       GimpContext          *context,
                                                       gint                  width,
                                                       gint                  height,
                                                       GeglColor            *fg_color);
static gchar       * gimp_brush_get_description       (GimpViewable         *viewable,
                                                       gchar               **tooltip);

static void          gimp_brush_dirty                 (GimpData             *data);
static const gchar * gimp_brush_get_extension         (GimpData             *data);
static void          gimp_brush_copy                  (GimpData             *data,
                                                       GimpData             *src_data);

static void          gimp_brush_real_begin_use        (GimpBrush            *brush);
static void          gimp_brush_real_end_use          (GimpBrush            *brush);
static GimpBrush   * gimp_brush_real_select_brush     (GimpBrush            *brush,
                                                       const GimpCoords     *last_coords,
                                                       const GimpCoords     *current_coords);
static gboolean      gimp_brush_real_want_null_motion (GimpBrush            *brush,
                                                       const GimpCoords     *last_coords,
                                                       const GimpCoords     *current_coords);

static gchar       * gimp_brush_get_checksum          (GimpTagged           *tagged);


G_DEFINE_TYPE_WITH_CODE (GimpBrush, gimp_brush, GIMP_TYPE_DATA,
                         G_ADD_PRIVATE (GimpBrush)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_TAGGED,
                                                gimp_brush_tagged_iface_init))

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
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize            = gimp_brush_finalize;
  object_class->get_property        = gimp_brush_get_property;
  object_class->set_property        = gimp_brush_set_property;

  gimp_object_class->get_memsize    = gimp_brush_get_memsize;

  viewable_class->default_icon_name = "gimp-tool-paintbrush";
  viewable_class->default_name      = _("Brush");
  viewable_class->get_size          = gimp_brush_get_size;
  viewable_class->get_new_preview   = gimp_brush_get_new_preview;
  viewable_class->get_description   = gimp_brush_get_description;

  data_class->dirty                 = gimp_brush_dirty;
  data_class->save                  = gimp_brush_save;
  data_class->get_extension         = gimp_brush_get_extension;
  data_class->copy                  = gimp_brush_copy;

  klass->begin_use                  = gimp_brush_real_begin_use;
  klass->end_use                    = gimp_brush_real_end_use;
  klass->select_brush               = gimp_brush_real_select_brush;
  klass->want_null_motion           = gimp_brush_real_want_null_motion;
  klass->transform_size             = gimp_brush_real_transform_size;
  klass->transform_mask             = gimp_brush_real_transform_mask;
  klass->transform_pixmap           = gimp_brush_real_transform_pixmap;
  klass->transform_boundary         = gimp_brush_real_transform_boundary;
  klass->spacing_changed            = NULL;

  g_object_class_install_property (object_class, PROP_SPACING,
                                   g_param_spec_double ("spacing", NULL,
                                                        _("Brush Spacing"),
                                                        1.0, 5000.0, 20.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_brush_tagged_iface_init (GimpTaggedInterface *iface)
{
  iface->get_checksum = gimp_brush_get_checksum;
}

static void
gimp_brush_init (GimpBrush *brush)
{
  brush->priv = gimp_brush_get_instance_private (brush);

  brush->priv->spacing  = 20;
  brush->priv->x_axis.x = 15.0;
  brush->priv->x_axis.y =  0.0;
  brush->priv->y_axis.x =  0.0;
  brush->priv->y_axis.y = 15.0;

  brush->priv->blur_hardness = 1.0;
}

static void
gimp_brush_finalize (GObject *object)
{
  GimpBrush *brush = GIMP_BRUSH (object);

  g_clear_pointer (&brush->priv->mask,          gimp_temp_buf_unref);
  g_clear_pointer (&brush->priv->pixmap,        gimp_temp_buf_unref);
  g_clear_pointer (&brush->priv->blurred_mask,   gimp_temp_buf_unref);
  g_clear_pointer (&brush->priv->blurred_pixmap, gimp_temp_buf_unref);

  gimp_brush_mipmap_clear (brush);

  g_clear_object (&brush->priv->mask_cache);
  g_clear_object (&brush->priv->pixmap_cache);
  g_clear_object (&brush->priv->boundary_cache);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
      g_value_set_double (value, gimp_brush_get_spacing (brush));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_brush_get_memsize (GimpObject *object,
                        gint64     *gui_size)
{
  GimpBrush *brush   = GIMP_BRUSH (object);
  gint64     memsize = 0;

  memsize += gimp_temp_buf_get_memsize (brush->priv->mask);
  memsize += gimp_temp_buf_get_memsize (brush->priv->pixmap);

  memsize += gimp_brush_mipmap_get_memsize (brush);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_brush_get_size (GimpViewable *viewable,
                     gint         *width,
                     gint         *height)
{
  GimpBrush *brush = GIMP_BRUSH (viewable);

  *width  = gimp_temp_buf_get_width  (brush->priv->mask);
  *height = gimp_temp_buf_get_height (brush->priv->mask);

  return TRUE;
}

static GimpTempBuf *
gimp_brush_get_new_preview (GimpViewable *viewable,
                            GimpContext  *context,
                            gint          width,
                            gint          height,
                            GeglColor    *fg_color)
{
  GimpBrush         *brush       = GIMP_BRUSH (viewable);
  const GimpTempBuf *mask_buf    = brush->priv->mask;
  const GimpTempBuf *pixmap_buf  = brush->priv->pixmap;
  GimpTempBuf       *return_buf  = NULL;
  gint               mask_width;
  gint               mask_height;
  guchar            *mask_data;
  guchar            *mask;
  guchar            *buf;
  gint               x, y;
  gboolean           free_mask = FALSE;
  gdouble            scale     = 1.0;

  mask_width  = gimp_temp_buf_get_width  (mask_buf);
  mask_height = gimp_temp_buf_get_height (mask_buf);

  if (mask_width > width || mask_height > height)
    {
      gdouble ratio_x = (gdouble) width  / (gdouble) mask_width;
      gdouble ratio_y = (gdouble) height / (gdouble) mask_height;

      scale = MIN (ratio_x, ratio_y);
    }

  if (GIMP_IS_BRUSH_GENERATED (brush) || scale != 1.0)
    {
      gimp_brush_begin_use (brush);

      if (GIMP_IS_BRUSH_GENERATED (brush))
        {
          GimpBrushGenerated *gen_brush = GIMP_BRUSH_GENERATED (brush);

          mask_buf = gimp_brush_transform_mask (brush, scale,
                                                (gimp_brush_generated_get_aspect_ratio (gen_brush) - 1.0) * 20.0 / 19.0,
                                                gimp_brush_generated_get_angle (gen_brush) / -360.0,
                                                FALSE,
                                                gimp_brush_generated_get_hardness (gen_brush));
        }
      else
        {
          mask_buf = gimp_brush_transform_mask (brush, scale, 0.0, 0.0, FALSE, 1.0);
        }

      if (! mask_buf)
        {
          mask_buf = gimp_temp_buf_new (1, 1, babl_format ("Y u8"));
          gimp_temp_buf_data_clear ((GimpTempBuf *) mask_buf);
        }
      else
        {
          gimp_temp_buf_ref ((GimpTempBuf *) mask_buf);
        }

      if (pixmap_buf)
        pixmap_buf = gimp_brush_transform_pixmap (brush, scale,
                                                  0.0, 0.0, FALSE, 1.0);

      mask_width  = gimp_temp_buf_get_width  (mask_buf);
      mask_height = gimp_temp_buf_get_height (mask_buf);

      free_mask = TRUE;
    }

  return_buf = gimp_temp_buf_new (mask_width, mask_height,
                                  babl_format ("R'G'B'A u8"));

  mask = mask_data = gimp_temp_buf_lock (mask_buf, babl_format ("Y u8"),
                                         GEGL_ACCESS_READ);
  buf  = gimp_temp_buf_get_data (return_buf);

  if (pixmap_buf)
    {
      guchar *pixmap_data;
      guchar *pixmap;

      pixmap = pixmap_data = gimp_temp_buf_lock (pixmap_buf,
                                                 babl_format ("R'G'B' u8"),
                                                 GEGL_ACCESS_READ);

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

      gimp_temp_buf_unlock (pixmap_buf, pixmap_data);
    }
  else
    {
      guint8 rgb[3] = { 0, 0, 0 };

      if (fg_color)
        gegl_color_get_pixel (fg_color, babl_format ("R'G'B' u8"), rgb);

      for (y = 0; y < mask_height; y++)
        {
          for (x = 0; x < mask_width ; x++)
            {
              *buf++ = rgb[0];
              *buf++ = rgb[1];
              *buf++ = rgb[2];
              *buf++ = *mask++;
            }
        }
    }

  gimp_temp_buf_unlock (mask_buf, mask_data);

  if (free_mask)
    {
      gimp_temp_buf_unref ((GimpTempBuf *) mask_buf);

      gimp_brush_end_use (brush);
    }

  return return_buf;
}

static gchar *
gimp_brush_get_description (GimpViewable  *viewable,
                            gchar        **tooltip)
{
  GimpBrush *brush = GIMP_BRUSH (viewable);

  return g_strdup_printf ("%s (%d × %d)",
                          gimp_object_get_name (brush),
                          gimp_temp_buf_get_width  (brush->priv->mask),
                          gimp_temp_buf_get_height (brush->priv->mask));
}

static void
gimp_brush_dirty (GimpData *data)
{
  GimpBrush *brush = GIMP_BRUSH (data);

  if (brush->priv->mask_cache)
    gimp_brush_cache_clear (brush->priv->mask_cache);

  if (brush->priv->pixmap_cache)
    gimp_brush_cache_clear (brush->priv->pixmap_cache);

  if (brush->priv->boundary_cache)
    gimp_brush_cache_clear (brush->priv->boundary_cache);

  gimp_brush_mipmap_clear (brush);

  g_clear_pointer (&brush->priv->blurred_mask,   gimp_temp_buf_unref);
  g_clear_pointer (&brush->priv->blurred_pixmap, gimp_temp_buf_unref);

  GIMP_DATA_CLASS (parent_class)->dirty (data);
}

static const gchar *
gimp_brush_get_extension (GimpData *data)
{
  return GIMP_BRUSH_FILE_EXTENSION;
}

static void
gimp_brush_copy (GimpData *data,
                 GimpData *src_data)
{
  GimpBrush *brush     = GIMP_BRUSH (data);
  GimpBrush *src_brush = GIMP_BRUSH (src_data);

  g_clear_pointer (&brush->priv->mask, gimp_temp_buf_unref);
  if (src_brush->priv->mask)
    brush->priv->mask = gimp_temp_buf_copy (src_brush->priv->mask);

  g_clear_pointer (&brush->priv->pixmap, gimp_temp_buf_unref);
  if (src_brush->priv->pixmap)
    brush->priv->pixmap = gimp_temp_buf_copy (src_brush->priv->pixmap);

  brush->priv->spacing = src_brush->priv->spacing;
  brush->priv->x_axis  = src_brush->priv->x_axis;
  brush->priv->y_axis  = src_brush->priv->y_axis;

  gimp_data_dirty (data);
}

static void
gimp_brush_real_begin_use (GimpBrush *brush)
{
  brush->priv->mask_cache =
    gimp_brush_cache_new ((GDestroyNotify) gimp_temp_buf_unref, 'M', 'm');

  brush->priv->pixmap_cache =
    gimp_brush_cache_new ((GDestroyNotify) gimp_temp_buf_unref, 'P', 'p');

  brush->priv->boundary_cache =
    gimp_brush_cache_new ((GDestroyNotify) gimp_bezier_desc_free, 'B', 'b');
}

static void
gimp_brush_real_end_use (GimpBrush *brush)
{
  g_clear_object (&brush->priv->mask_cache);
  g_clear_object (&brush->priv->pixmap_cache);
  g_clear_object (&brush->priv->boundary_cache);

  g_clear_pointer (&brush->priv->blurred_mask,   gimp_temp_buf_unref);
  g_clear_pointer (&brush->priv->blurred_pixmap, gimp_temp_buf_unref);
}

static GimpBrush *
gimp_brush_real_select_brush (GimpBrush        *brush,
                              const GimpCoords *last_coords,
                              const GimpCoords *current_coords)
{
  return brush;
}

static gboolean
gimp_brush_real_want_null_motion (GimpBrush        *brush,
                                  const GimpCoords *last_coords,
                                  const GimpCoords *current_coords)
{
  return TRUE;
}

static gchar *
gimp_brush_get_checksum (GimpTagged *tagged)
{
  GimpBrush *brush           = GIMP_BRUSH (tagged);
  gchar     *checksum_string = NULL;

  if (brush->priv->mask)
    {
      GChecksum *checksum = g_checksum_new (G_CHECKSUM_MD5);

      g_checksum_update (checksum,
                         gimp_temp_buf_get_data (brush->priv->mask),
                         gimp_temp_buf_get_data_size (brush->priv->mask));
      if (brush->priv->pixmap)
        g_checksum_update (checksum,
                           gimp_temp_buf_get_data (brush->priv->pixmap),
                           gimp_temp_buf_get_data_size (brush->priv->pixmap));
      g_checksum_update (checksum,
                         (const guchar *) &brush->priv->spacing,
                         sizeof (brush->priv->spacing));
      g_checksum_update (checksum,
                         (const guchar *) &brush->priv->x_axis,
                         sizeof (brush->priv->x_axis));
      g_checksum_update (checksum,
                         (const guchar *) &brush->priv->y_axis,
                         sizeof (brush->priv->y_axis));

      checksum_string = g_strdup (g_checksum_get_string (checksum));

      g_checksum_free (checksum);
    }

  return checksum_string;
}

/*  public functions  */

GimpData *
gimp_brush_new (GimpContext *context,
                const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);

  return gimp_brush_generated_new (name,
                                   GIMP_BRUSH_GENERATED_CIRCLE,
                                   5.0, 2, 0.5, 1.0, 0.0);
}

GimpData *
gimp_brush_get_standard (GimpContext *context)
{
  static GimpData *standard_brush = NULL;

  if (! standard_brush)
    {
      g_set_weak_pointer (&standard_brush,
                          gimp_brush_new (context, "Standard"));

      gimp_data_clean (standard_brush);
      gimp_data_make_internal (standard_brush, "gimp-brush-standard");
    }

  return standard_brush;
}

void
gimp_brush_begin_use (GimpBrush *brush)
{
  g_return_if_fail (GIMP_IS_BRUSH (brush));

  brush->priv->use_count++;

  if (brush->priv->use_count == 1)
    GIMP_BRUSH_GET_CLASS (brush)->begin_use (brush);
}

void
gimp_brush_end_use (GimpBrush *brush)
{
  g_return_if_fail (GIMP_IS_BRUSH (brush));
  g_return_if_fail (brush->priv->use_count > 0);

  brush->priv->use_count--;

  if (brush->priv->use_count == 0)
    GIMP_BRUSH_GET_CLASS (brush)->end_use (brush);
}

GimpBrush *
gimp_brush_select_brush (GimpBrush        *brush,
                         const GimpCoords *last_coords,
                         const GimpCoords *current_coords)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (last_coords != NULL, NULL);
  g_return_val_if_fail (current_coords != NULL, NULL);

  return GIMP_BRUSH_GET_CLASS (brush)->select_brush (brush,
                                                     last_coords,
                                                     current_coords);
}

gboolean
gimp_brush_want_null_motion (GimpBrush        *brush,
                             const GimpCoords *last_coords,
                             const GimpCoords *current_coords)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), FALSE);
  g_return_val_if_fail (last_coords != NULL, FALSE);
  g_return_val_if_fail (current_coords != NULL, FALSE);

  return GIMP_BRUSH_GET_CLASS (brush)->want_null_motion (brush,
                                                         last_coords,
                                                         current_coords);
}

void
gimp_brush_transform_size (GimpBrush     *brush,
                           gdouble        scale,
                           gdouble        aspect_ratio,
                           gdouble        angle,
                           gboolean       reflect,
                           gint          *width,
                           gint          *height)
{
  g_return_if_fail (GIMP_IS_BRUSH (brush));
  g_return_if_fail (scale > 0.0);
  g_return_if_fail (width != NULL);
  g_return_if_fail (height != NULL);

  if (scale             == 1.0 &&
      aspect_ratio      == 0.0 &&
      fmod (angle, 0.5) == 0.0)
    {
      *width  = gimp_temp_buf_get_width  (brush->priv->mask);
      *height = gimp_temp_buf_get_height (brush->priv->mask);

      return;
    }

  GIMP_BRUSH_GET_CLASS (brush)->transform_size (brush,
                                                scale, aspect_ratio, angle, reflect,
                                                width, height);
}

const GimpTempBuf *
gimp_brush_transform_mask (GimpBrush *brush,
                           gdouble    scale,
                           gdouble    aspect_ratio,
                           gdouble    angle,
                           gboolean   reflect,
                           gdouble    hardness)
{
  const GimpTempBuf *mask;
  gint               width;
  gint               height;
  gdouble            effective_hardness = hardness;

  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (scale > 0.0, NULL);

  gimp_brush_transform_size (brush,
                             scale, aspect_ratio, angle, reflect,
                             &width, &height);

  mask = gimp_brush_cache_get (brush->priv->mask_cache,
                               width, height,
                               scale, aspect_ratio, angle, reflect, hardness);

  if (! mask)
    {
#if 0
      /* This code makes sure that brushes using blur for hardness
       * (all of them but generated) are blurred once and no more.
       * It also makes hardnes dynamics not work for these brushes.
       * This is intentional. Confoliving for each stamp is too expensive.*/
      if (! brush->priv->blurred_mask &&
          ! GIMP_IS_BRUSH_GENERATED(brush) &&
          ! GIMP_IS_BRUSH_PIPE(brush) && /*Can't cache pipes. Sanely anyway*/
          hardness < 1.0)
        {
           brush->priv->blurred_mask = GIMP_BRUSH_GET_CLASS (brush)->transform_mask (brush,
                                                             1.0,
                                                             0.0,
                                                             0.0,
                                                             FALSE,
                                                             hardness);
           brush->priv->blur_hardness = hardness;
        }

      if (brush->priv->blurred_mask)
        {
          effective_hardness = 1.0; /*Hardness has already been applied*/
        }
#endif

      mask = GIMP_BRUSH_GET_CLASS (brush)->transform_mask (brush,
                                                           scale,
                                                           aspect_ratio,
                                                           angle,
                                                           reflect,
                                                           effective_hardness);

      gimp_brush_cache_add (brush->priv->mask_cache,
                            (gpointer) mask,
                            width, height,
                            scale, aspect_ratio, angle, reflect, effective_hardness);
    }

  return mask;
}

const GimpTempBuf *
gimp_brush_transform_pixmap (GimpBrush *brush,
                             gdouble    scale,
                             gdouble    aspect_ratio,
                             gdouble    angle,
                             gboolean   reflect,
                             gdouble    hardness)
{
  const GimpTempBuf *pixmap;
  gint               width;
  gint               height;
  gdouble            effective_hardness = hardness;

  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (brush->priv->pixmap != NULL, NULL);
  g_return_val_if_fail (scale > 0.0, NULL);

  gimp_brush_transform_size (brush,
                             scale, aspect_ratio, angle, reflect,
                             &width, &height);

  pixmap = gimp_brush_cache_get (brush->priv->pixmap_cache,
                                 width, height,
                                 scale, aspect_ratio, angle, reflect, hardness);

  if (! pixmap)
    {
#if 0
     if (! brush->priv->blurred_pixmap &&
         ! GIMP_IS_BRUSH_GENERATED(brush) &&
         ! GIMP_IS_BRUSH_PIPE(brush) /*Can't cache pipes. Sanely anyway*/
         && hardness < 1.0)
      {
         brush->priv->blurred_pixmap = GIMP_BRUSH_GET_CLASS (brush)->transform_pixmap (brush,
                                                                  1.0,
                                                                  0.0,
                                                                  0.0,
                                                                  FALSE,
                                                                  hardness);
         brush->priv->blur_hardness = hardness;
       }

      if (brush->priv->blurred_pixmap) {
        effective_hardness = 1.0; /*Hardness has already been applied*/
      }
#endif

      pixmap = GIMP_BRUSH_GET_CLASS (brush)->transform_pixmap (brush,
                                                               scale,
                                                               aspect_ratio,
                                                               angle,
                                                               reflect,
                                                               effective_hardness);

      gimp_brush_cache_add (brush->priv->pixmap_cache,
                            (gpointer) pixmap,
                            width, height,
                            scale, aspect_ratio, angle, reflect, effective_hardness);
    }

  return pixmap;
}

const GimpBezierDesc *
gimp_brush_transform_boundary (GimpBrush *brush,
                               gdouble    scale,
                               gdouble    aspect_ratio,
                               gdouble    angle,
                               gboolean   reflect,
                               gdouble    hardness,
                               gint      *width,
                               gint      *height)
{
  const GimpBezierDesc *boundary;

  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (scale > 0.0, NULL);
  g_return_val_if_fail (width != NULL, NULL);
  g_return_val_if_fail (height != NULL, NULL);

  gimp_brush_transform_size (brush,
                             scale, aspect_ratio, angle, reflect,
                             width, height);

  boundary = gimp_brush_cache_get (brush->priv->boundary_cache,
                                   *width, *height,
                                   scale, aspect_ratio, angle, reflect, hardness);

  if (! boundary)
    {
      boundary = GIMP_BRUSH_GET_CLASS (brush)->transform_boundary (brush,
                                                                   scale,
                                                                   aspect_ratio,
                                                                   angle,
                                                                   reflect,
                                                                   hardness,
                                                                   width,
                                                                   height);

      /*  while the brush mask is always at least 1x1 pixels, its
       *  outline can correctly be NULL
       *
       *  FIXME: make the cache handle NULL things when it is
       *         properly implemented
       */
      if (boundary)
        gimp_brush_cache_add (brush->priv->boundary_cache,
                              (gpointer) boundary,
                              *width, *height,
                              scale, aspect_ratio, angle, reflect, hardness);
    }

  return boundary;
}

GimpTempBuf *
gimp_brush_get_mask (GimpBrush *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);

  if (brush->priv->blurred_mask)
    {
      return brush->priv->blurred_mask;
    }
  return brush->priv->mask;
}

GimpTempBuf *
gimp_brush_get_pixmap (GimpBrush *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);

  if(brush->priv->blurred_pixmap)
    {
      return brush->priv->blurred_pixmap;
    }
  return brush->priv->pixmap;
}

void
gimp_brush_flush_blur_caches (GimpBrush *brush)
{
#if 0
  g_clear_pointer (&brush->priv->blurred_mask,   gimp_temp_buf_unref);
  g_clear_pointer (&brush->priv->blurred_pixmap, gimp_temp_buf_unref);

  if (brush->priv->mask_cache)
    gimp_brush_cache_clear (brush->priv->mask_cache);

  if (brush->priv->pixmap_cache)
    gimp_brush_cache_clear (brush->priv->pixmap_cache);

  if (brush->priv->boundary_cache)
    gimp_brush_cache_clear (brush->priv->boundary_cache);
#endif
}

gdouble
gimp_brush_get_blur_hardness (GimpBrush *brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), 0);

  return brush->priv->blur_hardness;
}

gint
gimp_brush_get_width (GimpBrush *brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), 0);

  if (brush->priv->blurred_mask)
    return gimp_temp_buf_get_width (brush->priv->blurred_mask);

  if (brush->priv->blurred_pixmap)
    return gimp_temp_buf_get_width (brush->priv->blurred_pixmap);

  return gimp_temp_buf_get_width (brush->priv->mask);
}

gint
gimp_brush_get_height (GimpBrush *brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), 0);

  if (brush->priv->blurred_mask)
    return gimp_temp_buf_get_height (brush->priv->blurred_mask);

  if (brush->priv->blurred_pixmap)
    return gimp_temp_buf_get_height (brush->priv->blurred_pixmap);

  return gimp_temp_buf_get_height (brush->priv->mask);
}

gint
gimp_brush_get_spacing (GimpBrush *brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), 0);

  return brush->priv->spacing;
}

void
gimp_brush_set_spacing (GimpBrush *brush,
                        gint       spacing)
{
  g_return_if_fail (GIMP_IS_BRUSH (brush));

  if (brush->priv->spacing != spacing)
    {
      brush->priv->spacing = spacing;

      g_signal_emit (brush, brush_signals[SPACING_CHANGED], 0);
      g_object_notify (G_OBJECT (brush), "spacing");
    }
}

static const GimpVector2 fail = { 0.0, 0.0 };

GimpVector2
gimp_brush_get_x_axis (GimpBrush *brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), fail);

  return brush->priv->x_axis;
}

GimpVector2
gimp_brush_get_y_axis (GimpBrush *brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), fail);

  return brush->priv->y_axis;
}
