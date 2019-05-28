/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpcolortransform.c
 * Copyright (C) 2014  Michael Natterer <mitch@gimp.org>
 *                     Elle Stone <ellestone@ninedegreesbelow.com>
 *                     Øyvind Kolås <pippin@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <lcms2.h>

#include <gio/gio.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "gimpcolortypes.h"

#include "gimpcolorprofile.h"
#include "gimpcolortransform.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpcolortransform
 * @title: GimpColorTransform
 * @short_description: Definitions and Functions relating to LCMS.
 *
 * Definitions and Functions relating to LCMS.
 **/

/**
 * GimpColorTransform:
 *
 * Simply a typedef to #gpointer, but actually is a cmsHTRANSFORM. It's
 * used in public GIMP APIs in order to avoid having to include LCMS
 * headers.
 **/


enum
{
  PROGRESS,
  LAST_SIGNAL
};


struct _GimpColorTransformPrivate
{
  GimpColorProfile *src_profile;
  const Babl       *src_format;
  const Babl       *src_space_format;

  GimpColorProfile *dest_profile;
  const Babl       *dest_format;
  const Babl       *dest_space_format;

  cmsHTRANSFORM     transform;
  const Babl       *fish;
};


static void   gimp_color_transform_finalize (GObject *object);


G_DEFINE_TYPE_WITH_PRIVATE (GimpColorTransform, gimp_color_transform,
                            G_TYPE_OBJECT)

#define parent_class gimp_color_transform_parent_class

static guint gimp_color_transform_signals[LAST_SIGNAL] = { 0 };

static gchar *lcms_last_error = NULL;


static void
lcms_error_clear (void)
{
  if (lcms_last_error)
    {
      g_free (lcms_last_error);
      lcms_last_error = NULL;
    }
}

static void
lcms_error_handler (cmsContext       ContextID,
                    cmsUInt32Number  ErrorCode,
                    const gchar     *text)
{
  lcms_error_clear ();

  lcms_last_error = g_strdup_printf ("lcms2 error %d: %s", ErrorCode, text);
}

static void
gimp_color_transform_class_init (GimpColorTransformClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_color_transform_finalize;

  gimp_color_transform_signals[PROGRESS] =
    g_signal_new ("progress",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorTransformClass,
                                   progress),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__DOUBLE,
                  G_TYPE_NONE, 1,
                  G_TYPE_DOUBLE);

  cmsSetLogErrorHandler (lcms_error_handler);
}

static void
gimp_color_transform_init (GimpColorTransform *transform)
{
  transform->priv = gimp_color_transform_get_instance_private (transform);
}

static void
gimp_color_transform_finalize (GObject *object)
{
  GimpColorTransform *transform = GIMP_COLOR_TRANSFORM (object);

  g_clear_object (&transform->priv->src_profile);
  g_clear_object (&transform->priv->dest_profile);

  g_clear_pointer (&transform->priv->transform, cmsDeleteTransform);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/**
 * gimp_color_transform_new:
 * @src_profile:      the source #GimpColorProfile
 * @src_format:       the source #Babl format
 * @dest_profile:     the destination #GimpColorProfile
 * @dest_format:      the destination #Babl format
 * @rendering_intent: the rendering intent
 * @flags:            transform flags
 *
 * This function creates an color transform.
 *
 * Return value: the #GimpColorTransform, or %NULL if no transform is needed
 *               to convert between pixels of @src_profile and @dest_profile.
 *
 * Since: 2.10
 **/
GimpColorTransform *
gimp_color_transform_new (GimpColorProfile         *src_profile,
                          const Babl               *src_format,
                          GimpColorProfile         *dest_profile,
                          const Babl               *dest_format,
                          GimpColorRenderingIntent  rendering_intent,
                          GimpColorTransformFlags   flags)
{
  GimpColorTransform        *transform;
  GimpColorTransformPrivate *priv;
  cmsHPROFILE                src_lcms;
  cmsHPROFILE                dest_lcms;
  cmsUInt32Number            lcms_src_format;
  cmsUInt32Number            lcms_dest_format;
  GError                    *error = NULL;

  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (src_profile), NULL);
  g_return_val_if_fail (src_format != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (dest_profile), NULL);
  g_return_val_if_fail (dest_format != NULL, NULL);

  if (gimp_color_transform_can_gegl_copy (src_profile, dest_profile))
    return NULL;

  transform = g_object_new (GIMP_TYPE_COLOR_TRANSFORM, NULL);

  priv = transform->priv;

  priv->src_space_format = gimp_color_profile_get_format (src_profile,
                                                          src_format,
                                                          BABL_ICC_INTENT_RELATIVE_COLORIMETRIC,
                                                          &error);
  if (! priv->src_space_format)
    {
      g_printerr ("%s: error making src format: %s\n",
                  G_STRFUNC, error->message);
      g_clear_error (&error);
    }

  priv->dest_space_format = gimp_color_profile_get_format (dest_profile,
                                                           dest_format,
                                                           rendering_intent,
                                                           &error);
  if (! priv->dest_space_format)
    {
      g_printerr ("%s: error making dest format: %s\n",
                  G_STRFUNC, error->message);
      g_clear_error (&error);
    }

  if (! g_getenv ("GIMP_COLOR_TRANSFORM_DISABLE_BABL") &&
      priv->src_space_format && priv->dest_space_format)
    {
      priv->src_format  = src_format;
      priv->dest_format = dest_format;
      priv->fish        = babl_fish (priv->src_space_format,
                                     priv->dest_space_format);

      g_printerr ("%s: using babl for '%s' -> '%s'\n",
                  G_STRFUNC,
                  gimp_color_profile_get_label (src_profile),
                  gimp_color_profile_get_label (dest_profile));

      return transform;
    }

  priv->src_space_format  = NULL;
  priv->dest_space_format = NULL;

  priv->src_format  = gimp_color_profile_get_lcms_format (src_format,
                                                          &lcms_src_format);
  priv->dest_format = gimp_color_profile_get_lcms_format (dest_format,
                                                          &lcms_dest_format);

  src_lcms  = gimp_color_profile_get_lcms_profile (src_profile);
  dest_lcms = gimp_color_profile_get_lcms_profile (dest_profile);

  lcms_error_clear ();

  priv->transform = cmsCreateTransform (src_lcms,  lcms_src_format,
                                        dest_lcms, lcms_dest_format,
                                        rendering_intent,
                                        flags |
                                        cmsFLAGS_COPY_ALPHA);

  if (lcms_last_error)
    {
      if (priv->transform)
        {
          cmsDeleteTransform (priv->transform);
          priv->transform = NULL;
        }

      g_printerr ("%s\n", lcms_last_error);
    }

  if (! priv->transform)
    {
      g_object_unref (transform);
      transform = NULL;
    }

  return transform;
}

/**
 * gimp_color_transform_new_proofing:
 * @src_profile:    the source #GimpColorProfile
 * @src_format:     the source #Babl format
 * @dest_profile:   the destination #GimpColorProfile
 * @dest_format:    the destination #Babl format
 * @proof_profile:  the proof #GimpColorProfile
 * @proof_intent:   the proof intent
 * @display_intent: the display intent
 * @flags:          transform flags
 *
 * This function creates a simulation / proofing color transform.
 *
 * Return value: the #GimpColorTransform, or %NULL.
 *
 * Since: 2.10
 **/
GimpColorTransform *
gimp_color_transform_new_proofing (GimpColorProfile         *src_profile,
                                   const Babl               *src_format,
                                   GimpColorProfile         *dest_profile,
                                   const Babl               *dest_format,
                                   GimpColorProfile         *proof_profile,
                                   GimpColorRenderingIntent  proof_intent,
                                   GimpColorRenderingIntent  display_intent,
                                   GimpColorTransformFlags   flags)
{
  GimpColorTransform        *transform;
  GimpColorTransformPrivate *priv;
  cmsHPROFILE                src_lcms;
  cmsHPROFILE                dest_lcms;
  cmsHPROFILE                proof_lcms;
  cmsUInt32Number            lcms_src_format;
  cmsUInt32Number            lcms_dest_format;

  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (src_profile), NULL);
  g_return_val_if_fail (src_format != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (dest_profile), NULL);
  g_return_val_if_fail (dest_format != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (proof_profile), NULL);

  transform = g_object_new (GIMP_TYPE_COLOR_TRANSFORM, NULL);

  priv = transform->priv;

  src_lcms   = gimp_color_profile_get_lcms_profile (src_profile);
  dest_lcms  = gimp_color_profile_get_lcms_profile (dest_profile);
  proof_lcms = gimp_color_profile_get_lcms_profile (proof_profile);

  priv->src_format  = gimp_color_profile_get_lcms_format (src_format,
                                                          &lcms_src_format);
  priv->dest_format = gimp_color_profile_get_lcms_format (dest_format,
                                                          &lcms_dest_format);

  lcms_error_clear ();

  priv->transform = cmsCreateProofingTransform (src_lcms,  lcms_src_format,
                                                dest_lcms, lcms_dest_format,
                                                proof_lcms,
                                                proof_intent,
                                                display_intent,
                                                flags                 |
                                                cmsFLAGS_SOFTPROOFING |
                                                cmsFLAGS_COPY_ALPHA);

  if (lcms_last_error)
    {
      if (priv->transform)
        {
          cmsDeleteTransform (priv->transform);
          priv->transform = NULL;
        }

      g_printerr ("%s\n", lcms_last_error);
    }

  if (! priv->transform)
    {
      g_object_unref (transform);
      transform = NULL;
    }

  return transform;
}

/**
 * gimp_color_transform_process_pixels:
 * @transform:   a #GimpColorTransform
 * @src_format:  #Babl format of @src_pixels
 * @src_pixels:  pointer to the source pixels
 * @dest_format: #Babl format of @dest_pixels
 * @dest_pixels: pointer to the destination pixels
 * @length:      number of pixels to process
 *
 * This function transforms a contiguous line of pixels.
 *
 * Since: 2.10
 **/
void
gimp_color_transform_process_pixels (GimpColorTransform *transform,
                                     const Babl         *src_format,
                                     gconstpointer       src_pixels,
                                     const Babl         *dest_format,
                                     gpointer            dest_pixels,
                                     gsize               length)
{
  GimpColorTransformPrivate *priv;
  gpointer                  *src;
  gpointer                  *dest;

  g_return_if_fail (GIMP_IS_COLOR_TRANSFORM (transform));
  g_return_if_fail (src_format != NULL);
  g_return_if_fail (src_pixels != NULL);
  g_return_if_fail (dest_format != NULL);
  g_return_if_fail (dest_pixels != NULL);

  priv = transform->priv;

  if (src_format != priv->src_format)
    {
      src = g_malloc (length * babl_format_get_bytes_per_pixel (priv->src_format));

      babl_process (babl_fish (src_format,
                               priv->src_format),
                    src_pixels, src, length);
    }
  else
    {
      src = (gpointer) src_pixels;
    }

  if (dest_format != priv->dest_format)
    {
      dest = g_malloc (length * babl_format_get_bytes_per_pixel (priv->dest_format));
    }
  else
    {
      dest = dest_pixels;
    }

  if (priv->transform)
    {
      cmsDoTransform (priv->transform, src, dest, length);
    }
  else
    {
      babl_process (priv->fish, src, dest, length);
    }

  if (src_format != priv->src_format)
    {
      g_free (src);
    }

  if (dest_format != priv->dest_format)
    {
      babl_process (babl_fish (priv->dest_format,
                               dest_format),
                    dest, dest_pixels, length);

      g_free (dest);
    }
}

/**
 * gimp_color_transform_process_buffer:
 * @transform:   a #GimpColorTransform
 * @src_buffer:  source #GeglBuffer
 * @src_rect:    rectangle in @src_buffer
 * @dest_buffer: destination #GeglBuffer
 * @dest_rect:   rectangle in @dest_buffer
 *
 * This function transforms buffer into another buffer.
 *
 * Since: 2.10
 **/
void
gimp_color_transform_process_buffer (GimpColorTransform  *transform,
                                     GeglBuffer          *src_buffer,
                                     const GeglRectangle *src_rect,
                                     GeglBuffer          *dest_buffer,
                                     const GeglRectangle *dest_rect)
{
  GimpColorTransformPrivate *priv;
  GeglBufferIterator        *iter;
  gint                       total_pixels;
  gint                       done_pixels = 0;

  g_return_if_fail (GIMP_IS_COLOR_TRANSFORM (transform));
  g_return_if_fail (GEGL_IS_BUFFER (src_buffer));
  g_return_if_fail (GEGL_IS_BUFFER (dest_buffer));

  priv = transform->priv;

  if (src_rect)
    {
      total_pixels = src_rect->width * src_rect->height;
    }
  else
    {
      total_pixels = (gegl_buffer_get_width  (src_buffer) *
                      gegl_buffer_get_height (src_buffer));
    }

  if (src_buffer != dest_buffer)
    {
      iter = gegl_buffer_iterator_new (src_buffer, src_rect, 0,
                                       priv->src_format,
                                       GEGL_ACCESS_READ,
                                       GEGL_ABYSS_NONE, 2);

      gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0,
                                priv->dest_format,
                                GEGL_ACCESS_WRITE,
                                GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          if (priv->transform)
            {
              cmsDoTransform (priv->transform,
                              iter->items[0].data, iter->items[1].data, iter->length);
            }
          else
            {
              babl_process (priv->fish,
                            iter->items[0].data, iter->items[1].data, iter->length);
            }

          done_pixels += iter->items[0].roi.width * iter->items[0].roi.height;

          g_signal_emit (transform, gimp_color_transform_signals[PROGRESS], 0,
                         (gdouble) done_pixels /
                         (gdouble) total_pixels);
        }
    }
  else
    {
      iter = gegl_buffer_iterator_new (src_buffer, src_rect, 0,
                                       priv->src_format,
                                       GEGL_ACCESS_READWRITE,
                                       GEGL_ABYSS_NONE, 1);

      while (gegl_buffer_iterator_next (iter))
        {
          if (priv->transform)
            {
              cmsDoTransform (priv->transform,
                              iter->items[0].data, iter->items[0].data, iter->length);
            }
          else
            {
              babl_process (priv->fish,
                            iter->items[0].data, iter->items[0].data, iter->length);
            }

          done_pixels += iter->items[0].roi.width * iter->items[0].roi.height;

          g_signal_emit (transform, gimp_color_transform_signals[PROGRESS], 0,
                         (gdouble) done_pixels /
                         (gdouble) total_pixels);
        }
    }

  g_signal_emit (transform, gimp_color_transform_signals[PROGRESS], 0,
                 1.0);
}

/**
 * gimp_color_transform_can_gegl_copy:
 * @src_profile:  source #GimpColorProfile
 * @dest_profile: destination #GimpColorProfile
 *
 * This function checks if a GimpColorTransform is needed at all.
 *
 * Return value: %TRUE if pixels can be correctly converted between
 *               @src_profile and @dest_profile by simply using
 *               gegl_buffer_copy(), babl_process() or similar.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_transform_can_gegl_copy (GimpColorProfile *src_profile,
                                    GimpColorProfile *dest_profile)
{
  static GimpColorProfile *srgb_profile        = NULL;
  static GimpColorProfile *srgb_linear_profile = NULL;
  static GimpColorProfile *gray_profile        = NULL;
  static GimpColorProfile *gray_linear_profile = NULL;

  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (src_profile), FALSE);
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (dest_profile), FALSE);

  if (gimp_color_profile_is_equal (src_profile, dest_profile))
    return TRUE;

  if (! srgb_profile)
    {
      srgb_profile        = gimp_color_profile_new_rgb_srgb ();
      srgb_linear_profile = gimp_color_profile_new_rgb_srgb_linear ();
      gray_profile        = gimp_color_profile_new_d65_gray_srgb_trc ();
      gray_linear_profile = gimp_color_profile_new_d65_gray_linear ();
    }

  if ((gimp_color_profile_is_equal (src_profile, srgb_profile)        ||
       gimp_color_profile_is_equal (src_profile, srgb_linear_profile) ||
       gimp_color_profile_is_equal (src_profile, gray_profile)        ||
       gimp_color_profile_is_equal (src_profile, gray_linear_profile))
      &&
      (gimp_color_profile_is_equal (dest_profile, srgb_profile)        ||
       gimp_color_profile_is_equal (dest_profile, srgb_linear_profile) ||
       gimp_color_profile_is_equal (dest_profile, gray_profile)        ||
       gimp_color_profile_is_equal (dest_profile, gray_linear_profile)))
    {
      return TRUE;
    }

  return FALSE;
}
