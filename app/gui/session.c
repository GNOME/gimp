/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Session-managment stuff
 * Copyright (C) 1998 Sven Neumann <sven@gimp.org>
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

#include <stdio.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gui-types.h"

#include "core/gimp.h"

#include "widgets/gimpdialogfactory.h"

#include "color-notebook.h"
#include "session.h"

#include "gimprc.h"



/*  public functions  */

void
session_init (Gimp *gimp)
{
  gchar *filename;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  filename = gimp_personal_rc_file ("sessionrc");

  if (! gimprc_parse_file (filename))
    {
      /*  always show L&C&P, Tool Options and Brushes on first invocation  */

      /* TODO */
    }

  g_free (filename);
}

void
session_restore (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_dialog_factories_session_restore ();
}

void
session_save (Gimp *gimp)
{
  gchar *filename;
  FILE  *fp;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  filename = gimp_personal_rc_file ("sessionrc");

  fp = fopen (filename, "wt");
  g_free (filename);
  if (!fp)
    return;

  fprintf (fp, ("# GIMP sessionrc\n"
		"# This file takes session-specific info (that is info,\n"
		"# you want to keep between two gimp-sessions). You are\n"
		"# not supposed to edit it manually, but of course you\n"
		"# can do. This file will be entirely rewritten every time\n" 
		"# you quit the gimp. If this file isn't found, defaults\n"
		"# are used.\n\n"));

  gimp_dialog_factories_session_save (fp);

  /* save last tip shown */
  fprintf (fp, "(last-tip-shown %d)\n\n", gimprc.last_tip + 1);

  color_history_write (fp);

  fclose (fp);
}
