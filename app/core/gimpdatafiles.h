/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Datafiles module copyight (C) 1996 Federico Mena Quintero
 * federico@nuclecu.unam.mx
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#ifndef _DATAFILES_H_
#define _DATAFILES_H_


#define INCLUDE_TEMP_DIR 0x1
#define MODE_EXECUTABLE  0x2


/***** Types *****/

typedef void (*datafile_loader_t) (char *filename);


/***** Functions *****/

void datafiles_read_directories(char *path_str,
				datafile_loader_t loader_func,
				int flags);

/* Return the current datafiles access, modification
 *  or change time. The current datafile is the one for
 *  which the "datafile_loader_t" function has been called
 *  on.
 */
time_t datafile_atime (void);
time_t datafile_mtime (void);
time_t datafile_ctime (void);


#endif
