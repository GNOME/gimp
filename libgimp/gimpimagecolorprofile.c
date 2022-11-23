/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * ligmaimagecolorprofile.c
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

#include "ligma.h"


/**
 * ligma_image_get_color_profile:
 * @image: The image.
 *
 * Returns the image's color profile
 *
 * This procedure returns the image's color profile, or NULL if the
 * image has no color profile assigned.
 *
 * Returns: (transfer full): The image's color profile. The returned
 *          value must be freed with g_object_unref().
 *
 * Since: 2.10
 **/
LigmaColorProfile *
ligma_image_get_color_profile (LigmaImage *image)
{
  guint8 *data;
  gint    length;

  data = _ligma_image_get_color_profile (image, &length);

  if (data)
    {
      LigmaColorProfile *profile;

      profile = ligma_color_profile_new_from_icc_profile (data, length, NULL);
      g_free (data);

      return profile;
    }

  return NULL;
}

/**
 * ligma_image_set_color_profile:
 * @image:   The image.
 * @profile: A #LigmaColorProfile, or %NULL.
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
ligma_image_set_color_profile (LigmaImage        *image,
                              LigmaColorProfile *profile)
{
  const guint8 *data   = NULL;
  gint          length = 0;

  g_return_val_if_fail (profile == NULL || LIGMA_IS_COLOR_PROFILE (profile),
                        FALSE);

  if (profile)
    {
      gsize l;

      data = ligma_color_profile_get_icc_profile (profile, &l);
      length = l;
    }

  return _ligma_image_set_color_profile (image, length, data);
}

/**
 * ligma_image_get_simulation_profile:
 * @image: The image.
 *
 * Returns the image's simulation color profile
 *
 * This procedure returns the image's simulation color profile, or NULL if
 * the image has no simulation color profile assigned.
 *
 * Returns: (transfer full): The image's simulation color profile. The
 *          returned value must be freed with g_object_unref().
 *
 * Since: 3.0
 **/
LigmaColorProfile *
ligma_image_get_simulation_profile (LigmaImage *image)
{
  guint8 *data;
  gint    length;

  data = _ligma_image_get_simulation_profile (image, &length);

  if (data)
    {
      LigmaColorProfile *profile;

      profile = ligma_color_profile_new_from_icc_profile (data, length, NULL);
      g_free (data);

      return profile;
    }

  return NULL;
}

/**
 * ligma_image_set_simulation_profile:
 * @image:   The image.
 * @profile: A #LigmaColorProfile, or %NULL.
 *
 * Sets the image's simulation color profile
 *
 * This procedure sets the image's simulation color profile.
 *
 * Returns: %TRUE on success.
 *
 * Since: 3.0
 **/
gboolean
ligma_image_set_simulation_profile (LigmaImage        *image,
                                   LigmaColorProfile *profile)
{
  const guint8 *data   = NULL;
  gint          length = 0;

  g_return_val_if_fail (profile == NULL || LIGMA_IS_COLOR_PROFILE (profile),
                        FALSE);

  if (profile)
    {
      gsize l;

      data = ligma_color_profile_get_icc_profile (profile, &l);
      length = l;
    }

  return _ligma_image_set_simulation_profile (image, length, data);
}

/**
 * ligma_image_get_effective_color_profile:
 * @image: The image.
 *
 * Returns the color profile that is used for the image.
 *
 * This procedure returns the color profile that is actually used for
 * this image, which is the profile returned by
 * ligma_image_get_color_profile() if the image has a profile assigned,
 * or the default RGB profile from preferences if no profile is
 * assigned to the image. If there is no default RGB profile configured
 * in preferences either, a generated default RGB profile is returned.
 *
 * Returns: (transfer full): The color profile. The returned value must
 *          be freed with g_object_unref().
 *
 * Since: 2.10
 **/
LigmaColorProfile *
ligma_image_get_effective_color_profile (LigmaImage *image)
{
  guint8 *data;
  gint    length;

  data = _ligma_image_get_effective_color_profile (image, &length);

  if (data)
    {
      LigmaColorProfile *profile;

      profile = ligma_color_profile_new_from_icc_profile (data, length, NULL);
      g_free (data);

      return profile;
    }

  return NULL;
}

/**
 * ligma_image_convert_color_profile:
 * @image:   The image.
 * @profile: The color profile to convert to.
 * @intent:  Rendering intent.
 * @bpc:     Black point compensation.
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
ligma_image_convert_color_profile (LigmaImage                 *image,
                                  LigmaColorProfile          *profile,
                                  LigmaColorRenderingIntent   intent,
                                  gboolean                   bpc)
{
  const guint8 *data   = NULL;
  gint          length = 0;

  g_return_val_if_fail (profile == NULL || LIGMA_IS_COLOR_PROFILE (profile),
                        FALSE);

  if (profile)
    {
      gsize l;

      data = ligma_color_profile_get_icc_profile (profile, &l);
      length = l;
    }

  return _ligma_image_convert_color_profile (image, length, data,
                                            intent, bpc);
}
