/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaColorConfig class
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

#if !defined (__LIGMA_CONFIG_H_INSIDE__) && !defined (LIGMA_CONFIG_COMPILATION)
#error "Only <libligmaconfig/ligmaconfig.h> can be included directly."
#endif

#ifndef __LIGMA_COLOR_CONFIG_H__
#define __LIGMA_COLOR_CONFIG_H__


#define LIGMA_TYPE_COLOR_CONFIG            (ligma_color_config_get_type ())
#define LIGMA_COLOR_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_CONFIG, LigmaColorConfig))
#define LIGMA_COLOR_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_CONFIG, LigmaColorConfigClass))
#define LIGMA_IS_COLOR_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_CONFIG))
#define LIGMA_IS_COLOR_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_CONFIG))


typedef struct _LigmaColorConfigPrivate LigmaColorConfigPrivate;
typedef struct _LigmaColorConfigClass   LigmaColorConfigClass;

struct _LigmaColorConfig
{
  GObject                 parent_instance;

  LigmaColorConfigPrivate *priv;
};

struct _LigmaColorConfigClass
{
  GObjectClass  parent_class;

  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType              ligma_color_config_get_type                     (void) G_GNUC_CONST;

LigmaColorManagementMode
                   ligma_color_config_get_mode                     (LigmaColorConfig  *config);

LigmaColorRenderingIntent
                   ligma_color_config_get_display_intent           (LigmaColorConfig  *config);
gboolean           ligma_color_config_get_display_bpc              (LigmaColorConfig  *config);
gboolean           ligma_color_config_get_display_optimize         (LigmaColorConfig  *config);
gboolean           ligma_color_config_get_display_profile_from_gdk (LigmaColorConfig  *config);

LigmaColorRenderingIntent
                   ligma_color_config_get_simulation_intent        (LigmaColorConfig  *config);
gboolean           ligma_color_config_get_simulation_bpc           (LigmaColorConfig  *config);
gboolean           ligma_color_config_get_simulation_optimize      (LigmaColorConfig  *config);
gboolean           ligma_color_config_get_simulation_gamut_check   (LigmaColorConfig  *config);
void               ligma_color_config_get_out_of_gamut_color       (LigmaColorConfig  *config,
                                                                   LigmaRGB          *color);

LigmaColorProfile * ligma_color_config_get_rgb_color_profile        (LigmaColorConfig  *config,
                                                                   GError          **error);
LigmaColorProfile * ligma_color_config_get_gray_color_profile       (LigmaColorConfig  *config,
                                                                   GError          **error);
LigmaColorProfile * ligma_color_config_get_cmyk_color_profile       (LigmaColorConfig  *config,
                                                                   GError          **error);
LigmaColorProfile * ligma_color_config_get_display_color_profile    (LigmaColorConfig  *config,
                                                                   GError          **error);
LigmaColorProfile * ligma_color_config_get_simulation_color_profile (LigmaColorConfig  *config,
                                                                   GError          **error);


#endif /* LIGMA_COLOR_CONFIG_H__ */
