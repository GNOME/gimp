/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * busy-dialog.c
 * Copyright (C) 2018 Ell
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-busy-dialog"
#define PLUG_IN_BINARY "busy-dialog"
#define PLUG_IN_ROLE   "gimp-busy-dialog"


typedef struct
{
  GIOChannel *read_channel;
  GIOChannel *write_channel;
} Context;


static void                query                           (void);
static void                run                             (const gchar      *name,
                                                            gint              nparams,
                                                            const GimpParam  *param,
                                                            gint             *nreturn_vals,
                                                            GimpParam       **return_vals);

static GimpPDBStatusType   busy_dialog                     (gint              read_fd,
                                                            gint              write_fd,
                                                            const gchar      *message,
                                                            gboolean          cancelable);

static gboolean            busy_dialog_read_channel_notify (GIOChannel       *source,
                                                            GIOCondition      condition,
                                                            Context          *context);

static gboolean            busy_dialog_delete_event        (GtkDialog        *dialog,
                                                            GdkEvent         *event,
                                                            Context          *context);
static void                busy_dialog_response            (GtkDialog        *dialog,
                                                            gint              response_id,
                                                            Context          *context);


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
  static const GimpParamDef args [] =
  {
    { GIMP_PDB_INT32,  "run-mode",   "The run mode { RUN-INTERACTIVE (0) }"             },
    { GIMP_PDB_INT32,  "read-fd",    "The read file descriptor"                         },
    { GIMP_PDB_INT32,  "write-fd",   "The write file descriptor"                        },
    { GIMP_PDB_STRING, "message",    "The message"                                      },
    { GIMP_PDB_INT32,  "cancelable", "Whether the dialog is cancelable (TRUE or FALSE)" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          "Show a dialog while waiting for an operation to finish",
                          "Used by GIMP to display a dialog, containing a "
                          "spinner and a custom message, while waiting for an "
                          "ongoing operation to finish. Optionally, the dialog "
                          "may provide a \"Cancel\" button, which can be used "
                          "to cancel the operation.",
                          "Ell",
                          "Ell",
                          "2018",
                          NULL,
                          "",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (const gchar      *name,
     gint              n_params,
     const GimpParam  *params,
     gint             *n_return_vals,
     GimpParam       **return_vals)
{
  GimpRunMode       run_mode = params[0].data.d_int32;
  GimpPDBStatusType status   = GIMP_PDB_SUCCESS;

  static GimpParam  values[1];

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *n_return_vals = 1;
  *return_vals   = values;

  INIT_I18N ();

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_NONINTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      if (n_params != 5)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          status = busy_dialog (params[1].data.d_int32,
                                params[2].data.d_int32,
                                params[3].data.d_string,
                                params[4].data.d_int32);
        }
      break;

    default:
      status = GIMP_PDB_CALLING_ERROR;
      break;
    }

  values[0].data.d_status = status;
}

static GimpPDBStatusType
busy_dialog (gint         read_fd,
             gint         write_fd,
             const gchar *message,
             gboolean     cancelable)
{
  Context    context;
  GtkWidget *window;
  GtkWidget *content_area;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *box;

#ifdef G_OS_WIN32
  context.read_channel  = g_io_channel_win32_new_fd (read_fd);
  context.write_channel = g_io_channel_win32_new_fd (write_fd);
#else
  context.read_channel  = g_io_channel_unix_new (read_fd);
  context.write_channel = g_io_channel_unix_new (write_fd);
#endif

  g_io_channel_set_close_on_unref (context.read_channel,  TRUE);
  g_io_channel_set_close_on_unref (context.write_channel, TRUE);

  /* triggered when the operation is finished in the main app, and we should
   * quit.
   */
  g_io_add_watch (context.read_channel, G_IO_IN | G_IO_ERR | G_IO_HUP,
                  (GIOFunc) busy_dialog_read_channel_notify,
                  &context);

  /* call gtk_init() before gimp_ui_init(), to avoid DESKTOP_STARTUP_ID from
   * taking effect -- we want the dialog to be prominently displayed above
   * other plug-in windows.
   */
  gtk_init (NULL, NULL);

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  /* the main window */
  if (! cancelable)
    {
      window = g_object_new (GTK_TYPE_WINDOW,
                             "title",             _("Please Wait"),
                             "skip-taskbar-hint", TRUE,
                             "deletable",         FALSE,
                             "resizable",         FALSE,
                             "role",              "gimp-busy-dialog",
                             "type-hint",         GDK_WINDOW_TYPE_HINT_DIALOG,
                             "window-position",   GTK_WIN_POS_CENTER,
                             NULL);

      g_signal_connect (window, "delete-event", G_CALLBACK (gtk_true), NULL);

      content_area = window;
    }
  else
    {
      window = g_object_new (GTK_TYPE_DIALOG,
                             "title",             _("Please Wait"),
                             "skip-taskbar-hint", TRUE,
                             "resizable",         FALSE,
                             "role",              "gimp-busy-dialog",
                             "window-position",   GTK_WIN_POS_CENTER,
                             NULL);

      gtk_dialog_add_button (GTK_DIALOG (window),
                             _("_Cancel"), GTK_RESPONSE_CANCEL);

      g_signal_connect (window, "delete-event",
                        G_CALLBACK (busy_dialog_delete_event),
                        &context);

      g_signal_connect (window, "response",
                        G_CALLBACK (busy_dialog_response),
                        &context);

      content_area = gtk_dialog_get_content_area (GTK_DIALOG (window));
    }

  /* the main vbox */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 16);
  gtk_container_add (GTK_CONTAINER (content_area), vbox);
  gtk_widget_show (vbox);

  /* the title label */
  label = gtk_label_new (_("Please wait for the operation to complete"));
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* the busy box */
  box = gimp_busy_box_new (message);
  gtk_container_set_border_width (GTK_CONTAINER (box), 8);
  gtk_box_pack_start (GTK_BOX (vbox), box, TRUE, TRUE, 0);
  gtk_widget_show (box);

  gtk_window_present (GTK_WINDOW (window));

  gtk_main ();

  gtk_widget_destroy (window);

  g_clear_pointer (&context.read_channel,  g_io_channel_unref);
  g_clear_pointer (&context.write_channel, g_io_channel_unref);

  return GIMP_PDB_SUCCESS;
}

static gboolean
busy_dialog_read_channel_notify (GIOChannel   *source,
                                 GIOCondition  condition,
                                 Context      *context)
{
  gtk_main_quit ();

  return FALSE;
}

static gboolean
busy_dialog_delete_event (GtkDialog *dialog,
                          GdkEvent  *event,
                          Context   *context)
{
  gtk_dialog_response (dialog, GTK_RESPONSE_CANCEL);

  return TRUE;
}

static void
busy_dialog_response (GtkDialog *dialog,
                      gint       response_id,
                      Context   *context)
{
  switch (response_id)
    {
    case GTK_RESPONSE_CANCEL:
      {
        GtkWidget *button;

        gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_CANCEL, FALSE);

        button = gtk_dialog_get_widget_for_response (dialog,
                                                     GTK_RESPONSE_CANCEL);
        gtk_button_set_label (GTK_BUTTON (button), _("Canceling..."));

        /* signal the cancellation request to the main app */
        g_clear_pointer (&context->write_channel, g_io_channel_unref);
      }
      break;

    default:
      break;
    }
}
