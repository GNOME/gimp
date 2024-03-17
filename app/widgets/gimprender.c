/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "widgets-types.h"

#include "core/gimp.h"

#include "gimprender.h"


static void   gimp_render_setup_notify (gpointer    config,
                                        GParamSpec *param_spec,
                                        Gimp       *gimp);


static GeglColor *color1 = NULL;
static GeglColor *color2 = NULL;


void
gimp_render_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  color1 = gegl_color_new (NULL);
  gegl_color_set_pixel (color1, babl_format ("R'G'B'A double"), GIMP_CHECKS_CUSTOM_COLOR1);
  color2 = gegl_color_new (NULL);
  gegl_color_set_pixel (color2, babl_format ("R'G'B'A double"), GIMP_CHECKS_CUSTOM_COLOR2);

  g_signal_connect (gimp->config, "notify::transparency-type",
                    G_CALLBACK (gimp_render_setup_notify),
                    gimp);

  g_signal_connect (gimp->config, "notify::transparency-custom-color1",
                    G_CALLBACK (gimp_render_setup_notify),
                    gimp);

  g_signal_connect (gimp->config, "notify::transparency-custom-color2",
                    G_CALLBACK (gimp_render_setup_notify),
                    gimp);

  gimp_render_setup_notify (gimp->config, NULL, gimp);
}

void
gimp_render_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_signal_handlers_disconnect_by_func (gimp->config,
                                        gimp_render_setup_notify,
                                        gimp);

  g_clear_object (&color1);
  g_clear_object (&color2);
}

const GeglColor *
gimp_render_check_color1 (void)
{
  return color1;
}

const GeglColor *
gimp_render_check_color2 (void)
{
  return color2;
}

static void
gimp_render_setup_notify (gpointer    config,
                          GParamSpec *param_spec,
                          Gimp       *gimp)
{
  GeglColor     *color1_custom = NULL;
  GeglColor     *color2_custom = NULL;
  GimpCheckType  check_type;

  g_object_get (config,
                "transparency-type",          &check_type,
                "transparency-custom-color1", &color1_custom,
                "transparency-custom-color2", &color2_custom,
                NULL);

  g_clear_object (&color1);
  g_clear_object (&color2);
  color1 = color1_custom;
  color2 = color2_custom;
  gimp_checks_get_colors (check_type, &color1, &color2);
  g_clear_object (&color1_custom);
  g_clear_object (&color2_custom);
}
