/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gimp.h"
#include "gimpui.h"

void
gimp_ui_init (gchar    *prog_name,
	      gboolean  preview)
{
  gint    argc;
  gchar **argv;

  static gboolean initialized = FALSE;

  g_return_if_fail (prog_name != NULL);

  if (initialized)
    return;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup (prog_name);

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  /*  FIXME: check if gimp could allocate and attach smh and don't blindly
   *  use the commandline option
   */
  gdk_set_use_xshm (gimp_use_xshm ());

  /*  FIXME: check if gimp's GdkRGB subsystem actually installed a cmap
   *  and don't blindly use the gimprc option
   */
  if (gimp_install_cmap ())
    {
      gdk_rgb_set_install (TRUE);

      /*  or is this needed in all cases?  */
      gtk_widget_set_default_visual (gdk_rgb_get_visual ());
      gtk_widget_set_default_colormap (gdk_rgb_get_cmap ());
    }

  /*  Set the gamma after installing the colormap because
   *  gtk_preview_set_gamma() initializes GdkRGB if not already done
   */
  if (preview)
    gtk_preview_set_gamma (gimp_gamma ());

  initialized = TRUE;
}
