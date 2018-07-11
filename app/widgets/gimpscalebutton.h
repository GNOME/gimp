/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpscalebutton.h
 * Copyright (C) 2008 Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_SCALE_BUTTON_H__
#define __GIMP_SCALE_BUTTON_H__


#define GIMP_TYPE_SCALE_BUTTON            (gimp_scale_button_get_type ())
#define GIMP_SCALE_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SCALE_BUTTON, GimpScaleButton))
#define GIMP_SCALE_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SCALE_BUTTON, GimpScaleButtonClass))
#define GIMP_IS_SCALE_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SCALE_BUTTON))
#define GIMP_IS_SCALE_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), GIMP_TYPE_SCALE_BUTTON))
#define GIMP_SCALE_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMP_TYPE_SCALE_BUTTON, GimpScaleButtonClass))


typedef struct _GimpScaleButtonClass GimpScaleButtonClass;

struct _GimpScaleButton
{
  GtkScaleButton      parent_instance;
};

struct _GimpScaleButtonClass
{
  GtkScaleButtonClass parent_class;
};


GType       gimp_scale_button_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_scale_button_new      (gdouble value,
                                        gdouble min,
                                        gdouble max);


#endif  /* __GIMP_SCALE_BUTTON_H__ */
