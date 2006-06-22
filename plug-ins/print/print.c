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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PROC_NAME        "file-print-gtk"
#define PLUG_IN_BINARY   "gtkprint"

static void      query          (void);
static void      run            (const gchar       *name,
                                 gint               nparams,
                                 const GimpParam   *param,
                                 gint              *nreturn_vals,
                                 GimpParam        **return_vals);

static gboolean  print_image    (gint32             image_ID,
                                 gint32             drawable_ID,
                                 gboolean           interactive);

static void      begin_print    (GtkPrintOperation *print,
                                 GtkPrintContext   *context,
                                 gpointer           user_data);

static void      end_print      (GtkPrintOperation *operation,
                                 GtkPrintContext   *context,
                                 gpointer           user_data);

static void      draw_page      (GtkPrintOperation *print,
                                 GtkPrintContext   *context,
                                 int                page_nr,
                                 gpointer           user_data);

static void      write_print_settings_to_file  (GtkPrintSettings  *settings);
static void      add_setting_to_key_file       (const gchar       *key,
                                                const gchar       *value,
                                                gpointer           data);
static gboolean  load_print_settings_from_file (GtkPrintSettings *settings);

typedef struct
{
  gint     num_pages;
  gint32   drawable_id;
} PrintData;

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
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to print" }
  };

  gimp_install_procedure (PROC_NAME,
                          N_("Print the image using the Gtk Print system"),
                          "FIXME: write help",
                          "Bill Skaggs  <weskaggs@primate.ucdavis.edu>",
                          "Bill Skaggs  <weskaggs@primate.ucdavis.edu>",
                          "2006",
                          N_("_Print..."),
			  "GRAY, RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (print_args), 0,
                          print_args, NULL);

  gimp_plugin_menu_register (PROC_NAME, "<Image>/File/Send");
  gimp_plugin_icon_register (PROC_NAME, GIMP_ICON_TYPE_STOCK_ID,
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

  if (strcmp (name, PROC_NAME) == 0)
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
  GtkPrintOperation       *operation = gtk_print_operation_new ();
  GtkPrintSettings        *settings  = gtk_print_settings_new ();
  GError                  *error = NULL;
  PrintData               *data;

  if (load_print_settings_from_file (settings))
    gtk_print_operation_set_print_settings (operation, settings);

  /* begin junk */
  data = g_new0 (PrintData, 1);
  data->num_pages = 1;
  data->drawable_id = drawable_ID;
  /* end junk */

  g_signal_connect (operation, "begin-print", G_CALLBACK (begin_print), data);
  g_signal_connect (operation, "draw-page",   G_CALLBACK (draw_page),   data);
  g_signal_connect (operation, "end-print",   G_CALLBACK (end_print),   data);

  if (interactive)
    {
      GtkPrintOperationResult  res;

      res = gtk_print_operation_run (operation,
                                     GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
                                     NULL, &error);

      if (res == GTK_PRINT_OPERATION_RESULT_APPLY)
        {
          settings = gtk_print_operation_get_print_settings (operation);

          if (settings)
            write_print_settings_to_file (settings);
        }
    }
  else
    gtk_print_operation_run (operation,
                             GTK_PRINT_OPERATION_ACTION_PRINT,
                             NULL, &error);

  g_object_unref (operation);

  if (error)
    g_message (error->message);

  return TRUE;
}

static void
begin_print (GtkPrintOperation *operation,
	     GtkPrintContext   *context,
	     gpointer           user_data)
{
  PrintData        *data = (PrintData *)user_data;

  data->num_pages = 1;
  gtk_print_operation_set_n_pages (operation, data->num_pages);
}

static void
end_print (GtkPrintOperation *operation,
	   GtkPrintContext   *context,
	   gpointer           user_data)
{
  GtkPrintSettings *settings;

  settings = gtk_print_operation_get_print_settings (operation);

}


static void
draw_page (GtkPrintOperation *operation,
	   GtkPrintContext   *context,
	   int                page_nr,
	   gpointer           user_data)
{
  PrintData *data = (PrintData *)user_data;
  cairo_t         *cr;
  gdouble          cr_width;
  gdouble          cr_height;
  GimpDrawable    *drawable;
  GimpPixelRgn     region;
  gint             width;
  gint             height;
  gint             rowstride;
  guchar          *pixels;
  cairo_format_t   format;
  cairo_surface_t *surface;

  cr = gtk_print_context_get_cairo_context (context);
  cr_width  = gtk_print_context_get_width (context);
  cr_height = gtk_print_context_get_height (context);

  drawable = gimp_drawable_get (data->drawable_id);

  width     = drawable->width;
  height    = drawable->height;
  rowstride = width * drawable->bpp;
  pixels    = g_new (guchar, height * rowstride);

  gimp_pixel_rgn_init (&region, drawable, 0, 0, width, height, FALSE, FALSE);

  gimp_pixel_rgn_get_rect (&region, pixels, 0, 0, width, height);

  gimp_drawable_detach (drawable);

  switch (gimp_drawable_type (data->drawable_id))
    {
    case GIMP_RGB_IMAGE:
      format = CAIRO_FORMAT_RGB24;
      break;

    case GIMP_RGBA_IMAGE:
      format = CAIRO_FORMAT_ARGB32;
      break;

    case GIMP_GRAY_IMAGE:
      format = CAIRO_FORMAT_A8;
      break;

    default:
      g_warning ("drawable type not implemented");
      g_free (pixels);
      return;
      break;
    }

  surface = cairo_image_surface_create_for_data (pixels, format,
                                                 width, height, rowstride);
  cairo_set_source_surface (cr, surface, 0, 0);

  cairo_new_path (cr);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_clip (cr);

  cairo_mask_surface (cr, surface, 0, 0);

  g_free (pixels);
}

static void
write_print_settings_to_file (GtkPrintSettings *settings)
{
  gchar     *fname;
  FILE      *settings_file;
  GKeyFile  *key_file       = g_key_file_new ();
  gchar     *contents;
  gsize      length;
  GError    *error          = NULL;

  g_key_file_set_list_separator (key_file, '=');

  gtk_print_settings_foreach (settings, add_setting_to_key_file, key_file);

  contents = g_key_file_to_data (key_file, &length, &error);
  if (error)
    {
      g_message ("Unable to get contents of settings key file.\n");
      return;
    }

  fname = g_strconcat (gimp_directory (), G_DIR_SEPARATOR_S, "print-settings", NULL);
  settings_file = fopen (fname, "w");
  if (! settings_file)
    {
      g_message ("Unable to create save file for print settings.\n");
      return;
    }

  fwrite (contents, sizeof (gchar), length, settings_file);

  fclose (settings_file);
  g_key_file_free (key_file);
  g_free (contents);
  g_free (fname);
}

static void
add_setting_to_key_file (const gchar *key,
                         const gchar *value,
                         gpointer     data)
{
  GKeyFile *key_file = data;

  g_key_file_set_value (key_file, "settings", key, value);
}

static gboolean
load_print_settings_from_file (GtkPrintSettings *settings)
{
  GKeyFile  *key_file = g_key_file_new ();
  gchar     *fname;
  gchar    **keys;
  gsize      n_keys;
  GError    *error    = NULL;
  gint       i;

  g_key_file_set_list_separator (key_file, '=');

  fname = g_strconcat (gimp_directory (),
                       G_DIR_SEPARATOR_S,
                       "print-settings",
                       NULL);

  if (! g_key_file_load_from_file (key_file, fname, G_KEY_FILE_NONE, &error))
    return FALSE;

  keys = g_key_file_get_keys (key_file, "settings", &n_keys, &error);
  if (! keys)
    {
      g_message ("Error reading print settings keys: %s\n", error->message);
      return FALSE;
    }

  for (i = 0; i < n_keys; i++)
    {
      gchar *value = g_key_file_get_value (key_file, "settings",
                                           keys[i], &error);

      gtk_print_settings_set (settings, keys[i], value);

      g_free (value);
    }

  g_key_file_free (key_file);
  g_free (fname);
  g_strfreev (keys);

  return TRUE;
}
