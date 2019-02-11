/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpparamspecs.h"

#include "plug-in/gimppluginmanager.h"
#include "plug-in/gimppluginprocedure.h"

#include "file-data.h"
#include "file-data-gbr.h"

#include "gimp-intl.h"


void
file_data_init (Gimp *gimp)
{
  GimpPlugInProcedure *proc;
  GFile               *file;
  GimpProcedure       *procedure;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /*  file-gbr-load  */
  file = g_file_new_for_path ("file-gbr-load");
  procedure = gimp_plug_in_procedure_new (GIMP_PLUGIN, file);
  g_object_unref (file);

  procedure->proc_type    = GIMP_INTERNAL;
  procedure->marshal_func = file_gbr_load_invoker;

  proc = GIMP_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (N_("GIMP brush"));
  gimp_plug_in_procedure_set_icon (proc, GIMP_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "gimp-brush",
                                   strlen ("gimp-brush") + 1);
  gimp_plug_in_procedure_set_image_types (proc, NULL);
  gimp_plug_in_procedure_set_file_proc (proc, "gbr, gbp", "",
                                        "20, string, GIMP");
  gimp_plug_in_procedure_set_mime_types (proc, "image/gimp-x-gbr");
  gimp_plug_in_procedure_set_handles_uri (proc);

  gimp_object_set_static_name (GIMP_OBJECT (procedure), "file-gbr-load");
  gimp_procedure_set_static_strings (procedure,
                                     "file-gbr-load",
                                     "Loads GIMP brushes",
                                     "Loads GIMP brushes (1 or 4 bpp "
                                     "and old .gpb format)",
                                     "Tim Newsome, Jens Lautenbacher, "
                                     "Sven Neumann, Michael Natterer",
                                     "Tim Newsome, Jens Lautenbacher, "
                                     "Sven Neumann, Michael Natterer",
                                     "1995-2019",
                                     NULL);

  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_int32 ("dummy-param",
                                                      "Dummy Param",
                                                      "Dummy parameter",
                                                      G_MININT32, G_MAXINT32, 0,
                                                      GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("uri",
                                                       "URI",
                                                       "The URI of the file "
                                                       "to load",
                                                       TRUE, FALSE, TRUE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("raw-uri",
                                                       "Raw URI",
                                                       "The URI of the file "
                                                       "to load",
                                                       TRUE, FALSE, TRUE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));

  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_image_id ("image",
                                                             "Image",
                                                             "Output image",
                                                             gimp, FALSE,
                                                             GIMP_PARAM_READWRITE));

  gimp_plug_in_manager_add_procedure (gimp->plug_in_manager, proc);
  g_object_unref (procedure);
}

void
file_data_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
}
