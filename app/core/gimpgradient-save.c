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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpgradient.h"
#include "gimpgradient-save.h"

#include "gimp-intl.h"


gboolean
gimp_gradient_save (GimpData       *data,
                    GOutputStream  *output,
                    GError        **error)
{
  GimpGradient        *gradient = GIMP_GRADIENT (data);
  GString             *string;
  GimpGradientSegment *seg;
  gint                 num_segments;

  /* File format is:
   *
   *   GIMP Gradient
   *   Name: name
   *   number_of_segments
   *   left middle right r0 g0 b0 a0 r1 g1 b1 a1 type coloring left_color_type
   *   left middle right r0 g0 b0 a0 r1 g1 b1 a1 type coloring right_color_type
   *   ...
   */

  string = g_string_new ("GIMP Gradient\n");

  g_string_append_printf (string, "Name: %s\n",
                          gimp_object_get_name (gradient));

  /* Count number of segments */
  num_segments = 0;
  seg          = gradient->segments;

  while (seg)
    {
      num_segments++;
      seg = seg->next;
    }

  /* Write rest of file */
  g_string_append_printf (string, "%d\n", num_segments);

  for (seg = gradient->segments; seg; seg = seg->next)
    {
      gchar buf[11][G_ASCII_DTOSTR_BUF_SIZE];

      g_ascii_dtostr (buf[0],  G_ASCII_DTOSTR_BUF_SIZE, seg->left);
      g_ascii_dtostr (buf[1],  G_ASCII_DTOSTR_BUF_SIZE, seg->middle);
      g_ascii_dtostr (buf[2],  G_ASCII_DTOSTR_BUF_SIZE, seg->right);
      g_ascii_dtostr (buf[3],  G_ASCII_DTOSTR_BUF_SIZE, seg->left_color.r);
      g_ascii_dtostr (buf[4],  G_ASCII_DTOSTR_BUF_SIZE, seg->left_color.g);
      g_ascii_dtostr (buf[5],  G_ASCII_DTOSTR_BUF_SIZE, seg->left_color.b);
      g_ascii_dtostr (buf[6],  G_ASCII_DTOSTR_BUF_SIZE, seg->left_color.a);
      g_ascii_dtostr (buf[7],  G_ASCII_DTOSTR_BUF_SIZE, seg->right_color.r);
      g_ascii_dtostr (buf[8],  G_ASCII_DTOSTR_BUF_SIZE, seg->right_color.g);
      g_ascii_dtostr (buf[9],  G_ASCII_DTOSTR_BUF_SIZE, seg->right_color.b);
      g_ascii_dtostr (buf[10], G_ASCII_DTOSTR_BUF_SIZE, seg->right_color.a);

      g_string_append_printf (string,
                              "%s %s %s %s %s %s %s %s %s %s %s %d %d %d %d\n",
                              buf[0], buf[1], buf[2],   /* left, middle, right */
                              buf[3], buf[4], buf[5], buf[6],  /* left color   */
                              buf[7], buf[8], buf[9], buf[10], /* right color  */
                              (gint) seg->type,
                              (gint) seg->color,
                              (gint) seg->left_color_type,
                              (gint) seg->right_color_type);
    }

  if (! g_output_stream_write_all (output, string->str, string->len,
                                   NULL, NULL, error))
    {
      g_string_free (string, TRUE);

      return FALSE;
    }

  g_string_free (string, TRUE);

  return TRUE;
}

gboolean
gimp_gradient_save_pov (GimpGradient  *gradient,
                        GFile         *file,
                        GError       **error)
{
  GOutputStream       *output;
  GString             *string;
  GimpGradientSegment *seg;
  gchar                buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar                color_buf[4][G_ASCII_DTOSTR_BUF_SIZE];
  GError              *my_error = NULL;

  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (! output)
    return FALSE;

  string = g_string_new ("/* color_map file created by GIMP */\n"
                         "/* https://www.gimp.org/          */\n"
                         "color_map {\n");

  for (seg = gradient->segments; seg; seg = seg->next)
    {
      /* Left */
      g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE,
                      seg->left);
      g_ascii_dtostr (color_buf[0], G_ASCII_DTOSTR_BUF_SIZE,
                      seg->left_color.r);
      g_ascii_dtostr (color_buf[1], G_ASCII_DTOSTR_BUF_SIZE,
                      seg->left_color.g);
      g_ascii_dtostr (color_buf[2], G_ASCII_DTOSTR_BUF_SIZE,
                      seg->left_color.b);
      g_ascii_dtostr (color_buf[3], G_ASCII_DTOSTR_BUF_SIZE,
                      1.0 - seg->left_color.a);

      g_string_append_printf (string,
                              "\t[%s color rgbt <%s, %s, %s, %s>]\n",
                              buf,
                              color_buf[0], color_buf[1],
                              color_buf[2], color_buf[3]);

      /* Middle */
      g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE,
                      seg->middle);
      g_ascii_dtostr (color_buf[0], G_ASCII_DTOSTR_BUF_SIZE,
                      (seg->left_color.r + seg->right_color.r) / 2.0);
      g_ascii_dtostr (color_buf[1], G_ASCII_DTOSTR_BUF_SIZE,
                      (seg->left_color.g + seg->right_color.g) / 2.0);
      g_ascii_dtostr (color_buf[2], G_ASCII_DTOSTR_BUF_SIZE,
                      (seg->left_color.b + seg->right_color.b) / 2.0);
      g_ascii_dtostr (color_buf[3], G_ASCII_DTOSTR_BUF_SIZE,
                      1.0 - (seg->left_color.a + seg->right_color.a) / 2.0);

      g_string_append_printf (string,
                              "\t[%s color rgbt <%s, %s, %s, %s>]\n",
                              buf,
                              color_buf[0], color_buf[1],
                              color_buf[2], color_buf[3]);

      /* Right */
      g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE,
                      seg->right);
      g_ascii_dtostr (color_buf[0], G_ASCII_DTOSTR_BUF_SIZE,
                      seg->right_color.r);
      g_ascii_dtostr (color_buf[1], G_ASCII_DTOSTR_BUF_SIZE,
                      seg->right_color.g);
      g_ascii_dtostr (color_buf[2], G_ASCII_DTOSTR_BUF_SIZE,
                      seg->right_color.b);
      g_ascii_dtostr (color_buf[3], G_ASCII_DTOSTR_BUF_SIZE,
                      1.0 - seg->right_color.a);

      g_string_append_printf (string,
                              "\t[%s color rgbt <%s, %s, %s, %s>]\n",
                              buf,
                              color_buf[0], color_buf[1],
                              color_buf[2], color_buf[3]);
    }

  g_string_append_printf (string, "} /* color_map */\n");

  if (! g_output_stream_write_all (output, string->str, string->len,
                                   NULL, NULL, &my_error))
    {
      GCancellable *cancellable = g_cancellable_new ();

      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_WRITE,
                   _("Writing POV file '%s' failed: %s"),
                   gimp_file_get_utf8_name (file),
                   my_error->message);
      g_clear_error (&my_error);
      g_string_free (string, TRUE);

      /* Cancel the overwrite initiated by g_file_replace(). */
      g_cancellable_cancel (cancellable);
      g_output_stream_close (output, cancellable, NULL);
      g_object_unref (cancellable);
      g_object_unref (output);

      return FALSE;
    }

  g_string_free (string, TRUE);
  g_object_unref (output);

  return TRUE;
}
