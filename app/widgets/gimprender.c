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


static GimpRGB light;
static GimpRGB dark;


void
gimp_render_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_signal_connect (gimp->config, "notify::transparency-type",
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
}

const GimpRGB *
gimp_render_light_check_color (void)
{
  return &light;
}

const GimpRGB *
gimp_render_dark_check_color (void)
{
  return &dark;
}

static void
gimp_render_setup_notify (gpointer    config,
                          GParamSpec *param_spec,
                          Gimp       *gimp)
{
  GimpCheckType check_type;
  guchar        dark_check;
  guchar        light_check;

  g_object_get (config,
                "transparency-type", &check_type,
                NULL);

  gimp_checks_get_shades (check_type, &light_check, &dark_check);

  gimp_rgba_set_uchar (&light, light_check, light_check, light_check, 255);
  gimp_rgba_set_uchar (&dark,  dark_check,  dark_check,  dark_check,  255);
}
