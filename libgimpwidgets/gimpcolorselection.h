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
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_COLOR_SELECTION_H__
#define __GIMP_COLOR_SELECTION_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


#define GIMP_TYPE_COLOR_SELECTION (gimp_color_selection_get_type ())
G_DECLARE_FINAL_TYPE (GimpColorSelection, gimp_color_selection, GIMP, COLOR_SELECTION, GtkBox)


GtkWidget * gimp_color_selection_new            (void);

void        gimp_color_selection_set_show_alpha (GimpColorSelection *selection,
                                                 gboolean           show_alpha);
gboolean    gimp_color_selection_get_show_alpha (GimpColorSelection *selection);

void        gimp_color_selection_set_color      (GimpColorSelection *selection,
                                                 GeglColor          *color);
GeglColor * gimp_color_selection_get_color      (GimpColorSelection *selection);

void        gimp_color_selection_set_old_color  (GimpColorSelection *selection,
                                                 GeglColor          *color);
GeglColor * gimp_color_selection_get_old_color  (GimpColorSelection *selection);

void        gimp_color_selection_reset          (GimpColorSelection *selection);

void        gimp_color_selection_color_changed  (GimpColorSelection *selection);

void        gimp_color_selection_set_format     (GimpColorSelection *selection,
                                                 const Babl          *format);
void        gimp_color_selection_set_simulation (GimpColorSelection *selection,
                                                 GimpColorProfile   *profile,
                                                 GimpColorRenderingIntent intent,
                                                 gboolean            bpc);

void        gimp_color_selection_set_config     (GimpColorSelection *selection,
                                                 GimpColorConfig    *config);

GtkWidget * gimp_color_selection_get_notebook   (GimpColorSelection *selection);
GtkWidget * gimp_color_selection_get_right_vbox (GimpColorSelection *selection);


G_END_DECLS

#endif /* __GIMP_COLOR_SELECTION_H__ */
