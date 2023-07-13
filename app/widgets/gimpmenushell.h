/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenushell.h
 * Copyright (C) 2023 Jehan
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

#ifndef __GIMP_MENU_SHELL_H__
#define __GIMP_MENU_SHELL_H__


#define GIMP_TYPE_MENU_SHELL               (gimp_menu_shell_get_type ())
#define GIMP_MENU_SHELL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MENU_SHELL, GimpMenuShell))
#define GIMP_IS_MENU_SHELL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MENU_SHELL))
#define GIMP_MENU_SHELL_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), GIMP_TYPE_MENU_SHELL, GimpMenuShellInterface))


enum
{
  GIMP_MENU_SHELL_PROP_0,
  GIMP_MENU_SHELL_PROP_MANAGER,
  GIMP_MENU_SHELL_PROP_MODEL,

  GIMP_MENU_SHELL_PROP_LAST = GIMP_MENU_SHELL_PROP_MODEL,
};

typedef struct _GimpMenuShellInterface GimpMenuShellInterface;

struct _GimpMenuShellInterface
{
  GTypeInterface base_interface;

  /* Signals */

  void          (* model_deleted) (GimpMenuShell  *shell);

  /* Virtual functions. */

  void          (* append)        (GimpMenuShell  *shell,
                                   GimpMenuModel  *model);
  void          (* add_ui)        (GimpMenuShell  *shell,
                                   const gchar   **paths,
                                   const gchar    *action_name,
                                   gboolean        top);
  void          (* remove_ui)     (GimpMenuShell  *shell,
                                   const gchar   **paths,
                                   const gchar    *action_name);
};


GType           gimp_menu_shell_get_type            (void) G_GNUC_CONST;

void            gimp_menu_shell_fill                (GimpMenuShell *shell,
                                                     GimpMenuModel *model,
                                                     gboolean       drop_top_submenu);


/* Protected functions. */

void            gimp_menu_shell_append              (GimpMenuShell *shell,
                                                     GimpMenuModel *model);

void            gimp_menu_shell_init                (GimpMenuShell *shell);
void            gimp_menu_shell_install_properties  (GObjectClass  *klass);
void            gimp_menu_shell_get_property        (GObject       *object,
                                                     guint          property_id,
                                                     GValue        *value,
                                                     GParamSpec    *pspec);
void            gimp_menu_shell_set_property        (GObject       *object,
                                                     guint          property_id,
                                                     const GValue  *value,
                                                     GParamSpec    *pspec);

GimpUIManager * gimp_menu_shell_get_manager         (GimpMenuShell *shell);


#endif  /* __GIMP_MENU_SHELL_H__ */
