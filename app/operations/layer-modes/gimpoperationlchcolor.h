/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationlchcolor.h
 * Copyright (C) 2015 Elle Stone <ellestone@ninedegreesbelow.com>
 *                    Massimo Valentini <mvalentini@src.gnome.org>
 *                    Thomas Manni <thomas.manni@free.fr>
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

#ifndef __GIMP_OPERATION_LCH_COLOR_H__
#define __GIMP_OPERATION_LCH_COLOR_H__


#include "gimpoperationpointlayermode.h"


#define GIMP_TYPE_OPERATION_LCH_COLOR            (gimp_operation_lch_color_get_type ())
#define GIMP_OPERATION_LCH_COLOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_LCH_COLOR, GimpOperationLchColor))
#define GIMP_OPERATION_LCH_COLOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_LCH_COLOR, GimpOperationLchColorClass))
#define GIMP_IS_OPERATION_LCH_COLOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_LCH_COLOR))
#define GIMP_IS_OPERATION_LCH_COLOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_LCH_COLOR))
#define GIMP_OPERATION_LCH_COLOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_LCH_COLOR, GimpOperationLchColorClass))


typedef struct _GimpOperationLchColor      GimpOperationLchColor;
typedef struct _GimpOperationLchColorClass GimpOperationLchColorClass;

struct _GimpOperationLchColor
{
  GimpOperationPointLayerMode  parent_instance;
};

struct _GimpOperationLchColorClass
{
  GimpOperationPointLayerModeClass  parent_class;
};


GType    gimp_operation_lch_color_get_type              (void) G_GNUC_CONST;

gboolean gimp_operation_lch_color_process_pixels_linear (gfloat                *in,
                                                         gfloat                *layer,
                                                         gfloat                *mask,
                                                         gfloat                *out,
                                                         gfloat                 opacity,
                                                         glong                  samples,
                                                         const GeglRectangle   *roi,
                                                         gint                   level,
                                                         GimpLayerBlendTRC      blend_trc,
                                                         GimpLayerBlendTRC      composite_trc,
                                                         GimpLayerCompositeMode composite_mode);

gboolean gimp_operation_lch_color_process_pixels        (gfloat                *in,
                                                         gfloat                *layer,
                                                         gfloat                *mask,
                                                         gfloat                *out,
                                                         gfloat                 opacity,
                                                         glong                  samples,
                                                         const GeglRectangle   *roi,
                                                         gint                   level,
                                                         GimpLayerBlendTRC      blend_trc,
                                                         GimpLayerBlendTRC      composite_trc,
                                                         GimpLayerCompositeMode composite_mode);


#endif /* __GIMP_OPERATION_LCH_COLOR_H__ */
