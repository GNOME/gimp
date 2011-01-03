/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorselection.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_SELECTION_H__
#define __GIMP_COLOR_SELECTION_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


#define GIMP_TYPE_COLOR_SELECTION            (gimp_color_selection_get_type ())
#define GIMP_COLOR_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_SELECTION, GimpColorSelection))
#define GIMP_COLOR_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_SELECTION, GimpColorSelectionClass))
#define GIMP_IS_COLOR_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_SELECTION))
#define GIMP_IS_COLOR_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_SELECTION))
#define GIMP_COLOR_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_SELECTION, GimpColorSelectionClass))


typedef struct _GimpColorSelectionClass GimpColorSelectionClass;

struct _GimpColorSelection
{
  GtkBox  parent_instance;
};

struct _GimpColorSelectionClass
{
  GtkBoxClass  parent_class;

  void (* color_changed) (GimpColorSelection *selection);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_color_selection_get_type       (void) G_GNUC_CONST;

GtkWidget * gimp_color_selection_new            (void);

void        gimp_color_selection_set_show_alpha (GimpColorSelection *selection,
                                                 gboolean           show_alpha);
gboolean    gimp_color_selection_get_show_alpha (GimpColorSelection *selection);

void        gimp_color_selection_set_color      (GimpColorSelection *selection,
                                                 const GimpRGB      *color);
void        gimp_color_selection_get_color      (GimpColorSelection *selection,
                                                 GimpRGB            *color);

void        gimp_color_selection_set_old_color  (GimpColorSelection *selection,
                                                 const GimpRGB      *color);
void        gimp_color_selection_get_old_color  (GimpColorSelection *selection,
                                                 GimpRGB            *color);

void        gimp_color_selection_reset          (GimpColorSelection *selection);

void        gimp_color_selection_color_changed  (GimpColorSelection *selection);

void        gimp_color_selection_set_config     (GimpColorSelection *selection,
                                                 GimpColorConfig    *config);

GtkWidget * gimp_color_selection_get_notebook   (GimpColorSelection *selection);
GtkWidget * gimp_color_selection_get_right_vbox (GimpColorSelection *selection);


G_END_DECLS

#endif /* __GIMP_COLOR_SELECTION_H__ */
