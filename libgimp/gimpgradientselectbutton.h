/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmagradientselectbutton.h
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

#ifndef __LIGMA_GRADIENT_SELECT_BUTTON_H__
#define __LIGMA_GRADIENT_SELECT_BUTTON_H__

#include <libligma/ligmaselectbutton.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_GRADIENT_SELECT_BUTTON            (ligma_gradient_select_button_get_type ())
#define LIGMA_GRADIENT_SELECT_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_GRADIENT_SELECT_BUTTON, LigmaGradientSelectButton))
#define LIGMA_GRADIENT_SELECT_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_GRADIENT_SELECT_BUTTON, LigmaGradientSelectButtonClass))
#define LIGMA_IS_GRADIENT_SELECT_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_GRADIENT_SELECT_BUTTON))
#define LIGMA_IS_GRADIENT_SELECT_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_GRADIENT_SELECT_BUTTON))
#define LIGMA_GRADIENT_SELECT_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_GRADIENT_SELECT_BUTTON, LigmaGradientSelectButtonClass))


typedef struct _LigmaGradientSelectButtonPrivate LigmaGradientSelectButtonPrivate;
typedef struct _LigmaGradientSelectButtonClass   LigmaGradientSelectButtonClass;

struct _LigmaGradientSelectButton
{
  LigmaSelectButton                 parent_instance;

  LigmaGradientSelectButtonPrivate *priv;
};

struct _LigmaGradientSelectButtonClass
{
  LigmaSelectButtonClass  parent_class;

  /* gradient_set signal is emitted when gradient is chosen */
  void (* gradient_set) (LigmaGradientSelectButton *button,
                         const gchar              *gradient_name,
                         gint                      width,
                         const gdouble            *gradient_data,
                         gboolean                  dialog_closing);

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


GType         ligma_gradient_select_button_get_type     (void) G_GNUC_CONST;

GtkWidget   * ligma_gradient_select_button_new          (const gchar *title,
                                                        const gchar *gradient_name);

const gchar * ligma_gradient_select_button_get_gradient (LigmaGradientSelectButton *button);
void          ligma_gradient_select_button_set_gradient (LigmaGradientSelectButton *button,
                                                        const gchar              *gradient_name);


G_END_DECLS

#endif /* __LIGMA_GRADIENT_SELECT_BUTTON_H__ */
