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

static void show_gimprc_globals()
{
    printf("%-20s=%s\n","plug_in_path",plug_in_path);
    printf("%-20s=%s\n","temp_path",temp_path);
    printf("%-20s=%s\n","brush_path",brush_path);
    printf("%-20s=%s\n","brush_vbr_path",brush_vbr_path);
    printf("%-20s=%s\n","default_brush",default_brush);
    printf("%-20s=%s\n","pattern_path",pattern_path);
    printf("%-20s=%s\n","default_pattern",default_pattern);
    printf("%-20s=%s\n","palette_path",palette_path);
    printf("%-20s=%s\n","default_palette",default_palette);
    printf("%-20s=%s\n","gradient_path",gradient_path);
    printf("%-20s=%s\n","default_gradient",default_gradient);
    printf("%-20s=%s\n","pluginrc_path",pluginrc_path);
    printf("%-20s=%s\n","module_path",module_path);
    printf("%-20s=%s\n","image_title_format",image_title_format); 
/*    printf("%-20s=%s\n","",); */
}

static void show_gimprc_funcs_tab()
{
    int i;
    int nfuncs = sizeof (funcs) / sizeof (funcs[0]);
	
    for (i = 0; i < nfuncs; i++)
    {
	printf("%2d: t:%02d %-25s =",i,funcs[i].type, funcs[i].name);
	switch (funcs[i].type)
        {
        case TT_STRING:
	case TT_PATH:
	{
	    char **pathp = funcs[i].val1p;
	    printf("%s", *pathp);
	    break;
	}
	case TT_DOUBLE:
	{
	    double *valp;
	    if (funcs[i].val1p == NULL)
		break;
	    valp =  funcs[i].val1p;
	    printf("%6.3f",*valp);
	    break;
	}
	case TT_BOOLEAN:
	{
	    int *valp = funcs[i].val1p;
	    if (valp == NULL)
		valp = funcs[i].val2p;
	    if (valp == NULL)
		break;
	    printf("%d",*valp);
	    break;
	}
	case TT_POSITION:
	{
	    int *valp;
	    int *valp2;
	    if (funcs[i].val1p == NULL)
		break;
	    if (funcs[i].val2p == NULL)
		break;
	    valp =  funcs[i].val1p;
	    valp2 =  funcs[i].val2p;
	    printf("%3d %3d",*valp, *valp2);
	    break;
	}
	case TT_XUNIT:
	case TT_IMAGETYPE:
	case TT_MEMSIZE:	
	case TT_INT:
	{
	    int *valp;
	    if (funcs[i].val1p == NULL)
		break;
	    valp =  funcs[i].val1p;
	    printf("%d",*valp);
	    break;
	}
	default:;
	};
	
	printf("\n");
    }
}

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

#if 0
    show_gimprc_globals();
#endif    
    show_gimprc_funcs_tab();
}

