/* The GIMP -- an image manipulation program
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


typedef enum
{
  ALL_HUES,
  RED_HUES,
  YELLOW_HUES,
  GREEN_HUES,
  CYAN_HUES,
  BLUE_HUES,
  MAGENTA_HUES
} HueRange;


#define GIMP_TYPE_HUE_SATURATION_TOOL            (gimp_hue_saturation_tool_get_type ())
#define GIMP_HUE_SATURATION_TOOL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_HUE_SATURATION_TOOL, GimpHueSaturationTool))
#define GIMP_IS_HUE_SATURATION_TOOL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_HUE_SATURATION_TOOL))
#define GIMP_HUE_SATURATION_TOOL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_HUE_SATURATION_TOOL, GimpHueSaturationToolClass))
#define GIMP_IS_HUE_SATURATION_TOOL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_HUE_SATURATION_TOOL))


typedef struct _GimpHueSaturationTool      GimpHueSaturationTool;
typedef struct _GimpHueSaturationToolClass GimpHueSaturationToolClass;

struct _GimpHueSaturationTool
{
  GimpImageMapTool  parent_instance;
};

struct _GimpHueSaturationToolClass
{
  GimpImageMapToolClass  parent_class;
};


typedef struct _HueSaturationDialog HueSaturationDialog;

struct _HueSaturationDialog
{
  GtkWidget     *shell;

  GtkWidget     *hue_partition_da[6];

  GtkAdjustment *hue_data;
  GtkAdjustment *lightness_data;
  GtkAdjustment *saturation_data;

  GimpDrawable  *drawable;
  ImageMap      *image_map;

  gdouble        hue[7];
  gdouble        lightness[7];
  gdouble        saturation[7];

  HueRange       hue_partition;
  gboolean       preview;
};


void       gimp_hue_saturation_tool_register (Gimp *gimp);

GtkType    gimp_hue_saturation_tool_get_type (void);

void   hue_saturation_free                (void);
void   hue_saturation_dialog_hide	  (void);
void   hue_saturation                     (PixelRegion          *srcPR,
					   PixelRegion          *destPR,
					   void                 *data);

void   hue_saturation_calculate_transfers (HueSaturationDialog  *hsd);


#endif  /*  __HUE_SATURATION_H__  */
