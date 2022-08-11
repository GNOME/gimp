/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpshortcutbutton.h
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

#ifndef __GIMP_SHORTCUT_BUTTON_H__
#define __GIMP_SHORTCUT_BUTTON_H__


#define GIMP_TYPE_SHORTCUT_BUTTON            (gimp_shortcut_button_get_type ())
#define GIMP_SHORTCUT_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SHORTCUT_BUTTON, GimpShortcutButton))
#define GIMP_SHORTCUT_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SHORTCUT_BUTTON, GimpShortcutButtonClass))
#define GIMP_IS_SHORTCUT_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SHORTCUT_BUTTON))
#define GIMP_IS_SHORTCUT_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SHORTCUT_BUTTON))
#define GIMP_SHORTCUT_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SHORTCUT_BUTTON, GimpShortcutButtonClass))


typedef struct _GimpShortcutButtonPrivate GimpShortcutButtonPrivate;
typedef struct _GimpShortcutButtonClass   GimpShortcutButtonClass;

struct _GimpShortcutButton
{
  GtkToggleButton            parent_instance;

  GimpShortcutButtonPrivate *priv;
};

struct _GimpShortcutButtonClass
{
  GtkToggleButtonClass       parent_class;

  void                    (* accelerator_changed)    (GimpShortcutButton *button,
                                                      const gchar        *previous_accelerator);
};


GType          gimp_shortcut_button_get_type         (void) G_GNUC_CONST;

GtkWidget    * gimp_shortcut_button_new              (const gchar        *accelerator);

gchar        * gimp_shortcut_button_get_accelerator  (GimpShortcutButton  *button);
void           gimp_shortcut_button_get_keys         (GimpShortcutButton  *button,
                                                      guint               *keyval,
                                                      GdkModifierType     *modifiers);
void           gimp_shortcut_button_set_accelerator  (GimpShortcutButton  *button,
                                                      const gchar         *accelerator,
                                                      guint                keyval,
                                                      GdkModifierType      modifiers);

void           gimp_shortcut_button_accepts_modifier (GimpShortcutButton *button,
                                                      gboolean            only,
                                                      gboolean            unique);


#endif /* __GIMP_SHORTCUT_BUTTON_H__ */
