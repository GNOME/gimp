/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcontrollerwheel.c
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpcontrollerwheel.h"
#include "gimphelp-ids.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


#define MODIFIER_MASK (GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK)


typedef struct _WheelEvent WheelEvent;

struct _WheelEvent
{
  const GdkScrollDirection  direction;
  const GdkModifierType     modifiers;
  const gchar              *name;
  gchar                    *blurb;
};


static GObject     * gimp_controller_wheel_constructor     (GType           type,
                                                            guint           n_params,
                                                            GObjectConstructParam *params);

static gint          gimp_controller_wheel_get_n_events    (GimpController *controller);
static const gchar * gimp_controller_wheel_get_event_name  (GimpController *controller,
                                                            gint            event_id);
static const gchar * gimp_controller_wheel_get_event_blurb (GimpController *controller,
                                                            gint            event_id);


G_DEFINE_TYPE (GimpControllerWheel, gimp_controller_wheel,
               GIMP_TYPE_CONTROLLER)

#define parent_class gimp_controller_wheel_parent_class


static WheelEvent wheel_events[] =
{
  { GDK_SCROLL_UP, 0,
    "scroll-up",
    N_("Scroll Up") },
  { GDK_SCROLL_UP, GDK_SHIFT_MASK,
    "scroll-up-shift",
    N_("Scroll Up") },
  { GDK_SCROLL_UP, GDK_CONTROL_MASK,
    "scroll-up-control",
    N_("Scroll Up") },
  { GDK_SCROLL_UP, GDK_MOD1_MASK,
    "scroll-up-alt",
    N_("Scroll Up") },
  { GDK_SCROLL_UP, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-up-shift-control",
    N_("Scroll Up") },
  { GDK_SCROLL_UP, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "scroll-up-shift-alt",
    N_("Scroll Up") },
  { GDK_SCROLL_UP, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "scroll-up-control-alt",
    N_("Scroll Up") },
  { GDK_SCROLL_UP, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-up-shift-control-alt",
    N_("Scroll Up") },

  { GDK_SCROLL_DOWN, 0,
    "scroll-down",
    N_("Scroll Down") },
  { GDK_SCROLL_DOWN, GDK_SHIFT_MASK,
    "scroll-down-shift",
    N_("Scroll Down") },
  { GDK_SCROLL_DOWN, GDK_CONTROL_MASK,
    "scroll-down-control",
    N_("Scroll Down") },
  { GDK_SCROLL_DOWN, GDK_MOD1_MASK,
    "scroll-down-alt",
    N_("Scroll Down") },
  { GDK_SCROLL_DOWN, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-down-shift-control",
    N_("Scroll Down") },
  { GDK_SCROLL_DOWN, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "scroll-down-shift-alt",
    N_("Scroll Down") },
  { GDK_SCROLL_DOWN, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "scroll-down-control-alt",
    N_("Scroll Down") },
  { GDK_SCROLL_DOWN, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-down-shift-control-alt",
    N_("Scroll Down") },

  { GDK_SCROLL_LEFT, 0,
    "scroll-left",
    N_("Scroll Left") },
  { GDK_SCROLL_LEFT, GDK_SHIFT_MASK,
    "scroll-left-shift",
    N_("Scroll Left") },
  { GDK_SCROLL_LEFT, GDK_CONTROL_MASK,
    "scroll-left-control",
    N_("Scroll Left") },
  { GDK_SCROLL_LEFT, GDK_MOD1_MASK,
    "scroll-left-alt",
    N_("Scroll Left") },
  { GDK_SCROLL_LEFT, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-left-shift-control",
    N_("Scroll Left") },
  { GDK_SCROLL_LEFT, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "scroll-left-shift-alt",
    N_("Scroll Left") },
  { GDK_SCROLL_LEFT, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "scroll-left-control-alt",
    N_("Scroll Left") },
  { GDK_SCROLL_LEFT, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-left-shift-control-alt",
    N_("Scroll Left") },

  { GDK_SCROLL_RIGHT, 0,
    "scroll-right",
    N_("Scroll Right") },
  { GDK_SCROLL_RIGHT, GDK_SHIFT_MASK,
    "scroll-right-shift",
    N_("Scroll Right") },
  { GDK_SCROLL_RIGHT, GDK_CONTROL_MASK,
    "scroll-right-control",
    N_("Scroll Right") },
  { GDK_SCROLL_RIGHT, GDK_MOD1_MASK,
    "scroll-right-alt",
    N_("Scroll Right") },
  { GDK_SCROLL_RIGHT, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-right-shift-control",
    N_("Scroll Right") },
  { GDK_SCROLL_RIGHT, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "scroll-right-shift-alt",
    N_("Scroll Right") },
  { GDK_SCROLL_RIGHT, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "scroll-right-control-alt",
    N_("Scroll Right") },
  { GDK_SCROLL_RIGHT, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-right-shift-control-alt",
    N_("Scroll Right") }
};


static void
gimp_controller_wheel_class_init (GimpControllerWheelClass *klass)
{
  GObjectClass        *object_class     = G_OBJECT_CLASS (klass);
  GimpControllerClass *controller_class = GIMP_CONTROLLER_CLASS (klass);

  object_class->constructor         = gimp_controller_wheel_constructor;

  controller_class->name            = _("Mouse Wheel");
  controller_class->help_id         = GIMP_HELP_CONTROLLER_WHEEL;
  controller_class->stock_id        = GIMP_STOCK_CONTROLLER_WHEEL;

  controller_class->get_n_events    = gimp_controller_wheel_get_n_events;
  controller_class->get_event_name  = gimp_controller_wheel_get_event_name;
  controller_class->get_event_blurb = gimp_controller_wheel_get_event_blurb;
}

static void
gimp_controller_wheel_init (GimpControllerWheel *wheel)
{
  static gboolean event_names_initialized = FALSE;

  if (! event_names_initialized)
    {
      gint i;

      for (i = 0; i < G_N_ELEMENTS (wheel_events); i++)
        {
          WheelEvent *wevent = &wheel_events[i];

          if (wevent->modifiers != 0)
            {
              wevent->blurb =
                g_strdup_printf ("%s (%s)", gettext (wevent->blurb),
                                 gimp_get_mod_string (wevent->modifiers));
            }
        }

      event_names_initialized = TRUE;
    }
}

static GObject *
gimp_controller_wheel_constructor (GType                  type,
                                   guint                  n_params,
                                   GObjectConstructParam *params)
{
  GObject *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  g_object_set (object,
                "name",  _("Mouse Wheel Events"),
                "state", _("Ready"),
                NULL);

  return object;
}

static gint
gimp_controller_wheel_get_n_events (GimpController *controller)
{
  return G_N_ELEMENTS (wheel_events);
}

static const gchar *
gimp_controller_wheel_get_event_name (GimpController *controller,
                                      gint            event_id)
{
  if (event_id < 0 || event_id >= G_N_ELEMENTS (wheel_events))
    return NULL;

  return wheel_events[event_id].name;
}

static const gchar *
gimp_controller_wheel_get_event_blurb (GimpController *controller,
                                       gint            event_id)
{
  if (event_id < 0 || event_id >= G_N_ELEMENTS (wheel_events))
    return NULL;

  return wheel_events[event_id].blurb;
}

gboolean
gimp_controller_wheel_scroll (GimpControllerWheel  *wheel,
                              const GdkEventScroll *sevent)
{
  gint i;

  g_return_val_if_fail (GIMP_IS_CONTROLLER_WHEEL (wheel), FALSE);
  g_return_val_if_fail (sevent != NULL, FALSE);

  for (i = 0; i < G_N_ELEMENTS (wheel_events); i++)
    {
      if (wheel_events[i].direction == sevent->direction)
        {
          if ((sevent->state & MODIFIER_MASK) == wheel_events[i].modifiers)
            {
              GimpControllerEvent         controller_event;
              GimpControllerEventTrigger *trigger;

              trigger = (GimpControllerEventTrigger *) &controller_event;

              trigger->type     = GIMP_CONTROLLER_EVENT_TRIGGER;
              trigger->source   = GIMP_CONTROLLER (wheel);
              trigger->event_id = i;

              return gimp_controller_event (GIMP_CONTROLLER (wheel),
                                            &controller_event);
            }
        }
    }

  return FALSE;
}
