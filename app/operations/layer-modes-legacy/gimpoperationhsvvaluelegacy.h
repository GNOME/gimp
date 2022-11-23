/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationhsvvaluelegacy.h
 * Copyright (C) 2008 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_OPERATION_HSV_VALUE_LEGACY_H__
#define __LIGMA_OPERATION_HSV_VALUE_LEGACY_H__


#include "operations/layer-modes/ligmaoperationlayermode.h"


#define LIGMA_TYPE_OPERATION_HSV_VALUE_LEGACY            (ligma_operation_hsv_value_legacy_get_type ())
#define LIGMA_OPERATION_HSV_VALUE_LEGACY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_HSV_VALUE_LEGACY, LigmaOperationHsvValueLegacy))
#define LIGMA_OPERATION_HSV_VALUE_LEGACY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_HSV_VALUE_LEGACY, LigmaOperationHsvValueLegacyClass))
#define LIGMA_IS_OPERATION_HSV_VALUE_LEGACY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_HSV_VALUE_LEGACY))
#define LIGMA_IS_OPERATION_HSV_VALUE_LEGACY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_HSV_VALUE_LEGACY))
#define LIGMA_OPERATION_HSV_VALUE_LEGACY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_HSV_VALUE_LEGACY, LigmaOperationHsvValueLegacyClass))


typedef struct _LigmaOperationHsvValueLegacy      LigmaOperationHsvValueLegacy;
typedef struct _LigmaOperationHsvValueLegacyClass LigmaOperationHsvValueLegacyClass;

struct _LigmaOperationHsvValueLegacy
{
  LigmaOperationLayerMode  parent_instance;
};

struct _LigmaOperationHsvValueLegacyClass
{
  LigmaOperationLayerModeClass  parent_class;
};


GType   ligma_operation_hsv_value_legacy_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_OPERATION_HSV_VALUE_LEGACY_H__ */
