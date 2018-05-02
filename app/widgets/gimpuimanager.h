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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_UI_MANAGER_H__
#define __GIMP_UI_MANAGER_H__


typedef struct _GimpUIManagerUIEntry GimpUIManagerUIEntry;

struct _GimpUIManagerUIEntry
{
  gchar                  *ui_path;
  gchar                  *basename;
  GimpUIManagerSetupFunc  setup_func;
  guint                   merge_id;
  GtkWidget              *widget;
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
  GtkUIManager  parent_instance;

  gchar        *name;
  Gimp         *gimp;
  GList        *registered_uis;
};

struct _GimpUIManagerClass
{
  GtkUIManagerClass  parent_class;

  GHashTable        *managers;

  void (* update)       (GimpUIManager *manager,
                         gpointer       update_data);
  void (* show_tooltip) (GimpUIManager *manager,
                         const gchar   *tooltip);
  void (* hide_tooltip) (GimpUIManager *manager);
};


GType           gimp_ui_manager_get_type    (void) G_GNUC_CONST;

GimpUIManager * gimp_ui_manager_new         (Gimp                   *gimp,
                                             const gchar            *name);

GList         * gimp_ui_managers_from_name  (const gchar            *name);

void            gimp_ui_manager_update      (GimpUIManager          *manager,
                                             gpointer                update_data);
GimpActionGroup * gimp_ui_manager_get_action_group (GimpUIManager   *manager,
                                                    const gchar     *name);

GtkAction     * gimp_ui_manager_find_action     (GimpUIManager      *manager,
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

void            gimp_ui_manager_ui_popup    (GimpUIManager          *manager,
                                             const gchar            *ui_path,
                                             GtkWidget              *parent,
                                             GimpMenuPositionFunc    position_func,
                                             gpointer                position_data,
                                             GDestroyNotify          popdown_func,
                                             gpointer                popdown_data);
void            gimp_ui_manager_ui_popup_at_widget
                                            (GimpUIManager          *manager,
                                             const gchar            *ui_path,
                                             GtkWidget              *widget,
                                             GdkGravity              widget_anchor,
                                             GdkGravity              menu_anchor,
                                             const GdkEvent         *trigger_event,
                                             GDestroyNotify          popdown_func,
                                             gpointer                popdown_data);
void            gimp_ui_manager_ui_popup_at_pointer
                                            (GimpUIManager          *manager,
                                             const gchar            *ui_path,
                                             const GdkEvent         *trigger_event,
                                             GDestroyNotify          popdown_func,
                                             gpointer                popdown_data);


#endif  /* __GIMP_UI_MANAGER_H__ */
