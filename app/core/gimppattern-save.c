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

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "ligmapattern.h"
#include "ligmapattern-header.h"
#include "ligmapattern-save.h"
#include "ligmatempbuf.h"

#include "ligma-intl.h"


gboolean
ligma_pattern_save (LigmaData       *data,
                   GOutputStream  *output,
                   GError        **error)
{
  LigmaPattern       *pattern = LIGMA_PATTERN (data);
  LigmaTempBuf       *mask    = ligma_pattern_get_mask (pattern);
  const Babl        *format  = ligma_temp_buf_get_format (mask);
  LigmaPatternHeader  header;
  const gchar       *name;
  gint               width;
  gint               height;

  name   = ligma_object_get_name (pattern);
  width  = ligma_temp_buf_get_width  (mask);
  height = ligma_temp_buf_get_height (mask);

  if (width > LIGMA_PATTERN_MAX_SIZE || height > LIGMA_PATTERN_MAX_SIZE)
    {
      g_set_error (error, LIGMA_DATA_ERROR, LIGMA_DATA_ERROR_READ,
                   _("Unsupported pattern dimensions %d x %d.\n"
                     "LIGMA Patterns have a maximum size of %d x %d."),
                   width, height,
                   LIGMA_PATTERN_MAX_SIZE, LIGMA_PATTERN_MAX_SIZE);
      return FALSE;
    }
  header.header_size  = g_htonl (sizeof (LigmaPatternHeader) +
                                 strlen (name) + 1);
  header.version      = g_htonl (1);
  header.width        = g_htonl (width);
  header.height       = g_htonl (height);
  header.bytes        = g_htonl (babl_format_get_bytes_per_pixel (format));
  header.magic_number = g_htonl (LIGMA_PATTERN_MAGIC);

  if (! g_output_stream_write_all (output, &header, sizeof (header),
                                   NULL, NULL, error))
    {
      return FALSE;
    }

  if (! g_output_stream_write_all (output, name, strlen (name) + 1,
                                   NULL, NULL, error))
    {
      return FALSE;
    }

  if (! g_output_stream_write_all (output,
                                   ligma_temp_buf_get_data (mask),
                                   ligma_temp_buf_get_data_size (mask),
                                   NULL, NULL, error))
    {
      return FALSE;
    }

  return TRUE;
}
