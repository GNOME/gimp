/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmemsizeentry.h
 * Copyright (C) 2000-2003  Sven Neumann <sven@gimp.org>
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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_MEMSIZE_ENTRY_H__
#define __GIMP_MEMSIZE_ENTRY_H__

G_BEGIN_DECLS


#define GIMP_TYPE_MEMSIZE_ENTRY            (gimp_memsize_entry_get_type ())
#define GIMP_MEMSIZE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MEMSIZE_ENTRY, GimpMemsizeEntry))
#define GIMP_MEMSIZE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MEMSIZE_ENTRY, GimpMemsizeEntryClass))
#define GIMP_IS_MEMSIZE_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MEMSIZE_ENTRY))
#define GIMP_IS_MEMSIZE_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MEMSIZE_ENTRY))
#define GIMP_MEMSIZE_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MEMSIZE_ENTRY, GimpMemsizeEntryClass))


typedef struct _GimpMemsizeEntryClass  GimpMemsizeEntryClass;

struct _GimpMemsizeEntry
{
  GtkBox  parent_instance;
};

struct _GimpMemsizeEntryClass
{
  GtkBoxClass  parent_class;

  void (* value_changed)  (GimpMemsizeEntry *entry);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_memsize_entry_get_type       (void) G_GNUC_CONST;

GtkWidget * gimp_memsize_entry_new            (guint64           value,
                                               guint64           lower,
                                               guint64           upper);
void        gimp_memsize_entry_set_value      (GimpMemsizeEntry *entry,
                                               guint64           value);
guint64     gimp_memsize_entry_get_value      (GimpMemsizeEntry *entry);

GtkWidget * gimp_memsize_entry_get_spinbutton (GimpMemsizeEntry *entry);


G_END_DECLS

#endif /* __GIMP_MEMSIZE_ENTRY_H__ */
