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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gimp.h"
#include "gimp-gradients.h"
#include "gimpcontext.h"
#include "gimpcontainer.h"
#include "gimpdatafactory.h"
#include "gimpgradient.h"

#include "gimp-intl.h"


#define CUSTOM_KEY                  "gimp-gradient-custom"
#define FG_BG_RGB_KEY               "gimp-gradient-fg-bg-rgb"
#define FG_BG_HARDEDGE_KEY          "gimp-gradient-fg-bg-rgb-hardedge"
#define FG_BG_HSV_CCW_KEY           "gimp-gradient-fg-bg-hsv-ccw"
#define FG_BG_HSV_CW_KEY            "gimp-gradient-fg-bg-hsv-cw"
#define FG_TRANSPARENT_KEY          "gimp-gradient-fg-transparent"
#define FG_TRANSPARENT_HARDEDGE_KEY "gimp-gradient-fg-transparent-hardedge"


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

  /* Custom */
  gradient = gimp_gradients_add_gradient (gimp,
                                          _("Custom"),
                                          CUSTOM_KEY);
  g_object_set (gradient,
                "writable", TRUE,
                NULL);
  gradient->segments->left_color_type  = GIMP_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = GIMP_GRADIENT_COLOR_BACKGROUND;

  /* FG to BG (RGB) */
  gradient = gimp_gradients_add_gradient (gimp,
                                          _("FG to BG (RGB)"),
                                          FG_BG_RGB_KEY);
  gradient->segments->left_color_type  = GIMP_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = GIMP_GRADIENT_COLOR_BACKGROUND;
  gimp_context_set_gradient (gimp->user_context, gradient);

  /* FG to BG (Hard Edge) */
  gradient = gimp_gradients_add_gradient (gimp,
                                          _("FG to BG (Hard Edge)"),
                                          FG_BG_HARDEDGE_KEY);
  gradient->segments->left_color_type  = GIMP_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = GIMP_GRADIENT_COLOR_BACKGROUND;
  gradient->segments->type             = GIMP_GRADIENT_SEGMENT_STEP;

  /* FG to BG (HSV Counter-Clockwise) */
  gradient = gimp_gradients_add_gradient (gimp,
                                          _("FG to BG (HSV Counter-Clockwise)"),
                                          FG_BG_HSV_CCW_KEY);
  gradient->segments->left_color_type  = GIMP_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = GIMP_GRADIENT_COLOR_BACKGROUND;
  gradient->segments->color            = GIMP_GRADIENT_SEGMENT_HSV_CCW;

  /* FG to BG (HSV Clockwise Hue) */
  gradient = gimp_gradients_add_gradient (gimp,
                                          _("FG to BG (HSV Clockwise Hue)"),
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

  /* FG to Transparent (Hard Edge) */
  gradient = gimp_gradients_add_gradient (gimp,
                                          _("FG to Transparent (Hard Edge)"),
                                          FG_TRANSPARENT_HARDEDGE_KEY);
  gradient->segments->left_color_type  = GIMP_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT;
  gradient->segments->type             = GIMP_GRADIENT_SEGMENT_STEP;
}

GimpGradient *
gimp_gradients_get_custom (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_get_data (G_OBJECT (gimp), CUSTOM_KEY);
}

GimpGradient *
gimp_gradients_get_fg_bg_rgb (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_get_data (G_OBJECT (gimp), FG_BG_RGB_KEY);
}

GimpGradient *
gimp_gradients_get_fg_bg_hsv_ccw (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_get_data (G_OBJECT (gimp), FG_BG_HSV_CCW_KEY);
}

GimpGradient *
gimp_gradients_get_fg_bg_hsv_cw (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_get_data (G_OBJECT (gimp), FG_BG_HSV_CW_KEY);
}

GimpGradient *
gimp_gradients_get_fg_transparent (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_get_data (G_OBJECT (gimp), FG_TRANSPARENT_KEY);
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
