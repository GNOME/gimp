/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpfontselection-dialog.h
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_FONT_SELECTION_DIALOG_H__
#define __GIMP_FONT_SELECTION_DIALOG_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GIMP_TYPE_FONT_SELECTION_DIALOG            (gimp_font_selection_dialog_get_type ())
#define GIMP_FONT_SELECTION_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FONT_SELECTION_DIALOG, GimpFontSelectionDialog))
#define GIMP_FONT_SELECTION_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FONT_SELECTION_DIALOG, GimpFontSelectionDialogClass))
#define GIMP_IS_FONT_SELECTION_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_FONT_SELECTION_DIALOG))
#define GIMP_IS_FONT_SELECTION_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FONT_SELECTION_DIALOG))


typedef struct _GimpFontSelectionDialog       GimpFontSelectionDialog;
typedef struct _GimpFontSelectionDialogClass  GimpFontSelectionDialogClass;

struct _GimpFontSelectionDialog
{
  GtkDialog             parent_instance;

  GimpFontSelection    *fontsel;
  PangoFontDescription *font_desc;

  GtkWidget            *font_clist;
  GtkWidget            *font_style_clist;
};

struct _GimpFontSelectionDialogClass
{
  GtkDialogClass        parent_class;
};


GType        gimp_font_selection_dialog_get_type (void);
GtkWidget *  gimp_font_selection_dialog_new      (GimpFontSelection *fontsel);
void    gimp_font_selection_dialog_set_font_desc (GimpFontSelectionDialog *dialog,
                                                  PangoFontDescription    *new_desc);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_FONT_SELECTION_DIALOG_H__ */
