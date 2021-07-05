/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_DISPLAY_H__
#define __GIMP_DISPLAY_H__


#include "gimpobject.h"


#define GIMP_TYPE_DISPLAY            (gimp_display_get_type ())
#define GIMP_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DISPLAY, GimpDisplay))
#define GIMP_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DISPLAY, GimpDisplayClass))
#define GIMP_IS_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DISPLAY))
#define GIMP_IS_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DISPLAY))
#define GIMP_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DISPLAY, GimpDisplayClass))


typedef struct _GimpDisplayClass   GimpDisplayClass;
typedef struct _GimpDisplayPrivate GimpDisplayPrivate;

struct _GimpDisplay
{
  GimpObject          parent_instance;

  Gimp               *gimp;
  GimpDisplayConfig  *config;

  GimpDisplayPrivate *priv;
};

struct _GimpDisplayClass
{
  GimpObjectClass     parent_class;

  gboolean         (* present)    (GimpDisplay *display);
  gboolean         (* grab_focus) (GimpDisplay *display);
};


GType         gimp_display_get_type  (void) G_GNUC_CONST;

gint          gimp_display_get_id    (GimpDisplay *display);
GimpDisplay * gimp_display_get_by_id (Gimp        *gimp,
                                      gint         id);

gboolean      gimp_display_present    (GimpDisplay *display);
gboolean      gimp_display_grab_focus (GimpDisplay *display);

Gimp        * gimp_display_get_gimp  (GimpDisplay *display);


#endif /*  __GIMP_DISPLAY_H__  */
