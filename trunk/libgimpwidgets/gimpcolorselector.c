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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorselector.h"
#include "gimpwidgetsmarshal.h"


enum
{
  COLOR_CHANGED,
  CHANNEL_CHANGED,
  LAST_SIGNAL
};


G_DEFINE_TYPE (GimpColorSelector, gimp_color_selector, GTK_TYPE_VBOX)

#define parent_class gimp_color_selector_parent_class

static guint selector_signals[LAST_SIGNAL] = { 0 };


static void
gimp_color_selector_class_init (GimpColorSelectorClass *klass)
{
  selector_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorSelectorClass, color_changed),
                  NULL, NULL,
                  _gimp_widgets_marshal_VOID__POINTER_POINTER,
                  G_TYPE_NONE, 2,
                  G_TYPE_POINTER,
                  G_TYPE_POINTER);

  selector_signals[CHANNEL_CHANGED] =
    g_signal_new ("channel-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorSelectorClass, channel_changed),
                  NULL, NULL,
                  _gimp_widgets_marshal_VOID__INT,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  klass->name                  = "Unnamed";
  klass->help_id               = NULL;
  klass->stock_id              = GTK_STOCK_SELECT_COLOR;

  klass->set_toggles_visible   = NULL;
  klass->set_toggles_sensitive = NULL;
  klass->set_show_alpha        = NULL;
  klass->set_color             = NULL;
  klass->set_channel           = NULL;
  klass->color_changed         = NULL;
  klass->channel_changed       = NULL;
  klass->set_config            = NULL;
}

static void
gimp_color_selector_init (GimpColorSelector *selector)
{
  selector->toggles_visible   = TRUE;
  selector->toggles_sensitive = TRUE;
  selector->show_alpha        = TRUE;

  gimp_rgba_set (&selector->rgb, 0.0, 0.0, 0.0, 1.0);
  gimp_rgb_to_hsv (&selector->rgb, &selector->hsv);

  selector->channel = GIMP_COLOR_SELECTOR_HUE;
}

GtkWidget *
gimp_color_selector_new (GType                     selector_type,
                         const GimpRGB            *rgb,
                         const GimpHSV            *hsv,
                         GimpColorSelectorChannel  channel)
{
  GimpColorSelector *selector;

  g_return_val_if_fail (g_type_is_a (selector_type, GIMP_TYPE_COLOR_SELECTOR),
                        NULL);
  g_return_val_if_fail (rgb != NULL, NULL);
  g_return_val_if_fail (hsv != NULL, NULL);

  selector = g_object_new (selector_type, NULL);

  gimp_color_selector_set_color (selector, rgb, hsv);
  gimp_color_selector_set_channel (selector, channel);

  return GTK_WIDGET (selector);
}

void
gimp_color_selector_set_toggles_visible (GimpColorSelector *selector,
                                         gboolean           visible)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  if (selector->toggles_visible != visible)
    {
      GimpColorSelectorClass *selector_class;

      selector->toggles_visible = visible ? TRUE : FALSE;

      selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_toggles_visible)
        selector_class->set_toggles_visible (selector, visible);
    }
}

void
gimp_color_selector_set_toggles_sensitive (GimpColorSelector *selector,
                                           gboolean           sensitive)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  if (selector->toggles_sensitive != sensitive)
    {
      GimpColorSelectorClass *selector_class;

      selector->toggles_sensitive = sensitive ? TRUE : FALSE;

      selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_toggles_sensitive)
        selector_class->set_toggles_sensitive (selector, sensitive);
    }
}

void
gimp_color_selector_set_show_alpha (GimpColorSelector *selector,
                                    gboolean           show_alpha)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  if (show_alpha != selector->show_alpha)
    {
      GimpColorSelectorClass *selector_class;

      selector->show_alpha = show_alpha ? TRUE : FALSE;

      selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_show_alpha)
        selector_class->set_show_alpha (selector, show_alpha);
    }
}

void
gimp_color_selector_set_color (GimpColorSelector *selector,
                               const GimpRGB     *rgb,
                               const GimpHSV     *hsv)
{
  GimpColorSelectorClass *selector_class;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (rgb != NULL);
  g_return_if_fail (hsv != NULL);

  selector->rgb = *rgb;
  selector->hsv = *hsv;

  selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

  if (selector_class->set_color)
    selector_class->set_color (selector, rgb, hsv);

  gimp_color_selector_color_changed (selector);
}

void
gimp_color_selector_set_channel (GimpColorSelector        *selector,
                                 GimpColorSelectorChannel  channel)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  if (channel != selector->channel)
    {
      GimpColorSelectorClass *selector_class;

      selector->channel = channel;

      selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

      if (selector_class->set_channel)
        selector_class->set_channel (selector, channel);

      gimp_color_selector_channel_changed (selector);
    }
}

void
gimp_color_selector_color_changed (GimpColorSelector *selector)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  g_signal_emit (selector, selector_signals[COLOR_CHANGED], 0,
                 &selector->rgb, &selector->hsv);
}

void
gimp_color_selector_channel_changed (GimpColorSelector *selector)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));

  g_signal_emit (selector, selector_signals[CHANNEL_CHANGED], 0,
                 selector->channel);
}

/**
 * gimp_color_selector_set_config:
 * @selector:
 * @config:
 *
 * Sets the color management configuration to use with this color selector.
 *
 * Since: GIMP 2.4
 */
void
gimp_color_selector_set_config (GimpColorSelector *selector,
                                GimpColorConfig   *config)
{
  GimpColorSelectorClass *selector_class;

  g_return_if_fail (GIMP_IS_COLOR_SELECTOR (selector));
  g_return_if_fail (config == NULL || GIMP_IS_COLOR_CONFIG (config));

  selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

  if (selector_class->set_config)
    selector_class->set_config (selector, config);
}
