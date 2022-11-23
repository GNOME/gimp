/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationbrightnesscontrast.h
 * Copyright (C) 2012 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_OPERATION_BRIGHTNESS_CONTRAST_H__
#define __LIGMA_OPERATION_BRIGHTNESS_CONTRAST_H__


#include "ligmaoperationpointfilter.h"


#define LIGMA_TYPE_OPERATION_BRIGHTNESS_CONTRAST            (ligma_operation_brightness_contrast_get_type ())
#define LIGMA_OPERATION_BRIGHTNESS_CONTRAST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_BRIGHTNESS_CONTRAST, LigmaOperationBrightnessContrast))
#define LIGMA_OPERATION_BRIGHTNESS_CONTRAST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_BRIGHTNESS_CONTRAST, LigmaOperationBrightnessContrastClass))
#define LIGMA_IS_OPERATION_BRIGHTNESS_CONTRAST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_BRIGHTNESS_CONTRAST))
#define LIGMA_IS_OPERATION_BRIGHTNESS_CONTRAST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_BRIGHTNESS_CONTRAST))
#define LIGMA_OPERATION_BRIGHTNESS_CONTRAST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_BRIGHTNESS_CONTRAST, LigmaOperationBrightnessContrastClass))


typedef struct _LigmaOperationBrightnessContrast      LigmaOperationBrightnessContrast;
typedef struct _LigmaOperationBrightnessContrastClass LigmaOperationBrightnessContrastClass;

struct _LigmaOperationBrightnessContrast
{
  LigmaOperationPointFilter  parent_instance;
};

struct _LigmaOperationBrightnessContrastClass
{
  LigmaOperationPointFilterClass  parent_class;
};


GType   ligma_operation_brightness_contrast_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_OPERATION_BRIGHTNESS_CONTRAST_H__ */
