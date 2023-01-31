/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoggleaction.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2008 Sven Neumann <sven@gimp.org>
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
  PROP_ENABLED = GIMP_ACTION_PROP_LAST + 1,
  PROP_PARAMETER_TYPE,
  PROP_STATE_TYPE,
  PROP_STATE
};

struct _GimpToggleActionPrivate
{
  GVariantType *parameter_type;
  GVariant     *state;
  GVariant     *state_hint;
  gboolean      state_set_already;
};

static void   gimp_toggle_action_g_action_iface_init (GActionInterface *iface);

static void   gimp_toggle_action_get_property        (GObject          *object,
                                                      guint             prop_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);
static void   gimp_toggle_action_set_property        (GObject          *object,
                                                      guint             prop_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);

static gboolean gimp_toggle_action_get_enabled       (GAction          *action);
static const GVariantType *
              gimp_toggle_action_get_parameter_type  (GAction          *action);
static const GVariantType *
              gimp_toggle_action_get_state_type      (GAction          *action);
static GVariant *
              gimp_toggle_action_get_state           (GAction          *action);
static GVariant *
              gimp_toggle_action_get_state_hint      (GAction          *action);

static void   gimp_toggle_action_set_state           (GimpAction       *gimp_action,
                                                      GVariant         *value);

static void   gimp_toggle_action_connect_proxy       (GtkAction        *action,
                                                      GtkWidget        *proxy);

static void   gimp_toggle_action_toggled             (GtkToggleAction  *action);

static void   gimp_toggle_action_g_activate          (GAction          *action,
                                                      GVariant         *parameter);


G_DEFINE_TYPE_WITH_CODE (GimpToggleAction, gimp_toggle_action,
                         GTK_TYPE_TOGGLE_ACTION,
                         G_ADD_PRIVATE (GimpToggleAction)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION, gimp_toggle_action_g_action_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_ACTION, NULL))

#define parent_class gimp_toggle_action_parent_class


static void
gimp_toggle_action_class_init (GimpToggleActionClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkActionClass       *action_class = GTK_ACTION_CLASS (klass);
  GtkToggleActionClass *toggle_class = GTK_TOGGLE_ACTION_CLASS (klass);

  object_class->get_property  = gimp_toggle_action_get_property;
  object_class->set_property  = gimp_toggle_action_set_property;

  action_class->connect_proxy = gimp_toggle_action_connect_proxy;

  toggle_class->toggled       = gimp_toggle_action_toggled;

  gimp_action_install_properties (object_class);

  /**
   * GimpToggleAction:enabled:
   *
   * If @action is currently enabled.
   *
   * If the action is disabled then calls to g_action_activate() and
   * g_action_change_state() have no effect.
   **/
  g_object_class_install_property (object_class, PROP_ENABLED,
                                   g_param_spec_boolean ("enabled",
                                                         "Enabled",
                                                         "If the action can be activated",
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE));
  /**
   * GimpToggleAction:parameter-type:
   *
   * The type of the parameter that must be given when activating the
   * action.
   **/
  g_object_class_install_property (object_class, PROP_PARAMETER_TYPE,
                                   g_param_spec_boxed ("parameter-type",
                                                       "Parameter Type",
                                                       "The type of GVariant passed to activate()",
                                                       G_TYPE_VARIANT_TYPE,
                                                       GIMP_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));
  /**
   * GimpToggleAction:state-type:
   *
   * The #GVariantType of the state that the action has, or %NULL if the
   * action is stateless.
   **/
  g_object_class_install_property (object_class, PROP_STATE_TYPE,
                                   g_param_spec_boxed ("state-type",
                                                       "State Type",
                                                       "The type of the state kept by the action",
                                                       G_TYPE_VARIANT_TYPE,
                                                       GIMP_PARAM_READABLE));

  /**
   * GimpToggleAction:state:
   *
   * The state of the action, or %NULL if the action is stateless.
   **/
  g_object_class_install_property (object_class, PROP_STATE,
                                   g_param_spec_variant ("state",
                                                         "State",
                                                         "The state the action is in",
                                                         G_VARIANT_TYPE_ANY,
                                                         NULL,
                                                         GIMP_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void
gimp_toggle_action_g_action_iface_init (GActionInterface *iface)
{
  iface->get_name           = (const gchar* (*) (GAction*)) gimp_action_get_name;
  iface->activate           = gimp_toggle_action_g_activate;

  iface->get_enabled        = gimp_toggle_action_get_enabled;
  iface->get_parameter_type = gimp_toggle_action_get_parameter_type;
  iface->get_state_type     = gimp_toggle_action_get_state_type;
  iface->get_state          = gimp_toggle_action_get_state;
  iface->get_state_hint     = gimp_toggle_action_get_state_hint;
}

static void
gimp_toggle_action_init (GimpToggleAction *action)
{
  action->priv                    = gimp_toggle_action_get_instance_private (action);
  action->priv->state_set_already = FALSE;

  gimp_action_init (GIMP_ACTION (action));
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
    case PROP_ENABLED:
      g_value_set_boolean (value, gimp_toggle_action_get_enabled (G_ACTION (action)));
      break;
    case PROP_PARAMETER_TYPE:
      g_value_set_boxed (value, gimp_toggle_action_get_parameter_type (G_ACTION (action)));
      break;
    case PROP_STATE_TYPE:
      g_value_set_boxed (value, gimp_toggle_action_get_state_type (G_ACTION (action)));
      break;
    case PROP_STATE:
      g_value_take_variant (value, gimp_toggle_action_get_state (G_ACTION (action)));
      break;

    default:
      gimp_action_get_property (object, prop_id, value, pspec);
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
    case PROP_ENABLED:
      gimp_action_set_sensitive (GIMP_ACTION (action),
                                 g_value_get_boolean (value), NULL);
      break;
    case PROP_PARAMETER_TYPE:
      action->priv->parameter_type = g_value_dup_boxed (value);
      break;
    case PROP_STATE:
      /* The first time we see this (during construct) we should just
       * take the state as it was handed to us.
       *
       * After that, we should make sure we go through the same checks
       * as the C API.
       */
      if (!action->priv->state_set_already)
        {
          action->priv->state             = g_value_dup_variant (value);
          action->priv->state_set_already = TRUE;
        }
      else
        {
          gimp_toggle_action_set_state (GIMP_ACTION (action), g_value_get_variant (value));
        }

      break;

    default:
      gimp_action_set_property (object, prop_id, value, pspec);
      break;
    }
}

static gboolean
gimp_toggle_action_get_enabled (GAction *action)
{
  return gimp_action_is_sensitive (GIMP_ACTION (action), NULL);
}

static const GVariantType *
gimp_toggle_action_get_parameter_type (GAction *action)
{
  GimpToggleAction *taction = GIMP_TOGGLE_ACTION (action);

  return taction->priv->parameter_type;
}

static const GVariantType *
gimp_toggle_action_get_state_type (GAction *action)
{
  GimpToggleAction *taction = GIMP_TOGGLE_ACTION (action);

  if (taction->priv->state != NULL)
    return g_variant_get_type (taction->priv->state);
  else
    return NULL;
}

static GVariant *
gimp_toggle_action_get_state (GAction *action)
{
  GimpToggleAction *taction = GIMP_TOGGLE_ACTION (action);

  return taction->priv->state ? g_variant_ref (taction->priv->state) : NULL;
}

static GVariant *
gimp_toggle_action_get_state_hint (GAction *action)
{
  GimpToggleAction *taction = GIMP_TOGGLE_ACTION (action);

  if (taction->priv->state_hint != NULL)
    return g_variant_ref (taction->priv->state_hint);
  else
    return NULL;
}

static void
gimp_toggle_action_set_state (GimpAction *gimp_action,
                              GVariant   *value)
{
  GimpToggleAction   *action;
  const GVariantType *state_type;

  g_return_if_fail (GIMP_IS_ACTION (gimp_action));
  g_return_if_fail (value != NULL);

  action = GIMP_TOGGLE_ACTION (gimp_action);

  state_type = action->priv->state ? g_variant_get_type (action->priv->state) : NULL;

  g_return_if_fail (state_type != NULL);
  g_return_if_fail (g_variant_is_of_type (value, state_type));

  g_variant_ref_sink (value);

  if (! action->priv->state || ! g_variant_equal (action->priv->state, value))
    {
      if (action->priv->state)
        g_variant_unref (action->priv->state);

      action->priv->state = g_variant_ref (value);

      g_object_notify (G_OBJECT (action), "state");
    }

  g_variant_unref (value);
}

static void
gimp_toggle_action_connect_proxy (GtkAction *action,
                                  GtkWidget *proxy)
{
  GTK_ACTION_CLASS (parent_class)->connect_proxy (action, proxy);

  gimp_action_set_proxy_tooltip (GIMP_ACTION (action), proxy);
}

static void
gimp_toggle_action_toggled (GtkToggleAction *action)
{
  gboolean value = gimp_toggle_action_get_active (GIMP_TOGGLE_ACTION (action));

  gimp_action_emit_change_state (GIMP_ACTION (action),
                                 g_variant_new_boolean (value));
}

static void
gimp_toggle_action_g_activate (GAction  *action,
                               GVariant *parameter)
{
  GimpToggleAction *taction = GIMP_TOGGLE_ACTION (action);
  gboolean          value   = gimp_toggle_action_get_active (taction);

  gimp_action_emit_change_state (GIMP_ACTION (action),
                                 g_variant_new_boolean (! value));

  gimp_action_history_action_activated (GIMP_ACTION (action));
}


/*  public functions  */

GtkToggleAction *
gimp_toggle_action_new (const gchar *name,
                        const gchar *label,
                        const gchar *tooltip,
                        const gchar *icon_name,
                        const gchar *help_id)
{
  GtkToggleAction *action;

  action = g_object_new (GIMP_TYPE_TOGGLE_ACTION,
                         "name",      name,
                         "label",     label,
                         "tooltip",   tooltip,
                         "icon-name", icon_name,
                         NULL);

  gimp_action_set_help_id (GIMP_ACTION (action), help_id);

  return action;
}

void
gimp_toggle_action_set_active (GimpToggleAction *action,
                               gboolean          active)
{
  return gtk_toggle_action_set_active ((GtkToggleAction *) action, active);
}

gboolean
gimp_toggle_action_get_active (GimpToggleAction *action)
{
  return gtk_toggle_action_get_active ((GtkToggleAction *) action);
}
