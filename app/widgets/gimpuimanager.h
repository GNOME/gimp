/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmauimanager.h
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_UI_MANAGER_H__
#define __LIGMA_UI_MANAGER_H__


typedef struct _LigmaUIManagerUIEntry LigmaUIManagerUIEntry;

struct _LigmaUIManagerUIEntry
{
  gchar                  *ui_path;
  gchar                  *basename;
  LigmaUIManagerSetupFunc  setup_func;
  guint                   merge_id;
  GtkWidget              *widget;
};


#define LIGMA_TYPE_UI_MANAGER              (ligma_ui_manager_get_type ())
#define LIGMA_UI_MANAGER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_UI_MANAGER, LigmaUIManager))
#define LIGMA_UI_MANAGER_CLASS(vtable)     (G_TYPE_CHECK_CLASS_CAST ((vtable), LIGMA_TYPE_UI_MANAGER, LigmaUIManagerClass))
#define LIGMA_IS_UI_MANAGER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_UI_MANAGER))
#define LIGMA_IS_UI_MANAGER_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), LIGMA_TYPE_UI_MANAGER))
#define LIGMA_UI_MANAGER_GET_CLASS(inst)   (G_TYPE_INSTANCE_GET_CLASS ((inst), LIGMA_TYPE_UI_MANAGER, LigmaUIManagerClass))


typedef struct _LigmaUIManagerClass LigmaUIManagerClass;

/**
 * Among other things, is responsible for updating menu bars. A more
 * appropriate name would perhaps be LigmaMenubarManager.
 */
struct _LigmaUIManager
{
  GtkUIManager  parent_instance;

  gchar        *name;
  Ligma         *ligma;
  GList        *registered_uis;
};

struct _LigmaUIManagerClass
{
  GtkUIManagerClass  parent_class;

  GHashTable        *managers;

  void (* update)       (LigmaUIManager *manager,
                         gpointer       update_data);
  void (* show_tooltip) (LigmaUIManager *manager,
                         const gchar   *tooltip);
  void (* hide_tooltip) (LigmaUIManager *manager);
};


GType           ligma_ui_manager_get_type    (void) G_GNUC_CONST;

LigmaUIManager * ligma_ui_manager_new         (Ligma                   *ligma,
                                             const gchar            *name);

GList         * ligma_ui_managers_from_name  (const gchar            *name);

void            ligma_ui_manager_update      (LigmaUIManager          *manager,
                                             gpointer                update_data);

void            ligma_ui_manager_insert_action_group (LigmaUIManager   *manager,
                                                     LigmaActionGroup *group,
                                                     gint             pos);
LigmaActionGroup * ligma_ui_manager_get_action_group  (LigmaUIManager   *manager,
                                                     const gchar     *name);
GList         * ligma_ui_manager_get_action_groups   (LigmaUIManager   *manager);

GtkAccelGroup * ligma_ui_manager_get_accel_group (LigmaUIManager      *manager);

GtkWidget     * ligma_ui_manager_get_widget      (LigmaUIManager      *manager,
                                                 const gchar        *path);

gchar          * ligma_ui_manager_get_ui         (LigmaUIManager      *manager);

guint            ligma_ui_manager_new_merge_id   (LigmaUIManager      *manager);
void             ligma_ui_manager_add_ui         (LigmaUIManager      *manager,
                                                 guint               merge_id,
                                                 const gchar        *path,
                                                 const gchar        *name,
                                                 const gchar        *action,
                                                 GtkUIManagerItemType type,
                                                 gboolean            top);
void            ligma_ui_manager_remove_ui       (LigmaUIManager      *manager,
                                                 guint               merge_id);

void            ligma_ui_manager_ensure_update   (LigmaUIManager      *manager);

LigmaAction    * ligma_ui_manager_get_action      (LigmaUIManager      *manager,
                                                 const gchar        *path);
LigmaAction    * ligma_ui_manager_find_action     (LigmaUIManager      *manager,
                                                 const gchar        *group_name,
                                                 const gchar        *action_name);
gboolean        ligma_ui_manager_activate_action (LigmaUIManager      *manager,
                                                 const gchar        *group_name,
                                                 const gchar        *action_name);
gboolean        ligma_ui_manager_toggle_action   (LigmaUIManager      *manager,
                                                 const gchar        *group_name,
                                                 const gchar        *action_name,
                                                 gboolean            active);

void            ligma_ui_manager_ui_register (LigmaUIManager          *manager,
                                             const gchar            *ui_path,
                                             const gchar            *basename,
                                             LigmaUIManagerSetupFunc  setup_func);

void            ligma_ui_manager_ui_popup_at_widget
                                            (LigmaUIManager          *manager,
                                             const gchar            *ui_path,
                                             GtkWidget              *widget,
                                             GdkGravity              widget_anchor,
                                             GdkGravity              menu_anchor,
                                             const GdkEvent         *trigger_event,
                                             GDestroyNotify          popdown_func,
                                             gpointer                popdown_data);
void            ligma_ui_manager_ui_popup_at_pointer
                                            (LigmaUIManager          *manager,
                                             const gchar            *ui_path,
                                             const GdkEvent         *trigger_event,
                                             GDestroyNotify          popdown_func,
                                             gpointer                popdown_data);
void            ligma_ui_manager_ui_popup_at_rect
                                            (LigmaUIManager           *manager,
                                             const gchar             *ui_path,
                                             GdkWindow               *window,
                                             const GdkRectangle      *rect,
                                             GdkGravity               rect_anchor,
                                             GdkGravity               menu_anchor,
                                             const GdkEvent          *trigger_event,
                                             GDestroyNotify           popdown_func,
                                             gpointer                 popdown_data);


#endif  /* __LIGMA_UI_MANAGER_H__ */
