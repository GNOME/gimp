/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpColorConfig class
 * Copyright (C) 2004  Stefan Döhla <stefan@doehla.de>
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

#if !defined (__GIMP_CONFIG_H_INSIDE__) && !defined (GIMP_CONFIG_COMPILATION)
#error "Only <libgimpconfig/gimpconfig.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_CONFIG_H__
#define __GIMP_COLOR_CONFIG_H__


#define GIMP_TYPE_COLOR_CONFIG (gimp_color_config_get_type ())
G_DECLARE_FINAL_TYPE (GimpColorConfig, gimp_color_config, GIMP, COLOR_CONFIG, GObject)


GimpColorManagementMode
                   gimp_color_config_get_mode                     (GimpColorConfig  *config);

GimpColorRenderingIntent
                   gimp_color_config_get_display_intent           (GimpColorConfig  *config);
gboolean           gimp_color_config_get_display_bpc              (GimpColorConfig  *config);
gboolean           gimp_color_config_get_display_optimize         (GimpColorConfig  *config);
gboolean           gimp_color_config_get_display_profile_from_gdk (GimpColorConfig  *config);

GimpColorRenderingIntent
                   gimp_color_config_get_simulation_intent        (GimpColorConfig  *config);
gboolean           gimp_color_config_get_simulation_bpc           (GimpColorConfig  *config);
gboolean           gimp_color_config_get_simulation_optimize      (GimpColorConfig  *config);
gboolean           gimp_color_config_get_simulation_gamut_check   (GimpColorConfig  *config);
GeglColor        * gimp_color_config_get_out_of_gamut_color       (GimpColorConfig  *config);

GimpColorProfile * gimp_color_config_get_rgb_color_profile        (GimpColorConfig  *config,
                                                                   GError          **error);
GimpColorProfile * gimp_color_config_get_gray_color_profile       (GimpColorConfig  *config,
                                                                   GError          **error);
GimpColorProfile * gimp_color_config_get_cmyk_color_profile       (GimpColorConfig  *config,
                                                                   GError          **error);
GimpColorProfile * gimp_color_config_get_display_color_profile    (GimpColorConfig  *config,
                                                                   GError          **error);
GimpColorProfile * gimp_color_config_get_simulation_color_profile (GimpColorConfig  *config,
                                                                   GError          **error);


#endif /* GIMP_COLOR_CONFIG_H__ */
