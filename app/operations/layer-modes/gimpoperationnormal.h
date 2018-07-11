/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationnormal.h
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

#ifndef __GIMP_OPERATION_NORMAL_H__
#define __GIMP_OPERATION_NORMAL_H__


#include "gimpoperationlayermode.h"


#define GIMP_TYPE_OPERATION_NORMAL            (gimp_operation_normal_get_type ())
#define GIMP_OPERATION_NORMAL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_NORMAL, GimpOperationNormal))
#define GIMP_OPERATION_NORMAL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_NORMAL, GimpOperationNormalClass))
#define GIMP_IS_OPERATION_NORMAL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_NORMAL))
#define GIMP_IS_OPERATION_NORMAL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_NORMAL))
#define GIMP_OPERATION_NORMAL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_NORMAL, GimpOperationNormalClass))


typedef struct _GimpOperationNormal      GimpOperationNormal;
typedef struct _GimpOperationNormalClass GimpOperationNormalClass;

struct _GimpOperationNormal
{
  GimpOperationLayerMode  parent_instance;
};

struct _GimpOperationNormalClass
{
  GimpOperationLayerModeClass  parent_class;
};


GType      gimp_operation_normal_get_type     (void) G_GNUC_CONST;


/*  protected  */

gboolean   gimp_operation_normal_process      (GeglOperation       *op,
                                               void                *in,
                                               void                *layer,
                                               void                *mask,
                                               void                *out,
                                               glong                samples,
                                               const GeglRectangle *roi,
                                               gint                 level);

#if COMPILE_SSE2_INTRINISICS

gboolean   gimp_operation_normal_process_sse2 (GeglOperation       *op,
                                               void                *in,
                                               void                *layer,
                                               void                *mask,
                                               void                *out,
                                               glong                samples,
                                               const GeglRectangle *roi,
                                               gint                 level);

#endif /* COMPILE_SSE2_INTRINISICS */

#if COMPILE_SSE4_1_INTRINISICS

gboolean   gimp_operation_normal_process_sse4 (GeglOperation       *op,
                                               void                *in,
                                               void                *layer,
                                               void                *mask,
                                               void                *out,
                                               glong                samples,
                                               const GeglRectangle *roi,
                                               gint                 level);

#endif /* COMPILE_SSE4_1_INTRINISICS */


#endif /* __GIMP_OPERATION_NORMAL_H__ */
