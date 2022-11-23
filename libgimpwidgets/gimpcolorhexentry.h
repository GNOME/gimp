/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorhexentry.h
 * Copyright (C) 2004  Sven Neumann <sven@ligma.org>
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_COLOR_HEX_ENTRY_H__
#define __LIGMA_COLOR_HEX_ENTRY_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_COLOR_HEX_ENTRY            (ligma_color_hex_entry_get_type ())
#define LIGMA_COLOR_HEX_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_HEX_ENTRY, LigmaColorHexEntry))
#define LIGMA_COLOR_HEX_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_HEX_ENTRY, LigmaColorHexEntryClass))
#define LIGMA_IS_COLOR_HEX_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_HEX_ENTRY))
#define LIGMA_IS_COLOR_HEX_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_HEX_ENTRY))
#define LIGMA_COLOR_HEX_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_HEX_AREA, LigmaColorHexEntryClass))


typedef struct _LigmaColorHexEntryPrivate LigmaColorHexEntryPrivate;
typedef struct _LigmaColorHexEntryClass   LigmaColorHexEntryClass;

struct _LigmaColorHexEntry
{
  GtkEntry                  parent_instance;

  LigmaColorHexEntryPrivate *priv;
};

struct _LigmaColorHexEntryClass
{
  GtkEntryClass   parent_class;

  void (* color_changed) (LigmaColorHexEntry *entry);

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


GType       ligma_color_hex_entry_get_type  (void) G_GNUC_CONST;

GtkWidget * ligma_color_hex_entry_new       (void);

void        ligma_color_hex_entry_set_color (LigmaColorHexEntry *entry,
                                            const LigmaRGB     *color);
void        ligma_color_hex_entry_get_color (LigmaColorHexEntry *entry,
                                            LigmaRGB           *color);


G_END_DECLS

#endif /* __LIGMA_COLOR_HEX_ENTRY_H__ */
