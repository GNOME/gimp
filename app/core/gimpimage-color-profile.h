/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaimage-color-profile.h
 * Copyright (C) 2015-2018 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_IMAGE_COLOR_PROFILE_H__
#define __LIGMA_IMAGE_COLOR_PROFILE_H__


#define LIGMA_ICC_PROFILE_PARASITE_NAME            "icc-profile"
#define LIGMA_SIMULATION_ICC_PROFILE_PARASITE_NAME "simulation-icc-profile"


gboolean             ligma_image_get_use_srgb_profile   (LigmaImage           *image,
                                                        gboolean            *hidden_profile);
void                 ligma_image_set_use_srgb_profile   (LigmaImage           *image,
                                                        gboolean             use_srgb);

LigmaColorProfile   * _ligma_image_get_hidden_profile    (LigmaImage           *image);
void                 _ligma_image_set_hidden_profile    (LigmaImage           *image,
                                                        LigmaColorProfile    *profile,
                                                        gboolean             push_undo);

gboolean             ligma_image_validate_icc_parasite  (LigmaImage           *image,
                                                        const LigmaParasite  *icc_parasite,
                                                        const gchar         *profile_type,
                                                        gboolean            *is_builtin,
                                                        GError             **error);
const LigmaParasite * ligma_image_get_icc_parasite       (LigmaImage           *image);
void                 ligma_image_set_icc_parasite       (LigmaImage           *image,
                                                        const LigmaParasite  *icc_parasite,
                                                        const gchar         *profile_type);

gboolean             ligma_image_validate_icc_profile   (LigmaImage           *image,
                                                        const guint8        *data,
                                                        gsize                length,
                                                        const gchar         *profile_type,
                                                        gboolean            *is_builtin,
                                                        GError             **error);
const guint8       * ligma_image_get_icc_profile        (LigmaImage           *image,
                                                        gsize               *length);
gboolean             ligma_image_set_icc_profile        (LigmaImage           *image,
                                                        const guint8        *data,
                                                        gsize                length,
                                                        const gchar         *profile_type,
                                                        GError             **error);

gboolean             ligma_image_validate_color_profile (LigmaImage           *image,
                                                        LigmaColorProfile    *profile,
                                                        gboolean            *is_builtin,
                                                        GError             **error);
LigmaColorProfile   * ligma_image_get_color_profile      (LigmaImage           *image);
gboolean             ligma_image_set_color_profile      (LigmaImage           *image,
                                                        LigmaColorProfile    *profile,
                                                        GError             **error);

LigmaColorProfile   * ligma_image_get_simulation_profile (LigmaImage           *image);
gboolean             ligma_image_set_simulation_profile (LigmaImage           *image,
                                                        LigmaColorProfile    *profile);

LigmaColorRenderingIntent
                     ligma_image_get_simulation_intent  (LigmaImage           *image);
void                 ligma_image_set_simulation_intent  (LigmaImage                *image,
                                                        LigmaColorRenderingIntent intent);

gboolean             ligma_image_get_simulation_bpc     (LigmaImage           *image);
void                 ligma_image_set_simulation_bpc     (LigmaImage           *image,
                                                        gboolean             bpc);

gboolean             ligma_image_validate_color_profile_by_format
                                                       (const Babl          *format,
                                                        LigmaColorProfile    *profile,
                                                        gboolean            *is_builtin,
                                                        GError             **error);

LigmaColorProfile   * ligma_image_get_builtin_color_profile
                                                       (LigmaImage           *image);

gboolean             ligma_image_assign_color_profile   (LigmaImage           *image,
                                                        LigmaColorProfile    *dest_profile,
                                                        LigmaProgress        *progress,
                                                        GError             **error);

gboolean             ligma_image_convert_color_profile  (LigmaImage           *image,
                                                        LigmaColorProfile    *dest_profile,
                                                        LigmaColorRenderingIntent  intent,
                                                        gboolean             bpc,
                                                        LigmaProgress        *progress,
                                                        GError             **error);

void                 ligma_image_import_color_profile   (LigmaImage           *image,
                                                        LigmaContext         *context,
                                                        LigmaProgress        *progress,
                                                        gboolean             interactive);

LigmaColorTransform * ligma_image_get_color_transform_to_srgb_u8
                                                       (LigmaImage           *image);
LigmaColorTransform * ligma_image_get_color_transform_from_srgb_u8
                                                       (LigmaImage           *image);

LigmaColorTransform * ligma_image_get_color_transform_to_srgb_double
                                                       (LigmaImage           *image);
LigmaColorTransform * ligma_image_get_color_transform_from_srgb_double
                                                       (LigmaImage           *image);

void                 ligma_image_color_profile_pixel_to_srgb
                                                       (LigmaImage           *image,
                                                        const Babl          *pixel_format,
                                                        gpointer             pixel,
                                                        LigmaRGB             *color);
void                 ligma_image_color_profile_srgb_to_pixel
                                                       (LigmaImage           *image,
                                                        const LigmaRGB       *color,
                                                        const Babl          *pixel_format,
                                                        gpointer             pixel);


/*  internal API, to be called only from ligmaimage.c  */

void                 _ligma_image_free_color_profile    (LigmaImage           *image);
void                 _ligma_image_free_color_transforms (LigmaImage           *image);
void                 _ligma_image_update_color_profile  (LigmaImage           *image,
                                                        const LigmaParasite  *icc_parasite);
void                 _ligma_image_update_simulation_profile
                                                       (LigmaImage           *image,
                                                        const LigmaParasite  *icc_parasite);


#endif /* __LIGMA_IMAGE_COLOR_PROFILE_H__ */
