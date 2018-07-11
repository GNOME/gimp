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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#undef GDK_MULTIHEAD_SAFE /* for gdk_keymap_get_default() */
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpcontrollermouse.h"
#include "gimphelp-ids.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


typedef struct _MouseEvent MouseEvent;

struct _MouseEvent
{
  const guint      button;
  const gchar     *modifier_string;
  GdkModifierType  modifiers;
  const gchar     *name;
  const gchar     *blurb;
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
  { 8, NULL, 0,
    "8",
    N_("Button 8") },
  { 8, "<Shift>", 0,
    "8-shift",
    N_("Button 8") },
  { 8, "<Primary>", 0,
    "8-primary",
    N_("Button 8") },
  { 8, "<Alt>", 0,
    "8-alt",
    N_("Button 8") },
  { 8, "<Shift><Primary>", 0,
    "8-shift-primary",
    N_("Button 8") },
  { 8, "<Shift><Alt>", 0,
    "8-shift-alt",
    N_("Button 8") },
  { 8, "<Primary><Alt>", 0,
    "8-primary-alt",
    N_("Button 8") },
  { 8, "<Shift><Primary><Alt>", 0,
    "8-shift-primary-alt",
    N_("Button 8") },

  { 9, NULL, 0,
    "9",
    N_("Button 9") },
  { 9, "<Shift>", 0,
    "9-shift",
    N_("Button 9") },
  { 9, "<Primary>", 0,
    "9-primary",
    N_("Button 9") },
  { 9, "<Alt>", 0,
    "9-alt",
    N_("Button 9") },
  { 9, "<Shift><Primary>", 0,
    "9-shift-primary",
    N_("Button 9") },
  { 9, "<Shift><Alt>", 0,
    "9-shift-alt",
    N_("Button 9") },
  { 9, "<Primary><Alt>", 0,
    "9-primary-alt",
    N_("Button 9") },
  { 9, "<Shift><Primary><Alt>", 0,
    "9-shift-primary-alt",
    N_("Button 9") },

  { 10, NULL, 0,
    "10",
    N_("Button 10") },
  { 10, "<Shift>", 0,
    "10-shift",
    N_("Button 10") },
  { 10, "<Primary>", 0,
    "10-primary",
    N_("Button 10") },
  { 10, "<Alt>", 0,
    "10-alt",
    N_("Button 10") },
  { 10, "<Shift><Primary>", 0,
    "10-shift-primary",
    N_("Button 10") },
  { 10, "<Shift><Alt>", 0,
    "10-shift-alt",
    N_("Button 10") },
  { 10, "<Primary><Alt>", 0,
    "10-primary-alt",
    N_("Button 10") },
  { 10, "<Shift><Primary><Alt>", 0,
    "10-shift-primary-alt",
    N_("Button 10") },

  { 11, NULL, 0,
    "11",
    N_("Button 11") },
  { 11, "<Shift>", 0,
    "11-shift",
    N_("Button 11") },
  { 11, "<Primary>", 0,
    "11-primary",
    N_("Button 11") },
  { 11, "<Alt>", 0,
    "11-alt",
    N_("Button 11") },
  { 11, "<Shift><Primary>", 0,
    "11-shift-primary",
    N_("Button 11") },
  { 11, "<Shift><Alt>", 0,
    "11-shift-alt",
    N_("Button 11") },
  { 11, "<Primary><Alt>", 0,
    "11-primary-alt",
    N_("Button 11") },
  { 11, "<Shift><Primary><Alt>", 0,
    "11-shift-primary-alt",
    N_("Button 11") },

  { 12, NULL, 0,
    "12",
    N_("Button 12") },
  { 12, "<Shift>", 0,
    "12-shift",
    N_("Button 12") },
  { 12, "<Primary>", 0,
    "12-primary",
    N_("Button 12") },
  { 12, "<Alt>", 0,
    "12-alt",
    N_("Button 12") },
  { 12, "<Shift><Primary>", 0,
    "12-shift-primary",
    N_("Button 12") },
  { 12, "<Shift><Alt>", 0,
    "12-shift-alt",
    N_("Button 12") },
  { 12, "<Primary><Alt>", 0,
    "12-primary-alt",
    N_("Button 12") },
  { 12, "<Shift><Primary><Alt>", 0,
    "12-shift-primary-alt",
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
  controller_class->icon_name       = GIMP_ICON_CONTROLLER_MOUSE;

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
      GdkKeymap *keymap = gdk_keymap_get_default ();
      gint       i;

      for (i = 0; i < G_N_ELEMENTS (mouse_events); i++)
        {
          MouseEvent *mevent = &mouse_events[i];

          if (mevent->modifier_string)
            {
              gtk_accelerator_parse (mevent->modifier_string, NULL,
                                     &mevent->modifiers);
              gdk_keymap_map_virtual_modifiers (keymap, &mevent->modifiers);
            }

          if (mevent->modifiers != 0)
            {
              mevent->blurb =
                g_strdup_printf ("%s (%s)", gettext (mevent->blurb),
                                 gimp_get_mod_string (mevent->modifiers));
            }
        }

      event_names_initialized = TRUE;
    }
}

static void
gimp_controller_mouse_constructed (GObject *object)
{
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

  /*  start with the last event because the last ones in the
   *  up,down,left,right groups have the most keyboard modifiers
   */
  for (i = G_N_ELEMENTS (mouse_events) - 1; i >= 0; i--)
    {
      if (mouse_events[i].button == bevent->button &&
          (mouse_events[i].modifiers & bevent->state) ==
          mouse_events[i].modifiers)
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

  return FALSE;
}
