/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmacolorprofile.h
 * Copyright (C) 2014  Michael Natterer <mitch@ligma.org>
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

#if !defined (__LIGMA_COLOR_H_INSIDE__) && !defined (LIGMA_COLOR_COMPILATION)
#error "Only <libligmacolor/ligmacolor.h> can be included directly."
#endif

#ifndef __LIGMA_COLOR_PROFILE_H__
#define __LIGMA_COLOR_PROFILE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_COLOR_PROFILE            (ligma_color_profile_get_type ())
#define LIGMA_COLOR_PROFILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_PROFILE, LigmaColorProfile))
#define LIGMA_COLOR_PROFILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_PROFILE, LigmaColorProfileClass))
#define LIGMA_IS_COLOR_PROFILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_PROFILE))
#define LIGMA_IS_COLOR_PROFILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_PROFILE))
#define LIGMA_COLOR_PROFILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_PROFILE, LigmaColorProfileClass))


typedef struct _LigmaColorProfilePrivate LigmaColorProfilePrivate;
typedef struct _LigmaColorProfileClass   LigmaColorProfileClass;

struct _LigmaColorProfile
{
  GObject                  parent_instance;

  LigmaColorProfilePrivate *priv;
};

struct _LigmaColorProfileClass
{
  GObjectClass  parent_class;

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType              ligma_color_profile_get_type              (void) G_GNUC_CONST;

LigmaColorProfile * ligma_color_profile_new_rgb_srgb          (void);
LigmaColorProfile * ligma_color_profile_new_rgb_srgb_linear   (void);
LigmaColorProfile * ligma_color_profile_new_rgb_adobe         (void);

LigmaColorProfile * ligma_color_profile_new_d65_gray_srgb_trc (void);
LigmaColorProfile * ligma_color_profile_new_d65_gray_linear   (void);
LigmaColorProfile * ligma_color_profile_new_d50_gray_lab_trc  (void);

LigmaColorProfile *
       ligma_color_profile_new_srgb_trc_from_color_profile   (LigmaColorProfile  *profile);
LigmaColorProfile *
       ligma_color_profile_new_linear_from_color_profile     (LigmaColorProfile  *profile);

LigmaColorProfile * ligma_color_profile_new_from_file         (GFile             *file,
                                                             GError           **error);

LigmaColorProfile * ligma_color_profile_new_from_icc_profile  (const guint8      *data,
                                                             gsize              length,
                                                             GError           **error);
LigmaColorProfile * ligma_color_profile_new_from_lcms_profile (gpointer           lcms_profile,
                                                             GError           **error);

gboolean           ligma_color_profile_save_to_file          (LigmaColorProfile  *profile,
                                                             GFile             *file,
                                                             GError           **error);

const guint8     * ligma_color_profile_get_icc_profile       (LigmaColorProfile  *profile,
                                                             gsize             *length);
gpointer           ligma_color_profile_get_lcms_profile      (LigmaColorProfile  *profile);

const gchar      * ligma_color_profile_get_description       (LigmaColorProfile  *profile);
const gchar      * ligma_color_profile_get_manufacturer      (LigmaColorProfile  *profile);
const gchar      * ligma_color_profile_get_model             (LigmaColorProfile  *profile);
const gchar      * ligma_color_profile_get_copyright         (LigmaColorProfile  *profile);

const gchar      * ligma_color_profile_get_label             (LigmaColorProfile  *profile);
const gchar      * ligma_color_profile_get_summary           (LigmaColorProfile  *profile);

gboolean           ligma_color_profile_is_equal              (LigmaColorProfile  *profile1,
                                                             LigmaColorProfile  *profile2);

gboolean           ligma_color_profile_is_rgb                (LigmaColorProfile  *profile);
gboolean           ligma_color_profile_is_gray               (LigmaColorProfile  *profile);
gboolean           ligma_color_profile_is_cmyk               (LigmaColorProfile  *profile);

gboolean           ligma_color_profile_is_linear             (LigmaColorProfile  *profile);

const Babl       * ligma_color_profile_get_space             (LigmaColorProfile  *profile,
                                                             LigmaColorRenderingIntent intent,
                                                             GError           **error);
const Babl       * ligma_color_profile_get_format            (LigmaColorProfile  *profile,
                                                             const Babl        *format,
                                                             LigmaColorRenderingIntent intent,
                                                             GError           **error);

const Babl       * ligma_color_profile_get_lcms_format       (const Babl        *format,
                                                             guint32           *lcms_format);


G_END_DECLS

#endif  /* __LIGMA_COLOR_PROFILE_H__ */
