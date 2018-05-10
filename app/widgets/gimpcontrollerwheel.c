/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcontrollerwheel.c
 * Copyright (C) 2004-2015 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpcontrollerwheel.h"
#include "gimphelp-ids.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


typedef struct _WheelEvent WheelEvent;

struct _WheelEvent
{
  const GdkScrollDirection  direction;
  const gchar              *modifier_string;
  GdkModifierType           modifiers;
  const gchar              *name;
  const gchar              *blurb;
};


static void          gimp_controller_wheel_constructed     (GObject        *object);

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
  { GDK_SCROLL_UP, NULL, 0,
    "scroll-up",
    N_("Scroll Up") },
  { GDK_SCROLL_UP, "<Shift>", 0,
    "scroll-up-shift",
    N_("Scroll Up") },
  { GDK_SCROLL_UP, "<Primary>", 0,
    "scroll-up-primary",
    N_("Scroll Up") },
  { GDK_SCROLL_UP, "<Alt>", 0,
    "scroll-up-alt",
    N_("Scroll Up") },
  { GDK_SCROLL_UP, "<Shift><Primary>", 0,
    "scroll-up-shift-primary",
    N_("Scroll Up") },
  { GDK_SCROLL_UP, "<Shift><Alt>", 0,
    "scroll-up-shift-alt",
    N_("Scroll Up") },
  { GDK_SCROLL_UP, "<Primary><Alt>", 0,
    "scroll-up-primary-alt",
    N_("Scroll Up") },
  { GDK_SCROLL_UP, "<Shift><Primary><Alt>", 0,
    "scroll-up-shift-primary-alt",
    N_("Scroll Up") },

  { GDK_SCROLL_DOWN, NULL, 0,
    "scroll-down",
    N_("Scroll Down") },
  { GDK_SCROLL_DOWN, "<Shift>", 0,
    "scroll-down-shift",
    N_("Scroll Down") },
  { GDK_SCROLL_DOWN, "<Primary>", 0,
    "scroll-down-primary",
    N_("Scroll Down") },
  { GDK_SCROLL_DOWN, "<Alt>", 0,
    "scroll-down-alt",
    N_("Scroll Down") },
  { GDK_SCROLL_DOWN, "<Shift><Primary>", 0,
    "scroll-down-shift-primary",
    N_("Scroll Down") },
  { GDK_SCROLL_DOWN, "<Shift><Alt>", 0,
    "scroll-down-shift-alt",
    N_("Scroll Down") },
  { GDK_SCROLL_DOWN, "<Primary><Alt>", 0,
    "scroll-down-primary-alt",
    N_("Scroll Down") },
  { GDK_SCROLL_DOWN, "<Shift><Primary><Alt>", 0,
    "scroll-down-shift-primary-alt",
    N_("Scroll Down") },

  { GDK_SCROLL_LEFT, NULL, 0,
    "scroll-left",
    N_("Scroll Left") },
  { GDK_SCROLL_LEFT, "<Shift>", 0,
    "scroll-left-shift",
    N_("Scroll Left") },
  { GDK_SCROLL_LEFT, "<Primary>", 0,
    "scroll-left-primary",
    N_("Scroll Left") },
  { GDK_SCROLL_LEFT, "<Alt>", 0,
    "scroll-left-alt",
    N_("Scroll Left") },
  { GDK_SCROLL_LEFT, "<Shift><Primary>", 0,
    "scroll-left-shift-primary",
    N_("Scroll Left") },
  { GDK_SCROLL_LEFT, "<Shift><Alt>", 0,
    "scroll-left-shift-alt",
    N_("Scroll Left") },
  { GDK_SCROLL_LEFT, "<Primary><Alt>", 0,
    "scroll-left-primary-alt",
    N_("Scroll Left") },
  { GDK_SCROLL_LEFT, "<Shift><Primary><Alt>", 0,
    "scroll-left-shift-primary-alt",
    N_("Scroll Left") },

  { GDK_SCROLL_RIGHT, NULL, 0,
    "scroll-right",
    N_("Scroll Right") },
  { GDK_SCROLL_RIGHT, "<Shift>", 0,
    "scroll-right-shift",
    N_("Scroll Right") },
  { GDK_SCROLL_RIGHT, "<Primary>", 0,
    "scroll-right-primary",
    N_("Scroll Right") },
  { GDK_SCROLL_RIGHT, "<Alt>", 0,
    "scroll-right-alt",
    N_("Scroll Right") },
  { GDK_SCROLL_RIGHT, "<Shift><Primary>", 0,
    "scroll-right-shift-primary",
    N_("Scroll Right") },
  { GDK_SCROLL_RIGHT, "<Shift><Alt>", 0,
    "scroll-right-shift-alt",
    N_("Scroll Right") },
  { GDK_SCROLL_RIGHT, "<Primary><Alt>", 0,
    "scroll-right-primary-alt",
    N_("Scroll Right") },
  { GDK_SCROLL_RIGHT, "<Shift><Primary><Alt>", 0,
    "scroll-right-shift-primary-alt",
    N_("Scroll Right") }
};


static void
gimp_controller_wheel_class_init (GimpControllerWheelClass *klass)
{
  GObjectClass        *object_class     = G_OBJECT_CLASS (klass);
  GimpControllerClass *controller_class = GIMP_CONTROLLER_CLASS (klass);

  object_class->constructed         = gimp_controller_wheel_constructed;

  controller_class->name            = _("Mouse Wheel");
  controller_class->help_id         = GIMP_HELP_CONTROLLER_WHEEL;
  controller_class->icon_name       = GIMP_ICON_CONTROLLER_WHEEL;

  controller_class->get_n_events    = gimp_controller_wheel_get_n_events;
  controller_class->get_event_name  = gimp_controller_wheel_get_event_name;
  controller_class->get_event_blurb = gimp_controller_wheel_get_event_blurb;
}

static void
gimp_controller_wheel_init (GimpControllerWheel *wheel)
{
  static gboolean events_initialized = FALSE;

  if (! events_initialized)
    {
      GdkKeymap *keymap = gdk_keymap_get_for_display (gdk_display_get_default ());
      gint       i;

      for (i = 0; i < G_N_ELEMENTS (wheel_events); i++)
        {
          WheelEvent *wevent = &wheel_events[i];

          if (wevent->modifier_string)
            {
              gtk_accelerator_parse (wevent->modifier_string, NULL,
                                     &wevent->modifiers);
              gdk_keymap_map_virtual_modifiers (keymap, &wevent->modifiers);
            }

          if (wevent->modifiers != 0)
            {
              wevent->blurb =
                g_strdup_printf ("%s (%s)", gettext (wevent->blurb),
                                 gimp_get_mod_string (wevent->modifiers));
            }
          else
            {
              wevent->blurb = gettext (wevent->blurb);
            }
        }

      events_initialized = TRUE;
    }
}

static void
gimp_controller_wheel_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_object_set (object,
                "name",  _("Mouse Wheel Events"),
                "state", _("Ready"),
                NULL);
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

  /*  start with the last event because the last ones in the
   *  up,down,left,right groups have the most keyboard modifiers
   */
  for (i = G_N_ELEMENTS (wheel_events) - 1; i >= 0; i--)
    {
      if (wheel_events[i].direction == sevent->direction &&
          (wheel_events[i].modifiers & sevent->state) ==
          wheel_events[i].modifiers)
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

  return FALSE;
}
