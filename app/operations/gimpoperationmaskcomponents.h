/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationmaskcomponents.h
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

#include <gegl-plugin.h>


#define GIMP_TYPE_OPERATION_MASK_COMPONENTS            (gimp_operation_mask_components_get_type ())
#define GIMP_OPERATION_MASK_COMPONENTS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_MASK_COMPONENTS, GimpOperationMaskComponents))
#define GIMP_OPERATION_MASK_COMPONENTS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_MASK_COMPONENTS, GimpOperationMaskComponentsClass))
#define GIMP_IS_OPERATION_MASK_COMPONENTS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_MASK_COMPONENTS))
#define GIMP_IS_OPERATION_MASK_COMPONENTS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_MASK_COMPONENTS))
#define GIMP_OPERATION_MASK_COMPONENTS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_MASK_COMPONENTS, GimpOperationMaskComponentsClass))


typedef struct _GimpOperationMaskComponents      GimpOperationMaskComponents;
typedef struct _GimpOperationMaskComponentsClass GimpOperationMaskComponentsClass;

struct _GimpOperationMaskComponents
{
  GeglOperationPointComposer  parent_instance;

  GimpComponentMask           mask;
  gdouble                     alpha;

  guint32                     alpha_value;
  gpointer                    process;
  const Babl                 *format;
};

struct _GimpOperationMaskComponentsClass
{
  GeglOperationPointComposerClass  parent_class;
};


GType        gimp_operation_mask_components_get_type   (void) G_GNUC_CONST;

const Babl * gimp_operation_mask_components_get_format (const Babl        *input_format);

void         gimp_operation_mask_components_process    (const Babl        *format,
                                                        gconstpointer      in,
                                                        gconstpointer      aux,
                                                        gpointer           out,
                                                        gint               n,
                                                        GimpComponentMask  mask);
