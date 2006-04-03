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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"

#include "plug-in/plug-in-data.h"
#include "plug-in/plug-in-run.h"
#include "plug-in/plug-in-proc-def.h"

#include "pdb/gimpargument.h"
#include "pdb/gimpprocedure.h"

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
  Gimp          *gimp;
  GimpProcedure *procedure;
  GimpArgument  *args;
  gint           n_args  = 0;
  GimpDisplay   *display = NULL;
  return_if_no_gimp (gimp, data);

  procedure = proc_def->procedure;

  args = gimp_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  g_value_set_int (&args[n_args].value, GIMP_RUN_INTERACTIVE);
  n_args++;

  switch (procedure->proc_type)
    {
    case GIMP_EXTENSION:
      break;

    case GIMP_PLUGIN:
    case GIMP_TEMPORARY:
      if (procedure->num_args > n_args &&
          GIMP_VALUE_HOLDS_IMAGE_ID (&args[n_args].value))
        {
          display = action_data_get_display (data);

          if (display)
            {
              gimp_value_set_image (&args[n_args].value, display->image);
              n_args++;

              if (procedure->num_args > n_args &&
                  GIMP_VALUE_HOLDS_DRAWABLE_ID (&args[n_args].value));
                {
                  GimpDrawable *drawable;

                  drawable = gimp_image_active_drawable (display->image);

                  if (drawable)
                    {
                      gimp_value_set_drawable (&args[n_args].value, drawable);
                      n_args++;
                    }
                  else
                    {
                      g_warning ("Uh-oh, no active drawable for the plug-in!");
                      goto error;
                    }
                }
            }
	}
      break;

    default:
      g_error ("Unknown procedure type.");
      goto error;
    }

  /* run the plug-in procedure */
  plug_in_run (gimp, gimp_get_user_context (gimp),
               GIMP_PROGRESS (display),
               procedure, args, n_args, FALSE, TRUE,
               display ? gimp_display_get_ID (display) : -1);

  /* remember only "standard" plug-ins */
  if (procedure->proc_type == GIMP_PLUGIN                       &&
      procedure->num_args  >= 3                                 &&
      GIMP_IS_PARAM_SPEC_IMAGE_ID    (procedure->args[1].pspec) &&
      GIMP_IS_PARAM_SPEC_DRAWABLE_ID (procedure->args[2].pspec))
    {
      gimp_set_last_plug_in (gimp, proc_def);
    }

 error:
  gimp_arguments_destroy (args, procedure->num_args);
}

void
plug_in_repeat_cmd_callback (GtkAction *action,
                             gint       value,
                             gpointer   data)
{
  GimpDisplay  *display;
  GimpDrawable *drawable;
  gboolean      interactive = TRUE;
  return_if_no_display (display, data);

  drawable = gimp_image_active_drawable (display->image);
  if (! drawable)
    return;

  if (strcmp (gtk_action_get_name (action), "plug-in-repeat") == 0)
    interactive = FALSE;

  plug_in_repeat (display->image->gimp, value,
                  gimp_get_user_context (display->image->gimp),
                  GIMP_PROGRESS (display),
                  gimp_display_get_ID (display),
                  gimp_image_get_ID (display->image),
                  gimp_item_get_ID (GIMP_ITEM (drawable)),
                  interactive);
}

void
plug_in_reset_all_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  Gimp      *gimp;
  GtkWidget *dialog;
  return_if_no_gimp (gimp, data);

  dialog = gimp_message_dialog_new (_("Reset all Filters"), GIMP_STOCK_QUESTION,
                                    NULL, 0,
                                    gimp_standard_help_func, NULL,

                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GIMP_STOCK_RESET, GTK_RESPONSE_OK,

                                    NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

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
    plug_in_data_free (gimp);
}
