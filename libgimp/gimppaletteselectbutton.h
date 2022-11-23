/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapaletteselectbutton.h
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

#if !defined (__LIGMA_UI_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligmaui.h> can be included directly."
#endif

#ifndef __LIGMA_PALETTE_SELECT_BUTTON_H__
#define __LIGMA_PALETTE_SELECT_BUTTON_H__

#include <libligma/ligmaselectbutton.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_PALETTE_SELECT_BUTTON            (ligma_palette_select_button_get_type ())
#define LIGMA_PALETTE_SELECT_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PALETTE_SELECT_BUTTON, LigmaPaletteSelectButton))
#define LIGMA_PALETTE_SELECT_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PALETTE_SELECT_BUTTON, LigmaPaletteSelectButtonClass))
#define LIGMA_IS_PALETTE_SELECT_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PALETTE_SELECT_BUTTON))
#define LIGMA_IS_PALETTE_SELECT_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PALETTE_SELECT_BUTTON))
#define LIGMA_PALETTE_SELECT_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PALETTE_SELECT_BUTTON, LigmaPaletteSelectButtonClass))


typedef struct _LigmaPaletteSelectButtonPrivate LigmaPaletteSelectButtonPrivate;
typedef struct _LigmaPaletteSelectButtonClass   LigmaPaletteSelectButtonClass;

struct _LigmaPaletteSelectButton
{
  LigmaSelectButton                parent_instance;

  LigmaPaletteSelectButtonPrivate *priv;
};

struct _LigmaPaletteSelectButtonClass
{
  LigmaSelectButtonClass  parent_class;

  /* palette_set signal is emitted when palette is chosen */
  void (* palette_set) (LigmaPaletteSelectButton *button,
                        const gchar             *palette_name,
                        gboolean                 dialog_closing);

  /* Padding for future expansion */
  void (*_ligma_reserved1) (void);
  void (*_ligma_reserved2) (void);
  void (*_ligma_reserved3) (void);
  void (*_ligma_reserved4) (void);
  void (*_ligma_reserved5) (void);
  void (*_ligma_reserved6) (void);
  void (*_ligma_reserved7) (void);
  void (*_ligma_reserved8) (void);
};


GType         ligma_palette_select_button_get_type    (void) G_GNUC_CONST;

GtkWidget   * ligma_palette_select_button_new         (const gchar *title,
                                                      const gchar *palette_name);

const gchar * ligma_palette_select_button_get_palette (LigmaPaletteSelectButton *button);
void          ligma_palette_select_button_set_palette (LigmaPaletteSelectButton *button,
                                                      const gchar             *palette_name);


G_END_DECLS

#endif /* __LIGMA_PALETTE_SELECT_BUTTON_H__ */
