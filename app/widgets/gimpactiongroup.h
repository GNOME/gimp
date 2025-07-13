/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpactiongroup.h
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

#pragma once

/*  leave this here  */
#ifndef __GIMP_ACTION_GROUP_H__
#define __GIMP_ACTION_GROUP_H__
#endif

#include "core/gimpobject.h"

#define GIMP_TYPE_ACTION_GROUP              (gimp_action_group_get_type ())
#define GIMP_ACTION_GROUP(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ACTION_GROUP, GimpActionGroup))
#define GIMP_ACTION_GROUP_CLASS(vtable)     (G_TYPE_CHECK_CLASS_CAST ((vtable), GIMP_TYPE_ACTION_GROUP, GimpActionGroupClass))
#define GIMP_IS_ACTION_GROUP(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ACTION_GROUP))
#define GIMP_IS_ACTION_GROUP_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), GIMP_TYPE_ACTION_GROUP))
#define GIMP_ACTION_GROUP_GET_CLASS(inst)   (G_TYPE_INSTANCE_GET_CLASS ((inst), GIMP_TYPE_ACTION_GROUP, GimpActionGroupClass))


typedef struct _GimpActionGroupClass GimpActionGroupClass;

struct _GimpActionGroup
{
  GimpObject                 parent_instance;

  Gimp                      *gimp;
  gchar                     *label;
  gchar                     *icon_name;

  gpointer                   user_data;

  GimpActionGroupUpdateFunc  update_func;

  GList                     *actions;
};

struct _GimpActionGroupClass
{
  GimpObjectClass      parent_class;

  GHashTable          *groups;
};


typedef void (* GimpActionCallback) (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data);

struct _GimpActionEntry
{
  const gchar        *name;
  const gchar        *icon_name;
  const gchar        *label;
  const gchar        *short_label;
  const gchar        *accelerator[4];
  const gchar        *tooltip;
  GimpActionCallback  callback;

  const gchar        *help_id;
};

struct _GimpToggleActionEntry
{
  const gchar        *name;
  const gchar        *icon_name;
  const gchar        *label;
  const gchar        *short_label;
  const gchar        *accelerator[4];
  const gchar        *tooltip;
  GimpActionCallback  callback;
  gboolean            is_active;

  const gchar        *help_id;
};

struct _GimpRadioActionEntry
{
  const gchar *name;
  const gchar *icon_name;
  const gchar *label;
  const gchar *short_label;
  const gchar *accelerator[4];
  const gchar *tooltip;
  gint         value;

  const gchar *help_id;
};

struct _GimpEnumActionEntry
{
  const gchar *name;
  const gchar *icon_name;
  const gchar *label;
  const gchar *short_label;
  const gchar *accelerator[4];
  const gchar *tooltip;
  gint         value;
  gboolean     value_variable;

  const gchar *help_id;
};

struct _GimpStringActionEntry
{
  const gchar *name;
  const gchar *icon_name;
  const gchar *label;
  const gchar *short_label;
  const gchar *accelerator[4];
  const gchar *tooltip;
  const gchar *value;

  const gchar *help_id;
};

struct _GimpDoubleActionEntry
{
  const gchar   *name;
  const gchar   *icon_name;
  const gchar   *label;
  const gchar   *short_label;
  const gchar   *accelerator[4];
  const gchar   *tooltip;
  const gdouble  value;

  const gchar   *help_id;
};

struct _GimpProcedureActionEntry
{
  const gchar   *name;
  const gchar   *icon_name;
  const gchar   *label;
  const gchar   *accelerator;
  const gchar   *tooltip;
  GimpProcedure *procedure;

  const gchar   *help_id;
};


GType            gimp_action_group_get_type   (void) G_GNUC_CONST;

GimpActionGroup *gimp_action_group_new        (Gimp                  *gimp,
                                               const gchar           *name,
                                               const gchar           *label,
                                               const gchar           *icon_name,
                                               gpointer               user_data,
                                               GimpActionGroupUpdateFunc update_func);

GList *gimp_action_groups_from_name           (const gchar           *name);

const gchar * gimp_action_group_get_name       (GimpActionGroup       *group);

void  gimp_action_group_add_action            (GimpActionGroup *action_group,
                                               GimpAction      *action);
void  gimp_action_group_add_action_with_accel (GimpActionGroup *action_group,
                                               GimpAction      *action,
                                               const gchar * const*accelerators);
void  gimp_action_group_remove_action         (GimpActionGroup *action_group,
                                               GimpAction      *action);

GimpAction  * gimp_action_group_get_action     (GimpActionGroup       *group,
                                                const gchar           *action_name);
GList       * gimp_action_group_list_actions   (GimpActionGroup       *group);

void   gimp_action_group_update               (GimpActionGroup       *group,
                                               gpointer               update_data);

void   gimp_action_group_add_actions          (GimpActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const GimpActionEntry       *entries,
                                               guint                        n_entries);
void   gimp_action_group_add_toggle_actions   (GimpActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const GimpToggleActionEntry *entries,
                                               guint                        n_entries);
GSList *gimp_action_group_add_radio_actions   (GimpActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const GimpRadioActionEntry  *entries,
                                               guint                        n_entries,
                                               GSList                      *radio_group,
                                               gint                         value,
                                               GimpActionCallback           callback);
void   gimp_action_group_add_enum_actions     (GimpActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const GimpEnumActionEntry   *entries,
                                               guint                        n_entries,
                                               GimpActionCallback           callback);
void   gimp_action_group_add_string_actions   (GimpActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const GimpStringActionEntry *entries,
                                               guint                        n_entries,
                                               GimpActionCallback           callback);
void   gimp_action_group_add_double_actions   (GimpActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const GimpDoubleActionEntry *entries,
                                               guint                        n_entries,
                                               GimpActionCallback           callback);
void   gimp_action_group_add_procedure_actions(GimpActionGroup             *group,
                                               const GimpProcedureActionEntry *entries,
                                               guint                        n_entries,
                                               GimpActionCallback           callback);

void   gimp_action_group_remove_action_and_accel (GimpActionGroup          *group,
                                                  GimpAction               *action);

void          gimp_action_group_set_action_visible    (GimpActionGroup *group,
                                                       const gchar     *action_name,
                                                       gboolean         visible);
void          gimp_action_group_set_action_sensitive  (GimpActionGroup *group,
                                                       const gchar     *action_name,
                                                       gboolean         sensitive,
                                                       const gchar     *reason);
void          gimp_action_group_set_action_active     (GimpActionGroup *group,
                                                       const gchar     *action_name,
                                                       gboolean         active);
void          gimp_action_group_set_action_label      (GimpActionGroup *group,
                                                       const gchar     *action_name,
                                                       const gchar     *label);
void          gimp_action_group_set_action_pixbuf     (GimpActionGroup *group,
                                                       const gchar     *action_name,
                                                       GdkPixbuf       *pixbuf);
void          gimp_action_group_set_action_tooltip    (GimpActionGroup *group,
                                                       const gchar     *action_name,
                                                       const gchar     *tooltip);
const gchar * gimp_action_group_get_action_tooltip    (GimpActionGroup *group,
                                                       const gchar     *action_name);
void          gimp_action_group_set_action_color      (GimpActionGroup *group,
                                                       const gchar     *action_name,
                                                       GeglColor       *color,
                                                       gboolean         set_label);
void          gimp_action_group_set_action_viewable   (GimpActionGroup *group,
                                                       const gchar     *action_name,
                                                       GimpViewable    *viewable);
void          gimp_action_group_set_action_hide_empty (GimpActionGroup *group,
                                                       const gchar     *action_name,
                                                       gboolean         hide_empty);
void   gimp_action_group_set_action_always_show_image (GimpActionGroup *group,
                                                       const gchar     *action_name,
                                                       gboolean         always_show_image);
