/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorbutton.h
 * Copyright (C) 1999-2001 Sven Neumann
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

/* This provides a button with a color preview. The preview
 * can handle transparency by showing the checkerboard.
 * On click, a color selector is opened, which is already
 * fully functional wired to the preview button.
 */

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_COLOR_BUTTON_H__
#define __LIGMA_COLOR_BUTTON_H__

#include <libligmawidgets/ligmabutton.h>

G_BEGIN_DECLS


#define LIGMA_TYPE_COLOR_BUTTON            (ligma_color_button_get_type ())
#define LIGMA_COLOR_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_BUTTON, LigmaColorButton))
#define LIGMA_COLOR_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_BUTTON, LigmaColorButtonClass))
#define LIGMA_IS_COLOR_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_BUTTON))
#define LIGMA_IS_COLOR_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_BUTTON))
#define LIGMA_COLOR_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_BUTTON, LigmaColorButtonClass))


typedef struct _LigmaColorButtonPrivate LigmaColorButtonPrivate;
typedef struct _LigmaColorButtonClass   LigmaColorButtonClass;

struct _LigmaColorButton
{
  LigmaButton              parent_instance;

  LigmaColorButtonPrivate *priv;
};

struct _LigmaColorButtonClass
{
  LigmaButtonClass  parent_class;

  /*  signals  */
  void  (* color_changed)   (LigmaColorButton *button);

  /*  virtual functions  */
  GType (* get_action_type) (LigmaColorButton *button);

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


GType          ligma_color_button_get_type         (void) G_GNUC_CONST;

GtkWidget    * ligma_color_button_new              (const gchar       *title,
                                                   gint               width,
                                                   gint               height,
                                                   const LigmaRGB     *color,
                                                   LigmaColorAreaType  type);

void           ligma_color_button_set_title        (LigmaColorButton   *button,
                                                   const gchar       *title);
const gchar  * ligma_color_button_get_title        (LigmaColorButton   *button);

void           ligma_color_button_set_color        (LigmaColorButton   *button,
                                                   const LigmaRGB     *color);
void           ligma_color_button_get_color        (LigmaColorButton   *button,
                                                   LigmaRGB           *color);

gboolean       ligma_color_button_has_alpha        (LigmaColorButton   *button);
void           ligma_color_button_set_type         (LigmaColorButton   *button,
                                                   LigmaColorAreaType  type);

gboolean       ligma_color_button_get_update       (LigmaColorButton   *button);
void           ligma_color_button_set_update       (LigmaColorButton   *button,
                                                   gboolean           continuous);

void           ligma_color_button_set_color_config (LigmaColorButton   *button,
                                                   LigmaColorConfig   *config);

GtkUIManager * ligma_color_button_get_ui_manager   (LigmaColorButton   *button);


G_END_DECLS

#endif /* __LIGMA_COLOR_BUTTON_H__ */
