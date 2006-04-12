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
#include "core/gimp-utils.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"

#include "plug-in/plug-in-data.h"

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
plug_in_run_cmd_callback (GtkAction           *action,
                          GimpPlugInProcedure *proc,
                          gpointer             data)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (proc);
  Gimp          *gimp;
  GValueArray   *args;
  gint           n_args    = 0;
  GimpDisplay   *display   = NULL;
  return_if_no_gimp (gimp, data);

  args = gimp_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  g_value_set_int (&args->values[n_args], GIMP_RUN_INTERACTIVE);
  n_args++;

  switch (procedure->proc_type)
    {
    case GIMP_EXTENSION:
      break;

    case GIMP_PLUGIN:
    case GIMP_TEMPORARY:
      if (args->n_values > n_args &&
          GIMP_VALUE_HOLDS_IMAGE_ID (&args->values[n_args]))
        {
          display = action_data_get_display (data);

          if (display)
            {
              gimp_value_set_image (&args->values[n_args], display->image);
              n_args++;

              if (args->n_values > n_args &&
                  GIMP_VALUE_HOLDS_DRAWABLE_ID (&args->values[n_args]));
                {
                  GimpDrawable *drawable;

                  drawable = gimp_image_active_drawable (display->image);

                  if (drawable)
                    {
                      gimp_value_set_drawable (&args->values[n_args], drawable);
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

  gimp_value_array_truncate (args, n_args);

  /* run the plug-in procedure */
  gimp_procedure_execute_async (procedure, gimp, gimp_get_user_context (gimp),
                                GIMP_PROGRESS (display), args,
                                display ? gimp_display_get_ID (display) : -1);

  /* remember only "standard" plug-ins */
  if (procedure->proc_type == GIMP_PLUGIN                 &&
      procedure->num_args  >= 3                           &&
      GIMP_IS_PARAM_SPEC_IMAGE_ID    (procedure->args[1]) &&
      GIMP_IS_PARAM_SPEC_DRAWABLE_ID (procedure->args[2]))
    {
      gimp_set_last_plug_in (gimp, proc);
    }

 error:
  g_value_array_free (args);
}

void
plug_in_repeat_cmd_callback (GtkAction *action,
                             gint       value,
                             gpointer   data)
{
  GimpProcedure *procedure;
  Gimp          *gimp;
  GimpDisplay   *display;
  GimpDrawable  *drawable;
  gboolean       interactive = TRUE;
  return_if_no_gimp (gimp, data);
  return_if_no_display (display, data);

  drawable = gimp_image_active_drawable (display->image);
  if (! drawable)
    return;

  if (strcmp (gtk_action_get_name (action), "plug-in-repeat") == 0)
    interactive = FALSE;

  procedure = g_slist_nth_data (gimp->last_plug_ins, value);

  if (procedure)
    {
      GValueArray *args = gimp_procedure_get_arguments (procedure);

      g_value_set_int         (&args->values[0],
                               interactive ?
                               GIMP_RUN_INTERACTIVE : GIMP_RUN_WITH_LAST_VALS);
      gimp_value_set_image    (&args->values[1], display->image);
      gimp_value_set_drawable (&args->values[2], drawable);

      /* run the plug-in procedure */
      gimp_procedure_execute_async (procedure, gimp,
                                    gimp_get_user_context (gimp),
                                    GIMP_PROGRESS (display), args,
                                    gimp_display_get_ID (display));

      g_value_array_free (args);
    }
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
