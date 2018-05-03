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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_CONFIG_H_INSIDE__) && !defined (GIMP_CONFIG_COMPILATION)
#error "Only <libgimpconfig/gimpconfig.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_CONFIG_H__
#define __GIMP_COLOR_CONFIG_H__


#define GIMP_TYPE_COLOR_CONFIG            (gimp_color_config_get_type ())
#define GIMP_COLOR_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_CONFIG, GimpColorConfig))
#define GIMP_COLOR_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_CONFIG, GimpColorConfigClass))
#define GIMP_IS_COLOR_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_CONFIG))
#define GIMP_IS_COLOR_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_CONFIG))


typedef struct _GimpColorConfigPrivate GimpColorConfigPrivate;
typedef struct _GimpColorConfigClass   GimpColorConfigClass;

struct _GimpColorConfig
{
  GObject                 parent_instance;

  GimpColorConfigPrivate *priv;

  /* FIXME MOVE TO PRIVATE */
  GimpColorManagementMode     mode;
  gchar                      *rgb_profile;
  gchar                      *cmyk_profile;
  gchar                      *display_profile;
  gboolean                    display_profile_from_gdk;
  gchar                      *printer_profile;
  GimpColorRenderingIntent    display_intent;
  GimpColorRenderingIntent    simulation_intent;

  gboolean                    simulation_gamut_check;
  GimpRGB                     out_of_gamut_color;

  gboolean                    display_use_black_point_compensation;
  gboolean                    simulation_use_black_point_compensation;

  gchar                      *gray_profile;
};

struct _GimpColorConfigClass
{
  GObjectClass  parent_class;

  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


GType              gimp_color_config_get_type                     (void) G_GNUC_CONST;

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
