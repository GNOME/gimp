/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcontrollermouse.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2011 Mikael Magnusson <mikachu@src.gnome.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpcontrollermouse.h"
#include "gimphelp-ids.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


#define MODIFIER_MASK (GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK)


typedef struct _MouseEvent MouseEvent;

struct _MouseEvent
{
  const guint               button;
  const GdkModifierType     modifiers;
  const gchar              *name;
  const gchar              *blurb;
};


static void          gimp_controller_mouse_constructed     (GObject        *object);

static gint          gimp_controller_mouse_get_n_events    (GimpController *controller);
static const gchar * gimp_controller_mouse_get_event_name  (GimpController *controller,
                                                            gint            event_id);
static const gchar * gimp_controller_mouse_get_event_blurb (GimpController *controller,
                                                            gint            event_id);


G_DEFINE_TYPE (GimpControllerMouse, gimp_controller_mouse,
               GIMP_TYPE_CONTROLLER)

#define parent_class gimp_controller_mouse_parent_class


static MouseEvent mouse_events[] =
{
  { 8, 0,
    "8",
    N_("Button 8") },
  { 8, GDK_SHIFT_MASK,
    "8-shift",
    N_("Button 8") },
  { 8, GDK_CONTROL_MASK,
    "8-control",
    N_("Button 8") },
  { 8, GDK_MOD1_MASK,
    "8-alt",
    N_("Button 8") },
  { 8, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "8-shift-control",
    N_("Button 8") },
  { 8, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "8-shift-alt",
    N_("Button 8") },
  { 8, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "8-control-alt",
    N_("Button 8") },
  { 8, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "8-shift-control-alt",
    N_("Button 8") },

  { 9, 0,
    "9",
    N_("Button 9") },
  { 9, GDK_SHIFT_MASK,
    "9-shift",
    N_("Button 9") },
  { 9, GDK_CONTROL_MASK,
    "9-control",
    N_("Button 9") },
  { 9, GDK_MOD1_MASK,
    "9-alt",
    N_("Button 9") },
  { 9, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "9-shift-control",
    N_("Button 9") },
  { 9, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "9-shift-alt",
    N_("Button 9") },
  { 9, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "9-control-alt",
    N_("Button 9") },
  { 9, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "9-shift-control-alt",
    N_("Button 9") },

  { 10, 0,
    "10",
    N_("Button 10") },
  { 10, GDK_SHIFT_MASK,
    "10-shift",
    N_("Button 10") },
  { 10, GDK_CONTROL_MASK,
    "10-control",
    N_("Button 10") },
  { 10, GDK_MOD1_MASK,
    "10-alt",
    N_("Button 10") },
  { 10, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "10-shift-control",
    N_("Button 10") },
  { 10, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "10-shift-alt",
    N_("Button 10") },
  { 10, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "10-control-alt",
    N_("Button 10") },
  { 10, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "10-shift-control-alt",
    N_("Button 10") },

  { 11, 0,
    "11",
    N_("Button 11") },
  { 11, GDK_SHIFT_MASK,
    "11-shift",
    N_("Button 11") },
  { 11, GDK_CONTROL_MASK,
    "11-control",
    N_("Button 11") },
  { 11, GDK_MOD1_MASK,
    "11-alt",
    N_("Button 11") },
  { 11, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "11-shift-control",
    N_("Button 11") },
  { 11, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "11-shift-alt",
    N_("Button 11") },
  { 11, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "11-control-alt",
    N_("Button 11") },
  { 11, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "11-shift-control-alt",
    N_("Button 11") },

  { 12, 0,
    "12",
    N_("Button 12") },
  { 12, GDK_SHIFT_MASK,
    "12-shift",
    N_("Button 12") },
  { 12, GDK_CONTROL_MASK,
    "12-control",
    N_("Button 12") },
  { 12, GDK_MOD1_MASK,
    "12-alt",
    N_("Button 12") },
  { 12, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "12-shift-control",
    N_("Button 12") },
  { 12, GDK_MOD1_MASK | GDK_SHIFT_MASK,
    "12-shift-alt",
    N_("Button 12") },
  { 12, GDK_MOD1_MASK | GDK_CONTROL_MASK,
    "12-control-alt",
    N_("Button 12") },
  { 12, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    "12-shift-control-alt",
    N_("Button 12") },
};


static void
gimp_controller_mouse_class_init (GimpControllerMouseClass *klass)
{
  GObjectClass        *object_class     = G_OBJECT_CLASS (klass);
  GimpControllerClass *controller_class = GIMP_CONTROLLER_CLASS (klass);

  object_class->constructed         = gimp_controller_mouse_constructed;

  controller_class->name            = _("Mouse Buttons");
  controller_class->help_id         = GIMP_HELP_CONTROLLER_MOUSE;
  controller_class->stock_id        = GIMP_STOCK_CONTROLLER_MOUSE;

  controller_class->get_n_events    = gimp_controller_mouse_get_n_events;
  controller_class->get_event_name  = gimp_controller_mouse_get_event_name;
  controller_class->get_event_blurb = gimp_controller_mouse_get_event_blurb;
}

static void
gimp_controller_mouse_init (GimpControllerMouse *mouse)
{
  static gboolean event_names_initialized = FALSE;

  if (! event_names_initialized)
    {
      gint i;

      for (i = 0; i < G_N_ELEMENTS (mouse_events); i++)
        {
          MouseEvent *wevent = &mouse_events[i];

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

static void
gimp_controller_mouse_constructed (GObject *object)
{
  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  g_object_set (object,
                "name",  _("Mouse Button Events"),
                "state", _("Ready"),
                NULL);
}

static gint
gimp_controller_mouse_get_n_events (GimpController *controller)
{
  return G_N_ELEMENTS (mouse_events);
}

static const gchar *
gimp_controller_mouse_get_event_name (GimpController *controller,
                                      gint            event_id)
{
  if (event_id < 0 || event_id >= G_N_ELEMENTS (mouse_events))
    return NULL;

  return mouse_events[event_id].name;
}

static const gchar *
gimp_controller_mouse_get_event_blurb (GimpController *controller,
                                       gint            event_id)
{
  if (event_id < 0 || event_id >= G_N_ELEMENTS (mouse_events))
    return NULL;

  return mouse_events[event_id].blurb;
}

gboolean
gimp_controller_mouse_button (GimpControllerMouse  *mouse,
                              const GdkEventButton *bevent)
{
  gint i;

  g_return_val_if_fail (GIMP_IS_CONTROLLER_MOUSE (mouse), FALSE);
  g_return_val_if_fail (bevent != NULL, FALSE);

  for (i = 0; i < G_N_ELEMENTS (mouse_events); i++)
    {
      if (mouse_events[i].button == bevent->button)
        {
          if ((bevent->state & MODIFIER_MASK) == mouse_events[i].modifiers)
            {
              GimpControllerEvent         controller_event;
              GimpControllerEventTrigger *trigger;

              trigger = (GimpControllerEventTrigger *) &controller_event;

              trigger->type     = GIMP_CONTROLLER_EVENT_TRIGGER;
              trigger->source   = GIMP_CONTROLLER (mouse);
              trigger->event_id = i;

              return gimp_controller_event (GIMP_CONTROLLER (mouse),
                                            &controller_event);
            }
        }
    }

  return FALSE;
}
