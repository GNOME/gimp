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

#include <string.h>

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


#define GIMP_LCMS_MD5_DIGEST_LENGTH 16


static GQuark
gimp_lcms_error_quark (void)
{
  static GQuark quark = 0;

  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("gimp-lcms-error-quark");

  return quark;
}

GimpColorProfile
gimp_lcms_profile_open_from_file (GFile   *file,
                                  GError **error)
{
  GimpColorProfile  profile = NULL;
  gchar            *path;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  path = g_file_get_path (file);

  if (path)
    {
      GMappedFile  *mapped;
      const guint8 *data;
      gsize         length;

      mapped = g_mapped_file_new (path, FALSE, error);

      if (! mapped)
        return NULL;

      data   = (const guint8 *) g_mapped_file_get_contents (mapped);
      length = g_mapped_file_get_length (mapped);

      profile = cmsOpenProfileFromMem (data, length);

      g_mapped_file_unref (mapped);
    }
  else
    {
      GFileInfo *info;

      info = g_file_query_info (file,
                                G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                G_FILE_QUERY_INFO_NONE,
                                NULL, error);
      if (info)
        {
          GInputStream *input;
          goffset       length = g_file_info_get_size (info);
          guint8       *data   = g_malloc (length);

          g_object_unref (info);

          input = G_INPUT_STREAM (g_file_read (file, NULL, error));

          if (input)
            {
              gsize bytes_read;

              if (g_input_stream_read_all (input, data, length,
                                           &bytes_read, NULL, error) &&
                  bytes_read == length)
                {
                  profile = cmsOpenProfileFromMem (data, length);
                }

              g_object_unref (input);
            }

          g_free (data);
        }
    }

  if (! profile && error && *error == NULL)
    g_set_error (error, gimp_lcms_error_quark (), 0,
                 _("'%s' does not appear to be an ICC color profile"),
                 gimp_file_get_utf8_name (file));

  return profile;
}

GimpColorProfile
gimp_lcms_profile_open_from_data (const guint8  *data,
                                  gsize          length,
                                  GError       **error)
{
  GimpColorProfile  profile;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (length > 0, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  profile = cmsOpenProfileFromMem (data, length);

  if (! profile)
    g_set_error_literal (error, gimp_lcms_error_quark (), 0,
                         _("Data does not appear to be an ICC color profile"));

  return profile;
}

guint8 *
gimp_lcms_profile_save_to_data (GimpColorProfile   profile,
                                gsize             *length,
                                GError           **error)
{
  cmsUInt32Number size;

  g_return_val_if_fail (profile != NULL, NULL);
  g_return_val_if_fail (length != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (cmsSaveProfileToMem (profile, NULL, &size))
    {
      guint8 *data = g_malloc (size);

      if (cmsSaveProfileToMem (profile, data, &size))
        {
          *length = size;

          return data;
        }

      g_free (data);
    }

  g_set_error_literal (error, gimp_lcms_error_quark (), 0,
                       _("Could not save color profile to memory"));

  return NULL;
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
gimp_lcms_profile_get_label (GimpColorProfile profile)
{
  gchar *label;

  g_return_val_if_fail (profile != NULL, NULL);

  label = gimp_lcms_profile_get_description (profile);

  if (label && ! strlen (label))
    {
      g_free (label);
      label = NULL;
    }

  if (! label)
    label = gimp_lcms_profile_get_model (profile);

  if (label && ! strlen (label))
    {
      g_free (label);
      label = NULL;
    }

  if (! label)
    label = g_strdup (_("(unnamed profile)"));

  return label;
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
gimp_lcms_profile_is_equal (GimpColorProfile profile1,
                            GimpColorProfile profile2)
{
  cmsUInt8Number digest1[GIMP_LCMS_MD5_DIGEST_LENGTH];
  cmsUInt8Number digest2[GIMP_LCMS_MD5_DIGEST_LENGTH];

  g_return_val_if_fail (profile1 != NULL, FALSE);
  g_return_val_if_fail (profile2 != NULL, FALSE);

  if (! cmsMD5computeID (profile1) ||
      ! cmsMD5computeID (profile2))
    {
      return FALSE;
    }

  cmsGetHeaderProfileID (profile1, digest1);
  cmsGetHeaderProfileID (profile2, digest2);

  return (memcmp (digest1, digest2, GIMP_LCMS_MD5_DIGEST_LENGTH) == 0);
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

static GimpColorProfile
gimp_lcms_create_srgb_profile_internal (void)
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
  static guint8 *profile_data   = NULL;
  static gsize   profile_length = 0;

  if (G_UNLIKELY (profile_data == NULL))
    {
      GimpColorProfile profile;

      profile = gimp_lcms_create_srgb_profile_internal ();

      profile_data = gimp_lcms_profile_save_to_data (profile, &profile_length,
                                                     NULL);

      cmsCloseProfile (profile);
    }

  return gimp_lcms_profile_open_from_data (profile_data, profile_length, NULL);
}
