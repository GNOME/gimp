/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpextensiondetails.h
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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

#ifndef __GIMP_EXTENSION_DETAILS_H__
#define __GIMP_EXTENSION_DETAILS_H__


#define GIMP_TYPE_EXTENSION_DETAILS            (gimp_extension_details_get_type ())
#define GIMP_EXTENSION_DETAILS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_EXTENSION_DETAILS, GimpExtensionDetails))
#define GIMP_EXTENSION_DETAILS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_EXTENSION_DETAILS, GimpExtensionDetailsClass))
#define GIMP_IS_EXTENSION_DETAILS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_EXTENSION_DETAILS))
#define GIMP_IS_EXTENSION_DETAILS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_EXTENSION_DETAILS))
#define GIMP_EXTENSION_DETAILS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_EXTENSION_DETAILS, GimpExtensionDetailsClass))


typedef struct _GimpExtensionDetailsClass    GimpExtensionDetailsClass;
typedef struct _GimpExtensionDetailsPrivate  GimpExtensionDetailsPrivate;

struct _GimpExtensionDetails
{
  GtkFrame                     parent_instance;

  GimpExtensionDetailsPrivate *p;
};

struct _GimpExtensionDetailsClass
{
  GtkFrameClass                parent_class;
};


GType        gimp_extension_details_get_type     (void) G_GNUC_CONST;

GtkWidget  * gimp_extension_details_new          (void);

void         gimp_extension_details_set          (GimpExtensionDetails *details,
                                                  GimpExtension        *extension);

#endif /* __GIMP_EXTENSION_DETAILS_H__ */

