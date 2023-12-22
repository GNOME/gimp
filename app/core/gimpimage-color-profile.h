/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimage-color-profile.h
 * Copyright (C) 2015-2018 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_IMAGE_COLOR_PROFILE_H__
#define __GIMP_IMAGE_COLOR_PROFILE_H__


#define GIMP_ICC_PROFILE_PARASITE_NAME            "icc-profile"
#define GIMP_SIMULATION_ICC_PROFILE_PARASITE_NAME "simulation-icc-profile"


gboolean             gimp_image_get_use_srgb_profile   (GimpImage           *image,
                                                        gboolean            *hidden_profile);
void                 gimp_image_set_use_srgb_profile   (GimpImage           *image,
                                                        gboolean             use_srgb);

GimpColorProfile   * _gimp_image_get_hidden_profile    (GimpImage           *image);
void                 _gimp_image_set_hidden_profile    (GimpImage           *image,
                                                        GimpColorProfile    *profile,
                                                        gboolean             push_undo);

gboolean             gimp_image_validate_icc_parasite  (GimpImage           *image,
                                                        const GimpParasite  *icc_parasite,
                                                        const gchar         *profile_type,
                                                        gboolean            *is_builtin,
                                                        GError             **error);
const GimpParasite * gimp_image_get_icc_parasite       (GimpImage           *image);
void                 gimp_image_set_icc_parasite       (GimpImage           *image,
                                                        const GimpParasite  *icc_parasite,
                                                        const gchar         *profile_type);

gboolean             gimp_image_validate_icc_profile   (GimpImage           *image,
                                                        const guint8        *data,
                                                        gsize                length,
                                                        const gchar         *profile_type,
                                                        gboolean            *is_builtin,
                                                        GError             **error);
const guint8       * gimp_image_get_icc_profile        (GimpImage           *image,
                                                        gsize               *length);
gboolean             gimp_image_set_icc_profile        (GimpImage           *image,
                                                        const guint8        *data,
                                                        gsize                length,
                                                        const gchar         *profile_type,
                                                        GError             **error);

gboolean             gimp_image_validate_color_profile (GimpImage           *image,
                                                        GimpColorProfile    *profile,
                                                        gboolean            *is_builtin,
                                                        GError             **error);
GimpColorProfile   * gimp_image_get_color_profile      (GimpImage           *image);
gboolean             gimp_image_set_color_profile      (GimpImage           *image,
                                                        GimpColorProfile    *profile,
                                                        GError             **error);

GimpColorProfile   * gimp_image_get_simulation_profile (GimpImage           *image);
gboolean             gimp_image_set_simulation_profile (GimpImage           *image,
                                                        GimpColorProfile    *profile);

GimpColorRenderingIntent
                     gimp_image_get_simulation_intent  (GimpImage           *image);
void                 gimp_image_set_simulation_intent  (GimpImage                *image,
                                                        GimpColorRenderingIntent intent);

gboolean             gimp_image_get_simulation_bpc     (GimpImage           *image);
void                 gimp_image_set_simulation_bpc     (GimpImage           *image,
                                                        gboolean             bpc);

gboolean             gimp_image_validate_color_profile_by_format
                                                       (const Babl          *format,
                                                        GimpColorProfile    *profile,
                                                        gboolean            *is_builtin,
                                                        GError             **error);

GimpColorProfile   * gimp_image_get_builtin_color_profile
                                                       (GimpImage           *image);

gboolean             gimp_image_assign_color_profile   (GimpImage           *image,
                                                        GimpColorProfile    *dest_profile,
                                                        GimpProgress        *progress,
                                                        GError             **error);

gboolean             gimp_image_convert_color_profile  (GimpImage           *image,
                                                        GimpColorProfile    *dest_profile,
                                                        GimpColorRenderingIntent  intent,
                                                        gboolean             bpc,
                                                        GimpProgress        *progress,
                                                        GError             **error);

void                 gimp_image_import_color_profile   (GimpImage           *image,
                                                        GimpContext         *context,
                                                        GimpProgress        *progress,
                                                        gboolean             interactive);

GimpColorTransform * gimp_image_get_color_transform_to_srgb_u8
                                                       (GimpImage           *image);

GimpColorTransform * gimp_image_get_color_transform_to_srgb_double
                                                       (GimpImage           *image);
GimpColorTransform * gimp_image_get_color_transform_from_srgb_double
                                                       (GimpImage           *image);


/*  internal API, to be called only from gimpimage.c  */

void                 _gimp_image_free_color_profile    (GimpImage           *image);
void                 _gimp_image_free_color_transforms (GimpImage           *image);
void                 _gimp_image_update_color_profile  (GimpImage           *image,
                                                        const GimpParasite  *icc_parasite);
void                 _gimp_image_update_simulation_profile
                                                       (GimpImage           *image,
                                                        const GimpParasite  *icc_parasite);


#endif /* __GIMP_IMAGE_COLOR_PROFILE_H__ */
