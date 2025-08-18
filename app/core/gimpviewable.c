/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpviewable.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp-memsize.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimptempbuf.h"
#include "gimpviewable.h"


enum
{
  PROP_0,
  PROP_ICON_NAME,
  PROP_ICON_PIXBUF,
  PROP_FROZEN,
  N_PROPS
};

static GParamSpec *obj_props[N_PROPS] = { NULL, };


enum
{
  INVALIDATE_PREVIEW,
  SIZE_CHANGED,
  EXPANDED_CHANGED,
  ANCESTRY_CHANGED,
  LAST_SIGNAL
};


typedef struct _GimpViewablePrivate GimpViewablePrivate;

struct _GimpViewablePrivate
{
  gchar        *icon_name;
  GdkPixbuf    *icon_pixbuf;
  gint          freeze_count;
  gboolean      invalidate_pending;
  gboolean      size_changed_pending;
  GimpViewable *parent;
  gint          depth;

  GimpTempBuf  *preview_temp_buf;
  GeglColor    *preview_temp_buf_color;

  GdkPixbuf    *preview_pixbuf;
  GeglColor    *preview_pixbuf_color;
};

#define GET_PRIVATE(viewable) \
  ((GimpViewablePrivate *) gimp_viewable_get_instance_private ((GimpViewable *) (viewable)))


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
static void    gimp_viewable_real_ancestry_changed   (GimpViewable  *viewable);

static GdkPixbuf * gimp_viewable_real_get_new_pixbuf (GimpViewable  *viewable,
                                                      GimpContext   *context,
                                                      gint           width,
                                                      gint           height,
                                                      GeglColor     *fg_color);
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
static gboolean gimp_viewable_real_is_name_editable  (GimpViewable  *viewable);
static GimpContainer * gimp_viewable_real_get_children (GimpViewable *viewable);

static gboolean gimp_viewable_serialize_property     (GimpConfig    *config,
                                                      guint          property_id,
                                                      const GValue  *value,
                                                      GParamSpec    *pspec,
                                                      GimpConfigWriter *writer);
static gboolean gimp_viewable_deserialize_property   (GimpConfig       *config,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec,
                                                      GScanner         *scanner,
                                                      GTokenType       *expected);


G_DEFINE_TYPE_WITH_CODE (GimpViewable, gimp_viewable, GIMP_TYPE_OBJECT,
                         G_ADD_PRIVATE (GimpViewable)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_viewable_config_iface_init))

#define parent_class gimp_viewable_parent_class

static guint viewable_signals[LAST_SIGNAL] = { 0 };


static void
gimp_viewable_class_init (GimpViewableClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  viewable_signals[INVALIDATE_PREVIEW] =
    g_signal_new ("invalidate-preview",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpViewableClass, invalidate_preview),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  viewable_signals[SIZE_CHANGED] =
    g_signal_new ("size-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpViewableClass, size_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  viewable_signals[EXPANDED_CHANGED] =
    g_signal_new ("expanded-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpViewableClass, expanded_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  viewable_signals[ANCESTRY_CHANGED] =
    g_signal_new ("ancestry-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpViewableClass, ancestry_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize         = gimp_viewable_finalize;
  object_class->get_property     = gimp_viewable_get_property;
  object_class->set_property     = gimp_viewable_set_property;

  gimp_object_class->get_memsize = gimp_viewable_get_memsize;

  klass->default_icon_name       = "gimp-image";
  klass->default_name            = "EEK: Missing Default Name";
  klass->name_changed_signal     = "name-changed";
  klass->name_editable           = FALSE;

  klass->invalidate_preview      = gimp_viewable_real_invalidate_preview;
  klass->size_changed            = NULL;
  klass->expanded_changed        = NULL;
  klass->ancestry_changed        = gimp_viewable_real_ancestry_changed;

  klass->get_size                = NULL;
  klass->get_preview_size        = gimp_viewable_real_get_preview_size;
  klass->get_popup_size          = gimp_viewable_real_get_popup_size;
  klass->get_preview             = NULL;
  klass->get_new_preview         = NULL;
  klass->get_pixbuf              = NULL;
  klass->get_new_pixbuf          = gimp_viewable_real_get_new_pixbuf;
  klass->get_description         = gimp_viewable_real_get_description;
  klass->is_name_editable        = gimp_viewable_real_is_name_editable;
  klass->preview_freeze          = NULL;
  klass->preview_thaw            = NULL;
  klass->get_children            = gimp_viewable_real_get_children;
  klass->set_expanded            = NULL;
  klass->get_expanded            = NULL;

  obj_props[PROP_ICON_NAME] =
      g_param_spec_string ("icon-name", NULL, NULL,
                           NULL,
                           GIMP_CONFIG_PARAM_FLAGS);

  obj_props[PROP_ICON_PIXBUF] =
      g_param_spec_object ("icon-pixbuf", NULL, NULL,
                           GDK_TYPE_PIXBUF,
                           GIMP_CONFIG_PARAM_FLAGS);

  obj_props[PROP_FROZEN] =
      g_param_spec_boolean ("frozen", NULL, NULL,
                            FALSE,
                            GIMP_PARAM_READABLE);

  g_object_class_install_properties (object_class, N_PROPS, obj_props);
}

static void
gimp_viewable_init (GimpViewable *viewable)
{
}

static void
gimp_viewable_config_iface_init (GimpConfigInterface *iface)
{
  iface->deserialize_property = gimp_viewable_deserialize_property;
  iface->serialize_property   = gimp_viewable_serialize_property;
}

static void
gimp_viewable_finalize (GObject *object)
{
  GimpViewablePrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->icon_name, g_free);
  g_clear_object  (&private->icon_pixbuf);
  g_clear_pointer (&private->preview_temp_buf, gimp_temp_buf_unref);
  g_clear_object  (&private->preview_temp_buf_color);
  g_clear_object  (&private->preview_pixbuf);
  g_clear_object  (&private->preview_pixbuf_color);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_viewable_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpViewable        *viewable = GIMP_VIEWABLE (object);
  GimpViewablePrivate *private  = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ICON_NAME:
      gimp_viewable_set_icon_name (viewable, g_value_get_string (value));
      break;
    case PROP_ICON_PIXBUF:
      g_set_object (&private->icon_pixbuf, g_value_get_object (value));
      gimp_viewable_invalidate_preview (viewable);
      break;
    case PROP_FROZEN:
      /* read-only, fall through */

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
  GimpViewable        *viewable = GIMP_VIEWABLE (object);
  GimpViewablePrivate *private  = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ICON_NAME:
      g_value_set_string (value, gimp_viewable_get_icon_name (viewable));
      break;
    case PROP_ICON_PIXBUF:
      g_value_set_object (value, private->icon_pixbuf);
      break;
    case PROP_FROZEN:
      g_value_set_boolean (value, gimp_viewable_preview_is_frozen (viewable));
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
  GimpViewablePrivate *private = GET_PRIVATE (object);

  *gui_size += gimp_temp_buf_get_memsize (private->preview_temp_buf);

  if (private->preview_pixbuf)
    {
      *gui_size +=
        (gimp_g_object_get_memsize (G_OBJECT (private->preview_pixbuf)) +
         (gsize) gdk_pixbuf_get_height (private->preview_pixbuf) *
         gdk_pixbuf_get_rowstride (private->preview_pixbuf));
    }

  return GIMP_OBJECT_CLASS (parent_class)->get_memsize (object, gui_size);
}

static void
gimp_viewable_real_invalidate_preview (GimpViewable *viewable)
{
  GimpViewablePrivate *private = GET_PRIVATE (viewable);

  g_clear_pointer (&private->preview_temp_buf, gimp_temp_buf_unref);
  g_clear_object  (&private->preview_temp_buf_color);
  g_clear_object  (&private->preview_pixbuf);
  g_clear_object  (&private->preview_pixbuf_color);
}

static void
gimp_viewable_real_ancestry_changed_propagate (GimpViewable *viewable,
                                               GimpViewable *parent)
{
  GimpViewablePrivate *private = GET_PRIVATE (viewable);

  private->depth = gimp_viewable_get_depth (parent) + 1;

  g_signal_emit (viewable, viewable_signals[ANCESTRY_CHANGED], 0);
}

static void
gimp_viewable_real_ancestry_changed (GimpViewable *viewable)
{
  GimpContainer *children;

  children = gimp_viewable_get_children (viewable);

  if (children)
    {
      gimp_container_foreach (children,
                              (GFunc) gimp_viewable_real_ancestry_changed_propagate,
                              viewable);
    }
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
                                   gint          height,
                                   GeglColor    *fg_color)
{
  GimpViewablePrivate *private = GET_PRIVATE (viewable);
  GdkPixbuf           *pixbuf  = NULL;
  GimpTempBuf         *temp_buf;

  temp_buf = gimp_viewable_get_preview (viewable, context,
                                        width, height,
                                        fg_color);

  if (temp_buf)
    {
      pixbuf = gimp_temp_buf_create_pixbuf (temp_buf);
    }
  else if (private->icon_pixbuf)
    {
      pixbuf = gdk_pixbuf_scale_simple (private->icon_pixbuf,
                                        width,
                                        height,
                                        GDK_INTERP_BILINEAR);
    }

  return pixbuf;
}

static gchar *
gimp_viewable_real_get_description (GimpViewable  *viewable,
                                    gchar        **tooltip)
{
  return g_strdup (gimp_object_get_name (viewable));
}

static gboolean
gimp_viewable_real_is_name_editable (GimpViewable *viewable)
{
  return GIMP_VIEWABLE_GET_CLASS (viewable)->name_editable;
}

static GimpContainer *
gimp_viewable_real_get_children (GimpViewable *viewable)
{
  return NULL;
}

static gboolean
gimp_viewable_serialize_property (GimpConfig       *config,
                                  guint             property_id,
                                  const GValue     *value,
                                  GParamSpec       *pspec,
                                  GimpConfigWriter *writer)
{
  GimpViewablePrivate *private = GET_PRIVATE (config);

  switch (property_id)
    {
    case PROP_ICON_NAME:
      if (private->icon_name)
        {
          gimp_config_writer_open (writer, pspec->name);
          gimp_config_writer_string (writer, private->icon_name);
          gimp_config_writer_close (writer);
        }
      return TRUE;

    case PROP_ICON_PIXBUF:
      {
        GdkPixbuf *icon_pixbuf = g_value_get_object (value);

        if (icon_pixbuf)
          {
            gchar  *pixbuffer;
            gsize   pixbuffer_size;
            GError *error = NULL;

            if (gdk_pixbuf_save_to_buffer (icon_pixbuf,
                                           &pixbuffer,
                                           &pixbuffer_size,
                                           "png", &error, NULL))
              {
                gchar *pixbuffer_enc;

                pixbuffer_enc = g_base64_encode ((guchar *)pixbuffer,
                                                 pixbuffer_size);
                gimp_config_writer_open (writer, "icon-pixbuf");
                gimp_config_writer_string (writer, pixbuffer_enc);
                gimp_config_writer_close (writer);

                g_free (pixbuffer_enc);
                g_free (pixbuffer);
              }
          }
      }
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static gboolean
gimp_viewable_deserialize_property (GimpConfig *config,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec,
                                    GScanner   *scanner,
                                    GTokenType *expected)
{
  switch (property_id)
    {
    case PROP_ICON_PIXBUF:
      {
        GdkPixbuf *icon_pixbuf = NULL;
        gchar     *encoded_image;

        if (! gimp_scanner_parse_string (scanner, &encoded_image))
          {
            *expected = G_TOKEN_STRING;
            return TRUE;
          }

        if (encoded_image && strlen (encoded_image) > 0)
          {
            gsize   out_len;
            guchar *decoded_image = g_base64_decode (encoded_image, &out_len);

            if (decoded_image)
              {
                GInputStream *stream;

                stream = g_memory_input_stream_new_from_data (decoded_image,
                                                              out_len, NULL);
                icon_pixbuf = gdk_pixbuf_new_from_stream (stream, NULL, NULL);
                g_object_unref (stream);

                g_free (decoded_image);
              }
          }

        g_free (encoded_image);

        g_value_take_object (value, icon_pixbuf);
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
  GimpViewablePrivate *private = GET_PRIVATE (viewable);

  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  if (private->freeze_count == 0)
    g_signal_emit (viewable, viewable_signals[INVALIDATE_PREVIEW], 0);
  else
    private->invalidate_pending = TRUE;
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
  GimpViewablePrivate *private = GET_PRIVATE (viewable);

  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  if (private->freeze_count == 0)
    g_signal_emit (viewable, viewable_signals[SIZE_CHANGED], 0);
  else
    private->size_changed_pending = TRUE;
}

/**
 * gimp_viewable_expanded_changed:
 * @viewable: a viewable object
 *
 * This function sends a signal that is handled at a lower level in the
 * object hierarchy, and provides a mechanism by which objects derived
 * from #GimpViewable can respond to expanded state changes.
 **/
void
gimp_viewable_expanded_changed (GimpViewable *viewable)
{
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  g_signal_emit (viewable, viewable_signals[EXPANDED_CHANGED], 0);
}

/**
 * gimp_viewable_calc_preview_size:
 * @aspect_width:   unscaled width of the preview for an item.
 * @aspect_height:  unscaled height of the preview for an item.
 * @width:          maximum available width for scaled preview.
 * @height:         maximum available height for scaled preview.
 * @dot_for_dot:    if %TRUE, ignore any differences in axis resolution.
 * @xresolution:    resolution in the horizontal direction.
 * @yresolution:    resolution in the vertical direction.
 * @return_width:   place to return the calculated preview width.
 * @return_height:  place to return the calculated preview height.
 * @scaling_up:     returns %TRUE here if the calculated preview size
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
 * If @dot_for_dot is %TRUE, and @xresolution and @yresolution are
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
gimp_viewable_has_preview (GimpViewable *viewable)
{
  GimpViewableClass *viewable_class;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), FALSE);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  return (viewable_class->get_preview     ||
          viewable_class->get_new_preview ||
          viewable_class->get_pixbuf      ||
          viewable_class->get_new_pixbuf);
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
 * @dot_for_dot: If %TRUE, ignore any differences in X and Y resolution.
 * @width: (out) (optional):  return location for the the calculated width.
 * @height: (out) (optional): return location for the calculated height.
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
 * @dot_for_dot:  If %TRUE, ignore any differences in X and Y resolution.
 * @popup_width: (out) (optional): return location for the calculated popup
 *                                  width.
 * @popup_height: (out) (optional): return location for the calculated popup
 *                                  height.
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
 * @fg_color :desired foreground color for the preview when the type of
 *            @viewable support recolorization.
 *
 * Gets a preview for a viewable object, by running through a variety
 * of methods until it finds one that works.  First, if an
 * implementation exists of a "get_preview" method, it is tried, and
 * the result is returned if it is not %NULL.  Second, the function
 * checks to see whether there is a cached preview with the correct
 * dimensions; if so, it is returned.  If neither of these works, then
 * the function looks for an implementation of the "get_new_preview"
 * method, and executes it, caching the result.  If everything fails,
 * %NULL is returned.
 *
 * When a drawable can be recolored (for instance a generated or mask
 * brush), then @color will be used. Note that in many cases, viewables
 * cannot be recolored (e.g. images, layers or color brushes).
 * When setting @color to %NULL on a recolorable viewable, the used
 * color may be anything.
 *
 * Returns: (nullable): A #GimpTempBuf containing the preview image, or %NULL if
 *          none can be found or created.
 **/
GimpTempBuf *
gimp_viewable_get_preview (GimpViewable *viewable,
                           GimpContext  *context,
                           gint          width,
                           gint          height,
                           GeglColor    *fg_color)
{
  GimpViewablePrivate *private = GET_PRIVATE (viewable);
  GimpViewableClass   *viewable_class;
  GimpTempBuf         *temp_buf = NULL;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  if (G_UNLIKELY (context == NULL))
    g_warning ("%s: context is NULL", G_STRFUNC);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (viewable_class->get_preview)
    temp_buf = viewable_class->get_preview (viewable, context,
                                            width, height,
                                            fg_color);

  if (temp_buf)
    return temp_buf;

  if (private->preview_temp_buf &&
      ((fg_color == NULL && private->preview_temp_buf_color == NULL) ||
       (fg_color != NULL && private->preview_temp_buf_color != NULL)))
    {
      if (gimp_temp_buf_get_width  (private->preview_temp_buf) == width &&
          gimp_temp_buf_get_height (private->preview_temp_buf) == height)
        {
          gboolean same_colors = TRUE;

          if (fg_color)
            {
              gdouble r1, g1, b1, a1;
              gdouble r2, g2, b2, a2;

              /* Don't use gimp_color_is_perceptually_identical(). Exact
               * comparison is fine for this use case.
               */
              gegl_color_get_rgba (fg_color,
                                   &r1, &g1, &b1, &a1);
              gegl_color_get_rgba (private->preview_temp_buf_color,
                                   &r2, &g2, &b2, &a2);

              same_colors = (r1 == r2 && g1 == g2 && b1 == b2 && a1 == a2);
            }

          if (same_colors)
            return private->preview_temp_buf;
        }
    }

  g_clear_pointer (&private->preview_temp_buf, gimp_temp_buf_unref);
  g_clear_object (&private->preview_temp_buf_color);

  if (viewable_class->get_new_preview)
    temp_buf = viewable_class->get_new_preview (viewable, context,
                                                width, height,
                                                fg_color);

  private->preview_temp_buf       = temp_buf;
  private->preview_temp_buf_color = fg_color ?
                                    gegl_color_duplicate (fg_color) : NULL;

  return temp_buf;
}

/**
 * gimp_viewable_get_new_preview:
 * @viewable: The viewable object to get a preview for.
 * @width:    desired width for the preview
 * @height:   desired height for the preview
 * @fg_color: desired foreground color for the preview when the type of
 *            @viewable support recolorization.
 *
 * Gets a new preview for a viewable object.  Similar to
 * gimp_viewable_get_preview(), except that it tries things in a
 * different order, first looking for a "get_new_preview" method, and
 * then if that fails for a "get_preview" method.  This function does
 * not look for a cached preview.
 *
 * Returns: (nullable): A #GimpTempBuf containing the preview image, or %NULL if
 *          none can be found or created.
 **/
GimpTempBuf *
gimp_viewable_get_new_preview (GimpViewable *viewable,
                               GimpContext  *context,
                               gint          width,
                               gint          height,
                               GeglColor    *fg_color)
{
  GimpViewableClass *viewable_class;
  GimpTempBuf       *temp_buf = NULL;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  if (G_UNLIKELY (context == NULL))
    g_warning ("%s: context is NULL", G_STRFUNC);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (viewable_class->get_new_preview)
    temp_buf = viewable_class->get_new_preview (viewable, context,
                                                width, height,
                                                fg_color);

  if (temp_buf)
    return temp_buf;

  if (viewable_class->get_preview)
    temp_buf = viewable_class->get_preview (viewable, context,
                                            width, height,
                                            fg_color);

  if (temp_buf)
    return gimp_temp_buf_copy (temp_buf);

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
 * Returns: a #GimpTempBuf containing the preview image.
 **/
GimpTempBuf *
gimp_viewable_get_dummy_preview (GimpViewable *viewable,
                                 gint          width,
                                 gint          height,
                                 const Babl   *format)
{
  GdkPixbuf   *pixbuf;
  GimpTempBuf *buf;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);
  g_return_val_if_fail (format != NULL, NULL);

  pixbuf = gimp_viewable_get_dummy_pixbuf (viewable, width, height,
                                           babl_format_has_alpha (format));

  buf = gimp_temp_buf_new_from_pixbuf (pixbuf, format);

  g_object_unref (pixbuf);

  return buf;
}

/**
 * gimp_viewable_get_pixbuf:
 * @viewable: The viewable object to get a pixbuf preview for.
 * @context:  The context to render the preview for.
 * @width:    desired width for the preview
 * @height:   desired height for the preview
 * @fg_color: desired foreground color for the preview when the type of
 *            @viewable supports recolorization.
 *
 * Gets a preview for a viewable object, by running through a variety
 * of methods until it finds one that works.  First, if an
 * implementation exists of a "get_pixbuf" method, it is tried, and
 * the result is returned if it is not %NULL.  Second, the function
 * checks to see whether there is a cached preview with the correct
 * dimensions; if so, it is returned.  If neither of these works, then
 * the function looks for an implementation of the "get_new_pixbuf"
 * method, and executes it, caching the result.  If everything fails,
 * %NULL is returned.
 *
 * Returns: (nullable): A #GdkPixbuf containing the preview pixbuf,
 *          or %NULL if none can be found or created.
 **/
GdkPixbuf *
gimp_viewable_get_pixbuf (GimpViewable *viewable,
                          GimpContext  *context,
                          gint          width,
                          gint          height,
                          GeglColor    *fg_color)
{
  GimpViewablePrivate *private = GET_PRIVATE (viewable);
  GimpViewableClass   *viewable_class;
  GdkPixbuf           *pixbuf = NULL;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  if (G_UNLIKELY (context == NULL))
    g_warning ("%s: context is NULL", G_STRFUNC);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (viewable_class->get_pixbuf)
    pixbuf = viewable_class->get_pixbuf (viewable, context,
                                         width, height,
                                         fg_color);

  if (pixbuf)
    return pixbuf;

  if (private->preview_pixbuf &&
      ((fg_color == NULL && private->preview_pixbuf_color == NULL) ||
       (fg_color != NULL && private->preview_pixbuf_color != NULL)))
    {
      if (gdk_pixbuf_get_width  (private->preview_pixbuf) == width &&
          gdk_pixbuf_get_height (private->preview_pixbuf) == height)
        {
          gboolean same_colors = TRUE;

          if (fg_color)
            {
              gdouble r1, g1, b1, a1;
              gdouble r2, g2, b2, a2;

              /* Don't use gimp_color_is_perceptually_identical(). Exact
               * comparison is fine for this use case.
               */
              gegl_color_get_rgba (fg_color,
                                   &r1, &g1, &b1, &a1);
              gegl_color_get_rgba (private->preview_pixbuf_color,
                                   &r2, &g2, &b2, &a2);

              same_colors = (r1 == r2 && g1 == g2 && b1 == b2 && a1 == a2);
            }

          if (same_colors)
            return private->preview_pixbuf;
        }
    }

  g_clear_object (&private->preview_pixbuf);
  g_clear_object (&private->preview_pixbuf_color);

  if (viewable_class->get_new_pixbuf)
    pixbuf = viewable_class->get_new_pixbuf (viewable, context,
                                             width, height,
                                             fg_color);

  private->preview_pixbuf       = pixbuf;
  private->preview_pixbuf_color = fg_color ?
                                  gegl_color_duplicate (fg_color) : NULL;

  return pixbuf;
}

/**
 * gimp_viewable_get_new_pixbuf:
 * @viewable: The viewable object to get a new pixbuf preview for.
 * @context:  The context to render the preview for.
 * @width:    desired width for the pixbuf
 * @height:   desired height for the pixbuf
 * @fg_color: desired foreground color for the preview when the type of
 *            @viewable support recolorization.
 *
 * Gets a new preview for a viewable object.  Similar to
 * gimp_viewable_get_pixbuf(), except that it tries things in a
 * different order, first looking for a "get_new_pixbuf" method, and
 * then if that fails for a "get_pixbuf" method.  This function does
 * not look for a cached pixbuf.
 *
 * Returns: (nullable): A #GdkPixbuf containing the preview,
 *          or %NULL if none can be created.
 **/
GdkPixbuf *
gimp_viewable_get_new_pixbuf (GimpViewable *viewable,
                              GimpContext  *context,
                              gint          width,
                              gint          height,
                              GeglColor    *fg_color)
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
    pixbuf = viewable_class->get_new_pixbuf (viewable, context,
                                             width, height,
                                             fg_color);

  if (pixbuf)
    return pixbuf;

  if (viewable_class->get_pixbuf)
    pixbuf = viewable_class->get_pixbuf (viewable, context,
                                         width, height,
                                         fg_color);

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
                                gboolean       with_alpha)
{
  GdkPixbuf *icon;
  GdkPixbuf *pixbuf;
  GError    *error = NULL;
  gdouble    ratio;
  gint       w, h;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (width  > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  icon = gdk_pixbuf_new_from_resource ("/org/gimp/icons/64/dialog-question.png",
                                       &error);
  if (! icon)
    {
      g_critical ("Failed to create icon image: %s", error->message);
      g_clear_error (&error);
      return NULL;
    }

  w = gdk_pixbuf_get_width (icon);
  h = gdk_pixbuf_get_height (icon);

  ratio = (gdouble) MIN (width, height) / (gdouble) MAX (w, h);
  ratio = MIN (ratio, 1.0);

  w = RINT (ratio * (gdouble) w);
  h = RINT (ratio * (gdouble) h);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, with_alpha, 8, width, height);
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
 * @tooltip: (out) (optional) (nullable): return location for an optional
 *                                        tooltip string.
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
 * gimp_viewable_is_name_editable:
 * @viewable: viewable object for which to retrieve a description.
 *
 * Returns: whether the viewable's name is editable by the user.
 **/
gboolean
gimp_viewable_is_name_editable (GimpViewable *viewable)
{
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), FALSE);

  return GIMP_VIEWABLE_GET_CLASS (viewable)->is_name_editable (viewable);
}

/**
 * gimp_viewable_get_icon_name:
 * @viewable: viewable object for which to retrieve a icon name.
 *
 * Gets the current value of the object's icon name, for use in
 * constructing an iconic representation of the object.
 *
 * Returns: a pointer to the string containing the icon name.  The
 *          contents must not be altered or freed.
 **/
const gchar *
gimp_viewable_get_icon_name (GimpViewable *viewable)
{
  GimpViewablePrivate *private = GET_PRIVATE (viewable);

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);

  if (private->icon_name)
    return (const gchar *) private->icon_name;

  return GIMP_VIEWABLE_GET_CLASS (viewable)->default_icon_name;
}

/**
 * gimp_viewable_set_icon_name:
 * @viewable: viewable object to assign the specified icon name.
 * @icon_name: string containing an icon name identifier.
 *
 * Set the object's icon name, for use in constructing iconic symbols
 * of the object. The contents of @icon_name are copied, so you can
 * free it when you are done with it.
 **/
void
gimp_viewable_set_icon_name (GimpViewable *viewable,
                             const gchar  *icon_name)
{
  GimpViewablePrivate *private = GET_PRIVATE (viewable);
  GimpViewableClass   *viewable_class;

  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  g_clear_pointer (&private->icon_name, g_free);

  viewable_class = GIMP_VIEWABLE_GET_CLASS (viewable);

  if (icon_name)
    {
      if (viewable_class->default_icon_name == NULL ||
          strcmp (icon_name, viewable_class->default_icon_name))
        private->icon_name = g_strdup (icon_name);
    }

  gimp_viewable_invalidate_preview (viewable);

  g_object_notify_by_pspec (G_OBJECT (viewable), obj_props[PROP_ICON_NAME]);
}

void
gimp_viewable_preview_freeze (GimpViewable *viewable)
{
  GimpViewablePrivate *private = GET_PRIVATE (viewable);

  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  private->freeze_count++;

  if (private->freeze_count == 1)
    {
      if (GIMP_VIEWABLE_GET_CLASS (viewable)->preview_freeze)
        GIMP_VIEWABLE_GET_CLASS (viewable)->preview_freeze (viewable);

      g_object_notify_by_pspec (G_OBJECT (viewable), obj_props[PROP_FROZEN]);
    }
}

void
gimp_viewable_preview_thaw (GimpViewable *viewable)
{
  GimpViewablePrivate *private = GET_PRIVATE (viewable);

  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));
  g_return_if_fail (private->freeze_count > 0);

  private->freeze_count--;

  if (private->freeze_count == 0)
    {
      if (private->size_changed_pending)
        {
          private->size_changed_pending = FALSE;

          gimp_viewable_size_changed (viewable);
        }

      if (private->invalidate_pending)
        {
          private->invalidate_pending = FALSE;

          gimp_viewable_invalidate_preview (viewable);
        }

      g_object_notify_by_pspec (G_OBJECT (viewable), obj_props[PROP_FROZEN]);

      if (GIMP_VIEWABLE_GET_CLASS (viewable)->preview_thaw)
        GIMP_VIEWABLE_GET_CLASS (viewable)->preview_thaw (viewable);
    }
}

gboolean
gimp_viewable_preview_is_frozen (GimpViewable *viewable)
{
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), FALSE);

  return GET_PRIVATE (viewable)->freeze_count != 0;
}

GimpViewable *
gimp_viewable_get_parent (GimpViewable *viewable)
{
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);

  return GET_PRIVATE (viewable)->parent;
}

void
gimp_viewable_set_parent (GimpViewable *viewable,
                          GimpViewable *parent)
{
  GimpViewablePrivate *private = GET_PRIVATE (viewable);

  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));
  g_return_if_fail (parent == NULL || GIMP_IS_VIEWABLE (parent));

  if (parent != private->parent)
    {
      private->parent = parent;
      private->depth  = parent ? gimp_viewable_get_depth (parent) + 1 : 0;

      g_signal_emit (viewable, viewable_signals[ANCESTRY_CHANGED], 0);
    }
}

gint
gimp_viewable_get_depth (GimpViewable *viewable)
{
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), 0);

  return GET_PRIVATE (viewable)->depth;
}

GimpContainer *
gimp_viewable_get_children (GimpViewable *viewable)
{
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);

  return GIMP_VIEWABLE_GET_CLASS (viewable)->get_children (viewable);
}

gboolean
gimp_viewable_get_expanded (GimpViewable *viewable)
{
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), FALSE);

  if (GIMP_VIEWABLE_GET_CLASS (viewable)->get_expanded)
    return GIMP_VIEWABLE_GET_CLASS (viewable)->get_expanded (viewable);

  return FALSE;
}

void
gimp_viewable_set_expanded (GimpViewable *viewable,
                            gboolean       expanded)
{
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  if (GIMP_VIEWABLE_GET_CLASS (viewable)->set_expanded)
    GIMP_VIEWABLE_GET_CLASS (viewable)->set_expanded (viewable, expanded);
}

gboolean
gimp_viewable_is_ancestor (GimpViewable *ancestor,
                           GimpViewable *descendant)
{
  g_return_val_if_fail (GIMP_IS_VIEWABLE (ancestor), FALSE);
  g_return_val_if_fail (GIMP_IS_VIEWABLE (descendant), FALSE);

  while (descendant)
    {
      GimpViewable *parent = gimp_viewable_get_parent (descendant);

      if (parent == ancestor)
        return TRUE;

      descendant = parent;
    }

  return FALSE;
}
