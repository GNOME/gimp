/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsettings.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_SETTINGS_H__
#define __GIMP_SETTINGS_H__


#include "gimpviewable.h"


#define GIMP_TYPE_SETTINGS            (gimp_settings_get_type ())
#define GIMP_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SETTINGS, GimpSettings))
#define GIMP_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_SETTINGS, GimpSettingsClass))
#define GIMP_IS_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SETTINGS))
#define GIMP_IS_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_SETTINGS))
#define GIMP_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_SETTINGS, GimpSettingsClass))


typedef struct _GimpSettingsClass GimpSettingsClass;

struct _GimpSettings
{
  GimpViewable  parent_instance;

  gint64        time;
};

struct _GimpSettingsClass
{
  GimpViewableClass  parent_class;
};


GType   gimp_settings_get_type (void) G_GNUC_CONST;

gint    gimp_settings_compare  (GimpSettings *a,
                                GimpSettings *b);


#endif /* __GIMP_SETTINGS_H__ */
