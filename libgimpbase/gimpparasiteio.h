/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpparasiteio.h
 * Copyright (C) 1999 Tor Lillqvist <tml@iki.fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_PARASITE_IO_H__
#define __GIMP_PARASITE_IO_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


/*  Data structures for various standard parasites used by plug-ins and
 *  the GIMP core, and functions to build and parse their string
 *  representations.
 */

/*
 *  Pixmap brush pipes.
 */

#define GIMP_PIXPIPE_MAXDIM 4

typedef struct
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
  gboolean  free_selection_string;
} GimpPixPipeParams;

/* Initalize with dummy values */
void    gimp_pixpipe_params_init  (GimpPixPipeParams *params);

/* Parse a string into a GimpPixPipeParams */
void    gimp_pixpipe_params_parse (gchar             *parameters,
				   GimpPixPipeParams *params);

/* Build a string representation of GimpPixPipeParams */
gchar * gimp_pixpipe_params_build (GimpPixPipeParams *params);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_PARASITE_IO_H__ */
