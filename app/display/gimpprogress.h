/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#ifndef __GIMPPROGRESS_H__
#define __GIMPPROGRESS_H__

#include "gdisplayF.h"


/* Progress bars for use internally by the main GIMP application. */


/* structures */
struct gimp_progress_pvt;

/* typedefs */
typedef struct gimp_progress_pvt gimp_progress;
typedef void (*progress_func_t) (int ymin, int ymax, int curr_y,
				 gpointer progress_data);

/* functions */
gimp_progress * progress_start (GDisplay *, const char *, gboolean,
				GtkSignalFunc, gpointer);
gimp_progress * progress_restart (gimp_progress *, const char *,
				  GtkSignalFunc, gpointer);
void            progress_update (gimp_progress *, float);
void            progress_step (gimp_progress *);
void            progress_end (gimp_progress *);

void            progress_update_and_flush (int, int, int, gpointer);


#endif /* __GIMPPROGRESS_H__ */
