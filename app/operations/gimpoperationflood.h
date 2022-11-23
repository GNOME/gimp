/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationflood.h
 * Copyright (C) 2016 Ell
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

#ifndef __LIGMA_OPERATION_FLOOD_H__
#define __LIGMA_OPERATION_FLOOD_H__


#include <gegl-plugin.h>


#define LIGMA_TYPE_OPERATION_FLOOD            (ligma_operation_flood_get_type ())
#define LIGMA_OPERATION_FLOOD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_FLOOD, LigmaOperationFlood))
#define LIGMA_OPERATION_FLOOD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_FLOOD, LigmaOperationFloodClass))
#define LIGMA_IS_OPERATION_FLOOD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_FLOOD))
#define LIGMA_IS_OPERATION_FLOOD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_FLOOD))
#define LIGMA_OPERATION_FLOOD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_FLOOD, LigmaOperationFloodClass))


typedef struct _LigmaOperationFlood      LigmaOperationFlood;
typedef struct _LigmaOperationFloodClass LigmaOperationFloodClass;

struct _LigmaOperationFlood
{
  GeglOperationFilter  parent_instance;
};

struct _LigmaOperationFloodClass
{
  GeglOperationFilterClass  parent_class;
};


GType   ligma_operation_flood_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_OPERATION_FLOOD_H__ */
