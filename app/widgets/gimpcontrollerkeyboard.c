/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcontrollerkeyboard.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#undef GDK_MULTIHEAD_SAFE /* for gdk_keymap_get_default() */
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
  const guint      keyval;
  const gchar     *modifier_string;
  GdkModifierType  modifiers;
  const gchar     *name;
  const gchar     *blurb;
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
  { GDK_KEY_Up, NULL, 0,
    "cursor-up",
    N_("Cursor Up") },
  { GDK_KEY_Up, "<Shift>", 0,
    "cursor-up-shift",
    N_("Cursor Up") },
  { GDK_KEY_Up, "<Primary>", 0,
    "cursor-up-primary",
    N_("Cursor Up") },
  { GDK_KEY_Up, "<Alt>", 0,
    "cursor-up-alt",
    N_("Cursor Up") },
  { GDK_KEY_Up, "<Shift><Primary>", 0,
    "cursor-up-shift-primary",
    N_("Cursor Up") },
  { GDK_KEY_Up, "<Shift><Alt>", 0,
    "cursor-up-shift-alt",
    N_("Cursor Up") },
  { GDK_KEY_Up, "<Primary><Alt>", 0,
    "cursor-up-primary-alt",
    N_("Cursor Up") },
  { GDK_KEY_Up, "<Shift><Primary><Alt>", 0,
    "cursor-up-shift-primary-alt",
    N_("Cursor Up") },

  { GDK_KEY_Down, NULL, 0,
    "cursor-down",
    N_("Cursor Down") },
  { GDK_KEY_Down, "<Shift>", 0,
    "cursor-down-shift",
    N_("Cursor Down") },
  { GDK_KEY_Down, "<Primary>", 0,
    "cursor-down-primary",
    N_("Cursor Down") },
  { GDK_KEY_Down, "<Alt>", 0,
    "cursor-down-alt",
    N_("Cursor Down") },
  { GDK_KEY_Down, "<Shift><Primary>", 0,
    "cursor-down-shift-primary",
    N_("Cursor Down") },
  { GDK_KEY_Down, "<Shift><Alt>", 0,
    "cursor-down-shift-alt",
    N_("Cursor Down") },
  { GDK_KEY_Down, "<Primary><Alt>", 0,
    "cursor-down-primary-alt",
    N_("Cursor Down") },
  { GDK_KEY_Down, "<Shift><Primary><Alt>", 0,
    "cursor-down-shift-primary-alt",
    N_("Cursor Down") },

  { GDK_KEY_Left, NULL, 0,
    "cursor-left",
    N_("Cursor Left") },
  { GDK_KEY_Left, "<Shift>", 0,
    "cursor-left-shift",
    N_("Cursor Left") },
  { GDK_KEY_Left, "<Primary>", 0,
    "cursor-left-primary",
    N_("Cursor Left") },
  { GDK_KEY_Left, "<Alt>", 0,
    "cursor-left-alt",
    N_("Cursor Left") },
  { GDK_KEY_Left, "<Shift><Primary>", 0,
    "cursor-left-shift-primary",
    N_("Cursor Left") },
  { GDK_KEY_Left, "<Shift><Alt>", 0,
    "cursor-left-shift-alt",
    N_("Cursor Left") },
  { GDK_KEY_Left, "<Primary><Alt>", 0,
    "cursor-left-primary-alt",
    N_("Cursor Left") },
  { GDK_KEY_Left, "<Shift><Primary><Alt>", 0,
    "cursor-left-shift-primary-alt",
    N_("Cursor Left") },

  { GDK_KEY_Right, NULL, 0,
    "cursor-right",
    N_("Cursor Right") },
  { GDK_KEY_Right, "<Shift>", 0,
    "cursor-right-shift",
    N_("Cursor Right") },
  { GDK_KEY_Right, "<Primary>", 0,
    "cursor-right-primary",
    N_("Cursor Right") },
  { GDK_KEY_Right, "<Alt>", 0,
    "cursor-right-alt",
    N_("Cursor Right") },
  { GDK_KEY_Right, "<Shift><Primary>", 0,
    "cursor-right-shift-primary",
    N_("Cursor Right") },
  { GDK_KEY_Right, "<Shift><Alt>", 0,
    "cursor-right-shift-alt",
    N_("Cursor Right") },
  { GDK_KEY_Right, "<Primary><Alt>", 0,
    "cursor-right-primary-alt",
    N_("Cursor Right") },
  { GDK_KEY_Right, "<Shift><Primary><Alt>", 0,
    "cursor-right-shift-primary-alt",
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
  controller_class->icon_name       = GIMP_ICON_CONTROLLER_KEYBOARD;

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
      GdkKeymap *keymap = gdk_keymap_get_default ();
      gint       i;

      for (i = 0; i < G_N_ELEMENTS (keyboard_events); i++)
        {
          KeyboardEvent *kevent = &keyboard_events[i];

          if (kevent->modifier_string)
            {
              gtk_accelerator_parse (kevent->modifier_string, NULL,
                                     &kevent->modifiers);
              gdk_keymap_map_virtual_modifiers (keymap, &kevent->modifiers);
            }

          if (kevent->modifiers != 0)
            {
              kevent->blurb =
                g_strdup_printf ("%s (%s)", gettext (kevent->blurb),
                                 gimp_get_mod_string (kevent->modifiers));
            }
          else
            {
              kevent->blurb = gettext (kevent->blurb);
            }
        }

      event_names_initialized = TRUE;
    }
}

static void
gimp_controller_keyboard_constructed (GObject *object)
{
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

  /*  start with the last event because the last ones in the
   *  up,down,left,right groups have the most keyboard modifiers
   */
  for (i = G_N_ELEMENTS (keyboard_events) - 1; i >= 0; i--)
    {
      if (keyboard_events[i].keyval == kevent->keyval &&
          (keyboard_events[i].modifiers & kevent->state) ==
          keyboard_events[i].modifiers)
        {
          GimpControllerEvent         controller_event;
          GimpControllerEventTrigger *trigger;

          trigger = (GimpControllerEventTrigger *) &controller_event;

          trigger->type     = GIMP_CONTROLLER_EVENT_TRIGGER;
          trigger->source   = GIMP_CONTROLLER (keyboard);
          trigger->event_id = i;

          return gimp_controller_event (GIMP_CONTROLLER (keyboard),
                                        &controller_event);
        }
    }

  return FALSE;
}
