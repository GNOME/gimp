/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationlayermode.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_OPERATION_LAYER_MODE            (gimp_operation_layer_mode_get_type ())
#define GIMP_OPERATION_LAYER_MODE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_LAYER_MODE, GimpOperationLayerMode))
#define GIMP_OPERATION_LAYER_MODE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_LAYER_MODE, GimpOperationLayerModeClass))
#define GIMP_IS_OPERATION_LAYER_MODE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_LAYER_MODE))
#define GIMP_IS_OPERATION_LAYER_MODE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_LAYER_MODE))
#define GIMP_OPERATION_LAYER_MODE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_LAYER_MODE, GimpOperationLayerModeClass))


typedef struct _GimpOperationLayerModeClass GimpOperationLayerModeClass;

struct _GimpOperationLayerMode
{
  GeglOperationPointComposer3  parent_instance;

  GimpLayerMode                layer_mode;
  gdouble                      opacity;
  GimpLayerColorSpace          blend_space;
  GimpLayerColorSpace          composite_space;
  GimpLayerCompositeMode       composite_mode;
  const Babl                  *cached_fish_format;
  const Babl                  *space_fish[4 /* from */][4 /* to */];
  GRWLock                      cache_lock;

  gdouble                      prop_opacity;
  GimpLayerCompositeMode       prop_composite_mode;

  GimpLayerModeFunc            function;
  GimpLayerModeBlendFunc       blend_function;
  gboolean                     is_last_node;
  gboolean                     has_mask;
};

struct _GimpOperationLayerModeClass
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
  GimpLayerCompositeRegion (* get_affected_region) (GimpOperationLayerMode *layer_mode);
};


GType                    gimp_operation_layer_mode_get_type            (void) G_GNUC_CONST;

GimpLayerCompositeRegion gimp_operation_layer_mode_get_affected_region (GimpOperationLayerMode *layer_mode);
