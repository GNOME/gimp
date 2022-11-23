/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacombotagentry.h
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#ifndef __LIGMA_COMBO_TAG_ENTRY_H__
#define __LIGMA_COMBO_TAG_ENTRY_H__

#include "ligmatagentry.h"

#define LIGMA_TYPE_COMBO_TAG_ENTRY            (ligma_combo_tag_entry_get_type ())
#define LIGMA_COMBO_TAG_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COMBO_TAG_ENTRY, LigmaComboTagEntry))
#define LIGMA_COMBO_TAG_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COMBO_TAG_ENTRY, LigmaComboTagEntryClass))
#define LIGMA_IS_COMBO_TAG_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COMBO_TAG_ENTRY))
#define LIGMA_IS_COMBO_TAG_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COMBO_TAG_ENTRY))
#define LIGMA_COMBO_TAG_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COMBO_TAG_ENTRY, LigmaComboTagEntryClass))


typedef struct _LigmaComboTagEntryClass  LigmaComboTagEntryClass;

struct _LigmaComboTagEntry
{
  LigmaTagEntry  parent_instance;

  GtkWidget    *popup;
};

struct _LigmaComboTagEntryClass
{
  LigmaTagEntryClass  parent_class;
};


GType       ligma_combo_tag_entry_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_combo_tag_entry_new      (LigmaTaggedContainer *container,
                                           LigmaTagEntryMode     mode);


#endif  /*  __LIGMA_COMBO_TAG_ENTRY_H__  */
