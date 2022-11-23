/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaaction.h
 * Copyright (C) 2004-2019 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_ACTION_H__
#define __LIGMA_ACTION_H__


#define LIGMA_TYPE_ACTION               (ligma_action_get_type ())
#define LIGMA_ACTION(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ACTION, LigmaAction))
#define LIGMA_IS_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ACTION))
#define LIGMA_ACTION_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), LIGMA_TYPE_ACTION, LigmaActionInterface))


typedef struct _LigmaActionInterface LigmaActionInterface;

struct _LigmaActionInterface
{
  GTypeInterface base_interface;

  /* Signals */
  void          (* activate)           (LigmaAction  *action,
                                        GVariant    *value);
  void          (* change_state)       (LigmaAction  *action,
                                        GVariant    *value);

  /* Virtual methods */
  void          (* set_disable_reason) (LigmaAction  *action,
                                        const gchar *reason);
  const gchar * (* get_disable_reason) (LigmaAction  *action);
};


GType         ligma_action_get_type            (void) G_GNUC_CONST;

void          ligma_action_init                (LigmaAction    *action);

void          ligma_action_emit_activate       (LigmaAction    *action,
                                               GVariant      *value);
void          ligma_action_emit_change_state   (LigmaAction    *action,
                                               GVariant      *value);

void          ligma_action_set_proxy           (LigmaAction    *action,
                                               GtkWidget     *proxy);

const gchar * ligma_action_get_name            (LigmaAction    *action);

void          ligma_action_set_label           (LigmaAction    *action,
                                               const gchar   *label);
const gchar * ligma_action_get_label           (LigmaAction    *action);

void          ligma_action_set_tooltip         (LigmaAction    *action,
                                               const gchar   *tooltip);
const gchar * ligma_action_get_tooltip         (LigmaAction    *action);

void          ligma_action_set_icon_name       (LigmaAction    *action,
                                               const gchar   *icon_name);
const gchar * ligma_action_get_icon_name       (LigmaAction    *action);

void          ligma_action_set_gicon           (LigmaAction    *action,
                                               GIcon         *icon);
GIcon       * ligma_action_get_gicon           (LigmaAction    *action);

void          ligma_action_set_help_id         (LigmaAction    *action,
                                               const gchar   *help_id);
const gchar * ligma_action_get_help_id         (LigmaAction    *action);

void          ligma_action_set_visible         (LigmaAction    *action,
                                               gboolean       visible);
gboolean      ligma_action_get_visible         (LigmaAction    *action);
gboolean      ligma_action_is_visible          (LigmaAction    *action);

void          ligma_action_set_sensitive       (LigmaAction    *action,
                                               gboolean       sensitive,
                                               const gchar   *reason);
gboolean      ligma_action_get_sensitive       (LigmaAction    *action,
                                               const gchar  **reason);
gboolean      ligma_action_is_sensitive        (LigmaAction    *action,
                                               const gchar  **reason);

GClosure    * ligma_action_get_accel_closure   (LigmaAction    *action);

void          ligma_action_set_accel_path      (LigmaAction    *action,
                                               const gchar   *accel_path);
const gchar * ligma_action_get_accel_path      (LigmaAction    *action);

void          ligma_action_set_accel_group     (LigmaAction    *action,
                                               GtkAccelGroup *accel_group);
void          ligma_action_connect_accelerator (LigmaAction    *action);

GSList      * ligma_action_get_proxies         (LigmaAction    *action);

void          ligma_action_activate            (LigmaAction    *action);

gint          ligma_action_name_compare        (LigmaAction    *action1,
                                               LigmaAction    *action2);

gboolean      ligma_action_is_gui_blacklisted  (const gchar   *action_name);


#endif  /* __LIGMA_ACTION_H__ */
