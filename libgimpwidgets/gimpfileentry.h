/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmafileentry.h
 * Copyright (C) 1999-2004 Michael Natterer <mitch@ligma.org>
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

#ifndef LIGMA_DISABLE_DEPRECATED

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_FILE_ENTRY_H__
#define __LIGMA_FILE_ENTRY_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_FILE_ENTRY            (ligma_file_entry_get_type ())
#define LIGMA_FILE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FILE_ENTRY, LigmaFileEntry))
#define LIGMA_FILE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FILE_ENTRY, LigmaFileEntryClass))
#define LIGMA_IS_FILE_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_FILE_ENTRY))
#define LIGMA_IS_FILE_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FILE_ENTRY))
#define LIGMA_FILE_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FILE_ENTRY, LigmaFileEntryClass))


typedef struct _LigmaFileEntryPrivate LigmaFileEntryPrivate;
typedef struct _LigmaFileEntryClass   LigmaFileEntryClass;

struct _LigmaFileEntry
{
  GtkBox                parent_instance;

  LigmaFileEntryPrivate *priv;
};

struct _LigmaFileEntryClass
{
  GtkBoxClass  parent_class;

  void (* filename_changed) (LigmaFileEntry *entry);

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType       ligma_file_entry_get_type     (void) G_GNUC_CONST;

GtkWidget * ligma_file_entry_new          (const gchar   *title,
                                          const gchar   *filename,
                                          gboolean       dir_only,
                                          gboolean       check_valid);

gchar     * ligma_file_entry_get_filename (LigmaFileEntry *entry);
void        ligma_file_entry_set_filename (LigmaFileEntry *entry,
                                          const gchar   *filename);

GtkWidget * ligma_file_entry_get_entry    (LigmaFileEntry *entry);


G_END_DECLS

#endif /* __LIGMA_FILE_ENTRY_H__ */

#endif /* LIGMA_DISABLE_DEPRECATED */
