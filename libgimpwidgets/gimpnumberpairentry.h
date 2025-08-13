/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpratioentry.h
 * Copyright (C) 2006  Simon Budig       <simon@gimp.org>
 * Copyright (C) 2007  Sven Neumann      <sven@gimp.org>
 * Copyright (C) 2007  Martin Nordholts  <martin@svn.gnome.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_NUMBER_PAIR_ENTRY_H__
#define __GIMP_NUMBER_PAIR_ENTRY_H__

G_BEGIN_DECLS


#define GIMP_TYPE_NUMBER_PAIR_ENTRY (gimp_number_pair_entry_get_type ())
G_DECLARE_FINAL_TYPE (GimpNumberPairEntry, gimp_number_pair_entry, GIMP, NUMBER_PAIR_ENTRY, GtkEntry)


GtkWidget *    gimp_number_pair_entry_new                (const gchar         *separators,
                                                          gboolean             allow_simplification,
                                                          gdouble              min_valid_value,
                                                          gdouble              max_valid_value);
void           gimp_number_pair_entry_set_default_values (GimpNumberPairEntry *entry,
                                                          gdouble              left,
                                                          gdouble              right);
void           gimp_number_pair_entry_get_default_values (GimpNumberPairEntry *entry,
                                                          gdouble             *left,
                                                          gdouble             *right);
void           gimp_number_pair_entry_set_values         (GimpNumberPairEntry *entry,
                                                          gdouble              left,
                                                          gdouble              right);
void           gimp_number_pair_entry_get_values         (GimpNumberPairEntry *entry,
                                                          gdouble             *left,
                                                          gdouble             *right);

void           gimp_number_pair_entry_set_default_text   (GimpNumberPairEntry *entry,
                                                          const gchar         *string);
const gchar *  gimp_number_pair_entry_get_default_text   (GimpNumberPairEntry *entry);

void           gimp_number_pair_entry_set_ratio          (GimpNumberPairEntry *entry,
                                                          gdouble              ratio);
gdouble        gimp_number_pair_entry_get_ratio          (GimpNumberPairEntry *entry);

void           gimp_number_pair_entry_set_aspect         (GimpNumberPairEntry *entry,
                                                          GimpAspectType       aspect);
GimpAspectType gimp_number_pair_entry_get_aspect         (GimpNumberPairEntry *entry);

void           gimp_number_pair_entry_set_user_override  (GimpNumberPairEntry *entry,
                                                          gboolean             user_override);
gboolean       gimp_number_pair_entry_get_user_override  (GimpNumberPairEntry *entry);


G_END_DECLS

#endif /* __GIMP_NUMBER_PAIR_ENTRY_H__ */
