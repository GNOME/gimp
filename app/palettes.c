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

#include "apptypes.h"

#include "datafiles.h"
#include "gimpcontext.h"
#include "gimpdatalist.h"
#include "gimppalette.h"
#include "gimprc.h"
#include "palette.h"
#include "palette_select.h"
#include "palettes.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */
static void   palettes_load_palette (const gchar *filename,
				     gpointer     loader_data);


/*  global variables  */
GimpContainer *global_palette_list = NULL;


/*  public functions  */

void
palettes_init (gboolean no_data)
{
  if (global_palette_list)
    palettes_free ();
  else
    global_palette_list =
      GIMP_CONTAINER (gimp_data_list_new (GIMP_TYPE_PALETTE));

  if (palette_path != NULL && !no_data)
    {
      palette_select_freeze_all ();

      datafiles_read_directories (palette_path, 0,
				  palettes_load_palette, global_palette_list);

      palette_select_thaw_all ();
    }

  gimp_context_refresh_palettes ();
}

void
palettes_free (void)
{
  if (! global_palette_list)
    return;

  palette_select_freeze_all ();

  gimp_data_list_save_and_clear (GIMP_DATA_LIST (global_palette_list),
				 palette_path,
				 GIMP_PALETTE_FILE_EXTENSION);

  palette_select_thaw_all ();
}

GimpPalette *
palettes_get_standard_palette (void)
{
  static GimpPalette *standard_palette = NULL;

  if (! standard_palette)
    {
      standard_palette = GIMP_PALETTE (gtk_type_new (GIMP_TYPE_PALETTE));

      gimp_object_set_name (GIMP_OBJECT (standard_palette), "Standard");
    }

  return standard_palette;
}


/*  private functions  */

static void
palettes_load_palette (const gchar *filename,
		       gpointer     loader_data)
{
  GimpPalette *palette;

  g_return_if_fail (filename != NULL);

  if (! datafiles_check_extension (filename, GIMP_PALETTE_FILE_EXTENSION))
    {
      g_warning ("%s(): trying old palette file format on file with "
		 "unknown extension: %s",
		 G_GNUC_FUNCTION, filename);
    }

  palette = gimp_palette_load (filename);

  if (! palette)
    g_message (_("Warning: Failed to load palette\n\"%s\""), filename);
  else
    gimp_container_add (GIMP_CONTAINER (loader_data), GIMP_OBJECT (palette));
}
