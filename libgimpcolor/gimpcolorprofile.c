/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpcolorprofile.c
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

#include "gimpcolorprofile.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpcolorprofile
 * @title: GimpColorProfile
 * @short_description: Definitions and Functions relating to LCMS.
 *
 * Definitions and Functions relating to LCMS.
 **/

/**
 * GimpColorProfile:
 *
 * Simply a typedef to #gpointer, but actually is a cmsHPROFILE. It's
 * used in public GIMP APIs in order to avoid having to include LCMS
 * headers.
 **/


#define GIMP_LCMS_MD5_DIGEST_LENGTH 16


static GQuark
gimp_color_profile_error_quark (void)
{
  static GQuark quark = 0;

  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("gimp-color-profile-error-quark");

  return quark;
}

/**
 * gimp_color_profile_open_from_file:
 * @file:  a #GFile
 * @error: return location for #GError
 *
 * This function opens an ICC color profile from @file.
 *
 * Return value: the #GimpColorProfile, or %NULL. On error, %NULL is
 *               returned and @error is set.
 *
 * Since: 2.10
 **/
GimpColorProfile
gimp_color_profile_open_from_file (GFile   *file,
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
    g_set_error (error, gimp_color_profile_error_quark (), 0,
                 _("'%s' does not appear to be an ICC color profile"),
                 gimp_file_get_utf8_name (file));

  return profile;
}

/**
 * gimp_color_profile_open_from_data:
 * @data:   pointer to memory containing an ICC profile
 * @length: lenght of the profile in memory, in bytes
 * @error:  return location for #GError
 *
 * This function opens an ICC color profile from memory. On error,
 * %NULL is returned and @error is set.
 *
 * Return value: the #GimpColorProfile, or %NULL.
 *
 * Since: 2.10
 **/
GimpColorProfile
gimp_color_profile_open_from_data (const guint8  *data,
                                   gsize          length,
                                   GError       **error)
{
  GimpColorProfile  profile;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (length > 0, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  profile = cmsOpenProfileFromMem (data, length);

  if (! profile)
    g_set_error_literal (error, gimp_color_profile_error_quark (), 0,
                         _("Data does not appear to be an ICC color profile"));

  return profile;
}

/**
 * gimp_color_profile_dave_to_data:
 * @profile: a #GimpColorProfile
 * @length:  return location for the number of bytes written
 * @error:   return location for #GError
 *
 * This function saves @profile to an ICC color profile in newly
 * allocated memory. On error, %NULL is returned and @error is set.
 *
 * Return value: a pointer to the written IIC profile in memory, or
 *               %NULL. Free with g_free().
 *
 * Since: 2.10
 **/
guint8 *
gimp_color_profile_save_to_data (GimpColorProfile   profile,
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

  g_set_error_literal (error, gimp_color_profile_error_quark (), 0,
                       _("Could not save color profile to memory"));

  return NULL;
}

/**
 * gimp_color_profile_close:
 * @profile: a #GimpColorProfile
 *
 * This function closes a #GimpColorProfile and frees its memory.
 *
 * Since: 2.10
 **/
void
gimp_color_profile_close (GimpColorProfile profile)
{
  g_return_if_fail (profile != NULL);

  cmsCloseProfile (profile);
}

static gchar *
gimp_color_profile_get_info (GimpColorProfile profile,
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

/**
 * gimp_color_profile_get_description:
 * @profile: a #GimpColorProfile
 *
 * Return value: a newly allocated string containing @profile's
 *               description. Free with g_free().
 *
 * Since: 2.10
 **/
gchar *
gimp_color_profile_get_description (GimpColorProfile profile)
{
  return gimp_color_profile_get_info (profile, cmsInfoDescription);
}

/**
 * gimp_color_profile_get_manufacturer:
 * @profile: a #GimpColorProfile
 *
 * Return value: a newly allocated string containing @profile's
 *               manufacturer. Free with g_free().
 *
 * Since: 2.10
 **/
gchar *
gimp_color_profile_get_manufacturer (GimpColorProfile profile)
{
  return gimp_color_profile_get_info (profile, cmsInfoManufacturer);
}

/**
 * gimp_color_profile_get_model:
 * @profile: a #GimpColorProfile
 *
 * Return value: a newly allocated string containing @profile's
 *               model. Free with g_free().
 *
 * Since: 2.10
 **/
gchar *
gimp_color_profile_get_model (GimpColorProfile profile)
{
  return gimp_color_profile_get_info (profile, cmsInfoModel);
}

/**
 * gimp_color_profile_get_copyright:
 * @profile: a #GimpColorProfile
 *
 * Return value: a newly allocated string containing @profile's
 *               copyright. Free with g_free().
 *
 * Since: 2.10
 **/
gchar *
gimp_color_profile_get_copyright (GimpColorProfile profile)
{
  return gimp_color_profile_get_info (profile, cmsInfoCopyright);
}

/**
 * gimp_color_profile_get_label:
 * @profile: a #GimpColorProfile
 *
 * This function returns a newly allocated string containing
 * @profile's "title", a string that can be used to label the profile
 * in a user interface.
 *
 * Return value: the @profile's label. Free with g_free().
 *
 * Since: 2.10
 **/
gchar *
gimp_color_profile_get_label (GimpColorProfile profile)
{
  gchar *label;

  g_return_val_if_fail (profile != NULL, NULL);

  label = gimp_color_profile_get_description (profile);

  if (label && ! strlen (label))
    {
      g_free (label);
      label = NULL;
    }

  if (! label)
    label = gimp_color_profile_get_model (profile);

  if (label && ! strlen (label))
    {
      g_free (label);
      label = NULL;
    }

  if (! label)
    label = g_strdup (_("(unnamed profile)"));

  return label;
}

/**
 * gimp_color_profile_get_summary:
 * @profile: a #GimpColorProfile
 *
 * This function return a newly allocated string containing a
 * multi-line summary of @profile's description, model, manufacturer
 * and copyright, to be used as detailled information about the
 * prpfile in a user interface.
 *
 * Return value: the @profile's summary. Free with g_free().
 *
 * Since: 2.10
 **/
gchar *
gimp_color_profile_get_summary (GimpColorProfile profile)
{
  GString *string;
  gchar   *text;

  g_return_val_if_fail (profile != NULL, NULL);

  string = g_string_new (NULL);

  text = gimp_color_profile_get_description (profile);
  if (text)
    {
      g_string_append (string, text);
      g_free (text);
    }

  text = gimp_color_profile_get_model (profile);
  if (text)
    {
      if (string->len > 0)
        g_string_append (string, "\n");

      g_string_append (string, text);
    }

  text = gimp_color_profile_get_manufacturer (profile);
  if (text)
    {
      if (string->len > 0)
        g_string_append (string, "\n");

      g_string_append (string, text);
    }

  text = gimp_color_profile_get_copyright (profile);
  if (text)
    {
      if (string->len > 0)
        g_string_append (string, "\n");

      g_string_append (string, text);
    }

  return g_string_free (string, FALSE);
}

/**
 * gimp_color_profile_is_equal:
 * @profile1: a #GimpColorProfile
 * @profile2: a #GimpColorProfile
 *
 * Compares two profiles.
 *
 * Return value: %TRUE if the profiles are equal, %FALSE otherwise.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_profile_is_equal (GimpColorProfile profile1,
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

/**
 * gimp_color_profile_is_rgb:
 * @profile: a #GimpColorProfile
 *
 * Return value: %TRUE if the profile's color space is RGB, %FALSE
 * otherwise.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_profile_is_rgb (GimpColorProfile profile)
{
  g_return_val_if_fail (profile != NULL, FALSE);

  return (cmsGetColorSpace (profile) == cmsSigRgbData);
}

/**
 * gimp_color_profile_is_cmyk:
 * @profile: a #GimpColorProfile
 *
 * Return value: %TRUE if the profile's color space is CMYK, %FALSE
 * otherwise.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_profile_is_cmyk (GimpColorProfile profile)
{
  g_return_val_if_fail (profile != NULL, FALSE);

  return (cmsGetColorSpace (profile) == cmsSigCmykData);
}

static void
gimp_color_profile_set_tag (cmsHPROFILE      profile,
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
gimp_color_profile_new_srgb_internal (void)
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

  gimp_color_profile_set_tag (srgb_profile, cmsSigProfileDescriptionTag,
                              "GIMP built-in sRGB");
  gimp_color_profile_set_tag (srgb_profile, cmsSigDeviceMfgDescTag,
                              "GIMP");
  gimp_color_profile_set_tag (srgb_profile, cmsSigDeviceModelDescTag,
                              "sRGB");
  gimp_color_profile_set_tag (srgb_profile, cmsSigCopyrightTag,
                              "Public Domain");

  /* The following line produces a V2 profile with a point curve TRC.
   * Profiles with point curve TRCs can't be used in LCMS2 unbounded
   * mode ICC profile conversions. A V2 profile might be appropriate
   * for embedding in sRGB images saved to disk, if the image is to be
   * opened by an image editing application that doesn't understand V4
   * profiles.
   *
   * cmsSetProfileVersion (srgb_profile, 2.1);
   */

  return srgb_profile;
}

/**
 * gimp_color_profile_new_srgb:
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
 * Since: 2.10
 **/
GimpColorProfile
gimp_color_profile_new_srgb (void)
{
  static guint8 *profile_data   = NULL;
  static gsize   profile_length = 0;

  if (G_UNLIKELY (profile_data == NULL))
    {
      GimpColorProfile profile;

      profile = gimp_color_profile_new_srgb_internal ();

      profile_data = gimp_color_profile_save_to_data (profile, &profile_length,
                                                      NULL);

      gimp_color_profile_close (profile);
    }

  return gimp_color_profile_open_from_data (profile_data, profile_length, NULL);
}

static GimpColorProfile
gimp_color_profile_new_linear_rgb_internal (void)
{
  cmsHPROFILE profile;

  /* white point is D65 from the sRGB specs */
  cmsCIExyY whitepoint = { 0.3127, 0.3290, 1.0 };

  /* primaries are ITU‐R BT.709‐5 (xYY), which are also the primaries
   * from the sRGB specs, modified to properly account for hexadecimal
   * quantization during the profile making process.
   */
  cmsCIExyYTRIPLE primaries =
    {
      /* R { 0.6400, 0.3300, 1.0 }, */
      /* G { 0.3000, 0.6000, 1.0 }, */
      /* B { 0.1500, 0.0600, 1.0 }  */
      /* R */ { 0.639998686, 0.330010138, 1.0 },
      /* G */ { 0.300003784, 0.600003357, 1.0 },
      /* B */ { 0.150002046, 0.059997204, 1.0 }
    };

  /* linear light */
  cmsToneCurve *linear[3];

  linear[0] = linear[1] = linear[2] = cmsBuildGamma (NULL, 1.0);

  /* create the profile, cleanup, and return */
  profile = cmsCreateRGBProfile (&whitepoint, &primaries, linear);
  cmsFreeToneCurve (linear[0]);

  gimp_color_profile_set_tag (profile, cmsSigProfileDescriptionTag,
                              "GIMP built-in Linear RGB");
  gimp_color_profile_set_tag (profile, cmsSigDeviceMfgDescTag,
                              "GIMP");
  gimp_color_profile_set_tag (profile, cmsSigDeviceModelDescTag,
                              "Linear RGB");
  gimp_color_profile_set_tag (profile, cmsSigCopyrightTag,
                              "Public Domain");

  return profile;
}

/**
 * gimp_color_profile_new_linear_rgb:
 *
 * This function creates a profile for babl_model("RGB"). Please
 * somebody write someting smarter here.
 *
 * Return value: the linear RGB cmsHPROFILE.
 *
 * Since: 2.10
 **/
GimpColorProfile
gimp_color_profile_new_linear_rgb (void)
{
  static guint8 *profile_data   = NULL;
  static gsize   profile_length = 0;

  if (G_UNLIKELY (profile_data == NULL))
    {
      GimpColorProfile profile;

      profile = gimp_color_profile_new_linear_rgb_internal ();

      profile_data = gimp_color_profile_save_to_data (profile, &profile_length,
                                                     NULL);

      gimp_color_profile_close (profile);
    }

  return gimp_color_profile_open_from_data (profile_data, profile_length, NULL);
}

/**
 * gimp_color_profile_get_format:
 * @format:      a #Babl format
 * @lcms_format: return location for an lcms format
 *
 * This function takes a #Babl format and returns the lcms format to
 * be used with that @format. It also returns a #Babl format to be
 * used instead of the passed @format, which usually is the same as
 * @format, unless lcms doesn't support @format.
 *
 * Note that this function currently only supports RGB, RGBA, R'G'B' and
 * R'G'B'A formats.
 *
 * Return value: the #Babl format to be used instead of @format, or %NULL
 *               is the passed @format is not supported at all.
 *
 * Since: 2.10
 **/
const Babl *
gimp_color_profile_get_format (const Babl *format,
                               guint32    *lcms_format)
{
  const Babl *output_format = NULL;
  const Babl *type;
  const Babl *model;
  gboolean    has_alpha;
  gboolean    linear;

  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (lcms_format != NULL, NULL);

  has_alpha = babl_format_has_alpha (format);
  type      = babl_format_get_type (format, 0);
  model     = babl_format_get_model (format);

  if (model == babl_model ("RGB") ||
      model == babl_model ("RGBA"))
    {
      linear = TRUE;
    }
  else if (model == babl_model ("R'G'B'") ||
           model == babl_model ("R'G'B'A"))
    {
      linear = FALSE;
    }
  else
    {
      g_return_val_if_reached (NULL);
    }

  *lcms_format = 0;

  if (type == babl_type ("u8"))
    {
      if (has_alpha)
        *lcms_format = TYPE_RGBA_8;
      else
        *lcms_format = TYPE_RGB_8;

      output_format = format;
    }
  else if (type == babl_type ("u16"))
    {
      if (has_alpha)
        *lcms_format = TYPE_RGBA_16;
      else
        *lcms_format = TYPE_RGB_16;

      output_format = format;
    }
  else if (type == babl_type ("half")) /* 16-bit floating point (half) */
    {
      if (has_alpha)
        *lcms_format = TYPE_RGBA_HALF_FLT;
      else
        *lcms_format = TYPE_RGB_HALF_FLT;

      output_format = format;
    }
  else if (type == babl_type ("float"))
    {
      if (has_alpha)
        *lcms_format = TYPE_RGBA_FLT;
      else
        *lcms_format = TYPE_RGB_FLT;

      output_format = format;
    }
  else if (type == babl_type ("double"))
    {
      if (has_alpha)
        {
#ifdef TYPE_RGBA_DBL
          /* RGBA double not implemented in lcms */
          *lcms_format = TYPE_RGBA_DBL;
          output_format = format;
#endif /* TYPE_RGBA_DBL */
        }
      else
        {
          *lcms_format = TYPE_RGB_DBL;
          output_format = format;
        }
    }

  if (*lcms_format == 0)
    {
      g_printerr ("%s: layer format %s not supported, "
                  "falling back to float\n",
                  G_STRFUNC, babl_get_name (format));

      if (has_alpha)
        {
          *lcms_format = TYPE_RGBA_FLT;

          if (linear)
            output_format = babl_format ("RGBA float");
          else
            output_format = babl_format ("R'G'B'A float");
        }
      else
        {
          *lcms_format = TYPE_RGB_FLT;

          if (linear)
            output_format = babl_format ("RGB float");
          else
            output_format = babl_format ("R'G'B' float");
        }
    }

  return output_format;
}
