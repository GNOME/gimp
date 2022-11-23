/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmalabelentry.h
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_LABEL_ENTRY_H__
#define __LIGMA_LABEL_ENTRY_H__

#include <libligmawidgets/ligmalabeled.h>

G_BEGIN_DECLS

#define LIGMA_TYPE_LABEL_ENTRY (ligma_label_entry_get_type ())
G_DECLARE_DERIVABLE_TYPE (LigmaLabelEntry, ligma_label_entry, LIGMA, LABEL_ENTRY, LigmaLabeled)

struct _LigmaLabelEntryClass
{
  LigmaLabeledClass   parent_class;

  /*  Signals */
  void (* value_changed)   (LigmaLabelEntry *entry);

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

GtkWidget   * ligma_label_entry_new        (const gchar   *label);

void          ligma_label_entry_set_value  (LigmaLabelEntry *entry,
                                           const gchar    *value);
const gchar * ligma_label_entry_get_value  (LigmaLabelEntry *entry);

GtkWidget   * ligma_label_entry_get_entry  (LigmaLabelEntry *entry);

G_END_DECLS

#endif /* __LIGMA_LABEL_ENTRY_H__ */
