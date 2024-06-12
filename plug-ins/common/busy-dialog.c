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


typedef struct _BusyDialog      BusyDialog;
typedef struct _BusyDialogClass BusyDialogClass;

struct _BusyDialog
{
  GimpPlugIn parent_instance;
};

struct _BusyDialogClass
{
  GimpPlugInClass parent_class;
};


#define BUSY_DIALOG_TYPE  (busy_dialog_get_type ())
#define BUSY_DIALOG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUSY_DIALOG_TYPE, BusyDialog))

GType                     busy_dialog_get_type         (void) G_GNUC_CONST;


static GList             * busy_dialog_query_procedures    (GimpPlugIn           *plug_in);
static GimpProcedure     * busy_dialog_create_procedure    (GimpPlugIn           *plug_in,
                                                            const gchar          *name);
static GimpValueArray    * busy_dialog_run                 (GimpProcedure        *procedure,
                                                            GimpProcedureConfig  *config,
                                                            gpointer              run_data);

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


G_DEFINE_TYPE (BusyDialog, busy_dialog, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (BUSY_DIALOG_TYPE)
DEFINE_STD_SET_I18N

static void
busy_dialog_class_init (BusyDialogClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = busy_dialog_query_procedures;
  plug_in_class->create_procedure = busy_dialog_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
busy_dialog_init (BusyDialog *busy_dialog)
{
}

static GList *
busy_dialog_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
busy_dialog_create_procedure (GimpPlugIn  *plug_in,
                              const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      busy_dialog_run, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Show a dialog while waiting for an "
                                        "operation to finish",
                                        "Used by GIMP to display a dialog, "
                                        "containing a spinner and a custom "
                                        "message, while waiting for an "
                                        "ongoing operation to finish. "
                                        "Optionally, the dialog may provide "
                                        "a \"Cancel\" button, which can be used "
                                        "to cancel the operation.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Ell",
                                      "Ell",
                                      "2018");

      gimp_procedure_add_enum_argument (procedure, "run-mode",
                                        "Run mode",
                                        "The run mode",
                                        GIMP_TYPE_RUN_MODE,
                                        GIMP_RUN_INTERACTIVE,
                                        G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "read-fd",
                                       "The read file descriptor",
                                       "The read file descriptor",
                                       G_MININT, G_MAXINT, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "write-fd",
                                       "The write file descriptor",
                                       "The write file descriptor",
                                       G_MININT, G_MAXINT, 0,
                                       G_PARAM_READWRITE);


      gimp_procedure_add_string_argument (procedure, "message",
                                          "The message",
                                          "The message",
                                          NULL,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "cancelable",
                                           "Whether the dialog is cancelable",
                                           "Whether the dialog is cancelable",
                                           FALSE,
                                           G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
busy_dialog_run (GimpProcedure        *procedure,
                 GimpProcedureConfig  *config,
                 gpointer              run_data)
{
  GimpValueArray    *return_vals = NULL;
  GimpPDBStatusType  status      = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode    = GIMP_RUN_INTERACTIVE;
  gint               read_fd;
  gint               write_fd;
  const gchar       *message;
  gboolean           cancelable;

  g_object_get (config,
                "run-mode",   &run_mode,
                "read-fd",    &read_fd,
                "write-fd",   &write_fd,
                "message",    &message,
                "cancelable", &cancelable,
                NULL);
  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_NONINTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      status = busy_dialog (read_fd, write_fd, message, cancelable);
      break;

    default:
      status = GIMP_PDB_CALLING_ERROR;
      break;
    }

  return_vals = gimp_procedure_new_return_values (procedure, status, NULL);

  return return_vals;
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

  gimp_ui_init (PLUG_IN_BINARY);

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
