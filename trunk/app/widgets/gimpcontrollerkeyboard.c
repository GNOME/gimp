/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcontrollerkeyboard.c
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
  gchar                 *blurb;
};


static GObject     * gimp_controller_keyboard_constructor     (GType           type,
                                                               guint           n_params,
                                                               GObjectConstructParam *params);

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
  { GDK_Up, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-up-shift-control-alt",
    N_("Cursor Up") },
  { GDK_Up, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "key-up-control-alt",
    N_("Cursor Up") },
  { GDK_Up, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "key-up-shift-alt",
    N_("Cursor Up") },
  { GDK_Up, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-up-shift-control",
    N_("Cursor Up") },
  { GDK_Up, GDK_MOD1_MASK,
    "key-up-alt",
    N_("Cursor Up") },
  { GDK_Up, GDK_CONTROL_MASK,
    "key-up-control",
    N_("Cursor Up") },
  { GDK_Up, GDK_SHIFT_MASK,
    "key-up-shift",
    N_("Cursor Up") },
  { GDK_Up, 0,
    "key-up",
    N_("Cursor Up") },

  { GDK_Down, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-down-shift-control-alt",
    N_("Cursor Down") },
  { GDK_Down, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "key-down-control-alt",
    N_("Cursor Down") },
  { GDK_Down, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "key-down-shift-alt",
    N_("Cursor Down") },
  { GDK_Down, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-down-shift-control",
    N_("Cursor Down") },
  { GDK_Down, GDK_MOD1_MASK,
    "key-down-alt",
    N_("Cursor Down") },
  { GDK_Down, GDK_CONTROL_MASK,
    "key-down-control",
    N_("Cursor Down") },
  { GDK_Down, GDK_SHIFT_MASK,
    "key-down-shift",
    N_("Cursor Down") },
  { GDK_Down, 0,
    "key-down",
    N_("Cursor Down") },

  { GDK_Left, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-left-shift-control-alt",
    N_("Cursor Left") },
  { GDK_Left, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "key-left-control-alt",
    N_("Cursor Left") },
  { GDK_Left, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "key-left-shift-alt",
    N_("Cursor Left") },
  { GDK_Left, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-left-shift-control",
    N_("Cursor Left") },
  { GDK_Left, GDK_MOD1_MASK,
    "key-left-alt",
    N_("Cursor Left") },
  { GDK_Left, GDK_CONTROL_MASK,
    "key-left-control",
    N_("Cursor Left") },
  { GDK_Left, GDK_SHIFT_MASK,
    "key-left-shift",
    N_("Cursor Left") },
  { GDK_Left, 0,
    "key-left",
    N_("Cursor Left") },

  { GDK_Right, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-right-shift-control-alt",
    N_("Cursor Right") },
  { GDK_Right, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "key-right-control-alt",
    N_("Cursor Right") },
  { GDK_Right, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "key-right-shift-alt",
    N_("Cursor Right") },
  { GDK_Right, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-right-shift-control",
    N_("Cursor Right") },
  { GDK_Right, GDK_MOD1_MASK,
    "key-right-alt",
    N_("Cursor Right") },
  { GDK_Right, GDK_CONTROL_MASK,
    "key-right-control",
    N_("Cursor Right") },
  { GDK_Right, GDK_SHIFT_MASK,
    "key-right-shift",
    N_("Cursor Right") },
  { GDK_Right, 0,
    "key-right",
    N_("Cursor Right") }
};


static void
gimp_controller_keyboard_class_init (GimpControllerKeyboardClass *klass)
{
  GObjectClass        *object_class     = G_OBJECT_CLASS (klass);
  GimpControllerClass *controller_class = GIMP_CONTROLLER_CLASS (klass);

  object_class->constructor         = gimp_controller_keyboard_constructor;

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

static GObject *
gimp_controller_keyboard_constructor (GType                  type,
                                      guint                  n_params,
                                      GObjectConstructParam *params)
{
  GObject *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  g_object_set (object,
                "name",  _("Keyboard Events"),
                "state", _("Ready"),
                NULL);

  return object;
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
