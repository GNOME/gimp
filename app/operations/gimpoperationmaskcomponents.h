/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationmaskcomponents.h
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

#ifndef __LIGMA_OPERATION_MASK_COMPONENTS_H__
#define __LIGMA_OPERATION_MASK_COMPONENTS_H__

#include <gegl-plugin.h>


#define LIGMA_TYPE_OPERATION_MASK_COMPONENTS            (ligma_operation_mask_components_get_type ())
#define LIGMA_OPERATION_MASK_COMPONENTS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_MASK_COMPONENTS, LigmaOperationMaskComponents))
#define LIGMA_OPERATION_MASK_COMPONENTS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_MASK_COMPONENTS, LigmaOperationMaskComponentsClass))
#define LIGMA_IS_OPERATION_MASK_COMPONENTS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_MASK_COMPONENTS))
#define LIGMA_IS_OPERATION_MASK_COMPONENTS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_MASK_COMPONENTS))
#define LIGMA_OPERATION_MASK_COMPONENTS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_MASK_COMPONENTS, LigmaOperationMaskComponentsClass))


typedef struct _LigmaOperationMaskComponents      LigmaOperationMaskComponents;
typedef struct _LigmaOperationMaskComponentsClass LigmaOperationMaskComponentsClass;

struct _LigmaOperationMaskComponents
{
  GeglOperationPointComposer  parent_instance;

  LigmaComponentMask           mask;
  gdouble                     alpha;

  guint32                     alpha_value;
  gpointer                    process;
  const Babl                 *format;
};

struct _LigmaOperationMaskComponentsClass
{
  GeglOperationPointComposerClass  parent_class;
};


GType        ligma_operation_mask_components_get_type   (void) G_GNUC_CONST;

const Babl * ligma_operation_mask_components_get_format (const Babl        *input_format);

void         ligma_operation_mask_components_process    (const Babl        *format,
                                                        gconstpointer      in,
                                                        gconstpointer      aux,
                                                        gpointer           out,
                                                        gint               n,
                                                        LigmaComponentMask  mask);


#endif /* __LIGMA_OPERATION_MASK_COMPONENTS_H__ */
