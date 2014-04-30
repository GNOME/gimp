/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * gimp-gradients.c
 * Copyright (C) 2002 Michael Natterer  <mitch@gimp.org>
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

#include <glib-object.h>

#include "core-types.h"

#include "gimp.h"
#include "gimp-gradients.h"
#include "gimpcontext.h"
#include "gimpcontainer.h"
#include "gimpdatafactory.h"
#include "gimpgradient.h"

#include "gimp-intl.h"


#define FG_BG_RGB_KEY      "gimp-gradient-fg-bg-rgb"
#define FG_BG_HARDEDGE_KEY "gimp-gradient-fg-bg-rgb-hardedge"
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

  /* FG to BG (RGB) */
  gradient = gimp_gradients_add_gradient (gimp,
                                          _("FG to BG (RGB)"),
                                          FG_BG_RGB_KEY);
  gradient->segments->left_color_type  = GIMP_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = GIMP_GRADIENT_COLOR_BACKGROUND;
  gimp_context_set_gradient (gimp->user_context, gradient);

  /* FG to BG (Hardedge) */
  gradient = gimp_gradients_add_gradient (gimp,
                                          _("FG to BG (Hardedge)"),
                                          FG_BG_HARDEDGE_KEY);
  gradient->segments->left                   = 0.00;
  gradient->segments->middle                 = 0.25;
  gradient->segments->right                  = 0.50;
  gradient->segments->left_color_type        = GIMP_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type       = GIMP_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->next                   = gimp_gradient_segment_new ();
  gradient->segments->next->prev             = gradient->segments;
  gradient->segments->next->left             = 0.50;
  gradient->segments->next->middle           = 0.75;
  gradient->segments->next->right            = 1.00;
  gradient->segments->next->left_color_type  = GIMP_GRADIENT_COLOR_BACKGROUND;
  gradient->segments->next->right_color_type = GIMP_GRADIENT_COLOR_BACKGROUND;

  /* FG to BG (HSV counter-clockwise) */
  gradient = gimp_gradients_add_gradient (gimp,
                                          _("FG to BG (HSV counter-clockwise)"),
                                          FG_BG_HSV_CCW_KEY);
  gradient->segments->left_color_type  = GIMP_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = GIMP_GRADIENT_COLOR_BACKGROUND;
  gradient->segments->color            = GIMP_GRADIENT_SEGMENT_HSV_CCW;

  /* FG to BG (HSV clockwise hue) */
  gradient = gimp_gradients_add_gradient (gimp,
                                          _("FG to BG (HSV clockwise hue)"),
                                          FG_BG_HSV_CW_KEY);
  gradient->segments->left_color_type  = GIMP_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = GIMP_GRADIENT_COLOR_BACKGROUND;
  gradient->segments->color            = GIMP_GRADIENT_SEGMENT_HSV_CW;

  /* FG to Transparent */
  gradient = gimp_gradients_add_gradient (gimp,
                                          _("FG to Transparent"),
                                          FG_TRANSPARENT_KEY);
  gradient->segments->left_color_type  = GIMP_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT;
}


/*  private functions  */

static GimpGradient *
gimp_gradients_add_gradient (Gimp        *gimp,
                             const gchar *name,
                             const gchar *id)
{
  GimpGradient *gradient;

  gradient = GIMP_GRADIENT (gimp_gradient_new (gimp_get_user_context (gimp),
                                               name));

  gimp_data_make_internal (GIMP_DATA (gradient), id);

  gimp_container_add (gimp_data_factory_get_container (gimp->gradient_factory),
                      GIMP_OBJECT (gradient));
  g_object_unref (gradient);

  g_object_set_data (G_OBJECT (gimp), id, gradient);

  return gradient;
}
