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

#include "config.h"

#include <gtk/gtk.h>

#include "plug-in-types.h"

#ifdef __GNUC__
#warning FIXME #include "display/display-types.h"
#endif
#include "display/display-types.h"

#include "pdb/procedural_db.h"

#include "display/gimpdisplay.h"
#include "display/gimpprogress.h"

#include "plug-in.h"
#include "plug-in-progress.h"


/*  local function prototypes  */

static void   plug_in_progress_cancel (GtkWidget *widget,
                                       PlugIn    *plug_in);


/*  public functions  */

void
plug_in_progress_start (PlugIn      *plug_in,
                        const gchar *message,
                        gint         gdisp_ID)
{
  GimpDisplay *gdisp = NULL;

  g_return_if_fail (plug_in != NULL);

  if (! message)
    message = plug_in->prog;

  if (gdisp_ID > 0)
    gdisp = gimp_display_get_by_ID (plug_in->gimp, gdisp_ID);

  if (plug_in->progress)
    plug_in->progress = gimp_progress_restart (plug_in->progress, message,
                                               G_CALLBACK (plug_in_progress_cancel),
                                               plug_in);
  else
    plug_in->progress = gimp_progress_start (gdisp, message, TRUE,
                                             G_CALLBACK (plug_in_progress_cancel), 
                                             plug_in);
}

void
plug_in_progress_update (PlugIn  *plug_in,
			 gdouble  percentage)
{
  g_return_if_fail (plug_in != NULL);

  if (! plug_in->progress)
    plug_in_progress_start (plug_in, NULL, -1);

  gimp_progress_update (plug_in->progress, percentage);
}

void
plug_in_progress_end (PlugIn *plug_in)
{
  g_return_if_fail (plug_in != NULL);

  if (plug_in->progress)
    {
      gimp_progress_end (plug_in->progress);
      plug_in->progress = NULL;
    }
}


/*  private functions  */

static void
plug_in_progress_cancel (GtkWidget *widget,
			 PlugIn    *plug_in)
{
  if (plug_in->recurse)
    {
      plug_in->return_vals   = g_new (Argument, 1);
      plug_in->n_return_vals = 1;

      plug_in->return_vals->arg_type      = GIMP_PDB_STATUS;
      plug_in->return_vals->value.pdb_int = GIMP_PDB_CANCEL;
    }

  plug_in_close (plug_in, TRUE);
  plug_in_unref (plug_in);
}
