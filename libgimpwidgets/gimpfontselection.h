/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpfontselection.h
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

#ifndef __GIMP_FONT_SELECTION_H__
#define __GIMP_FONT_SELECTION_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_FONT_SELECTION            (gimp_font_selection_get_type ())
#define GIMP_FONT_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FONT_SELECTION, GimpFontSelection))
#define GIMP_FONT_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FONT_SELECTION, GimpFontSelectionClass))
#define GIMP_IS_FONT_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_FONT_SELECTION))
#define GIMP_IS_FONT_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FONT_SELECTION))


typedef struct _GimpFontSelectionClass  GimpFontSelectionClass;

struct _GimpFontSelection
{
  GtkVBox               parent_instance;

  GtkWidget            *font_clist;
  GtkWidget            *font_style_clist;

  PangoContext         *context;
  PangoFontDescription *font_desc;
};

struct _GimpFontSelectionClass
{
  GtkVBoxClass          parent_class;

  void (* font_changed) (GimpFontSelection *fontsel);
};


GType        gimp_font_selection_get_type     (void);
GtkWidget *  gimp_font_selection_new          (PangoContext      *context);
gboolean     gimp_font_selection_set_fontname (GimpFontSelection *fontsel,
                                               const gchar       *fontname);
const gchar *gimp_font_selection_get_fontname (GimpFontSelection *fontsel);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_FONT_SELECTION_H__ */
