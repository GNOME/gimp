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

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimpprogress.h"

#include "pdb/procedural_db.h"

#include "plug-in/plug-in-run.h"
#include "plug-in/plug-in-proc-def.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"

#include "display/gimpdisplay.h"

#include "actions.h"
#include "plug-in-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void  plug_in_reset_all_response (GtkWidget *dialog,
                                         gint       response_id,
                                         Gimp      *gimp);


/*  public functions  */

void
plug_in_run_cmd_callback (GtkAction     *action,
                          PlugInProcDef *proc_def,
                          gpointer       data)
{
  Gimp        *gimp;
  ProcRecord  *proc_rec;
  Argument    *args;
  GimpDisplay *gdisp    = NULL;
  gint         gdisp_ID = -1;
  gint         i;
  gint         argc;

  gimp = action_data_get_gimp (data);
  if (! gimp)
    return;

  proc_rec = &proc_def->db_info;

  /* construct the procedures arguments */
  args = g_new0 (Argument, proc_rec->num_args);

  /* initialize the argument types */
  for (i = 0; i < proc_rec->num_args; i++)
    args[i].arg_type = proc_rec->args[i].arg_type;

  /* initialize the first argument  */
  args[0].value.pdb_int = GIMP_RUN_INTERACTIVE;
  argc = 1;

  switch (proc_rec->proc_type)
    {
    case GIMP_EXTENSION:
      break;

    case GIMP_PLUGIN:
    case GIMP_TEMPORARY:
      if (proc_rec->num_args >= 2 &&
          proc_rec->args[1].arg_type == GIMP_PDB_IMAGE)
        {
          gdisp = action_data_get_display (data);

          if (gdisp)
            {
              gdisp_ID = gimp_display_get_ID (gdisp);

              args[1].value.pdb_int = gimp_image_get_ID (gdisp->gimage);
              argc++;

              if (proc_rec->num_args >= 2 &&
                  proc_rec->args[2].arg_type == GIMP_PDB_DRAWABLE)
                {
                  GimpDrawable *drawable;

                  drawable = gimp_image_active_drawable (gdisp->gimage);

                  if (drawable)
                    {
                      args[2].value.pdb_int =
                        gimp_item_get_ID (GIMP_ITEM (drawable));
                      argc++;
                    }
                  else
                    {
                      g_warning ("Uh-oh, no active drawable for the plug-in!");
                      g_free (args);
                      return;
                    }
                }
            }
	}
      break;

    default:
      g_error ("Unknown procedure type.");
      g_free (args);
      return;
    }

  /* run the plug-in procedure */
  plug_in_run (gimp, gimp_get_user_context (gimp),
               gdisp ? GIMP_PROGRESS (gdisp) : NULL,
               proc_rec, args, argc, FALSE, TRUE, gdisp_ID);

  /* remember only "standard" plug-ins */
  if (proc_rec->proc_type == GIMP_PLUGIN           &&
      proc_rec->num_args >= 3                      &&
      proc_rec->args[1].arg_type == GIMP_PDB_IMAGE &&
      proc_rec->args[2].arg_type == GIMP_PDB_DRAWABLE)
    {
      gimp_set_last_plug_in (gimp, proc_def);
    }

  g_free (args);
}

void
plug_in_repeat_cmd_callback (GtkAction *action,
                             gint       value,
                             gpointer   data)
{
  GimpDisplay  *gdisp;
  GimpDrawable *drawable;
  gboolean      interactive;

  gdisp = action_data_get_display (data);
  if (! gdisp)
    return;

  drawable = gimp_image_active_drawable (gdisp->gimage);
  if (! drawable)
    return;

  interactive = value ? TRUE : FALSE;

  plug_in_repeat (gdisp->gimage->gimp,
                  gimp_get_user_context (gdisp->gimage->gimp),
                  GIMP_PROGRESS (gdisp),
                  gimp_display_get_ID (gdisp),
                  gimp_image_get_ID (gdisp->gimage),
                  gimp_item_get_ID (GIMP_ITEM (drawable)),
                  interactive);
}

void
plug_in_reset_all_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  Gimp      *gimp = action_data_get_gimp (data);
  GtkWidget *dialog;

  if (! gimp)
    return;

  dialog = gimp_message_dialog_new (_("Reset all Filters"), GIMP_STOCK_QUESTION,
                                    NULL, 0,
                                    gimp_standard_help_func, NULL,

                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GIMP_STOCK_RESET, GTK_RESPONSE_OK,

                                    NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (plug_in_reset_all_response),
                    gimp);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("Do you really want to reset all "
                                       "filters to default values?"));

  gtk_widget_show (dialog);
}


/*  private functions  */

static void
plug_in_reset_all_response (GtkWidget *dialog,
                            gint       response_id,
                            Gimp      *gimp)
{
  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    procedural_db_free_data (gimp);
}
