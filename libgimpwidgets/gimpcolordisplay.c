/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolordisplay.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcolordisplay.h"


enum
{
  CHANGED,
  LAST_SIGNAL
};


static void   gimp_color_display_class_init (GimpColorDisplayClass *klass);
static void   gimp_color_display_init       (GimpColorDisplay      *display);


static GObjectClass *parent_class = NULL;

static guint  display_signals[LAST_SIGNAL] = { 0 };


GType
gimp_color_display_get_type (void)
{
  static GType display_type = 0;

  if (! display_type)
    {
      static const GTypeInfo display_info =
      {
        sizeof (GimpColorDisplayClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_color_display_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpColorDisplay),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_color_display_init,
      };

      display_type = g_type_register_static (G_TYPE_OBJECT,
                                             "GimpColorDisplay",
                                             &display_info, 0);
    }

  return display_type;
}

static void
gimp_color_display_class_init (GimpColorDisplayClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);

  display_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorDisplayClass, changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  klass->name            = "Unnamed";
  klass->help_id         = NULL;

  klass->clone           = NULL;
  klass->convert         = NULL;
  klass->load_state      = NULL;
  klass->save_state      = NULL;
  klass->configure       = NULL;
  klass->configure_reset = NULL;
  klass->changed         = NULL;
}

static void
gimp_color_display_init (GimpColorDisplay *display)
{
  display->enabled = TRUE;
}

GimpColorDisplay *
gimp_color_display_new (GType display_type)
{
  GimpColorDisplay *display;

  g_return_val_if_fail (g_type_is_a (display_type, GIMP_TYPE_COLOR_DISPLAY),
                        NULL);

  display = g_object_new (display_type, NULL);

  return display;
}

GimpColorDisplay *
gimp_color_display_clone (GimpColorDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY (display), NULL);

  if (GIMP_COLOR_DISPLAY_GET_CLASS (display)->clone)
    return GIMP_COLOR_DISPLAY_GET_CLASS (display)->clone (display);

  return NULL;
}

void
gimp_color_display_set_enabled (GimpColorDisplay *display,
                                gboolean          enabled)
{
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  if (enabled != display->enabled)
    {
      display->enabled = enabled ? TRUE : FALSE;

      gimp_color_display_changed (display);
    }
}

gboolean
gimp_color_display_get_enabled (GimpColorDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY (display), FALSE);

  return display->enabled;
}

void
gimp_color_display_convert (GimpColorDisplay *display,
                            guchar            *buf,
                            gint               width,
                            gint               height,
                            gint               bpp,
                            gint               bpl)
{
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  if (display->enabled && GIMP_COLOR_DISPLAY_GET_CLASS (display)->convert)
    GIMP_COLOR_DISPLAY_GET_CLASS (display)->convert (display, buf,
                                                     width, height,
                                                     bpp, bpl);
}

void
gimp_color_display_load_state (GimpColorDisplay *display,
                               GimpParasite     *state)
{
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));
  g_return_if_fail (state != NULL);

  if (GIMP_COLOR_DISPLAY_GET_CLASS (display)->load_state)
    GIMP_COLOR_DISPLAY_GET_CLASS (display)->load_state (display, state);
}

GimpParasite *
gimp_color_display_save_state (GimpColorDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY (display), NULL);

  if (GIMP_COLOR_DISPLAY_GET_CLASS (display)->save_state)
    return GIMP_COLOR_DISPLAY_GET_CLASS (display)->save_state (display);

  return NULL;
}

GtkWidget *
gimp_color_display_configure (GimpColorDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY (display), NULL);

  if (GIMP_COLOR_DISPLAY_GET_CLASS (display)->configure)
    return GIMP_COLOR_DISPLAY_GET_CLASS (display)->configure (display);

  return NULL;
}

void
gimp_color_display_configure_reset (GimpColorDisplay *display)
{
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  if (GIMP_COLOR_DISPLAY_GET_CLASS (display)->configure_reset)
    GIMP_COLOR_DISPLAY_GET_CLASS (display)->configure_reset (display);
}

void
gimp_color_display_changed (GimpColorDisplay *display)
{
  g_return_if_fail (GIMP_IS_COLOR_DISPLAY (display));

  g_signal_emit (display, display_signals[CHANGED], 0);
}
