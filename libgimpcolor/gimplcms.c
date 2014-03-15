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

#include <gio/gio.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "gimpcolortypes.h"

#include "gimplcms.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimplcms
 * @title: GimpLcms
 * @short_description: Definitions and Functions relating to LCMS.
 *
 * Definitions and Functions relating to LCMS.
 **/

static GQuark
gimp_lcms_error_quark (void)
{
  static GQuark quark = 0;

  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("gimp-lcms-error-quark");

  return quark;
}

static void
gimp_lcms_calculate_checksum (const guint8 *data,
                              gsize         length,
                              guint8       *md5_digest)
{
  GChecksum *md5 = g_checksum_new (G_CHECKSUM_MD5);

  g_checksum_update (md5,
                     (const guchar *) data + sizeof (cmsICCHeader),
                     length - sizeof (cmsICCHeader));

  length = GIMP_LCMS_MD5_DIGEST_LENGTH;
  g_checksum_get_digest (md5, md5_digest, &length);
  g_checksum_free (md5);
}

GimpColorProfile
gimp_lcms_profile_open_from_file (const gchar  *filename,
                                  guint8       *md5_digest,
                                  GError      **error)
{
  GimpColorProfile  profile;
  GMappedFile      *file;
  const guint8     *data;
  gsize             length;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  file = g_mapped_file_new (filename, FALSE, error);

  if (! file)
    return NULL;

  data   = (const guint8 *) g_mapped_file_get_contents (file);
  length = g_mapped_file_get_length (file);

  profile = cmsOpenProfileFromMem (data, length);

  if (! profile)
    {
      g_set_error (error, gimp_lcms_error_quark (), 0,
                   _("'%s' does not appear to be an ICC color profile"),
                   gimp_filename_to_utf8 (filename));
    }
  else if (md5_digest)
    {
      gimp_lcms_calculate_checksum (data, length, md5_digest);
    }

  g_mapped_file_unref (file);

  return profile;
}

GimpColorProfile
gimp_lcms_profile_open_from_data (const guint8  *data,
                                  gsize          length,
                                  guint8        *md5_digest,
                                  GError       **error)
{
  GimpColorProfile  profile;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (length > 0, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  profile = cmsOpenProfileFromMem (data, length);

  if (! profile)
    {
      g_set_error_literal (error, gimp_lcms_error_quark (), 0,
                           _("Data does not appear to be an ICC color profile"));
    }
  else if (md5_digest)
    {
      gimp_lcms_calculate_checksum (data, length, md5_digest);
    }

  return profile;
}

static gchar *
gimp_lcms_profile_get_info (GimpColorProfile profile,
                            cmsInfoType      info)
{
  cmsUInt32Number  size;
  gchar           *text = NULL;

  g_return_val_if_fail (profile != NULL, NULL);

  size = cmsGetProfileInfoASCII (profile, info,
                                 "en", "US", NULL, 0);
  if (size > 0)
    {
      gchar *data = g_new (gchar, size + 1);

      size = cmsGetProfileInfoASCII (profile, info,
                                     "en", "US", data, size);
      if (size > 0)
        text = gimp_any_to_utf8 (data, -1, NULL);

      g_free (data);
    }

  return text;
}

gchar *
gimp_lcms_profile_get_description (GimpColorProfile profile)
{
  return gimp_lcms_profile_get_info (profile, cmsInfoDescription);
}

gchar *
gimp_lcms_profile_get_manufacturer (GimpColorProfile profile)
{
  return gimp_lcms_profile_get_info (profile, cmsInfoManufacturer);
}

gchar *
gimp_lcms_profile_get_model (GimpColorProfile profile)
{
  return gimp_lcms_profile_get_info (profile, cmsInfoModel);
}

gchar *
gimp_lcms_profile_get_copyright (GimpColorProfile profile)
{
  return gimp_lcms_profile_get_info (profile, cmsInfoCopyright);
}

gchar *
gimp_lcms_profile_get_summary (GimpColorProfile profile)
{
  GString *string;
  gchar   *text;

  g_return_val_if_fail (profile != NULL, NULL);

  string = g_string_new (NULL);

  text = gimp_lcms_profile_get_description (profile);
  if (text)
    {
      g_string_append (string, text);
      g_free (text);
    }

  text = gimp_lcms_profile_get_model (profile);
  if (text)
    {
      if (string->len > 0)
        g_string_append (string, "\n");

      g_string_append (string, text);
    }

  text = gimp_lcms_profile_get_manufacturer (profile);
  if (text)
    {
      if (string->len > 0)
        g_string_append (string, "\n");

      g_string_append (string, text);
    }

  text = gimp_lcms_profile_get_copyright (profile);
  if (text)
    {
      if (string->len > 0)
        g_string_append (string, "\n");

      g_string_append (string, text);
    }

  return g_string_free (string, FALSE);
}

gboolean
gimp_lcms_profile_is_rgb (GimpColorProfile profile)
{
  g_return_val_if_fail (profile != NULL, FALSE);

  return (cmsGetColorSpace (profile) == cmsSigRgbData);
}

gboolean
gimp_lcms_profile_is_cmyk (GimpColorProfile profile)
{
  g_return_val_if_fail (profile != NULL, FALSE);

  return (cmsGetColorSpace (profile) == cmsSigCmykData);
}

static void
gimp_lcms_profile_set_tag (cmsHPROFILE      profile,
                           cmsTagSignature  sig,
                           const gchar     *tag)
{
  cmsMLU *mlu;

  mlu = cmsMLUalloc (NULL, 1);
  cmsMLUsetASCII (mlu, "en", "US", tag);
  cmsWriteTag (profile, sig, mlu);
  cmsMLUfree (mlu);
}

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
 * The actual primaries in the sRGB specification are
 * red xy:   {0.6400, 0.3300, 1.0}
 * green xy: {0.3000, 0.6000, 1.0}
 * blue xy:  {0.1500, 0.0600, 1.0}
 *
 * The sRGB primaries given below are "pre-quantized" to compensate
 * for hexadecimal quantization during the profile-making process.
 * Unless the profile-making code compensates for this quantization,
 * the resulting profile's red, green, and blue colorants will deviate
 * slightly from the correct XYZ values.
 *
 * LCMS2 doesn't compensate for hexadecimal quantization. The
 * "pre-quantized" primaries below were back-calculated from the
 * ArgyllCMS sRGB.icm profile. The resulting sRGB profile's colorants
 * exactly matches the ArgyllCMS sRGB.icm profile colorants.
 *
 * Return value: the sRGB cmsHPROFILE.
 *
 * Since: GIMP 2.10
 **/
GimpColorProfile
gimp_lcms_create_srgb_profile (void)
{
  cmsHPROFILE srgb_profile;
  cmsCIExyY   d65_srgb_specs = { 0.3127, 0.3290, 1.0 };

  cmsCIExyYTRIPLE srgb_primaries_pre_quantized =
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

  gimp_lcms_profile_set_tag (srgb_profile, cmsSigProfileDescriptionTag,
                             "GIMP built-in sRGB");
  gimp_lcms_profile_set_tag (srgb_profile, cmsSigDeviceMfgDescTag,
                             "GIMP");
  gimp_lcms_profile_set_tag (srgb_profile, cmsSigDeviceModelDescTag,
                             "sRGB");
  gimp_lcms_profile_set_tag (srgb_profile, cmsSigCopyrightTag,
                             "Public Domain");

  /**
   * The following line produces a V2 profile with a point curve TRC.
   * Profiles with point curve TRCs can't be used in LCMS2 unbounded
   * mode ICC profile conversions. A V2 profile might be appropriate
   * for embedding in sRGB images saved to disk, if the image is to be
   * opened by an image editing application that doesn't understand V4
   * profiles.
   *
   * cmsSetProfileVersion (srgb_profile, 2.1);
   **/

  return srgb_profile;
}
