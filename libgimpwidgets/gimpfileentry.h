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


#define GIMP_TYPE_FILE_ENTRY            (gimp_file_entry_get_type ())
#define GIMP_FILE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FILE_ENTRY, GimpFileEntry))
#define GIMP_FILE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FILE_ENTRY, GimpFileEntryClass))
#define GIMP_IS_FILE_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_FILE_ENTRY))
#define GIMP_IS_FILE_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FILE_ENTRY))
#define GIMP_FILE_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FILE_ENTRY, GimpFileEntryClass))


typedef struct _GimpFileEntryClass  GimpFileEntryClass;

struct _GimpFileEntry
{
  GtkBox     parent_instance;

  GtkWidget *file_exists;
  GtkWidget *entry;
  GtkWidget *browse_button;

  GtkWidget *file_dialog;

  gchar     *title;
  gboolean   dir_only;
  gboolean   check_valid;
};

struct _GimpFileEntryClass
{
  GtkBoxClass  parent_class;

  void (* filename_changed) (GimpFileEntry *entry);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_file_entry_get_type     (void) G_GNUC_CONST;

GtkWidget * gimp_file_entry_new          (const gchar   *title,
                                          const gchar   *filename,
                                          gboolean       dir_only,
                                          gboolean       check_valid);

gchar     * gimp_file_entry_get_filename (GimpFileEntry *entry);
void        gimp_file_entry_set_filename (GimpFileEntry *entry,
                                          const gchar   *filename);


G_END_DECLS

#endif /* __GIMP_FILE_ENTRY_H__ */

#endif /* GIMP_DISABLE_DEPRECATED */
