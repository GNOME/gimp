/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmabutton.h
 * Copyright (C) 2001 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_BUTTON_H__
#define __LIGMA_BUTTON_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_BUTTON            (ligma_button_get_type ())
#define LIGMA_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BUTTON, LigmaButton))
#define LIGMA_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BUTTON, LigmaButtonClass))
#define LIGMA_IS_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BUTTON))
#define LIGMA_IS_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BUTTON))
#define LIGMA_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BUTTON, LigmaButtonClass))


typedef struct _LigmaButtonPrivate LigmaButtonPrivate;
typedef struct _LigmaButtonClass   LigmaButtonClass;

struct _LigmaButton
{
  GtkButton         parent_instance;

  LigmaButtonPrivate *priv;
};

struct _LigmaButtonClass
{
  GtkButtonClass  parent_class;

  void (* extended_clicked) (LigmaButton      *button,
                             GdkModifierType  modifier_state);

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


GType       ligma_button_get_type         (void) G_GNUC_CONST;

GtkWidget * ligma_button_new              (void);

void        ligma_button_extended_clicked (LigmaButton      *button,
                                          GdkModifierType  modifier_state);


G_END_DECLS

#endif /* __LIGMA_BUTTON_H__ */
