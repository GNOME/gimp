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
 * @image_ID: The image.
 *
 * Returns the image's color profile
 *
 * This procedure returns the image's color profile, or NULL if the
 * image has no color profile assigned.
 *
 * Returns: The image's color profile. The returned value
 *          must be freed with g_object_unref().
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_image_get_color_profile (gint32 image_ID)
{
  guint8 *data;
  gint    length;

  data = _gimp_image_get_color_profile (image_ID, &length);

  if (data)
    {
      GimpColorProfile *profile;

      profile = gimp_color_profile_new_from_icc_profile (data, length, NULL);
      g_free (data);

      return profile;
    }

  return NULL;
}

/**
 * gimp_image_set_color_profile:
 * @image_ID: The image.
 * @profile:  A #GimpColorProfile, or %NULL.
 *
 * Sets the image's color profile
 *
 * This procedure sets the image's color profile.
 *
 * Returns: %TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
gimp_image_set_color_profile (gint32            image_ID,
                              GimpColorProfile *profile)
{
  const guint8 *data   = NULL;
  gint          length = 0;

  g_return_val_if_fail (profile == NULL || GIMP_IS_COLOR_PROFILE (profile),
                        FALSE);

  if (profile)
    {
      gsize l;

      data = gimp_color_profile_get_icc_profile (profile, &l);
      length = l;
    }

  return _gimp_image_set_color_profile (image_ID, length, data);
}

/**
 * gimp_image_get_effective_color_profile:
 * @image_ID: The image.
 *
 * Returns the color profile that is used for the image.
 *
 * This procedure returns the color profile that is actually used for
 * this image, which is the profile returned by
 * gimp_image_get_color_profile() if the image has a profile assigned,
 * or the default RGB profile from preferences if no profile is
 * assigned to the image. If there is no default RGB profile configured
 * in preferences either, a generated default RGB profile is returned.
 *
 * Returns: The color profile. The returned value
 *          must be freed with g_object_unref().
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_image_get_effective_color_profile (gint32 image_ID)
{
  guint8 *data;
  gint    length;

  data = _gimp_image_get_effective_color_profile (image_ID, &length);

  if (data)
    {
      GimpColorProfile *profile;

      profile = gimp_color_profile_new_from_icc_profile (data, length, NULL);
      g_free (data);

      return profile;
    }

  return NULL;
}

/**
 * gimp_image_convert_color_profile:
 * @image_ID: The image.
 * @profile:  The color profile to convert to.
 * @intent:   Rendering intent.
 * @bpc:      Black point compensation.
 *
 * Convert the image's layers to a color profile
 *
 * This procedure converts from the image's color profile (or the
 * default RGB profile if none is set) to the given color profile. Only
 * RGB color profiles are accepted.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
gimp_image_convert_color_profile (gint32                     image_ID,
                                  GimpColorProfile          *profile,
                                  GimpColorRenderingIntent   intent,
                                  gboolean                   bpc)
{
  const guint8 *data   = NULL;
  gint          length = 0;

  g_return_val_if_fail (profile == NULL || GIMP_IS_COLOR_PROFILE (profile),
                        FALSE);

  if (profile)
    {
      gsize l;

      data = gimp_color_profile_get_icc_profile (profile, &l);
      length = l;
    }

  return _gimp_image_convert_color_profile (image_ID, length, data,
                                            intent, bpc);
}
