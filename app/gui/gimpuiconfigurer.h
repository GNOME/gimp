/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpuiconfigurer.h
 * Copyright (C) 2009 Martin Nordholts <martinn@src.gnome.org>
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

#ifndef __GIMP_UI_CONFIGURER_H__
#define __GIMP_UI_CONFIGURER_H__


#include "core/gimpobject.h"


#define GIMP_TYPE_UI_CONFIGURER              (gimp_ui_configurer_get_type ())
#define GIMP_UI_CONFIGURER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_UI_CONFIGURER, GimpUIConfigurer))
#define GIMP_UI_CONFIGURER_CLASS(vtable)     (G_TYPE_CHECK_CLASS_CAST ((vtable), GIMP_TYPE_UI_CONFIGURER, GimpUIConfigurerClass))
#define GIMP_IS_UI_CONFIGURER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_UI_CONFIGURER))
#define GIMP_IS_UI_CONFIGURER_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), GIMP_TYPE_UI_CONFIGURER))
#define GIMP_UI_CONFIGURER_GET_CLASS(inst)   (G_TYPE_INSTANCE_GET_CLASS ((inst), GIMP_TYPE_UI_CONFIGURER, GimpUIConfigurerClass))


typedef struct _GimpUIConfigurerClass   GimpUIConfigurerClass;
typedef struct _GimpUIConfigurerPrivate GimpUIConfigurerPrivate;

struct _GimpUIConfigurer
{
  GimpObject parent_instance;

  GimpUIConfigurerPrivate *p;
};

struct _GimpUIConfigurerClass
{
  GimpObjectClass parent_class;
};


GType         gimp_ui_configurer_get_type  (void) G_GNUC_CONST;
void          gimp_ui_configurer_configure (GimpUIConfigurer *ui_configurer,
                                            gboolean          single_window_mode);


#endif  /* __GIMP_UI_CONFIGURER_H__ */
