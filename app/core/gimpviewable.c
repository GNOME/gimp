/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimp-utils.h"
#include "gimpcontext.h"
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


static void    gimp_viewable_config_iface_init (GimpConfigInterface *iface);

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
                                                      GimpContext   *context,
                                                      gint           width,
                                                      gint           height);
static void    gimp_viewable_real_get_preview_size   (GimpViewable  *viewable,
                                                      gint           size,
                                                      gboolean       popup,
                                                      gboolean       dot_for_dot,
                                                      gint          *width,
                                                      gint          *height);
static gboolean gimp_viewable_real_get_popup_size    (GimpViewable  *viewable,
                                                      gint           width,
                                                      gint           height,
                                                      gboolean       dot_for_dot,
                                                      gint          *popup_width,
                                                      gint          *popup_height);
static gchar * gimp_viewable_real_get_description    (GimpViewable  *viewable,
                                                      gchar        **tooltip);
static gboolean gimp_viewable_serialize_property     (GimpConfig    *config,
                                                      guint          property_id,
                                                      const GValue  *value,
                                                      GParamSpec    *pspec,
                                                      GimpConfigWriter *writer);


G_DEFINE_TYPE_WITH_CODE (GimpViewable, gimp_viewable, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_viewable_config_iface_init))

#define parent_class gimp_viewable_parent_class

static guint  viewable_signals[LAST_SIGNAL] = { 0 };
static GQuark quark_preview_temp_buf        = 0;
static GQuark quark_preview_pixbuf          = 0;


static void
gimp_viewable_class_init (GimpViewableClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  quark_preview_temp_buf = g_quark_from_static_string ("viewable-preview-temp-buf");
  quark_preview_pixbuf   = g_quark_from_static_string ("viewable-preview-pixbuf");

  viewable_signals[INVALIDATE_PREVIEW] =
    g_signal_new ("invalidate-preview",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpViewableClass, invalidate_preview),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  viewable_signals[SIZE_CHANGED] =
    g_signal_new ("size-changed",
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
  klass->name_changed_signal     = "name-changed";

  klass->invalidate_preview      = gimp_viewable_real_invalidate_preview;
  klass->size_changed            = NULL;

  klass->get_size                = NULL;
  klass->get_preview_size        = gimp_viewable_real_get_preview_size;
  klass->get_popup_size          = gimp_viewable_real_get_popup_size;
  klass->get_preview             = NULL;
  klass->get_new_preview         = NULL;
  klass->get_pixbuf              = NULL;
  klass->get_new_pixbuf          = gimp_viewable_real_get_new_pixbuf;
  klass->get_description         = gimp_viewable_real_get_description;

  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_STOCK_ID, "stock-id",
                                   NULL, NULL,
                                   GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_viewable_init (GimpViewable *viewable)
{
  viewable->stock_id = NULL;
}

static void
gimp_viewable_config_iface_init (GimpConfigInterface *iface)
{
  iface->serialize_property = gimp_viewable_serialize_property;
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

static gboolean
gimp_viewable_real_get_popup_size (GimpViewable *viewable,
                                   gint          width,
                                   gint          height,
                                   gboolean      dot_for_dot,
                                   gint         *popup_width,
                                   gint         *popup_height)
{
  gint w, h;

  if (gimp_viewable_get_size (viewable, &w, &h))
    {
      if (w > width || h > height)
        {
          *popup_width  = w;
          *popup_height = h;

          return TRUE;
        }
    }

  return FALSE;
}

static GdkPixbuf *
gimp_viewable_real_get_new_pixbuf (GimpViewable *viewable,
                                   GimpContext  *context,
                                   gint          width,
                                   gint          height)
{
  TempBuf   *temp_buf;
  GdkPixbuf *pixbuf = NULL;

  temp_buf = gimp_viewable_get_preview (viewable, context, width, height);

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

/**
 * gimp_viewable_invalidate_preview:
 * @viewable: a viewable object
 *
 * Causes any cached preview to be marked as invalid, so that a new
 * preview will be generated at the next attempt to display one.
 **/
void
gimp_viewable_invalidate_preview (GimpViewable *viewable)
{
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  g_signal_emit (viewable, viewable_signals[INVALIDATE_PREVIEW], 0);
}

/**
 * gimp_viewable_size_changed:
 * @viewable: a viewable object
 *
 * This function sends a signal that is handled at a lower level in the
 * object hierarchy, and provides a mechanism by which objects derived
 * from #GimpViewable can respond to size changes.
 **/
void
gimp_viewable_size_changed (GimpViewable *viewable)
{
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  g_signal_emit (viewable, viewable_signals[SIZE_CHANGED], 0);
}

/**
 * gimp_viewable_calc_preview_size:
 * @aspect_width:   unscaled width of the preview for an item.
 * @aspect_height:  unscaled height of the preview for an item.
 * @width:          maximum available width for scaled preview.
 * @height:         maximum available height for scaled preview.
 * @dot_for_dot:    if #TRUE, ignore any differences in axis resolution.
 * @xresolution:    resolution in the horizontal direction.
 * @yresolution:    resolution in the vertical direction.
 * @return_width:   place to return the calculated preview width.
 * @return_height:  place to return the calculated preview height.
 * @scaling_up:     returns #TRUE here if the calculated preview size
 *                  is larger than the viewable itself.
 *
 * A utility function, for calculating the dimensions of a preview
 * based on the information specified in the arguments.  The arguments
 * @aspect_width and @aspect_height are the dimensions of the unscaled
 * preview.  The arguments @width and @height represent the maximum
 * width and height that the scaled preview must fit into. The
 * preview is scaled to be as large as possible without exceeding
 * these constraints.
 *
 * If @dot_for_dot is #TRUE, and @xresolution and @yresolution are
 * different, then these results are corrected for the difference in
 * resolution on the two axes, so that the requested aspect ratio
 * applies to the appearance of the display rather than to pixel
 * counts.
 **/
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

gboolean
gimp_viewable_get_size (GimpViewable  *viewable,
                        gint          *width,
                        gint          *height)
{
  GimpViewableClass *viewable_class;
  gboolean           retval = FALSE;
  gint               w      = 0;
  gint               h      = 0;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), FALSE);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (viewable_class->get_size)
    retval = viewable_class->get_size (viewable, &w, &h);

  if (width)  *width  = w;
  if (height) *height = h;

  return retval;
}

/**
 * gimp_viewable_get_preview_size:
 * @viewable:    the object for which to calculate the preview size.
 * @size:        requested size for preview.
 * @popup:       %TRUE if the preview is intended for a popup window.
 * @dot_for_dot: If #TRUE, ignore any differences in X and Y resolution.
 * @width:       return location for the the calculated width.
 * @height:      return location for the calculated height.
 *
 * Retrieve the size of a viewable's preview.  By default, this
 * simply returns the value of the @size argument for both the @width
 * and @height, but this can be overridden in objects derived from
 * #GimpViewable.  If either the width or height exceeds
 * #GIMP_VIEWABLE_MAX_PREVIEW_SIZE, they are silently truncated.
 **/
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

/**
 * gimp_viewable_get_popup_size:
 * @viewable:     the object for which to calculate the popup size.
 * @width:        the width of the preview from which the popup will be shown.
 * @height:       the height of the preview from which the popup will be shown.
 * @dot_for_dot:  If #TRUE, ignore any differences in X and Y resolution.
 * @popup_width:  return location for the calculated popup width.
 * @popup_height: return location for the calculated popup height.
 *
 * Calculate the size of a viewable's preview, for use in making a
 * popup. The arguments @width and @height specify the size of the
 * preview from which the popup will be shown.
 *
 * Returns: Whether the viewable wants a popup to be shown. Usually
 *          %TRUE if the passed preview size is smaller than the viewable
 *          size, and %FALSE if the viewable completely fits into the
 *          original preview.
 **/
gboolean
gimp_viewable_get_popup_size (GimpViewable *viewable,
                              gint          width,
                              gint          height,
                              gboolean      dot_for_dot,
                              gint         *popup_width,
                              gint         *popup_height)
{
  gint w, h;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), FALSE);

  if (GIMP_VIEWABLE_GET_CLASS (viewable)->get_popup_size (viewable,
                                                          width, height,
                                                          dot_for_dot,
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

  return FALSE;
}

/**
 * gimp_viewable_get_preview:
 * @viewable: The viewable object to get a preview for.
 * @context:  The context to render the preview for.
 * @width:    desired width for the preview
 * @height:   desired height for the preview
 *
 * Gets a preview for a viewable object, by running through a variety
 * of methods until it finds one that works.  First, if an
 * implementation exists of a "get_preview" method, it is tried, and
 * the result is returned if it is not #NULL.  Second, the function
 * checks to see whether there is a cached preview with the correct
 * dimensions; if so, it is returned.  If neither of these works, then
 * the function looks for an implementation of the "get_new_preview"
 * method, and executes it, caching the result.  If everything fails,
 * #NULL is returned.
 *
 * Returns: A #TempBuf containg the preview image, or #NULL if none can
 *          be found or created.
 **/
TempBuf *
gimp_viewable_get_preview (GimpViewable *viewable,
                           GimpContext  *context,
                           gint          width,
                           gint          height)
{
  GimpViewableClass *viewable_class;
  TempBuf           *temp_buf = NULL;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  if (G_UNLIKELY (context == NULL))
    g_warning ("%s: context is NULL", G_STRFUNC);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (viewable_class->get_preview)
    temp_buf = viewable_class->get_preview (viewable, context, width, height);

  if (temp_buf)
    return temp_buf;

  temp_buf = g_object_get_qdata (G_OBJECT (viewable), quark_preview_temp_buf);

  if (temp_buf                   &&
      temp_buf->width  == width  &&
      temp_buf->height == height)
    return temp_buf;

  temp_buf = NULL;

  if (viewable_class->get_new_preview)
    temp_buf = viewable_class->get_new_preview (viewable, context,
                                                width, height);

  g_object_set_qdata_full (G_OBJECT (viewable), quark_preview_temp_buf,
                           temp_buf,
                           (GDestroyNotify) temp_buf_free);

  return temp_buf;
}

/**
 * gimp_viewable_get_new_preview:
 * @viewable: The viewable object to get a preview for.
 * @width:    desired width for the preview
 * @height:   desired height for the preview
 *
 * Gets a new preview for a viewable object.  Similar to
 * gimp_viewable_get_preview(), except that it tries things in a
 * different order, first looking for a "get_new_preview" method, and
 * then if that fails for a "get_preview" method.  This function does
 * not look for a cached preview.
 *
 * Returns: A #TempBuf containg the preview image, or #NULL if none can
 *          be found or created.
 **/
TempBuf *
gimp_viewable_get_new_preview (GimpViewable *viewable,
                               GimpContext  *context,
                               gint          width,
                               gint          height)
{
  GimpViewableClass *viewable_class;
  TempBuf           *temp_buf = NULL;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  if (G_UNLIKELY (context == NULL))
    g_warning ("%s: context is NULL", G_STRFUNC);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (viewable_class->get_new_preview)
    temp_buf = viewable_class->get_new_preview (viewable, context,
                                                width, height);

  if (temp_buf)
    return temp_buf;

  if (viewable_class->get_preview)
    temp_buf = viewable_class->get_preview (viewable, context,
                                            width, height);

  if (temp_buf)
    return temp_buf_copy (temp_buf, NULL);

  return NULL;
}

/**
 * gimp_viewable_get_dummy_preview:
 * @viewable: viewable object for which to get a dummy preview.
 * @width:    width of the preview.
 * @height:   height of the preview.
 * @bpp:      bytes per pixel for the preview, must be 3 or 4.
 *
 * Creates a dummy preview the fits into the specified dimensions,
 * containing a default "question" symbol.  This function is used to
 * generate a preview in situations where layer previews have been
 * disabled in the current Gimp configuration.
 *
 * Returns: a #TempBuf containing the preview image.
 **/
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

/**
 * gimp_viewable_get_pixbuf:
 * @viewable: The viewable object to get a pixbuf preview for.
 * @context:  The context to render the preview for.
 * @width:    desired width for the preview
 * @height:   desired height for the preview
 *
 * Gets a preview for a viewable object, by running through a variety
 * of methods until it finds one that works.  First, if an
 * implementation exists of a "get_pixbuf" method, it is tried, and
 * the result is returned if it is not #NULL.  Second, the function
 * checks to see whether there is a cached preview with the correct
 * dimensions; if so, it is returned.  If neither of these works, then
 * the function looks for an implementation of the "get_new_pixbuf"
 * method, and executes it, caching the result.  If everything fails,
 * #NULL is returned.
 *
 * Returns: A #GdkPixbuf containing the preview pixbuf, or #NULL if none can
 *          be found or created.
 **/
GdkPixbuf *
gimp_viewable_get_pixbuf (GimpViewable *viewable,
                          GimpContext  *context,
                          gint          width,
                          gint          height)
{
  GimpViewableClass *viewable_class;
  GdkPixbuf         *pixbuf = NULL;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  if (G_UNLIKELY (context == NULL))
    g_warning ("%s: context is NULL", G_STRFUNC);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (viewable_class->get_pixbuf)
    pixbuf = viewable_class->get_pixbuf (viewable, context, width, height);

  if (pixbuf)
    return pixbuf;

  pixbuf = g_object_get_qdata (G_OBJECT (viewable), quark_preview_pixbuf);

  if (pixbuf                                  &&
      gdk_pixbuf_get_width (pixbuf)  == width &&
      gdk_pixbuf_get_height (pixbuf) == height)
    return pixbuf;

  pixbuf = NULL;

  if (viewable_class->get_new_pixbuf)
    pixbuf = viewable_class->get_new_pixbuf (viewable, context, width, height);

  g_object_set_qdata_full (G_OBJECT (viewable), quark_preview_pixbuf,
                           pixbuf,
                           (GDestroyNotify) g_object_unref);

  return pixbuf;
}

/**
 * gimp_viewable_get_new_pixbuf:
 * @viewable: The viewable object to get a new pixbuf preview for.
 * @context:  The context to render the preview for.
 * @width:    desired width for the pixbuf
 * @height:   desired height for the pixbuf
 *
 * Gets a new preview for a viewable object.  Similar to
 * gimp_viewable_get_pixbuf(), except that it tries things in a
 * different order, first looking for a "get_new_pixbuf" method, and
 * then if that fails for a "get_pixbuf" method.  This function does
 * not look for a cached pixbuf.
 *
 * Returns: A #GdkPixbuf containing the preview, or #NULL if none can
 *          be created.
 **/
GdkPixbuf *
gimp_viewable_get_new_pixbuf (GimpViewable *viewable,
                              GimpContext  *context,
                              gint          width,
                              gint          height)
{
  GimpViewableClass *viewable_class;
  GdkPixbuf         *pixbuf = NULL;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  if (G_UNLIKELY (context == NULL))
    g_warning ("%s: context is NULL", G_STRFUNC);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (viewable_class->get_new_pixbuf)
    pixbuf = viewable_class->get_new_pixbuf (viewable, context, width, height);

  if (pixbuf)
    return pixbuf;

  if (viewable_class->get_pixbuf)
    pixbuf = viewable_class->get_pixbuf (viewable, context, width, height);

  if (pixbuf)
    return gdk_pixbuf_copy (pixbuf);

  return NULL;
}

/**
 * gimp_viewable_get_dummy_pixbuf:
 * @viewable: the viewable object for which to create a dummy representation.
 * @width:    maximum permitted width for the pixbuf.
 * @height:   maximum permitted height for the pixbuf.
 * @bpp:      bytes per pixel for the pixbuf, must equal 3 or 4.
 *
 * Creates a pixbuf containing a default "question" symbol, sized to
 * fit into the specified dimensions.  The depth of the pixbuf must be
 * 3 or 4 because #GdkPixbuf does not support grayscale.  This
 * function is used to generate a preview in situations where
 * previewing has been disabled in the current Gimp configuration.
 * [Note: this function is currently unused except internally to
 * #GimpViewable -- consider making it static?]
 *
 * Returns: the created #GdkPixbuf.
 **/
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

/**
 * gimp_viewable_get_description:
 * @viewable: viewable object for which to retrieve a description.
 * @tooltip:  return loaction for an optional tooltip string.
 *
 * Retrieves a string containing a description of the viewable object,
 * By default, it simply returns the name of the object, but this can
 * be overridden by object types that inherit from #GimpViewable.
 *
 * Returns: a copy of the description string.  This should be freed
 *          when it is no longer needed.
 **/
gchar *
gimp_viewable_get_description (GimpViewable  *viewable,
                               gchar        **tooltip)
{
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);

  if (tooltip)
    *tooltip = NULL;

  return GIMP_VIEWABLE_GET_CLASS (viewable)->get_description (viewable,
                                                              tooltip);
}

/**
 * gimp_viewable_get_stock_id:
 * @viewable: viewable object for which to retrieve a stock ID.
 *
 * Gets the current value of the object's stock ID, for use in
 * constructing an iconic representation of the object.
 *
 * Returns: a pointer to the string containing the stock ID.  The
 *          contents must not be altered or freed.
 **/
const gchar *
gimp_viewable_get_stock_id (GimpViewable *viewable)
{
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);

  if (viewable->stock_id)
    return (const gchar *) viewable->stock_id;

  return GIMP_VIEWABLE_GET_CLASS (viewable)->default_stock_id;
}

/**
 * gimp_viewable_set_stock_id:
 * @viewable: viewable object to assign the specified stock ID.
 * @stock_id: string containing a stock identifier.
 *
 * Seta the object's stock ID, for use in constructing iconic smbols
 * of the object.  The contents of @stock_id are copied, so you can
 * free it when you are done with it.
 **/
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
