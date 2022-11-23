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

#include <string.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "print.h"
#include "print-settings.h"
#include "print-page-layout.h"
#include "print-page-setup.h"
#include "print-draw-page.h"

#include "libligma/stdplugins-intl.h"


#define PLUG_IN_BINARY       "print"
#define PLUG_IN_ROLE         "ligma-print"
#define PRINT_PROC_NAME      "file-print-gtk"

#ifndef EMBED_PAGE_SETUP
#define PAGE_SETUP_PROC_NAME "file-print-gtk-page-setup"
#define PRINT_TEMP_PROC_NAME "file-print-gtk-page-setup-notify-temp"
#endif


G_DEFINE_QUARK (ligma-plugin-print-error-quark, ligma_plugin_print_error)


typedef struct _Print      Print;
typedef struct _PrintClass PrintClass;

struct _Print
{
  LigmaPlugIn      parent_instance;
};

struct _PrintClass
{
  LigmaPlugInClass parent_class;
};


#define PRINT_TYPE  (print_get_type ())
#define PRINT (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PRINT_TYPE, Print))

GType                     print_get_type         (void) G_GNUC_CONST;

static GList            * print_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure    * print_create_procedure (LigmaPlugIn           *plug_in,
                                                  const gchar          *name);

static LigmaValueArray   * print_run              (LigmaProcedure        *procedure,
                                                  LigmaRunMode           run_mode,
                                                  LigmaImage            *image,
                                                  gint                  n_drawables,
                                                  LigmaDrawable        **drawables,
                                                  const LigmaValueArray *args,
                                                  gpointer              run_data);

static LigmaPDBStatusType  print_image            (LigmaImage         *image,
                                                  gboolean           interactive,
                                                  GError           **error);
#ifndef EMBED_PAGE_SETUP
static LigmaPDBStatusType  page_setup             (LigmaImage         *image);
#endif

static void        print_show_error         (const gchar       *message);
static void        print_operation_set_name (GtkPrintOperation *operation,
                                             LigmaImage         *image);

static void        begin_print              (GtkPrintOperation *operation,
                                             GtkPrintContext   *context,
                                             PrintData         *data);
static void        end_print                (GtkPrintOperation *operation,
                                             GtkPrintContext   *context,
                                             LigmaLayer        **layer);
static void        draw_page                (GtkPrintOperation *print,
                                             GtkPrintContext   *context,
                                             gint               page_nr,
                                             PrintData         *data);

static GtkWidget * create_custom_widget     (GtkPrintOperation *operation,
                                             PrintData         *data);

#ifndef EMBED_PAGE_SETUP
static gchar     * print_temp_proc_name     (LigmaImage         *image);
static gchar     * print_temp_proc_install  (LigmaImage         *image);

/*  Keep a reference to the current GtkPrintOperation
 *  for access by the temporary procedure.
 */
static GtkPrintOperation *print_operation = NULL;
#endif


G_DEFINE_TYPE (Print, print, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (PRINT_TYPE)
DEFINE_STD_SET_I18N


static void
print_class_init (PrintClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = print_query_procedures;
  plug_in_class->create_procedure = print_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
print_init (Print *print)
{
}

static GList *
print_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (PRINT_PROC_NAME));

#ifndef EMBED_PAGE_SETUP
  list = g_list_append (list, g_strdup (PAGE_SETUP_PROC_NAME));
#endif

  return list;
}

static LigmaProcedure *
print_create_procedure (LigmaPlugIn  *plug_in,
                        const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PRINT_PROC_NAME))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            print_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLES |
                                           LIGMA_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      ligma_procedure_set_menu_label (procedure, _("_Print..."));
      ligma_procedure_set_icon_name (procedure, LIGMA_ICON_DOCUMENT_PRINT);
      ligma_procedure_add_menu_path (procedure, "<Image>/File/Send");

      ligma_procedure_set_documentation (procedure,
                                        _("Print the image"),
                                        "Print the image using the "
                                        "GTK+ Print API.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Bill Skaggs, Sven Neumann, Stefan Röllin",
                                      "Bill Skaggs <weskaggs@primate.ucdavis.edu>",
                                      "2006 - 2008");

    }
#ifndef EMBED_PAGE_SETUP
  else if (! strcmp (name, PAGE_SETUP_PROC_NAME))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            print_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "*");

      ligma_procedure_set_menu_label (procedure, _("Page Set_up..."));
      ligma_procedure_set_icon_name (procedure, LIGMA_ICON_DOCUMENT_PAGE_SETUP);
      ligma_procedure_add_menu_path (procedure, "<Image>/File/Send");

      ligma_procedure_set_documentation (procedure,
                                        _("Adjust page size and orientation "
                                          "for printing"),
                                        "Adjust page size and orientation "
                                        "for printing the image using the "
                                        "GTK+ Print API.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Bill Skaggs, Sven Neumann, Stefan Röllin",
                                      "Sven Neumann <sven@ligma.org>",
                                      "2008");
    }
#endif

  return procedure;
}

static LigmaValueArray *
print_run (LigmaProcedure        *procedure,
           LigmaRunMode           run_mode,
           LigmaImage            *image,
           gint                  n_drawables,
           LigmaDrawable        **drawables,
           const LigmaValueArray *args,
           gpointer              run_data)
{
  LigmaPDBStatusType  status;
  GError            *error = NULL;

  gegl_init (NULL, NULL);

  if (strcmp (ligma_procedure_get_name (procedure),
              PRINT_PROC_NAME) == 0)
    {
      status = print_image (image, run_mode == LIGMA_RUN_INTERACTIVE, &error);

      if (error && run_mode == LIGMA_RUN_INTERACTIVE)
        {
          print_show_error (error->message);
        }
    }
#ifndef EMBED_PAGE_SETUP
  else if (strcmp (ligma_procedure_get_name (procedure),
                   PAGE_SETUP_PROC_NAME) == 0)
    {
      if (run_mode == LIGMA_RUN_INTERACTIVE)
        {
          status = page_setup (image);
        }
      else
        {
          status = LIGMA_PDB_CALLING_ERROR;
        }
    }
#endif
  else
    {
      status = LIGMA_PDB_CALLING_ERROR;
    }

  return ligma_procedure_new_return_values (procedure, status, error);
}

static LigmaPDBStatusType
print_image (LigmaImage *image,
             gboolean   interactive,
             GError   **error)
{
  GtkPrintOperation       *operation;
  GtkPrintOperationResult  result;
  LigmaLayer               *layer;
  PrintData                data;
#ifndef EMBED_PAGE_SETUP
  gchar                   *temp_proc;
#endif

  /*  create a print layer from the projection  */
  layer = ligma_layer_new_from_visible (image, image, PRINT_PROC_NAME);

  operation = gtk_print_operation_new ();

  gtk_print_operation_set_n_pages (operation, 1);
  print_operation_set_name (operation, image);

  print_page_setup_load (operation, image);

  /* fill in the PrintData struct */
  data.image           = image;
  data.drawable        = LIGMA_DRAWABLE (layer);
  data.unit            = ligma_get_default_unit ();
  data.image_unit      = ligma_image_get_unit (image);
  data.offset_x        = 0;
  data.offset_y        = 0;
  data.center          = CENTER_BOTH;
  data.use_full_page   = FALSE;
  data.draw_crop_marks = FALSE;
  data.operation       = operation;

  ligma_image_get_resolution (image, &data.xres, &data.yres);

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
  ligma_plug_in_extension_enable (ligma_get_plug_in ());
#endif

  if (interactive)
    {
      ligma_ui_init (PLUG_IN_BINARY);

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
  ligma_plug_in_remove_temp_procedure (ligma_get_plug_in (), temp_proc);
  g_free (temp_proc);
  print_operation = NULL;
#endif

  g_object_unref (operation);

  if (ligma_item_is_valid (LIGMA_ITEM (layer)))
    ligma_item_delete (LIGMA_ITEM (layer));

  switch (result)
    {
    case GTK_PRINT_OPERATION_RESULT_APPLY:
    case GTK_PRINT_OPERATION_RESULT_IN_PROGRESS:
      return LIGMA_PDB_SUCCESS;

    case GTK_PRINT_OPERATION_RESULT_CANCEL:
      return LIGMA_PDB_CANCEL;

    case GTK_PRINT_OPERATION_RESULT_ERROR:
      return LIGMA_PDB_EXECUTION_ERROR;
    }

  return LIGMA_PDB_EXECUTION_ERROR;
}

#ifndef EMBED_PAGE_SETUP
static LigmaPDBStatusType
page_setup (LigmaImage *image)
{
  GtkPrintOperation  *operation;
  LigmaValueArray     *return_vals;
  gchar              *name;

  ligma_ui_init (PLUG_IN_BINARY);

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
  ligma_plug_in_set_pdb_error_handler (ligma_get_plug_in (),
                                      LIGMA_PDB_ERROR_HANDLER_PLUGIN);

  return_vals = ligma_pdb_run_procedure (ligma_get_pdb (),
                                        name,
                                        LIGMA_TYPE_IMAGE, image,
                                        G_TYPE_NONE);
  ligma_value_array_unref (return_vals);

  g_free (name);

  return LIGMA_PDB_SUCCESS;
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
                          LigmaImage         *image)
{
  gchar *name = ligma_image_get_name (image);

  gtk_print_operation_set_job_name (operation, name);

  g_free (name);
}

static void
begin_print (GtkPrintOperation *operation,
             GtkPrintContext   *context,
             PrintData         *data)
{
  gtk_print_operation_set_use_full_page (operation, data->use_full_page);

  ligma_progress_init (_("Printing"));
}

static void
end_print (GtkPrintOperation *operation,
           GtkPrintContext   *context,
           LigmaLayer        **layer)
{
  /* we don't need the print layer any longer, delete it */
  if (ligma_item_is_valid (LIGMA_ITEM (*layer)))
    {
      ligma_item_delete (LIGMA_ITEM (*layer));
      *layer = NULL;
    }

  ligma_progress_end ();

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
      ligma_progress_update (1.0);
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
static LigmaValueArray *
print_temp_proc_run (LigmaProcedure        *procedure,
                     const LigmaValueArray *args,
                     gpointer              run_data)
{
  LigmaImage *image = LIGMA_VALUES_GET_IMAGE (args, 0);

  if (print_operation)
    print_page_setup_load (print_operation, image);

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}

static gchar *
print_temp_proc_name (LigmaImage *image)
{
  return g_strdup_printf (PRINT_TEMP_PROC_NAME "-%d", image);
}

static gchar *
print_temp_proc_install (LigmaImage *image)
{
  LigmaPlugIn    *plug_in = ligma_get_plug_in ();
  gchar         *name    = print_temp_proc_name (image);
  LigmaProcedure *procedure;

  procedure = ligma_procedure_new (plug_in, name,
                                  LIGMA_PDB_PROC_TYPE_TEMPORARY,
                                  print_temp_proc_run, NULL, NULL);

  ligma_procedure_set_documentation (procedure,
                                    "DON'T USE THIS ONE",
                                    "Temporary procedure to notify the "
                                    "Print plug-in about changes to the "
                                    "Page Setup.",
                                    NULL);
  ligma_procedure_set_attribution (procedure,
                                  "Sven Neumann",
                                  "Sven Neumann",
                                  "2008");

  LIGMA_PROC_ARG_IMAGE (procedure, "image",
                       "Image",
                       "The image to notify about",
                       FALSE,
                       G_PARAM_READWRITE);

  ligma_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);

  return name;
}
#endif
