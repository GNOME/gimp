/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcontrollerkeyboard.c
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
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpcontrollerkeyboard.h"
#include "gimphelp-ids.h"

#include "gimp-intl.h"


typedef struct _KeyboardEvent KeyboardEvent;

struct _KeyboardEvent
{
  guint            keyval;
  GdkModifierType  modifiers;
  const gchar     *name;
  const gchar     *blurb;
};


static void          gimp_controller_keyboard_class_init      (GimpControllerKeyboardClass *klass);
static void          gimp_controller_keyboard_init            (GimpControllerKeyboard      *keyboard);

static GObject     * gimp_controller_keyboard_constructor     (GType           type,
                                                               guint           n_params,
                                                               GObjectConstructParam *params);

static gint          gimp_controller_keyboard_get_n_events    (GimpController *controller);
static const gchar * gimp_controller_keyboard_get_event_name  (GimpController *controller,
                                                               gint            event_id);
static const gchar * gimp_controller_keyboard_get_event_blurb (GimpController *controller,
                                                               gint            event_id);


static GimpControllerClass *parent_class = NULL;

static const KeyboardEvent keyboard_events[] =
{
  { GDK_Up, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-up-shift-control-alt",
    N_("Cursor Up (Shift + Control + Alt)") },
  { GDK_Up, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "key-up-control-alt",
    N_("Cursor Up (Control + Alt)") },
  { GDK_Up, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "key-up-shift-alt",
    N_("Cursor Up (Shift + Alt)") },
  { GDK_Up, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-up-shift-control",
    N_("Cursor Up (Shift + Control)") },
  { GDK_Up, GDK_MOD1_MASK,
    "key-up-alt",
    N_("Cursor Up (Alt)") },
  { GDK_Up, GDK_CONTROL_MASK,
    "key-up-control",
    N_("Cursor Up (Control)") },
  { GDK_Up, GDK_SHIFT_MASK,
    "key-up-shift",
    N_("Cursor Up (Shift)") },
  { GDK_Up, 0,
    "key-up",
    N_("Cursor Up") },

  { GDK_Down, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-down-shift-control-alt",
    N_("Cursor Down (Shift + Control + Alt)") },
  { GDK_Down, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "key-down-control-alt",
    N_("Cursor Down (Control + Alt)") },
  { GDK_Down, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "key-down-shift-alt",
    N_("Cursor Down (Shift + Alt)") },
  { GDK_Down, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-down-shift-control",
    N_("Cursor Down (Shift + Control)") },
  { GDK_Down, GDK_MOD1_MASK,
    "key-down-alt",
    N_("Cursor Down (Alt)") },
  { GDK_Down, GDK_CONTROL_MASK,
    "key-down-control",
    N_("Cursor Down (Control)") },
  { GDK_Down, GDK_SHIFT_MASK,
    "key-down-shift",
    N_("Cursor Down (Shift)") },
  { GDK_Down, 0,
    "key-down",
    N_("Cursor Down") },

  { GDK_Left, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-left-shift-control-alt",
    N_("Cursor Left (Shift + Control + Alt)") },
  { GDK_Left, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "key-left-control-alt",
    N_("Cursor Left (Control + Alt)") },
  { GDK_Left, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "key-left-shift-alt",
    N_("Cursor Left (Shift + Alt)") },
  { GDK_Left, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-left-shift-control",
    N_("Cursor Left (Shift + Control)") },
  { GDK_Left, GDK_MOD1_MASK,
    "key-left-alt",
    N_("Cursor Left (Alt)") },
  { GDK_Left, GDK_CONTROL_MASK,
    "key-left-control",
    N_("Cursor Left (Control)") },
  { GDK_Left, GDK_SHIFT_MASK,
    "key-left-shift",
    N_("Cursor Left (Shift)") },
  { GDK_Left, 0,
    "key-left",
    N_("Cursor Left") },

  { GDK_Right, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-right-shift-control-alt",
    N_("Cursor Right (Shift + Control + Alt)") },
  { GDK_Right, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "key-right-control-alt",
    N_("Cursor Right (Control + Alt)") },
  { GDK_Right, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "key-right-shift-alt",
    N_("Cursor Right (Shift + Alt)") },
  { GDK_Right, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "key-right-shift-control",
    N_("Cursor Right (Shift + Control)") },
  { GDK_Right, GDK_MOD1_MASK,
    "key-right-alt",
    N_("Cursor Right (Alt)") },
  { GDK_Right, GDK_CONTROL_MASK,
    "key-right-control",
    N_("Cursor Right (Control)") },
  { GDK_Right, GDK_SHIFT_MASK,
    "key-right-shift",
    N_("Cursor Right (Shift)") },
  { GDK_Right, 0,
    "key-right",
    N_("Cursor Right") }
};


GType
gimp_controller_keyboard_get_type (void)
{
  static GType controller_type = 0;

  if (! controller_type)
    {
      static const GTypeInfo controller_info =
      {
        sizeof (GimpControllerKeyboardClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_controller_keyboard_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpControllerKeyboard),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_controller_keyboard_init,
      };

      controller_type = g_type_register_static (GIMP_TYPE_CONTROLLER,
                                                "GimpControllerKeyboard",
                                                &controller_info, 0);
    }

  return controller_type;
}

static void
gimp_controller_keyboard_class_init (GimpControllerKeyboardClass *klass)
{
  GObjectClass        *object_class     = G_OBJECT_CLASS (klass);
  GimpControllerClass *controller_class = GIMP_CONTROLLER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor         = gimp_controller_keyboard_constructor;

  controller_class->name            = _("Keyboard");
  controller_class->help_id         = GIMP_HELP_CONTROLLER_KEYBOARD;

  controller_class->get_n_events    = gimp_controller_keyboard_get_n_events;
  controller_class->get_event_name  = gimp_controller_keyboard_get_event_name;
  controller_class->get_event_blurb = gimp_controller_keyboard_get_event_blurb;
}

static void
gimp_controller_keyboard_init (GimpControllerKeyboard *keyboard)
{
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

  return gettext (keyboard_events[event_id].blurb);
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
