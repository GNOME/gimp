/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpstringaction.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpmarshal.h"

#include "gimpstringaction.h"


enum
{
  SELECTED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_VALUE
};


static void   gimp_string_action_finalize     (GObject      *object);
static void   gimp_string_action_set_property (GObject      *object,
                                               guint         prop_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void   gimp_string_action_get_property (GObject      *object,
                                               guint         prop_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);

static void   gimp_string_action_activate     (GtkAction    *action);


G_DEFINE_TYPE (GimpStringAction, gimp_string_action, GIMP_TYPE_ACTION)

#define parent_class gimp_string_action_parent_class

static guint action_signals[LAST_SIGNAL] = { 0 };


static void
gimp_string_action_class_init (GimpStringActionClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkActionClass *action_class = GTK_ACTION_CLASS (klass);

  object_class->finalize     = gimp_string_action_finalize;
  object_class->set_property = gimp_string_action_set_property;
  object_class->get_property = gimp_string_action_get_property;

  action_class->activate = gimp_string_action_activate;

  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_string ("value",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));

  action_signals[SELECTED] =
    g_signal_new ("selected",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpStringActionClass, selected),
                  NULL, NULL,
                  gimp_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}

static void
gimp_string_action_init (GimpStringAction *action)
{
  action->value = NULL;
}

static void
gimp_string_action_finalize (GObject *object)
{
  GimpStringAction *action = GIMP_STRING_ACTION (object);

  if (action->value)
    {
      g_free (action->value);
      action->value = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_string_action_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpStringAction *action = GIMP_STRING_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      g_value_set_string (value, action->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_string_action_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpStringAction *action = GIMP_STRING_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      g_free (action->value);
      action->value = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

GimpStringAction *
gimp_string_action_new (const gchar *name,
                        const gchar *label,
                        const gchar *tooltip,
                        const gchar *stock_id,
                        const gchar *value)
{
  return g_object_new (GIMP_TYPE_STRING_ACTION,
                       "name",     name,
                       "label",    label,
                       "tooltip",  tooltip,
                       "stock-id", stock_id,
                       "value",    value,
                       NULL);
}

static void
gimp_string_action_activate (GtkAction *action)
{
  GimpStringAction *string_action = GIMP_STRING_ACTION (action);

  gimp_string_action_selected (string_action, string_action->value);
}

void
gimp_string_action_selected (GimpStringAction *action,
                             const gchar      *value)
{
  g_return_if_fail (GIMP_IS_STRING_ACTION (action));

  g_signal_emit (action, action_signals[SELECTED], 0, value);
}
