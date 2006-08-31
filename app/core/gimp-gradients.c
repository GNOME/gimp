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

#include "core-types.h"

#include "gimp.h"
#include "gimpcontainer.h"
#include "gimpdatafactory.h"
#include "gimpgradient.h"

#include "gimp-intl.h"


#define FG_BG_RGB_KEY      "gimp-gradient-fg-bg-rgb"
#define FG_BG_HSV_CCW_KEY  "gimp-gradient-fg-bg-hsv-ccw"
#define FG_BG_HSV_CW_KEY   "gimp-gradient-fg-bg-hsv-cw"
#define FG_TRANSPARENT_KEY "gimp-gradient-fg-transparent"


/*  local function prototypes  */

static GimpGradient * gimp_gradients_add_gradient (Gimp        *gimp,
                                                   const gchar *name,
                                                   const gchar *id);


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
  gradient->segments->right_color_type = GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT;
}


/*  private functions  */

static GimpGradient *
gimp_gradients_add_gradient (Gimp        *gimp,
                             const gchar *name,
                             const gchar *id)
{
  GimpGradient *gradient = GIMP_GRADIENT (gimp_gradient_new (name));

  gimp_data_make_internal (GIMP_DATA (gradient));

  gradient->segments->left_color_type  = GIMP_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = GIMP_GRADIENT_COLOR_BACKGROUND;

  gimp_container_add (gimp->gradient_factory->container,
                      GIMP_OBJECT (gradient));
  g_object_unref (gradient);

  g_object_set_data (G_OBJECT (gimp), id, gradient);

  return gradient;
}
