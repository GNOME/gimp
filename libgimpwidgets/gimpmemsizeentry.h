/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmamemsizeentry.h
 * Copyright (C) 2000-2003  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_MEMSIZE_ENTRY_H__
#define __LIGMA_MEMSIZE_ENTRY_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_MEMSIZE_ENTRY            (ligma_memsize_entry_get_type ())
#define LIGMA_MEMSIZE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MEMSIZE_ENTRY, LigmaMemsizeEntry))
#define LIGMA_MEMSIZE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MEMSIZE_ENTRY, LigmaMemsizeEntryClass))
#define LIGMA_IS_MEMSIZE_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MEMSIZE_ENTRY))
#define LIGMA_IS_MEMSIZE_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MEMSIZE_ENTRY))
#define LIGMA_MEMSIZE_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MEMSIZE_ENTRY, LigmaMemsizeEntryClass))


typedef struct _LigmaMemsizeEntryPrivate LigmaMemsizeEntryPrivate;
typedef struct _LigmaMemsizeEntryClass   LigmaMemsizeEntryClass;

struct _LigmaMemsizeEntry
{
  GtkBox  parent_instance;

  LigmaMemsizeEntryPrivate *priv;
};

struct _LigmaMemsizeEntryClass
{
  GtkBoxClass  parent_class;

  void (* value_changed)  (LigmaMemsizeEntry *entry);

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


GType       ligma_memsize_entry_get_type       (void) G_GNUC_CONST;

GtkWidget * ligma_memsize_entry_new            (guint64           value,
                                               guint64           lower,
                                               guint64           upper);
void        ligma_memsize_entry_set_value      (LigmaMemsizeEntry *entry,
                                               guint64           value);
guint64     ligma_memsize_entry_get_value      (LigmaMemsizeEntry *entry);

GtkWidget * ligma_memsize_entry_get_spinbutton (LigmaMemsizeEntry *entry);


G_END_DECLS

#endif /* __LIGMA_MEMSIZE_ENTRY_H__ */
