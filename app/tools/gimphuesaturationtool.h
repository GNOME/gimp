/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_HUE_SATURATION_TOOL_H__
#define __GIMP_HUE_SATURATION_TOOL_H__


#include "gimpimagemaptool.h"


#define GIMP_TYPE_HUE_SATURATION_TOOL            (gimp_hue_saturation_tool_get_type ())
#define GIMP_HUE_SATURATION_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_HUE_SATURATION_TOOL, GimpHueSaturationTool))
#define GIMP_HUE_SATURATION_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_HUE_SATURATION_TOOL, GimpHueSaturationToolClass))
#define GIMP_IS_HUE_SATURATION_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_HUE_SATURATION_TOOL))
#define GIMP_IS_HUE_SATURATION_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_HUE_SATURATION_TOOL))
#define GIMP_HUE_SATURATION_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_HUE_SATURATION_TOOL, GimpHueSaturationToolClass))


typedef struct _GimpHueSaturationTool      GimpHueSaturationTool;
typedef struct _GimpHueSaturationToolClass GimpHueSaturationToolClass;

struct _GimpHueSaturationTool
{
  GimpImageMapTool  parent_instance;

  HueSaturation    *hue_saturation;

  /*  dialog  */
  GimpHueRange      hue_partition;
  GtkWidget        *hue_partition_da[6];
  GtkAdjustment    *overlap_data;
  GtkAdjustment    *hue_data;
  GtkAdjustment    *lightness_data;
  GtkAdjustment    *saturation_data;
};

struct _GimpHueSaturationToolClass
{
  GimpImageMapToolClass  parent_class;
};


void    gimp_hue_saturation_tool_register (GimpToolRegisterCallback  callback,
                                           gpointer                  data);

GType   gimp_hue_saturation_tool_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_HUE_SATURATION_TOOL_H__  */
