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

#pragma once

G_BEGIN_DECLS

#include "core/gimpobject.h"

#define GIMP_TYPE_ACTION (gimp_action_get_type ())
G_DECLARE_INTERFACE (GimpAction, gimp_action, GIMP, ACTION, GimpObject)

enum
{
  GIMP_ACTION_PROP_0,
  GIMP_ACTION_PROP_CONTEXT,
  GIMP_ACTION_PROP_SENSITIVE,
  GIMP_ACTION_PROP_VISIBLE,
  GIMP_ACTION_PROP_LABEL,
  GIMP_ACTION_PROP_SHORT_LABEL,
  GIMP_ACTION_PROP_TOOLTIP,
  GIMP_ACTION_PROP_ICON_NAME,
  GIMP_ACTION_PROP_ICON,

  GIMP_ACTION_PROP_COLOR,
  GIMP_ACTION_PROP_VIEWABLE,
  GIMP_ACTION_PROP_ELLIPSIZE,
  GIMP_ACTION_PROP_MAX_WIDTH_CHARS,

  GIMP_ACTION_PROP_LAST = GIMP_ACTION_PROP_MAX_WIDTH_CHARS,
};

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


void             gimp_action_init                (GimpAction       *action);

void             gimp_action_emit_activate       (GimpAction       *action,
                                                  GVariant         *value);
void             gimp_action_emit_change_state   (GimpAction       *action,
                                                  GVariant         *value);

const gchar     * gimp_action_get_name            (GimpAction      *action);
GimpActionGroup * gimp_action_get_group           (GimpAction      *action);

void              gimp_action_set_label           (GimpAction      *action,
                                                   const gchar     *label);
const gchar     * gimp_action_get_label           (GimpAction      *action);

void              gimp_action_set_short_label     (GimpAction      *action,
                                                   const gchar     *label);
const gchar     * gimp_action_get_short_label     (GimpAction      *action);

void              gimp_action_set_tooltip         (GimpAction      *action,
                                                   const gchar     *tooltip);
const gchar     * gimp_action_get_tooltip         (GimpAction      *action);

void              gimp_action_set_icon_name       (GimpAction      *action,
                                                   const gchar     *icon_name);
const gchar     * gimp_action_get_icon_name       (GimpAction      *action);

void              gimp_action_set_gicon           (GimpAction      *action,
                                                   GIcon           *icon);
GIcon           * gimp_action_get_gicon           (GimpAction      *action);

void              gimp_action_set_help_id         (GimpAction      *action,
                                                   const gchar     *help_id);
const gchar     * gimp_action_get_help_id         (GimpAction      *action);

void              gimp_action_set_visible         (GimpAction      *action,
                                                   gboolean         visible);
gboolean          gimp_action_get_visible         (GimpAction      *action);
gboolean          gimp_action_is_visible          (GimpAction      *action);

void              gimp_action_set_sensitive       (GimpAction      *action,
                                                   gboolean         sensitive,
                                                   const gchar     *reason);
gboolean          gimp_action_get_sensitive       (GimpAction      *action,
                                                   const gchar    **reason);
gboolean          gimp_action_is_sensitive        (GimpAction      *action,
                                                   const gchar    **reason);

void              gimp_action_set_accels          (GimpAction      *action,
                                                   const gchar    **accels);
const gchar    ** gimp_action_get_default_accels  (GimpAction      *action);
const gchar    ** gimp_action_get_accels          (GimpAction      *action);
gchar **          gimp_action_get_display_accels  (GimpAction      *action);
gboolean          gimp_action_use_default_accels  (GimpAction      *action);
gboolean          gimp_action_is_default_accel    (GimpAction      *action,
                                                   const gchar     *accel);

const gchar     * gimp_action_get_menu_path       (GimpAction      *action);

void              gimp_action_activate            (GimpAction      *action);

gint              gimp_action_name_compare        (GimpAction      *action1,
                                                   GimpAction      *action2);

gboolean          gimp_action_is_gui_blacklisted  (const gchar     *action_name);
gboolean          gimp_action_is_action_search_blacklisted
                                                  (const gchar     *action_name);

GimpContext     * gimp_action_get_context         (GimpAction      *action);
GimpViewable    * gimp_action_get_viewable        (GimpAction      *action);


/* Protected functions. */

void              gimp_action_install_properties  (GObjectClass    *klass);
void              gimp_action_get_property        (GObject         *object,
                                                   guint            property_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);
void              gimp_action_set_property        (GObject         *object,
                                                   guint            property_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);

void              gimp_action_set_proxy           (GimpAction      *action,
                                                   GtkWidget       *proxy);


/* Friend functions. */

void              gimp_action_set_group           (GimpAction      *action,
                                                   GimpActionGroup *group);
void              gimp_action_set_default_accels  (GimpAction      *action,
                                                   const gchar    **accels);
void              gimp_action_set_menu_path       (GimpAction      *action,
                                                   const gchar     *menu_path);

G_END_DECLS
