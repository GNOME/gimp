/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorselector.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on:
 * Colour selector module
 * Copyright (C) 1999 Austin Donnelly <austin@greenend.org.uk>
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

#include "gimpcolorselector.h"
#include "gimpwidgetsmarshal.h"


enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};


static void   gimp_color_selector_class_init (GimpColorSelectorClass *klass);
static void   gimp_color_selector_init       (GimpColorSelector      *selector);


static GtkVBoxClass *parent_class = NULL;

static guint  selector_signals[LAST_SIGNAL] = { 0 };


GType
gimp_color_selector_get_type (void)
{
  static GType selector_type = 0;

  if (! selector_type)
    {
      static const GTypeInfo selector_info =
      {
        sizeof (GimpColorSelectorClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_color_selector_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpColorSelector),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_color_selector_init,
      };

      selector_type = g_type_register_static (GTK_TYPE_VBOX,
                                              "GimpColorSelector", 
                                              &selector_info, 0);
    }

  return selector_type;
}

static void
gimp_color_selector_class_init (GimpColorSelectorClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);

  selector_signals[COLOR_CHANGED] =
    g_signal_new ("color_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorSelectorClass, color_changed),
                  NULL, NULL,
                  _gimp_widgets_marshal_VOID__POINTER_POINTER,
                  G_TYPE_NONE, 2,
                  G_TYPE_POINTER,
                  G_TYPE_POINTER);

  klass->set_color     = NULL;
  klass->set_channel   = NULL;
  klass->color_changed = NULL;
}

static void
gimp_color_selector_init (GimpColorSelector *selector)
{
}

GtkWidget *
gimp_color_selector_new (GType          selector_type,
                         const GimpRGB *rgb,
                         const GimpHSV *hsv)
{
  GimpColorSelector *selector;

  g_return_val_if_fail (g_type_is_a (selector_type, GIMP_TYPE_COLOR_SELECTOR),
                        NULL);
  g_return_val_if_fail (rgb != NULL, NULL);
  g_return_val_if_fail (hsv != NULL, NULL);

  selector = g_object_new (selector_type, NULL);

  gimp_color_selector_set_color (selector, rgb, hsv);

  return GTK_WIDGET (selector);
}

void
gimp_color_selector_set_color (GimpColorSelector *selector,
                               const GimpRGB     *rgb,
                               const GimpHSV     *hsv)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsv != NULL);

  if (GIMP_COLOR_SELECTOR_GET_CLASS (selector)->set_color)
    GIMP_COLOR_SELECTOR_GET_CLASS (selector)->set_color (selector, rgb, hsv);
}

void
gimp_color_selector_set_channel (GimpColorSelector        *selector,
                                 GimpColorSelectorChannel  channel)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  if (GIMP_COLOR_SELECTOR_GET_CLASS (selector)->set_channel)
    GIMP_COLOR_SELECTOR_GET_CLASS (selector)->set_channel (selector, channel);
}

void
gimp_color_selector_color_changed (GimpColorSelector *selector,
                                   const GimpRGB     *rgb,
                                   const GimpHSV     *hsv)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsv != NULL);

  g_signal_emit (G_OBJECT (selector), selector_signals[COLOR_CHANGED], 0,
                 rgb, hsv);
}
