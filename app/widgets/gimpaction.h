/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpaction.h
 * Copyright (C) 2004-2019 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_ACTION_H__
#define __GIMP_ACTION_H__


#define GIMP_TYPE_ACTION               (gimp_action_get_type ())
#define GIMP_ACTION(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ACTION, GimpAction))
#define GIMP_IS_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ACTION))
#define GIMP_ACTION_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE((obj), GIMP_TYPE_ACTION, GimpActionInterface))


enum
{
  GIMP_ACTION_PROP_0,
  GIMP_ACTION_PROP_CONTEXT,
  GIMP_ACTION_PROP_SENSITIVE,

  GIMP_ACTION_PROP_COLOR,
  GIMP_ACTION_PROP_VIEWABLE,
  GIMP_ACTION_PROP_ELLIPSIZE,
  GIMP_ACTION_PROP_MAX_WIDTH_CHARS,

  GIMP_ACTION_PROP_LAST = GIMP_ACTION_PROP_MAX_WIDTH_CHARS,
};

typedef struct _GimpActionInterface GimpActionInterface;

struct _GimpActionInterface
{
  GTypeInterface base_interface;

  /* Signals */
  void          (* activate)           (GimpAction   *action,
                                        GVariant     *value);
  void          (* change_state)       (GimpAction   *action,
                                        GVariant     *value);
  void          (* accels_changed)     (GimpAction   *action,
                                        const gchar **accels);
};


GType         gimp_action_get_type            (void) G_GNUC_CONST;

void          gimp_action_init                (GimpAction    *action);

void          gimp_action_emit_activate       (GimpAction    *action,
                                               GVariant      *value);
void          gimp_action_emit_change_state   (GimpAction    *action,
                                               GVariant      *value);

const gchar * gimp_action_get_name            (GimpAction    *action);

void          gimp_action_set_label           (GimpAction    *action,
                                               const gchar   *label);
const gchar * gimp_action_get_label           (GimpAction    *action);

void          gimp_action_set_tooltip         (GimpAction    *action,
                                               const gchar   *tooltip);
const gchar * gimp_action_get_tooltip         (GimpAction    *action);

void          gimp_action_set_icon_name       (GimpAction    *action,
                                               const gchar   *icon_name);
const gchar * gimp_action_get_icon_name       (GimpAction    *action);

void          gimp_action_set_gicon           (GimpAction    *action,
                                               GIcon         *icon);
GIcon       * gimp_action_get_gicon           (GimpAction    *action);

void          gimp_action_set_help_id         (GimpAction    *action,
                                               const gchar   *help_id);
const gchar * gimp_action_get_help_id         (GimpAction    *action);

void          gimp_action_set_visible         (GimpAction    *action,
                                               gboolean       visible);
gboolean      gimp_action_get_visible         (GimpAction    *action);
gboolean      gimp_action_is_visible          (GimpAction    *action);

void          gimp_action_set_sensitive       (GimpAction    *action,
                                               gboolean       sensitive,
                                               const gchar   *reason);
gboolean      gimp_action_get_sensitive       (GimpAction    *action,
                                               const gchar  **reason);
gboolean      gimp_action_is_sensitive        (GimpAction    *action,
                                               const gchar  **reason);

void          gimp_action_set_accel_group     (GimpAction    *action,
                                               GtkAccelGroup *accel_group);
void          gimp_action_connect_accelerator (GimpAction    *action);

void          gimp_action_set_accels          (GimpAction    *action,
                                               const gchar  **accels);
gchar **      gimp_action_get_accels          (GimpAction    *action);
gchar **      gimp_action_get_display_accels  (GimpAction    *action);

void          gimp_action_activate            (GimpAction    *action);

gint          gimp_action_name_compare        (GimpAction    *action1,
                                               GimpAction    *action2);

gboolean      gimp_action_is_gui_blacklisted  (const gchar   *action_name);

GimpContext  * gimp_action_get_context        (GimpAction    *action);
GimpViewable * gimp_action_get_viewable       (GimpAction    *action);


/* Protected functions. */

void          gimp_action_install_properties  (GObjectClass  *klass);
void          gimp_action_get_property        (GObject       *object,
                                               guint          property_id,
                                               GValue        *value,
                                               GParamSpec    *pspec);
void          gimp_action_set_property        (GObject       *object,
                                               guint          property_id,
                                               const GValue  *value,
                                               GParamSpec    *pspec);

void          gimp_action_set_proxy           (GimpAction    *action,
                                               GtkWidget     *proxy);


#endif  /* __GIMP_ACTION_H__ */
