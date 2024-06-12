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


typedef struct _Print      Print;
typedef struct _PrintClass PrintClass;

struct _Print
{
  GimpPlugIn      parent_instance;
};

struct _PrintClass
{
  GimpPlugInClass parent_class;
};


#define PRINT_TYPE  (print_get_type ())
#define PRINT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PRINT_TYPE, Print))

GType                     print_get_type         (void) G_GNUC_CONST;

static GList            * print_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure    * print_create_procedure (GimpPlugIn           *plug_in,
                                                  const gchar          *name);

static GimpValueArray   * print_run              (GimpProcedure        *procedure,
                                                  GimpRunMode           run_mode,
                                                  GimpImage            *image,
                                                  gint                  n_drawables,
                                                  GimpDrawable        **drawables,
                                                  GimpProcedureConfig  *config,
                                                  gpointer              run_data);

static GimpPDBStatusType  print_image            (GimpImage         *image,
                                                  gboolean           interactive,
                                                  GError           **error);
#ifndef EMBED_PAGE_SETUP
static GimpPDBStatusType  page_setup             (GimpImage         *image);
#endif

static void        print_show_error         (const gchar       *message);
static void        print_operation_set_name (GtkPrintOperation *operation,
                                             GimpImage         *image);

static void        begin_print              (GtkPrintOperation *operation,
                                             GtkPrintContext   *context,
                                             PrintData         *data);
static void        end_print                (GtkPrintOperation *operation,
                                             GtkPrintContext   *context,
                                             GimpLayer        **layer);
static void        draw_page                (GtkPrintOperation *print,
                                             GtkPrintContext   *context,
                                             gint               page_nr,
                                             PrintData         *data);

static GtkWidget * create_custom_widget     (GtkPrintOperation *operation,
                                             PrintData         *data);

#ifndef EMBED_PAGE_SETUP
static gchar     * print_temp_proc_name     (GimpImage         *image);
static gchar     * print_temp_proc_install  (GimpImage         *image);

/*  Keep a reference to the current GtkPrintOperation
 *  for access by the temporary procedure.
 */
static GtkPrintOperation *print_operation = NULL;
#endif


G_DEFINE_TYPE (Print, print, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PRINT_TYPE)
DEFINE_STD_SET_I18N


static void
print_class_init (PrintClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = print_query_procedures;
  plug_in_class->create_procedure = print_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
print_init (Print *print)
{
}

static GList *
print_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (PRINT_PROC_NAME));

#ifndef EMBED_PAGE_SETUP
  list = g_list_append (list, g_strdup (PAGE_SETUP_PROC_NAME));
#endif

  return list;
}

static GimpProcedure *
print_create_procedure (GimpPlugIn  *plug_in,
                        const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PRINT_PROC_NAME))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            print_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES |
                                           GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      gimp_procedure_set_menu_label (procedure, _("_Print..."));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_DOCUMENT_PRINT);
      gimp_procedure_add_menu_path (procedure, "<Image>/File/[Send]");

      gimp_procedure_set_documentation (procedure,
                                        _("Print the image"),
                                        "Print the image using the "
                                        "GTK+ Print API.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Bill Skaggs, Sven Neumann, Stefan Röllin",
                                      "Bill Skaggs <weskaggs@primate.ucdavis.edu>",
                                      "2006 - 2008");

    }
#ifndef EMBED_PAGE_SETUP
  else if (! strcmp (name, PAGE_SETUP_PROC_NAME))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            print_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("Page Set_up..."));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_DOCUMENT_PAGE_SETUP);
      gimp_procedure_add_menu_path (procedure, "<Image>/File/[Send]");

      gimp_procedure_set_documentation (procedure,
                                        _("Adjust page size and orientation "
                                          "for printing"),
                                        "Adjust page size and orientation "
                                        "for printing the image using the "
                                        "GTK+ Print API.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Bill Skaggs, Sven Neumann, Stefan Röllin",
                                      "Sven Neumann <sven@gimp.org>",
                                      "2008");
    }
#endif

  return procedure;
}

static GimpValueArray *
print_run (GimpProcedure        *procedure,
           GimpRunMode           run_mode,
           GimpImage            *image,
           gint                  n_drawables,
           GimpDrawable        **drawables,
           GimpProcedureConfig  *config,
           gpointer              run_data)
{
  GimpPDBStatusType  status;
  GError            *error = NULL;

  gegl_init (NULL, NULL);

  if (strcmp (gimp_procedure_get_name (procedure),
              PRINT_PROC_NAME) == 0)
    {
      status = print_image (image, run_mode == GIMP_RUN_INTERACTIVE, &error);

      if (error && run_mode == GIMP_RUN_INTERACTIVE)
        {
          print_show_error (error->message);
        }
    }
#ifndef EMBED_PAGE_SETUP
  else if (strcmp (gimp_procedure_get_name (procedure),
                   PAGE_SETUP_PROC_NAME) == 0)
    {
      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          status = page_setup (image);
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

  return gimp_procedure_new_return_values (procedure, status, error);
}

static GimpPDBStatusType
print_image (GimpImage *image,
             gboolean   interactive,
             GError   **error)
{
  GtkPrintOperation       *operation;
  GtkPrintOperationResult  result;
  GimpLayer               *layer;
  PrintData                data;
#ifndef EMBED_PAGE_SETUP
  gchar                   *temp_proc;
#endif

  /*  create a print layer from the projection  */
  layer = gimp_layer_new_from_visible (image, image, PRINT_PROC_NAME);

  operation = gtk_print_operation_new ();

  gtk_print_operation_set_n_pages (operation, 1);
  print_operation_set_name (operation, image);

  print_page_setup_load (operation, image);

  /* fill in the PrintData struct */
  data.image           = image;
  data.drawable        = GIMP_DRAWABLE (layer);
  data.unit            = gimp_get_default_unit ();
  data.image_unit      = gimp_image_get_unit (image);
  data.offset_x        = 0;
  data.offset_y        = 0;
  data.center          = CENTER_BOTH;
  data.use_full_page   = FALSE;
  data.draw_crop_marks = FALSE;
  data.operation       = operation;

  gimp_image_get_resolution (image, &data.xres, &data.yres);

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
  temp_proc = print_temp_proc_install (image);
  gimp_plug_in_extension_enable (gimp_get_plug_in ());
#endif

  if (interactive)
    {
      gimp_ui_init (PLUG_IN_BINARY);

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
          print_page_setup_save (operation, image);
        }
    }
  else
    {
      result = gtk_print_operation_run (operation,
                                        GTK_PRINT_OPERATION_ACTION_PRINT,
                                        NULL, error);
    }

#ifndef EMBED_PAGE_SETUP
  gimp_plug_in_remove_temp_procedure (gimp_get_plug_in (), temp_proc);
  g_free (temp_proc);
  print_operation = NULL;
#endif

  g_object_unref (operation);

  if (gimp_item_is_valid (GIMP_ITEM (layer)))
    gimp_item_delete (GIMP_ITEM (layer));

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
page_setup (GimpImage *image)
{
  GtkPrintOperation *operation;
  GimpProcedure     *procedure;
  GimpValueArray    *return_vals;
  gchar             *name;

  gimp_ui_init (PLUG_IN_BINARY);

  operation = gtk_print_operation_new ();

  print_page_setup_load (operation, image);
  print_page_setup_dialog (operation);
  print_page_setup_save (operation, image);

  g_object_unref (operation);

  /* now notify a running print procedure about this change */
  name = print_temp_proc_name (image);

  /* we don't want the core to show an error message if the
   * temporary procedure does not exist
   */
  gimp_plug_in_set_pdb_error_handler (gimp_get_plug_in (),
                                      GIMP_PDB_ERROR_HANDLER_PLUGIN);

  /* Notify the Print plug-in if Page Setup was called from there */
  procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (), name);
  if (procedure)
    {
      return_vals = gimp_procedure_run (procedure, "image", image, NULL);
      gimp_value_array_unref (return_vals);
    }

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
                          GimpImage         *image)
{
  gchar *name = gimp_image_get_name (image);

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
           GimpLayer        **layer)
{
  /* we don't need the print layer any longer, delete it */
  if (gimp_item_is_valid (GIMP_ITEM (*layer)))
    {
      gimp_item_delete (GIMP_ITEM (*layer));
      *layer = NULL;
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
static GimpValueArray *
print_temp_proc_run (GimpProcedure        *procedure,
                     GimpProcedureConfig  *config,
                     gpointer              run_data)
{
  GimpImage *image;

  g_object_get (config, "image", &image, NULL);

  if (print_operation)
    print_page_setup_load (print_operation, image);

  g_object_unref (image);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gchar *
print_temp_proc_name (GimpImage *image)
{
  return g_strdup_printf (PRINT_TEMP_PROC_NAME "-%d",
                          gimp_image_get_id (image));
}

static gchar *
print_temp_proc_install (GimpImage *image)
{
  GimpPlugIn    *plug_in = gimp_get_plug_in ();
  gchar         *name    = print_temp_proc_name (image);
  GimpProcedure *procedure;

  procedure = gimp_procedure_new (plug_in, name,
                                  GIMP_PDB_PROC_TYPE_TEMPORARY,
                                  print_temp_proc_run, NULL, NULL);

  gimp_procedure_set_documentation (procedure,
                                    "DON'T USE THIS ONE",
                                    "Temporary procedure to notify the "
                                    "Print plug-in about changes to the "
                                    "Page Setup.",
                                    NULL);
  gimp_procedure_set_attribution (procedure,
                                  "Sven Neumann",
                                  "Sven Neumann",
                                  "2008");

  gimp_procedure_add_image_argument (procedure, "image",
                                     "Image",
                                     "The image to notify about",
                                     FALSE,
                                     G_PARAM_READWRITE);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);

  return name;
}
#endif
