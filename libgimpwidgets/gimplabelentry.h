/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimplabelentry.h
 * Copyright (C) 2022 Jehan
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

#ifndef __GIMP_LABEL_ENTRY_H__
#define __GIMP_LABEL_ENTRY_H__

#include <libgimpwidgets/gimplabeled.h>

G_BEGIN_DECLS

#define GIMP_TYPE_LABEL_ENTRY (gimp_label_entry_get_type ())
G_DECLARE_FINAL_TYPE (GimpLabelEntry, gimp_label_entry, GIMP, LABEL_ENTRY, GimpLabeled)


GtkWidget   * gimp_label_entry_new        (const gchar   *label);

void          gimp_label_entry_set_value  (GimpLabelEntry *entry,
                                           const gchar    *value);
const gchar * gimp_label_entry_get_value  (GimpLabelEntry *entry);

GtkWidget   * gimp_label_entry_get_entry  (GimpLabelEntry *entry);

G_END_DECLS

#endif /* __GIMP_LABEL_ENTRY_H__ */
