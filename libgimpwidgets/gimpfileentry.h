/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpfileentry.h
 * Copyright (C) 1999-2004 Michael Natterer <mitch@gimp.org>
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

#ifndef GIMP_DISABLE_DEPRECATED

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_FILE_ENTRY_H__
#define __GIMP_FILE_ENTRY_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

/* The whole GimpFileEntry widget was deprecated in 2006 (commit
 * 99f979e1189) apparently to be replaced by GtkFileChooserButton, yet
 * it is still used in the GimpPathEditor. For GIMP 3.0, we removed the
 * header from installed headers and made the class and all functions
 * internal, though we still use it.
 *
 * TODO: eventually we want either this widget fully removed and
 * replaced by a generic GTK widget or reimplemented and made public
 * again.
 */

#define GIMP_TYPE_FILE_ENTRY (_gimp_file_entry_get_type ())
G_DECLARE_FINAL_TYPE (GimpFileEntry, _gimp_file_entry, GIMP, FILE_ENTRY, GtkBox)


G_GNUC_INTERNAL GtkWidget * _gimp_file_entry_new          (const gchar   *title,
                                                           const gchar   *filename,
                                                           gboolean       dir_only,
                                                           gboolean       check_valid);

G_GNUC_INTERNAL gchar     * _gimp_file_entry_get_filename (GimpFileEntry *entry);
G_GNUC_INTERNAL void        _gimp_file_entry_set_filename (GimpFileEntry *entry,
                                                           const gchar   *filename);

G_GNUC_INTERNAL GtkWidget * _gimp_file_entry_get_entry    (GimpFileEntry *entry);


G_END_DECLS

#endif /* __GIMP_FILE_ENTRY_H__ */

#endif /* GIMP_DISABLE_DEPRECATED */
