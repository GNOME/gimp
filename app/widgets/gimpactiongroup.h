/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaactiongroup.h
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

#ifndef __LIGMA_ACTION_GROUP_H__
#define __LIGMA_ACTION_GROUP_H__


#define LIGMA_TYPE_ACTION_GROUP              (ligma_action_group_get_type ())
#define LIGMA_ACTION_GROUP(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ACTION_GROUP, LigmaActionGroup))
#define LIGMA_ACTION_GROUP_CLASS(vtable)     (G_TYPE_CHECK_CLASS_CAST ((vtable), LIGMA_TYPE_ACTION_GROUP, LigmaActionGroupClass))
#define LIGMA_IS_ACTION_GROUP(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ACTION_GROUP))
#define LIGMA_IS_ACTION_GROUP_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), LIGMA_TYPE_ACTION_GROUP))
#define LIGMA_ACTION_GROUP_GET_CLASS(inst)   (G_TYPE_INSTANCE_GET_CLASS ((inst), LIGMA_TYPE_ACTION_GROUP, LigmaActionGroupClass))


typedef struct _LigmaActionGroupClass LigmaActionGroupClass;

struct _LigmaActionGroup
{
  GtkActionGroup             parent_instance;

  Ligma                      *ligma;
  gchar                     *label;
  gchar                     *icon_name;

  gpointer                   user_data;

  LigmaActionGroupUpdateFunc  update_func;
};

struct _LigmaActionGroupClass
{
  GtkActionGroupClass  parent_class;

  GHashTable          *groups;

  /* signals */
  void (* action_added) (LigmaActionGroup *group,
                         LigmaAction      *action);
};


typedef void (* LigmaActionCallback) (LigmaAction *action,
                                     GVariant   *value,
                                     gpointer    data);

struct _LigmaActionEntry
{
  const gchar        *name;
  const gchar        *icon_name;
  const gchar        *label;
  const gchar        *accelerator;
  const gchar        *tooltip;
  LigmaActionCallback  callback;

  const gchar        *help_id;
};

struct _LigmaToggleActionEntry
{
  const gchar        *name;
  const gchar        *icon_name;
  const gchar        *label;
  const gchar        *accelerator;
  const gchar        *tooltip;
  LigmaActionCallback  callback;
  gboolean            is_active;

  const gchar *help_id;
};

struct _LigmaRadioActionEntry
{
  const gchar *name;
  const gchar *icon_name;
  const gchar *label;
  const gchar *accelerator;
  const gchar *tooltip;
  gint         value;

  const gchar *help_id;
};

struct _LigmaEnumActionEntry
{
  const gchar *name;
  const gchar *icon_name;
  const gchar *label;
  const gchar *accelerator;
  const gchar *tooltip;
  gint         value;
  gboolean     value_variable;

  const gchar *help_id;
};

struct _LigmaStringActionEntry
{
  const gchar *name;
  const gchar *icon_name;
  const gchar *label;
  const gchar *accelerator;
  const gchar *tooltip;
  const gchar *value;

  const gchar *help_id;
};

struct _LigmaDoubleActionEntry
{
  const gchar   *name;
  const gchar   *icon_name;
  const gchar   *label;
  const gchar   *accelerator;
  const gchar   *tooltip;
  const gdouble  value;

  const gchar   *help_id;
};

struct _LigmaProcedureActionEntry
{
  const gchar   *name;
  const gchar   *icon_name;
  const gchar   *label;
  const gchar   *accelerator;
  const gchar   *tooltip;
  LigmaProcedure *procedure;

  const gchar   *help_id;
};


GType            ligma_action_group_get_type   (void) G_GNUC_CONST;

LigmaActionGroup *ligma_action_group_new        (Ligma                  *ligma,
                                               const gchar           *name,
                                               const gchar           *label,
                                               const gchar           *icon_name,
                                               gpointer               user_data,
                                               LigmaActionGroupUpdateFunc update_func);

GList *ligma_action_groups_from_name           (const gchar           *name);

const gchar * ligma_action_group_get_name       (LigmaActionGroup       *group);

void  ligma_action_group_add_action            (LigmaActionGroup *action_group,
                                               LigmaAction      *action);
void  ligma_action_group_add_action_with_accel (LigmaActionGroup *action_group,
                                               LigmaAction      *action,
                                               const gchar     *accelerator);
void  ligma_action_group_remove_action         (LigmaActionGroup *action_group,
                                               LigmaAction      *action);

LigmaAction  * ligma_action_group_get_action     (LigmaActionGroup       *group,
                                                const gchar           *action_name);
GList       * ligma_action_group_list_actions   (LigmaActionGroup       *group);

void   ligma_action_group_update               (LigmaActionGroup       *group,
                                               gpointer               update_data);

void   ligma_action_group_add_actions          (LigmaActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const LigmaActionEntry       *entries,
                                               guint                        n_entries);
void   ligma_action_group_add_toggle_actions   (LigmaActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const LigmaToggleActionEntry *entries,
                                               guint                        n_entries);
GSList *ligma_action_group_add_radio_actions   (LigmaActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const LigmaRadioActionEntry  *entries,
                                               guint                        n_entries,
                                               GSList                      *radio_group,
                                               gint                         value,
                                               LigmaActionCallback           callback);
void   ligma_action_group_add_enum_actions     (LigmaActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const LigmaEnumActionEntry   *entries,
                                               guint                        n_entries,
                                               LigmaActionCallback           callback);
void   ligma_action_group_add_string_actions   (LigmaActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const LigmaStringActionEntry *entries,
                                               guint                        n_entries,
                                               LigmaActionCallback           callback);
void   ligma_action_group_add_double_actions   (LigmaActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const LigmaDoubleActionEntry *entries,
                                               guint                        n_entries,
                                               LigmaActionCallback           callback);
void   ligma_action_group_add_procedure_actions(LigmaActionGroup             *group,
                                               const LigmaProcedureActionEntry *entries,
                                               guint                        n_entries,
                                               LigmaActionCallback           callback);

void   ligma_action_group_remove_action_and_accel (LigmaActionGroup          *group,
                                                  LigmaAction               *action);

void          ligma_action_group_activate_action       (LigmaActionGroup *group,
                                                       const gchar     *action_name);
void          ligma_action_group_set_action_visible    (LigmaActionGroup *group,
                                                       const gchar     *action_name,
                                                       gboolean         visible);
void          ligma_action_group_set_action_sensitive  (LigmaActionGroup *group,
                                                       const gchar     *action_name,
                                                       gboolean         sensitive,
                                                       const gchar     *reason);
void          ligma_action_group_set_action_active     (LigmaActionGroup *group,
                                                       const gchar     *action_name,
                                                       gboolean         active);
void          ligma_action_group_set_action_label      (LigmaActionGroup *group,
                                                       const gchar     *action_name,
                                                       const gchar     *label);
void          ligma_action_group_set_action_pixbuf     (LigmaActionGroup *group,
                                                       const gchar     *action_name,
                                                       GdkPixbuf       *pixbuf);
void          ligma_action_group_set_action_tooltip    (LigmaActionGroup *group,
                                                       const gchar     *action_name,
                                                       const gchar     *tooltip);
const gchar * ligma_action_group_get_action_tooltip    (LigmaActionGroup *group,
                                                       const gchar     *action_name);
void          ligma_action_group_set_action_context    (LigmaActionGroup *group,
                                                       const gchar     *action_name,
                                                       LigmaContext     *context);
void          ligma_action_group_set_action_color      (LigmaActionGroup *group,
                                                       const gchar     *action_name,
                                                       const LigmaRGB   *color,
                                                       gboolean         set_label);
void          ligma_action_group_set_action_viewable   (LigmaActionGroup *group,
                                                       const gchar     *action_name,
                                                       LigmaViewable    *viewable);
void          ligma_action_group_set_action_hide_empty (LigmaActionGroup *group,
                                                       const gchar     *action_name,
                                                       gboolean         hide_empty);
void   ligma_action_group_set_action_always_show_image (LigmaActionGroup *group,
                                                       const gchar     *action_name,
                                                       gboolean         always_show_image);



#endif  /* __LIGMA_ACTION_GROUP_H__ */
