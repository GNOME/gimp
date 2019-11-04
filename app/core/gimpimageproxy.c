/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimageproxy.c
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

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"

#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimpimage-preview.h"
#include "gimpimageproxy.h"
#include "gimppickable.h"
#include "gimpprojectable.h"
#include "gimptempbuf.h"


enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_SHOW_ALL,
  PROP_BUFFER
};


struct _GimpImageProxyPrivate
{
  GimpImage     *image;
  gboolean       show_all;

  GeglRectangle  bounding_box;
  gboolean       frozen;
};


/*  local function prototypes  */

static void           gimp_image_proxy_pickable_iface_init      (GimpPickableInterface  *iface);
                    
static void           gimp_image_proxy_finalize                 (GObject                *object);
static void           gimp_image_proxy_set_property             (GObject                *object,
                                                                 guint                   property_id,
                                                                 const GValue           *value,
                                                                 GParamSpec             *pspec);
static void           gimp_image_proxy_get_property             (GObject                *object,
                                                                 guint                   property_id,
                                                                 GValue                 *value,
                                                                 GParamSpec             *pspec);
                    
static gboolean       gimp_image_proxy_get_size                 (GimpViewable           *viewable,
                                                                 gint                   *width,
                                                                 gint                   *height);
static void           gimp_image_proxy_get_preview_size         (GimpViewable           *viewable,
                                                                 gint                    size,
                                                                 gboolean                is_popup,
                                                                 gboolean                dot_for_dot,
                                                                 gint                   *width,
                                                                 gint                   *height);
static gboolean       gimp_image_proxy_get_popup_size           (GimpViewable           *viewable,
                                                                 gint                    width,
                                                                 gint                    height,
                                                                 gboolean                dot_for_dot,
                                                                 gint                   *popup_width,
                                                                 gint                   *popup_height);
static GimpTempBuf  * gimp_image_proxy_get_new_preview          (GimpViewable           *viewable,
                                                                 GimpContext            *context,
                                                                 gint                    width,
                                                                 gint                    height);
static GdkPixbuf    * gimp_image_proxy_get_new_pixbuf           (GimpViewable           *viewable,
                                                                 GimpContext            *context,
                                                                 gint                    width,
                                                                 gint                    height);
static gchar        * gimp_image_proxy_get_description          (GimpViewable           *viewable,
                                                                 gchar                 **tooltip);
                    
static void           gimp_image_proxy_flush                    (GimpPickable           *pickable);
static const Babl   * gimp_image_proxy_get_format               (GimpPickable           *pickable);
static const Babl   * gimp_image_proxy_get_format_with_alpha    (GimpPickable           *pickable);
static GeglBuffer   * gimp_image_proxy_get_buffer               (GimpPickable           *pickable);
static gboolean       gimp_image_proxy_get_pixel_at             (GimpPickable           *pickable,
                                                                 gint                    x,
                                                                 gint                    y,
                                                                 const Babl             *format,
                                                                 gpointer                pixel);
static gdouble        gimp_image_proxy_get_opacity_at           (GimpPickable           *pickable,
                                                                 gint                    x,
                                                                 gint                    y);
static void           gimp_image_proxy_get_pixel_average        (GimpPickable           *pickable,
                                                                 const GeglRectangle    *rect,
                                                                 const Babl             *format,
                                                                 gpointer                pixel);
static void           gimp_image_proxy_pixel_to_srgb            (GimpPickable           *pickable,
                                                                 const Babl             *format,
                                                                 gpointer                pixel,
                                                                 GimpRGB                *color);
static void           gimp_image_proxy_srgb_to_pixel            (GimpPickable           *pickable,
                                                                 const GimpRGB          *color,
                                                                 const Babl             *format,
                                                                 gpointer                pixel);
                    
static void           gimp_image_proxy_image_frozen_notify      (GimpImage              *image,
                                                                 const GParamSpec       *pspec,
                                                                 GimpImageProxy         *image_proxy);
static void           gimp_image_proxy_image_invalidate_preview (GimpImage              *image,
                                                                 GimpImageProxy         *image_proxy);
static void           gimp_image_proxy_image_size_changed       (GimpImage              *image,
                                                                 GimpImageProxy         *image_proxy);
static void           gimp_image_proxy_image_bounds_changed     (GimpImage              *image,
                                                                 gint                    old_x,
                                                                 gint                    old_y,
                                                                 GimpImageProxy         *image_proxy);
                    
static void           gimp_image_proxy_set_image                (GimpImageProxy         *image_proxy,
                                                                 GimpImage              *image);
static GimpPickable * gimp_image_proxy_get_pickable             (GimpImageProxy         *image_proxy);
static void           gimp_image_proxy_update_bounding_box      (GimpImageProxy         *image_proxy);
static void           gimp_image_proxy_update_frozen            (GimpImageProxy         *image_proxy);


G_DEFINE_TYPE_WITH_CODE (GimpImageProxy, gimp_image_proxy, GIMP_TYPE_VIEWABLE,
                         G_ADD_PRIVATE (GimpImageProxy)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PICKABLE,
                                                gimp_image_proxy_pickable_iface_init))

#define parent_class gimp_image_proxy_parent_class


/*  private functions  */


static void
gimp_image_proxy_class_init (GimpImageProxyClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->finalize            = gimp_image_proxy_finalize;
  object_class->set_property        = gimp_image_proxy_set_property;
  object_class->get_property        = gimp_image_proxy_get_property;

  viewable_class->default_icon_name = "gimp-image";
  viewable_class->get_size          = gimp_image_proxy_get_size;
  viewable_class->get_preview_size  = gimp_image_proxy_get_preview_size;
  viewable_class->get_popup_size    = gimp_image_proxy_get_popup_size;
  viewable_class->get_new_preview   = gimp_image_proxy_get_new_preview;
  viewable_class->get_new_pixbuf    = gimp_image_proxy_get_new_pixbuf;
  viewable_class->get_description   = gimp_image_proxy_get_description;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        NULL, NULL,
                                                        GIMP_TYPE_IMAGE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SHOW_ALL,
                                   g_param_spec_boolean ("show-all",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_override_property (object_class, PROP_BUFFER, "buffer");
}

static void
gimp_image_proxy_pickable_iface_init (GimpPickableInterface *iface)
{
  iface->flush                 = gimp_image_proxy_flush;
  iface->get_image             = (gpointer) gimp_image_proxy_get_image;
  iface->get_format            = gimp_image_proxy_get_format;
  iface->get_format_with_alpha = gimp_image_proxy_get_format_with_alpha;
  iface->get_buffer            = gimp_image_proxy_get_buffer;
  iface->get_pixel_at          = gimp_image_proxy_get_pixel_at;
  iface->get_opacity_at        = gimp_image_proxy_get_opacity_at;
  iface->get_pixel_average     = gimp_image_proxy_get_pixel_average;
  iface->pixel_to_srgb         = gimp_image_proxy_pixel_to_srgb;
  iface->srgb_to_pixel         = gimp_image_proxy_srgb_to_pixel;
}

static void
gimp_image_proxy_init (GimpImageProxy *image_proxy)
{
  image_proxy->priv = gimp_image_proxy_get_instance_private (image_proxy);
}

static void
gimp_image_proxy_finalize (GObject *object)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (object);

  gimp_image_proxy_set_image (image_proxy, NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_image_proxy_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      gimp_image_proxy_set_image (image_proxy,
                                  g_value_get_object (value));
      break;

    case PROP_SHOW_ALL:
      gimp_image_proxy_set_show_all (image_proxy,
                                     g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_proxy_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value,
                          gimp_image_proxy_get_image (image_proxy));
      break;

    case PROP_SHOW_ALL:
      g_value_set_boolean (value,
                           gimp_image_proxy_get_show_all (image_proxy));
      break;

    case PROP_BUFFER:
      g_value_set_object (value,
                          gimp_pickable_get_buffer (
                            GIMP_PICKABLE (image_proxy)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_image_proxy_get_size (GimpViewable *viewable,
                           gint         *width,
                           gint         *height)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (viewable);

  *width  = image_proxy->priv->bounding_box.width;
  *height = image_proxy->priv->bounding_box.height;

  return TRUE;
}

static void
gimp_image_proxy_get_preview_size (GimpViewable *viewable,
                                   gint          size,
                                   gboolean      is_popup,
                                   gboolean      dot_for_dot,
                                   gint         *width,
                                   gint         *height)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (viewable);
  GimpImage      *image       = image_proxy->priv->image;
  gdouble         xres;
  gdouble         yres;
  gint            viewable_width;
  gint            viewable_height;

  gimp_image_get_resolution (image, &xres, &yres);

  gimp_viewable_get_size (viewable, &viewable_width, &viewable_height);

  gimp_viewable_calc_preview_size (viewable_width,
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
gimp_image_proxy_get_popup_size (GimpViewable *viewable,
                                 gint          width,
                                 gint          height,
                                 gboolean      dot_for_dot,
                                 gint         *popup_width,
                                 gint         *popup_height)
{
  gint viewable_width;
  gint viewable_height;

  gimp_viewable_get_size (viewable, &viewable_width, &viewable_height);

  if (viewable_width > width || viewable_height > height)
    {
      gboolean scaling_up;

      gimp_viewable_calc_preview_size (viewable_width,
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

static GimpTempBuf *
gimp_image_proxy_get_new_preview (GimpViewable *viewable,
                                  GimpContext  *context,
                                  gint          width,
                                  gint          height)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (viewable);
  GimpImage      *image       = image_proxy->priv->image;
  GimpPickable   *pickable;
  const Babl     *format;
  GeglRectangle   bounding_box;
  GimpTempBuf    *buf;
  gdouble         scale_x;
  gdouble         scale_y;
  gdouble         scale;

  pickable     = gimp_image_proxy_get_pickable     (image_proxy);
  bounding_box = gimp_image_proxy_get_bounding_box (image_proxy);

  scale_x = (gdouble) width  / (gdouble) bounding_box.width;
  scale_y = (gdouble) height / (gdouble) bounding_box.height;

  scale   = MIN (scale_x, scale_y);

  format = gimp_image_get_preview_format (image);

  buf = gimp_temp_buf_new (width, height, format);

  gegl_buffer_get (gimp_pickable_get_buffer (pickable),
                   GEGL_RECTANGLE (bounding_box.x * scale,
                                   bounding_box.y * scale,
                                   width,
                                   height),
                   scale,
                   gimp_temp_buf_get_format (buf),
                   gimp_temp_buf_get_data (buf),
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  return buf;
}

static GdkPixbuf *
gimp_image_proxy_get_new_pixbuf (GimpViewable *viewable,
                                 GimpContext  *context,
                                 gint          width,
                                 gint          height)
{
  GimpImageProxy     *image_proxy = GIMP_IMAGE_PROXY (viewable);
  GimpImage          *image       = image_proxy->priv->image;
  GimpPickable       *pickable;
  GeglRectangle       bounding_box;
  GdkPixbuf          *pixbuf;
  gdouble             scale_x;
  gdouble             scale_y;
  gdouble             scale;
  GimpColorTransform *transform;

  pickable     = gimp_image_proxy_get_pickable     (image_proxy);
  bounding_box = gimp_image_proxy_get_bounding_box (image_proxy);

  scale_x = (gdouble) width  / (gdouble) bounding_box.width;
  scale_y = (gdouble) height / (gdouble) bounding_box.height;

  scale   = MIN (scale_x, scale_y);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           width, height);

  transform = gimp_image_get_color_transform_to_srgb_u8 (image);

  if (transform)
    {
      GimpTempBuf *temp_buf;
      GeglBuffer  *src_buf;
      GeglBuffer  *dest_buf;

      temp_buf = gimp_temp_buf_new (width, height,
                                    gimp_pickable_get_format (pickable));

      gegl_buffer_get (gimp_pickable_get_buffer (pickable),
                       GEGL_RECTANGLE (bounding_box.x * scale,
                                       bounding_box.y * scale,
                                       width,
                                       height),
                       scale,
                       gimp_temp_buf_get_format (temp_buf),
                       gimp_temp_buf_get_data (temp_buf),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      src_buf  = gimp_temp_buf_create_buffer (temp_buf);
      dest_buf = gimp_pixbuf_create_buffer (pixbuf);

      gimp_temp_buf_unref (temp_buf);

      gimp_color_transform_process_buffer (transform,
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
      gegl_buffer_get (gimp_pickable_get_buffer (pickable),
                       GEGL_RECTANGLE (bounding_box.x * scale,
                                       bounding_box.y * scale,
                                       width,
                                       height),
                       scale,
                       gimp_pixbuf_get_format (pixbuf),
                       gdk_pixbuf_get_pixels (pixbuf),
                       gdk_pixbuf_get_rowstride (pixbuf),
                       GEGL_ABYSS_CLAMP);
    }

  return pixbuf;
}

static gchar *
gimp_image_proxy_get_description (GimpViewable  *viewable,
                                  gchar        **tooltip)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (viewable);
  GimpImage      *image       = image_proxy->priv->image;

  if (tooltip)
    *tooltip = g_strdup (gimp_image_get_display_path (image));

  return g_strdup_printf ("%s-%d",
                          gimp_image_get_display_name (image),
                          gimp_image_get_ID (image));
}

static void
gimp_image_proxy_flush (GimpPickable *pickable)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (pickable);
  GimpPickable   *proxy_pickable;

  proxy_pickable = gimp_image_proxy_get_pickable (image_proxy);

  gimp_pickable_flush (proxy_pickable);
}

static const Babl *
gimp_image_proxy_get_format (GimpPickable *pickable)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (pickable);
  GimpPickable   *proxy_pickable;

  proxy_pickable = gimp_image_proxy_get_pickable (image_proxy);

  return gimp_pickable_get_format (proxy_pickable);
}

static const Babl *
gimp_image_proxy_get_format_with_alpha (GimpPickable *pickable)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (pickable);
  GimpPickable   *proxy_pickable;

  proxy_pickable = gimp_image_proxy_get_pickable (image_proxy);

  return gimp_pickable_get_format_with_alpha (proxy_pickable);
}

static GeglBuffer *
gimp_image_proxy_get_buffer (GimpPickable *pickable)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (pickable);
  GimpPickable   *proxy_pickable;

  proxy_pickable = gimp_image_proxy_get_pickable (image_proxy);

  return gimp_pickable_get_buffer (proxy_pickable);
}

static gboolean
gimp_image_proxy_get_pixel_at (GimpPickable *pickable,
                               gint          x,
                               gint          y,
                               const Babl   *format,
                               gpointer      pixel)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (pickable);
  GimpPickable   *proxy_pickable;

  proxy_pickable = gimp_image_proxy_get_pickable (image_proxy);

  return gimp_pickable_get_pixel_at (proxy_pickable, x, y, format, pixel);
}

static gdouble
gimp_image_proxy_get_opacity_at (GimpPickable *pickable,
                                 gint          x,
                                 gint          y)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (pickable);
  GimpPickable   *proxy_pickable;

  proxy_pickable = gimp_image_proxy_get_pickable (image_proxy);

  return gimp_pickable_get_opacity_at (proxy_pickable, x, y);
}

static void
gimp_image_proxy_get_pixel_average (GimpPickable        *pickable,
                                    const GeglRectangle *rect,
                                    const Babl          *format,
                                    gpointer             pixel)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (pickable);
  GimpPickable   *proxy_pickable;

  proxy_pickable = gimp_image_proxy_get_pickable (image_proxy);

  gimp_pickable_get_pixel_average (proxy_pickable, rect, format, pixel);
}

static void
gimp_image_proxy_pixel_to_srgb (GimpPickable *pickable,
                                const Babl   *format,
                                gpointer      pixel,
                                GimpRGB      *color)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (pickable);
  GimpPickable   *proxy_pickable;

  proxy_pickable = gimp_image_proxy_get_pickable (image_proxy);

  gimp_pickable_pixel_to_srgb (proxy_pickable, format, pixel, color);
}

static void
gimp_image_proxy_srgb_to_pixel (GimpPickable  *pickable,
                                const GimpRGB *color,
                                const Babl    *format,
                                gpointer       pixel)
{
  GimpImageProxy *image_proxy = GIMP_IMAGE_PROXY (pickable);
  GimpPickable   *proxy_pickable;

  proxy_pickable = gimp_image_proxy_get_pickable (image_proxy);

  gimp_pickable_srgb_to_pixel (proxy_pickable, color, format, pixel);
}

static void
gimp_image_proxy_image_frozen_notify (GimpImage        *image,
                                      const GParamSpec *pspec,
                                      GimpImageProxy   *image_proxy)
{
  gimp_image_proxy_update_frozen (image_proxy);
}

static void
gimp_image_proxy_image_invalidate_preview (GimpImage      *image,
                                           GimpImageProxy *image_proxy)
{
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (image_proxy));
}

static void
gimp_image_proxy_image_size_changed (GimpImage      *image,
                                     GimpImageProxy *image_proxy)
{
  gimp_image_proxy_update_bounding_box (image_proxy);
}

static void
gimp_image_proxy_image_bounds_changed (GimpImage      *image,
                                       gint            old_x,
                                       gint            old_y,
                                       GimpImageProxy *image_proxy)
{
  gimp_image_proxy_update_bounding_box (image_proxy);
}

static void
gimp_image_proxy_set_image (GimpImageProxy *image_proxy,
                            GimpImage      *image)
{
  if (image_proxy->priv->image)
    {
      g_signal_handlers_disconnect_by_func (
        image_proxy->priv->image,
        gimp_image_proxy_image_frozen_notify,
        image_proxy);
      g_signal_handlers_disconnect_by_func (
        image_proxy->priv->image,
        gimp_image_proxy_image_invalidate_preview,
        image_proxy);
      g_signal_handlers_disconnect_by_func (
        image_proxy->priv->image,
        gimp_image_proxy_image_size_changed,
        image_proxy);
      g_signal_handlers_disconnect_by_func (
        image_proxy->priv->image,
        gimp_image_proxy_image_bounds_changed,
        image_proxy);

      g_object_unref (image_proxy->priv->image);
    }

  image_proxy->priv->image = image;

  if (image_proxy->priv->image)
    {
      g_object_ref (image_proxy->priv->image);

      g_signal_connect (
        image_proxy->priv->image, "notify::frozen",
        G_CALLBACK (gimp_image_proxy_image_frozen_notify),
        image_proxy);
      g_signal_connect (
        image_proxy->priv->image, "invalidate-preview",
        G_CALLBACK (gimp_image_proxy_image_invalidate_preview),
        image_proxy);
      g_signal_connect (
        image_proxy->priv->image, "size-changed",
        G_CALLBACK (gimp_image_proxy_image_size_changed),
        image_proxy);
      g_signal_connect (
        image_proxy->priv->image, "bounds-changed",
        G_CALLBACK (gimp_image_proxy_image_bounds_changed),
        image_proxy);

      gimp_image_proxy_update_bounding_box (image_proxy);
      gimp_image_proxy_update_frozen (image_proxy);

      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (image_proxy));
    }
}

static GimpPickable *
gimp_image_proxy_get_pickable (GimpImageProxy *image_proxy)
{
  GimpImage *image = image_proxy->priv->image;

  if (! image_proxy->priv->show_all)
    return GIMP_PICKABLE (image);
  else
    return GIMP_PICKABLE (gimp_image_get_projection (image));
}

static void
gimp_image_proxy_update_bounding_box (GimpImageProxy *image_proxy)
{
  GimpImage     *image = image_proxy->priv->image;
  GeglRectangle  bounding_box;

  if (gimp_viewable_preview_is_frozen (GIMP_VIEWABLE (image_proxy)))
    return;

  if (! image_proxy->priv->show_all)
    {
      bounding_box.x      = 0;
      bounding_box.y      = 0;
      bounding_box.width  = gimp_image_get_width  (image);
      bounding_box.height = gimp_image_get_height (image);
    }
  else
    {
      bounding_box = gimp_projectable_get_bounding_box (
        GIMP_PROJECTABLE (image));
    }

  if (! gegl_rectangle_equal (&bounding_box,
                              &image_proxy->priv->bounding_box))
    {
      image_proxy->priv->bounding_box = bounding_box;

      gimp_viewable_size_changed (GIMP_VIEWABLE (image_proxy));
    }
}

static void
gimp_image_proxy_update_frozen (GimpImageProxy *image_proxy)
{
  gboolean frozen;

  frozen = gimp_viewable_preview_is_frozen (
    GIMP_VIEWABLE (image_proxy->priv->image));

  if (frozen != image_proxy->priv->frozen)
    {
      image_proxy->priv->frozen = frozen;

      if (frozen)
        {
          gimp_viewable_preview_freeze (GIMP_VIEWABLE (image_proxy));
        }
      else
        {
          gimp_viewable_preview_thaw (GIMP_VIEWABLE (image_proxy));

          gimp_image_proxy_update_bounding_box (image_proxy);
        }
    }
}


/*  public functions  */


GimpImageProxy *
gimp_image_proxy_new (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return g_object_new (GIMP_TYPE_IMAGE_PROXY,
                       "image", image,
                       NULL);
}

GimpImage *
gimp_image_proxy_get_image (GimpImageProxy *image_proxy)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_PROXY (image_proxy), NULL);

  return image_proxy->priv->image;
}

void
gimp_image_proxy_set_show_all (GimpImageProxy *image_proxy,
                               gboolean        show_all)
{
  g_return_if_fail (GIMP_IS_IMAGE_PROXY (image_proxy));

  if (show_all != image_proxy->priv->show_all)
    {
      image_proxy->priv->show_all = show_all;

      gimp_image_proxy_update_bounding_box (image_proxy);
    }
}

gboolean
gimp_image_proxy_get_show_all (GimpImageProxy *image_proxy)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_PROXY (image_proxy), FALSE);

  return image_proxy->priv->show_all;
}

GeglRectangle
gimp_image_proxy_get_bounding_box (GimpImageProxy *image_proxy)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_PROXY (image_proxy),
                        *GEGL_RECTANGLE (0, 0, 0, 0));

  return image_proxy->priv->bounding_box;
}
