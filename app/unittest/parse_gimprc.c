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

#if 0
gboolean
parse_absolute_gimprc_file (char *filename)
{
  int status;

  parse_info.fp = fopen (filename, "rt");
  if (!parse_info.fp)
    return FALSE;

  if ((be_verbose == TRUE) || (no_splash == TRUE))
    g_print (_("parsing \"%s\"\n"), filename);

  cur_token = -1;
  next_token = -1;

  parse_info.position = -1;
  parse_info.linenum = 1;
  parse_info.charnum = 1;
  parse_info.inc_linenum = FALSE;
  parse_info.inc_charnum = FALSE;

  done = FALSE;
  while ((status = parse_statement ()) == OK)
    ;

  fclose (parse_info.fp);

  if (status == ERROR)
    {
      g_print (_("error parsing: \"%s\"\n"), filename);
      g_print (_("  at line %d column %d\n"), parse_info.linenum, parse_info.charnum);
      g_print (_("  unexpected token: %s\n"), token_sym);

      return FALSE;
    }

  return TRUE;
}
#endif

int
main (int argc, char **argv)
{
    char libfilename[MAXPATHLEN];
    char filename[MAXPATHLEN];
    gboolean status;
    int showit = 1;
    
    if (argc > 1)
	showit = 0;

    global_parse_init(showit);

    if (argc == 1)
    {
	parse_get_system_file (alternate_system_gimprc, libfilename);
	parse_get_alt_personal_gimprc(alternate_gimprc, filename);
	printf("\n----- Parse files: \n");
	printf("libfilename= %s\n", libfilename);
	printf("filename= %s\n\n", filename);
	
	status = parse_absolute_gimprc_file (libfilename);
	printf("----- Parse Result:%d,filename=%s\n", status,libfilename);
	if (status)
	    parse_show_tokens (unknown_tokens);
	
	status = parse_absolute_gimprc_file (filename);
	printf("----- Parse Result:%d,filename=%s\n", status,filename);
	if (status)
	    parse_show_tokens (unknown_tokens);
    }
    else
    {
	parse_gimprc ();
	parse_show_tokens (unknown_tokens);
    }
}

