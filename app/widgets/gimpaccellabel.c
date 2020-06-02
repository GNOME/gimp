/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpaccellabel.c
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

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "gimpaction.h"
#include "gimpaccellabel.h"


enum
{
  PROP_0,
  PROP_ACTION
};


struct _GimpAccelLabelPrivate
{
  GimpAction *action;
};


/*  local function prototypes  */

static void   gimp_accel_label_dispose       (GObject             *object);
static void   gimp_accel_label_set_property  (GObject             *object,
                                              guint                property_id,
                                              const GValue        *value,
                                              GParamSpec          *pspec);
static void   gimp_accel_label_get_property  (GObject             *object,
                                              guint                property_id,
                                              GValue              *value,
                                              GParamSpec          *pspec);

static void   gimp_accel_label_accel_changed (GtkAccelGroup       *accel_group,
                                              guint                keyval,
                                              GdkModifierType      modifier,
                                              GClosure            *accel_closure,
                                              GimpAccelLabel      *accel_label);

static void   gimp_accel_label_update        (GimpAccelLabel      *accel_label);


G_DEFINE_TYPE_WITH_PRIVATE (GimpAccelLabel, gimp_accel_label, GTK_TYPE_LABEL)

#define parent_class gimp_accel_label_parent_class


/*  private functions  */

static void
gimp_accel_label_class_init (GimpAccelLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = gimp_accel_label_dispose;
  object_class->get_property = gimp_accel_label_get_property;
  object_class->set_property = gimp_accel_label_set_property;

  g_object_class_install_property (object_class, PROP_ACTION,
                                   g_param_spec_object ("action",
                                                        NULL, NULL,
                                                        GIMP_TYPE_ACTION,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_accel_label_init (GimpAccelLabel *accel_label)
{
  accel_label->priv = gimp_accel_label_get_instance_private (accel_label);
}

static void
gimp_accel_label_dispose (GObject *object)
{
  GimpAccelLabel *accel_label = GIMP_ACCEL_LABEL (object);

  gimp_accel_label_set_action (accel_label, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_accel_label_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpAccelLabel *accel_label = GIMP_ACCEL_LABEL (object);

  switch (property_id)
    {
    case PROP_ACTION:
      gimp_accel_label_set_action (accel_label, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_accel_label_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpAccelLabel *accel_label = GIMP_ACCEL_LABEL (object);

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
gimp_accel_label_accel_changed (GtkAccelGroup   *accel_group,
                                guint            keyval,
                                GdkModifierType  modifier,
                                GClosure        *accel_closure,
                                GimpAccelLabel  *accel_label)
{
  if (accel_closure ==
      gimp_action_get_accel_closure (accel_label->priv->action))
    {
      gimp_accel_label_update (accel_label);
    }
}

static gboolean
gimp_accel_label_update_accel_find_func (GtkAccelKey *key,
                                         GClosure    *closure,
                                         gpointer     data)
{
  return (GClosure *) data == closure;
}

static void
gimp_accel_label_update (GimpAccelLabel *accel_label)
{
  GClosure      *accel_closure;
  GtkAccelGroup *accel_group;
  GtkAccelKey   *accel_key;

  gtk_label_set_label (GTK_LABEL (accel_label), NULL);

  if (! accel_label->priv->action)
    return;

  accel_closure = gimp_action_get_accel_closure (accel_label->priv->action);

  if (! accel_closure)
    return;

  accel_group = gtk_accel_group_from_accel_closure (accel_closure);

  if (! accel_group)
    return;

  accel_key = gtk_accel_group_find (accel_group,
                                    gimp_accel_label_update_accel_find_func,
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
gimp_accel_label_new (GimpAction *action)
{
  g_return_val_if_fail (action == NULL || GIMP_IS_ACTION (action), NULL);

  return g_object_new (GIMP_TYPE_ACCEL_LABEL,
                       "action", action,
                       NULL);
}

void
gimp_accel_label_set_action (GimpAccelLabel *accel_label,
                             GimpAction     *action)
{
  g_return_if_fail (GIMP_IS_ACCEL_LABEL (accel_label));
  g_return_if_fail (action == NULL || GIMP_IS_ACTION (action));

  if (action != accel_label->priv->action)
    {
      if (accel_label->priv->action)
        {
          GClosure *accel_closure;

          accel_closure = gimp_action_get_accel_closure (
            accel_label->priv->action);

          if (accel_closure)
            {
              GtkAccelGroup *accel_group;

              accel_group = gtk_accel_group_from_accel_closure (accel_closure);

              g_signal_handlers_disconnect_by_func (
                accel_group,
                gimp_accel_label_accel_changed,
                accel_label);
            }
        }

      g_set_object (&accel_label->priv->action, action);

      if (accel_label->priv->action)
        {
          GClosure *accel_closure;

          accel_closure = gimp_action_get_accel_closure (
            accel_label->priv->action);

          if (accel_closure)
            {
              GtkAccelGroup *accel_group;

              accel_group = gtk_accel_group_from_accel_closure (accel_closure);

              g_signal_connect (accel_group, "accel-changed",
                                G_CALLBACK (gimp_accel_label_accel_changed),
                                accel_label);
            }
        }

      gimp_accel_label_update (accel_label);

      g_object_notify (G_OBJECT (accel_label), "action");
    }
}

GimpAction *
gimp_accel_label_get_action (GimpAccelLabel *accel_label)
{
  g_return_val_if_fail (GIMP_IS_ACCEL_LABEL (accel_label), NULL);

  return accel_label->priv->action;
}
