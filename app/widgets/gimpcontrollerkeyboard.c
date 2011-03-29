/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcontrollerkeyboard.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpcontrollerkeyboard.h"
#include "gimphelp-ids.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


typedef struct _KeyboardEvent KeyboardEvent;

struct _KeyboardEvent
{
  const guint            keyval;
  const GdkModifierType  modifiers;
  const gchar           *name;
  const gchar           *blurb;
};


static void          gimp_controller_keyboard_constructed     (GObject        *object);

static gint          gimp_controller_keyboard_get_n_events    (GimpController *controller);
static const gchar * gimp_controller_keyboard_get_event_name  (GimpController *controller,
                                                               gint            event_id);
static const gchar * gimp_controller_keyboard_get_event_blurb (GimpController *controller,
                                                               gint            event_id);


G_DEFINE_TYPE (GimpControllerKeyboard, gimp_controller_keyboard,
               GIMP_TYPE_CONTROLLER)

#define parent_class gimp_controller_keyboard_parent_class


static KeyboardEvent keyboard_events[] =
{
  { GDK_KEY_Up, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "cursor-up-shift-control-alt",
    N_("Cursor Up") },
  { GDK_KEY_Up, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "cursor-up-control-alt",
    N_("Cursor Up") },
  { GDK_KEY_Up, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "cursor-up-shift-alt",
    N_("Cursor Up") },
  { GDK_KEY_Up, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "cursor-up-shift-control",
    N_("Cursor Up") },
  { GDK_KEY_Up, GDK_MOD1_MASK,
    "cursor-up-alt",
    N_("Cursor Up") },
  { GDK_KEY_Up, GDK_CONTROL_MASK,
    "cursor-up-control",
    N_("Cursor Up") },
  { GDK_KEY_Up, GDK_SHIFT_MASK,
    "cursor-up-shift",
    N_("Cursor Up") },
  { GDK_KEY_Up, 0,
    "cursor-up",
    N_("Cursor Up") },

  { GDK_KEY_Down, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "cursor-down-shift-control-alt",
    N_("Cursor Down") },
  { GDK_KEY_Down, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "cursor-down-control-alt",
    N_("Cursor Down") },
  { GDK_KEY_Down, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "cursor-down-shift-alt",
    N_("Cursor Down") },
  { GDK_KEY_Down, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "cursor-down-shift-control",
    N_("Cursor Down") },
  { GDK_KEY_Down, GDK_MOD1_MASK,
    "cursor-down-alt",
    N_("Cursor Down") },
  { GDK_KEY_Down, GDK_CONTROL_MASK,
    "cursor-down-control",
    N_("Cursor Down") },
  { GDK_KEY_Down, GDK_SHIFT_MASK,
    "cursor-down-shift",
    N_("Cursor Down") },
  { GDK_KEY_Down, 0,
    "cursor-down",
    N_("Cursor Down") },

  { GDK_KEY_Left, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "cursor-left-shift-control-alt",
    N_("Cursor Left") },
  { GDK_KEY_Left, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "cursor-left-control-alt",
    N_("Cursor Left") },
  { GDK_KEY_Left, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "cursor-left-shift-alt",
    N_("Cursor Left") },
  { GDK_KEY_Left, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "cursor-left-shift-control",
    N_("Cursor Left") },
  { GDK_KEY_Left, GDK_MOD1_MASK,
    "cursor-left-alt",
    N_("Cursor Left") },
  { GDK_KEY_Left, GDK_CONTROL_MASK,
    "cursor-left-control",
    N_("Cursor Left") },
  { GDK_KEY_Left, GDK_SHIFT_MASK,
    "cursor-left-shift",
    N_("Cursor Left") },
  { GDK_KEY_Left, 0,
    "cursor-left",
    N_("Cursor Left") },

  { GDK_KEY_Right, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "cursor-right-shift-control-alt",
    N_("Cursor Right") },
  { GDK_KEY_Right, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "cursor-right-control-alt",
    N_("Cursor Right") },
  { GDK_KEY_Right, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "cursor-right-shift-alt",
    N_("Cursor Right") },
  { GDK_KEY_Right, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "cursor-right-shift-control",
    N_("Cursor Right") },
  { GDK_KEY_Right, GDK_MOD1_MASK,
    "cursor-right-alt",
    N_("Cursor Right") },
  { GDK_KEY_Right, GDK_CONTROL_MASK,
    "cursor-right-control",
    N_("Cursor Right") },
  { GDK_KEY_Right, GDK_SHIFT_MASK,
    "cursor-right-shift",
    N_("Cursor Right") },
  { GDK_KEY_Right, 0,
    "cursor-right",
    N_("Cursor Right") }
};


static void
gimp_controller_keyboard_class_init (GimpControllerKeyboardClass *klass)
{
  GObjectClass        *object_class     = G_OBJECT_CLASS (klass);
  GimpControllerClass *controller_class = GIMP_CONTROLLER_CLASS (klass);

  object_class->constructed         = gimp_controller_keyboard_constructed;

  controller_class->name            = _("Keyboard");
  controller_class->help_id         = GIMP_HELP_CONTROLLER_KEYBOARD;
  controller_class->stock_id        = GIMP_STOCK_CONTROLLER_KEYBOARD;

  controller_class->get_n_events    = gimp_controller_keyboard_get_n_events;
  controller_class->get_event_name  = gimp_controller_keyboard_get_event_name;
  controller_class->get_event_blurb = gimp_controller_keyboard_get_event_blurb;
}

static void
gimp_controller_keyboard_init (GimpControllerKeyboard *keyboard)
{
  static gboolean event_names_initialized = FALSE;

  if (! event_names_initialized)
    {
      gint i;

      for (i = 0; i < G_N_ELEMENTS (keyboard_events); i++)
        {
          KeyboardEvent *kevent = &keyboard_events[i];

          if (kevent->modifiers != 0)
            {
              kevent->blurb =
                g_strdup_printf ("%s (%s)", gettext (kevent->blurb),
                                 gimp_get_mod_string (kevent->modifiers));
            }
        }

      event_names_initialized = TRUE;
    }
}

static void
gimp_controller_keyboard_constructed (GObject *object)
{
  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  g_object_set (object,
                "name",  _("Keyboard Events"),
                "state", _("Ready"),
                NULL);
}

static gint
gimp_controller_keyboard_get_n_events (GimpController *controller)
{
  return G_N_ELEMENTS (keyboard_events);
}

static const gchar *
gimp_controller_keyboard_get_event_name (GimpController *controller,
                                         gint            event_id)
{
  if (event_id < 0 || event_id >= G_N_ELEMENTS (keyboard_events))
    return NULL;

  return keyboard_events[event_id].name;
}

static const gchar *
gimp_controller_keyboard_get_event_blurb (GimpController *controller,
                                          gint            event_id)
{
  if (event_id < 0 || event_id >= G_N_ELEMENTS (keyboard_events))
    return NULL;

  return keyboard_events[event_id].blurb;
}

gboolean
gimp_controller_keyboard_key_press (GimpControllerKeyboard *keyboard,
                                    const GdkEventKey      *kevent)
{
  gint i;

  g_return_val_if_fail (GIMP_IS_CONTROLLER_KEYBOARD (keyboard), FALSE);
  g_return_val_if_fail (kevent != NULL, FALSE);

  for (i = 0; i < G_N_ELEMENTS (keyboard_events); i++)
    {
      if (keyboard_events[i].keyval == kevent->keyval)
        {
          if ((keyboard_events[i].modifiers & kevent->state) ==
              keyboard_events[i].modifiers)
            {
              GimpControllerEvent         controller_event;
              GimpControllerEventTrigger *trigger;

              trigger = (GimpControllerEventTrigger *) &controller_event;

              trigger->type     = GIMP_CONTROLLER_EVENT_TRIGGER;
              trigger->source   = GIMP_CONTROLLER (keyboard);
              trigger->event_id = i;

              if (gimp_controller_event (GIMP_CONTROLLER (keyboard),
                                         &controller_event))
                {
                  return TRUE;
                }
            }
        }
    }

  return FALSE;
}
