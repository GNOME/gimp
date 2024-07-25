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
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimp-memsize.h"
#include "gimpbuffer.h"
#include "gimpimage.h"
#include "gimptempbuf.h"


static void          gimp_color_managed_iface_init (GimpColorManagedInterface *iface);

static void          gimp_buffer_finalize          (GObject           *object);

static gint64        gimp_buffer_get_memsize       (GimpObject        *object,
                                                    gint64            *gui_size);

static gboolean      gimp_buffer_get_size          (GimpViewable      *viewable,
                                                    gint              *width,
                                                    gint              *height);
static void          gimp_buffer_get_preview_size  (GimpViewable      *viewable,
                                                    gint               size,
                                                    gboolean           is_popup,
                                                    gboolean           dot_for_dot,
                                                    gint              *popup_width,
                                                    gint              *popup_height);
static gboolean      gimp_buffer_get_popup_size    (GimpViewable      *viewable,
                                                    gint               width,
                                                    gint               height,
                                                    gboolean           dot_for_dot,
                                                    gint              *popup_width,
                                                    gint              *popup_height);
static GimpTempBuf * gimp_buffer_get_new_preview   (GimpViewable      *viewable,
                                                    GimpContext       *context,
                                                    gint               width,
                                                    gint               height);
static GdkPixbuf   * gimp_buffer_get_new_pixbuf    (GimpViewable      *viewable,
                                                    GimpContext       *context,
                                                    gint               width,
                                                    gint               height);
static gchar       * gimp_buffer_get_description   (GimpViewable      *viewable,
                                                    gchar            **tooltip);

static const guint8 *
       gimp_buffer_color_managed_get_icc_profile   (GimpColorManaged  *managed,
                                                    gsize             *len);
static GimpColorProfile *
       gimp_buffer_color_managed_get_color_profile (GimpColorManaged  *managed);
static void
       gimp_buffer_color_managed_profile_changed   (GimpColorManaged  *managed);


G_DEFINE_TYPE_WITH_CODE (GimpBuffer, gimp_buffer, GIMP_TYPE_VIEWABLE,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_COLOR_MANAGED,
                                                gimp_color_managed_iface_init))

#define parent_class gimp_buffer_parent_class


static void
gimp_buffer_class_init (GimpBufferClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);

  object_class->finalize            = gimp_buffer_finalize;

  gimp_object_class->get_memsize    = gimp_buffer_get_memsize;

  viewable_class->default_icon_name = "edit-paste";
  viewable_class->name_editable     = TRUE;
  viewable_class->get_size          = gimp_buffer_get_size;
  viewable_class->get_preview_size  = gimp_buffer_get_preview_size;
  viewable_class->get_popup_size    = gimp_buffer_get_popup_size;
  viewable_class->get_new_preview   = gimp_buffer_get_new_preview;
  viewable_class->get_new_pixbuf    = gimp_buffer_get_new_pixbuf;
  viewable_class->get_description   = gimp_buffer_get_description;
}

static void
gimp_color_managed_iface_init (GimpColorManagedInterface *iface)
{
  iface->get_icc_profile   = gimp_buffer_color_managed_get_icc_profile;
  iface->get_color_profile = gimp_buffer_color_managed_get_color_profile;
  iface->profile_changed   = gimp_buffer_color_managed_profile_changed;
}

static void
gimp_buffer_init (GimpBuffer *buffer)
{
}

static void
gimp_buffer_finalize (GObject *object)
{
  GimpBuffer *buffer = GIMP_BUFFER (object);

  g_clear_object (&buffer->buffer);

  gimp_buffer_set_color_profile (buffer, NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_buffer_get_memsize (GimpObject *object,
                         gint64     *gui_size)
{
  GimpBuffer *buffer  = GIMP_BUFFER (object);
  gint64      memsize = 0;

  memsize += gimp_gegl_buffer_get_memsize (buffer->buffer);
  memsize += gimp_g_object_get_memsize (G_OBJECT (buffer->color_profile));
  memsize += gimp_g_object_get_memsize (G_OBJECT (buffer->format_profile));

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_buffer_get_size (GimpViewable *viewable,
                      gint         *width,
                      gint         *height)
{
  GimpBuffer *buffer = GIMP_BUFFER (viewable);

  *width  = gimp_buffer_get_width (buffer);
  *height = gimp_buffer_get_height (buffer);

  return TRUE;
}

static void
gimp_buffer_get_preview_size (GimpViewable *viewable,
                              gint          size,
                              gboolean      is_popup,
                              gboolean      dot_for_dot,
                              gint         *width,
                              gint         *height)
{
  GimpBuffer *buffer = GIMP_BUFFER (viewable);

  gimp_viewable_calc_preview_size (gimp_buffer_get_width (buffer),
                                   gimp_buffer_get_height (buffer),
                                   size,
                                   size,
                                   dot_for_dot, 1.0, 1.0,
                                   width,
                                   height,
                                   NULL);
}

static gboolean
gimp_buffer_get_popup_size (GimpViewable *viewable,
                            gint          width,
                            gint          height,
                            gboolean      dot_for_dot,
                            gint         *popup_width,
                            gint         *popup_height)
{
  GimpBuffer *buffer;
  gint        buffer_width;
  gint        buffer_height;

  buffer        = GIMP_BUFFER (viewable);
  buffer_width  = gimp_buffer_get_width (buffer);
  buffer_height = gimp_buffer_get_height (buffer);

  if (buffer_width > width || buffer_height > height)
    {
      gboolean scaling_up;

      gimp_viewable_calc_preview_size (buffer_width,
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

static GimpTempBuf *
gimp_buffer_get_new_preview (GimpViewable *viewable,
                             GimpContext  *context,
                             gint          width,
                             gint          height)
{
  GimpBuffer  *buffer = GIMP_BUFFER (viewable);
  const Babl  *format = gimp_buffer_get_format (buffer);
  GimpTempBuf *preview;

  if (babl_format_is_palette (format))
    format = gimp_babl_format (GIMP_RGB,
                               GIMP_PRECISION_U8_NON_LINEAR,
                               babl_format_has_alpha (format),
                               babl_format_get_space (format));
  else
    format = gimp_babl_format (gimp_babl_format_get_base_type (format),
                               gimp_babl_precision (GIMP_COMPONENT_TYPE_U8,
                                                    gimp_babl_format_get_trc (format)),
                               babl_format_has_alpha (format),
                               babl_format_get_space (format));

  preview = gimp_temp_buf_new (width, height, format);

  gegl_buffer_get (buffer->buffer, GEGL_RECTANGLE (0, 0, width, height),
                   MIN ((gdouble) width  / (gdouble) gimp_buffer_get_width (buffer),
                        (gdouble) height / (gdouble) gimp_buffer_get_height (buffer)),
                   format,
                   gimp_temp_buf_get_data (preview),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  return preview;
}

static GdkPixbuf *
gimp_buffer_get_new_pixbuf (GimpViewable *viewable,
                            GimpContext  *context,
                            gint          width,
                            gint          height)
{
  GimpBuffer *buffer = GIMP_BUFFER (viewable);
  GdkPixbuf  *pixbuf;
  gdouble     scale;

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           width, height);

  scale = MIN ((gdouble) width  / (gdouble) gimp_buffer_get_width (buffer),
               (gdouble) height / (gdouble) gimp_buffer_get_height (buffer));

  if (buffer->color_profile)
    {
      GimpColorProfile *srgb_profile;
      GimpTempBuf      *temp_buf;
      GeglBuffer       *src_buf;
      GeglBuffer       *dest_buf;

      srgb_profile = gimp_color_profile_new_rgb_srgb ();

      temp_buf = gimp_temp_buf_new (width, height,
                                    gimp_buffer_get_format (buffer));

      gegl_buffer_get (buffer->buffer,
                       GEGL_RECTANGLE (0, 0, width, height),
                       scale,
                       gimp_temp_buf_get_format (temp_buf),
                       gimp_temp_buf_get_data (temp_buf),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      src_buf  = gimp_temp_buf_create_buffer (temp_buf);
      dest_buf = gimp_pixbuf_create_buffer (pixbuf);

      gimp_temp_buf_unref (temp_buf);

      gimp_gegl_convert_color_profile (src_buf,
                                       GEGL_RECTANGLE (0, 0, width, height),
                                       buffer->color_profile,
                                       dest_buf,
                                       GEGL_RECTANGLE (0, 0, 0, 0),
                                       srgb_profile,
                                       GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
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
                       gimp_pixbuf_get_format (pixbuf),
                       gdk_pixbuf_get_pixels (pixbuf),
                       gdk_pixbuf_get_rowstride (pixbuf),
                       GEGL_ABYSS_CLAMP);
    }

  return pixbuf;
}

static gchar *
gimp_buffer_get_description (GimpViewable  *viewable,
                             gchar        **tooltip)
{
  GimpBuffer *buffer = GIMP_BUFFER (viewable);

  return g_strdup_printf ("%s (%d × %d)",
                          gimp_object_get_name (buffer),
                          gimp_buffer_get_width (buffer),
                          gimp_buffer_get_height (buffer));
}

static const guint8 *
gimp_buffer_color_managed_get_icc_profile (GimpColorManaged *managed,
                                           gsize            *len)
{
  GimpBuffer *buffer = GIMP_BUFFER (managed);

  if (buffer->color_profile)
    return gimp_color_profile_get_icc_profile (buffer->color_profile, len);

  /* creates buffer->format_profile */
  gimp_color_managed_get_color_profile (managed);

  return gimp_color_profile_get_icc_profile (buffer->format_profile, len);
}

static GimpColorProfile *
gimp_buffer_color_managed_get_color_profile (GimpColorManaged *managed)
{
  GimpBuffer *buffer = GIMP_BUFFER (managed);

  if (buffer->color_profile)
    return buffer->color_profile;

  if (! buffer->format_profile)
    buffer->format_profile =
      gimp_babl_format_get_color_profile (gimp_buffer_get_format (buffer));

  return buffer->format_profile;
}

static void
gimp_buffer_color_managed_profile_changed (GimpColorManaged *managed)
{
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (managed));
}


/*  public functions  */

GimpBuffer *
gimp_buffer_new (GeglBuffer    *buffer,
                 const gchar   *name,
                 gint           offset_x,
                 gint           offset_y,
                 gboolean       copy_pixels)
{
  GimpBuffer *gimp_buffer;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  gimp_buffer = g_object_new (GIMP_TYPE_BUFFER,
                              "name", name,
                              NULL);

  if (copy_pixels)
    gimp_buffer->buffer = gimp_gegl_buffer_dup (buffer);
  else
    gimp_buffer->buffer = g_object_ref (buffer);

  gimp_buffer->offset_x = offset_x;
  gimp_buffer->offset_y = offset_y;

  return gimp_buffer;
}

GimpBuffer *
gimp_buffer_new_from_pixbuf (GdkPixbuf   *pixbuf,
                             const gchar *name,
                             gint         offset_x,
                             gint         offset_y)
{
  GimpBuffer       *gimp_buffer;
  GeglBuffer       *buffer;
  guint8           *icc_data;
  gsize             icc_len;
  GimpColorProfile *profile = NULL;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  buffer = gimp_pixbuf_create_buffer (pixbuf);

  gimp_buffer = gimp_buffer_new (buffer, name,
                                 offset_x, offset_y, FALSE);

  icc_data = gimp_pixbuf_get_icc_profile (pixbuf, &icc_len);
  if (icc_data)
    {
      profile = gimp_color_profile_new_from_icc_profile (icc_data, icc_len,
                                                         NULL);
      g_free (icc_data);
    }

  if (! profile && gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB)
    {
      profile = gimp_color_profile_new_rgb_srgb ();
    }

  if (profile)
    {
      gimp_buffer_set_color_profile (gimp_buffer, profile);
      g_object_unref (profile);
    }

  g_object_unref (buffer);

  return gimp_buffer;
}

gint
gimp_buffer_get_width (GimpBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), 0);

  return gegl_buffer_get_width (buffer->buffer);
}

gint
gimp_buffer_get_height (GimpBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), 0);

  return gegl_buffer_get_height (buffer->buffer);
}

const Babl *
gimp_buffer_get_format (GimpBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), NULL);

  return gegl_buffer_get_format (buffer->buffer);
}

GeglBuffer *
gimp_buffer_get_buffer (GimpBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), NULL);

  return buffer->buffer;
}

void
gimp_buffer_set_resolution (GimpBuffer *buffer,
                            gdouble     resolution_x,
                            gdouble     resolution_y)
{
  g_return_if_fail (GIMP_IS_BUFFER (buffer));
  g_return_if_fail (resolution_x >= 0.0 && resolution_x <= GIMP_MAX_RESOLUTION);
  g_return_if_fail (resolution_y >= 0.0 && resolution_y <= GIMP_MAX_RESOLUTION);

  buffer->resolution_x = resolution_x;
  buffer->resolution_y = resolution_y;
}

gboolean
gimp_buffer_get_resolution (GimpBuffer *buffer,
                            gdouble    *resolution_x,
                            gdouble    *resolution_y)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), FALSE);

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
gimp_buffer_set_unit (GimpBuffer *buffer,
                      GimpUnit   *unit)
{
  g_return_if_fail (GIMP_IS_BUFFER (buffer));
  g_return_if_fail (GIMP_IS_UNIT (unit));

  buffer->unit = unit;
}

GimpUnit *
gimp_buffer_get_unit (GimpBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), gimp_unit_pixel ());

  return buffer->unit;
}

void
gimp_buffer_set_color_profile (GimpBuffer       *buffer,
                               GimpColorProfile *profile)
{
  g_return_if_fail (GIMP_IS_BUFFER (buffer));
  g_return_if_fail (profile == NULL || GIMP_IS_COLOR_PROFILE (profile));

  if (profile != buffer->color_profile)
    {
      g_set_object (&buffer->color_profile, profile);
    }

  g_clear_object (&buffer->format_profile);
}

GimpColorProfile *
gimp_buffer_get_color_profile (GimpBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), NULL);

  return buffer->color_profile;
}
