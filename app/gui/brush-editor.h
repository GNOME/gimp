/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * brush_edit module Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#ifndef  __BRUSH_EDIT_H__
#define  __BRUSH_EDIT_H__

#include "gimpbrushgenerated.h"

typedef struct _BrushEditGeneratedWindow
{
  GtkWidget *shell;
  GtkWidget *frame;
  GtkWidget *preview;
  GtkWidget *scale_label;
  GtkWidget *options_box;
  GtkWidget *name;
  GtkAdjustment *radius_data;
  GtkAdjustment *hardness_data;
  GtkAdjustment *angle_data;
  GtkAdjustment *aspect_ratio_data;
  /*  Brush preview  */
  GtkWidget *brush_preview;
  GimpBrushGenerated *brush;
  int scale;
} BrushEditGeneratedWindow;

void brush_edit_generated_set_brush(BrushEditGeneratedWindow *begw,
				    GimpBrush *brush);

BrushEditGeneratedWindow *brush_edit_generated_new ();

#endif  /*  __BRUSH_EDIT_H__  */
