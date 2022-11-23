/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmalanguageentry.h
 * Copyright (C) 2008  Sven Neumann <sven@ligma.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_LANGUAGE_ENTRY_H__
#define __LIGMA_LANGUAGE_ENTRY_H__


#define LIGMA_TYPE_LANGUAGE_ENTRY            (ligma_language_entry_get_type ())
#define LIGMA_LANGUAGE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LANGUAGE_ENTRY, LigmaLanguageEntry))
#define LIGMA_LANGUAGE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_LANGUAGE_ENTRY, LigmaLanguageEntryClass))
#define LIGMA_IS_LANGUAGE_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LANGUAGE_ENTRY))
#define LIGMA_IS_LANGUAGE_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_LANGUAGE_ENTRY))
#define LIGMA_LANGUAGE_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_LANGUAGE_ENTRY, LigmaLanguageEntryClass))


typedef struct _LigmaLanguageEntryClass  LigmaLanguageEntryClass;

struct _LigmaLanguageEntryClass
{
  GtkEntryClass  parent_class;
};


GType         ligma_language_entry_get_type     (void) G_GNUC_CONST;

GtkWidget   * ligma_language_entry_new      (void);

const gchar * ligma_language_entry_get_code (LigmaLanguageEntry *entry);
gboolean      ligma_language_entry_set_code (LigmaLanguageEntry *entry,
                                            const gchar       *code);


#endif  /* __LIGMA_LANGUAGE_ENTRY_H__ */
