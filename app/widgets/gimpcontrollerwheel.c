/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcontrollerwheel.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpcontrollerwheel.h"

#include "gimp-intl.h"


typedef struct _WheelEvent WheelEvent;

struct _WheelEvent
{
  GdkScrollDirection  direction;
  GdkModifierType     modifiers;
  const gchar        *name;
  const gchar        *blurb;
};


static void          gimp_controller_wheel_class_init      (GimpControllerWheelClass *klass);

static gint          gimp_controller_wheel_get_n_events    (GimpController *controller);
static const gchar * gimp_controller_wheel_get_event_name  (GimpController *controller,
                                                            gint            event_id);
static const gchar * gimp_controller_wheel_get_event_blurb (GimpController *controller,
                                                            gint            event_id);


static GimpControllerClass *parent_class = NULL;

static const WheelEvent wheel_events[] =
{
  { GDK_SCROLL_UP, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-up-shift-control-alt",
    N_("Alt + Control + Shift + Scroll Up") },
  { GDK_SCROLL_UP, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "scroll-up-control-alt",
    N_("Alt + Control + Scroll Up") },
  { GDK_SCROLL_UP, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "scroll-up-shift-alt",
    N_("Alt + Shift + Scroll Up") },
  { GDK_SCROLL_UP, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-up-shift-control",
    N_("Control + Shift + Scroll Up") },
  { GDK_SCROLL_UP, GDK_MOD1_MASK,
    "scroll-up-alt",
    N_("Alt + Scroll Up") },
  { GDK_SCROLL_UP, GDK_CONTROL_MASK,
    "scroll-up-control",
    N_("Control + Scroll Up") },
  { GDK_SCROLL_UP, GDK_SHIFT_MASK,
    "scroll-up-shift",
    N_("Shift + Scroll Up") },
  { GDK_SCROLL_UP, 0,
    "scroll-up",
    N_("Scroll Up") },

  { GDK_SCROLL_DOWN, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-down-shift-control-alt",
    N_("Alt + Control + Shift + Scroll Down") },
  { GDK_SCROLL_DOWN, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "scroll-down-control-alt",
    N_("Alt + Control + Scroll Down") },
  { GDK_SCROLL_DOWN, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "scroll-down-shift-alt",
    N_("Alt + Shift + Scroll Down") },
  { GDK_SCROLL_DOWN, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-down-shift-control",
    N_("Control + Shift + Scroll Down") },
  { GDK_SCROLL_DOWN, GDK_MOD1_MASK,
    "scroll-down-alt",
    N_("Alt + Scroll Down") },
  { GDK_SCROLL_DOWN, GDK_CONTROL_MASK,
    "scroll-down-control",
    N_("Control + Scroll Down") },
  { GDK_SCROLL_DOWN, GDK_SHIFT_MASK,
    "scroll-down-shift",
    N_("Shift + Scroll Down") },
  { GDK_SCROLL_DOWN, 0,
    "scroll-down",
    N_("Scroll Down") },

  { GDK_SCROLL_LEFT, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-left-shift-control-alt",
    N_("Alt + Control + Shift + Scroll Left") },
  { GDK_SCROLL_LEFT, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "scroll-left-control-alt",
    N_("Alt + Control + Scroll Left") },
  { GDK_SCROLL_LEFT, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "scroll-left-shift-alt",
    N_("Alt + Shift + Scroll Left") },
  { GDK_SCROLL_LEFT, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-left-shift-control",
    N_("Control + Shift + Scroll Left") },
  { GDK_SCROLL_LEFT, GDK_MOD1_MASK,
    "scroll-left-alt",
    N_("Alt + Scroll Left") },
  { GDK_SCROLL_LEFT, GDK_CONTROL_MASK,
    "scroll-left-control",
    N_("Control + Scroll Left") },
  { GDK_SCROLL_LEFT, GDK_SHIFT_MASK,
    "scroll-left-shift",
    N_("Shift + Scroll Left") },
  { GDK_SCROLL_LEFT, 0,
    "scroll-left",
    N_("Scroll Left") },

  { GDK_SCROLL_RIGHT, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-right-shift-control-alt",
    N_("Alt + Control + Shift + Scroll Right") },
  { GDK_SCROLL_RIGHT, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "scroll-right-control-alt",
    N_("Alt + Control + Scroll Right") },
  { GDK_SCROLL_RIGHT, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "scroll-right-shift-alt",
    N_("Alt + Shift + Scroll Right") },
  { GDK_SCROLL_RIGHT, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "scroll-right-shift-control",
    N_("Control + Shift + Scroll Right") },
  { GDK_SCROLL_RIGHT, GDK_MOD1_MASK,
    "scroll-right-alt",
    N_("Alt + Scroll Right") },
  { GDK_SCROLL_RIGHT, GDK_CONTROL_MASK,
    "scroll-right-control",
    N_("Control + Scroll Right") },
  { GDK_SCROLL_RIGHT, GDK_SHIFT_MASK,
    "scroll-right-shift",
    N_("Shift + Scroll Right") },
  { GDK_SCROLL_RIGHT, 0,
    "scroll-right",
    N_("Scroll Right") }
};


GType
gimp_controller_wheel_get_type (void)
{
  static GType controller_type = 0;

  if (! controller_type)
    {
      static const GTypeInfo controller_info =
      {
        sizeof (GimpControllerWheelClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_controller_wheel_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpControllerWheel),
        0,              /* n_preallocs    */
        NULL            /* instance_init  */
      };

      controller_type = g_type_register_static (GIMP_TYPE_CONTROLLER,
                                                "GimpControllerWheel",
                                                &controller_info, 0);
    }

  return controller_type;
}

static void
gimp_controller_wheel_class_init (GimpControllerWheelClass *klass)
{
  GimpControllerClass *controller_class = GIMP_CONTROLLER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  controller_class->name            = _("Mouse Wheel");

  controller_class->get_n_events    = gimp_controller_wheel_get_n_events;
  controller_class->get_event_name  = gimp_controller_wheel_get_event_name;
  controller_class->get_event_blurb = gimp_controller_wheel_get_event_blurb;
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

  return gettext (wheel_events[event_id].blurb);
}

gboolean
gimp_controller_wheel_scrolled (GimpControllerWheel  *wheel,
                                const GdkEventScroll *sevent)
{
  gint i;

  g_return_val_if_fail (GIMP_IS_CONTROLLER_WHEEL (wheel), FALSE);
  g_return_val_if_fail (sevent != NULL, FALSE);

  for (i = 0; i < G_N_ELEMENTS (wheel_events); i++)
    {
      if (wheel_events[i].direction == sevent->direction)
        {
          if ((wheel_events[i].modifiers & sevent->state) ==
              wheel_events[i].modifiers)
            {
              GimpControllerEvent         controller_event;
              GimpControllerEventTrigger *trigger;

              trigger = (GimpControllerEventTrigger *) &controller_event;

              trigger->type     = GIMP_CONTROLLER_EVENT_TRIGGER;
              trigger->source   = GIMP_CONTROLLER (wheel);
              trigger->event_id = i;

              if (gimp_controller_event (GIMP_CONTROLLER (wheel),
                                         &controller_event))
                {
                  return TRUE;
                }
            }
        }
    }

  return FALSE;
}
