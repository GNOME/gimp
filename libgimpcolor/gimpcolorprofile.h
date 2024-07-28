/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpcolorprofile.h
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
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_COLOR_H_INSIDE__) && !defined (GIMP_COLOR_COMPILATION)
#error "Only <libgimpcolor/gimpcolor.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_PROFILE_H__
#define __GIMP_COLOR_PROFILE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_COLOR_PROFILE (gimp_color_profile_get_type ())
G_DECLARE_FINAL_TYPE (GimpColorProfile, gimp_color_profile, GIMP, COLOR_PROFILE, GObject)


GimpColorProfile * gimp_color_profile_new_rgb_srgb          (void);
GimpColorProfile * gimp_color_profile_new_rgb_srgb_linear   (void);
GimpColorProfile * gimp_color_profile_new_rgb_adobe         (void);

GimpColorProfile * gimp_color_profile_new_d65_gray_srgb_trc (void);
GimpColorProfile * gimp_color_profile_new_d65_gray_linear   (void);
GimpColorProfile * gimp_color_profile_new_d50_gray_lab_trc  (void);

GimpColorProfile *
       gimp_color_profile_new_srgb_trc_from_color_profile   (GimpColorProfile  *profile);
GimpColorProfile *
       gimp_color_profile_new_linear_from_color_profile     (GimpColorProfile  *profile);

GimpColorProfile * gimp_color_profile_new_from_file         (GFile             *file,
                                                             GError           **error);

GimpColorProfile * gimp_color_profile_new_from_icc_profile  (const guint8      *data,
                                                             gsize              length,
                                                             GError           **error);
GimpColorProfile * gimp_color_profile_new_from_lcms_profile (gpointer           lcms_profile,
                                                             GError           **error);

gboolean           gimp_color_profile_save_to_file          (GimpColorProfile  *profile,
                                                             GFile             *file,
                                                             GError           **error);

const guint8     * gimp_color_profile_get_icc_profile       (GimpColorProfile  *profile,
                                                             gsize             *length);
gpointer           gimp_color_profile_get_lcms_profile      (GimpColorProfile  *profile);

const gchar      * gimp_color_profile_get_description       (GimpColorProfile  *profile);
const gchar      * gimp_color_profile_get_manufacturer      (GimpColorProfile  *profile);
const gchar      * gimp_color_profile_get_model             (GimpColorProfile  *profile);
const gchar      * gimp_color_profile_get_copyright         (GimpColorProfile  *profile);

const gchar      * gimp_color_profile_get_label             (GimpColorProfile  *profile);
const gchar      * gimp_color_profile_get_summary           (GimpColorProfile  *profile);

gboolean           gimp_color_profile_is_equal              (GimpColorProfile  *profile1,
                                                             GimpColorProfile  *profile2);

gboolean           gimp_color_profile_is_rgb                (GimpColorProfile  *profile);
gboolean           gimp_color_profile_is_gray               (GimpColorProfile  *profile);
gboolean           gimp_color_profile_is_cmyk               (GimpColorProfile  *profile);

gboolean           gimp_color_profile_is_linear             (GimpColorProfile  *profile);

const Babl       * gimp_color_profile_get_space             (GimpColorProfile  *profile,
                                                             GimpColorRenderingIntent intent,
                                                             GError           **error);
const Babl       * gimp_color_profile_get_format            (GimpColorProfile  *profile,
                                                             const Babl        *format,
                                                             GimpColorRenderingIntent intent,
                                                             GError           **error);

const Babl       * gimp_color_profile_get_lcms_format       (const Babl        *format,
                                                             guint32           *lcms_format);


G_END_DECLS

#endif  /* __GIMP_COLOR_PROFILE_H__ */
