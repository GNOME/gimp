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

#ifndef __COLORMAP_DIALOG_H__
#define __COLORMAP_DIALOG_H__


#include <gtk/gtkdialog.h>


#define GIMP_TYPE_COLORMAP_DIALOG            (gimp_colormap_dialog_get_type ())
#define GIMP_COLORMAP_DIALOG(obj)            (GTK_CHECK_CAST((obj), GIMP_TYPE_COLORMAP_DIALOG, GimpColormapDialog))
#define GIMP_COLORMAP_DIALOG_CLASS(klass)    (GTK_CHECK_CLASS_CAST((klass), GIMP_TYPE_COLORMAP_DIALOG, GimpColormapDialogClass))
#define GIMP_IS_COLORMAP_DIALOG(obj)         (GTK_CHECK_TYPE((obj), GIMP_TYPE_COLORMAP_DIALOG))
#define GIMP_IS_COLORMAP_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_TYPE((klass), GIMP_TYPE_COLORMAP_DIALOG))


typedef struct _GimpColormapDialog      GimpColormapDialog;
typedef struct _GimpColormapDialogClass GimpColormapDialogClass;

struct _GimpColormapDialog
{
  GtkVBox        parent;

  GimpImage     *image;
  gint           col_index;
  gint           dnd_col_index;
  GtkWidget     *palette;
  GtkWidget     *image_menu;
  GtkWidget     *popup_menu;
  GtkOptionMenu *option_menu;
  gint           xn;
  gint           yn;
  gint           cellsize;
  GtkAdjustment *index_adjustment;
  GtkWidget     *index_spinbutton;
  GtkWidget     *color_entry;
  GtkWidget     *add_item;
  ColorNotebook *color_notebook;
};

struct _GimpColormapDialogClass
{
  GtkVBoxClass  parent_class;

  void (* selected) (GimpColormapDialog *gcd);
};


GtkType     gimp_colormap_dialog_get_type  (void);

GtkWidget * gimp_colormap_dialog_new       (GimpImage          *gimage);

void        gimp_colormap_dialog_selected  (GimpColormapDialog *gcd);

void        gimp_colormap_dialog_set_image (GimpColormapDialog *gcd,
					    GimpImage          *gimage);
GimpImage * gimp_colormap_dialog_get_image (GimpColormapDialog *gcd);
gint        gimp_colormap_dialog_col_index (GimpColormapDialog *gcd);


#endif /* __COLORMAP_DIALOG_H__ */
