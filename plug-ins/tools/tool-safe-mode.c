/* The GIMP -- an image manipulation program
 *
 * tool-safe-mode -- support plug-in for dynamically loaded tool modules
 * Copyright (C) 2000 Nathan Summers
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


#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <gtk/gtk.h>


#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "libgimptool/gimptoolmodule.h"


/* Declare local functions */
static void safe_mode_init (void);


GimpPlugInInfo PLUG_IN_INFO = {
  safe_mode_init,		/* init_proc */
  NULL,				/* query_proc  */
  NULL,				/* quit_proc  */
  NULL,				/* run_proc   */
};


MAIN ()

static void
safe_mode_register_tool ()
{
}


/* much code borrowed from gimp_datafiles_read_directories --
 * why isn't that function available in libgimp? It would be
 * better than the ad-hoc stuff in gimpressionist, gflare, etc.
 */
static void 
safe_mode_init (void)
{
  gchar *tool_plug_in_path;

  g_message ("tool-safe-mode init called");

  tool_plug_in_path = gimp_gimprc_query ("tool-plug-in-path");

  /* FIXME: why does it return the string with quotes around it?
   * gflare-path does not
   */

  tool_plug_in_path++;
  tool_plug_in_path[strlen(tool_plug_in_path)-1] = 0;
  g_message ("bah1");
  
  if (g_module_supported () && tool_plug_in_path) 
    {
      GList         *path;
      GList         *list;
      gchar         *filename;
      gint           err;
      GDir          *dir;
      const gchar   *dir_ent;
      struct stat    filestat;

  g_message ("bah2");

#ifdef __EMX__
  /*
   *  Change drive so opendir works.
   */
      if (tool_plug_in_path[1] == ':')
        {
          _chdrive (tool_plug_in_path[0]);
        }
#endif
  g_message ("%s", tool_plug_in_path);

      path = gimp_path_parse (tool_plug_in_path, 16, TRUE, NULL);
      g_message ("%p", path);

      for (list = path; list; list = g_list_next (list))
        {
          g_message("reading directory %s", list->data);
          /* Open directory */
          dir = g_dir_open ((gchar *) list->data, 0, NULL);

          if (!dir)
    	    {
   	      g_message ("error reading datafiles directory \"%s\"",
	    	     	 (gchar *) list->data);
	    }
          else
	    {
	      while ((dir_ent = g_dir_read_name (dir)))
	       {
	          filename = g_build_filename ((gchar *) list->data,
                                               dir_ent, NULL);

 	          /* Check the file and see that it is not a sub-directory */
	          err = stat (filename, &filestat);

	          if (! err && !S_ISDIR (filestat.st_mode))
	    	    {
	              g_message("loading tool %s\n", filename);
                      gimp_tool_module_new (filename, 
                                            safe_mode_register_tool,
                                            NULL);
		    }

	          g_free (filename);
	        }

	      g_dir_close (dir);
	    }
        }

      gimp_path_free (path);
      g_free (tool_plug_in_path);
   }

  g_message ("tool-safe-mode init done");
}





