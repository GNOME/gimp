/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaenumwidgets.h
 * Copyright (C) 2002-2004  Sven Neumann <sven@ligma.org>
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_ENUM_WIDGETS_H__
#define __LIGMA_ENUM_WIDGETS_H__

G_BEGIN_DECLS


GtkWidget * ligma_enum_radio_box_new               (GType          enum_type,
                                                   GCallback      callback,
                                                   gpointer       callback_data,
                                                   GDestroyNotify callback_data_destroy,
                                                   GtkWidget    **first_button);
GtkWidget * ligma_enum_radio_box_new_with_range    (GType          enum_type,
                                                   gint           minimum,
                                                   gint           maximum,
                                                   GCallback      callback,
                                                   gpointer       callback_data,
                                                   GDestroyNotify callback_data_destroy,
                                                   GtkWidget    **first_button);

GtkWidget * ligma_enum_radio_frame_new             (GType          enum_type,
                                                   GtkWidget     *label_widget,
                                                   GCallback      callback,
                                                   gpointer       callback_data,
                                                   GDestroyNotify callback_data_destroy,
                                                   GtkWidget    **first_button);
GtkWidget * ligma_enum_radio_frame_new_with_range  (GType          enum_type,
                                                   gint           minimum,
                                                   gint           maximum,
                                                   GtkWidget     *label_widget,
                                                   GCallback      callback,
                                                   gpointer       callback_data,
                                                   GDestroyNotify callback_data_destroy,
                                                   GtkWidget    **first_button);

GtkWidget * ligma_enum_icon_box_new                (GType          enum_type,
                                                   const gchar   *icon_prefix,
                                                   GtkIconSize    icon_size,
                                                   GCallback      callback,
                                                   gpointer       callback_data,
                                                   GDestroyNotify callback_data_destroy,
                                                   GtkWidget    **first_button);
GtkWidget * ligma_enum_icon_box_new_with_range     (GType          enum_type,
                                                   gint           minimum,
                                                   gint           maximum,
                                                   const gchar   *icon_prefix,
                                                   GtkIconSize    icon_size,
                                                   GCallback      callback,
                                                   gpointer       callback_data,
                                                   GDestroyNotify callback_data_destroy,
                                                   GtkWidget    **first_button);
void        ligma_enum_icon_box_set_child_padding  (GtkWidget     *icon_box,
                                                   gint           xpad,
                                                   gint           ypad);

G_END_DECLS

#endif  /* __LIGMA_ENUM_WIDGETS_H__ */
