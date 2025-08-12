/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpaccellabel.h
 * Copyright (C) 2020 Ell
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

#pragma once


#define GIMP_TYPE_ACCEL_LABEL            (gimp_accel_label_get_type ())
#define GIMP_ACCEL_LABEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ACCEL_LABEL, GimpAccelLabel))
#define GIMP_ACCEL_LABEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ACCEL_LABEL, GimpAccelLabelClass))
#define GIMP_IS_ACCEL_LABEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_ACCEL_LABEL))
#define GIMP_IS_ACCEL_LABEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ACCEL_LABEL))
#define GIMP_ACCEL_LABEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ACCEL_LABEL, GimpAccelLabelClass))


typedef struct _GimpAccelLabelPrivate GimpAccelLabelPrivate;
typedef struct _GimpAccelLabelClass   GimpAccelLabelClass;

struct _GimpAccelLabel
{
  GtkLabel               parent_instance;

  GimpAccelLabelPrivate *priv;
};

struct _GimpAccelLabelClass
{
  GtkLabelClass  parent_class;
};


GType        gimp_accel_label_get_type   (void) G_GNUC_CONST;

GtkWidget  * gimp_accel_label_new        (GimpAction     *action);

void         gimp_accel_label_set_action (GimpAccelLabel *accel_label,
                                          GimpAction     *action);
GimpAction * gimp_accel_label_get_action (GimpAccelLabel *accel_label);
