/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpcolorprofile.c
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

#include "gimpcolortypes.h"

#include "gimpcolorprofile.h"

#include "libgimp/libgimp-intl.h"


#ifndef TYPE_RGBA_DBL
#define TYPE_RGBA_DBL       (FLOAT_SH(1)|COLORSPACE_SH(PT_RGB)|EXTRA_SH(1)|CHANNELS_SH(3)|BYTES_SH(0))
#endif

#ifndef TYPE_GRAYA_HALF_FLT
#define TYPE_GRAYA_HALF_FLT (FLOAT_SH(1)|COLORSPACE_SH(PT_GRAY)|EXTRA_SH(1)|CHANNELS_SH(1)|BYTES_SH(2))
#endif

#ifndef TYPE_GRAYA_FLT
#define TYPE_GRAYA_FLT      (FLOAT_SH(1)|COLORSPACE_SH(PT_GRAY)|EXTRA_SH(1)|CHANNELS_SH(1)|BYTES_SH(4))
#endif

#ifndef TYPE_GRAYA_DBL
#define TYPE_GRAYA_DBL      (FLOAT_SH(1)|COLORSPACE_SH(PT_GRAY)|EXTRA_SH(1)|CHANNELS_SH(1)|BYTES_SH(0))
#endif

#ifndef TYPE_CMYKA_DBL
#define TYPE_CMYKA_DBL      (FLOAT_SH(1)|COLORSPACE_SH(PT_CMYK)|EXTRA_SH(1)|CHANNELS_SH(4)|BYTES_SH(0))
#endif

#ifndef TYPE_CMYKA_HALF_FLT
#define TYPE_CMYKA_HALF_FLT (FLOAT_SH(1)|COLORSPACE_SH(PT_CMYK)|EXTRA_SH(1)|CHANNELS_SH(4)|BYTES_SH(2))
#endif

#ifndef TYPE_CMYKA_FLT
#define TYPE_CMYKA_FLT      (FLOAT_SH(1)|COLORSPACE_SH(PT_CMYK)|EXTRA_SH(1)|CHANNELS_SH(4)|BYTES_SH(4))
#endif

#ifndef TYPE_CMYKA_16
#define TYPE_CMYKA_16       (COLORSPACE_SH(PT_CMYK)|EXTRA_SH(1)|CHANNELS_SH(4)|BYTES_SH(2))
#endif


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


struct _GimpColorProfile
{
  GObject      parent_instance;

  cmsHPROFILE  lcms_profile;
  guint8      *data;
  gsize        length;

  gchar       *description;
  gchar       *manufacturer;
  gchar       *model;
  gchar       *copyright;
  gchar       *label;
  gchar       *summary;
};


static void   gimp_color_profile_finalize (GObject *object);


G_DEFINE_TYPE (GimpColorProfile, gimp_color_profile, G_TYPE_OBJECT)

#define parent_class gimp_color_profile_parent_class


#define GIMP_COLOR_PROFILE_ERROR gimp_color_profile_error_quark ()

static GQuark
gimp_color_profile_error_quark (void)
{
  static GQuark quark = 0;

  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("gimp-color-profile-error-quark");

  return quark;
}

static void
gimp_color_profile_class_init (GimpColorProfileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_color_profile_finalize;
}

static void
gimp_color_profile_init (GimpColorProfile *profile)
{
}

static void
gimp_color_profile_finalize (GObject *object)
{
  GimpColorProfile *profile = GIMP_COLOR_PROFILE (object);

  g_clear_pointer (&profile->lcms_profile, cmsCloseProfile);

  g_clear_pointer (&profile->data, g_free);
  profile->length = 0;

  g_clear_pointer (&profile->description,  g_free);
  g_clear_pointer (&profile->manufacturer, g_free);
  g_clear_pointer (&profile->model,        g_free);
  g_clear_pointer (&profile->copyright,    g_free);
  g_clear_pointer (&profile->label,        g_free);
  g_clear_pointer (&profile->summary,      g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/**
 * gimp_color_profile_new_from_file:
 * @file:  a #GFile
 * @error: return location for #GError
 *
 * This function opens an ICC color profile from @file.
 *
 * Returns: (nullable): the #GimpColorProfile, or %NULL. On error, %NULL is
 *               returned and @error is set.
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_profile_new_from_file (GFile   *file,
                                  GError **error)
{
  GimpColorProfile *profile      = NULL;
  cmsHPROFILE       lcms_profile = NULL;
  guint8           *data         = NULL;
  gsize             length       = 0;
  gchar            *path;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  path = g_file_get_path (file);

  if (path)
    {
      GMappedFile *mapped;

      mapped = g_mapped_file_new (path, FALSE, error);
      g_free (path);

      if (! mapped)
        return NULL;

      length = g_mapped_file_get_length (mapped);
      data   = g_memdup2 (g_mapped_file_get_contents (mapped), length);

      lcms_profile = cmsOpenProfileFromMem (data, length);

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

          length = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_STANDARD_SIZE);
          data   = g_malloc (length);

          g_object_unref (info);

          input = G_INPUT_STREAM (g_file_read (file, NULL, error));

          if (input)
            {
              gsize bytes_read;

              if (g_input_stream_read_all (input, data, length,
                                           &bytes_read, NULL, error) &&
                  bytes_read == length)
                {
                  lcms_profile = cmsOpenProfileFromMem (data, length);
                }

              g_object_unref (input);
            }
        }
    }

  if (lcms_profile)
    {
      profile = g_object_new (GIMP_TYPE_COLOR_PROFILE, NULL);

      profile->lcms_profile = lcms_profile;
      profile->data         = data;
      profile->length       = length;
    }
  else
    {
      if (data)
        g_free (data);

      if (error && *error == NULL)
        {
          g_set_error (error, GIMP_COLOR_PROFILE_ERROR, 0,
                       _("'%s' does not appear to be an ICC color profile"),
                       gimp_file_get_utf8_name (file));
        }
    }

  return profile;
}

/**
 * gimp_color_profile_new_from_icc_profile:
 * @data: (array length=length): The memory containing an ICC profile
 * @length: length of the profile in memory, in bytes
 * @error:  return location for #GError
 *
 * This function opens an ICC color profile from memory. On error,
 * %NULL is returned and @error is set.
 *
 * Returns: (nullable): the #GimpColorProfile, or %NULL.
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_profile_new_from_icc_profile (const guint8  *data,
                                         gsize          length,
                                         GError       **error)
{
  cmsHPROFILE       lcms_profile = 0;
  GimpColorProfile *profile      = NULL;

  g_return_val_if_fail (data != NULL || length == 0, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (length > 0)
    lcms_profile = cmsOpenProfileFromMem (data, length);

  if (lcms_profile)
    {
      profile = g_object_new (GIMP_TYPE_COLOR_PROFILE, NULL);

      profile->lcms_profile = lcms_profile;
      profile->data         = g_memdup2 (data, length);
      profile->length       = length;
   }
  else
    {
      g_set_error_literal (error, GIMP_COLOR_PROFILE_ERROR, 0,
                           _("Data does not appear to be an ICC color profile"));
    }

  return profile;
}

/**
 * gimp_color_profile_new_from_lcms_profile:
 * @lcms_profile: an LCMS cmsHPROFILE pointer
 * @error:        return location for #GError
 *
 * This function creates a GimpColorProfile from a cmsHPROFILE. On
 * error, %NULL is returned and @error is set. The passed
 * @lcms_profile pointer is not retained by the created
 * #GimpColorProfile.
 *
 * Returns: (nullable): the #GimpColorProfile, or %NULL.
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_profile_new_from_lcms_profile (gpointer   lcms_profile,
                                          GError   **error)
{
  cmsUInt32Number size;

  g_return_val_if_fail (lcms_profile != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (cmsSaveProfileToMem (lcms_profile, NULL, &size))
    {
      guint8 *data = g_malloc (size);

      if (cmsSaveProfileToMem (lcms_profile, data, &size))
        {
          gsize length = size;

          lcms_profile = cmsOpenProfileFromMem (data, length);

          if (lcms_profile)
            {
              GimpColorProfile *profile;

              profile = g_object_new (GIMP_TYPE_COLOR_PROFILE, NULL);

              profile->lcms_profile = lcms_profile;
              profile->data         = data;
              profile->length       = length;

              return profile;
            }
        }

      g_free (data);
    }

  g_set_error_literal (error, GIMP_COLOR_PROFILE_ERROR, 0,
                       _("Could not save color profile to memory"));

  return NULL;
}

/**
 * gimp_color_profile_save_to_file:
 * @profile: a #GimpColorProfile
 * @file:    a #GFile
 * @error:   return location for #GError
 *
 * This function saves @profile to @file as ICC profile.
 *
 * Returns: %TRUE on success, %FALSE if an error occurred.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_profile_save_to_file (GimpColorProfile  *profile,
                                 GFile             *file,
                                 GError           **error)
{
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return g_file_replace_contents (file,
                                  (const gchar *) profile->data,
                                  profile->length,
                                  NULL, FALSE,
                                  G_FILE_CREATE_NONE,
                                  NULL,
                                  NULL,
                                  error);
}

/**
 * gimp_color_profile_get_icc_profile:
 * @profile: a #GimpColorProfile
 * @length: (out): return location for the number of bytes
 *
 * This function returns @profile as ICC profile data. The returned
 * memory belongs to @profile and must not be modified or freed.
 *
 * Returns: (array length=length): a pointer to the IIC profile data.
 *
 * Since: 2.10
 **/
const guint8 *
gimp_color_profile_get_icc_profile (GimpColorProfile  *profile,
                                    gsize             *length)
{
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), NULL);
  g_return_val_if_fail (length != NULL, NULL);

  *length = profile->length;

  return profile->data;
}

/**
 * gimp_color_profile_get_lcms_profile:
 * @profile: a #GimpColorProfile
 *
 * This function returns @profile's cmsHPROFILE. The returned
 * value belongs to @profile and must not be modified or freed.
 *
 * Returns: a pointer to the cmsHPROFILE.
 *
 * Since: 2.10
 **/
gpointer
gimp_color_profile_get_lcms_profile (GimpColorProfile *profile)
{
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), NULL);

  return profile->lcms_profile;
}

static gchar *
gimp_color_profile_get_info (GimpColorProfile *profile,
                             cmsInfoType       info)
{
  cmsUInt32Number  size;
  gchar           *text = NULL;

  size = cmsGetProfileInfoASCII (profile->lcms_profile, info,
                                 "en", "US", NULL, 0);
  if (size > 0)
    {
      gchar *data = g_new (gchar, size + 1);

      size = cmsGetProfileInfoASCII (profile->lcms_profile, info,
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
 * Returns: a string containing @profile's description. The
 *               returned value belongs to @profile and must not be
 *               modified or freed.
 *
 * Since: 2.10
 **/
const gchar *
gimp_color_profile_get_description (GimpColorProfile *profile)
{
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), NULL);

  if (! profile->description)
    profile->description =
      gimp_color_profile_get_info (profile, cmsInfoDescription);

  return profile->description;
}

/**
 * gimp_color_profile_get_manufacturer:
 * @profile: a #GimpColorProfile
 *
 * Returns: a string containing @profile's manufacturer. The
 *               returned value belongs to @profile and must not be
 *               modified or freed.
 *
 * Since: 2.10
 **/
const gchar *
gimp_color_profile_get_manufacturer (GimpColorProfile *profile)
{
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), NULL);

  if (! profile->manufacturer)
    profile->manufacturer =
      gimp_color_profile_get_info (profile, cmsInfoManufacturer);

  return profile->manufacturer;
}

/**
 * gimp_color_profile_get_model:
 * @profile: a #GimpColorProfile
 *
 * Returns: a string containing @profile's model. The returned
 *               value belongs to @profile and must not be modified or
 *               freed.
 *
 * Since: 2.10
 **/
const gchar *
gimp_color_profile_get_model (GimpColorProfile *profile)
{
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), NULL);

  if (! profile->model)
    profile->model =
      gimp_color_profile_get_info (profile, cmsInfoModel);

  return profile->model;
}

/**
 * gimp_color_profile_get_copyright:
 * @profile: a #GimpColorProfile
 *
 * Returns: a string containing @profile's copyright. The
 *               returned value belongs to @profile and must not be
 *               modified or freed.
 *
 * Since: 2.10
 **/
const gchar *
gimp_color_profile_get_copyright (GimpColorProfile *profile)
{
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), NULL);

  if (! profile->copyright)
    profile->copyright =
      gimp_color_profile_get_info (profile, cmsInfoCopyright);

  return profile->copyright;
}

/**
 * gimp_color_profile_get_label:
 * @profile: a #GimpColorProfile
 *
 * This function returns a string containing @profile's "title", a
 * string that can be used to label the profile in a user interface.
 *
 * Unlike gimp_color_profile_get_description(), this function always
 * returns a string (as a fallback, it returns "(unnamed profile)").
 *
 * Returns: the @profile's label. The returned value belongs to
 *               @profile and must not be modified or freed.
 *
 * Since: 2.10
 **/
const gchar *
gimp_color_profile_get_label (GimpColorProfile *profile)
{

  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), NULL);

  if (! profile->label)
    {
      const gchar *label = gimp_color_profile_get_description (profile);

      if (! label || ! strlen (label))
        label = _("(unnamed profile)");

      profile->label = g_strdup (label);
    }

  return profile->label;
}

/**
 * gimp_color_profile_get_summary:
 * @profile: a #GimpColorProfile
 *
 * This function return a string containing a multi-line summary of
 * @profile's description, model, manufacturer and copyright, to be
 * used as detailed information about the profile in a user
 * interface.
 *
 * Returns: the @profile's summary. The returned value belongs to
 *               @profile and must not be modified or freed.
 *
 * Since: 2.10
 **/
const gchar *
gimp_color_profile_get_summary (GimpColorProfile *profile)
{
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), NULL);

  if (! profile->summary)
    {
      GString     *string = g_string_new (NULL);
      const gchar *text;

      text = gimp_color_profile_get_description (profile);
      if (text)
        g_string_append (string, text);

      text = gimp_color_profile_get_model (profile);
      if (text)
        {
          if (string->len > 0)
            g_string_append (string, "\n");

          g_string_append_printf (string, _("Model: %s"), text);
        }

      text = gimp_color_profile_get_manufacturer (profile);
      if (text)
        {
          if (string->len > 0)
            g_string_append (string, "\n");

          g_string_append_printf (string, _("Manufacturer: %s"), text);
        }

      text = gimp_color_profile_get_copyright (profile);
      if (text)
        {
          if (string->len > 0)
            g_string_append (string, "\n");

          g_string_append_printf (string, _("Copyright: %s"), text);
        }

      profile->summary = g_string_free (string, FALSE);
    }

  return profile->summary;
}

/**
 * gimp_color_profile_is_equal:
 * @profile1: a #GimpColorProfile
 * @profile2: a #GimpColorProfile
 *
 * Compares two profiles.
 *
 * Returns: %TRUE if the profiles are equal, %FALSE otherwise.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_profile_is_equal (GimpColorProfile *profile1,
                             GimpColorProfile *profile2)
{
  const gsize header_len = sizeof (cmsICCHeader);

  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile1), FALSE);
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile2), FALSE);

  return profile1 == profile2                              ||
         (profile1->length == profile2->length &&
          memcmp (profile1->data + header_len,
                  profile2->data + header_len,
                  profile1->length - header_len) == 0);
}

/**
 * gimp_color_profile_is_rgb:
 * @profile: a #GimpColorProfile
 *
 * Returns: %TRUE if the profile's color space is RGB, %FALSE
 * otherwise.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_profile_is_rgb (GimpColorProfile *profile)
{
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), FALSE);

  return (cmsGetColorSpace (profile->lcms_profile) == cmsSigRgbData);
}

/**
 * gimp_color_profile_is_gray:
 * @profile: a #GimpColorProfile
 *
 * Returns: %TRUE if the profile's color space is grayscale, %FALSE
 * otherwise.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_profile_is_gray (GimpColorProfile *profile)
{
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), FALSE);

  return (cmsGetColorSpace (profile->lcms_profile) == cmsSigGrayData);
}

/**
 * gimp_color_profile_is_cmyk:
 * @profile: a #GimpColorProfile
 *
 * Returns: %TRUE if the profile's color space is CMYK, %FALSE
 * otherwise.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_profile_is_cmyk (GimpColorProfile *profile)
{
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), FALSE);

  return (cmsGetColorSpace (profile->lcms_profile) == cmsSigCmykData);
}


/**
 * gimp_color_profile_is_linear:
 * @profile: a #GimpColorProfile
 *
 * This function determines is the ICC profile represented by a GimpColorProfile
 * is a linear RGB profile or not, some profiles that are LUTs though linear
 * will also return FALSE;
 *
 * Returns: %TRUE if the profile is a matrix shaping profile with linear
 * TRCs, %FALSE otherwise.
 *
 * Since: 2.10
 **/
gboolean
gimp_color_profile_is_linear (GimpColorProfile *profile)
{
  cmsHPROFILE   prof;
  cmsToneCurve *curve;

  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), FALSE);

  prof = profile->lcms_profile;

  if (! cmsIsMatrixShaper (prof))
    return FALSE;

  if (cmsIsCLUT (prof, INTENT_PERCEPTUAL, LCMS_USED_AS_INPUT))
    return FALSE;

  if (cmsIsCLUT (prof, INTENT_PERCEPTUAL, LCMS_USED_AS_OUTPUT))
    return FALSE;

  if (gimp_color_profile_is_rgb (profile))
    {
      curve = cmsReadTag(prof, cmsSigRedTRCTag);
      if (curve == NULL || ! cmsIsToneCurveLinear (curve))
        return FALSE;

      curve = cmsReadTag (prof, cmsSigGreenTRCTag);
      if (curve == NULL || ! cmsIsToneCurveLinear (curve))
        return FALSE;

      curve = cmsReadTag (prof, cmsSigBlueTRCTag);
      if (curve == NULL || ! cmsIsToneCurveLinear (curve))
        return FALSE;
    }
  else if (gimp_color_profile_is_gray (profile))
    {
      curve = cmsReadTag(prof, cmsSigGrayTRCTag);
      if (curve == NULL || ! cmsIsToneCurveLinear (curve))
        return FALSE;
    }
  else
    {
      return FALSE;
    }

  return TRUE;
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

static gboolean
gimp_color_profile_get_rgb_matrix_colorants (GimpColorProfile *profile,
                                             GimpMatrix3      *matrix)
{
  cmsHPROFILE  lcms_profile;
  cmsCIEXYZ   *red;
  cmsCIEXYZ   *green;
  cmsCIEXYZ   *blue;

  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), FALSE);

  lcms_profile = profile->lcms_profile;

  red   = cmsReadTag (lcms_profile, cmsSigRedColorantTag);
  green = cmsReadTag (lcms_profile, cmsSigGreenColorantTag);
  blue  = cmsReadTag (lcms_profile, cmsSigBlueColorantTag);

  if (red && green && blue)
    {
      if (matrix)
        {
          matrix->coeff[0][0] = red->X;
          matrix->coeff[0][1] = red->Y;
          matrix->coeff[0][2] = red->Z;

          matrix->coeff[1][0] = green->X;
          matrix->coeff[1][1] = green->Y;
          matrix->coeff[1][2] = green->Z;

          matrix->coeff[2][0] = blue->X;
          matrix->coeff[2][1] = blue->Y;
          matrix->coeff[2][2] = blue->Z;
        }

      return TRUE;
    }

  return FALSE;
}

static void
gimp_color_profile_make_tag (cmsHPROFILE       profile,
                             cmsTagSignature   sig,
                             const gchar      *gimp_tag,
                             const gchar      *gimp_prefix,
                             const gchar      *gimp_prefix_alt,
                             const gchar      *original_tag)
{
  if (! original_tag || ! strlen (original_tag) ||
      ! strcmp (original_tag, gimp_tag))
    {
      /* if there is no original tag (or it is the same as the new
       * tag), just use the new tag
       */

      gimp_color_profile_set_tag (profile, sig, gimp_tag);
    }
  else
    {
      /* otherwise prefix the existing tag with a gimp prefix
       * indicating that the profile has been generated
       */

      if (g_str_has_prefix (original_tag, gimp_prefix))
        {
          /* don't add multiple GIMP prefixes */
          gimp_color_profile_set_tag (profile, sig, original_tag);
        }
      else if (gimp_prefix_alt &&
               g_str_has_prefix (original_tag, gimp_prefix_alt))
        {
          /* replace GIMP prefix_alt by prefix */
          gchar *new_tag = g_strconcat (gimp_prefix,
                                        original_tag + strlen (gimp_prefix_alt),
                                        NULL);

          gimp_color_profile_set_tag (profile, sig, new_tag);
          g_free (new_tag);
        }
      else
        {
          gchar *new_tag = g_strconcat (gimp_prefix,
                                        original_tag,
                                        NULL);

          gimp_color_profile_set_tag (profile, sig, new_tag);
          g_free (new_tag);
        }
    }
}

static GimpColorProfile *
gimp_color_profile_new_from_color_profile (GimpColorProfile *profile,
                                           gboolean          linear)
{
  GimpColorProfile *new_profile;
  cmsHPROFILE       target_profile;
  GimpMatrix3       matrix = { { { 0, } } };
  cmsCIEXYZ        *whitepoint;
  cmsToneCurve     *curve;

  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), NULL);

  if (gimp_color_profile_is_rgb (profile))
    {
      if (! gimp_color_profile_get_rgb_matrix_colorants (profile, &matrix))
        return NULL;
    }
  else if (! gimp_color_profile_is_gray (profile))
    {
      return NULL;
    }

  whitepoint = cmsReadTag (profile->lcms_profile,
                           cmsSigMediaWhitePointTag);

  target_profile = cmsCreateProfilePlaceholder (0);

  cmsSetProfileVersion (target_profile, 4.3);
  cmsSetDeviceClass (target_profile, cmsSigDisplayClass);
  cmsSetPCS (target_profile, cmsSigXYZData);

  cmsWriteTag (target_profile, cmsSigMediaWhitePointTag, whitepoint);

  if (linear)
    {
      /* linear light */
      curve = cmsBuildGamma (NULL, 1.00);

      gimp_color_profile_make_tag (target_profile, cmsSigProfileDescriptionTag,
                                   "linear TRC from unnamed profile",
                                   "linear TRC from ",
                                   "sRGB TRC from ",
                                   gimp_color_profile_get_description (profile));
    }
  else
    {
      cmsFloat64Number srgb_parameters[5] =
        { 2.4, 1.0 / 1.055,  0.055 / 1.055, 1.0 / 12.92, 0.04045 };

      /* sRGB curve */
      curve = cmsBuildParametricToneCurve (NULL, 4, srgb_parameters);

      gimp_color_profile_make_tag (target_profile, cmsSigProfileDescriptionTag,
                                   "sRGB TRC from unnamed profile",
                                   "sRGB TRC from ",
                                   "linear TRC from ",
                                   gimp_color_profile_get_description (profile));
    }

  if (gimp_color_profile_is_rgb (profile))
    {
      cmsCIEXYZ red;
      cmsCIEXYZ green;
      cmsCIEXYZ blue;

      cmsSetColorSpace (target_profile, cmsSigRgbData);

      red.X = matrix.coeff[0][0];
      red.Y = matrix.coeff[0][1];
      red.Z = matrix.coeff[0][2];

      green.X = matrix.coeff[1][0];
      green.Y = matrix.coeff[1][1];
      green.Z = matrix.coeff[1][2];

      blue.X = matrix.coeff[2][0];
      blue.Y = matrix.coeff[2][1];
      blue.Z = matrix.coeff[2][2];

      cmsWriteTag (target_profile, cmsSigRedColorantTag,   &red);
      cmsWriteTag (target_profile, cmsSigGreenColorantTag, &green);
      cmsWriteTag (target_profile, cmsSigBlueColorantTag,  &blue);

      cmsWriteTag (target_profile, cmsSigRedTRCTag,   curve);
      cmsWriteTag (target_profile, cmsSigGreenTRCTag, curve);
      cmsWriteTag (target_profile, cmsSigBlueTRCTag,  curve);
    }
  else
    {
      cmsSetColorSpace (target_profile, cmsSigGrayData);

      cmsWriteTag (target_profile, cmsSigGrayTRCTag, curve);
    }

  cmsFreeToneCurve (curve);

  gimp_color_profile_make_tag (target_profile, cmsSigDeviceMfgDescTag,
                               "GIMP",
                               "GIMP from ", NULL,
                               gimp_color_profile_get_manufacturer (profile));
  gimp_color_profile_make_tag (target_profile, cmsSigDeviceModelDescTag,
                               "Generated by GIMP",
                               "GIMP from ", NULL,
                               gimp_color_profile_get_model (profile));
  gimp_color_profile_make_tag (target_profile, cmsSigCopyrightTag,
                               "Public Domain",
                               "GIMP from ", NULL,
                               gimp_color_profile_get_copyright (profile));

  new_profile = gimp_color_profile_new_from_lcms_profile (target_profile, NULL);

  cmsCloseProfile (target_profile);

  return new_profile;
}

/**
 * gimp_color_profile_new_srgb_trc_from_color_profile:
 * @profile: a #GimpColorProfile
 *
 * This function creates a new RGB #GimpColorProfile with a sRGB gamma
 * TRC and @profile's RGB chromacities and whitepoint.
 *
 * Returns: (nullable) (transfer full): the new #GimpColorProfile, or %NULL if
 *               @profile is not an RGB profile or not matrix-based.
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_profile_new_srgb_trc_from_color_profile (GimpColorProfile *profile)
{
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), NULL);

  return gimp_color_profile_new_from_color_profile (profile, FALSE);
}

/**
 * gimp_color_profile_new_linear_from_color_profile:
 * @profile: a #GimpColorProfile
 *
 * This function creates a new RGB #GimpColorProfile with a linear TRC
 * and @profile's RGB chromacities and whitepoint.
 *
 * Returns: (nullable) (transfer full): the new #GimpColorProfile, or %NULL if
 *               @profile is not an RGB profile or not matrix-based.
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_profile_new_linear_from_color_profile (GimpColorProfile *profile)
{
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), NULL);

  return gimp_color_profile_new_from_color_profile (profile, TRUE);
}

static cmsHPROFILE *
gimp_color_profile_new_rgb_srgb_internal (void)
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

  cmsFloat64Number srgb_parameters[5] =
    { 2.4, 1.0 / 1.055,  0.055 / 1.055, 1.0 / 12.92, 0.04045 };

  cmsToneCurve *curve[3];

  /* sRGB curve */
  curve[0] = curve[1] = curve[2] = cmsBuildParametricToneCurve (NULL, 4,
                                                                srgb_parameters);

  profile = cmsCreateRGBProfile (&whitepoint, &primaries, curve);

  cmsFreeToneCurve (curve[0]);

  gimp_color_profile_set_tag (profile, cmsSigProfileDescriptionTag,
                              "GIMP built-in sRGB");
  gimp_color_profile_set_tag (profile, cmsSigDeviceMfgDescTag,
                              "GIMP");
  gimp_color_profile_set_tag (profile, cmsSigDeviceModelDescTag,
                              "sRGB");
  gimp_color_profile_set_tag (profile, cmsSigCopyrightTag,
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

  return profile;
}

/**
 * gimp_color_profile_new_rgb_srgb:
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
 * Returns: the sRGB #GimpColorProfile.
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_profile_new_rgb_srgb (void)
{
  static GimpColorProfile *profile = NULL;

  const guint8 *data;
  gsize         length = 0;

  if (G_UNLIKELY (profile == NULL))
    {
      cmsHPROFILE lcms_profile = gimp_color_profile_new_rgb_srgb_internal ();

      profile = gimp_color_profile_new_from_lcms_profile (lcms_profile, NULL);

      cmsCloseProfile (lcms_profile);
    }

  data = gimp_color_profile_get_icc_profile (profile, &length);

  return gimp_color_profile_new_from_icc_profile (data, length, NULL);
}

static cmsHPROFILE
gimp_color_profile_new_rgb_srgb_linear_internal (void)
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

  cmsToneCurve *curve[3];

  /* linear light */
  curve[0] = curve[1] = curve[2] = cmsBuildGamma (NULL, 1.0);

  profile = cmsCreateRGBProfile (&whitepoint, &primaries, curve);

  cmsFreeToneCurve (curve[0]);

  gimp_color_profile_set_tag (profile, cmsSigProfileDescriptionTag,
                              "GIMP built-in Linear sRGB");
  gimp_color_profile_set_tag (profile, cmsSigDeviceMfgDescTag,
                              "GIMP");
  gimp_color_profile_set_tag (profile, cmsSigDeviceModelDescTag,
                              "Linear sRGB");
  gimp_color_profile_set_tag (profile, cmsSigCopyrightTag,
                              "Public Domain");

  return profile;
}

/**
 * gimp_color_profile_new_rgb_srgb_linear:
 *
 * This function creates a profile for babl_model("RGB"). Please
 * somebody write something smarter here.
 *
 * Returns: the linear RGB #GimpColorProfile.
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_profile_new_rgb_srgb_linear (void)
{
  static GimpColorProfile *profile = NULL;

  const guint8 *data;
  gsize         length = 0;

  if (G_UNLIKELY (profile == NULL))
    {
      cmsHPROFILE lcms_profile = gimp_color_profile_new_rgb_srgb_linear_internal ();

      profile = gimp_color_profile_new_from_lcms_profile (lcms_profile, NULL);

      cmsCloseProfile (lcms_profile);
    }

  data = gimp_color_profile_get_icc_profile (profile, &length);

  return gimp_color_profile_new_from_icc_profile (data, length, NULL);
}

static cmsHPROFILE *
gimp_color_profile_new_rgb_adobe_internal (void)
{
  cmsHPROFILE profile;

  /* white point is D65 from the sRGB specs */
  cmsCIExyY whitepoint = { 0.3127, 0.3290, 1.0 };

  /* AdobeRGB1998 and sRGB have the same white point.
   *
   * The primaries below are technically correct, but because of
   * hexadecimal rounding these primaries don't make a profile that
   * matches the original.
   *
   *  cmsCIExyYTRIPLE primaries = {
   *    { 0.6400, 0.3300, 1.0 },
   *    { 0.2100, 0.7100, 1.0 },
   *    { 0.1500, 0.0600, 1.0 }
   *  };
   */
  cmsCIExyYTRIPLE primaries =
    {
      { 0.639996511, 0.329996864, 1.0 },
      { 0.210005295, 0.710004866, 1.0 },
      { 0.149997606, 0.060003644, 1.0 }
    };

  cmsToneCurve *curve[3];

  /* gamma 2.2 */
  curve[0] = curve[1] = curve[2] = cmsBuildGamma (NULL, 2.19921875);

  profile = cmsCreateRGBProfile (&whitepoint, &primaries, curve);

  cmsFreeToneCurve (curve[0]);

  gimp_color_profile_set_tag (profile, cmsSigProfileDescriptionTag,
                              "Compatible with Adobe RGB (1998)");
  gimp_color_profile_set_tag (profile, cmsSigDeviceMfgDescTag,
                              "GIMP");
  gimp_color_profile_set_tag (profile, cmsSigDeviceModelDescTag,
                              "Compatible with Adobe RGB (1998)");
  gimp_color_profile_set_tag (profile, cmsSigCopyrightTag,
                              "Public Domain");

  return profile;
}

/**
 * gimp_color_profile_new_rgb_adobe:
 *
 * This function creates a profile compatible with AbobeRGB (1998).
 *
 * Returns: the AdobeRGB-compatible #GimpColorProfile.
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_profile_new_rgb_adobe (void)
{
  static GimpColorProfile *profile = NULL;

  const guint8 *data;
  gsize         length = 0;

  if (G_UNLIKELY (profile == NULL))
    {
      cmsHPROFILE lcms_profile = gimp_color_profile_new_rgb_adobe_internal ();

      profile = gimp_color_profile_new_from_lcms_profile (lcms_profile, NULL);

      cmsCloseProfile (lcms_profile);
    }

  data = gimp_color_profile_get_icc_profile (profile, &length);

  return gimp_color_profile_new_from_icc_profile (data, length, NULL);
}

static cmsHPROFILE *
gimp_color_profile_new_d65_gray_srgb_trc_internal (void)
{
  cmsHPROFILE profile;

  /* white point is D65 from the sRGB specs */
  cmsCIExyY whitepoint = { 0.3127, 0.3290, 1.0 };

  cmsFloat64Number srgb_parameters[5] =
    { 2.4, 1.0 / 1.055,  0.055 / 1.055, 1.0 / 12.92, 0.04045 };

  cmsToneCurve *curve = cmsBuildParametricToneCurve (NULL, 4,
                                                     srgb_parameters);

  profile = cmsCreateGrayProfile (&whitepoint, curve);

  cmsFreeToneCurve (curve);

  gimp_color_profile_set_tag (profile, cmsSigProfileDescriptionTag,
                              "GIMP built-in D65 Grayscale with sRGB TRC");
  gimp_color_profile_set_tag (profile, cmsSigDeviceMfgDescTag,
                              "GIMP");
  gimp_color_profile_set_tag (profile, cmsSigDeviceModelDescTag,
                              "D65 Grayscale with sRGB TRC");
  gimp_color_profile_set_tag (profile, cmsSigCopyrightTag,
                              "Public Domain");

  return profile;
}

/**
 * gimp_color_profile_new_d65_gray_srgb_trc
 *
 * This function creates a grayscale #GimpColorProfile with an
 * sRGB TRC. See gimp_color_profile_new_rgb_srgb().
 *
 * Returns: the sRGB-gamma grayscale #GimpColorProfile.
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_profile_new_d65_gray_srgb_trc (void)
{
  static GimpColorProfile *profile = NULL;

  const guint8 *data;
  gsize         length = 0;

  if (G_UNLIKELY (profile == NULL))
    {
      cmsHPROFILE lcms_profile = gimp_color_profile_new_d65_gray_srgb_trc_internal ();

      profile = gimp_color_profile_new_from_lcms_profile (lcms_profile, NULL);

      cmsCloseProfile (lcms_profile);
    }

  data = gimp_color_profile_get_icc_profile (profile, &length);

  return gimp_color_profile_new_from_icc_profile (data, length, NULL);
}

static cmsHPROFILE
gimp_color_profile_new_d65_gray_linear_internal (void)
{
  cmsHPROFILE profile;

  /* white point is D65 from the sRGB specs */
  cmsCIExyY whitepoint = { 0.3127, 0.3290, 1.0 };

  cmsToneCurve *curve = cmsBuildGamma (NULL, 1.0);

  profile = cmsCreateGrayProfile (&whitepoint, curve);

  cmsFreeToneCurve (curve);

  gimp_color_profile_set_tag (profile, cmsSigProfileDescriptionTag,
                              "GIMP built-in D65 Linear Grayscale");
  gimp_color_profile_set_tag (profile, cmsSigDeviceMfgDescTag,
                              "GIMP");
  gimp_color_profile_set_tag (profile, cmsSigDeviceModelDescTag,
                              "D65 Linear Grayscale");
  gimp_color_profile_set_tag (profile, cmsSigCopyrightTag,
                              "Public Domain");

  return profile;
}

/**
 * gimp_color_profile_new_d65_gray_srgb_gray:
 *
 * This function creates a profile for babl_model("Y"). Please
 * somebody write something smarter here.
 *
 * Returns: the linear grayscale #GimpColorProfile.
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_profile_new_d65_gray_linear (void)
{
  static GimpColorProfile *profile = NULL;

  const guint8 *data;
  gsize         length = 0;

  if (G_UNLIKELY (profile == NULL))
    {
      cmsHPROFILE lcms_profile = gimp_color_profile_new_d65_gray_linear_internal ();

      profile = gimp_color_profile_new_from_lcms_profile (lcms_profile, NULL);

      cmsCloseProfile (lcms_profile);
    }

  data = gimp_color_profile_get_icc_profile (profile, &length);

  return gimp_color_profile_new_from_icc_profile (data, length, NULL);
}

static cmsHPROFILE *
gimp_color_profile_new_d50_gray_lab_trc_internal (void)
{
  cmsHPROFILE profile;

  /* white point is D50 from the ICC profile illuminant specs */
  cmsCIExyY whitepoint = {0.345702915, 0.358538597, 1.0};

  cmsFloat64Number lab_parameters[5] =
    { 3.0, 1.0 / 1.16,  0.16 / 1.16, 2700.0 / 24389.0, 0.08000  };

  cmsToneCurve *curve = cmsBuildParametricToneCurve (NULL, 4,
                                                     lab_parameters);

  profile = cmsCreateGrayProfile (&whitepoint, curve);

  cmsFreeToneCurve (curve);

  gimp_color_profile_set_tag (profile, cmsSigProfileDescriptionTag,
                              "GIMP built-in D50 Grayscale with LAB L TRC");
  gimp_color_profile_set_tag (profile, cmsSigDeviceMfgDescTag,
                              "GIMP");
  gimp_color_profile_set_tag (profile, cmsSigDeviceModelDescTag,
                              "D50 Grayscale with LAB L TRC");
  gimp_color_profile_set_tag (profile, cmsSigCopyrightTag,
                              "Public Domain");

  return profile;
}


/**
 * gimp_color_profile_new_d50_gray_lab_trc
 *
 * This function creates a grayscale #GimpColorProfile with the
 * D50 ICC profile illuminant as the profile white point and the
 * LAB companding curve as the TRC.
 *
 * Returns: a gray profile with the D50 ICC profile illuminant
 * as the profile white point and the LAB companding curve as the TRC.
 * as the TRC.
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_profile_new_d50_gray_lab_trc (void)
{
  static GimpColorProfile *profile = NULL;

  const guint8 *data;
  gsize         length = 0;

  if (G_UNLIKELY (profile == NULL))
    {
      cmsHPROFILE lcms_profile = gimp_color_profile_new_d50_gray_lab_trc_internal ();

      profile = gimp_color_profile_new_from_lcms_profile (lcms_profile, NULL);

      cmsCloseProfile (lcms_profile);
    }

  data = gimp_color_profile_get_icc_profile (profile, &length);

  return gimp_color_profile_new_from_icc_profile (data, length, NULL);
}

/**
 * gimp_color_profile_get_space:
 * @profile: a #GimpColorProfile
 * @intent:  a #GimpColorRenderingIntent
 * @error:   return location for #GError
 *
 * This function returns the #Babl space of @profile, for the
 * specified @intent.
 *
 * Returns: the new #Babl space.
 *
 * Since: 2.10.6
 **/
const Babl *
gimp_color_profile_get_space (GimpColorProfile          *profile,
                              GimpColorRenderingIntent   intent,
                              GError                   **error)
{
  const Babl  *space;
  const gchar *babl_error = NULL;

  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  space = babl_space_from_icc ((const gchar *) profile->data,
                               profile->length,
                               (BablIccIntent) intent,
                               &babl_error);

  if (! space)
    g_set_error (error, GIMP_COLOR_PROFILE_ERROR, 0,
                 "%s: %s",
                 gimp_color_profile_get_label (profile), babl_error);

  return space;
}

/**
 * gimp_color_profile_get_format:
 * @profile: a #GimpColorProfile
 * @format:  a #Babl format
 * @intent:  a #GimpColorRenderingIntent
 * @error:   return location for #GError
 *
 * This function takes a #GimpColorProfile and a #Babl format and
 * returns a new #Babl format with @profile's RGB primaries and TRC,
 * and @format's pixel layout.
 *
 * Returns: the new #Babl format.
 *
 * Since: 2.10
 **/
const Babl *
gimp_color_profile_get_format (GimpColorProfile          *profile,
                               const Babl                *format,
                               GimpColorRenderingIntent   intent,
                               GError                   **error)
{
  const Babl *space;

  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), NULL);
  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  space = gimp_color_profile_get_space (profile, intent, error);

  if (! space)
    return NULL;

  return babl_format_with_space ((const gchar *) format, space);
}

/**
 * gimp_color_profile_get_lcms_format:
 * @format:      a #Babl format
 * @lcms_format: (out): return location for an lcms format
 *
 * This function takes a #Babl format and returns the lcms format to
 * be used with that @format. It also returns a #Babl format to be
 * used instead of the passed @format, which usually is the same as
 * @format, unless lcms doesn't support @format.
 *
 * Note that this function currently only supports RGB, RGBA, R'G'B',
 * R'G'B'A, Y, YA, Y', Y'A and the cairo-RGB24 and cairo-ARGB32 formats.
 *
 * Returns: (nullable): the #Babl format to be used instead of @format, or %NULL
 *               if the passed @format is not supported at all.
 *
 * Since: 2.10
 **/
const Babl *
gimp_color_profile_get_lcms_format (const Babl *format,
                                    guint32    *lcms_format)
{
  const Babl *output_format = NULL;
  const Babl *type;
  const Babl *model;
  const Babl *space;
  gboolean    has_alpha;
  gboolean    rgb      = FALSE;
  gboolean    gray     = FALSE;
  gboolean    cmyk     = FALSE;
  gboolean    linear   = FALSE;
  gboolean    srgb_trc = FALSE;

  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (lcms_format != NULL, NULL);

  has_alpha = babl_format_has_alpha (format);
  type      = babl_format_get_type (format, 0);
  model     = babl_format_get_model (format);
  space     = babl_format_get_space (format);

  if (format == babl_format ("cairo-RGB24"))
    {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
      *lcms_format = TYPE_BGRA_8;
#else
      *lcms_format = TYPE_ARGB_8;
#endif

      return format;
    }
  else if (format == babl_format ("cairo-ARGB32"))
    {
      rgb = TRUE;
    }
  else if (model == babl_model ("RGB")  ||
           model == babl_model ("RGBA") ||
           model == babl_model ("RaGaBaA"))
    {
      rgb    = TRUE;
      linear = TRUE;
    }
  else if (model == babl_model ("R~G~B~")  ||
           model == babl_model ("R~G~B~A") ||
           model == babl_model ("R~aG~aB~aA"))
    {
      rgb      = TRUE;
      srgb_trc = TRUE;
    }
  else if (model == babl_model ("R'G'B'")  ||
           model == babl_model ("R'G'B'A") ||
           model == babl_model ("R'aG'aB'aA"))
    {
      rgb = TRUE;
    }
  else if (model == babl_model ("Y")  ||
           model == babl_model ("YA") ||
           model == babl_model ("YaA"))
    {
      gray   = TRUE;
      linear = TRUE;
    }
  else if (model == babl_model ("Y~")  ||
           model == babl_model ("Y~A") ||
           model == babl_model ("Y~aA"))
    {
      gray     = TRUE;
      srgb_trc = TRUE;
    }
  else if (model == babl_model ("Y'")  ||
           model == babl_model ("Y'A") ||
           model == babl_model ("Y'aA"))
    {
      gray = TRUE;
    }
  else if (model == babl_model ("CMYK"))
#if 0
    /* FIXME missing from babl */
           || model == babl_model ("CMYKA"))
#endif
    {
      cmyk = TRUE;
    }
  else if (model == babl_model ("CIE Lab")       ||
           model == babl_model ("CIE Lab alpha") ||
           model == babl_model ("CIE LCH(ab)")   ||
           model == babl_model ("CIE LCH(ab) alpha"))
    {
      if (has_alpha)
        {
          *lcms_format = TYPE_RGBA_FLT;

          return babl_format_with_space ("RGBA float", space);
        }
      else
        {
          *lcms_format = TYPE_RGB_FLT;

          return babl_format_with_space ("RGB float", space);
        }
    }
  else if (babl_format_is_palette (format))
    {
      if (has_alpha)
        {
          *lcms_format = TYPE_RGBA_8;

          return babl_format_with_space ("R'G'B'A u8", space);
        }
      else
        {
          *lcms_format = TYPE_RGB_8;

          return babl_format_with_space ("R'G'B' u8", space);
        }
    }
  else
    {
      g_printerr ("format not supported: %s\n"
                  "has_alpha = %s\n"
                  "type = %s\n"
                  "model = %s\n",
                  babl_get_name (format),
                  has_alpha ? "TRUE" : "FALSE",
                  babl_get_name (type),
                  babl_get_name (model));
      g_return_val_if_reached (NULL);
    }

  *lcms_format = 0;

  #define FIND_FORMAT_FOR_TYPE(babl_t, lcms_t)                                 \
    do                                                                         \
      {                                                                        \
        if (has_alpha)                                                         \
          {                                                                    \
            if (rgb)                                                           \
              {                                                                \
                *lcms_format = TYPE_RGBA_##lcms_t;                             \
                                                                               \
                if (linear)                                                    \
                  output_format = babl_format_with_space ("RGBA " babl_t,      \
                                                          space);              \
                else if (srgb_trc)                                             \
                  output_format = babl_format_with_space ("R~G~B~A " babl_t,   \
                                                          space);              \
                else                                                           \
                  output_format = babl_format_with_space ("R'G'B'A " babl_t,   \
                                                          space);              \
              }                                                                \
            else if (gray)                                                     \
              {                                                                \
                *lcms_format = TYPE_GRAYA_##lcms_t;                            \
                                                                               \
                if (linear)                                                    \
                  output_format = babl_format_with_space ("YA " babl_t,        \
                                                          space);              \
                else if (srgb_trc)                                             \
                  output_format = babl_format_with_space ("Y~A " babl_t,       \
                                                          space);              \
                else                                                           \
                  output_format = babl_format_with_space ("Y'A " babl_t,       \
                                                          space);              \
              }                                                                \
            else if (cmyk)                                                     \
              {                                                                \
                *lcms_format = TYPE_CMYKA_##lcms_t;                            \
                                                                               \
                output_format = format;                                        \
              }                                                                \
          }                                                                    \
        else                                                                   \
          {                                                                    \
            if (rgb)                                                           \
              {                                                                \
                *lcms_format = TYPE_RGB_##lcms_t;                              \
                                                                               \
                if (linear)                                                    \
                  output_format = babl_format_with_space ("RGB " babl_t,       \
                                                          space);              \
                else if (srgb_trc)                                             \
                  output_format = babl_format_with_space ("R~G~B~ " babl_t,    \
                                                          space);              \
                else                                                           \
                  output_format = babl_format_with_space ("R'G'B' " babl_t,    \
                                                          space);              \
              }                                                                \
            else if (gray)                                                     \
              {                                                                \
                *lcms_format = TYPE_GRAY_##lcms_t;                             \
                                                                               \
                if (linear)                                                    \
                  output_format = babl_format_with_space ("Y " babl_t,         \
                                                          space);              \
                else if (srgb_trc)                                             \
                  output_format = babl_format_with_space ("Y~ " babl_t,        \
                                                          space);              \
                else                                                           \
                  output_format = babl_format_with_space ("Y' " babl_t,        \
                                                          space);              \
              }                                                                \
            else if (cmyk)                                                     \
              {                                                                \
                *lcms_format = TYPE_CMYK_##lcms_t;                             \
                                                                               \
                output_format = format;                                        \
              }                                                                \
          }                                                                    \
      }                                                                        \
    while (FALSE)

  if (type == babl_type ("u8"))
    FIND_FORMAT_FOR_TYPE ("u8", 8);
  else if (type == babl_type ("u16"))
    FIND_FORMAT_FOR_TYPE ("u16", 16);
  else if (type == babl_type ("half")) /* 16-bit floating point (half) */
    FIND_FORMAT_FOR_TYPE ("half", HALF_FLT);
  else if (type == babl_type ("float"))
    FIND_FORMAT_FOR_TYPE ("float", FLT);
  else if (type == babl_type ("double"))
    FIND_FORMAT_FOR_TYPE ("double", DBL);

  if (*lcms_format == 0)
    {
      g_printerr ("%s: format %s not supported, "
                  "falling back to float\n",
                  G_STRFUNC, babl_get_name (format));

      rgb = ! gray;

      FIND_FORMAT_FOR_TYPE ("float", FLT);

      g_return_val_if_fail (output_format != NULL, NULL);
    }

  #undef FIND_FORMAT_FOR_TYPE

  return output_format;
}
