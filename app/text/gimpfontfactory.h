/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfontfactory.h
 * Copyright (C) 2018 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_FONT_FACTORY_H__
#define __GIMP_FONT_FACTORY_H__


#include "core/gimpdatafactory.h"


#define GIMP_TYPE_FONT_FACTORY            (gimp_font_factory_get_type ())
#define GIMP_FONT_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FONT_FACTORY, GimpFontFactory))
#define GIMP_FONT_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FONT_FACTORY, GimpFontFactoryClass))
#define GIMP_IS_FONT_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FONT_FACTORY))
#define GIMP_IS_FONT_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FONT_FACTORY))
#define GIMP_FONT_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FONT_FACTORY, GimpFontFactoryClass))


typedef struct _GimpFontFactoryPrivate GimpFontFactoryPrivate;
typedef struct _GimpFontFactoryClass   GimpFontFactoryClass;

struct _GimpFontFactory
{
  GimpDataFactory         parent_instance;

  gchar                  *fonts_renaming_config;
  gchar                  *conf;
  gchar                  *sysconf;

  GimpFontFactoryPrivate *priv;
};

struct _GimpFontFactoryClass
{
  GimpDataFactoryClass  parent_class;
};


GType             gimp_font_factory_get_type (void) G_GNUC_CONST;

GimpDataFactory * gimp_font_factory_new                        (Gimp             *gimp,
                                                                const gchar      *path_property_name);
GList           * gimp_font_factory_get_custom_fonts_dirs      (GimpFontFactory  *factory);
void              gimp_font_factory_get_custom_config_path     (GimpFontFactory  *factory,
                                                                gchar           **conf,
                                                                gchar           **sysconf);
gchar           * gimp_font_factory_get_fonts_renaming_config  (GimpFontFactory  *factory);


#endif  /*  __GIMP_FONT_FACTORY_H__  */
