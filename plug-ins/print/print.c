/* GIMP - The GNU Image Manipulation Program
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <glib/gstdio.h>

#include "print.h"
#include "print-settings.h"
#include "print-page-layout.h"
#include "print-draw-page.h"

#include "libgimp/stdplugins-intl.h"


#define PRINT_PROC_NAME  "file-print-gtk"
#define PLUG_IN_BINARY   "print"


static void        query (void);
static void        run   (const gchar       *name,
                          gint               nparams,
                          const GimpParam   *param,
                          gint              *nreturn_vals,
                          GimpParam        **return_vals);

static gboolean    print_image          (gint32             image_ID,
                                         gint32             drawable_ID,
                                         gboolean           interactive);

static void        begin_print          (GtkPrintOperation *operation,
                                         GtkPrintContext   *context,
                                         PrintData         *data);

static void        end_print            (GtkPrintOperation *operation,
                                         GtkPrintContext   *context,
                                         PrintData         *data);

static void        draw_page            (GtkPrintOperation *print,
                                         GtkPrintContext   *context,
                                         int                page_nr,
                                         PrintData         *data);

static GtkWidget * create_custom_widget (GtkPrintOperation *operation,
                                         PrintData         *data);

static void        custom_widget_apply  (GtkPrintOperation *operation,
                                         GtkWidget         *widget,
                                         PrintData         *data);

static gboolean    print_preview        (GtkPrintOperation        *operation,
                                         GtkPrintOperationPreview *preview,
                                         GtkPrintContext          *context,
                                         GtkWindow                *parent,
                                         PrintData                *data);

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
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable", "Drawable to print"            }
  };

  gimp_install_procedure (PRINT_PROC_NAME,
                          N_("Print the image"),
                          "Print the image using the GTK+ Print API.",
                          "Bill Skaggs  <weskaggs@primate.ucdavis.edu>",
                          "Bill Skaggs  <weskaggs@primate.ucdavis.edu>",
                          "2006",
                          N_("_Print..."),
                          "GRAY, RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (print_args), 0,
                          print_args, NULL);

  gimp_plugin_menu_register (PRINT_PROC_NAME, "<Image>/File/Send");
  gimp_plugin_icon_register (PRINT_PROC_NAME, GIMP_ICON_TYPE_STOCK_ID,
                             (const guint8 *) GTK_STOCK_PRINT);

}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            image_ID;
  gint32            drawable_ID;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  image_ID    = param[1].data.d_int32;
  drawable_ID = param[2].data.d_int32;

  if (strcmp (name, PRINT_PROC_NAME) == 0)
    {
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_ui_init (PLUG_IN_BINARY, FALSE);

      if (! print_image (image_ID, drawable_ID,
                         run_mode == GIMP_RUN_INTERACTIVE))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static gboolean
print_image (gint32    image_ID,
             gint32    drawable_ID,
             gboolean  interactive)
{
  GtkPrintOperation *operation = gtk_print_operation_new ();
  GError            *error     = NULL;
  PrintData         *data;

  data = g_new0 (PrintData, 1);

  data->num_pages          = 1;
  data->image_id           = image_ID;
  data->drawable_id        = drawable_ID;
  data->operation          = operation;
  data->print_size_changed = FALSE;

  gimp_image_get_resolution (data->image_id, &data->xres, &data->yres);

  load_print_settings (data);

  g_signal_connect (operation, "begin-print",
                    G_CALLBACK (begin_print),
                    data);
  g_signal_connect (operation, "draw-page",
                    G_CALLBACK (draw_page),
                    data);
  g_signal_connect (operation, "end-print",
                    G_CALLBACK (end_print),
                    data);

  if (interactive)
    {
      GtkPrintOperationResult  result;

      g_signal_connect (operation, "create-custom-widget",
                        G_CALLBACK (create_custom_widget),
                        data);
      g_signal_connect (operation, "custom-widget-apply",
                        G_CALLBACK (custom_widget_apply),
                        data);
      g_signal_connect (operation, "preview",
                        G_CALLBACK (print_preview),
                        data);

      gtk_print_operation_set_custom_tab_label (operation, _("Layout"));

      result = gtk_print_operation_run (operation,
                                        GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
                                        NULL, &error);

      switch (result)
        {
        case GTK_PRINT_OPERATION_RESULT_APPLY:
        case GTK_PRINT_OPERATION_RESULT_IN_PROGRESS:
          save_print_settings (data);
          break;

        case GTK_PRINT_OPERATION_RESULT_ERROR:
        case GTK_PRINT_OPERATION_RESULT_CANCEL:
          break;
        }
    }
  else
    {
      gtk_print_operation_run (operation,
                               GTK_PRINT_OPERATION_ACTION_PRINT,
                               NULL, &error);
    }

  g_object_unref (operation);

  if (error)
    {
      g_message (error->message);
      g_error_free (error);
    }

  return TRUE;
}

static void
begin_print (GtkPrintOperation *operation,
             GtkPrintContext   *context,
             PrintData         *data)
{
  data->num_pages = 1;

  gtk_print_operation_set_n_pages (operation, data->num_pages);
}

static void
end_print (GtkPrintOperation *operation,
           GtkPrintContext   *context,
           PrintData         *data)
{
  GtkPrintSettings *settings;

  settings = gtk_print_operation_get_print_settings (operation);
}


static void
draw_page (GtkPrintOperation *operation,
           GtkPrintContext   *context,
           int                page_nr,
           PrintData         *data)
{
  draw_page_cairo (context, data);
}

/*
 * This callback creates a "custom" widget that gets inserted into the
 * print operation dialog.
 */
static GtkWidget *
create_custom_widget (GtkPrintOperation *operation,
                      PrintData         *data)
{
  return print_page_layout_gui (data);
}

/*
 * This function is called once before printing begins, and should be
 * used to apply any changes that have been made to the contents of the
 * custom widget, since it is not guaranteed to be around later.  This
 * function is guaranteed to be called at least once, even if printing
 * is cancelled.
 */
static void
custom_widget_apply (GtkPrintOperation *operation,
                     GtkWidget         *widget,
                     PrintData         *data)
{
  if (data->print_size_changed)
    {
      gimp_image_undo_group_start (data->image_id);

      gimp_image_set_resolution (data->image_id, data->xres, data->yres);
      gimp_image_set_unit (data->image_id, data->unit);

      gimp_image_undo_group_end (data->image_id);
    }
}


#define PREVIEW_SCALE 72

static gboolean
print_preview (GtkPrintOperation        *operation,
               GtkPrintOperationPreview *preview,
               GtkPrintContext          *context,
               GtkWindow                *parent,
               PrintData                *data)
{
  GtkPageSetup       *page_setup = gtk_print_context_get_page_setup (context);
  GtkPaperSize       *paper_size;
  gdouble             paper_width;
  gdouble             paper_height;
  gdouble             top_margin;
  gdouble             bottom_margin;
  gdouble             left_margin;
  gdouble             right_margin;
  gint                preview_width;
  gint                preview_height;
  cairo_t            *cr;
  cairo_surface_t    *surface;
  GtkPageOrientation  orientation;

  paper_size    = gtk_page_setup_get_paper_size (page_setup);
  paper_width   = gtk_paper_size_get_width (paper_size, GTK_UNIT_INCH);
  paper_height  = gtk_paper_size_get_height (paper_size, GTK_UNIT_INCH);
  top_margin    = gtk_page_setup_get_top_margin (page_setup, GTK_UNIT_INCH);
  bottom_margin = gtk_page_setup_get_bottom_margin (page_setup, GTK_UNIT_INCH);
  left_margin   = gtk_page_setup_get_left_margin (page_setup, GTK_UNIT_INCH);
  right_margin  = gtk_page_setup_get_right_margin (page_setup, GTK_UNIT_INCH);

  /* the print context does not have the page orientation, it is transformed */
  orientation   = data->orientation;

  if (orientation == GTK_PAGE_ORIENTATION_PORTRAIT ||
      orientation == GTK_PAGE_ORIENTATION_REVERSE_PORTRAIT)
    {
      preview_width  = PREVIEW_SCALE * paper_width;
      preview_height = PREVIEW_SCALE * paper_height;
    }
  else
    {
      preview_width  = PREVIEW_SCALE * paper_height;
      preview_height = PREVIEW_SCALE * paper_width;
    }

  surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
                                        preview_width, preview_height);

  if (CAIRO_STATUS_SUCCESS != cairo_surface_status (surface))
    {
      g_message ("Unable to create preview (not enough memory?)");
      return TRUE;
    }


  cr = cairo_create (surface);
  gtk_print_context_set_cairo_context (context,
                                       cr, PREVIEW_SCALE, PREVIEW_SCALE);

  /* fill page with white */
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_new_path (cr);
  cairo_rectangle (cr, 0, 0, preview_width, preview_height);
  cairo_fill (cr);

  cairo_translate (cr,
                   left_margin * PREVIEW_SCALE, right_margin * PREVIEW_SCALE);

  if (draw_page_cairo (context, data))
    {
      cairo_status_t   status;
      gchar           *filename;

      filename = gimp_temp_name ("png");
      status = cairo_surface_write_to_png (surface, filename);
      cairo_destroy (cr);
      cairo_surface_destroy (surface);

      if (status == CAIRO_STATUS_SUCCESS)
        {
          GtkWidget *dialog;
          GtkWidget *image;

          /* FIXME: add a Print action to the Preview dialog */

          dialog = gtk_dialog_new ();
          gtk_window_set_title (GTK_WINDOW (dialog), _("Print Preview"));

          image = gtk_image_new_from_file (filename);
          gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                              image, FALSE, FALSE, 0);
          gtk_widget_show (image);

          gtk_dialog_run (GTK_DIALOG (dialog));

          gtk_widget_destroy (dialog);
        }

      g_unlink (filename);
      g_free (filename);
    }

  return TRUE;
}
