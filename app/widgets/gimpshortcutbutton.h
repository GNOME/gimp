/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmashortcutbutton.h
 * Copyright (C) 2022 Jehan
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

#ifndef __LIGMA_SHORTCUT_BUTTON_H__
#define __LIGMA_SHORTCUT_BUTTON_H__


#define LIGMA_TYPE_SHORTCUT_BUTTON            (ligma_shortcut_button_get_type ())
#define LIGMA_SHORTCUT_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SHORTCUT_BUTTON, LigmaShortcutButton))
#define LIGMA_SHORTCUT_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SHORTCUT_BUTTON, LigmaShortcutButtonClass))
#define LIGMA_IS_SHORTCUT_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SHORTCUT_BUTTON))
#define LIGMA_IS_SHORTCUT_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SHORTCUT_BUTTON))
#define LIGMA_SHORTCUT_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SHORTCUT_BUTTON, LigmaShortcutButtonClass))


typedef struct _LigmaShortcutButtonPrivate LigmaShortcutButtonPrivate;
typedef struct _LigmaShortcutButtonClass   LigmaShortcutButtonClass;

struct _LigmaShortcutButton
{
  GtkToggleButton            parent_instance;

  LigmaShortcutButtonPrivate *priv;
};

struct _LigmaShortcutButtonClass
{
  GtkToggleButtonClass       parent_class;

  void                    (* accelerator_changed)    (LigmaShortcutButton *button,
                                                      const gchar        *previous_accelerator);
};


GType          ligma_shortcut_button_get_type         (void) G_GNUC_CONST;

GtkWidget    * ligma_shortcut_button_new              (const gchar        *accelerator);

gchar        * ligma_shortcut_button_get_accelerator  (LigmaShortcutButton  *button);
void           ligma_shortcut_button_get_keys         (LigmaShortcutButton  *button,
                                                      guint               *keyval,
                                                      GdkModifierType     *modifiers);
void           ligma_shortcut_button_set_accelerator  (LigmaShortcutButton  *button,
                                                      const gchar         *accelerator,
                                                      guint                keyval,
                                                      GdkModifierType      modifiers);

void           ligma_shortcut_button_accepts_modifier (LigmaShortcutButton *button,
                                                      gboolean            only,
                                                      gboolean            unique);


#endif /* __LIGMA_SHORTCUT_BUTTON_H__ */
