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

#include "libgimpbase/gimpbase.h"
#include "libgimpproxy/gimpproxytypes.h"

#include "libgimptool/gimptooltypes.h"

#include "libgimptool/gimptoolmodule.h"

#include "libgimp/stdplugins-intl.h"


static void
safe_mode_register_tool ()
{
}


/* much code borrowed from gimp_datafiles_read_directories --
 * why isn't that function available in libgimp? It would be
 * better than the ad-hoc stuff in gimpressionist, gflare, etc.
 */
void
tool_safe_mode_init (const gchar *tool_plug_in_path)
{
  g_message ("tool-safe-mode init called");

  if (g_module_supported () && tool_plug_in_path) 
    {
      GList         *path;
      GList         *list;
      gchar         *filename;
      gint           err;
      GDir          *dir;
      const gchar   *dir_ent;
      struct stat    filestat;


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
   }

  g_message ("tool-safe-mode init done");
}
