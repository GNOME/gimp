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

#include "context_manager.h"
#include "gimpdatalist.h"
#include "gimppattern.h"
#include "gimprc.h"
#include "patterns.h"
#include "pattern_select.h"
#include "temp_buf.h"


/*  public functions  */

void
patterns_init (gboolean no_data)
{
  patterns_free ();

  if (pattern_path != NULL && !no_data)
    {
      pattern_select_freeze_all ();

      gimp_data_list_load (GIMP_DATA_LIST (global_pattern_list),
			   pattern_path,

			   (GimpDataObjectLoaderFunc) gimp_pattern_load,
			   GIMP_PATTERN_FILE_EXTENSION,

			   NULL);

      pattern_select_thaw_all ();
    }
}

void
patterns_free (void)
{
  if (gimp_container_num_children (global_pattern_list) == 0)
    return;

  pattern_select_freeze_all ();

  gimp_data_list_save_and_clear (GIMP_DATA_LIST (global_pattern_list),
				 pattern_path,
				 GIMP_PATTERN_FILE_EXTENSION);

  pattern_select_thaw_all ();
}

GimpPattern *
patterns_get_standard_pattern (void)
{
  static GimpPattern *standard_pattern = NULL;

  if (! standard_pattern)
    {
      guchar *data;
      gint    row, col;

      standard_pattern = GIMP_PATTERN (gtk_type_new (GIMP_TYPE_PATTERN));

      gimp_object_set_name (GIMP_OBJECT (standard_pattern), "Standard");

      standard_pattern->mask = temp_buf_new (32, 32, 3, 0, 0, NULL);

      data = temp_buf_data (standard_pattern->mask);

      for (row = 0; row < standard_pattern->mask->height; row++)
	for (col = 0; col < standard_pattern->mask->width; col++)
	  {
	    memset (data, (col % 2) && (row % 2) ? 255 : 0, 3);
	    data += 3;
	  }

      /*  set ref_count to 2 --> never swap the standard pattern  */
      gtk_object_ref (GTK_OBJECT (standard_pattern));
      gtk_object_ref (GTK_OBJECT (standard_pattern));
      gtk_object_sink (GTK_OBJECT (standard_pattern));
    }

  return standard_pattern;
}
