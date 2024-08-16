/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpradioaction.c
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
#include "gimpradioaction.h"

/**
 * GimpRadioAction:
 *
 * An action which is part of a group of radio actions. It is guaranteed that at
 * most one action is active in the group.
 *
 * The "state" in any of the actions of the group is the @value associated to
 * the active action of the group.
 */

enum
{
  PROP_0,
  PROP_VALUE,
  PROP_GROUP,
  PROP_GROUP_LABEL,
  PROP_CURRENT_VALUE
};

struct _GimpRadioActionPrivate
{
  GSList *group;
  gchar  *group_label;

  gint    value;
};

static void      gimp_radio_action_g_action_iface_init (GActionInterface *iface);

static void      gimp_radio_action_dispose             (GObject          *object);
static void      gimp_radio_action_finalize            (GObject          *object);
static void      gimp_radio_action_get_property        (GObject          *object,
                                                        guint             prop_id,
                                                        GValue           *value,
                                                        GParamSpec       *pspec);
static void      gimp_radio_action_set_property        (GObject          *object,
                                                        guint             prop_id,
                                                        const GValue     *value,
                                                        GParamSpec       *pspec);

static gboolean  gimp_radio_action_get_enabled         (GAction          *action);
static void      gimp_radio_action_change_state        (GAction          *action,
                                                        GVariant         *value);
static const GVariantType *
                 gimp_radio_action_get_state_type      (GAction          *action);
static GVariant *
                 gimp_radio_action_get_state           (GAction          *action);
static const GVariantType *
                 gimp_radio_action_get_parameter_type  (GAction          *action);

static void      gimp_radio_action_activate            (GAction          *action,
                                                        GVariant         *parameter);

static gboolean  gimp_radio_action_toggle              (GimpToggleAction *action);


G_DEFINE_TYPE_WITH_CODE (GimpRadioAction, gimp_radio_action, GIMP_TYPE_TOGGLE_ACTION,
                         G_ADD_PRIVATE (GimpRadioAction)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION, gimp_radio_action_g_action_iface_init))

#define parent_class gimp_radio_action_parent_class


static void
gimp_radio_action_class_init (GimpRadioActionClass *klass)
{
  GObjectClass          *object_class = G_OBJECT_CLASS (klass);
  GimpToggleActionClass *toggle_class = GIMP_TOGGLE_ACTION_CLASS (klass);

  object_class->dispose      = gimp_radio_action_dispose;
  object_class->finalize     = gimp_radio_action_finalize;
  object_class->get_property = gimp_radio_action_get_property;
  object_class->set_property = gimp_radio_action_set_property;

  toggle_class->toggle       = gimp_radio_action_toggle;

  /**
   * GimpRadioAction:value:
   *
   * The value is an arbitrary integer which can be used as a convenient way to
   * determine which action in the group is currently active in an ::activate
   * signal handler.
   * See gimp_radio_action_get_current_value() for convenient ways to get and
   * set this property.
   **/
  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_int ("value",
                                                     "Value",
                                                     "The value returned by gimp_radio_action_get_current_value() when this action is the current action of its group.",
                                                     G_MININT,
                                                     G_MAXINT,
                                                     0,
                                                     GIMP_PARAM_READWRITE));

  /**
   * GimpRadioAction:group:
   *
   * Sets a new group for a radio action.
   */
  g_object_class_install_property (object_class, PROP_GROUP,
                                   g_param_spec_object ("group",
                                                        "Group",
                                                        "The radio action whose group this action belongs to.",
                                                        GIMP_TYPE_RADIO_ACTION,
                                                        GIMP_PARAM_WRITABLE));

  /**
   * GimpRadioAction:group-label:
   *
   * Sets the group label. It will be common to all actions in a same group.
   */
  g_object_class_install_property (object_class, PROP_GROUP_LABEL,
                                   g_param_spec_string ("group-label",
                                                        "Group string",
                                                        "The label for all the radio action in the group this action belongs to.",
                                                        NULL,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_EXPLICIT_NOTIFY));
  /**
   * GimpRadioAction:value:
   *
   * The value property of the currently active member of the group to which
   * this action belongs.
   **/
  g_object_class_install_property (object_class, PROP_CURRENT_VALUE,
                                   g_param_spec_int ("current-value",
                                                     "Current value",
                                                     "The value property of the currently active member of the group to which this action belongs.",
                                                     G_MININT,
                                                     G_MAXINT,
                                                     0,
                                                     GIMP_PARAM_READWRITE));
}

static void
gimp_radio_action_g_action_iface_init (GActionInterface *iface)
{
  iface->get_name           = (const gchar* (*) (GAction*)) gimp_action_get_name;
  iface->activate           = gimp_radio_action_activate;
  iface->get_enabled        = gimp_radio_action_get_enabled;

  iface->change_state       = gimp_radio_action_change_state;
  iface->get_state_type     = gimp_radio_action_get_state_type;
  iface->get_state          = gimp_radio_action_get_state;
  iface->get_parameter_type = gimp_radio_action_get_parameter_type;
}

static void
gimp_radio_action_init (GimpRadioAction *action)
{
  action->priv = gimp_radio_action_get_instance_private (action);

  action->priv->group       = NULL;
  action->priv->group_label = NULL;
}

static void
gimp_radio_action_dispose (GObject *object)
{
  GimpRadioAction *action = GIMP_RADIO_ACTION (object);

  if (action->priv->group)
    {
      GSList *group;
      GSList *slist;

      group = g_slist_remove (action->priv->group, action);

      for (slist = group; slist; slist = slist->next)
        {
          GimpRadioAction *tmp_action = slist->data;

          tmp_action->priv->group = group;
        }

      action->priv->group = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_radio_action_finalize (GObject *object)
{
  GimpRadioAction *action = GIMP_RADIO_ACTION (object);

  g_clear_pointer (&action->priv->group_label, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_radio_action_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpRadioAction *action = GIMP_RADIO_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      g_value_set_int (value, action->priv->value);
      break;
    case PROP_CURRENT_VALUE:
      g_value_set_int (value,
                       gimp_radio_action_get_current_value (action));
      break;
    case PROP_GROUP_LABEL:
      g_value_set_string (value,
                          gimp_radio_action_get_group_label (action));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_radio_action_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpRadioAction *action = GIMP_RADIO_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      action->priv->value = g_value_get_int (value);
      break;
    case PROP_GROUP:
      {
        GimpRadioAction *arg;
        GSList          *slist = NULL;

        if (G_VALUE_HOLDS_OBJECT (value))
          {
            arg = GIMP_RADIO_ACTION (g_value_get_object (value));
            if (arg)
              slist = gimp_radio_action_get_group (arg);
            gimp_radio_action_set_group (action, slist);
          }
      }
    break;
    case PROP_GROUP_LABEL:
      gimp_radio_action_set_group_label (action, g_value_get_string (value));
      break;
    case PROP_CURRENT_VALUE:
      gimp_radio_action_set_current_value (action,
                                           g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static gboolean
gimp_radio_action_get_enabled (GAction *action)
{
  GimpRadioAction *radio = GIMP_RADIO_ACTION (action);

  return gimp_radio_action_get_current_value (radio) != radio->priv->value &&
         gimp_action_is_sensitive (GIMP_ACTION (action), NULL);
}

static void
gimp_radio_action_change_state (GAction  *action,
                                GVariant *value)
{
  g_return_if_fail (GIMP_IS_ACTION (action));
  g_return_if_fail (value != NULL);
  g_return_if_fail (g_variant_is_of_type (value, G_VARIANT_TYPE_INT32));

  gimp_radio_action_set_current_value (GIMP_RADIO_ACTION (action),
                                       g_variant_get_int32 (value));
}

static const GVariantType *
gimp_radio_action_get_state_type (GAction *action)
{
  return G_VARIANT_TYPE_INT32;
}

static GVariant *
gimp_radio_action_get_state (GAction *action)
{
  GimpRadioAction *radio;
  GVariant        *state;

  g_return_val_if_fail (GIMP_IS_RADIO_ACTION (action), NULL);

  radio = GIMP_RADIO_ACTION (action);

  state = g_variant_new_int32 (gimp_radio_action_get_current_value (radio));

  return g_variant_ref_sink (state);
}

/* This is a bit on the ugly side. In order to be shown as a radio item when the
 * GUI is created by GTK (which happens either on macOS or with environment
 * variable GIMP_GTK_MENUBAR), the menu item requires a "target" attribute
 * (which is automatically added in GimpMenuModel) and the action needs to have
 * a parameter type of the right type.
 * In reality, how we use our radio actions is to have several tied actions
 * rather than a single action to which we pass a parameter (we ignore the
 * passed parameter anyway). This makes it a lot easier to have people assign
 * shortcuts to these.
 * So the ugly part is to add a parameter type here, even though the parameter
 * just won't be used.
 * See #9704.
 */
static const GVariantType *
gimp_radio_action_get_parameter_type (GAction *action)
{
  return G_VARIANT_TYPE_INT32;
}

static void
gimp_radio_action_activate (GAction  *action,
                            GVariant *parameter)
{
  GimpRadioAction *radio;

  g_return_if_fail (GIMP_IS_RADIO_ACTION (action));

  radio = GIMP_RADIO_ACTION (action);

  gimp_radio_action_set_current_value (radio, radio->priv->value);
  g_signal_emit_by_name (radio, "toggled");
}

static gboolean
gimp_radio_action_toggle (GimpToggleAction *action)
{
  gboolean active = gimp_toggle_action_get_active (action);

  if (! active)
    {
      GimpRadioAction *radio = GIMP_RADIO_ACTION (action);

      gimp_radio_action_set_current_value (radio, radio->priv->value);

      return gimp_toggle_action_get_active (action);
    }
  return FALSE;
}


/*  public functions  */

GimpAction *
gimp_radio_action_new (const gchar *name,
                       const gchar *label,
                       const gchar *short_label,
                       const gchar *tooltip,
                       const gchar *icon_name,
                       const gchar *help_id,
                       gint         value,
                       GimpContext *context)
{
  GimpAction *action;

  action = g_object_new (GIMP_TYPE_RADIO_ACTION,
                         "name",        name,
                         "label",       label,
                         "short-label", short_label,
                         "tooltip",     tooltip,
                         "icon-name",   icon_name,
                         "value",       value,
                         "context",     context,
                         NULL);

  gimp_action_set_help_id (action, help_id);

  return action;
}

GSList *
gimp_radio_action_get_group (GimpRadioAction *action)
{
  g_return_val_if_fail (GIMP_IS_RADIO_ACTION (action), NULL);

  return action->priv->group;
}

void
gimp_radio_action_set_group (GimpRadioAction *action,
                             GSList          *group)
{
  g_return_if_fail (GIMP_IS_RADIO_ACTION (action));
  g_return_if_fail (! g_slist_find (group, action));

  if (action->priv->group)
    {
      GSList *slist;

      action->priv->group = g_slist_remove (action->priv->group, action);

      for (slist = action->priv->group; slist; slist = slist->next)
        {
          GimpRadioAction *tmp_action = slist->data;

          tmp_action->priv->group = action->priv->group;
        }
    }

  action->priv->group = g_slist_prepend (group, action);
  g_clear_pointer (&action->priv->group_label, g_free);

  if (group)
    {
      GSList *slist;

      action->priv->group_label = g_strdup (GIMP_RADIO_ACTION (group->data)->priv->group_label);

      for (slist = action->priv->group; slist; slist = slist->next)
        {
          GimpRadioAction *tmp_action = slist->data;

          tmp_action->priv->group = action->priv->group;
        }
    }
  else
    {
      gimp_toggle_action_set_active (GIMP_TOGGLE_ACTION (action), TRUE);
    }

  g_object_notify (G_OBJECT (action), "group-label");
}

void
gimp_radio_action_set_group_label (GimpRadioAction *action,
                                   const gchar     *label)
{
  GSList *slist;

  g_return_if_fail (GIMP_IS_RADIO_ACTION (action));

  for (slist = action->priv->group; slist; slist = slist->next)
    {
      GimpRadioAction *tmp_action = slist->data;

      g_clear_pointer (&tmp_action->priv->group_label, g_free);

      if (label != NULL)
        tmp_action->priv->group_label = g_strdup (label);

      g_object_notify (G_OBJECT (tmp_action), "group-label");
    }
}

const gchar *
gimp_radio_action_get_group_label (GimpRadioAction *action)
{
  g_return_val_if_fail (GIMP_IS_RADIO_ACTION (action), NULL);

  return action->priv->group_label;
}

gint
gimp_radio_action_get_current_value (GimpRadioAction *action)
{
  GSList *slist;

  g_return_val_if_fail (GIMP_IS_RADIO_ACTION (action), 0);

  if (action->priv->group)
    {
      for (slist = action->priv->group; slist; slist = slist->next)
        {
          GimpToggleAction *toggle_action = slist->data;

          if (gimp_toggle_action_get_active (toggle_action))
            return GIMP_RADIO_ACTION (toggle_action)->priv->value;
        }
    }

  return action->priv->value;
}

void
gimp_radio_action_set_current_value (GimpRadioAction *action,
                                     gint             current_value)
{
  GSList           *slist;
  GimpToggleAction *changed1 = NULL;
  GimpToggleAction *changed2 = NULL;

  g_return_if_fail (GIMP_IS_RADIO_ACTION (action));

  if (action->priv->group)
    {
      for (slist = action->priv->group; slist; slist = slist->next)
        {
          GimpToggleAction *toggle = slist->data;
          GimpRadioAction  *radio  = slist->data;

          if (radio->priv->value == current_value &&
              ! gimp_toggle_action_get_active (toggle))
            {
              /* Change the "active" state but don't notify the property change
               * immediately. We want to notify both "active" properties
               * together so that we are always in consistent state.
               */
              _gimp_toggle_action_set_active (toggle, TRUE);
              changed1 = toggle;
            }
          else if (gimp_toggle_action_get_active (toggle))
            {
              changed2 = toggle;
            }
        }
    }

  if (! changed1)
    {
      g_warning ("Radio group does not contain an action with value '%d'",
                 current_value);
    }
  else
    {
      if (changed2)
        {
          _gimp_toggle_action_set_active (changed2, FALSE);
          g_object_notify (G_OBJECT (changed2), "active");
          g_signal_emit_by_name (changed2, "toggled");
          g_object_notify (G_OBJECT (changed2), "enabled");
          g_object_notify (G_OBJECT (changed2), "state");
        }
      g_object_notify (G_OBJECT (changed1), "active");

      for (slist = action->priv->group; slist; slist = slist->next)
        {
          g_object_notify (slist->data, "current-value");
          gimp_action_emit_change_state (GIMP_ACTION (slist->data),
                                         g_variant_new_int32 (current_value));
        }
      g_object_notify (G_OBJECT (changed1), "enabled");
      g_object_notify (G_OBJECT (changed1), "state");
    }
}
