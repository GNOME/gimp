/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacontroller.c
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "ligmawidgetstypes.h"

#include "ligmawidgetsmarshal.h"

#define LIGMA_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "ligmacontroller.h"
#include "ligmaicons.h"


/**
 * SECTION: ligmacontroller
 * @title: LigmaController
 * @short_description: Pluggable LIGMA input controller modules.
 *
 * An abstract interface for implementing arbitrary input controllers.
 **/


enum
{
  PROP_0,
  PROP_NAME,
  PROP_STATE
};

enum
{
  EVENT,
  LAST_SIGNAL
};


static void   ligma_controller_finalize     (GObject      *object);
static void   ligma_controller_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void   ligma_controller_get_property (GObject      *object,
                                            guint         property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (LigmaController, ligma_controller, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG, NULL))

#define parent_class ligma_controller_parent_class

static guint controller_signals[LAST_SIGNAL] = { 0 };


static void
ligma_controller_class_init (LigmaControllerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = ligma_controller_finalize;
  object_class->set_property = ligma_controller_set_property;
  object_class->get_property = ligma_controller_get_property;

  klass->name                = "Unnamed";
  klass->help_domain         = NULL;
  klass->help_id             = NULL;
  klass->icon_name           = LIGMA_ICON_CONTROLLER;

  klass->get_n_events        = NULL;
  klass->get_event_name      = NULL;
  klass->event               = NULL;

  g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "The controller's name",
                                                        "Unnamed Controller",
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_STATE,
                                   g_param_spec_string ("state",
                                                        "State",
                                                        "The controller's state, as human-readable string",
                                                        "Unknown",
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  controller_signals[EVENT] =
    g_signal_new ("event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaControllerClass, event),
                  g_signal_accumulator_true_handled, NULL,
                  _ligma_widgets_marshal_BOOLEAN__POINTER,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_POINTER);
}

static void
ligma_controller_init (LigmaController *controller)
{
}

static void
ligma_controller_finalize (GObject *object)
{
  LigmaController *controller = LIGMA_CONTROLLER (object);

  g_clear_pointer (&controller->name,  g_free);
  g_clear_pointer (&controller->state, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_controller_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaController *controller = LIGMA_CONTROLLER (object);

  switch (property_id)
    {
    case PROP_NAME:
      if (controller->name)
        g_free (controller->name);
      controller->name = g_value_dup_string (value);
      break;
    case PROP_STATE:
      if (controller->state)
        g_free (controller->state);
      controller->state = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_controller_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  LigmaController *controller = LIGMA_CONTROLLER (object);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, controller->name);
      break;
    case PROP_STATE:
      g_value_set_string (value, controller->state);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

LigmaController *
ligma_controller_new (GType controller_type)
{
  LigmaController *controller;

  g_return_val_if_fail (g_type_is_a (controller_type, LIGMA_TYPE_CONTROLLER),
                        NULL);

  controller = g_object_new (controller_type, NULL);

  return controller;
}

gint
ligma_controller_get_n_events (LigmaController *controller)
{
  g_return_val_if_fail (LIGMA_IS_CONTROLLER (controller), 0);

  if (LIGMA_CONTROLLER_GET_CLASS (controller)->get_n_events)
    return LIGMA_CONTROLLER_GET_CLASS (controller)->get_n_events (controller);

  return 0;
}

const gchar *
ligma_controller_get_event_name (LigmaController *controller,
                                gint            event_id)
{
  const gchar *name = NULL;

  g_return_val_if_fail (LIGMA_IS_CONTROLLER (controller), NULL);

  if (LIGMA_CONTROLLER_GET_CLASS (controller)->get_event_name)
    name = LIGMA_CONTROLLER_GET_CLASS (controller)->get_event_name (controller,
                                                                   event_id);

  if (! name)
    name = "<invalid event id>";

  return name;
}

const gchar *
ligma_controller_get_event_blurb (LigmaController *controller,
                                 gint            event_id)
{
  const gchar *blurb = NULL;

  g_return_val_if_fail (LIGMA_IS_CONTROLLER (controller), NULL);

  if (LIGMA_CONTROLLER_GET_CLASS (controller)->get_event_blurb)
    blurb =  LIGMA_CONTROLLER_GET_CLASS (controller)->get_event_blurb (controller,
                                                                      event_id);

  if (! blurb)
    blurb = "<invalid event id>";

  return blurb;
}

gboolean
ligma_controller_event (LigmaController            *controller,
                       const LigmaControllerEvent *event)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (LIGMA_IS_CONTROLLER (controller), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  g_signal_emit (controller, controller_signals[EVENT], 0,
                 event, &retval);

  return retval;
}
