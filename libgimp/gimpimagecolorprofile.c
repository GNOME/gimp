/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpimagecolorprofile.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"


/**
 * gimp_image_get_color_profile:
 * @image: The image.
 *
 * This procedure returns the image's color profile, or %NULL if the
 * image uses the built-in color profile.
 *
 * See also [method@Gimp.Image.get_effective_color_profile] if you want
 * the effective color profile and don't want to have to deal with %NULL
 * return values.
 *
 * Returns: (transfer full) (nullable): The image's color profile. The
 *          returned value must be freed with [method@GObject.Object.unref].
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_image_get_color_profile (GimpImage *image)
{
  GBytes *data;

  data = _gimp_image_get_color_profile (image);
  if (data)
    {
      GimpColorProfile *profile;

      profile = gimp_color_profile_new_from_icc_profile (g_bytes_get_data (data, NULL),
                                                         g_bytes_get_size (data),
                                                         NULL);
      g_bytes_unref (data);

      return profile;
    }

  return NULL;
}

/**
 * gimp_image_set_color_profile:
 * @image:   The image.
 * @profile: (nullable): A #GimpColorProfile, or %NULL.
 *
 * This procedure sets the image's color profile.
 *
 * If %NULL is passed as @profile, then the built-in profile for
 * @image's color model is set.
 *
 * Returns: %TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
gimp_image_set_color_profile (GimpImage        *image,
                              GimpColorProfile *profile)
{
  const guint8 *data   = NULL;
  GBytes       *bytes  = NULL;
  gboolean      ret;

  g_return_val_if_fail (profile == NULL || GIMP_IS_COLOR_PROFILE (profile),
                        FALSE);

  if (profile)
    {
      gsize l;

      data = gimp_color_profile_get_icc_profile (profile, &l);
      bytes = g_bytes_new_static (data, l);
    }

  ret = _gimp_image_set_color_profile (image, bytes);

  g_bytes_unref (bytes);
  return ret;
}

/**
 * gimp_image_get_simulation_profile:
 * @image: The image.
 *
 * This procedure returns the image's simulation color profile, or %NULL if
 * the image has no simulation color profile assigned.
 *
 * Returns: (transfer full) (nullable): The image's simulation color profile. The
 *          returned value must be freed with [method@GObject.Object.unref].
 *
 * Since: 3.0
 **/
GimpColorProfile *
gimp_image_get_simulation_profile (GimpImage *image)
{
  GBytes *data;

  data = _gimp_image_get_simulation_profile (image);
  if (data)
    {
      GimpColorProfile *profile;

      profile = gimp_color_profile_new_from_icc_profile (g_bytes_get_data (data, NULL),
                                                         g_bytes_get_size (data),
                                                         NULL);
      g_bytes_unref (data);

      return profile;
    }

  return NULL;
}

/**
 * gimp_image_set_simulation_profile:
 * @image:   The image.
 * @profile: (nullable): A #GimpColorProfile, or %NULL.
 *
 * This procedure sets the image's simulation color profile.
 *
 * If %NULL is passed as @profile, then the simulation profile is unset.
 *
 * Returns: %TRUE on success.
 *
 * Since: 3.0
 **/
gboolean
gimp_image_set_simulation_profile (GimpImage        *image,
                                   GimpColorProfile *profile)
{
  const guint8 *data   = NULL;
  GBytes       *bytes  = NULL;
  gboolean      ret;

  g_return_val_if_fail (profile == NULL || GIMP_IS_COLOR_PROFILE (profile),
                        FALSE);

  if (profile)
    {
      gsize l;

      data = gimp_color_profile_get_icc_profile (profile, &l);
      bytes = g_bytes_new_static (data, l);
    }

  ret = _gimp_image_set_simulation_profile (image, bytes);
  g_bytes_unref (bytes);
  return ret;
}

/**
 * gimp_image_get_effective_color_profile:
 * @image: The image.
 *
 * This procedure returns the color profile that is actually used for
 * this image, which is the profile returned by
 * [method@Gimp.Image.get_color_profile] if the image has a profile
 * assigned, or a built-in profile for the given color space otherwise.
 *
 * Returns: (transfer full): The color profile. The returned value must
 *          be freed with [method@GObject.Object.unref].
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_image_get_effective_color_profile (GimpImage *image)
{
  GBytes *data;

  data = _gimp_image_get_effective_color_profile (image);

  if (data)
    {
      GimpColorProfile *profile;

      profile = gimp_color_profile_new_from_icc_profile (g_bytes_get_data (data, NULL),
                                                         g_bytes_get_size (data),
                                                         NULL);
      g_bytes_unref (data);

      return profile;
    }

  return NULL;
}

/**
 * gimp_image_convert_color_profile:
 * @image:   The image.
 * @profile: (nullable): The color profile to convert to.
 * @intent:  Rendering intent.
 * @bpc:     Black point compensation.
 *
 * This procedure converts from the image's color profile (or the
 * built-in profile if none is set) to the given color profile.
 *
 * Make sure you use a valid color profile, according to the image's
 * type (e.g. RGB or grayscale color profiles respectively for RGB or
 * grayscale images).
 *
 * Returns: %TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
gimp_image_convert_color_profile (GimpImage                 *image,
                                  GimpColorProfile          *profile,
                                  GimpColorRenderingIntent   intent,
                                  gboolean                   bpc)
{
  const guint8 *data   = NULL;
  GBytes       *bytes  = NULL;
  gboolean      ret;

  g_return_val_if_fail (profile == NULL || GIMP_IS_COLOR_PROFILE (profile),
                        FALSE);

  if (profile)
    {
      gsize l;

      data = gimp_color_profile_get_icc_profile (profile, &l);
      bytes = g_bytes_new_static (data, l);
    }

  ret = _gimp_image_convert_color_profile (image, bytes, intent, bpc);
  g_bytes_unref (bytes);
  return ret;
}
