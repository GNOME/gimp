/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"

#include "widgets-types.h"

#include "core/ligma.h"

#include "ligmarender.h"


static void   ligma_render_setup_notify (gpointer    config,
                                        GParamSpec *param_spec,
                                        Ligma       *ligma);


static LigmaRGB color1;
static LigmaRGB color2;


void
ligma_render_init (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  g_signal_connect (ligma->config, "notify::transparency-type",
                    G_CALLBACK (ligma_render_setup_notify),
                    ligma);

  g_signal_connect (ligma->config, "notify::transparency-custom-color1",
                    G_CALLBACK (ligma_render_setup_notify),
                    ligma);

  g_signal_connect (ligma->config, "notify::transparency-custom-color2",
                    G_CALLBACK (ligma_render_setup_notify),
                    ligma);

  ligma_render_setup_notify (ligma->config, NULL, ligma);
}

void
ligma_render_exit (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  g_signal_handlers_disconnect_by_func (ligma->config,
                                        ligma_render_setup_notify,
                                        ligma);
}

const LigmaRGB *
ligma_render_check_color1 (void)
{
  return &color1;
}

const LigmaRGB *
ligma_render_check_color2 (void)
{
  return &color2;
}

static void
ligma_render_setup_notify (gpointer    config,
                          GParamSpec *param_spec,
                          Ligma       *ligma)
{
  LigmaRGB       *color1_custom;
  LigmaRGB       *color2_custom;
  LigmaCheckType  check_type;

  g_object_get (config,
                "transparency-type",          &check_type,
                "transparency-custom-color1", &color1_custom,
                "transparency-custom-color2", &color2_custom,
                NULL);

  color1 = *color1_custom;
  color2 = *color2_custom;
  ligma_checks_get_colors (check_type, &color1, &color2);
  g_free (color1_custom);
  g_free (color2_custom);
}
