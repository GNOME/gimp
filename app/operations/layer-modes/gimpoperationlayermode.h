/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationlayermode.h
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

#ifndef __LIGMA_OPERATION_LAYER_MODE_H__
#define __LIGMA_OPERATION_LAYER_MODE_H__


#include <gegl-plugin.h>


#define LIGMA_TYPE_OPERATION_LAYER_MODE            (ligma_operation_layer_mode_get_type ())
#define LIGMA_OPERATION_LAYER_MODE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_LAYER_MODE, LigmaOperationLayerMode))
#define LIGMA_OPERATION_LAYER_MODE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_LAYER_MODE, LigmaOperationLayerModeClass))
#define LIGMA_IS_OPERATION_LAYER_MODE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_LAYER_MODE))
#define LIGMA_IS_OPERATION_LAYER_MODE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_LAYER_MODE))
#define LIGMA_OPERATION_LAYER_MODE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_LAYER_MODE, LigmaOperationLayerModeClass))


typedef struct _LigmaOperationLayerModeClass LigmaOperationLayerModeClass;

struct _LigmaOperationLayerMode
{
  GeglOperationPointComposer3  parent_instance;

  LigmaLayerMode                layer_mode;
  gdouble                      opacity;
  LigmaLayerColorSpace          blend_space;
  LigmaLayerColorSpace          composite_space;
  LigmaLayerCompositeMode       composite_mode;
  const Babl                  *cached_fish_format;
  const Babl                  *space_fish[3 /* from */][3 /* to */];

  gdouble                      prop_opacity;
  LigmaLayerCompositeMode       prop_composite_mode;

  LigmaLayerModeFunc            function;
  LigmaLayerModeBlendFunc       blend_function;
  gboolean                     is_last_node;
  gboolean                     has_mask;
};

struct _LigmaOperationLayerModeClass
{
  GeglOperationPointComposer3Class  parent_class;

  /*  virtual functions  */
  gboolean                 (* parent_process)      (GeglOperation          *operation,
                                                    GeglOperationContext   *context,
                                                    const gchar            *output_prop,
                                                    const GeglRectangle    *result,
                                                    gint                    level);
  gboolean                 (* process)             (GeglOperation          *operation,
                                                    void                   *in,
                                                    void                   *aux,
                                                    void                   *mask,
                                                    void                   *out,
                                                    glong                   samples,
                                                    const GeglRectangle    *roi,
                                                    gint                    level);

  /* Returns the composite region (any combination of the layer and the
   * backdrop) that the layer mode affects.  Most modes only affect the
   * overlapping region, and don't need to override this function.
   */
  LigmaLayerCompositeRegion (* get_affected_region) (LigmaOperationLayerMode *layer_mode);
};


GType                    ligma_operation_layer_mode_get_type            (void) G_GNUC_CONST;

LigmaLayerCompositeRegion ligma_operation_layer_mode_get_affected_region (LigmaOperationLayerMode *layer_mode);


#endif /* __LIGMA_OPERATION_LAYER_MODE_H__ */
