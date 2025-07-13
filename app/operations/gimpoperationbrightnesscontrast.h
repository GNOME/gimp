/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationbrightnesscontrast.h
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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

#include "gimpoperationpointfilter.h"


#define GIMP_TYPE_OPERATION_BRIGHTNESS_CONTRAST            (gimp_operation_brightness_contrast_get_type ())
#define GIMP_OPERATION_BRIGHTNESS_CONTRAST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_BRIGHTNESS_CONTRAST, GimpOperationBrightnessContrast))
#define GIMP_OPERATION_BRIGHTNESS_CONTRAST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_BRIGHTNESS_CONTRAST, GimpOperationBrightnessContrastClass))
#define GIMP_IS_OPERATION_BRIGHTNESS_CONTRAST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_BRIGHTNESS_CONTRAST))
#define GIMP_IS_OPERATION_BRIGHTNESS_CONTRAST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_BRIGHTNESS_CONTRAST))
#define GIMP_OPERATION_BRIGHTNESS_CONTRAST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_BRIGHTNESS_CONTRAST, GimpOperationBrightnessContrastClass))


typedef struct _GimpOperationBrightnessContrast      GimpOperationBrightnessContrast;
typedef struct _GimpOperationBrightnessContrastClass GimpOperationBrightnessContrastClass;

struct _GimpOperationBrightnessContrast
{
  GimpOperationPointFilter  parent_instance;
};

struct _GimpOperationBrightnessContrastClass
{
  GimpOperationPointFilterClass  parent_class;
};


GType   gimp_operation_brightness_contrast_get_type (void) G_GNUC_CONST;
