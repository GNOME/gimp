/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <glib.h>  /* lcms.h uses the "inline" keyword */

#include <lcms2.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "gimperror.h"
#include "gimpimage.h"
#include "gimpimage-profile.h"

#include "gimp-intl.h"


/* public functions */

const GimpParasite *
gimp_image_get_icc_profile (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_image_parasite_find (image, GIMP_ICC_PROFILE_PARASITE_NAME);
}

void
gimp_image_set_icc_profile (GimpImage          *image,
                            const GimpParasite *icc_profile)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  if (icc_profile)
    {
      g_return_if_fail (strcmp (gimp_parasite_name (icc_profile),
                                GIMP_ICC_PROFILE_PARASITE_NAME) == 0);
      g_return_if_fail (gimp_parasite_flags (icc_profile) ==
                        (GIMP_PARASITE_PERSISTENT |
                         GIMP_PARASITE_UNDOABLE));

      gimp_image_parasite_attach (image, icc_profile);
    }
  else
    {
      gimp_image_parasite_detach (image, GIMP_ICC_PROFILE_PARASITE_NAME);
    }
}

GimpColorProfile
gimp_image_get_profile (GimpImage  *image,
                        guint8     *md5_digest,
                        GError    **error)
{
  GimpColorConfig    *config;
  const GimpParasite *parasite;
  GimpColorProfile   *profile = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  config = image->gimp->config->color_management;

  parasite = gimp_image_get_icc_profile (image);

  if (parasite)
    {
      profile = gimp_lcms_profile_open_from_data (gimp_parasite_data (parasite),
                                                  gimp_parasite_data_size (parasite),
                                                  md5_digest, error);

      if (! profile)
        g_prefix_error (error,
                        _("Error parsing data attached as 'icc-profile': "));

      if (profile && ! gimp_lcms_profile_is_rgb (profile))
        {
          g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                               _("Color profile attached as 'icc-profile' is "
                                 "not for RGB color space"));
          cmsCloseProfile (profile);
          profile = NULL;
        }
    }
  else if (config->rgb_profile)
    {
      profile = gimp_lcms_profile_open_from_file (config->rgb_profile,
                                                  md5_digest, error);

      if (profile && ! gimp_lcms_profile_is_rgb (profile))
        {
          g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                       _("Color profile '%s' is not for RGB color space"),
                       gimp_filename_to_utf8 (config->rgb_profile));
          cmsCloseProfile (profile);
          profile = NULL;
        }
    }

  return profile;
}
