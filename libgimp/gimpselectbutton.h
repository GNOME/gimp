/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaselectbutton.h
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

#ifndef __LIGMA_SELECT_BUTTON_H__
#define __LIGMA_SELECT_BUTTON_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_SELECT_BUTTON            (ligma_select_button_get_type ())
#define LIGMA_SELECT_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SELECT_BUTTON, LigmaSelectButton))
#define LIGMA_SELECT_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SELECT_BUTTON, LigmaSelectButtonClass))
#define LIGMA_IS_SELECT_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SELECT_BUTTON))
#define LIGMA_IS_SELECT_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SELECT_BUTTON))
#define LIGMA_SELECT_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SELECT_BUTTON, LigmaSelectButtonClass))


typedef struct _LigmaSelectButtonPrivate LigmaSelectButtonPrivate;
typedef struct _LigmaSelectButtonClass   LigmaSelectButtonClass;

struct _LigmaSelectButton
{
  GtkBox                   parent_instance;

  LigmaSelectButtonPrivate *priv;

  /* FIXME MOVE TO PRIVATE */
  const gchar *temp_callback;
};

struct _LigmaSelectButtonClass
{
  GtkBoxClass  parent_class;

  gchar       *default_title;

  void (* select_destroy) (const gchar *callback);

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


GType       ligma_select_button_get_type    (void) G_GNUC_CONST;

void        ligma_select_button_close_popup (LigmaSelectButton *button);


G_END_DECLS

#endif /* __LIGMA_SELECT_BUTTON_H__ */
