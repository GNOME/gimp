/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Adrian Likins and Tor Lillqvist
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <glib-object.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpparasiteio.h"

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimpbrush-load.h"
#include "gimpbrushpipe.h"
#include "gimpbrushpipe-load.h"

#include "gimp-intl.h"


GList *
gimp_brush_pipe_load (const gchar  *filename,
                      GError      **error)
{
  GimpBrushPipe     *pipe = NULL;
  GimpPixPipeParams  params;
  gint               i;
  gint               num_of_brushes = 0;
  gint               totalcells;
  gchar             *paramstring;
  GString           *buffer;
  gchar              c;
  gint               fd;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  fd = g_open (filename, O_RDONLY | _O_BINARY, 0);

  if (fd == -1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  /* The file format starts with a painfully simple text header */

  /*  get the name  */
  buffer = g_string_new (NULL);
  while (read (fd, &c, 1) == 1 && c != '\n' && buffer->len < 1024)
    g_string_append_c (buffer, c);

  if (buffer->len > 0 && buffer->len < 1024)
    {
      gchar *utf8 =
        gimp_any_to_utf8 (buffer->str, buffer->len,
                          _("Invalid UTF-8 string in brush file '%s'."),
                          gimp_filename_to_utf8 (filename));

      pipe = g_object_new (GIMP_TYPE_BRUSH_PIPE,
                           "name",      utf8,
                           "mime-type", "image/x-gimp-gih",
                           NULL);

      g_free (utf8);
    }

  g_string_free (buffer, TRUE);

  if (! pipe)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "File is corrupt."),
                   gimp_filename_to_utf8 (filename));
      close (fd);
      return NULL;
    }

  /*  get the number of brushes  */
  buffer = g_string_new (NULL);
  while (read (fd, &c, 1) == 1 && c != '\n' && buffer->len < 1024)
    g_string_append_c (buffer, c);

  if (buffer->len > 0 && buffer->len < 1024)
    {
      num_of_brushes = strtol (buffer->str, &paramstring, 10);
    }

  if (num_of_brushes < 1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "File is corrupt."),
                   gimp_filename_to_utf8 (filename));
      close (fd);
      g_object_unref (pipe);
      g_string_free (buffer, TRUE);
      return NULL;
    }

  while (*paramstring && g_ascii_isspace (*paramstring))
    paramstring++;

  if (*paramstring)
    {
      gimp_pixpipe_params_init (&params);
      gimp_pixpipe_params_parse (paramstring, &params);

      pipe->dimension = params.dim;
      pipe->rank      = g_new0 (gint, pipe->dimension);
      pipe->select    = g_new0 (PipeSelectModes, pipe->dimension);
      pipe->index     = g_new0 (gint, pipe->dimension);

      /* placement is not used at all ?? */
      if (params.free_placement_string)
        g_free (params.placement);

      for (i = 0; i < pipe->dimension; i++)
        {
          pipe->rank[i] = MAX (1, params.rank[i]);
          if (strcmp (params.selection[i], "incremental") == 0)
            pipe->select[i] = PIPE_SELECT_INCREMENTAL;
          else if (strcmp (params.selection[i], "angular") == 0)
            pipe->select[i] = PIPE_SELECT_ANGULAR;
          else if (strcmp (params.selection[i], "velocity") == 0)
            pipe->select[i] = PIPE_SELECT_VELOCITY;
          else if (strcmp (params.selection[i], "random") == 0)
            pipe->select[i] = PIPE_SELECT_RANDOM;
          else if (strcmp (params.selection[i], "pressure") == 0)
            pipe->select[i] = PIPE_SELECT_PRESSURE;
          else if (strcmp (params.selection[i], "xtilt") == 0)
            pipe->select[i] = PIPE_SELECT_TILT_X;
          else if (strcmp (params.selection[i], "ytilt") == 0)
            pipe->select[i] = PIPE_SELECT_TILT_Y;
          else
            pipe->select[i] = PIPE_SELECT_CONSTANT;
          if (params.free_selection_string)
            g_free (params.selection[i]);
          pipe->index[i] = 0;
        }
    }
  else
    {
      pipe->dimension = 1;
      pipe->rank      = g_new (gint, 1);
      pipe->rank[0]   = num_of_brushes;
      pipe->select    = g_new (PipeSelectModes, 1);
      pipe->select[0] = PIPE_SELECT_INCREMENTAL;
      pipe->index     = g_new (gint, 1);
      pipe->index[0]  = 0;
    }

  g_string_free (buffer, TRUE);

  totalcells = 1;                /* Not all necessarily present, maybe */
  for (i = 0; i < pipe->dimension; i++)
    totalcells *= pipe->rank[i];
  pipe->stride = g_new0 (gint, pipe->dimension);
  for (i = 0; i < pipe->dimension; i++)
    {
      if (i == 0)
        pipe->stride[i] = totalcells / pipe->rank[i];
      else
        pipe->stride[i] = pipe->stride[i-1] / pipe->rank[i];
    }
  g_assert (pipe->stride[pipe->dimension-1] == 1);

  pipe->brushes = g_new0 (GimpBrush *, num_of_brushes);

  while (pipe->nbrushes < num_of_brushes)
    {
      pipe->brushes[pipe->nbrushes] = gimp_brush_load_brush (fd, filename, NULL);

      if (pipe->brushes[pipe->nbrushes])
        {
          gimp_object_set_name (GIMP_OBJECT (pipe->brushes[pipe->nbrushes]),
                                NULL);
        }
      else
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Fatal parse error in brush file '%s': "
                         "File is corrupt."),
                       gimp_filename_to_utf8 (filename));
          close (fd);
          g_object_unref (pipe);
          return NULL;
        }

      pipe->nbrushes++;
    }

  close (fd);

  /* Current brush is the first one. */
  pipe->current = pipe->brushes[0];

  /*  just to satisfy the code that relies on this crap  */
  GIMP_BRUSH (pipe)->spacing  = pipe->current->spacing;
  GIMP_BRUSH (pipe)->x_axis   = pipe->current->x_axis;
  GIMP_BRUSH (pipe)->y_axis   = pipe->current->y_axis;
  GIMP_BRUSH (pipe)->mask     = pipe->current->mask;
  GIMP_BRUSH (pipe)->pixmap   = pipe->current->pixmap;

  return g_list_prepend (NULL, pipe);
}
