/* metainfo.c - main() for the metainfo editor
 *
 * Copyright (C) 2014, Hartmut Kuhse <hatti@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"

#include "metainfo.h"
#include "interface.h"


/* prototypes of local functions */
static void  query (void);
static void  run   (const gchar      *name,
                    gint              nparams,
                    const GimpParam  *param,
                    gint             *nreturn_vals,
                    GimpParam       **return_vals);


/* local variables */
const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

/* local functions */
MAIN ()

static void
query (void)
{
  static const GimpParamDef editor_args[] =
  {
    { GIMP_PDB_INT32,       "run-mode",  "Run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,       "image",     "Input image"                  },
    { GIMP_PDB_DRAWABLE,    "drawable",  "Input drawable (unused)"      }
  };


  gimp_install_procedure (EDITOR_PROC,
                          N_("View and edit metainformation"),
                          "View and edit meta information reagarding digital "
                          "rights, attached to the current image. "
                          "This metainformation will be saved in the file, "
                          "depending on the output file format.",
                          "Hartmut Kuhse <hatti@gimp.org>",
                          "Hartmut Kuhse <hatti@gimp.org>",
                          "2014",
                          N_("Digital Rights"),
                          "RGB*, INDEXED*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (editor_args), 0,
                          editor_args, NULL);

  gimp_plugin_menu_register (EDITOR_PROC, "<Image>/Image");
  // XXX gimp_plugin_icon_register (EDITOR_PROC, GIMP_ICON_TYPE_STOCK_ID,
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[4];
  gint32             image_ID;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpAttributes    *attributes;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  INIT_I18N();

  if (! strcmp (name, EDITOR_PROC))
    image_ID = param[1].data.d_image;
  else
    image_ID = param[0].data.d_image;



  /* Now check what we are supposed to do */
if (! strcmp (name, EDITOR_PROC))
    {
      GimpRunMode run_mode;

      run_mode = param[0].data.d_int32;
      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          attributes = gimp_image_get_attributes (image_ID);

          if (! attributes)
            {
              attributes = gimp_attributes_new ();
            }

          if (attributes)
            {
              if (! metainfo_dialog (image_ID, attributes))
                status = GIMP_PDB_CANCEL;
              else
                {
                  if (metainfo_get_save_attributes())
                    gimp_image_set_attributes (image_ID, attributes);
                  status = GIMP_PDB_SUCCESS;
                }
            }
          else
            {
              g_message (_("General attributes failure."));
            }
          g_object_unref (attributes);
        }
      else
        status = GIMP_PDB_CALLING_ERROR;
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
    }

  values[0].data.d_status = status;
}
