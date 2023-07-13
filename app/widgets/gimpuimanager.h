/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpuimanager.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_UI_MANAGER_H__
#define __GIMP_UI_MANAGER_H__


#include "core/gimpobject.h"


typedef void (* GimpUIMenuCallback)  (GimpUIManager *manager,
                                      const gchar   *path,
                                      const gchar   *action_name,
                                      gboolean       top,
                                      gpointer       user_data);


typedef struct _GimpUIManagerUIEntry GimpUIManagerUIEntry;

struct _GimpUIManagerUIEntry
{
  gchar                  *ui_path;
  gchar                  *basename;
  GimpUIManagerSetupFunc  setup_func;
  gboolean                setup_done;
  GtkBuilder             *builder;
};


#define GIMP_TYPE_UI_MANAGER              (gimp_ui_manager_get_type ())
#define GIMP_UI_MANAGER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_UI_MANAGER, GimpUIManager))
#define GIMP_UI_MANAGER_CLASS(vtable)     (G_TYPE_CHECK_CLASS_CAST ((vtable), GIMP_TYPE_UI_MANAGER, GimpUIManagerClass))
#define GIMP_IS_UI_MANAGER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_UI_MANAGER))
#define GIMP_IS_UI_MANAGER_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), GIMP_TYPE_UI_MANAGER))
#define GIMP_UI_MANAGER_GET_CLASS(inst)   (G_TYPE_INSTANCE_GET_CLASS ((inst), GIMP_TYPE_UI_MANAGER, GimpUIManagerClass))


typedef struct _GimpUIManagerClass GimpUIManagerClass;

/**
 * Among other things, is responsible for updating menu bars. A more
 * appropriate name would perhaps be GimpMenubarManager.
 */
struct _GimpUIManager
{
  GimpObject    parent_instance;

  gchar        *name;
  Gimp         *gimp;
  GList        *registered_uis;

  GList        *ui_items;
  GList        *action_groups;

  /* Special property which should be set to TRUE only for the singleton UI
   * manager of the top menu bar.
   */
  gboolean      store_action_paths;
};

struct _GimpUIManagerClass
{
  GimpObjectClass    parent_class;

  GHashTable        *managers;

  void (* update)       (GimpUIManager *manager,
                         gpointer       update_data);
  void (* show_tooltip) (GimpUIManager *manager,
                         const gchar   *tooltip);
  void (* hide_tooltip) (GimpUIManager *manager);
  void (* ui_added)     (GimpUIManager *manager,
                         const gchar   *path,
                         const gchar   *action_name,
                         gboolean       top);
  void (* ui_removed)   (GimpUIManager *manager,
                         const gchar   *path,
                         const gchar   *action_name);
};


GType           gimp_ui_manager_get_type    (void) G_GNUC_CONST;

GimpUIManager * gimp_ui_manager_new         (Gimp                   *gimp,
                                             const gchar            *name);

GList         * gimp_ui_managers_from_name  (const gchar            *name);

void            gimp_ui_manager_update      (GimpUIManager          *manager,
                                             gpointer                update_data);

void            gimp_ui_manager_add_action_group   (GimpUIManager   *manager,
                                                    GimpActionGroup *group);
GimpActionGroup * gimp_ui_manager_get_action_group (GimpUIManager   *manager,
                                                    const gchar     *name);
GList         * gimp_ui_manager_get_action_groups  (GimpUIManager   *manager);

GimpMenuModel * gimp_ui_manager_get_model       (GimpUIManager      *manager,
                                                 const gchar        *path);

void            gimp_ui_manager_remove_ui       (GimpUIManager      *manager,
                                                 const gchar        *action_name);
void            gimp_ui_manager_remove_uis      (GimpUIManager      *manager,
                                                 const gchar        *action_name_prefix);
void            gimp_ui_manager_add_ui          (GimpUIManager      *manager,
                                                 const gchar        *path,
                                                 const gchar        *action_name,
                                                 gboolean            top);
void            gimp_ui_manager_foreach_ui      (GimpUIManager      *manager,
                                                 GimpUIMenuCallback  callback,
                                                 gpointer            user_data);

GimpAction    * gimp_ui_manager_find_action     (GimpUIManager      *manager,
                                                 const gchar        *group_name,
                                                 const gchar        *action_name);
gboolean        gimp_ui_manager_activate_action (GimpUIManager      *manager,
                                                 const gchar        *group_name,
                                                 const gchar        *action_name);
gboolean        gimp_ui_manager_toggle_action   (GimpUIManager      *manager,
                                                 const gchar        *group_name,
                                                 const gchar        *action_name,
                                                 gboolean            active);

void            gimp_ui_manager_ui_register (GimpUIManager          *manager,
                                             const gchar            *ui_path,
                                             const gchar            *basename,
                                             GimpUIManagerSetupFunc  setup_func);

void            gimp_ui_manager_ui_popup_at_widget
                                            (GimpUIManager          *manager,
                                             const gchar            *ui_path,
                                             GimpUIManager          *child_ui_manager,
                                             const gchar            *child_ui_path,
                                             GtkWidget              *widget,
                                             GdkGravity              widget_anchor,
                                             GdkGravity              menu_anchor,
                                             const GdkEvent         *trigger_event,
                                             GDestroyNotify          popdown_func,
                                             gpointer                popdown_data);
void            gimp_ui_manager_ui_popup_at_pointer
                                            (GimpUIManager          *manager,
                                             const gchar            *ui_path,
                                             GtkWidget              *attached_widget,
                                             const GdkEvent         *trigger_event,
                                             GDestroyNotify          popdown_func,
                                             gpointer                popdown_data);
void            gimp_ui_manager_ui_popup_at_rect
                                            (GimpUIManager           *manager,
                                             const gchar             *ui_path,
                                             GtkWidget               *attached_widget,
                                             GdkWindow               *window,
                                             const GdkRectangle      *rect,
                                             GdkGravity               rect_anchor,
                                             GdkGravity               menu_anchor,
                                             const GdkEvent          *trigger_event,
                                             GDestroyNotify           popdown_func,
                                             gpointer                 popdown_data);


#endif  /* __GIMP_UI_MANAGER_H__ */
