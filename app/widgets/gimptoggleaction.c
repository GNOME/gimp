/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoggleaction.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2008 Sven Neumann <sven@gimp.org>
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpaction.h"
#include "gimpaction-history.h"
#include "gimptoggleaction.h"


enum
{
  PROP_0,
  PROP_ACTIVE,
};

enum
{
  TOGGLED,
  LAST_SIGNAL
};

struct _GimpToggleActionPrivate
{
  gboolean active;
};

static void     gimp_toggle_action_g_action_iface_init (GActionInterface *iface);

static void     gimp_toggle_action_get_property        (GObject          *object,
                                                        guint             prop_id,
                                                        GValue           *value,
                                                        GParamSpec       *pspec);
static void     gimp_toggle_action_set_property        (GObject          *object,
                                                        guint             prop_id,
                                                        const GValue     *value,
                                                        GParamSpec       *pspec);

static gboolean gimp_toggle_action_get_enabled         (GAction          *action);
static const GVariantType *
                gimp_toggle_action_get_state_type      (GAction          *action);
static GVariant *
                gimp_toggle_action_get_state           (GAction          *action);

static void     gimp_toggle_action_activate            (GAction          *action,
                                                        GVariant         *parameter);

static gboolean gimp_toggle_action_real_toggle         (GimpToggleAction *action);

static void     gimp_toggle_action_toggle              (GimpToggleAction *action);


G_DEFINE_TYPE_WITH_CODE (GimpToggleAction, gimp_toggle_action,
                         GIMP_TYPE_ACTION_IMPL,
                         G_ADD_PRIVATE (GimpToggleAction)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION, gimp_toggle_action_g_action_iface_init))

#define parent_class gimp_toggle_action_parent_class

static guint gimp_toggle_action_signals[LAST_SIGNAL] = { 0 };

static void
gimp_toggle_action_class_init (GimpToggleActionClass *klass)
{
  GObjectClass          *object_class = G_OBJECT_CLASS (klass);
  GimpToggleActionClass *toggle_class = GIMP_TOGGLE_ACTION_CLASS (klass);

  gimp_toggle_action_signals[TOGGLED] =
    g_signal_new ("toggled",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToggleActionClass, toggled),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->get_property  = gimp_toggle_action_get_property;
  object_class->set_property  = gimp_toggle_action_set_property;

  toggle_class->toggle        = gimp_toggle_action_real_toggle;

  /**
   * GimpToggleAction:active:
   *
   * If @action state is currently active.
   *
   * Calls to g_action_activate() will flip this property value.
   **/
  g_object_class_install_property (object_class, PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         "Active state",
                                                         "If the action state is active",
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE));
}

static void
gimp_toggle_action_g_action_iface_init (GActionInterface *iface)
{
  iface->get_name       = (const gchar* (*) (GAction*)) gimp_action_get_name;
  iface->activate       = gimp_toggle_action_activate;

  iface->get_enabled    = gimp_toggle_action_get_enabled;
  iface->get_state_type = gimp_toggle_action_get_state_type;
  iface->get_state      = gimp_toggle_action_get_state;
}

static void
gimp_toggle_action_init (GimpToggleAction *action)
{
  action->priv = gimp_toggle_action_get_instance_private (action);
}

static void
gimp_toggle_action_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpToggleAction *action = GIMP_TOGGLE_ACTION (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, gimp_toggle_action_get_active (action));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_toggle_action_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpToggleAction *action = GIMP_TOGGLE_ACTION (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      gimp_toggle_action_set_active (action, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
gimp_toggle_action_get_enabled (GAction *action)
{
  return gimp_action_is_sensitive (GIMP_ACTION (action), NULL);
}

static const GVariantType *
gimp_toggle_action_get_state_type (GAction *action)
{
  return G_VARIANT_TYPE_BOOLEAN;
}

static GVariant *
gimp_toggle_action_get_state (GAction *action)
{
  GVariant *state;

  state = g_variant_new_boolean (GIMP_TOGGLE_ACTION (action)->priv->active);

  return g_variant_ref_sink (state);
}

static void
gimp_toggle_action_activate (GAction  *action,
                             GVariant *parameter)
{
  gimp_toggle_action_toggle (GIMP_TOGGLE_ACTION (action));
}

static gboolean
gimp_toggle_action_real_toggle (GimpToggleAction *action)
{
  gboolean value = gimp_toggle_action_get_active (action);

  action->priv->active = ! value;
  gimp_action_emit_change_state (GIMP_ACTION (action),
                                 g_variant_new_boolean (! value));

  /* Toggling always works for the base class. */
  return TRUE;
}

static void
gimp_toggle_action_toggle (GimpToggleAction *action)
{
  if (GIMP_TOGGLE_ACTION_GET_CLASS (action)->toggle != NULL &&
      GIMP_TOGGLE_ACTION_GET_CLASS (action)->toggle (action))
    {
      g_signal_emit (action, gimp_toggle_action_signals[TOGGLED], 0);
      g_object_notify (G_OBJECT (action), "state");
      g_object_notify (G_OBJECT (action), "active");

      gimp_action_history_action_activated (GIMP_ACTION (action));
    }
}


/*  public functions  */

GimpAction *
gimp_toggle_action_new (const gchar *name,
                        const gchar *label,
                        const gchar *short_label,
                        const gchar *tooltip,
                        const gchar *icon_name,
                        const gchar *help_id,
                        GimpContext *context)
{
  GimpAction *action;

  action = g_object_new (GIMP_TYPE_TOGGLE_ACTION,
                         "name",        name,
                         "label",       label,
                         "short-label", short_label,
                         "tooltip",     tooltip,
                         "icon-name",   icon_name,
                         "context",     context,
                         NULL);

  gimp_action_set_help_id (GIMP_ACTION (action), help_id);

  return action;
}

void
gimp_toggle_action_set_active (GimpToggleAction *action,
                               gboolean          active)
{
  if (action->priv->active != active)
    gimp_toggle_action_toggle (action);
}

gboolean
gimp_toggle_action_get_active (GimpToggleAction *action)
{
  return action->priv->active;
}


/* Protected functions. */

void
_gimp_toggle_action_set_active (GimpToggleAction *action,
                                gboolean          active)
{
  action->priv->active = active;
}
