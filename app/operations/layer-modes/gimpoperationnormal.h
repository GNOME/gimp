/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationnormal.h
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

#ifndef __LIGMA_OPERATION_NORMAL_H__
#define __LIGMA_OPERATION_NORMAL_H__


#include "ligmaoperationlayermode.h"


#define LIGMA_TYPE_OPERATION_NORMAL            (ligma_operation_normal_get_type ())
#define LIGMA_OPERATION_NORMAL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_NORMAL, LigmaOperationNormal))
#define LIGMA_OPERATION_NORMAL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_NORMAL, LigmaOperationNormalClass))
#define LIGMA_IS_OPERATION_NORMAL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_NORMAL))
#define LIGMA_IS_OPERATION_NORMAL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_NORMAL))
#define LIGMA_OPERATION_NORMAL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_NORMAL, LigmaOperationNormalClass))


typedef struct _LigmaOperationNormal      LigmaOperationNormal;
typedef struct _LigmaOperationNormalClass LigmaOperationNormalClass;

struct _LigmaOperationNormal
{
  LigmaOperationLayerMode  parent_instance;
};

struct _LigmaOperationNormalClass
{
  LigmaOperationLayerModeClass  parent_class;
};


GType      ligma_operation_normal_get_type     (void) G_GNUC_CONST;


/*  protected  */

gboolean   ligma_operation_normal_process      (GeglOperation       *op,
                                               void                *in,
                                               void                *layer,
                                               void                *mask,
                                               void                *out,
                                               glong                samples,
                                               const GeglRectangle *roi,
                                               gint                 level);

#if COMPILE_SSE2_INTRINISICS

gboolean   ligma_operation_normal_process_sse2 (GeglOperation       *op,
                                               void                *in,
                                               void                *layer,
                                               void                *mask,
                                               void                *out,
                                               glong                samples,
                                               const GeglRectangle *roi,
                                               gint                 level);

#endif /* COMPILE_SSE2_INTRINISICS */

#if COMPILE_SSE4_1_INTRINISICS

gboolean   ligma_operation_normal_process_sse4 (GeglOperation       *op,
                                               void                *in,
                                               void                *layer,
                                               void                *mask,
                                               void                *out,
                                               glong                samples,
                                               const GeglRectangle *roi,
                                               gint                 level);

#endif /* COMPILE_SSE4_1_INTRINISICS */


#endif /* __LIGMA_OPERATION_NORMAL_H__ */
