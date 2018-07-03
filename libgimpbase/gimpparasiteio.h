/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpparasiteio.h
 * Copyright (C) 1999 Tor Lillqvist <tml@iki.fi>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_PARASITE_IO_H__
#define __GIMP_PARASITE_IO_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*  Data structures for various standard parasites used by plug-ins and
 *  the GIMP core, and functions to build and parse their string
 *  representations.
 */

/*
 *  Pixmap brush pipes.
 */

#define GIMP_PIXPIPE_MAXDIM 4

typedef struct _GimpPixPipeParams GimpPixPipeParams;

struct _GimpPixPipeParams
{
  gint      step;
  gint      ncells;
  gint      dim;
  gint      cols;
  gint      rows;
  gint      cellwidth;
  gint      cellheight;
  gchar    *placement;
  gboolean  free_placement_string;
  gint      rank[GIMP_PIXPIPE_MAXDIM];
  gchar    *selection[GIMP_PIXPIPE_MAXDIM];
  /* this flag is now useless. All selection strings are allocated. */
  gboolean  free_selection_string;
};

/* Initialize with dummy values */
void    gimp_pixpipe_params_init  (GimpPixPipeParams *params);

/* Parse a string into a GimpPixPipeParams */
void    gimp_pixpipe_params_parse (const gchar       *parameters,
                                   GimpPixPipeParams *params);

/* Build a string representation of GimpPixPipeParams */
gchar * gimp_pixpipe_params_build (GimpPixPipeParams *params) G_GNUC_MALLOC;

/* Free the internal values. It does not free the struct itsef. */
void    gimp_pixpipe_params_free  (GimpPixPipeParams *params);

G_END_DECLS

#endif /* __GIMP_PARASITE_IO_H__ */
