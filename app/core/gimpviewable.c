/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpviewable.h
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

#include <glib-object.h>

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimpmarshal.h"
#include "gimpviewable.h"


enum
{
  INVALIDATE_PREVIEW,
  SIZE_CHANGED,
  LAST_SIGNAL
};


static void    gimp_viewable_class_init         (GimpViewableClass *klass);
static void    gimp_viewable_init               (GimpViewable      *viewable);

static void    gimp_viewable_finalize                (GObject      *object);

static gsize   gimp_viewable_get_memsize             (GimpObject   *object);

static void    gimp_viewable_real_invalidate_preview (GimpViewable *viewable);

static void    gimp_viewable_real_get_preview_size   (GimpViewable *viewable,
                                                      gint          size,
                                                      gboolean      popup,
                                                      gboolean      dot_for_dot,
                                                      gint         *width,
                                                      gint         *height);


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

      viewable_type = g_type_register_static (GIMP_TYPE_OBJECT,
					      "GimpViewable",
					      &viewable_info, 0);
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

  gimp_object_class->get_memsize = gimp_viewable_get_memsize;

  klass->default_stock_id        = "gtk-dialog-question";
  klass->name_changed_signal     = "name_changed";

  klass->invalidate_preview      = gimp_viewable_real_invalidate_preview;
  klass->size_changed            = NULL;

  klass->get_preview_size        = gimp_viewable_real_get_preview_size;
  klass->get_preview             = NULL;
  klass->get_new_preview         = NULL;
}

static void
gimp_viewable_init (GimpViewable *viewable)
{
  viewable->stock_id = NULL;
}

static void
gimp_viewable_finalize (GObject *object)
{
  GimpViewable *viewable;

  viewable = GIMP_VIEWABLE (object);

  if (viewable->stock_id)
    {
      g_free (viewable->stock_id);
      viewable->stock_id = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gsize
gimp_viewable_get_memsize (GimpObject *object)
{
  TempBuf   *temp_buf;
  GdkPixbuf *pixbuf;
  gsize      memsize = 0;

  temp_buf = g_object_get_qdata (G_OBJECT (object), quark_preview_temp_buf);
  pixbuf   = g_object_get_qdata (G_OBJECT (object), quark_preview_pixbuf);

  if (temp_buf)
    {
      memsize += temp_buf_get_memsize (temp_buf);
    }

  if (pixbuf)
    {
      static gsize pixbuf_instance_size = 0;

      if (! pixbuf_instance_size)
        {
          GTypeQuery type_query;

          g_type_query (G_TYPE_FROM_INSTANCE (pixbuf), &type_query);

          pixbuf_instance_size = type_query.instance_size;
        }

      memsize += (pixbuf_instance_size +
                  gdk_pixbuf_get_height (pixbuf) *
                  gdk_pixbuf_get_rowstride (pixbuf));
    }

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object);
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
gimp_viewable_calc_preview_size (GimpViewable *viewable,
                                 gint          aspect_width,
                                 gint          aspect_height,
                                 gint          width,
                                 gint          height,
                                 gboolean      dot_for_dot,
                                 gdouble       xresolution,
                                 gdouble       yresolution,
                                 gint         *return_width,
                                 gint         *return_height,
                                 gboolean     *scaling_up)
{
  gdouble xratio;
  gdouble yratio;

  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  if (aspect_width > aspect_height)
    {
      xratio = yratio = (gdouble) width / (gdouble) aspect_width;
    }
  else
    {
      xratio = yratio = (gdouble) height / (gdouble) aspect_height;
    }

  if (dot_for_dot && xresolution != yresolution)
    {
      yratio *= xresolution / yresolution;
    }

  width  = RINT (xratio * (gdouble) aspect_width);
  height = RINT (yratio * (gdouble) aspect_height);

  if (width  < 1) width  = 1;
  if (height < 1) height = 1;

  if (return_width)  *return_width  = width;
  if (return_height) *return_height = height;
  if (scaling_up)    *scaling_up = (xratio > 1.0) || (yratio > 1.0);
}

void
gimp_viewable_get_preview_size (GimpViewable *viewable,
                                gint          size,
                                gboolean      popup,
                                gboolean      dot_for_dot,
                                gint         *width,
                                gint         *height)
{
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));
  g_return_if_fail (size > 0);
  g_return_if_fail (width != NULL);
  g_return_if_fail (height != NULL);

  GIMP_VIEWABLE_GET_CLASS (viewable)->get_preview_size (viewable, size,
                                                        popup, dot_for_dot,
                                                        width, height);
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

GdkPixbuf *
gimp_viewable_get_preview_pixbuf (GimpViewable *viewable,
                                  gint          width,
                                  gint          height)
{
  GdkPixbuf *pixbuf;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  pixbuf = g_object_get_qdata (G_OBJECT (viewable), quark_preview_pixbuf);

  if (pixbuf                                  &&
      gdk_pixbuf_get_width (pixbuf) ==  width &&
      gdk_pixbuf_get_height (pixbuf) == height)
    {
      return pixbuf;
    }

  pixbuf = gimp_viewable_get_new_preview_pixbuf (viewable, width, height);

  g_object_set_qdata_full (G_OBJECT (viewable), quark_preview_pixbuf,
                           pixbuf,
                           (GDestroyNotify) g_object_unref);

  return pixbuf;
}

GdkPixbuf *
gimp_viewable_get_new_preview_pixbuf (GimpViewable *viewable,
                                      gint          width,
                                      gint          height)
{
  TempBuf   *temp_buf;
  GdkPixbuf *pixbuf    = NULL;

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
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  g_free (viewable->stock_id);
  viewable->stock_id = g_strdup (stock_id);
}
