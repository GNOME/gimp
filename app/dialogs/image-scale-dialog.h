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

#ifndef __IMAGE_SCALE_LAYERS_DIALOG_H__
#define __IMAGE_SCALE_LAYERS_DIALOG_H__


typedef struct _ImageScaleDialog ImageScaleDialog;

typedef void (* ImageScaleDialogCallback) (ImageScaleDialog *dialog);

struct _ImageScaleDialog
{
  GtkWidget                *dialog;

  GimpImage                *gimage;
  GimpDisplay              *gdisp;
  GimpContext              *context;

  gint                      width;
  gint                      height;
  GimpUnit                  unit;
  GimpInterpolationType     interpolation;
  gdouble                   xresolution;
  gdouble                   yresolution;
  GimpUnit                  resolution_unit;

  ImageScaleDialogCallback  callback;
};


ImageScaleDialog * image_scale_dialog_new (GimpImage                *image,
                                           GimpDisplay              *display,
                                           GimpContext              *context,
                                           GtkWidget                *parent,
                                           ImageScaleDialogCallback  callback);


#endif /* __IMAGE_SCALE_DIALOG_H__ */
