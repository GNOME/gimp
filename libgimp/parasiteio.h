/* parasite.h
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

#ifndef _PARASITEIO_H_
#define _PARASITEIO_H_

/* Data structures for various standard parasites used by plug-ins and
 * the GIMP core, and functions to build and parse their string
 * representations.
 */

/*
 * Pixmap brush pipes.
 */

#define PIXPIPE_MAXDIM 4

typedef struct {
  gint step;
  gint ncells;
  gint dim;
  gint cols;
  gint rows;
  gint cellwidth;
  gint cellheight;
  gchar *placement;
  gboolean free_placement_string;
  gint rank[PIXPIPE_MAXDIM];
  gchar *selection[PIXPIPE_MAXDIM];
  gboolean free_selection_string;
} PixPipeParams;

/* Initalize with dummy values */
void   pixpipeparams_init (PixPipeParams *params);

/* Parse a string into a PixPipeParams */
void   pixpipeparams_parse (gchar *parameters,
			    PixPipeParams *params);

/* Build a string representation of PixPipeParams */
gchar *pixpipeparams_build (PixPipeParams *params);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PARASITEIO_H_ */
