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

#ifndef __GIMP_COLOR_BALANCE_DIALOG_H__
#define __GIMP_COLOR_BALANCE_DIALOG_H__


#include "gimpimagemaptool.h"


#define GIMP_TYPE_COLOR_BALANCE_TOOL            (gimp_color_balance_tool_get_type ())
#define GIMP_COLOR_BALANCE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_BALANCE_TOOL, GimpColorBalanceTool))
#define GIMP_COLOR_BALANCE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_BALANCE_TOOL, GimpColorBalanceToolClass))
#define GIMP_IS_COLOR_BALANCE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_BALANCE_TOOL))
#define GIMP_COLOR_BALANCE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_BALANCE_TOOL, GimpColorBalanceToolClass))
#define GIMP_IS_COLOR_BALANCE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_BALANCE_TOOL))
#define GIMP_COLOR_BALANCE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_BALANCE_TOOL, GimpColorBalanceToolClass))


typedef struct _GimpColorBalanceTool      GimpColorBalanceTool;
typedef struct _GimpColorBalanceToolClass GimpColorBalanceToolClass;

struct _GimpColorBalanceTool
{
  GimpImageMapTool  parent_instance;
};

struct _GimpColorBalanceToolClass
{
  GimpImageMapToolClass  parent_class;
};


typedef struct _ColorBalanceDialog ColorBalanceDialog;

struct _ColorBalanceDialog
{
  GtkWidget        *shell;

  GtkAdjustment    *cyan_red_data;
  GtkAdjustment    *magenta_green_data;
  GtkAdjustment    *yellow_blue_data;

  GimpDrawable     *drawable;
  ImageMap         *image_map;

  gdouble           cyan_red[3];
  gdouble           magenta_green[3];
  gdouble           yellow_blue[3];

  guchar            r_lookup[256];
  guchar            g_lookup[256];
  guchar            b_lookup[256];

  gboolean          preserve_luminosity;
  gboolean          preview;
  GimpTransferMode  application_mode;
};


void   gimp_color_balance_tool_register   (Gimp *gimp);

GType  gimp_color_balance_tool_get_type   (void);


void   color_balance_dialog_hide	  (void);
void   color_balance                      (PixelRegion       *srcPR,
					   PixelRegion       *destPR,
					   void              *data);

void   color_balance_create_lookup_tables (ColorBalanceDialog *cbd);


#endif  /*  __GIMP_COLOR_BALANCE_GIMP_H__  */
