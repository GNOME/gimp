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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURIGHTE.  See the
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
#include "gimpgradient.h"
#include "gimprc.h"
#include "gradient_select.h"
#include "gradients.h"


/*  public functions  */

void
gradients_init (gint no_data)
{
  gradients_free ();

  if (gradient_path != NULL && !no_data)
    {
      gradient_select_freeze_all ();

      gimp_data_list_load (GIMP_DATA_LIST (global_gradient_list),
			   gradient_path,

			   (GimpDataObjectLoaderFunc) gimp_gradient_load,
			   GIMP_GRADIENT_FILE_EXTENSION,

			   (GimpDataObjectLoaderFunc) gimp_gradient_load,
			   NULL /* legacy loader */);

      gradient_select_thaw_all ();
    }
}

void
gradients_free (void)
{
  if (gimp_container_num_children (global_gradient_list) == 0)
    return;

  gradient_select_freeze_all ();

  gimp_data_list_save_and_clear (GIMP_DATA_LIST (global_gradient_list),
				 gradient_path,
				 GIMP_GRADIENT_FILE_EXTENSION);

  gradient_select_thaw_all ();
}

GimpGradient *
gradients_get_standard_gradient (void)
{
  static GimpGradient *standard_gradient = NULL;

  if (! standard_gradient)
    {
      standard_gradient = gimp_gradient_new ("Standard");

      gtk_object_ref (GTK_OBJECT (standard_gradient));
      gtk_object_sink (GTK_OBJECT (standard_gradient));
    }

  return standard_gradient;
}
