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

#include <sys/types.h>

#include <gtk/gtk.h>

#include "apptypes.h"

#include "brushes.h"
#include "gimpbrushgenerated.h"
#include "gimpbrushpipe.h"
#include "brush_header.h"
#include "brush_select.h"
#include "datafiles.h"
#include "gimpcontext.h"
#include "gimprc.h"
#include "gimplist.h"
#include "gimpbrush.h"
#include "gimpdatalist.h"

#include "libgimp/gimpenv.h"

#include "libgimp/gimpintl.h"


/*  global variables  */
GimpList *global_brush_list = NULL;


/*  local function prototypes  */
static void   brushes_brush_load (const gchar   *filename);


/*  function declarations  */
void
brushes_init (gboolean no_data)
{
  if (global_brush_list)
    brushes_free ();
  else
    global_brush_list = GIMP_LIST (gimp_data_list_new (GIMP_TYPE_BRUSH));

  if (brush_path != NULL && !no_data)
    {
      brush_select_freeze_all ();

      datafiles_read_directories (brush_path, brushes_brush_load, 0);
      datafiles_read_directories (brush_vbr_path, brushes_brush_load, 0);

      brush_select_thaw_all ();
    }

  gimp_context_refresh_brushes ();
}

GimpBrush *
brushes_get_standard_brush (void)
{
  static GimpBrush *standard_brush = NULL;

  if (! standard_brush)
    {
      standard_brush =
	GIMP_BRUSH (gimp_brush_generated_new (5.0, 0.5, 0.0, 1.0));

      gimp_object_set_name (GIMP_OBJECT (standard_brush), "Standard");

      /*  set ref_count to 2 --> never swap the standard brush  */
      gtk_object_ref (GTK_OBJECT (standard_brush));
      gtk_object_ref (GTK_OBJECT (standard_brush));
      gtk_object_sink (GTK_OBJECT (standard_brush));
    }

  return standard_brush;
}

static void
brushes_brush_load (const gchar *filename)
{
  GimpBrush *brush = NULL;

  if (strcmp (&filename[strlen (filename) - 4], ".gbr") == 0 ||
      strcmp (&filename[strlen (filename) - 4], ".gpb") == 0)
    {
      brush = gimp_brush_load (filename);

      if (! brush)
	g_message (_("Warning: Failed to load brush\n\"%s\""), filename);
    }
  else if (strcmp (&filename[strlen(filename) - 4], ".vbr") == 0)
    {
      brush = gimp_brush_generated_load (filename);

      if (! brush)
	g_message (_("Warning: Failed to load brush\n\"%s\""), filename);
    }
  else if (strcmp (&filename[strlen (filename) - 4], ".gih") == 0)
    {
      brush = gimp_brush_pipe_load (filename);

      if (! brush)
	g_message (_("Warning: Failed to load brush pipe\n\"%s\""), filename);
    }

  if (brush != NULL)
    gimp_container_add (GIMP_CONTAINER (global_brush_list),
			GIMP_OBJECT (brush));
}

void
brushes_free (void)
{
  GList *vbr_path;
  gchar *vbr_dir;

  if (! global_brush_list)
    return;

  vbr_path = gimp_path_parse (brush_vbr_path, 16, TRUE, NULL);
  vbr_dir  = gimp_path_get_user_writable_dir (vbr_path);
  gimp_path_free (vbr_path);

  brush_select_freeze_all ();

  while (GIMP_LIST (global_brush_list)->list)
    {
      GimpBrush *brush = GIMP_BRUSH (GIMP_LIST (global_brush_list)->list->data);

      if (GIMP_IS_BRUSH_GENERATED (brush) && vbr_dir)
	gimp_brush_generated_save (GIMP_BRUSH_GENERATED (brush), vbr_dir);

      gimp_container_remove (GIMP_CONTAINER (global_brush_list),
			     GIMP_OBJECT (brush));
    }

  brush_select_thaw_all ();

  g_free (vbr_dir);
}
