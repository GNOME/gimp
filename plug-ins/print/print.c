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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "print.h"
#include "print-settings.h"
#include "print-page-layout.h"
#include "print-page-setup.h"
#include "print-draw-page.h"

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_BINARY       "print"
#define PLUG_IN_ROLE         "gimp-print"
#define PRINT_PROC_NAME      "file-print-gtk"

#ifndef EMBED_PAGE_SETUP
#define PAGE_SETUP_PROC_NAME "file-print-gtk-page-setup"
#define PRINT_TEMP_PROC_NAME "file-print-gtk-page-setup-notify-temp"
#endif

G_DEFINE_QUARK (gimp-plugin-print-error-quark, gimp_plugin_print_error)

static void        query (void);
static void        run   (const gchar       *name,
                          gint               nparams,
                          const GimpParam   *param,
                          gint              *nreturn_vals,
                          GimpParam        **return_vals);

static GimpPDBStatusType  print_image       (gint32             image_ID,
                                             gboolean           interactive,
                                             GError           **error);
#ifndef EMBED_PAGE_SETUP
static GimpPDBStatusType  page_setup        (gint32             image_ID);
#endif

static void        print_show_error         (const gchar       *message);
static void        print_operation_set_name (GtkPrintOperation *operation,
                                             gint               image_ID);

static void        begin_print              (GtkPrintOperation *operation,
                                             GtkPrintContext   *context,
                                             PrintData         *data);
static void        end_print                (GtkPrintOperation *operation,
                                             GtkPrintContext   *context,
                                             gint32            *layer_ID);
static void        draw_page                (GtkPrintOperation *print,
                                             GtkPrintContext   *context,
                                             gint               page_nr,
                                             PrintData         *data);

static GtkWidget * create_custom_widget     (GtkPrintOperation *operation,
                                             PrintData         *data);

#ifndef EMBED_PAGE_SETUP
static gchar     * print_temp_proc_name     (gint32             image_ID);
static gchar     * print_temp_proc_install  (gint32             image_ID);

/*  Keep a reference to the current GtkPrintOperation
 *  for access by the temporary procedure.
 */
static GtkPrintOperation *print_operation = NULL;
#endif


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
  static const GimpParamDef print_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0) }" },
    { GIMP_PDB_IMAGE,    "image",    "Image to print"                       }
  };

  gimp_install_procedure (PRINT_PROC_NAME,
                          N_("Print the image"),
                          "Print the image using the GTK+ Print API.",
                          "Bill Skaggs, Sven Neumann, Stefan Röllin",
                          "Bill Skaggs <weskaggs@primate.ucdavis.edu>",
                          "2006 - 2008",
                          N_("_Print..."),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (print_args), 0,
                          print_args, NULL);

  gimp_plugin_menu_register (PRINT_PROC_NAME, "<Image>/File/Send");
  gimp_plugin_icon_register (PRINT_PROC_NAME, GIMP_ICON_TYPE_ICON_NAME,
                             (const guint8 *) GIMP_ICON_DOCUMENT_PRINT);

#ifndef EMBED_PAGE_SETUP
  gimp_install_procedure (PAGE_SETUP_PROC_NAME,
                          N_("Adjust page size and orientation for printing"),
                          "Adjust page size and orientation for printing the "
                          "image using the GTK+ Print API.",
                          "Bill Skaggs, Sven Neumann, Stefan Röllin",
                          "Sven Neumann <sven@gimp.org>",
                          "2008",
                          N_("Page Set_up..."),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (print_args), 0,
                          print_args, NULL);

  gimp_plugin_menu_register (PAGE_SETUP_PROC_NAME, "<Image>/File/Send");
  gimp_plugin_icon_register (PAGE_SETUP_PROC_NAME, GIMP_ICON_TYPE_ICON_NAME,
                             (const guint8 *) GIMP_ICON_DOCUMENT_PAGE_SETUP);
#endif
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status;
  gint32             image_ID;
  GError            *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  image_ID = param[1].data.d_int32;

  if (strcmp (name, PRINT_PROC_NAME) == 0)
    {
      status = print_image (image_ID, run_mode == GIMP_RUN_INTERACTIVE, &error);

      if (error && run_mode == GIMP_RUN_INTERACTIVE)
        {
          print_show_error (error->message);
        }
    }
#ifndef EMBED_PAGE_SETUP
  else if (strcmp (name, PAGE_SETUP_PROC_NAME) == 0)
    {
      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          status = page_setup (image_ID);
        }
      else
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
    }
#endif
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}

static GimpPDBStatusType
print_image (gint32     image_ID,
             gboolean   interactive,
             GError   **error)
{
  GtkPrintOperation       *operation;
  GtkPrintOperationResult  result;
  gint32                   layer;
  PrintData                data;
#ifndef EMBED_PAGE_SETUP
  gchar                   *temp_proc;
#endif

  /*  create a print layer from the projection  */
  layer = gimp_layer_new_from_visible (image_ID, image_ID, PRINT_PROC_NAME);

  operation = gtk_print_operation_new ();

  gtk_print_operation_set_n_pages (operation, 1);
  print_operation_set_name (operation, image_ID);

  print_page_setup_load (operation, image_ID);

  /* fill in the PrintData struct */
  data.image_id        = image_ID;
  data.drawable_id     = layer;
  data.unit            = gimp_get_default_unit ();
  data.image_unit      = gimp_image_get_unit (image_ID);
  data.offset_x        = 0;
  data.offset_y        = 0;
  data.center          = CENTER_BOTH;
  data.use_full_page   = FALSE;
  data.draw_crop_marks = FALSE;
  data.operation       = operation;

  gimp_image_get_resolution (image_ID, &data.xres, &data.yres);

  print_settings_load (&data);

  gtk_print_operation_set_unit (operation, GTK_UNIT_PIXEL);

  g_signal_connect (operation, "begin-print",
                    G_CALLBACK (begin_print),
                    &data);
  g_signal_connect (operation, "draw-page",
                    G_CALLBACK (draw_page),
                    &data);
  g_signal_connect (operation, "end-print",
                    G_CALLBACK (end_print),
                    &layer);

#ifndef EMBED_PAGE_SETUP
  print_operation = operation;
  temp_proc = print_temp_proc_install (image_ID);
  gimp_extension_enable ();
#endif

  if (interactive)
    {
      gimp_ui_init (PLUG_IN_BINARY, FALSE);

      g_signal_connect_swapped (operation, "end-print",
                                G_CALLBACK (print_settings_save),
                                &data);

      g_signal_connect (operation, "create-custom-widget",
                        G_CALLBACK (create_custom_widget),
                        &data);

      gtk_print_operation_set_custom_tab_label (operation, _("Image Settings"));

#ifdef EMBED_PAGE_SETUP
      gtk_print_operation_set_embed_page_setup (operation, TRUE);
#endif

      result = gtk_print_operation_run (operation,
                                        GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
                                        NULL, error);

      if (result == GTK_PRINT_OPERATION_RESULT_APPLY ||
          result == GTK_PRINT_OPERATION_RESULT_IN_PROGRESS)
        {
          print_page_setup_save (operation, image_ID);
        }
    }
  else
    {
      result = gtk_print_operation_run (operation,
                                        GTK_PRINT_OPERATION_ACTION_PRINT,
                                        NULL, error);
    }

#ifndef EMBED_PAGE_SETUP
  gimp_uninstall_temp_proc (temp_proc);
  g_free (temp_proc);
  print_operation = NULL;
#endif

  g_object_unref (operation);

  if (gimp_item_is_valid (layer))
    gimp_item_delete (layer);

  switch (result)
    {
    case GTK_PRINT_OPERATION_RESULT_APPLY:
    case GTK_PRINT_OPERATION_RESULT_IN_PROGRESS:
      return GIMP_PDB_SUCCESS;

    case GTK_PRINT_OPERATION_RESULT_CANCEL:
      return GIMP_PDB_CANCEL;

    case GTK_PRINT_OPERATION_RESULT_ERROR:
      return GIMP_PDB_EXECUTION_ERROR;
    }

  return GIMP_PDB_EXECUTION_ERROR;
}

#ifndef EMBED_PAGE_SETUP
static GimpPDBStatusType
page_setup (gint32 image_ID)
{
  GtkPrintOperation  *operation;
  GimpParam          *return_vals;
  gchar              *name;
  gint                n_return_vals;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  operation = gtk_print_operation_new ();

  print_page_setup_load (operation, image_ID);
  print_page_setup_dialog (operation);
  print_page_setup_save (operation, image_ID);

  g_object_unref (operation);

  /* now notify a running print procedure about this change */
  name = print_temp_proc_name (image_ID);

  /* we don't want the core to show an error message if the
   * temporary procedure does not exist
   */
  gimp_plugin_set_pdb_error_handler (GIMP_PDB_ERROR_HANDLER_PLUGIN);

  return_vals = gimp_run_procedure (name,
                                    &n_return_vals,
                                    GIMP_PDB_IMAGE, image_ID,
                                    GIMP_PDB_END);
  gimp_destroy_params (return_vals, n_return_vals);

  g_free (name);

  return GIMP_PDB_SUCCESS;
}
#endif

static void
print_show_error (const gchar *message)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (NULL, 0,
                                   GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_OK,
                                   "%s",
                                   _("An error occurred while trying to print:"));

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            "%s", message);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
print_operation_set_name (GtkPrintOperation *operation,
                          gint               image_ID)
{
  gchar *name = gimp_image_get_name (image_ID);

  gtk_print_operation_set_job_name (operation, name);

  g_free (name);
}

static void
begin_print (GtkPrintOperation *operation,
             GtkPrintContext   *context,
             PrintData         *data)
{
  gtk_print_operation_set_use_full_page (operation, data->use_full_page);

  gimp_progress_init (_("Printing"));
}

static void
end_print (GtkPrintOperation *operation,
           GtkPrintContext   *context,
           gint32            *layer_ID)
{
  /* we don't need the print layer any longer, delete it */
  if (gimp_item_is_valid (*layer_ID))
    {
      gimp_item_delete (*layer_ID);
      *layer_ID = -1;
    }

  gimp_progress_end ();

  /* generate events to solve the problems described in bug #466928 */
  g_timeout_add_seconds (1, (GSourceFunc) gtk_true, NULL);
}

static void
draw_page (GtkPrintOperation *operation,
           GtkPrintContext   *context,
           gint               page_nr,
           PrintData         *data)
{
  GError *error = NULL;

  if (print_draw_page (context, data, &error))
    {
      gimp_progress_update (1.0);
    }
  else
    {
      print_show_error (error->message);
      g_error_free (error);
    }
}


/*
 * This callback creates a "custom" widget that gets inserted into the
 * print operation dialog.
 */
static GtkWidget *
create_custom_widget (GtkPrintOperation *operation,
                      PrintData         *data)
{
  return print_page_layout_gui (data, PRINT_PROC_NAME);
}

#ifndef EMBED_PAGE_SETUP
static void
print_temp_proc_run (const gchar      *name,
                     gint              nparams,
                     const GimpParam  *param,
                     gint             *nreturn_vals,
                     GimpParam       **return_vals)
{
  static GimpParam  values[1];

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;

  if (print_operation && nparams == 1)
    print_page_setup_load (print_operation, param[0].data.d_int32);
}

static gchar *
print_temp_proc_name (gint32 image_ID)
{
  return g_strdup_printf (PRINT_TEMP_PROC_NAME "-%d", image_ID);
}

static gchar *
print_temp_proc_install (gint32  image_ID)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_IMAGE, "image", "Image to print" }
  };

  gchar *name = print_temp_proc_name (image_ID);

  gimp_install_temp_proc (name,
                          "DON'T USE THIS ONE",
                          "Temporary procedure to notify the Print plug-in "
                          "about changes to the Page Setup.",
                         "Sven Neumann",
                         "Sven Neumann",
                         "2008",
                          NULL,
                          "",
                          GIMP_TEMPORARY,
                          G_N_ELEMENTS (args), 0, args, NULL,
                          print_temp_proc_run);

  return name;
}
#endif
