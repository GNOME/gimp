/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationpointfilter.h
 * Copyright (C) 2007 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_OPERATION_POINT_FILTER_H__
#define __LIGMA_OPERATION_POINT_FILTER_H__


#include <gegl-plugin.h>
#include <operation/gegl-operation-point-filter.h>


enum
{
  LIGMA_OPERATION_POINT_FILTER_PROP_0,
  LIGMA_OPERATION_POINT_FILTER_PROP_TRC,
  LIGMA_OPERATION_POINT_FILTER_PROP_CONFIG
};


#define LIGMA_TYPE_OPERATION_POINT_FILTER            (ligma_operation_point_filter_get_type ())
#define LIGMA_OPERATION_POINT_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_POINT_FILTER, LigmaOperationPointFilter))
#define LIGMA_OPERATION_POINT_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_POINT_FILTER, LigmaOperationPointFilterClass))
#define LIGMA_IS_OPERATION_POINT_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_POINT_FILTER))
#define LIGMA_IS_OPERATION_POINT_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_POINT_FILTER))
#define LIGMA_OPERATION_POINT_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_POINT_FILTER, LigmaOperationPointFilterClass))


typedef struct _LigmaOperationPointFilterClass LigmaOperationPointFilterClass;

struct _LigmaOperationPointFilter
{
  GeglOperationPointFilter  parent_instance;

  LigmaTRCType               trc;
  GObject                  *config;
};

struct _LigmaOperationPointFilterClass
{
  GeglOperationPointFilterClass  parent_class;
};


GType   ligma_operation_point_filter_get_type     (void) G_GNUC_CONST;

void    ligma_operation_point_filter_get_property (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
void    ligma_operation_point_filter_set_property (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);


#endif /* __LIGMA_OPERATION_POINT_FILTER_H__ */
