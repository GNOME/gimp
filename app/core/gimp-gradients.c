/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * gimp-gradients.c
 * Copyright (C) 2002 Michael Natterer  <mitch@gimp.org>
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

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "config/gimpbaseconfig.h"

#include "gimp.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpdatafactory.h"
#include "gimpgradient.h"

#include "gimp-intl.h"


#define FG_BG_RGB_KEY      "gimp-fg-bg-rgb-gradient"
#define FG_BG_HSV_CCW_KEY  "gimp-fg-bg-hsv-ccw-gradient"
#define FG_BG_HSV_CW_KEY   "gimp-fg-bg-hsv-cw-gradient"
#define FG_TRANSPARENT_KEY "gimp-fg-transparent-gradient"


/*  local function prototypes  */

static GimpGradient * gimp_gradients_add_gradient       (Gimp          *gimp,
                                                         const gchar   *name,
                                                         const gchar   *id);
static void           gimp_gradients_foreground_changed (GimpContext   *context,
                                                         const GimpRGB *fg,
                                                         Gimp          *gimp);
static void           gimp_gradients_background_changed (GimpContext   *context,
                                                         const GimpRGB *bg,
                                                         Gimp          *gimp);


/*  public functions  */

void
gimp_gradients_init (Gimp *gimp)
{
  GimpGradient *gradient;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gradient = gimp_gradients_add_gradient (gimp,
                                          _("FG to BG (RGB)"),
                                          FG_BG_RGB_KEY);
  gimp_context_set_gradient (gimp->user_context, gradient);

  gradient = gimp_gradients_add_gradient (gimp,
                                          _("FG to BG (HSV counter-clockwise)"),
                                          FG_BG_HSV_CCW_KEY);
  gradient->segments->color = GIMP_GRADIENT_SEGMENT_HSV_CCW;

  gradient = gimp_gradients_add_gradient (gimp,
                                          _("FG to BG (HSV clockwise hue)"),
                                          FG_BG_HSV_CW_KEY);
  gradient->segments->color = GIMP_GRADIENT_SEGMENT_HSV_CW;

  gradient = gimp_gradients_add_gradient (gimp,
                                          _("FG to Transparent"),
                                          FG_TRANSPARENT_KEY);
  gimp_rgba_set (&gradient->segments->right_color,
                 gradient->segments->left_color.r,
                 gradient->segments->left_color.g,
                 gradient->segments->left_color.b,
                 0.0);

  g_signal_connect (gimp->user_context, "foreground_changed",
                    G_CALLBACK (gimp_gradients_foreground_changed),
                    gimp);
  g_signal_connect (gimp->user_context, "background_changed",
                    G_CALLBACK (gimp_gradients_background_changed),
                    gimp);
}


/*  private functions  */

static GimpGradient *
gimp_gradients_add_gradient (Gimp        *gimp,
                             const gchar *name,
                             const gchar *id)
{
  GimpBaseConfig *base_config;
  GimpGradient   *gradient;

  base_config = GIMP_BASE_CONFIG (gimp->config);

  gradient = GIMP_GRADIENT (gimp_gradient_new (name,
                                               base_config->stingy_memory_use));

  gimp_data_make_internal (GIMP_DATA (gradient));

  gimp_context_get_foreground (gimp->user_context,
                               &gradient->segments->left_color);
  gimp_context_get_background (gimp->user_context,
                               &gradient->segments->right_color);

  gimp_container_add (gimp->gradient_factory->container,
                      GIMP_OBJECT (gradient));
  g_object_unref (gradient);

  g_object_set_data (G_OBJECT (gimp), id, gradient);

  return gradient;
}

static void
gimp_gradients_foreground_changed (GimpContext   *context,
                                   const GimpRGB *fg,
                                   Gimp          *gimp)
{
  GimpGradient *gradient;

  gradient = g_object_get_data (G_OBJECT (gimp), FG_BG_RGB_KEY);

  if (gradient)
    {
      gradient->segments->left_color = *fg;
      gimp_data_dirty (GIMP_DATA (gradient));
    }

  gradient = g_object_get_data (G_OBJECT (gimp), FG_BG_HSV_CCW_KEY);

  if (gradient)
    {
      gradient->segments->left_color = *fg;
      gimp_data_dirty (GIMP_DATA (gradient));
    }

  gradient = g_object_get_data (G_OBJECT (gimp), FG_BG_HSV_CW_KEY);

  if (gradient)
    {
      gradient->segments->left_color = *fg;
      gimp_data_dirty (GIMP_DATA (gradient));
    }

  gradient = g_object_get_data (G_OBJECT (gimp), FG_TRANSPARENT_KEY);

  if (gradient)
    {
      gradient->segments->left_color    = *fg;
      gradient->segments->right_color   = *fg;
      gradient->segments->right_color.a = 0.0;
      gimp_data_dirty (GIMP_DATA (gradient));
    }
}

static void
gimp_gradients_background_changed (GimpContext   *context,
                                   const GimpRGB *bg,
                                   Gimp          *gimp)
{
  GimpGradient *gradient;

  gradient = g_object_get_data (G_OBJECT (gimp), FG_BG_RGB_KEY);

  if (gradient)
    {
      gradient->segments->right_color = *bg;
      gimp_data_dirty (GIMP_DATA (gradient));
    }

  gradient = g_object_get_data (G_OBJECT (gimp), FG_BG_HSV_CCW_KEY);

  if (gradient)
    {
      gradient->segments->right_color = *bg;
      gimp_data_dirty (GIMP_DATA (gradient));
    }

  gradient = g_object_get_data (G_OBJECT (gimp), FG_BG_HSV_CW_KEY);

  if (gradient)
    {
      gradient->segments->right_color = *bg;
      gimp_data_dirty (GIMP_DATA (gradient));
    }
}
