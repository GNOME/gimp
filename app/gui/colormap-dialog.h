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
  GtkDialog parent;

  GimpImage        *image;
  gint              col_index;
  gint              dnd_col_index;
  GtkWidget        *vbox;
  GtkPreview       *palette;
  GtkWidget        *image_menu;
  GtkWidget        *popup_menu;
  GtkOptionMenu    *option_menu;
  GimpContainer    *context;
  guint             event_handler;
  gint              xn;
  gint              yn;
  gint              cellsize;
  GtkWidget        *index_spinbutton;
  GtkAdjustment    *index_adjustment;
  GtkEntry         *color_entry;
  GQuark            rename_handler_id;
  GQuark            cmap_changed_handler_id;
  GtkWidget        *add_item;
  ColorNotebook    *color_notebook;
};

struct _GimpColormapDialogClass
{
  GtkDialogClass parent_class;

  void (* selected) (GimpColormapDialog *gcd);
};


GtkType              gimp_colormap_dialog_get_type (void);
GimpColormapDialog * gimp_colormap_dialog_create   (GimpContainer    *context);

void        gimp_colormap_dialog_selected  (GimpColormapDialog       *colormap_dialog);

GimpImage * gimp_colormap_dialog_image     (const GimpColormapDialog *colormap_dialog);
gint        gimp_colormap_dialog_col_index (const GimpColormapDialog *colormap_dialog);


#endif /* __COLORMAP_DIALOG_H__ */
