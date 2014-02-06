/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimplcms.c
 * Copyright (C) 2014  Michael Natterer <mitch@gimp.org>
 *                     Elle Stone <ellestone@ninedegreesbelow.com>
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
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib.h>  /* lcms.h uses the "inline" keyword */

#include <lcms2.h>

#include <gegl.h>

#include "gimpcolortypes.h"

#include "gimplcms.h"


/**
 * SECTION: gimplcms
 * @title: GimpLcms
 * @short_description: Definitions and Functions relating to LCMS.
 *
 * Definitions and Functions relating to LCMS.
 **/


/**
 * gimp_lcms_create_srgb_profile:
 *
 * This function is a replacement for cmsCreate_sRGBProfile() and
 * returns an sRGB profile that is functionally the same as the
 * ArgyllCMS sRGB.icm profile. "Functionally the same" means it has
 * the same red, green, and blue colorants and the V4 "chad"
 * equivalent of the ArgyllCMS V2 white point. The profile TRC is also
 * functionally equivalent to the ArgyllCMS sRGB.icm TRC and is the
 * same as the LCMS sRGB built-in profile TRC.
 *
 * Return value: the sRGB cmsHPROFILE.
 *
 * Since: GIMP 2.10
 **/
gpointer
gimp_lcms_create_srgb_profile (void)
{
  cmsHPROFILE      srgb_profile;
  cmsMLU          *description;
  cmsCIExyY        d65_srgb_specs = { 0.3127, 0.3290, 1.0 };
  cmsCIExyYTRIPLE  srgb_primaries_pre_quantized =
    {
      { 0.639998686, 0.330010138, 1.0 },
      { 0.300003784, 0.600003357, 1.0 },
      { 0.150002046, 0.059997204, 1.0 }
    };

  cmsFloat64Number srgb_parameters[5] =
    { 2.4, 1.0 / 1.055,  0.055 / 1.055, 1.0 / 12.92, 0.04045 };

  cmsToneCurve *srgb_parametric_curve =
    cmsBuildParametricToneCurve (NULL, 4, srgb_parameters);

  cmsToneCurve *tone_curve[3];

  tone_curve[0] = tone_curve[1] = tone_curve[2] = srgb_parametric_curve;

  srgb_profile = cmsCreateRGBProfile (&d65_srgb_specs,
                                      &srgb_primaries_pre_quantized,
                                      tone_curve);

  cmsFreeToneCurve (srgb_parametric_curve);

  description = cmsMLUalloc (NULL, 1);
  cmsMLUsetASCII (description,
                  "en", "US",
                  "sRGB made with the correct white point and primaries.");
  cmsWriteTag (srgb_profile, cmsSigProfileDescriptionTag, description);
  cmsMLUfree (description);

  cmsSetProfileVersion (srgb_profile, 2.1);

  return srgb_profile;
}
