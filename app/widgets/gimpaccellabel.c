/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaaccellabel.c
 * Copyright (C) 2020 Ell
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

#include "libligmabase/ligmabase.h"

#include "widgets-types.h"

#include "ligmaaction.h"
#include "ligmaaccellabel.h"


enum
{
  PROP_0,
  PROP_ACTION
};


struct _LigmaAccelLabelPrivate
{
  LigmaAction *action;
};


/*  local function prototypes  */

static void   ligma_accel_label_dispose       (GObject             *object);
static void   ligma_accel_label_set_property  (GObject             *object,
                                              guint                property_id,
                                              const GValue        *value,
                                              GParamSpec          *pspec);
static void   ligma_accel_label_get_property  (GObject             *object,
                                              guint                property_id,
                                              GValue              *value,
                                              GParamSpec          *pspec);

static void   ligma_accel_label_accel_changed (GtkAccelGroup       *accel_group,
                                              guint                keyval,
                                              GdkModifierType      modifier,
                                              GClosure            *accel_closure,
                                              LigmaAccelLabel      *accel_label);

static void   ligma_accel_label_update        (LigmaAccelLabel      *accel_label);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaAccelLabel, ligma_accel_label, GTK_TYPE_LABEL)

#define parent_class ligma_accel_label_parent_class


/*  private functions  */

static void
ligma_accel_label_class_init (LigmaAccelLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = ligma_accel_label_dispose;
  object_class->get_property = ligma_accel_label_get_property;
  object_class->set_property = ligma_accel_label_set_property;

  g_object_class_install_property (object_class, PROP_ACTION,
                                   g_param_spec_object ("action",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_ACTION,
                                                        LIGMA_PARAM_READWRITE));
}

static void
ligma_accel_label_init (LigmaAccelLabel *accel_label)
{
  accel_label->priv = ligma_accel_label_get_instance_private (accel_label);
}

static void
ligma_accel_label_dispose (GObject *object)
{
  LigmaAccelLabel *accel_label = LIGMA_ACCEL_LABEL (object);

  ligma_accel_label_set_action (accel_label, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_accel_label_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaAccelLabel *accel_label = LIGMA_ACCEL_LABEL (object);

  switch (property_id)
    {
    case PROP_ACTION:
      ligma_accel_label_set_action (accel_label, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_accel_label_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaAccelLabel *accel_label = LIGMA_ACCEL_LABEL (object);

  switch (property_id)
    {
    case PROP_ACTION:
      g_value_set_object (value, accel_label->priv->action);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_accel_label_accel_changed (GtkAccelGroup   *accel_group,
                                guint            keyval,
                                GdkModifierType  modifier,
                                GClosure        *accel_closure,
                                LigmaAccelLabel  *accel_label)
{
  if (accel_closure ==
      ligma_action_get_accel_closure (accel_label->priv->action))
    {
      ligma_accel_label_update (accel_label);
    }
}

static gboolean
ligma_accel_label_update_accel_find_func (GtkAccelKey *key,
                                         GClosure    *closure,
                                         gpointer     data)
{
  return (GClosure *) data == closure;
}

static void
ligma_accel_label_update (LigmaAccelLabel *accel_label)
{
  GClosure      *accel_closure;
  GtkAccelGroup *accel_group;
  GtkAccelKey   *accel_key;

  gtk_label_set_label (GTK_LABEL (accel_label), NULL);

  if (! accel_label->priv->action)
    return;

  accel_closure = ligma_action_get_accel_closure (accel_label->priv->action);

  if (! accel_closure)
    return;

  accel_group = gtk_accel_group_from_accel_closure (accel_closure);

  if (! accel_group)
    return;

  accel_key = gtk_accel_group_find (accel_group,
                                    ligma_accel_label_update_accel_find_func,
                                    accel_closure);

  if (accel_key            &&
      accel_key->accel_key &&
      (accel_key->accel_flags & GTK_ACCEL_VISIBLE))
    {
      gchar *label;

      label = gtk_accelerator_get_label (accel_key->accel_key,
                                         accel_key->accel_mods);

      gtk_label_set_label (GTK_LABEL (accel_label), label);

      g_free (label);
    }
}


/*  public functions  */

GtkWidget *
ligma_accel_label_new (LigmaAction *action)
{
  g_return_val_if_fail (action == NULL || LIGMA_IS_ACTION (action), NULL);

  return g_object_new (LIGMA_TYPE_ACCEL_LABEL,
                       "action", action,
                       NULL);
}

void
ligma_accel_label_set_action (LigmaAccelLabel *accel_label,
                             LigmaAction     *action)
{
  g_return_if_fail (LIGMA_IS_ACCEL_LABEL (accel_label));
  g_return_if_fail (action == NULL || LIGMA_IS_ACTION (action));

  if (action != accel_label->priv->action)
    {
      if (accel_label->priv->action)
        {
          GClosure *accel_closure;

          accel_closure = ligma_action_get_accel_closure (
            accel_label->priv->action);

          if (accel_closure)
            {
              GtkAccelGroup *accel_group;

              accel_group = gtk_accel_group_from_accel_closure (accel_closure);

              g_signal_handlers_disconnect_by_func (
                accel_group,
                ligma_accel_label_accel_changed,
                accel_label);
            }
        }

      g_set_object (&accel_label->priv->action, action);

      if (accel_label->priv->action)
        {
          GClosure *accel_closure;

          accel_closure = ligma_action_get_accel_closure (
            accel_label->priv->action);

          if (accel_closure)
            {
              GtkAccelGroup *accel_group;

              accel_group = gtk_accel_group_from_accel_closure (accel_closure);

              g_signal_connect (accel_group, "accel-changed",
                                G_CALLBACK (ligma_accel_label_accel_changed),
                                accel_label);
            }
        }

      ligma_accel_label_update (accel_label);

      g_object_notify (G_OBJECT (accel_label), "action");
    }
}

LigmaAction *
ligma_accel_label_get_action (LigmaAccelLabel *accel_label)
{
  g_return_val_if_fail (LIGMA_IS_ACCEL_LABEL (accel_label), NULL);

  return accel_label->priv->action;
}
