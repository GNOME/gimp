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
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-loops.h"
#include "gegl/ligma-gegl-utils.h"

#include "ligma-memsize.h"
#include "ligmabuffer.h"
#include "ligmaimage.h"
#include "ligmatempbuf.h"


static void          ligma_color_managed_iface_init (LigmaColorManagedInterface *iface);

static void          ligma_buffer_finalize          (GObject           *object);

static gint64        ligma_buffer_get_memsize       (LigmaObject        *object,
                                                    gint64            *gui_size);

static gboolean      ligma_buffer_get_size          (LigmaViewable      *viewable,
                                                    gint              *width,
                                                    gint              *height);
static void          ligma_buffer_get_preview_size  (LigmaViewable      *viewable,
                                                    gint               size,
                                                    gboolean           is_popup,
                                                    gboolean           dot_for_dot,
                                                    gint              *popup_width,
                                                    gint              *popup_height);
static gboolean      ligma_buffer_get_popup_size    (LigmaViewable      *viewable,
                                                    gint               width,
                                                    gint               height,
                                                    gboolean           dot_for_dot,
                                                    gint              *popup_width,
                                                    gint              *popup_height);
static LigmaTempBuf * ligma_buffer_get_new_preview   (LigmaViewable      *viewable,
                                                    LigmaContext       *context,
                                                    gint               width,
                                                    gint               height);
static GdkPixbuf   * ligma_buffer_get_new_pixbuf    (LigmaViewable      *viewable,
                                                    LigmaContext       *context,
                                                    gint               width,
                                                    gint               height);
static gchar       * ligma_buffer_get_description   (LigmaViewable      *viewable,
                                                    gchar            **tooltip);

static const guint8 *
       ligma_buffer_color_managed_get_icc_profile   (LigmaColorManaged  *managed,
                                                    gsize             *len);
static LigmaColorProfile *
       ligma_buffer_color_managed_get_color_profile (LigmaColorManaged  *managed);
static void
       ligma_buffer_color_managed_profile_changed   (LigmaColorManaged  *managed);


G_DEFINE_TYPE_WITH_CODE (LigmaBuffer, ligma_buffer, LIGMA_TYPE_VIEWABLE,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_COLOR_MANAGED,
                                                ligma_color_managed_iface_init))

#define parent_class ligma_buffer_parent_class


static void
ligma_buffer_class_init (LigmaBufferClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);

  object_class->finalize            = ligma_buffer_finalize;

  ligma_object_class->get_memsize    = ligma_buffer_get_memsize;

  viewable_class->default_icon_name = "edit-paste";
  viewable_class->name_editable     = TRUE;
  viewable_class->get_size          = ligma_buffer_get_size;
  viewable_class->get_preview_size  = ligma_buffer_get_preview_size;
  viewable_class->get_popup_size    = ligma_buffer_get_popup_size;
  viewable_class->get_new_preview   = ligma_buffer_get_new_preview;
  viewable_class->get_new_pixbuf    = ligma_buffer_get_new_pixbuf;
  viewable_class->get_description   = ligma_buffer_get_description;
}

static void
ligma_color_managed_iface_init (LigmaColorManagedInterface *iface)
{
  iface->get_icc_profile   = ligma_buffer_color_managed_get_icc_profile;
  iface->get_color_profile = ligma_buffer_color_managed_get_color_profile;
  iface->profile_changed   = ligma_buffer_color_managed_profile_changed;
}

static void
ligma_buffer_init (LigmaBuffer *buffer)
{
}

static void
ligma_buffer_finalize (GObject *object)
{
  LigmaBuffer *buffer = LIGMA_BUFFER (object);

  g_clear_object (&buffer->buffer);

  ligma_buffer_set_color_profile (buffer, NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
ligma_buffer_get_memsize (LigmaObject *object,
                         gint64     *gui_size)
{
  LigmaBuffer *buffer  = LIGMA_BUFFER (object);
  gint64      memsize = 0;

  memsize += ligma_gegl_buffer_get_memsize (buffer->buffer);
  memsize += ligma_g_object_get_memsize (G_OBJECT (buffer->color_profile));
  memsize += ligma_g_object_get_memsize (G_OBJECT (buffer->format_profile));

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
ligma_buffer_get_size (LigmaViewable *viewable,
                      gint         *width,
                      gint         *height)
{
  LigmaBuffer *buffer = LIGMA_BUFFER (viewable);

  *width  = ligma_buffer_get_width (buffer);
  *height = ligma_buffer_get_height (buffer);

  return TRUE;
}

static void
ligma_buffer_get_preview_size (LigmaViewable *viewable,
                              gint          size,
                              gboolean      is_popup,
                              gboolean      dot_for_dot,
                              gint         *width,
                              gint         *height)
{
  LigmaBuffer *buffer = LIGMA_BUFFER (viewable);

  ligma_viewable_calc_preview_size (ligma_buffer_get_width (buffer),
                                   ligma_buffer_get_height (buffer),
                                   size,
                                   size,
                                   dot_for_dot, 1.0, 1.0,
                                   width,
                                   height,
                                   NULL);
}

static gboolean
ligma_buffer_get_popup_size (LigmaViewable *viewable,
                            gint          width,
                            gint          height,
                            gboolean      dot_for_dot,
                            gint         *popup_width,
                            gint         *popup_height)
{
  LigmaBuffer *buffer;
  gint        buffer_width;
  gint        buffer_height;

  buffer        = LIGMA_BUFFER (viewable);
  buffer_width  = ligma_buffer_get_width (buffer);
  buffer_height = ligma_buffer_get_height (buffer);

  if (buffer_width > width || buffer_height > height)
    {
      gboolean scaling_up;

      ligma_viewable_calc_preview_size (buffer_width,
                                       buffer_height,
                                       width  * 2,
                                       height * 2,
                                       dot_for_dot, 1.0, 1.0,
                                       popup_width,
                                       popup_height,
                                       &scaling_up);

      if (scaling_up)
        {
          *popup_width  = buffer_width;
          *popup_height = buffer_height;
        }

      return TRUE;
    }

  return FALSE;
}

static LigmaTempBuf *
ligma_buffer_get_new_preview (LigmaViewable *viewable,
                             LigmaContext  *context,
                             gint          width,
                             gint          height)
{
  LigmaBuffer  *buffer = LIGMA_BUFFER (viewable);
  const Babl  *format = ligma_buffer_get_format (buffer);
  LigmaTempBuf *preview;

  if (babl_format_is_palette (format))
    format = ligma_babl_format (LIGMA_RGB,
                               LIGMA_PRECISION_U8_NON_LINEAR,
                               babl_format_has_alpha (format),
                               babl_format_get_space (format));
  else
    format = ligma_babl_format (ligma_babl_format_get_base_type (format),
                               ligma_babl_precision (LIGMA_COMPONENT_TYPE_U8,
                                                    ligma_babl_format_get_trc (format)),
                               babl_format_has_alpha (format),
                               babl_format_get_space (format));

  preview = ligma_temp_buf_new (width, height, format);

  gegl_buffer_get (buffer->buffer, GEGL_RECTANGLE (0, 0, width, height),
                   MIN ((gdouble) width  / (gdouble) ligma_buffer_get_width (buffer),
                        (gdouble) height / (gdouble) ligma_buffer_get_height (buffer)),
                   format,
                   ligma_temp_buf_get_data (preview),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  return preview;
}

static GdkPixbuf *
ligma_buffer_get_new_pixbuf (LigmaViewable *viewable,
                            LigmaContext  *context,
                            gint          width,
                            gint          height)
{
  LigmaBuffer *buffer = LIGMA_BUFFER (viewable);
  GdkPixbuf  *pixbuf;
  gdouble     scale;

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           width, height);

  scale = MIN ((gdouble) width  / (gdouble) ligma_buffer_get_width (buffer),
               (gdouble) height / (gdouble) ligma_buffer_get_height (buffer));

  if (buffer->color_profile)
    {
      LigmaColorProfile *srgb_profile;
      LigmaTempBuf      *temp_buf;
      GeglBuffer       *src_buf;
      GeglBuffer       *dest_buf;

      srgb_profile = ligma_color_profile_new_rgb_srgb ();

      temp_buf = ligma_temp_buf_new (width, height,
                                    ligma_buffer_get_format (buffer));

      gegl_buffer_get (buffer->buffer,
                       GEGL_RECTANGLE (0, 0, width, height),
                       scale,
                       ligma_temp_buf_get_format (temp_buf),
                       ligma_temp_buf_get_data (temp_buf),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      src_buf  = ligma_temp_buf_create_buffer (temp_buf);
      dest_buf = ligma_pixbuf_create_buffer (pixbuf);

      ligma_temp_buf_unref (temp_buf);

      ligma_gegl_convert_color_profile (src_buf,
                                       GEGL_RECTANGLE (0, 0, width, height),
                                       buffer->color_profile,
                                       dest_buf,
                                       GEGL_RECTANGLE (0, 0, 0, 0),
                                       srgb_profile,
                                       LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                       TRUE,
                                       NULL);

      g_object_unref (src_buf);
      g_object_unref (dest_buf);

      g_object_unref (srgb_profile);
    }
  else
    {
      gegl_buffer_get (buffer->buffer,
                       GEGL_RECTANGLE (0, 0, width, height),
                       scale,
                       ligma_pixbuf_get_format (pixbuf),
                       gdk_pixbuf_get_pixels (pixbuf),
                       gdk_pixbuf_get_rowstride (pixbuf),
                       GEGL_ABYSS_CLAMP);
    }

  return pixbuf;
}

static gchar *
ligma_buffer_get_description (LigmaViewable  *viewable,
                             gchar        **tooltip)
{
  LigmaBuffer *buffer = LIGMA_BUFFER (viewable);

  return g_strdup_printf ("%s (%d × %d)",
                          ligma_object_get_name (buffer),
                          ligma_buffer_get_width (buffer),
                          ligma_buffer_get_height (buffer));
}

static const guint8 *
ligma_buffer_color_managed_get_icc_profile (LigmaColorManaged *managed,
                                           gsize            *len)
{
  LigmaBuffer *buffer = LIGMA_BUFFER (managed);

  if (buffer->color_profile)
    return ligma_color_profile_get_icc_profile (buffer->color_profile, len);

  /* creates buffer->format_profile */
  ligma_color_managed_get_color_profile (managed);

  return ligma_color_profile_get_icc_profile (buffer->format_profile, len);
}

static LigmaColorProfile *
ligma_buffer_color_managed_get_color_profile (LigmaColorManaged *managed)
{
  LigmaBuffer *buffer = LIGMA_BUFFER (managed);

  if (buffer->color_profile)
    return buffer->color_profile;

  if (! buffer->format_profile)
    buffer->format_profile =
      ligma_babl_format_get_color_profile (ligma_buffer_get_format (buffer));

  return buffer->format_profile;
}

static void
ligma_buffer_color_managed_profile_changed (LigmaColorManaged *managed)
{
  ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (managed));
}


/*  public functions  */

LigmaBuffer *
ligma_buffer_new (GeglBuffer    *buffer,
                 const gchar   *name,
                 gint           offset_x,
                 gint           offset_y,
                 gboolean       copy_pixels)
{
  LigmaBuffer *ligma_buffer;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  ligma_buffer = g_object_new (LIGMA_TYPE_BUFFER,
                              "name", name,
                              NULL);

  if (copy_pixels)
    ligma_buffer->buffer = ligma_gegl_buffer_dup (buffer);
  else
    ligma_buffer->buffer = g_object_ref (buffer);

  ligma_buffer->offset_x = offset_x;
  ligma_buffer->offset_y = offset_y;

  return ligma_buffer;
}

LigmaBuffer *
ligma_buffer_new_from_pixbuf (GdkPixbuf   *pixbuf,
                             const gchar *name,
                             gint         offset_x,
                             gint         offset_y)
{
  LigmaBuffer       *ligma_buffer;
  GeglBuffer       *buffer;
  guint8           *icc_data;
  gsize             icc_len;
  LigmaColorProfile *profile = NULL;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  buffer = ligma_pixbuf_create_buffer (pixbuf);

  ligma_buffer = ligma_buffer_new (buffer, name,
                                 offset_x, offset_y, FALSE);

  icc_data = ligma_pixbuf_get_icc_profile (pixbuf, &icc_len);
  if (icc_data)
    {
      profile = ligma_color_profile_new_from_icc_profile (icc_data, icc_len,
                                                         NULL);
      g_free (icc_data);
    }

  if (! profile && gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB)
    {
      profile = ligma_color_profile_new_rgb_srgb ();
    }

  if (profile)
    {
      ligma_buffer_set_color_profile (ligma_buffer, profile);
      g_object_unref (profile);
    }

  g_object_unref (buffer);

  return ligma_buffer;
}

gint
ligma_buffer_get_width (LigmaBuffer *buffer)
{
  g_return_val_if_fail (LIGMA_IS_BUFFER (buffer), 0);

  return gegl_buffer_get_width (buffer->buffer);
}

gint
ligma_buffer_get_height (LigmaBuffer *buffer)
{
  g_return_val_if_fail (LIGMA_IS_BUFFER (buffer), 0);

  return gegl_buffer_get_height (buffer->buffer);
}

const Babl *
ligma_buffer_get_format (LigmaBuffer *buffer)
{
  g_return_val_if_fail (LIGMA_IS_BUFFER (buffer), NULL);

  return gegl_buffer_get_format (buffer->buffer);
}

GeglBuffer *
ligma_buffer_get_buffer (LigmaBuffer *buffer)
{
  g_return_val_if_fail (LIGMA_IS_BUFFER (buffer), NULL);

  return buffer->buffer;
}

void
ligma_buffer_set_resolution (LigmaBuffer *buffer,
                            gdouble     resolution_x,
                            gdouble     resolution_y)
{
  g_return_if_fail (LIGMA_IS_BUFFER (buffer));
  g_return_if_fail (resolution_x >= 0.0 && resolution_x <= LIGMA_MAX_RESOLUTION);
  g_return_if_fail (resolution_y >= 0.0 && resolution_y <= LIGMA_MAX_RESOLUTION);

  buffer->resolution_x = resolution_x;
  buffer->resolution_y = resolution_y;
}

gboolean
ligma_buffer_get_resolution (LigmaBuffer *buffer,
                            gdouble    *resolution_x,
                            gdouble    *resolution_y)
{
  g_return_val_if_fail (LIGMA_IS_BUFFER (buffer), FALSE);

  if (buffer->resolution_x > 0.0 &&
      buffer->resolution_y > 0.0)
    {
      if (resolution_x) *resolution_x = buffer->resolution_x;
      if (resolution_y) *resolution_y = buffer->resolution_y;

      return TRUE;
    }

  return FALSE;
}

void
ligma_buffer_set_unit (LigmaBuffer *buffer,
                      LigmaUnit    unit)
{
  g_return_if_fail (LIGMA_IS_BUFFER (buffer));
  g_return_if_fail (unit > LIGMA_UNIT_PIXEL);

  buffer->unit = unit;
}

LigmaUnit
ligma_buffer_get_unit (LigmaBuffer *buffer)
{
  g_return_val_if_fail (LIGMA_IS_BUFFER (buffer), LIGMA_UNIT_PIXEL);

  return buffer->unit;
}

void
ligma_buffer_set_color_profile (LigmaBuffer       *buffer,
                               LigmaColorProfile *profile)
{
  g_return_if_fail (LIGMA_IS_BUFFER (buffer));
  g_return_if_fail (profile == NULL || LIGMA_IS_COLOR_PROFILE (profile));

  if (profile != buffer->color_profile)
    {
      g_set_object (&buffer->color_profile, profile);
    }

  g_clear_object (&buffer->format_profile);
}

LigmaColorProfile *
ligma_buffer_get_color_profile (LigmaBuffer *buffer)
{
  g_return_val_if_fail (LIGMA_IS_BUFFER (buffer), NULL);

  return buffer->color_profile;
}
