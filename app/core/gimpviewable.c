/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpviewable.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <glib-object.h>

#include "core-types.h"

#include "base/temp-buf.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-params.h"
#include "config/gimpconfigwriter.h"

#include "gimp-utils.h"
#include "gimpmarshal.h"
#include "gimpviewable.h"

#include "themes/Default/images/gimp-core-pixbufs.h"


enum
{
  PROP_0,
  PROP_STOCK_ID
};

enum
{
  INVALIDATE_PREVIEW,
  SIZE_CHANGED,
  LAST_SIGNAL
};


static void    gimp_viewable_class_init        (GimpViewableClass   *klass);
static void    gimp_viewable_init              (GimpViewable        *viewable);
static void    gimp_viewable_config_iface_init (GimpConfigInterface *config_iface);

static void    gimp_viewable_finalize               (GObject        *object);
static void    gimp_viewable_set_property           (GObject        *object,
                                                     guint           property_id,
                                                     const GValue   *value,
                                                     GParamSpec     *pspec);
static void    gimp_viewable_get_property           (GObject        *object,
                                                     guint           property_id,
                                                     GValue         *value,
                                                     GParamSpec     *pspec);

static gint64  gimp_viewable_get_memsize             (GimpObject    *object,
                                                      gint64        *gui_size);

static void    gimp_viewable_real_invalidate_preview (GimpViewable  *viewable);

static GdkPixbuf * gimp_viewable_real_get_new_pixbuf (GimpViewable  *viewable,
                                                      gint           width,
                                                      gint           height);
static void    gimp_viewable_real_get_preview_size   (GimpViewable  *viewable,
                                                      gint           size,
                                                      gboolean       popup,
                                                      gboolean       dot_for_dot,
                                                      gint          *width,
                                                      gint          *height);
static gchar * gimp_viewable_real_get_description    (GimpViewable  *viewable,
                                                      gchar        **tooltip);
static gboolean gimp_viewable_serialize_property     (GimpConfig    *config,
                                                      guint          property_id,
                                                      const GValue  *value,
                                                      GParamSpec    *pspec,
                                                      GimpConfigWriter *writer);


static guint  viewable_signals[LAST_SIGNAL] = { 0 };

static GimpObjectClass *parent_class = NULL;

static GQuark  quark_preview_temp_buf = 0;
static GQuark  quark_preview_pixbuf   = 0;


GType
gimp_viewable_get_type (void)
{
  static GType viewable_type = 0;

  if (! viewable_type)
    {
      static const GTypeInfo viewable_info =
      {
        sizeof (GimpViewableClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_viewable_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpViewable),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_viewable_init,
      };
      static const GInterfaceInfo config_iface_info =
      {
        (GInterfaceInitFunc) gimp_viewable_config_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      viewable_type = g_type_register_static (GIMP_TYPE_OBJECT,
					      "GimpViewable",
					      &viewable_info, 0);

      g_type_add_interface_static (viewable_type, GIMP_TYPE_CONFIG,
                                   &config_iface_info);
    }

  return viewable_type;
}

static void
gimp_viewable_class_init (GimpViewableClass *klass)
{
  GObjectClass    *object_class;
  GimpObjectClass *gimp_object_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  quark_preview_temp_buf = g_quark_from_static_string ("viewable-preview-temp-buf");
  quark_preview_pixbuf   = g_quark_from_static_string ("viewable-preview-pixbuf");

  viewable_signals[INVALIDATE_PREVIEW] =
    g_signal_new ("invalidate_preview",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpViewableClass, invalidate_preview),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  viewable_signals[SIZE_CHANGED] =
    g_signal_new ("size_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpViewableClass, size_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->finalize         = gimp_viewable_finalize;
  object_class->get_property     = gimp_viewable_get_property;
  object_class->set_property     = gimp_viewable_set_property;

  gimp_object_class->get_memsize = gimp_viewable_get_memsize;

  klass->default_stock_id        = "gimp-question";
  klass->name_changed_signal     = "name_changed";

  klass->invalidate_preview      = gimp_viewable_real_invalidate_preview;
  klass->size_changed            = NULL;

  klass->get_preview_size        = gimp_viewable_real_get_preview_size;
  klass->get_popup_size          = NULL;
  klass->get_preview             = NULL;
  klass->get_new_preview         = NULL;
  klass->get_pixbuf              = NULL;
  klass->get_new_pixbuf          = gimp_viewable_real_get_new_pixbuf;
  klass->get_description         = gimp_viewable_real_get_description;

  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_STOCK_ID, "stock-id",
                                   NULL, NULL, 0);
}

static void
gimp_viewable_init (GimpViewable *viewable)
{
  viewable->stock_id = NULL;
}

static void
gimp_viewable_config_iface_init (GimpConfigInterface *config_iface)
{
  config_iface->serialize_property = gimp_viewable_serialize_property;
}

static void
gimp_viewable_finalize (GObject *object)
{
  GimpViewable *viewable = GIMP_VIEWABLE (object);

  if (viewable->stock_id)
    {
      g_free (viewable->stock_id);
      viewable->stock_id = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_viewable_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_STOCK_ID:
      gimp_viewable_set_stock_id (GIMP_VIEWABLE (object),
                                  g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_viewable_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  switch (property_id)
    {
    case PROP_STOCK_ID:
      g_value_set_string (value,
                          gimp_viewable_get_stock_id (GIMP_VIEWABLE (object)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_viewable_get_memsize (GimpObject *object,
                           gint64     *gui_size)
{
  TempBuf   *temp_buf;
  GdkPixbuf *pixbuf;

  temp_buf = g_object_get_qdata (G_OBJECT (object), quark_preview_temp_buf);
  pixbuf   = g_object_get_qdata (G_OBJECT (object), quark_preview_pixbuf);

  if (temp_buf)
    *gui_size += temp_buf_get_memsize (temp_buf);

  if (pixbuf)
    {
      static gsize pixbuf_instance_size = 0;

      if (! pixbuf_instance_size)
        {
          GTypeQuery type_query;

          g_type_query (G_TYPE_FROM_INSTANCE (pixbuf), &type_query);

          pixbuf_instance_size = type_query.instance_size;
        }

      *gui_size += (pixbuf_instance_size +
                    (gsize) gdk_pixbuf_get_height (pixbuf) *
                            gdk_pixbuf_get_rowstride (pixbuf));
    }

  return GIMP_OBJECT_CLASS (parent_class)->get_memsize (object, gui_size);
}

static void
gimp_viewable_real_invalidate_preview (GimpViewable *viewable)
{
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  g_object_set_qdata (G_OBJECT (viewable), quark_preview_temp_buf, NULL);
  g_object_set_qdata (G_OBJECT (viewable), quark_preview_pixbuf, NULL);
}

static void
gimp_viewable_real_get_preview_size (GimpViewable *viewable,
                                     gint          size,
                                     gboolean      popup,
                                     gboolean      dot_for_dot,
                                     gint         *width,
                                     gint         *height)
{
  *width  = size;
  *height = size;
}

static GdkPixbuf *
gimp_viewable_real_get_new_pixbuf (GimpViewable *viewable,
                                   gint          width,
                                   gint          height)
{
  TempBuf   *temp_buf;
  GdkPixbuf *pixbuf = NULL;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  temp_buf = gimp_viewable_get_preview (viewable, width, height);

  if (temp_buf)
    {
      TempBuf *color_buf = NULL;
      gint     width;
      gint     height;
      gint     bytes;

      bytes  = temp_buf->bytes;
      width  = temp_buf->width;
      height = temp_buf->height;

      if (bytes == 1 || bytes == 2)
        {
          gint color_bytes;

          color_bytes = (bytes == 2) ? 4 : 3;

          color_buf = temp_buf_new (width, height, color_bytes, 0, 0, NULL);
          temp_buf_copy (temp_buf, color_buf);

          temp_buf = color_buf;
          bytes    = color_bytes;
        }

      pixbuf = gdk_pixbuf_new_from_data (g_memdup (temp_buf_data (temp_buf),
                                                   width * height * bytes),
                                         GDK_COLORSPACE_RGB,
                                         (bytes == 4),
                                         8,
                                         width,
                                         height,
                                         width * bytes,
                                         (GdkPixbufDestroyNotify) g_free,
                                         NULL);

      if (color_buf)
        temp_buf_free (color_buf);
    }

  return pixbuf;
}

static gchar *
gimp_viewable_real_get_description (GimpViewable  *viewable,
                                    gchar        **tooltip)
{
  if (tooltip)
    *tooltip = NULL;

  return g_strdup (gimp_object_get_name (GIMP_OBJECT (viewable)));
}

static gboolean
gimp_viewable_serialize_property (GimpConfig       *config,
                                  guint             property_id,
                                  const GValue     *value,
                                  GParamSpec       *pspec,
                                  GimpConfigWriter *writer)
{
  GimpViewable *viewable = GIMP_VIEWABLE (config);

  switch (property_id)
    {
    case PROP_STOCK_ID:
      if (viewable->stock_id)
        {
          gimp_config_writer_open (writer, pspec->name);
          gimp_config_writer_string (writer, viewable->stock_id);
          gimp_config_writer_close (writer);
        }
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

void
gimp_viewable_invalidate_preview (GimpViewable *viewable)
{
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  g_signal_emit (viewable, viewable_signals[INVALIDATE_PREVIEW], 0);
}

void
gimp_viewable_size_changed (GimpViewable *viewable)
{
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  g_signal_emit (viewable, viewable_signals[SIZE_CHANGED], 0);
}

void
gimp_viewable_calc_preview_size (gint       aspect_width,
                                 gint       aspect_height,
                                 gint       width,
                                 gint       height,
                                 gboolean   dot_for_dot,
                                 gdouble    xresolution,
                                 gdouble    yresolution,
                                 gint      *return_width,
                                 gint      *return_height,
                                 gboolean  *scaling_up)
{
  gdouble xratio;
  gdouble yratio;

  if (aspect_width > aspect_height)
    {
      xratio = yratio = (gdouble) width / (gdouble) aspect_width;
    }
  else
    {
      xratio = yratio = (gdouble) height / (gdouble) aspect_height;
    }

  if (! dot_for_dot && xresolution != yresolution)
    {
      yratio *= xresolution / yresolution;
    }

  width  = RINT (xratio * (gdouble) aspect_width);
  height = RINT (yratio * (gdouble) aspect_height);

  if (width  < 1) width  = 1;
  if (height < 1) height = 1;

  if (return_width)  *return_width  = width;
  if (return_height) *return_height = height;
  if (scaling_up)    *scaling_up    = (xratio > 1.0) || (yratio > 1.0);
}

void
gimp_viewable_get_preview_size (GimpViewable *viewable,
                                gint          size,
                                gboolean      popup,
                                gboolean      dot_for_dot,
                                gint         *width,
                                gint         *height)
{
  gint w, h;

  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));
  g_return_if_fail (size > 0);

  GIMP_VIEWABLE_GET_CLASS (viewable)->get_preview_size (viewable, size,
                                                        popup, dot_for_dot,
                                                        &w, &h);

  w = MIN (w, GIMP_VIEWABLE_MAX_PREVIEW_SIZE);
  h = MIN (h, GIMP_VIEWABLE_MAX_PREVIEW_SIZE);

  if (width)  *width  = w;
  if (height) *height = h;

}

gboolean
gimp_viewable_get_popup_size (GimpViewable *viewable,
                              gint          width,
                              gint          height,
                              gboolean      dot_for_dot,
                              gint         *popup_width,
                              gint         *popup_height)
{
  GimpViewableClass *viewable_class;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), FALSE);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (viewable_class->get_popup_size)
    {
      gint w, h;

      if (viewable_class->get_popup_size (viewable,
                                          width, height, dot_for_dot,
                                          &w, &h))
        {
          if (w < 1) w = 1;
          if (h < 1) h = 1;

          /*  limit the popup to 2 * GIMP_VIEWABLE_MAX_POPUP_SIZE
           *  on each axis.
           */
          if ((w > (2 * GIMP_VIEWABLE_MAX_POPUP_SIZE)) ||
              (h > (2 * GIMP_VIEWABLE_MAX_POPUP_SIZE)))
            {
              gimp_viewable_calc_preview_size (w, h,
                                               2 * GIMP_VIEWABLE_MAX_POPUP_SIZE,
                                               2 * GIMP_VIEWABLE_MAX_POPUP_SIZE,
                                               dot_for_dot, 1.0, 1.0,
                                               &w, &h, NULL);
            }

          /*  limit the number of pixels to
           *  GIMP_VIEWABLE_MAX_POPUP_SIZE ^ 2
           */
          if ((w * h) > SQR (GIMP_VIEWABLE_MAX_POPUP_SIZE))
            {
              gdouble factor;

              factor = sqrt (((gdouble) (w * h) /
                              (gdouble) SQR (GIMP_VIEWABLE_MAX_POPUP_SIZE)));

              w = RINT ((gdouble) w / factor);
              h = RINT ((gdouble) h / factor);
            }

          if (w < 1) w = 1;
          if (h < 1) h = 1;

          if (popup_width)  *popup_width  = w;
          if (popup_height) *popup_height = h;

          return TRUE;
        }
    }

  return FALSE;
}

TempBuf *
gimp_viewable_get_preview (GimpViewable *viewable,
			   gint          width,
			   gint          height)
{
  GimpViewableClass *viewable_class;
  TempBuf           *temp_buf = NULL;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (viewable_class->get_preview)
    temp_buf = viewable_class->get_preview (viewable, width, height);

  if (temp_buf)
    return temp_buf;

  temp_buf = g_object_get_qdata (G_OBJECT (viewable), quark_preview_temp_buf);

  if (temp_buf                   &&
      temp_buf->width  == width  &&
      temp_buf->height == height)
    return temp_buf;

  temp_buf = NULL;

  if (viewable_class->get_new_preview)
    temp_buf = viewable_class->get_new_preview (viewable, width, height);

  g_object_set_qdata_full (G_OBJECT (viewable), quark_preview_temp_buf,
                           temp_buf,
                           (GDestroyNotify) temp_buf_free);

  return temp_buf;
}

TempBuf *
gimp_viewable_get_new_preview (GimpViewable *viewable,
			       gint          width,
			       gint          height)
{
  GimpViewableClass *viewable_class;
  TempBuf           *temp_buf = NULL;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (viewable_class->get_new_preview)
    temp_buf = viewable_class->get_new_preview (viewable, width, height);

  if (temp_buf)
    return temp_buf;

  if (viewable_class->get_preview)
    temp_buf = viewable_class->get_preview (viewable, width, height);

  if (temp_buf)
    return temp_buf_copy (temp_buf, NULL);

  return NULL;
}

TempBuf *
gimp_viewable_get_dummy_preview (GimpViewable  *viewable,
                                 gint           width,
                                 gint           height,
                                 gint           bpp)
{
  GdkPixbuf *pixbuf;
  TempBuf   *buf;
  guchar    *src;
  guchar    *dest;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);
  g_return_val_if_fail (bpp == 3 || bpp == 4, NULL);

  pixbuf = gimp_viewable_get_dummy_pixbuf (viewable, width, height, bpp);

  buf = temp_buf_new (width, height, bpp, 0, 0, NULL);

  src  = gdk_pixbuf_get_pixels (pixbuf);
  dest = temp_buf_data (buf);

  while (height--)
    {
      memcpy (dest, src, width * bpp);

      src  += gdk_pixbuf_get_rowstride (pixbuf);
      dest += width * bpp;
    }

  g_object_unref (pixbuf);

  return buf;
}

GdkPixbuf *
gimp_viewable_get_pixbuf (GimpViewable *viewable,
                          gint          width,
                          gint          height)
{
  GimpViewableClass *viewable_class;
  GdkPixbuf         *pixbuf = NULL;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (viewable_class->get_pixbuf)
    pixbuf = viewable_class->get_pixbuf (viewable, width, height);

  if (pixbuf)
    return pixbuf;

  pixbuf = g_object_get_qdata (G_OBJECT (viewable), quark_preview_pixbuf);

  if (pixbuf                                  &&
      gdk_pixbuf_get_width (pixbuf)  == width &&
      gdk_pixbuf_get_height (pixbuf) == height)
    return pixbuf;

  pixbuf = NULL;

  if (viewable_class->get_new_pixbuf)
    pixbuf = viewable_class->get_new_pixbuf (viewable, width, height);

  g_object_set_qdata_full (G_OBJECT (viewable), quark_preview_pixbuf,
                           pixbuf,
                           (GDestroyNotify) g_object_unref);

  return pixbuf;
}

GdkPixbuf *
gimp_viewable_get_new_pixbuf (GimpViewable *viewable,
                              gint          width,
                              gint          height)
{
  GimpViewableClass *viewable_class;
  GdkPixbuf         *pixbuf = NULL;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (viewable_class->get_new_pixbuf)
    pixbuf = viewable_class->get_new_pixbuf (viewable, width, height);

  if (pixbuf)
    return pixbuf;

  if (viewable_class->get_pixbuf)
    pixbuf = viewable_class->get_pixbuf (viewable, width, height);

  if (pixbuf)
    return gdk_pixbuf_copy (pixbuf);

  return NULL;
}

GdkPixbuf *
gimp_viewable_get_dummy_pixbuf (GimpViewable  *viewable,
                                gint           width,
                                gint           height,
                                gint           bpp)
{
  GdkPixbuf *icon;
  GdkPixbuf *pixbuf;
  gdouble    ratio;
  gint       w, h;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);
  g_return_val_if_fail (bpp == 3 || bpp == 4, NULL);

  icon = gdk_pixbuf_new_from_inline (-1, stock_question_64, FALSE, NULL);

  g_return_val_if_fail (icon != NULL, NULL);

  w = gdk_pixbuf_get_width (icon);
  h = gdk_pixbuf_get_height (icon);

  ratio = (gdouble) MIN (width, height) / (gdouble) MAX (w, h);
  ratio = MIN (ratio, 1.0);

  w = RINT (ratio * (gdouble) w);
  h = RINT (ratio * (gdouble) h);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, (bpp == 4), 8, width, height);
  gdk_pixbuf_fill (pixbuf, 0xffffffff);

  if (w && h)
    gdk_pixbuf_composite (icon, pixbuf,
                          (width - w) / 2, (height - h) / 2, w, h,
                          (width - w) / 2, (height - h) / 2, ratio, ratio,
                          GDK_INTERP_BILINEAR, 0xFF);

  g_object_unref (icon);

  return pixbuf;
}

gchar *
gimp_viewable_get_description (GimpViewable  *viewable,
                               gchar        **tooltip)
{
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);

  return GIMP_VIEWABLE_GET_CLASS (viewable)->get_description (viewable,
                                                              tooltip);
}

const gchar *
gimp_viewable_get_stock_id (GimpViewable *viewable)
{
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);

  if (viewable->stock_id)
    return (const gchar *) viewable->stock_id;

  return GIMP_VIEWABLE_GET_CLASS (viewable)->default_stock_id;
}

void
gimp_viewable_set_stock_id (GimpViewable *viewable,
                            const gchar  *stock_id)
{
  GimpViewableClass *viewable_class;

  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  g_free (viewable->stock_id);
  viewable->stock_id = NULL;

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (stock_id)
    {
      if (viewable_class->default_stock_id == NULL ||
          strcmp (stock_id, viewable_class->default_stock_id))
        viewable->stock_id = g_strdup (stock_id);
    }

  g_object_notify (G_OBJECT (viewable), "stock-id");
}
