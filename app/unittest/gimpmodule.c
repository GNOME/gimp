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
#include "../module_db.c"

static gboolean
valid_module_name2 (const char *filename)
{
  const char *basename;
  int len;

  basename = g_basename (filename);

  len = strlen (basename);
  printf("basename=%s len=%d\n",basename,len);
  
#if !defined(G_OS_WIN32) && !defined(G_WITH_CYGWIN) && !defined(__EMX__)
  if (len < 3 + 1 + 3)
    return FALSE;

  if (strncmp (basename, "lib", 3))
    return FALSE;

  if (strcmp (basename + len - 3, ".so"))
    return FALSE;

#else
  if (len < 1 + 4)
      return FALSE;

  if (g_strcasecmp (basename + len - 4, ".dll"))
    return FALSE;
#endif

  return TRUE;
}

int
main (int argc, char **argv)
{
    char filename[300];
    gboolean status;
    int showit = 1;
    
    if (argc < 2)
    {
	printf("%s [option] \n\n",argv[0]);
	printf("ex: %s -s      # write modulerc \n",argv[0]);	
	printf("ex: %s -f%s      # parse a files\n",argv[0], filename);
	
	exit (1);
    }


/*    parse_buffers_init(); */
    if (strncmp (argv[1], "-s",2) == 0)
    {
	gboolean saved;
	printf("call  module_db_write_modulerc()\n");

	/*  init */
	modules = gimp_set_new (MODULE_INFO_TYPE, FALSE);
/*	need_to_rewrite_modulerc = TRUE; */
	saved = module_db_write_modulerc();
        printf("saved=%d\n",saved);	
	exit (0);
    }
    if (strncmp (argv[1], "-f",2) == 0)
    {
	strcpy (filename, argv[1] + 2);
	
	if (!valid_module_name2 (filename))
	{
	    printf("%s not valid module file !\n",filename);
/*	    exit (0); */
	}
	printf("call module_find_by_path()\n");
	/*  init */
	modules = gimp_set_new (MODULE_INFO_TYPE, FALSE);
	
	if (module_find_by_path (filename))
	{
	}
	module_initialize (filename);
	exit (0);
    }

    printf("caLL module_db_init()\n");    
    module_db_init();
} 
