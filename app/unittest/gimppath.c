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

void show_personal(const char *filename)
{
  char *full_filename = gimp_personal_rc_file(filename);
  
  g_print ("%-50s = gimp_personal_rc_file(\"%s\");\n",
	   full_filename, filename);
}

void show_personal_files(void)
{
  show_personal("devicerc");
  show_personal("ideas");
  show_personal("parasiterc");
  show_personal("gimprc");
  show_personal("unitrc");
  show_personal("menurc");
  show_personal("pluginrc");
  show_personal("sessionrc");
}

void show_gimp_dirs(void)
{
  g_print ( "%s %s\n", _("GIMP version"), GIMP_VERSION);    
  g_print ("%-50s = g_get_home_dir();\n",g_get_home_dir ());
  g_print ("%-50s = g_getenv(\"GIMP_DIRECTORY\");\n",g_getenv ("GIMP_DIRECTORY"));
  g_print ("%-50s = g_getenv(\"GIMP_DATADIR\");\n",g_getenv ("GIMP_DATADIR"));
  g_print ("%-50s = gimp_directory();\n",gimp_directory());
  g_print ("%-50s = gimp_data_directory();\n",gimp_data_directory());
  g_print ("%-50s = gimp_gtkrc();\n",gimp_gtkrc());
}

int
main (int argc, char **argv)
{
  show_gimp_dirs();
  show_personal_files();
}

