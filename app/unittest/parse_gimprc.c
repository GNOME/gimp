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
#include "../gimprc.c"

static void
parse_show_tokens (GList *list)
{
  UnknownToken *ut;

  while (list)
    {
      ut = (UnknownToken *) list->data;
      printf("value=%10d token=%s\n",ut->value,ut->token);
      list = list->next;
    }
}

static void global_parse_init(int showit)
{
  GList *list;
  list = parse_add_directory_tokens();
  
  if (showit)
  {
      printf("----- Directory Tokens:\n");
      parse_show_tokens (list);
  }
  parse_buffers_init();
/*  next_token = -1;*/
}

static void parse_get_absolute_gimprc_file (char *filename)
{
  char rfilename[MAXPATHLEN];

  if (!g_path_is_absolute (filename))
    {
      if (g_get_home_dir () != NULL)
	{
	  g_snprintf (rfilename, sizeof (rfilename),
		      "%s" G_DIR_SEPARATOR_S "%s",
		      g_get_home_dir (), filename);
	  strcpy (filename,rfilename);
	}
    }
}

static void parse_get_system_file(char *alternate, char *libfilename)
{
    strcpy (libfilename, gimp_system_rc_file ());
    if (alternate != NULL) 
	strncpy (libfilename, alternate, MAXPATHLEN);
    parse_get_absolute_gimprc_file(libfilename);
}

static void parse_get_alt_personal_gimprc(char *alternate, char *filename)
{
    if (alternate != NULL) 
	strncpy (filename, alternate, MAXPATHLEN);
    else 
	strncpy (filename, gimp_personal_rc_file ("gimprc"), MAXPATHLEN);
    parse_get_absolute_gimprc_file(filename);
}

static gboolean p_asb_show_result(char *filename)
{
    gboolean status;
    
    status = parse_absolute_gimprc_file (filename);
    printf("----- Parse Result:%d,filename=%s\n", status,filename);
    if (status)
	parse_show_tokens (unknown_tokens);
    return (status);
}

static void p_show_default_files ()
{ 
    char libfilename[MAXPATHLEN];
    char filename[MAXPATHLEN];

    parse_get_system_file (alternate_system_gimprc, libfilename);
    parse_get_alt_personal_gimprc (alternate_gimprc, filename);
    printf("\n----- Parse files: \n");
    printf("libfilename= %s\n", libfilename);
    printf("filename= %s\n\n", filename);
}

int
main (int argc, char **argv)
{
    char libfilename[MAXPATHLEN];
    char filename[MAXPATHLEN];
    gboolean status;
    int showit = 1;
    
    if (argc < 2)
    {
	printf("%s [option] \n\n",argv[0]);
	printf("ex: %s -a      # call alternative gimprc parsing\n",argv[0]);
	printf("ex: %s -p      # call parse_gimprc ()\n",argv[0]);
	printf("ex: %s -h      # show default files\n",argv[0]);
	
	parse_get_system_file (alternate_system_gimprc, libfilename);
	printf("ex: %s -f%s      # parse a files\n",argv[0], libfilename);
	
	exit (1);
    }

    if (strcmp (argv[1], "-h") == 0)
    {
	p_show_default_files ();
	exit (0);
    }
    
    if (strcmp (argv[1], "-p") == 0)
	showit = 0;

    global_parse_init(showit);

    if (strcmp (argv[1], "-a") == 0)
    {
	p_show_default_files ();

	parse_get_system_file (alternate_system_gimprc, libfilename);
	parse_get_alt_personal_gimprc(alternate_gimprc, filename);

	status = p_asb_show_result(libfilename);	
	status = p_asb_show_result(filename);
    }
    else if (strncmp (argv[1], "-f",2) == 0)
    {
	strcpy (filename, argv[1] + 2);
	status = p_asb_show_result(filename);
    }
    else if (strcmp (argv[1], "-p") == 0)
    {
	parse_gimprc ();
	parse_show_tokens (unknown_tokens);
    }
}

