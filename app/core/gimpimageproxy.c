/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaimageproxy.c
 * Copyright (C) 2019 Ell
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
#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-loops.h"

#include "ligmaimage.h"
#include "ligmaimage-color-profile.h"
#include "ligmaimage-preview.h"
#include "ligmaimageproxy.h"
#include "ligmapickable.h"
#include "ligmaprojectable.h"
#include "ligmatempbuf.h"


enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_SHOW_ALL,
  PROP_BUFFER
};


struct _LigmaImageProxyPrivate
{
  LigmaImage     *image;
  gboolean       show_all;

  GeglRectangle  bounding_box;
  gboolean       frozen;
};


/*  local function prototypes  */

static void               ligma_image_proxy_pickable_iface_init      (LigmaPickableInterface      *iface);
static void               ligma_image_proxy_color_managed_iface_init (LigmaColorManagedInterface  *iface);

static void               ligma_image_proxy_finalize                 (GObject                    *object);
static void               ligma_image_proxy_set_property             (GObject                    *object,
                                                                     guint                       property_id,
                                                                     const GValue               *value,
                                                                     GParamSpec                 *pspec);
static void               ligma_image_proxy_get_property             (GObject                    *object,
                                                                     guint                       property_id,
                                                                     GValue                     *value,
                                                                     GParamSpec                 *pspec);

static gboolean           ligma_image_proxy_get_size                 (LigmaViewable               *viewable,
                                                                     gint                       *width,
                                                                     gint                       *height);
static void               ligma_image_proxy_get_preview_size         (LigmaViewable               *viewable,
                                                                     gint                        size,
                                                                     gboolean                    is_popup,
                                                                     gboolean                    dot_for_dot,
                                                                     gint                       *width,
                                                                     gint                       *height);
static gboolean           ligma_image_proxy_get_popup_size           (LigmaViewable               *viewable,
                                                                     gint                        width,
                                                                     gint                        height,
                                                                     gboolean                    dot_for_dot,
                                                                     gint                       *popup_width,
                                                                     gint                       *popup_height);
static LigmaTempBuf      * ligma_image_proxy_get_new_preview          (LigmaViewable               *viewable,
                                                                     LigmaContext                *context,
                                                                     gint                        width,
                                                                     gint                        height);
static GdkPixbuf        * ligma_image_proxy_get_new_pixbuf           (LigmaViewable               *viewable,
                                                                     LigmaContext                *context,
                                                                     gint                        width,
                                                                     gint                        height);
static gchar            * ligma_image_proxy_get_description          (LigmaViewable               *viewable,
                                                                     gchar                     **tooltip);

static void               ligma_image_proxy_flush                    (LigmaPickable               *pickable);
static const Babl       * ligma_image_proxy_get_format               (LigmaPickable               *pickable);
static const Babl       * ligma_image_proxy_get_format_with_alpha    (LigmaPickable               *pickable);
static GeglBuffer       * ligma_image_proxy_get_buffer               (LigmaPickable               *pickable);
static gboolean           ligma_image_proxy_get_pixel_at             (LigmaPickable               *pickable,
                                                                     gint                        x,
                                                                     gint                        y,
                                                                     const Babl                 *format,
                                                                     gpointer                    pixel);
static gdouble            ligma_image_proxy_get_opacity_at           (LigmaPickable               *pickable,
                                                                     gint                        x,
                                                                     gint                        y);
static void               ligma_image_proxy_get_pixel_average        (LigmaPickable               *pickable,
                                                                     const GeglRectangle        *rect,
                                                                     const Babl                 *format,
                                                                     gpointer                    pixel);
static void               ligma_image_proxy_pixel_to_srgb            (LigmaPickable               *pickable,
                                                                     const Babl                 *format,
                                                                     gpointer                    pixel,
                                                                     LigmaRGB                    *color);
static void               ligma_image_proxy_srgb_to_pixel            (LigmaPickable               *pickable,
                                                                     const LigmaRGB              *color,
                                                                     const Babl                 *format,
                                                                     gpointer                    pixel);

static const guint8     * ligma_image_proxy_get_icc_profile          (LigmaColorManaged           *managed,
                                                                     gsize                      *len);
static LigmaColorProfile * ligma_image_proxy_get_color_profile        (LigmaColorManaged           *managed);
static void               ligma_image_proxy_profile_changed          (LigmaColorManaged           *managed);

static void               ligma_image_proxy_image_frozen_notify      (LigmaImage                  *image,
                                                                     const GParamSpec           *pspec,
                                                                     LigmaImageProxy             *image_proxy);
static void               ligma_image_proxy_image_invalidate_preview (LigmaImage                  *image,
                                                                     LigmaImageProxy             *image_proxy);
static void               ligma_image_proxy_image_size_changed       (LigmaImage                  *image,
                                                                     LigmaImageProxy             *image_proxy);
static void               ligma_image_proxy_image_bounds_changed     (LigmaImage                  *image,
                                                                     gint                        old_x,
                                                                     gint                        old_y,
                                                                     LigmaImageProxy             *image_proxy);
static void               ligma_image_proxy_image_profile_changed    (LigmaImage                  *image,
                                                                     LigmaImageProxy             *image_proxy);

static void               ligma_image_proxy_set_image                (LigmaImageProxy             *image_proxy,
                                                                     LigmaImage                  *image);
static LigmaPickable     * ligma_image_proxy_get_pickable             (LigmaImageProxy             *image_proxy);
static void               ligma_image_proxy_update_bounding_box      (LigmaImageProxy             *image_proxy);
static void               ligma_image_proxy_update_frozen            (LigmaImageProxy             *image_proxy);


G_DEFINE_TYPE_WITH_CODE (LigmaImageProxy, ligma_image_proxy, LIGMA_TYPE_VIEWABLE,
                         G_ADD_PRIVATE (LigmaImageProxy)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PICKABLE,
                                                ligma_image_proxy_pickable_iface_init)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_COLOR_MANAGED,
                                                ligma_image_proxy_color_managed_iface_init))

#define parent_class ligma_image_proxy_parent_class


/*  private functions  */


static void
ligma_image_proxy_class_init (LigmaImageProxyClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class = LIGMA_VIEWABLE_CLASS (klass);

  object_class->finalize            = ligma_image_proxy_finalize;
  object_class->set_property        = ligma_image_proxy_set_property;
  object_class->get_property        = ligma_image_proxy_get_property;

  viewable_class->default_icon_name = "ligma-image";
  viewable_class->get_size          = ligma_image_proxy_get_size;
  viewable_class->get_preview_size  = ligma_image_proxy_get_preview_size;
  viewable_class->get_popup_size    = ligma_image_proxy_get_popup_size;
  viewable_class->get_new_preview   = ligma_image_proxy_get_new_preview;
  viewable_class->get_new_pixbuf    = ligma_image_proxy_get_new_pixbuf;
  viewable_class->get_description   = ligma_image_proxy_get_description;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_IMAGE,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SHOW_ALL,
                                   g_param_spec_boolean ("show-all",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_override_property (object_class, PROP_BUFFER, "buffer");
}

static void
ligma_image_proxy_pickable_iface_init (LigmaPickableInterface *iface)
{
  iface->flush                 = ligma_image_proxy_flush;
  iface->get_image             = (gpointer) ligma_image_proxy_get_image;
  iface->get_format            = ligma_image_proxy_get_format;
  iface->get_format_with_alpha = ligma_image_proxy_get_format_with_alpha;
  iface->get_buffer            = ligma_image_proxy_get_buffer;
  iface->get_pixel_at          = ligma_image_proxy_get_pixel_at;
  iface->get_opacity_at        = ligma_image_proxy_get_opacity_at;
  iface->get_pixel_average     = ligma_image_proxy_get_pixel_average;
  iface->pixel_to_srgb         = ligma_image_proxy_pixel_to_srgb;
  iface->srgb_to_pixel         = ligma_image_proxy_srgb_to_pixel;
}

static void
ligma_image_proxy_color_managed_iface_init (LigmaColorManagedInterface *iface)
{
  iface->get_icc_profile   = ligma_image_proxy_get_icc_profile;
  iface->get_color_profile = ligma_image_proxy_get_color_profile;
  iface->profile_changed   = ligma_image_proxy_profile_changed;
}

static void
ligma_image_proxy_init (LigmaImageProxy *image_proxy)
{
  image_proxy->priv = ligma_image_proxy_get_instance_private (image_proxy);
}

static void
ligma_image_proxy_finalize (GObject *object)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (object);

  ligma_image_proxy_set_image (image_proxy, NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_image_proxy_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      ligma_image_proxy_set_image (image_proxy,
                                  g_value_get_object (value));
      break;

    case PROP_SHOW_ALL:
      ligma_image_proxy_set_show_all (image_proxy,
                                     g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_image_proxy_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value,
                          ligma_image_proxy_get_image (image_proxy));
      break;

    case PROP_SHOW_ALL:
      g_value_set_boolean (value,
                           ligma_image_proxy_get_show_all (image_proxy));
      break;

    case PROP_BUFFER:
      g_value_set_object (value,
                          ligma_pickable_get_buffer (
                            LIGMA_PICKABLE (image_proxy)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
ligma_image_proxy_get_size (LigmaViewable *viewable,
                           gint         *width,
                           gint         *height)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (viewable);

  *width  = image_proxy->priv->bounding_box.width;
  *height = image_proxy->priv->bounding_box.height;

  return TRUE;
}

static void
ligma_image_proxy_get_preview_size (LigmaViewable *viewable,
                                   gint          size,
                                   gboolean      is_popup,
                                   gboolean      dot_for_dot,
                                   gint         *width,
                                   gint         *height)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (viewable);
  LigmaImage      *image       = image_proxy->priv->image;
  gdouble         xres;
  gdouble         yres;
  gint            viewable_width;
  gint            viewable_height;

  ligma_image_get_resolution (image, &xres, &yres);

  ligma_viewable_get_size (viewable, &viewable_width, &viewable_height);

  ligma_viewable_calc_preview_size (viewable_width,
                                   viewable_height,
                                   size,
                                   size,
                                   dot_for_dot,
                                   xres,
                                   yres,
                                   width,
                                   height,
                                   NULL);
}

static gboolean
ligma_image_proxy_get_popup_size (LigmaViewable *viewable,
                                 gint          width,
                                 gint          height,
                                 gboolean      dot_for_dot,
                                 gint         *popup_width,
                                 gint         *popup_height)
{
  gint viewable_width;
  gint viewable_height;

  ligma_viewable_get_size (viewable, &viewable_width, &viewable_height);

  if (viewable_width > width || viewable_height > height)
    {
      gboolean scaling_up;

      ligma_viewable_calc_preview_size (viewable_width,
                                       viewable_height,
                                       width  * 2,
                                       height * 2,
                                       dot_for_dot, 1.0, 1.0,
                                       popup_width,
                                       popup_height,
                                       &scaling_up);

      if (scaling_up)
        {
          *popup_width  = viewable_width;
          *popup_height = viewable_height;
        }

      return TRUE;
    }

  return FALSE;
}

static LigmaTempBuf *
ligma_image_proxy_get_new_preview (LigmaViewable *viewable,
                                  LigmaContext  *context,
                                  gint          width,
                                  gint          height)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (viewable);
  LigmaImage      *image       = image_proxy->priv->image;
  LigmaPickable   *pickable;
  const Babl     *format;
  GeglRectangle   bounding_box;
  LigmaTempBuf    *buf;
  gdouble         scale_x;
  gdouble         scale_y;
  gdouble         scale;

  pickable     = ligma_image_proxy_get_pickable     (image_proxy);
  bounding_box = ligma_image_proxy_get_bounding_box (image_proxy);

  scale_x = (gdouble) width  / (gdouble) bounding_box.width;
  scale_y = (gdouble) height / (gdouble) bounding_box.height;

  scale   = MIN (scale_x, scale_y);

  format = ligma_image_get_preview_format (image);

  buf = ligma_temp_buf_new (width, height, format);

  gegl_buffer_get (ligma_pickable_get_buffer (pickable),
                   GEGL_RECTANGLE (bounding_box.x * scale,
                                   bounding_box.y * scale,
                                   width,
                                   height),
                   scale,
                   ligma_temp_buf_get_format (buf),
                   ligma_temp_buf_get_data (buf),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  return buf;
}

static GdkPixbuf *
ligma_image_proxy_get_new_pixbuf (LigmaViewable *viewable,
                                 LigmaContext  *context,
                                 gint          width,
                                 gint          height)
{
  LigmaImageProxy     *image_proxy = LIGMA_IMAGE_PROXY (viewable);
  LigmaImage          *image       = image_proxy->priv->image;
  LigmaPickable       *pickable;
  GeglRectangle       bounding_box;
  GdkPixbuf          *pixbuf;
  gdouble             scale_x;
  gdouble             scale_y;
  gdouble             scale;
  LigmaColorTransform *transform;

  pickable     = ligma_image_proxy_get_pickable     (image_proxy);
  bounding_box = ligma_image_proxy_get_bounding_box (image_proxy);

  scale_x = (gdouble) width  / (gdouble) bounding_box.width;
  scale_y = (gdouble) height / (gdouble) bounding_box.height;

  scale   = MIN (scale_x, scale_y);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           width, height);

  transform = ligma_image_get_color_transform_to_srgb_u8 (image);

  if (transform)
    {
      LigmaTempBuf *temp_buf;
      GeglBuffer  *src_buf;
      GeglBuffer  *dest_buf;

      temp_buf = ligma_temp_buf_new (width, height,
                                    ligma_pickable_get_format (pickable));

      gegl_buffer_get (ligma_pickable_get_buffer (pickable),
                       GEGL_RECTANGLE (bounding_box.x * scale,
                                       bounding_box.y * scale,
                                       width,
                                       height),
                       scale,
                       ligma_temp_buf_get_format (temp_buf),
                       ligma_temp_buf_get_data (temp_buf),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      src_buf  = ligma_temp_buf_create_buffer (temp_buf);
      dest_buf = ligma_pixbuf_create_buffer (pixbuf);

      ligma_temp_buf_unref (temp_buf);

      ligma_color_transform_process_buffer (transform,
                                           src_buf,
                                           GEGL_RECTANGLE (0, 0,
                                                           width, height),
                                           dest_buf,
                                           GEGL_RECTANGLE (0, 0, 0, 0));

      g_object_unref (src_buf);
      g_object_unref (dest_buf);
    }
  else
    {
      gegl_buffer_get (ligma_pickable_get_buffer (pickable),
                       GEGL_RECTANGLE (bounding_box.x * scale,
                                       bounding_box.y * scale,
                                       width,
                                       height),
                       scale,
                       ligma_pixbuf_get_format (pixbuf),
                       gdk_pixbuf_get_pixels (pixbuf),
                       gdk_pixbuf_get_rowstride (pixbuf),
                       GEGL_ABYSS_CLAMP);
    }

  return pixbuf;
}

static gchar *
ligma_image_proxy_get_description (LigmaViewable  *viewable,
                                  gchar        **tooltip)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (viewable);
  LigmaImage      *image       = image_proxy->priv->image;

  if (tooltip)
    *tooltip = g_strdup (ligma_image_get_display_path (image));

  return g_strdup_printf ("%s-%d",
                          ligma_image_get_display_name (image),
                          ligma_image_get_id (image));
}

static void
ligma_image_proxy_flush (LigmaPickable *pickable)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (pickable);
  LigmaPickable   *proxy_pickable;

  proxy_pickable = ligma_image_proxy_get_pickable (image_proxy);

  ligma_pickable_flush (proxy_pickable);
}

static const Babl *
ligma_image_proxy_get_format (LigmaPickable *pickable)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (pickable);
  LigmaPickable   *proxy_pickable;

  proxy_pickable = ligma_image_proxy_get_pickable (image_proxy);

  return ligma_pickable_get_format (proxy_pickable);
}

static const Babl *
ligma_image_proxy_get_format_with_alpha (LigmaPickable *pickable)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (pickable);
  LigmaPickable   *proxy_pickable;

  proxy_pickable = ligma_image_proxy_get_pickable (image_proxy);

  return ligma_pickable_get_format_with_alpha (proxy_pickable);
}

static GeglBuffer *
ligma_image_proxy_get_buffer (LigmaPickable *pickable)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (pickable);
  LigmaPickable   *proxy_pickable;

  proxy_pickable = ligma_image_proxy_get_pickable (image_proxy);

  return ligma_pickable_get_buffer (proxy_pickable);
}

static gboolean
ligma_image_proxy_get_pixel_at (LigmaPickable *pickable,
                               gint          x,
                               gint          y,
                               const Babl   *format,
                               gpointer      pixel)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (pickable);
  LigmaPickable   *proxy_pickable;

  proxy_pickable = ligma_image_proxy_get_pickable (image_proxy);

  return ligma_pickable_get_pixel_at (proxy_pickable, x, y, format, pixel);
}

static gdouble
ligma_image_proxy_get_opacity_at (LigmaPickable *pickable,
                                 gint          x,
                                 gint          y)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (pickable);
  LigmaPickable   *proxy_pickable;

  proxy_pickable = ligma_image_proxy_get_pickable (image_proxy);

  return ligma_pickable_get_opacity_at (proxy_pickable, x, y);
}

static void
ligma_image_proxy_get_pixel_average (LigmaPickable        *pickable,
                                    const GeglRectangle *rect,
                                    const Babl          *format,
                                    gpointer             pixel)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (pickable);
  LigmaPickable   *proxy_pickable;

  proxy_pickable = ligma_image_proxy_get_pickable (image_proxy);

  ligma_pickable_get_pixel_average (proxy_pickable, rect, format, pixel);
}

static void
ligma_image_proxy_pixel_to_srgb (LigmaPickable *pickable,
                                const Babl   *format,
                                gpointer      pixel,
                                LigmaRGB      *color)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (pickable);
  LigmaPickable   *proxy_pickable;

  proxy_pickable = ligma_image_proxy_get_pickable (image_proxy);

  ligma_pickable_pixel_to_srgb (proxy_pickable, format, pixel, color);
}

static void
ligma_image_proxy_srgb_to_pixel (LigmaPickable  *pickable,
                                const LigmaRGB *color,
                                const Babl    *format,
                                gpointer       pixel)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (pickable);
  LigmaPickable   *proxy_pickable;

  proxy_pickable = ligma_image_proxy_get_pickable (image_proxy);

  ligma_pickable_srgb_to_pixel (proxy_pickable, color, format, pixel);
}

static const guint8 *
ligma_image_proxy_get_icc_profile (LigmaColorManaged *managed,
                                  gsize            *len)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (managed);

  return ligma_color_managed_get_icc_profile (
    LIGMA_COLOR_MANAGED (image_proxy->priv->image),
    len);
}

static LigmaColorProfile *
ligma_image_proxy_get_color_profile (LigmaColorManaged *managed)
{
  LigmaImageProxy *image_proxy = LIGMA_IMAGE_PROXY (managed);

  return ligma_color_managed_get_color_profile (
    LIGMA_COLOR_MANAGED (image_proxy->priv->image));
}

static void
ligma_image_proxy_profile_changed (LigmaColorManaged *managed)
{
  ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (managed));
}

static void
ligma_image_proxy_image_frozen_notify (LigmaImage        *image,
                                      const GParamSpec *pspec,
                                      LigmaImageProxy   *image_proxy)
{
  ligma_image_proxy_update_frozen (image_proxy);
}

static void
ligma_image_proxy_image_invalidate_preview (LigmaImage      *image,
                                           LigmaImageProxy *image_proxy)
{
  ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (image_proxy));
}

static void
ligma_image_proxy_image_size_changed (LigmaImage      *image,
                                     LigmaImageProxy *image_proxy)
{
  ligma_image_proxy_update_bounding_box (image_proxy);
}

static void
ligma_image_proxy_image_bounds_changed (LigmaImage      *image,
                                       gint            old_x,
                                       gint            old_y,
                                       LigmaImageProxy *image_proxy)
{
  ligma_image_proxy_update_bounding_box (image_proxy);
}

static void
ligma_image_proxy_image_profile_changed (LigmaImage      *image,
                                        LigmaImageProxy *image_proxy)
{
  ligma_color_managed_profile_changed (LIGMA_COLOR_MANAGED (image_proxy));
}

static void
ligma_image_proxy_set_image (LigmaImageProxy *image_proxy,
                            LigmaImage      *image)
{
  if (image_proxy->priv->image)
    {
      g_signal_handlers_disconnect_by_func (
        image_proxy->priv->image,
        ligma_image_proxy_image_frozen_notify,
        image_proxy);
      g_signal_handlers_disconnect_by_func (
        image_proxy->priv->image,
        ligma_image_proxy_image_invalidate_preview,
        image_proxy);
      g_signal_handlers_disconnect_by_func (
        image_proxy->priv->image,
        ligma_image_proxy_image_size_changed,
        image_proxy);
      g_signal_handlers_disconnect_by_func (
        image_proxy->priv->image,
        ligma_image_proxy_image_bounds_changed,
        image_proxy);
      g_signal_handlers_disconnect_by_func (
        image_proxy->priv->image,
        ligma_image_proxy_image_profile_changed,
        image_proxy);

      g_object_unref (image_proxy->priv->image);
    }

  image_proxy->priv->image = image;

  if (image_proxy->priv->image)
    {
      g_object_ref (image_proxy->priv->image);

      g_signal_connect (
        image_proxy->priv->image, "notify::frozen",
        G_CALLBACK (ligma_image_proxy_image_frozen_notify),
        image_proxy);
      g_signal_connect (
        image_proxy->priv->image, "invalidate-preview",
        G_CALLBACK (ligma_image_proxy_image_invalidate_preview),
        image_proxy);
      g_signal_connect (
        image_proxy->priv->image, "size-changed",
        G_CALLBACK (ligma_image_proxy_image_size_changed),
        image_proxy);
      g_signal_connect (
        image_proxy->priv->image, "bounds-changed",
        G_CALLBACK (ligma_image_proxy_image_bounds_changed),
        image_proxy);
      g_signal_connect (
        image_proxy->priv->image, "profile-changed",
        G_CALLBACK (ligma_image_proxy_image_profile_changed),
        image_proxy);

      ligma_image_proxy_update_bounding_box (image_proxy);
      ligma_image_proxy_update_frozen (image_proxy);

      ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (image_proxy));
    }
}

static LigmaPickable *
ligma_image_proxy_get_pickable (LigmaImageProxy *image_proxy)
{
  LigmaImage *image = image_proxy->priv->image;

  if (! image_proxy->priv->show_all)
    return LIGMA_PICKABLE (image);
  else
    return LIGMA_PICKABLE (ligma_image_get_projection (image));
}

static void
ligma_image_proxy_update_bounding_box (LigmaImageProxy *image_proxy)
{
  LigmaImage     *image = image_proxy->priv->image;
  GeglRectangle  bounding_box;

  if (ligma_viewable_preview_is_frozen (LIGMA_VIEWABLE (image_proxy)))
    return;

  if (! image_proxy->priv->show_all)
    {
      bounding_box.x      = 0;
      bounding_box.y      = 0;
      bounding_box.width  = ligma_image_get_width  (image);
      bounding_box.height = ligma_image_get_height (image);
    }
  else
    {
      bounding_box = ligma_projectable_get_bounding_box (
        LIGMA_PROJECTABLE (image));
    }

  if (! gegl_rectangle_equal (&bounding_box,
                              &image_proxy->priv->bounding_box))
    {
      image_proxy->priv->bounding_box = bounding_box;

      ligma_viewable_size_changed (LIGMA_VIEWABLE (image_proxy));
    }
}

static void
ligma_image_proxy_update_frozen (LigmaImageProxy *image_proxy)
{
  gboolean frozen;

  frozen = ligma_viewable_preview_is_frozen (
    LIGMA_VIEWABLE (image_proxy->priv->image));

  if (frozen != image_proxy->priv->frozen)
    {
      image_proxy->priv->frozen = frozen;

      if (frozen)
        {
          ligma_viewable_preview_freeze (LIGMA_VIEWABLE (image_proxy));
        }
      else
        {
          ligma_viewable_preview_thaw (LIGMA_VIEWABLE (image_proxy));

          ligma_image_proxy_update_bounding_box (image_proxy);
        }
    }
}


/*  public functions  */


LigmaImageProxy *
ligma_image_proxy_new (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return g_object_new (LIGMA_TYPE_IMAGE_PROXY,
                       "image", image,
                       NULL);
}

LigmaImage *
ligma_image_proxy_get_image (LigmaImageProxy *image_proxy)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE_PROXY (image_proxy), NULL);

  return image_proxy->priv->image;
}

void
ligma_image_proxy_set_show_all (LigmaImageProxy *image_proxy,
                               gboolean        show_all)
{
  g_return_if_fail (LIGMA_IS_IMAGE_PROXY (image_proxy));

  if (show_all != image_proxy->priv->show_all)
    {
      image_proxy->priv->show_all = show_all;

      ligma_image_proxy_update_bounding_box (image_proxy);
    }
}

gboolean
ligma_image_proxy_get_show_all (LigmaImageProxy *image_proxy)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE_PROXY (image_proxy), FALSE);

  return image_proxy->priv->show_all;
}

GeglRectangle
ligma_image_proxy_get_bounding_box (LigmaImageProxy *image_proxy)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE_PROXY (image_proxy),
                        *GEGL_RECTANGLE (0, 0, 0, 0));

  return image_proxy->priv->bounding_box;
}
