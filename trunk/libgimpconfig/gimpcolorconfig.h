/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpColorConfig class
 * Copyright (C) 2004  Stefan Döhla <stefan@doehla.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_COLOR_CONFIG_H__
#define __GIMP_COLOR_CONFIG_H__


#define GIMP_TYPE_COLOR_CONFIG            (gimp_color_config_get_type ())
#define GIMP_COLOR_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_CONFIG, GimpColorConfig))
#define GIMP_COLOR_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_CONFIG, GimpColorConfigClass))
#define GIMP_IS_COLOR_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_CONFIG))
#define GIMP_IS_COLOR_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_CONFIG))


typedef struct _GimpColorConfigClass GimpColorConfigClass;

struct _GimpColorConfig
{
  GObject                     parent_instance;

  GimpColorManagementMode     mode;
  gchar                      *rgb_profile;
  gchar                      *cmyk_profile;
  gchar                      *display_profile;
  gboolean                    display_profile_from_gdk;
  gchar                      *printer_profile;
  GimpColorRenderingIntent    display_intent;
  GimpColorRenderingIntent    simulation_intent;

  gchar                      *display_module;

  gboolean                    simulation_gamut_check;
  GimpRGB                     out_of_gamut_color;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};

struct _GimpColorConfigClass
{
  GObjectClass                parent_class;
};


GType  gimp_color_config_get_type (void) G_GNUC_CONST;


#endif /* GIMP_COLOR_CONFIG_H__ */
