/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * ligma-gradients.c
 * Copyright (C) 2002 Michael Natterer  <mitch@ligma.org>
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

#include "ligma.h"
#include "ligma-gradients.h"
#include "ligmacontext.h"
#include "ligmacontainer.h"
#include "ligmadatafactory.h"
#include "ligmagradient.h"

#include "ligma-intl.h"


#define CUSTOM_KEY         "ligma-gradient-custom"
#define FG_BG_RGB_KEY      "ligma-gradient-fg-bg-rgb"
#define FG_BG_HARDEDGE_KEY "ligma-gradient-fg-bg-rgb-hardedge"
#define FG_BG_HSV_CCW_KEY  "ligma-gradient-fg-bg-hsv-ccw"
#define FG_BG_HSV_CW_KEY   "ligma-gradient-fg-bg-hsv-cw"
#define FG_TRANSPARENT_KEY "ligma-gradient-fg-transparent"


/*  local function prototypes  */

static LigmaGradient * ligma_gradients_add_gradient (Ligma        *ligma,
                                                   const gchar *name,
                                                   const gchar *id);


/*  public functions  */

void
ligma_gradients_init (Ligma *ligma)
{
  LigmaGradient *gradient;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  /* Custom */
  gradient = ligma_gradients_add_gradient (ligma,
                                          _("Custom"),
                                          CUSTOM_KEY);
  g_object_set (gradient,
                "writable", TRUE,
                NULL);
  gradient->segments->left_color_type  = LIGMA_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = LIGMA_GRADIENT_COLOR_BACKGROUND;

  /* FG to BG (RGB) */
  gradient = ligma_gradients_add_gradient (ligma,
                                          _("FG to BG (RGB)"),
                                          FG_BG_RGB_KEY);
  gradient->segments->left_color_type  = LIGMA_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = LIGMA_GRADIENT_COLOR_BACKGROUND;
  ligma_context_set_gradient (ligma->user_context, gradient);

  /* FG to BG (Hardedge) */
  gradient = ligma_gradients_add_gradient (ligma,
                                          _("FG to BG (Hardedge)"),
                                          FG_BG_HARDEDGE_KEY);
  gradient->segments->left_color_type  = LIGMA_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = LIGMA_GRADIENT_COLOR_BACKGROUND;
  gradient->segments->type             = LIGMA_GRADIENT_SEGMENT_STEP;

  /* FG to BG (HSV counter-clockwise) */
  gradient = ligma_gradients_add_gradient (ligma,
                                          _("FG to BG (HSV counter-clockwise)"),
                                          FG_BG_HSV_CCW_KEY);
  gradient->segments->left_color_type  = LIGMA_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = LIGMA_GRADIENT_COLOR_BACKGROUND;
  gradient->segments->color            = LIGMA_GRADIENT_SEGMENT_HSV_CCW;

  /* FG to BG (HSV clockwise hue) */
  gradient = ligma_gradients_add_gradient (ligma,
                                          _("FG to BG (HSV clockwise hue)"),
                                          FG_BG_HSV_CW_KEY);
  gradient->segments->left_color_type  = LIGMA_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = LIGMA_GRADIENT_COLOR_BACKGROUND;
  gradient->segments->color            = LIGMA_GRADIENT_SEGMENT_HSV_CW;

  /* FG to Transparent */
  gradient = ligma_gradients_add_gradient (ligma,
                                          _("FG to Transparent"),
                                          FG_TRANSPARENT_KEY);
  gradient->segments->left_color_type  = LIGMA_GRADIENT_COLOR_FOREGROUND;
  gradient->segments->right_color_type = LIGMA_GRADIENT_COLOR_FOREGROUND_TRANSPARENT;
}

LigmaGradient *
ligma_gradients_get_custom (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return g_object_get_data (G_OBJECT (ligma), CUSTOM_KEY);
}

LigmaGradient *
ligma_gradients_get_fg_bg_rgb (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return g_object_get_data (G_OBJECT (ligma), FG_BG_RGB_KEY);
}

LigmaGradient *
ligma_gradients_get_fg_bg_hsv_ccw (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return g_object_get_data (G_OBJECT (ligma), FG_BG_HSV_CCW_KEY);
}

LigmaGradient *
ligma_gradients_get_fg_bg_hsv_cw (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return g_object_get_data (G_OBJECT (ligma), FG_BG_HSV_CW_KEY);
}

LigmaGradient *
ligma_gradients_get_fg_transparent (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return g_object_get_data (G_OBJECT (ligma), FG_TRANSPARENT_KEY);
}


/*  private functions  */

static LigmaGradient *
ligma_gradients_add_gradient (Ligma        *ligma,
                             const gchar *name,
                             const gchar *id)
{
  LigmaGradient *gradient;

  gradient = LIGMA_GRADIENT (ligma_gradient_new (ligma_get_user_context (ligma),
                                               name));

  ligma_data_make_internal (LIGMA_DATA (gradient), id);

  ligma_container_add (ligma_data_factory_get_container (ligma->gradient_factory),
                      LIGMA_OBJECT (gradient));
  g_object_unref (gradient);

  g_object_set_data (G_OBJECT (ligma), id, gradient);

  return gradient;
}
