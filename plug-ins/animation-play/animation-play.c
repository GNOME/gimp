/*
 * Animation Playback plug-in version 0.99.1
 *
 * (c) Adam D. Moss : 1997-2000 : adam@gimp.org : adam@foxbox.org
 * (c) Mircea Purdea : 2009 : someone_else@exhalus.net
 * (c) Jehan : 2012-2016 : jehan at girinstud.io
 *
 * GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <libgimp/gimp.h>
#undef GDK_DISABLE_DEPRECATED
#include <libgimp/gimpui.h>
#include "libgimp/stdplugins-intl.h"

#include "widgets/animation-dialog.h"

#include "animation-utils.h"

static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"      }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Preview an animation"),
                          "",
                          "Adam D. Moss <adam@gimp.org>",
                          "Adam D. Moss <adam@gimp.org>",
                          "1997, 1998...",
                          N_("Animation _Playback..."),
                          "RGB*, INDEXED*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Animation");
  gimp_plugin_icon_register (PLUG_IN_PROC, GIMP_ICON_TYPE_ICON_NAME,
                             (const guint8 *) "media-playback-start");
}

static void
run (const gchar      *name,
     gint              n_params,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpPDBStatusType  status;
  GimpRunMode        run_mode;
  GeglConfig        *config;

  INIT_I18N ();

  gegl_init (NULL, NULL);
  config = gegl_config ();
  /* For preview, we want fast (0.0) over high quality (1.0). */
  g_object_set (config, "quality", 0.0, NULL);

  run_mode = param[0].data.d_int32;

  if (run_mode == GIMP_RUN_NONINTERACTIVE ||
      n_params != 3)
    {
      /* This plugin is meaningless right now other than interactive. */
      status = GIMP_PDB_CALLING_ERROR;
    }
  else
    {
      GtkWidget       *dialog;
      gint32           image_id;

      gimp_ui_init (PLUG_IN_BINARY, TRUE);

      image_id = param[1].data.d_image;
      dialog   = animation_dialog_new (image_id);

      gimp_help_connect (GTK_WIDGET (dialog),
                         gimp_standard_help_func,
                         PLUG_IN_PROC, NULL);
      gtk_widget_show_now (GTK_WIDGET (dialog));

      gtk_main ();
      gimp_displays_flush ();

      status = GIMP_PDB_SUCCESS;
    }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  gegl_exit ();
  gimp_quit ();
}
