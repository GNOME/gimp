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

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "tool-safe-mode.h"

#include "libgimp/stdplugins-intl.h"


static void
safe_mode_init (void)
{
  gchar *tool_plug_in_path;
  gchar *freeme;

  g_type_init ();

  freeme = tool_plug_in_path = gimp_gimprc_query ("tool-plug-in-path");

  /* FIXME: why does it return the string with quotes around it?
   * gflare-path does not
   *
   * It's a bug that will be fixed with the death of app/gimprc.c
   * --mitch
   */

  tool_plug_in_path++;
  tool_plug_in_path[strlen(tool_plug_in_path)-1] = 0;
  
  tool_safe_mode_init (tool_plug_in_path);

  g_free (freeme);
}


GimpPlugInInfo PLUG_IN_INFO =
{
  safe_mode_init,   /* init_proc  */
  NULL,             /* query_proc */
  NULL,             /* quit_proc  */
  NULL,             /* run_proc   */
};

MAIN ()
