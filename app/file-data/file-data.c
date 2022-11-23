/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"

#include "core/core-types.h"

#include "core/ligma.h"
#include "core/ligmadrawable.h"
#include "core/ligmaparamspecs.h"

#include "plug-in/ligmapluginmanager.h"
#include "plug-in/ligmapluginprocedure.h"

#include "file-data.h"
#include "file-data-gbr.h"
#include "file-data-gex.h"
#include "file-data-gih.h"
#include "file-data-pat.h"

#include "ligma-intl.h"


void
file_data_init (Ligma *ligma)
{
  LigmaPlugInProcedure *proc;
  GFile               *file;
  LigmaProcedure       *procedure;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  /*  file-gbr-load  */
  file = g_file_new_for_path ("file-gbr-load");
  procedure = ligma_plug_in_procedure_new (LIGMA_PDB_PROC_TYPE_PLUGIN, file);
  g_object_unref (file);

  procedure->proc_type    = LIGMA_PDB_PROC_TYPE_INTERNAL;
  procedure->marshal_func = file_gbr_load_invoker;

  proc = LIGMA_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (N_("LIGMA brush"));
  ligma_plug_in_procedure_set_icon (proc, LIGMA_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "ligma-brush",
                                   strlen ("ligma-brush") + 1,
                                   NULL);
  ligma_plug_in_procedure_set_image_types (proc, NULL);
  ligma_plug_in_procedure_set_file_proc (proc, "gbr, gbp", "",
                                        "20, string, LIGMA");
  ligma_plug_in_procedure_set_mime_types (proc, "image/ligma-x-gbr");
  ligma_plug_in_procedure_set_handles_remote (proc);

  ligma_object_set_static_name (LIGMA_OBJECT (procedure), "file-gbr-load");
  ligma_procedure_set_static_help (procedure,
                                  "Loads LIGMA brushes",
                                  "Loads LIGMA brushes (1 or 4 bpp "
                                  "and old .gpb format)",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Tim Newsome, Jens Lautenbacher, "
                                         "Sven Neumann, Michael Natterer",
                                         "Tim Newsome, Jens Lautenbacher, "
                                         "Sven Neumann, Michael Natterer",
                                         "1995-2019");

  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_enum ("dummy-param",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     LIGMA_TYPE_RUN_MODE,
                                                     LIGMA_RUN_INTERACTIVE,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to load",
                                                    G_TYPE_FILE,
                                                    LIGMA_PARAM_READWRITE));

  ligma_procedure_add_return_value (procedure,
                                   ligma_param_spec_image ("image",
                                                          "Image",
                                                          "Output image",
                                                          FALSE,
                                                          LIGMA_PARAM_READWRITE));

  ligma_plug_in_manager_add_procedure (ligma->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  file-gbr-save-internal  */
  file = g_file_new_for_path ("file-gbr-save-internal");
  procedure = ligma_plug_in_procedure_new (LIGMA_PDB_PROC_TYPE_PLUGIN, file);
  g_object_unref (file);

  procedure->proc_type    = LIGMA_PDB_PROC_TYPE_INTERNAL;
  procedure->marshal_func = file_gbr_save_invoker;

  proc = LIGMA_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (N_("LIGMA brush"));
  ligma_plug_in_procedure_set_icon (proc, LIGMA_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "ligma-brush",
                                   strlen ("ligma-brush") + 1,
                                   NULL);

#if 0
  /* do not register as file procedure */
  ligma_plug_in_procedure_set_image_types (proc, "RGB*, GRAY*, INDEXED*");
  ligma_plug_in_procedure_set_file_proc (proc, "gbr", "", NULL);
  ligma_plug_in_procedure_set_mime_types (proc, "image/x-ligma-gbr");
  ligma_plug_in_procedure_set_handles_remote (proc);
#endif

  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "file-gbr-save-internal");
  ligma_procedure_set_static_help (procedure,
                                  "Exports Ligma brush file (.GBR)",
                                  "Exports Ligma brush file (.GBR)",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Tim Newsome, Michael Natterer",
                                         "Tim Newsome, Michael Natterer",
                                         "1995-2019");

  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_enum ("dummy-param",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     LIGMA_TYPE_RUN_MODE,
                                                     LIGMA_RUN_INTERACTIVE,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_image ("image",
                                                      "Image",
                                                      "Input image",
                                                      FALSE,
                                                      LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_drawable ("drawable",
                                                         "Drawable",
                                                         "Active drawable "
                                                         "of input image",
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to export",
                                                    G_TYPE_FILE,
                                                    LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_int ("spacing",
                                                 "spacing",
                                                 "Spacing of the brush",
                                                 1, 1000, 10,
                                                 LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("name",
                                                       "name",
                                                       "The name of the "
                                                       "brush",
                                                       FALSE, FALSE, TRUE,
                                                       "LIGMA Brush",
                                                       LIGMA_PARAM_READWRITE));

  ligma_plug_in_manager_add_procedure (ligma->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  file-gih-load  */
  file = g_file_new_for_path ("file-gih-load");
  procedure = ligma_plug_in_procedure_new (LIGMA_PDB_PROC_TYPE_PLUGIN, file);
  g_object_unref (file);

  procedure->proc_type    = LIGMA_PDB_PROC_TYPE_INTERNAL;
  procedure->marshal_func = file_gih_load_invoker;

  proc = LIGMA_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (N_("LIGMA brush (animated)"));
  ligma_plug_in_procedure_set_icon (proc, LIGMA_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "ligma-brush",
                                   strlen ("ligma-brush") + 1,
                                   NULL);
  ligma_plug_in_procedure_set_image_types (proc, NULL);
  ligma_plug_in_procedure_set_file_proc (proc, "gih", "", "");
  ligma_plug_in_procedure_set_mime_types (proc, "image/ligma-x-gih");
  ligma_plug_in_procedure_set_handles_remote (proc);

  ligma_object_set_static_name (LIGMA_OBJECT (procedure), "file-gih-load");
  ligma_procedure_set_static_help (procedure,
                                  "Loads LIGMA animated brushes",
                                  "This procedure loads a LIGMA brush "
                                  "pipe as an image.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Tor Lillqvist, Michael Natterer",
                                         "Tor Lillqvist, Michael Natterer",
                                         "1999-2019");

  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_enum ("dummy-param",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     LIGMA_TYPE_RUN_MODE,
                                                     LIGMA_RUN_INTERACTIVE,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to load",
                                                    G_TYPE_FILE,
                                                    LIGMA_PARAM_READWRITE));

  ligma_procedure_add_return_value (procedure,
                                   ligma_param_spec_image ("image",
                                                          "Image",
                                                          "Output image",
                                                          FALSE,
                                                          LIGMA_PARAM_READWRITE));

  ligma_plug_in_manager_add_procedure (ligma->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  file-gih-save-internal  */
  file = g_file_new_for_path ("file-gih-save-internal");
  procedure = ligma_plug_in_procedure_new (LIGMA_PDB_PROC_TYPE_PLUGIN, file);
  g_object_unref (file);

  procedure->proc_type    = LIGMA_PDB_PROC_TYPE_INTERNAL;
  procedure->marshal_func = file_gih_save_invoker;

  proc = LIGMA_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (N_("LIGMA brush (animated)"));
  ligma_plug_in_procedure_set_icon (proc, LIGMA_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "ligma-brush",
                                   strlen ("ligma-brush") + 1,
                                   NULL);

#if 0
  /* do not register as file procedure */
  ligma_plug_in_procedure_set_image_types (proc, "RGB*, GRAY*, INDEXED*");
  ligma_plug_in_procedure_set_file_proc (proc, "gih", "", NULL);
  ligma_plug_in_procedure_set_mime_types (proc, "image/x-ligma-gih");
  ligma_plug_in_procedure_set_handles_remote (proc);
#endif

  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "file-gih-save-internal");
  ligma_procedure_set_static_help (procedure,
                                  "Exports Ligma animated brush file (.gih)",
                                  "Exports Ligma animated brush file (.gih)",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Tor Lillqvist, Michael Natterer",
                                         "Tor Lillqvist, Michael Natterer",
                                         "1999-2019");

  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_enum ("dummy-param",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     LIGMA_TYPE_RUN_MODE,
                                                     LIGMA_RUN_INTERACTIVE,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_image ("image",
                                                      "Image",
                                                      "Input image",
                                                      FALSE,
                                                      LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_int ("num-drawables",
                                                 "num drawables",
                                                 "The number of drawables to save",
                                                 1, G_MAXINT32, 1,
                                                 LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_object_array ("drawables",
                                                             "drawables",
                                                             "Drawables to save",
                                                             LIGMA_TYPE_DRAWABLE,
                                                             LIGMA_PARAM_READWRITE | LIGMA_PARAM_NO_VALIDATE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to export",
                                                    G_TYPE_FILE,
                                                    LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_int ("spacing",
                                                 "spacing",
                                                 "Spacing of the brush",
                                                 1, 1000, 10,
                                                 LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("name",
                                                       "name",
                                                       "The name of the "
                                                       "brush",
                                                       FALSE, FALSE, TRUE,
                                                       "LIGMA Brush",
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("params",
                                                       "params",
                                                       "The pipe's parameters",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));

  ligma_plug_in_manager_add_procedure (ligma->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  file-pat-load  */
  file = g_file_new_for_path ("file-pat-load");
  procedure = ligma_plug_in_procedure_new (LIGMA_PDB_PROC_TYPE_PLUGIN, file);
  g_object_unref (file);

  procedure->proc_type    = LIGMA_PDB_PROC_TYPE_INTERNAL;
  procedure->marshal_func = file_pat_load_invoker;

  proc = LIGMA_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (N_("LIGMA pattern"));
  ligma_plug_in_procedure_set_icon (proc, LIGMA_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "ligma-pattern",
                                   strlen ("ligma-pattern") + 1,
                                   NULL);
  ligma_plug_in_procedure_set_image_types (proc, NULL);
  ligma_plug_in_procedure_set_file_proc (proc, "pat", "",
                                        "20,string,GPAT");
  ligma_plug_in_procedure_set_mime_types (proc, "image/ligma-x-pat");
  ligma_plug_in_procedure_set_handles_remote (proc);

  ligma_object_set_static_name (LIGMA_OBJECT (procedure), "file-pat-load");
  ligma_procedure_set_static_help (procedure,
                                  "Loads LIGMA patterns",
                                  "Loads LIGMA patterns",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Tim Newsome, Michael Natterer",
                                         "Tim Newsome, Michael Natterer",
                                         "1997-2019");

  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_enum ("dummy-param",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     LIGMA_TYPE_RUN_MODE,
                                                     LIGMA_RUN_INTERACTIVE,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to load",
                                                    G_TYPE_FILE,
                                                    LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   ligma_param_spec_image ("image",
                                                          "Image",
                                                          "Output image",
                                                          FALSE,
                                                          LIGMA_PARAM_READWRITE));

  ligma_plug_in_manager_add_procedure (ligma->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  file-pat-save-internal  */
  file = g_file_new_for_path ("file-pat-save-internal");
  procedure = ligma_plug_in_procedure_new (LIGMA_PDB_PROC_TYPE_PLUGIN, file);
  g_object_unref (file);

  procedure->proc_type    = LIGMA_PDB_PROC_TYPE_INTERNAL;
  procedure->marshal_func = file_pat_save_invoker;

  proc = LIGMA_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (N_("LIGMA pattern"));
  ligma_plug_in_procedure_set_icon (proc, LIGMA_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "ligma-pattern",
                                   strlen ("ligma-pattern") + 1,
                                   NULL);

#if 0
  /* do not register as file procedure */
  ligma_plug_in_procedure_set_image_types (proc, "RGB*, GRAY*, INDEXED*");
  ligma_plug_in_procedure_set_file_proc (proc, "pat", "", NULL);
  ligma_plug_in_procedure_set_mime_types (proc, "image/x-ligma-pat");
  ligma_plug_in_procedure_set_handles_remote (proc);
#endif

  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "file-pat-save-internal");
  ligma_procedure_set_static_help (procedure,
                                  "Exports Ligma pattern file (.PAT)",
                                  "Exports Ligma pattern file (.PAT)",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Tim Newsome, Michael Natterer",
                                         "Tim Newsome, Michael Natterer",
                                         "1995-2019");

  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_enum ("dummy-param",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     LIGMA_TYPE_RUN_MODE,
                                                     LIGMA_RUN_INTERACTIVE,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_image ("image",
                                                      "Image",
                                                      "Input image",
                                                      FALSE,
                                                      LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_int ("n-drawables",
                                                 "Num drawables",
                                                 "Number of drawables",
                                                 1, G_MAXINT, 1,
                                                 LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_object_array ("drawables",
                                                             "Drawables",
                                                             "Selected drawables",
                                                             LIGMA_TYPE_DRAWABLE,
                                                             LIGMA_PARAM_READWRITE | LIGMA_PARAM_NO_VALIDATE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to export",
                                                    G_TYPE_FILE,
                                                    LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("name",
                                                       "name",
                                                       "The name of the "
                                                       "pattern",
                                                       FALSE, FALSE, TRUE,
                                                       "LIGMA Pattern",
                                                       LIGMA_PARAM_READWRITE));

  ligma_plug_in_manager_add_procedure (ligma->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  file-gex-load  */
  file = g_file_new_for_path ("file-gex-load");
  procedure = ligma_plug_in_procedure_new (LIGMA_PDB_PROC_TYPE_PLUGIN, file);
  g_object_unref (file);

  procedure->proc_type    = LIGMA_PDB_PROC_TYPE_INTERNAL;
  procedure->marshal_func = file_gex_load_invoker;

  proc = LIGMA_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (N_("LIGMA extension"));
  ligma_plug_in_procedure_set_icon (proc, LIGMA_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "ligma-plugin",
                                   strlen ("ligma-plugin") + 1,
                                   NULL);
  ligma_plug_in_procedure_set_file_proc (proc, "gex", "",
                                        "20, string, LIGMA");
  ligma_plug_in_procedure_set_generic_file_proc (proc, TRUE);
  ligma_plug_in_procedure_set_mime_types (proc, "image/ligma-x-gex");
  ligma_plug_in_procedure_set_handles_remote (proc);

  ligma_object_set_static_name (LIGMA_OBJECT (procedure), "file-gex-load");
  ligma_procedure_set_static_help (procedure,
                                  "Loads LIGMA extension",
                                  "Loads LIGMA extension",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Jehan", "Jehan", "2019");

  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_enum ("dummy-param",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     LIGMA_TYPE_RUN_MODE,
                                                     LIGMA_RUN_INTERACTIVE,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to load",
                                                    G_TYPE_FILE,
                                                    LIGMA_PARAM_READWRITE));

  ligma_procedure_add_return_value (procedure,
                                   ligma_param_spec_string ("extension-id",
                                                           "ID of installed extension",
                                                           "Identifier of the newly installed extension",
                                                           FALSE, TRUE, FALSE, NULL,
                                                           LIGMA_PARAM_READWRITE));

  ligma_plug_in_manager_add_procedure (ligma->plug_in_manager, proc);
  g_object_unref (procedure);
}

void
file_data_exit (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
}
