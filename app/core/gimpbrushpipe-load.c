/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Adrian Likins and Tor Lillqvist
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

#include <stdlib.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligmabrush-load.h"
#include "ligmabrush-private.h"
#include "ligmabrushpipe.h"
#include "ligmabrushpipe-load.h"

#include "ligma-intl.h"


GList *
ligma_brush_pipe_load (LigmaContext   *context,
                      GFile         *file,
                      GInputStream  *input,
                      GError       **error)
{
  LigmaBrushPipe *pipe      = NULL;
  gint           n_brushes = 0;
  GString       *buffer;
  gchar         *paramstring;
  gchar          c;
  gsize          bytes_read;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /* The file format starts with a painfully simple text header */

  /*  get the name  */
  buffer = g_string_new (NULL);
  while (g_input_stream_read_all (input, &c, 1, &bytes_read, NULL, NULL) &&
         bytes_read == 1 &&
         c != '\n'       &&
         buffer->len < 1024)
    {
      g_string_append_c (buffer, c);
    }

  if (buffer->len > 0 && buffer->len < 1024)
    {
      gchar *utf8 =
        ligma_any_to_utf8 (buffer->str, buffer->len,
                          _("Invalid UTF-8 string in brush file '%s'."),
                          ligma_file_get_utf8_name (file));

      pipe = g_object_new (LIGMA_TYPE_BRUSH_PIPE,
                           "name",      utf8,
                           "mime-type", "image/x-ligma-gih",
                           NULL);

      g_free (utf8);
    }

  g_string_free (buffer, TRUE);

  if (! pipe)
    {
      g_set_error (error, LIGMA_DATA_ERROR, LIGMA_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "File is corrupt."),
                   ligma_file_get_utf8_name (file));
      return NULL;
    }

  /*  get the number of brushes  */
  buffer = g_string_new (NULL);
  while (g_input_stream_read_all (input, &c, 1, &bytes_read, NULL, NULL) &&
         bytes_read == 1 &&
         c != '\n'       &&
         buffer->len < 1024)
    {
      g_string_append_c (buffer, c);
    }

  if (buffer->len > 0 && buffer->len < 1024)
    {
      n_brushes = strtol (buffer->str, &paramstring, 10);
    }

  if (n_brushes < 1)
    {
      g_set_error (error, LIGMA_DATA_ERROR, LIGMA_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "File is corrupt."),
                   ligma_file_get_utf8_name (file));
      g_object_unref (pipe);
      g_string_free (buffer, TRUE);
      return NULL;
    }

  while (*paramstring && g_ascii_isspace (*paramstring))
    paramstring++;

  pipe->brushes = g_new0 (LigmaBrush *, n_brushes);

  while (pipe->n_brushes < n_brushes)
    {
      pipe->brushes[pipe->n_brushes] = ligma_brush_load_brush (context,
                                                              file, input,
                                                              error);

      if (! pipe->brushes[pipe->n_brushes])
        {
          g_object_unref (pipe);
          g_string_free (buffer, TRUE);
          return NULL;
        }

      pipe->n_brushes++;
    }

  if (! ligma_brush_pipe_set_params (pipe, paramstring))
    {
      g_set_error (error, LIGMA_DATA_ERROR, LIGMA_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "Inconsistent parameters."),
                   ligma_file_get_utf8_name (file));
      g_object_unref (pipe);
      g_string_free (buffer, TRUE);
      return NULL;
    }

  g_string_free (buffer, TRUE);

  /* Current brush is the first one. */
  pipe->current = pipe->brushes[0];

  /*  just to satisfy the code that relies on this crap  */
  LIGMA_BRUSH (pipe)->priv->spacing  = pipe->current->priv->spacing;
  LIGMA_BRUSH (pipe)->priv->x_axis   = pipe->current->priv->x_axis;
  LIGMA_BRUSH (pipe)->priv->y_axis   = pipe->current->priv->y_axis;
  LIGMA_BRUSH (pipe)->priv->mask     = pipe->current->priv->mask;
  LIGMA_BRUSH (pipe)->priv->pixmap   = pipe->current->priv->pixmap;

  return g_list_prepend (NULL, pipe);
}
