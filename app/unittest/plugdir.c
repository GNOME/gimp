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

#include "unittest.h"
#include "globals.c"
#include "../plug_in.c"
#if 1
#include "../gimprc.c"
#endif

#define DPRINTF if (0) printf

static void
plug_in_init_file2 (char *filename)
{
  char *plug_in_name;
  char *name;

  name = strrchr (filename, G_DIR_SEPARATOR);
  if (name)
    name = name + 1;
  else
    name = filename;
 
  printf("%s\n", g_strdup (filename));
}

int
main (int argc, char **argv)
{
    if (argc < 2)
    {
	printf ("ex: %s %s/plug-ins \n",argv[0],gimp_directory());
	exit (0);
    }
    
  g_print ( "%s %s\n", _("GIMP version"), GIMP_VERSION);

  plug_in_path = argv[1];
  datafiles_read_directories (plug_in_path, plug_in_init_file2,
			      MODE_EXECUTABLE);
  return 0;
}
