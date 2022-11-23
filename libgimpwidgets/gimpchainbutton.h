/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmachainbutton.h
 * Copyright (C) 1999-2000 Sven Neumann <sven@ligma.org>
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

/*
 * This implements a widget derived from GtkGrid that visualizes
 * it's state with two different pixmaps showing a closed and a
 * broken chain. It's intended to be used with the LigmaSizeEntry
 * widget. The usage is quite similar to the one the GtkToggleButton
 * provides.
 */

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_CHAIN_BUTTON_H__
#define __LIGMA_CHAIN_BUTTON_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_CHAIN_BUTTON            (ligma_chain_button_get_type ())
#define LIGMA_CHAIN_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CHAIN_BUTTON, LigmaChainButton))
#define LIGMA_CHAIN_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CHAIN_BUTTON, LigmaChainButtonClass))
#define LIGMA_IS_CHAIN_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CHAIN_BUTTON))
#define LIGMA_IS_CHAIN_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CHAIN_BUTTON))
#define LIGMA_CHAIN_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CHAIN_BUTTON, LigmaChainButtonClass))


typedef struct _LigmaChainButtonPrivate LigmaChainButtonPrivate;
typedef struct _LigmaChainButtonClass   LigmaChainButtonClass;

struct _LigmaChainButton
{
  GtkGrid                 parent_instance;

  LigmaChainButtonPrivate *priv;
};

struct _LigmaChainButtonClass
{
  GtkGridClass  parent_class;

  void (* toggled)  (LigmaChainButton *button);

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


GType         ligma_chain_button_get_type      (void) G_GNUC_CONST;

GtkWidget   * ligma_chain_button_new           (LigmaChainPosition  position);

void          ligma_chain_button_set_icon_size (LigmaChainButton   *button,
                                               GtkIconSize        size);
GtkIconSize   ligma_chain_button_get_icon_size (LigmaChainButton   *button);

void          ligma_chain_button_set_active    (LigmaChainButton   *button,
                                               gboolean           active);
gboolean      ligma_chain_button_get_active    (LigmaChainButton   *button);

GtkWidget   * ligma_chain_button_get_button    (LigmaChainButton   *button);


G_END_DECLS

#endif /* __LIGMA_CHAIN_BUTTON_H__ */
