/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "ligmabezierdesc.h"
#include "ligmabrush.h"
#include "ligmabrush-boundary.h"
#include "ligmabrush-load.h"
#include "ligmabrush-mipmap.h"
#include "ligmabrush-private.h"
#include "ligmabrush-save.h"
#include "ligmabrush-transform.h"
#include "ligmabrushcache.h"
#include "ligmabrushgenerated.h"
#include "ligmabrushpipe.h"
#include "ligmatagged.h"
#include "ligmatempbuf.h"

#include "ligma-intl.h"


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


static void          ligma_brush_tagged_iface_init     (LigmaTaggedInterface  *iface);

static void          ligma_brush_finalize              (GObject              *object);
static void          ligma_brush_set_property          (GObject              *object,
                                                       guint                 property_id,
                                                       const GValue         *value,
                                                       GParamSpec           *pspec);
static void          ligma_brush_get_property          (GObject              *object,
                                                       guint                 property_id,
                                                       GValue               *value,
                                                       GParamSpec           *pspec);

static gint64        ligma_brush_get_memsize           (LigmaObject           *object,
                                                       gint64               *gui_size);

static gboolean      ligma_brush_get_size              (LigmaViewable         *viewable,
                                                       gint                 *width,
                                                       gint                 *height);
static LigmaTempBuf * ligma_brush_get_new_preview       (LigmaViewable         *viewable,
                                                       LigmaContext          *context,
                                                       gint                  width,
                                                       gint                  height);
static gchar       * ligma_brush_get_description       (LigmaViewable         *viewable,
                                                       gchar               **tooltip);

static void          ligma_brush_dirty                 (LigmaData             *data);
static const gchar * ligma_brush_get_extension         (LigmaData             *data);
static void          ligma_brush_copy                  (LigmaData             *data,
                                                       LigmaData             *src_data);

static void          ligma_brush_real_begin_use        (LigmaBrush            *brush);
static void          ligma_brush_real_end_use          (LigmaBrush            *brush);
static LigmaBrush   * ligma_brush_real_select_brush     (LigmaBrush            *brush,
                                                       const LigmaCoords     *last_coords,
                                                       const LigmaCoords     *current_coords);
static gboolean      ligma_brush_real_want_null_motion (LigmaBrush            *brush,
                                                       const LigmaCoords     *last_coords,
                                                       const LigmaCoords     *current_coords);

static gchar       * ligma_brush_get_checksum          (LigmaTagged           *tagged);


G_DEFINE_TYPE_WITH_CODE (LigmaBrush, ligma_brush, LIGMA_TYPE_DATA,
                         G_ADD_PRIVATE (LigmaBrush)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_TAGGED,
                                                ligma_brush_tagged_iface_init))

#define parent_class ligma_brush_parent_class

static guint brush_signals[LAST_SIGNAL] = { 0 };


static void
ligma_brush_class_init (LigmaBrushClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);
  LigmaDataClass     *data_class        = LIGMA_DATA_CLASS (klass);

  brush_signals[SPACING_CHANGED] =
    g_signal_new ("spacing-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaBrushClass, spacing_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize            = ligma_brush_finalize;
  object_class->get_property        = ligma_brush_get_property;
  object_class->set_property        = ligma_brush_set_property;

  ligma_object_class->get_memsize    = ligma_brush_get_memsize;

  viewable_class->default_icon_name = "ligma-tool-paintbrush";
  viewable_class->get_size          = ligma_brush_get_size;
  viewable_class->get_new_preview   = ligma_brush_get_new_preview;
  viewable_class->get_description   = ligma_brush_get_description;

  data_class->dirty                 = ligma_brush_dirty;
  data_class->save                  = ligma_brush_save;
  data_class->get_extension         = ligma_brush_get_extension;
  data_class->copy                  = ligma_brush_copy;

  klass->begin_use                  = ligma_brush_real_begin_use;
  klass->end_use                    = ligma_brush_real_end_use;
  klass->select_brush               = ligma_brush_real_select_brush;
  klass->want_null_motion           = ligma_brush_real_want_null_motion;
  klass->transform_size             = ligma_brush_real_transform_size;
  klass->transform_mask             = ligma_brush_real_transform_mask;
  klass->transform_pixmap           = ligma_brush_real_transform_pixmap;
  klass->transform_boundary         = ligma_brush_real_transform_boundary;
  klass->spacing_changed            = NULL;

  g_object_class_install_property (object_class, PROP_SPACING,
                                   g_param_spec_double ("spacing", NULL,
                                                        _("Brush Spacing"),
                                                        1.0, 5000.0, 20.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
ligma_brush_tagged_iface_init (LigmaTaggedInterface *iface)
{
  iface->get_checksum = ligma_brush_get_checksum;
}

static void
ligma_brush_init (LigmaBrush *brush)
{
  brush->priv = ligma_brush_get_instance_private (brush);

  brush->priv->spacing  = 20;
  brush->priv->x_axis.x = 15.0;
  brush->priv->x_axis.y =  0.0;
  brush->priv->y_axis.x =  0.0;
  brush->priv->y_axis.y = 15.0;

  brush->priv->blur_hardness = 1.0;
}

static void
ligma_brush_finalize (GObject *object)
{
  LigmaBrush *brush = LIGMA_BRUSH (object);

  g_clear_pointer (&brush->priv->mask,          ligma_temp_buf_unref);
  g_clear_pointer (&brush->priv->pixmap,        ligma_temp_buf_unref);
  g_clear_pointer (&brush->priv->blurred_mask,   ligma_temp_buf_unref);
  g_clear_pointer (&brush->priv->blurred_pixmap, ligma_temp_buf_unref);

  ligma_brush_mipmap_clear (brush);

  g_clear_object (&brush->priv->mask_cache);
  g_clear_object (&brush->priv->pixmap_cache);
  g_clear_object (&brush->priv->boundary_cache);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_brush_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  LigmaBrush *brush = LIGMA_BRUSH (object);

  switch (property_id)
    {
    case PROP_SPACING:
      ligma_brush_set_spacing (brush, g_value_get_double (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_brush_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  LigmaBrush *brush = LIGMA_BRUSH (object);

  switch (property_id)
    {
    case PROP_SPACING:
      g_value_set_double (value, ligma_brush_get_spacing (brush));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
ligma_brush_get_memsize (LigmaObject *object,
                        gint64     *gui_size)
{
  LigmaBrush *brush   = LIGMA_BRUSH (object);
  gint64     memsize = 0;

  memsize += ligma_temp_buf_get_memsize (brush->priv->mask);
  memsize += ligma_temp_buf_get_memsize (brush->priv->pixmap);

  memsize += ligma_brush_mipmap_get_memsize (brush);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
ligma_brush_get_size (LigmaViewable *viewable,
                     gint         *width,
                     gint         *height)
{
  LigmaBrush *brush = LIGMA_BRUSH (viewable);

  *width  = ligma_temp_buf_get_width  (brush->priv->mask);
  *height = ligma_temp_buf_get_height (brush->priv->mask);

  return TRUE;
}

static LigmaTempBuf *
ligma_brush_get_new_preview (LigmaViewable *viewable,
                            LigmaContext  *context,
                            gint          width,
                            gint          height)
{
  LigmaBrush         *brush       = LIGMA_BRUSH (viewable);
  const LigmaTempBuf *mask_buf    = brush->priv->mask;
  const LigmaTempBuf *pixmap_buf  = brush->priv->pixmap;
  LigmaTempBuf       *return_buf  = NULL;
  gint               mask_width;
  gint               mask_height;
  guchar            *mask_data;
  guchar            *mask;
  guchar            *buf;
  gint               x, y;
  gboolean           scaled = FALSE;

  mask_width  = ligma_temp_buf_get_width  (mask_buf);
  mask_height = ligma_temp_buf_get_height (mask_buf);

  if (mask_width > width || mask_height > height)
    {
      gdouble ratio_x = (gdouble) width  / (gdouble) mask_width;
      gdouble ratio_y = (gdouble) height / (gdouble) mask_height;
      gdouble scale   = MIN (ratio_x, ratio_y);

      if (scale != 1.0)
        {
          ligma_brush_begin_use (brush);

          if (LIGMA_IS_BRUSH_GENERATED (brush))
            {
               LigmaBrushGenerated *gen_brush = LIGMA_BRUSH_GENERATED (brush);

               mask_buf = ligma_brush_transform_mask (brush, scale,
                                                     (ligma_brush_generated_get_aspect_ratio (gen_brush) - 1.0) * 20.0 / 19.0,
                                                     ligma_brush_generated_get_angle (gen_brush) / 360.0,
                                                     FALSE,
                                                     ligma_brush_generated_get_hardness (gen_brush));
            }
          else
            mask_buf = ligma_brush_transform_mask (brush, scale,
                                                  0.0, 0.0, FALSE, 1.0);

          if (! mask_buf)
            {
              mask_buf = ligma_temp_buf_new (1, 1, babl_format ("Y u8"));
              ligma_temp_buf_data_clear ((LigmaTempBuf *) mask_buf);
            }
          else
            {
              ligma_temp_buf_ref ((LigmaTempBuf *) mask_buf);
            }

          if (pixmap_buf)
            pixmap_buf = ligma_brush_transform_pixmap (brush, scale,
                                                      0.0, 0.0, FALSE, 1.0);

          mask_width  = ligma_temp_buf_get_width  (mask_buf);
          mask_height = ligma_temp_buf_get_height (mask_buf);

          scaled = TRUE;
        }
    }

  return_buf = ligma_temp_buf_new (mask_width, mask_height,
                                  babl_format ("R'G'B'A u8"));

  mask = mask_data = ligma_temp_buf_lock (mask_buf, babl_format ("Y u8"),
                                         GEGL_ACCESS_READ);
  buf  = ligma_temp_buf_get_data (return_buf);

  if (pixmap_buf)
    {
      guchar *pixmap_data;
      guchar *pixmap;

      pixmap = pixmap_data = ligma_temp_buf_lock (pixmap_buf,
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

      ligma_temp_buf_unlock (pixmap_buf, pixmap_data);
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

  ligma_temp_buf_unlock (mask_buf, mask_data);

  if (scaled)
    {
      ligma_temp_buf_unref ((LigmaTempBuf *) mask_buf);

      ligma_brush_end_use (brush);
    }

  return return_buf;
}

static gchar *
ligma_brush_get_description (LigmaViewable  *viewable,
                            gchar        **tooltip)
{
  LigmaBrush *brush = LIGMA_BRUSH (viewable);

  return g_strdup_printf ("%s (%d × %d)",
                          ligma_object_get_name (brush),
                          ligma_temp_buf_get_width  (brush->priv->mask),
                          ligma_temp_buf_get_height (brush->priv->mask));
}

static void
ligma_brush_dirty (LigmaData *data)
{
  LigmaBrush *brush = LIGMA_BRUSH (data);

  if (brush->priv->mask_cache)
    ligma_brush_cache_clear (brush->priv->mask_cache);

  if (brush->priv->pixmap_cache)
    ligma_brush_cache_clear (brush->priv->pixmap_cache);

  if (brush->priv->boundary_cache)
    ligma_brush_cache_clear (brush->priv->boundary_cache);

  ligma_brush_mipmap_clear (brush);

  g_clear_pointer (&brush->priv->blurred_mask,   ligma_temp_buf_unref);
  g_clear_pointer (&brush->priv->blurred_pixmap, ligma_temp_buf_unref);

  LIGMA_DATA_CLASS (parent_class)->dirty (data);
}

static const gchar *
ligma_brush_get_extension (LigmaData *data)
{
  return LIGMA_BRUSH_FILE_EXTENSION;
}

static void
ligma_brush_copy (LigmaData *data,
                 LigmaData *src_data)
{
  LigmaBrush *brush     = LIGMA_BRUSH (data);
  LigmaBrush *src_brush = LIGMA_BRUSH (src_data);

  g_clear_pointer (&brush->priv->mask, ligma_temp_buf_unref);
  if (src_brush->priv->mask)
    brush->priv->mask = ligma_temp_buf_copy (src_brush->priv->mask);

  g_clear_pointer (&brush->priv->pixmap, ligma_temp_buf_unref);
  if (src_brush->priv->pixmap)
    brush->priv->pixmap = ligma_temp_buf_copy (src_brush->priv->pixmap);

  brush->priv->spacing = src_brush->priv->spacing;
  brush->priv->x_axis  = src_brush->priv->x_axis;
  brush->priv->y_axis  = src_brush->priv->y_axis;

  ligma_data_dirty (data);
}

static void
ligma_brush_real_begin_use (LigmaBrush *brush)
{
  brush->priv->mask_cache =
    ligma_brush_cache_new ((GDestroyNotify) ligma_temp_buf_unref, 'M', 'm');

  brush->priv->pixmap_cache =
    ligma_brush_cache_new ((GDestroyNotify) ligma_temp_buf_unref, 'P', 'p');

  brush->priv->boundary_cache =
    ligma_brush_cache_new ((GDestroyNotify) ligma_bezier_desc_free, 'B', 'b');
}

static void
ligma_brush_real_end_use (LigmaBrush *brush)
{
  g_clear_object (&brush->priv->mask_cache);
  g_clear_object (&brush->priv->pixmap_cache);
  g_clear_object (&brush->priv->boundary_cache);

  g_clear_pointer (&brush->priv->blurred_mask,   ligma_temp_buf_unref);
  g_clear_pointer (&brush->priv->blurred_pixmap, ligma_temp_buf_unref);
}

static LigmaBrush *
ligma_brush_real_select_brush (LigmaBrush        *brush,
                              const LigmaCoords *last_coords,
                              const LigmaCoords *current_coords)
{
  return brush;
}

static gboolean
ligma_brush_real_want_null_motion (LigmaBrush        *brush,
                                  const LigmaCoords *last_coords,
                                  const LigmaCoords *current_coords)
{
  return TRUE;
}

static gchar *
ligma_brush_get_checksum (LigmaTagged *tagged)
{
  LigmaBrush *brush           = LIGMA_BRUSH (tagged);
  gchar     *checksum_string = NULL;

  if (brush->priv->mask)
    {
      GChecksum *checksum = g_checksum_new (G_CHECKSUM_MD5);

      g_checksum_update (checksum,
                         ligma_temp_buf_get_data (brush->priv->mask),
                         ligma_temp_buf_get_data_size (brush->priv->mask));
      if (brush->priv->pixmap)
        g_checksum_update (checksum,
                           ligma_temp_buf_get_data (brush->priv->pixmap),
                           ligma_temp_buf_get_data_size (brush->priv->pixmap));
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

LigmaData *
ligma_brush_new (LigmaContext *context,
                const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);

  return ligma_brush_generated_new (name,
                                   LIGMA_BRUSH_GENERATED_CIRCLE,
                                   5.0, 2, 0.5, 1.0, 0.0);
}

LigmaData *
ligma_brush_get_standard (LigmaContext *context)
{
  static LigmaData *standard_brush = NULL;

  if (! standard_brush)
    {
      standard_brush = ligma_brush_new (context, "Standard");

      ligma_data_clean (standard_brush);
      ligma_data_make_internal (standard_brush, "ligma-brush-standard");

      g_object_add_weak_pointer (G_OBJECT (standard_brush),
                                 (gpointer *) &standard_brush);
    }

  return standard_brush;
}

void
ligma_brush_begin_use (LigmaBrush *brush)
{
  g_return_if_fail (LIGMA_IS_BRUSH (brush));

  brush->priv->use_count++;

  if (brush->priv->use_count == 1)
    LIGMA_BRUSH_GET_CLASS (brush)->begin_use (brush);
}

void
ligma_brush_end_use (LigmaBrush *brush)
{
  g_return_if_fail (LIGMA_IS_BRUSH (brush));
  g_return_if_fail (brush->priv->use_count > 0);

  brush->priv->use_count--;

  if (brush->priv->use_count == 0)
    LIGMA_BRUSH_GET_CLASS (brush)->end_use (brush);
}

LigmaBrush *
ligma_brush_select_brush (LigmaBrush        *brush,
                         const LigmaCoords *last_coords,
                         const LigmaCoords *current_coords)
{
  g_return_val_if_fail (LIGMA_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (last_coords != NULL, NULL);
  g_return_val_if_fail (current_coords != NULL, NULL);

  return LIGMA_BRUSH_GET_CLASS (brush)->select_brush (brush,
                                                     last_coords,
                                                     current_coords);
}

gboolean
ligma_brush_want_null_motion (LigmaBrush        *brush,
                             const LigmaCoords *last_coords,
                             const LigmaCoords *current_coords)
{
  g_return_val_if_fail (LIGMA_IS_BRUSH (brush), FALSE);
  g_return_val_if_fail (last_coords != NULL, FALSE);
  g_return_val_if_fail (current_coords != NULL, FALSE);

  return LIGMA_BRUSH_GET_CLASS (brush)->want_null_motion (brush,
                                                         last_coords,
                                                         current_coords);
}

void
ligma_brush_transform_size (LigmaBrush     *brush,
                           gdouble        scale,
                           gdouble        aspect_ratio,
                           gdouble        angle,
                           gboolean       reflect,
                           gint          *width,
                           gint          *height)
{
  g_return_if_fail (LIGMA_IS_BRUSH (brush));
  g_return_if_fail (scale > 0.0);
  g_return_if_fail (width != NULL);
  g_return_if_fail (height != NULL);

  if (scale             == 1.0 &&
      aspect_ratio      == 0.0 &&
      fmod (angle, 0.5) == 0.0)
    {
      *width  = ligma_temp_buf_get_width  (brush->priv->mask);
      *height = ligma_temp_buf_get_height (brush->priv->mask);

      return;
    }

  LIGMA_BRUSH_GET_CLASS (brush)->transform_size (brush,
                                                scale, aspect_ratio, angle, reflect,
                                                width, height);
}

const LigmaTempBuf *
ligma_brush_transform_mask (LigmaBrush *brush,
                           gdouble    scale,
                           gdouble    aspect_ratio,
                           gdouble    angle,
                           gboolean   reflect,
                           gdouble    hardness)
{
  const LigmaTempBuf *mask;
  gint               width;
  gint               height;
  gdouble            effective_hardness = hardness;

  g_return_val_if_fail (LIGMA_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (scale > 0.0, NULL);

  ligma_brush_transform_size (brush,
                             scale, aspect_ratio, angle, reflect,
                             &width, &height);

  mask = ligma_brush_cache_get (brush->priv->mask_cache,
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
          ! LIGMA_IS_BRUSH_GENERATED(brush) &&
          ! LIGMA_IS_BRUSH_PIPE(brush) && /*Can't cache pipes. Sanely anyway*/
          hardness < 1.0)
        {
           brush->priv->blurred_mask = LIGMA_BRUSH_GET_CLASS (brush)->transform_mask (brush,
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

      mask = LIGMA_BRUSH_GET_CLASS (brush)->transform_mask (brush,
                                                           scale,
                                                           aspect_ratio,
                                                           angle,
                                                           reflect,
                                                           effective_hardness);

      ligma_brush_cache_add (brush->priv->mask_cache,
                            (gpointer) mask,
                            width, height,
                            scale, aspect_ratio, angle, reflect, effective_hardness);
    }

  return mask;
}

const LigmaTempBuf *
ligma_brush_transform_pixmap (LigmaBrush *brush,
                             gdouble    scale,
                             gdouble    aspect_ratio,
                             gdouble    angle,
                             gboolean   reflect,
                             gdouble    hardness)
{
  const LigmaTempBuf *pixmap;
  gint               width;
  gint               height;
  gdouble            effective_hardness = hardness;

  g_return_val_if_fail (LIGMA_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (brush->priv->pixmap != NULL, NULL);
  g_return_val_if_fail (scale > 0.0, NULL);

  ligma_brush_transform_size (brush,
                             scale, aspect_ratio, angle, reflect,
                             &width, &height);

  pixmap = ligma_brush_cache_get (brush->priv->pixmap_cache,
                                 width, height,
                                 scale, aspect_ratio, angle, reflect, hardness);

  if (! pixmap)
    {
#if 0
     if (! brush->priv->blurred_pixmap &&
         ! LIGMA_IS_BRUSH_GENERATED(brush) &&
         ! LIGMA_IS_BRUSH_PIPE(brush) /*Can't cache pipes. Sanely anyway*/
         && hardness < 1.0)
      {
         brush->priv->blurred_pixmap = LIGMA_BRUSH_GET_CLASS (brush)->transform_pixmap (brush,
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

      pixmap = LIGMA_BRUSH_GET_CLASS (brush)->transform_pixmap (brush,
                                                               scale,
                                                               aspect_ratio,
                                                               angle,
                                                               reflect,
                                                               effective_hardness);

      ligma_brush_cache_add (brush->priv->pixmap_cache,
                            (gpointer) pixmap,
                            width, height,
                            scale, aspect_ratio, angle, reflect, effective_hardness);
    }

  return pixmap;
}

const LigmaBezierDesc *
ligma_brush_transform_boundary (LigmaBrush *brush,
                               gdouble    scale,
                               gdouble    aspect_ratio,
                               gdouble    angle,
                               gboolean   reflect,
                               gdouble    hardness,
                               gint      *width,
                               gint      *height)
{
  const LigmaBezierDesc *boundary;

  g_return_val_if_fail (LIGMA_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (scale > 0.0, NULL);
  g_return_val_if_fail (width != NULL, NULL);
  g_return_val_if_fail (height != NULL, NULL);

  ligma_brush_transform_size (brush,
                             scale, aspect_ratio, angle, reflect,
                             width, height);

  boundary = ligma_brush_cache_get (brush->priv->boundary_cache,
                                   *width, *height,
                                   scale, aspect_ratio, angle, reflect, hardness);

  if (! boundary)
    {
      boundary = LIGMA_BRUSH_GET_CLASS (brush)->transform_boundary (brush,
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
        ligma_brush_cache_add (brush->priv->boundary_cache,
                              (gpointer) boundary,
                              *width, *height,
                              scale, aspect_ratio, angle, reflect, hardness);
    }

  return boundary;
}

LigmaTempBuf *
ligma_brush_get_mask (LigmaBrush *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (LIGMA_IS_BRUSH (brush), NULL);

  if (brush->priv->blurred_mask)
    {
      return brush->priv->blurred_mask;
    }
  return brush->priv->mask;
}

LigmaTempBuf *
ligma_brush_get_pixmap (LigmaBrush *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (LIGMA_IS_BRUSH (brush), NULL);

  if(brush->priv->blurred_pixmap)
    {
      return brush->priv->blurred_pixmap;
    }
  return brush->priv->pixmap;
}

void
ligma_brush_flush_blur_caches (LigmaBrush *brush)
{
#if 0
  g_clear_pointer (&brush->priv->blurred_mask,   ligma_temp_buf_unref);
  g_clear_pointer (&brush->priv->blurred_pixmap, ligma_temp_buf_unref);

  if (brush->priv->mask_cache)
    ligma_brush_cache_clear (brush->priv->mask_cache);

  if (brush->priv->pixmap_cache)
    ligma_brush_cache_clear (brush->priv->pixmap_cache);

  if (brush->priv->boundary_cache)
    ligma_brush_cache_clear (brush->priv->boundary_cache);
#endif
}

gdouble
ligma_brush_get_blur_hardness (LigmaBrush *brush)
{
  g_return_val_if_fail (LIGMA_IS_BRUSH (brush), 0);

  return brush->priv->blur_hardness;
}

gint
ligma_brush_get_width (LigmaBrush *brush)
{
  g_return_val_if_fail (LIGMA_IS_BRUSH (brush), 0);

  if (brush->priv->blurred_mask)
    return ligma_temp_buf_get_width (brush->priv->blurred_mask);

  if (brush->priv->blurred_pixmap)
    return ligma_temp_buf_get_width (brush->priv->blurred_pixmap);

  return ligma_temp_buf_get_width (brush->priv->mask);
}

gint
ligma_brush_get_height (LigmaBrush *brush)
{
  g_return_val_if_fail (LIGMA_IS_BRUSH (brush), 0);

  if (brush->priv->blurred_mask)
    return ligma_temp_buf_get_height (brush->priv->blurred_mask);

  if (brush->priv->blurred_pixmap)
    return ligma_temp_buf_get_height (brush->priv->blurred_pixmap);

  return ligma_temp_buf_get_height (brush->priv->mask);
}

gint
ligma_brush_get_spacing (LigmaBrush *brush)
{
  g_return_val_if_fail (LIGMA_IS_BRUSH (brush), 0);

  return brush->priv->spacing;
}

void
ligma_brush_set_spacing (LigmaBrush *brush,
                        gint       spacing)
{
  g_return_if_fail (LIGMA_IS_BRUSH (brush));

  if (brush->priv->spacing != spacing)
    {
      brush->priv->spacing = spacing;

      g_signal_emit (brush, brush_signals[SPACING_CHANGED], 0);
      g_object_notify (G_OBJECT (brush), "spacing");
    }
}

static const LigmaVector2 fail = { 0.0, 0.0 };

LigmaVector2
ligma_brush_get_x_axis (LigmaBrush *brush)
{
  g_return_val_if_fail (LIGMA_IS_BRUSH (brush), fail);

  return brush->priv->x_axis;
}

LigmaVector2
ligma_brush_get_y_axis (LigmaBrush *brush)
{
  g_return_val_if_fail (LIGMA_IS_BRUSH (brush), fail);

  return brush->priv->y_axis;
}
