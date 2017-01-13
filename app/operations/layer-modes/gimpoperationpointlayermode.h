/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationpointlayermode.h
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_OPERATION_POINT_LAYER_MODE_H__
#define __GIMP_OPERATION_POINT_LAYER_MODE_H__


#include <gegl-plugin.h>


#define GIMP_TYPE_OPERATION_POINT_LAYER_MODE            (gimp_operation_point_layer_mode_get_type ())
#define GIMP_OPERATION_POINT_LAYER_MODE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_POINT_LAYER_MODE, GimpOperationPointLayerMode))
#define GIMP_OPERATION_POINT_LAYER_MODE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_POINT_LAYER_MODE, GimpOperationPointLayerModeClass))
#define GIMP_IS_OPERATION_POINT_LAYER_MODE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_POINT_LAYER_MODE))
#define GIMP_IS_OPERATION_POINT_LAYER_MODE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_POINT_LAYER_MODE))
#define GIMP_OPERATION_POINT_LAYER_MODE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_POINT_LAYER_MODE, GimpOperationPointLayerModeClass))


typedef struct _GimpOperationPointLayerModeClass GimpOperationPointLayerModeClass;

struct _GimpOperationPointLayerModeClass
{
  GeglOperationPointComposer3Class  parent_class;
};

struct _GimpOperationPointLayerMode
{
  GeglOperationPointComposer3  parent_instance;

  gboolean                     linear;
  gdouble                      opacity;
};


GType   gimp_operation_point_layer_mode_get_type (void) G_GNUC_CONST;


static inline void
gimp_operation_layer_composite (const gfloat *in,
                                const gfloat *layer,
                                const gfloat *mask,
                                gfloat       *out,
                                gfloat        opacity,
                                glong         samples)
{
  while (samples--)
    {
      gfloat comp_alpha = layer[ALPHA] * opacity;
      if (mask)
        comp_alpha *= *mask++;
      if (comp_alpha != 0.0f)
        {
          out[RED]   = out[RED]   * comp_alpha + in[RED]   * (1.0f - comp_alpha);
          out[GREEN] = out[GREEN] * comp_alpha + in[GREEN] * (1.0f - comp_alpha);
          out[BLUE]  = out[BLUE]  * comp_alpha + in[BLUE]  * (1.0f - comp_alpha);
        }
      else
        {
          gint b;
          for (b = RED; b < ALPHA; b++)
            {
              out[b] = in[b];
            }
        }

      out[ALPHA] = in[ALPHA];

      in    += 4;
      layer += 4;
      out   += 4;
    }
}



#endif /* __GIMP_OPERATION_POINT_LAYER_MODE_H__ */
