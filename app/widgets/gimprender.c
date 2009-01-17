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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpviewable.h"

#include "gimprender.h"


static void   gimp_render_setup_notify (gpointer    config,
                                        GParamSpec *param_spec,
                                        Gimp       *gimp);


/*  accelerate blending on the checkerboard  */

guchar *gimp_render_check_buf         = NULL;
guchar *gimp_render_empty_buf         = NULL;
guchar *gimp_render_white_buf         = NULL;

guchar *gimp_render_blend_dark_check  = NULL;
guchar *gimp_render_blend_light_check = NULL;
guchar *gimp_render_blend_white       = NULL;


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

  if (gimp_render_blend_dark_check)
    {
      g_free (gimp_render_blend_dark_check);
      gimp_render_blend_dark_check = NULL;
    }

  if (gimp_render_blend_light_check)
    {
      g_free (gimp_render_blend_light_check);
      gimp_render_blend_light_check = NULL;
    }

  if (gimp_render_blend_white)
    {
      g_free (gimp_render_blend_white);
      gimp_render_blend_white = NULL;
    }

  if (gimp_render_check_buf)
    {
      g_free (gimp_render_check_buf);
      gimp_render_check_buf = NULL;
    }

  if (gimp_render_empty_buf)
    {
      g_free (gimp_render_empty_buf);
      gimp_render_empty_buf = NULL;
    }

  if (gimp_render_white_buf)
    {
      g_free (gimp_render_white_buf);
      gimp_render_white_buf = NULL;
    }
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
  gint          i, j;

  g_object_get (config,
                "transparency-type", &check_type,
                NULL);

  gimp_checks_get_shades (check_type, &light_check, &dark_check);

  gimp_rgba_set_uchar (&light, light_check, light_check, light_check, 255);
  gimp_rgba_set_uchar (&dark,  dark_check,  dark_check,  dark_check,  255);

  if (! gimp_render_blend_dark_check)
    gimp_render_blend_dark_check = g_new (guchar, 65536);

  if (! gimp_render_blend_light_check)
    gimp_render_blend_light_check = g_new (guchar, 65536);

  if (! gimp_render_blend_white)
    gimp_render_blend_white = g_new (guchar, 65536);

  for (i = 0; i < 256; i++)
    for (j = 0; j < 256; j++)
      {
        gimp_render_blend_dark_check [(i << 8) + j] =
          (guchar) ((j * i + dark_check * (255 - i)) / 255);

        gimp_render_blend_light_check [(i << 8) + j] =
          (guchar) ((j * i + light_check * (255 - i)) / 255);

        gimp_render_blend_white [(i << 8) + j] =
          (guchar) ((j * i + 255 * (255 - i)) / 255);
      }

  g_free (gimp_render_check_buf);
  g_free (gimp_render_empty_buf);
  g_free (gimp_render_white_buf);

#define BUF_SIZE (GIMP_VIEWABLE_MAX_PREVIEW_SIZE + 4)

  gimp_render_check_buf = g_new  (guchar, BUF_SIZE * 3);
  gimp_render_empty_buf = g_new0 (guchar, BUF_SIZE * 3);
  gimp_render_white_buf = g_new  (guchar, BUF_SIZE * 3);

  /*  calculate check buffer for previews  */

  memset (gimp_render_white_buf, 255, BUF_SIZE * 3);

  for (i = 0; i < BUF_SIZE; i++)
    {
      if (i & 0x4)
        {
          gimp_render_check_buf[i * 3 + 0] = dark_check;
          gimp_render_check_buf[i * 3 + 1] = dark_check;
          gimp_render_check_buf[i * 3 + 2] = dark_check;
        }
      else
        {
          gimp_render_check_buf[i * 3 + 0] = light_check;
          gimp_render_check_buf[i * 3 + 1] = light_check;
          gimp_render_check_buf[i * 3 + 2] = light_check;
        }
    }

#undef BUF_SIZE
}
