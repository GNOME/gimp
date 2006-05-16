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

#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-job.h>

#include <libgnomeprintui/gnome-print-dialog.h>
#include <libgnomeprintui/gnome-print-job-preview.h>

#include "libgimp/stdplugins-intl.h"


#define PROC_NAME        "file-print-gnome"
#define PLUG_IN_BINARY   "gnomeprint"

static void      query          (void);
static void      run            (const gchar       *name,
                                 gint               nparams,
                                 const GimpParam   *param,
                                 gint              *nreturn_vals,
                                 GimpParam        **return_vals);

static gboolean  print_image    (gint32             image_ID,
                                 gint32             drawable_ID,
                                 gboolean           interactive);

static GnomePrintJob * print_job_new (void);

static gboolean  print_job_do   (GnomePrintJob     *job,
                                 gint32             drawable_ID);
static void      print_drawable (GnomePrintContext *context,
                                 GnomePrintConfig  *config,
                                 gint32             drawable_ID);

static void      print_fit_size (GnomePrintConfig  *config,
                                 gint               width,
                                 gint               height,
                                 gdouble           *scale_x,
                                 gdouble           *scale_y,
                                 gdouble           *trans_x,
                                 gdouble           *trans_y);


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
                          N_("Print the image using the GNOME Print system"),
                          "FIXME: write help",
                          "Sven Neumann  <sven@gimp.org>",
                          "Sven Neumann  <sven@gimp.org>",
                          "2005",
                          N_("_GNOME Print..."),
			  "GRAY, RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (print_args), 0,
                          print_args, NULL);

  gimp_plugin_menu_register (PROC_NAME, "<Image>/File/Send");
  gimp_plugin_icon_register (PROC_NAME,
                             GIMP_ICON_TYPE_STOCK_ID, GTK_STOCK_PRINT);
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
  GnomePrintJob *job  = print_job_new ();
  gboolean       done = FALSE;

  if (interactive)
    {
      GtkWidget *dialog;
      gint       response;

      dialog = gnome_print_dialog_new (job, (const guchar *) _("Print"),
                                       GNOME_PRINT_DIALOG_COPIES);

      gimp_window_set_transient (GTK_WINDOW (dialog));

      do
        {
          switch (response = gtk_dialog_run (GTK_DIALOG (dialog)))
            {
            case GNOME_PRINT_DIALOG_RESPONSE_PRINT:
              break;

            case GNOME_PRINT_DIALOG_RESPONSE_PREVIEW:
              {
                GtkWidget *preview;

                if (! done)
                  done = print_job_do (job, drawable_ID);

                preview = gnome_print_job_preview_new (job,
                                                       (const guchar *)
                                                       _("Print Preview"));
                gtk_window_set_transient_for (GTK_WINDOW (preview),
                                              GTK_WINDOW (dialog));
                gtk_window_set_modal (GTK_WINDOW (preview), TRUE);
                gtk_widget_show (preview);

                g_signal_connect (preview, "destroy",
                                  G_CALLBACK (gtk_main_quit),
                                  NULL);

                gtk_main();
              }
              break;

            case GNOME_PRINT_DIALOG_RESPONSE_CANCEL:
              break;
            }
        }
      while (response == GNOME_PRINT_DIALOG_RESPONSE_PREVIEW);

      gtk_widget_destroy (dialog);

      if (response == GNOME_PRINT_DIALOG_RESPONSE_CANCEL)
        {
          g_object_unref (job);
          return FALSE;
        }
    }

  if (! done)
    done = print_job_do (job, drawable_ID);

  gnome_print_job_print (job);

  g_object_unref (job);

  return TRUE;
}

static GnomePrintJob *
print_job_new (void)
{
  GnomePrintConfig  *config;
  GnomePrintJob     *job;

  config = gnome_print_config_default ();
  job = gnome_print_job_new (config);
  g_object_unref (config);

  return job;
}

static gboolean
print_job_do (GnomePrintJob *job,
              gint32         drawable_ID)
{
  GnomePrintContext *context;
  GnomePrintConfig  *config;

  context = gnome_print_job_get_context (job);
  config = gnome_print_job_get_config (job);

  print_drawable (context, config, drawable_ID);

  g_object_unref (config);
  g_object_unref (context);

  gnome_print_job_close (job);

  return TRUE;
}

static void
print_drawable (GnomePrintContext *context,
                GnomePrintConfig  *config,
                gint32             drawable_ID)
{
  GimpDrawable *drawable = gimp_drawable_get (drawable_ID);
  GimpPixelRgn  region;
  gint          width;
  gint          height;
  gint          rowstride;
  guchar       *pixels;
  gchar        *name;
  gdouble       scale_x;
  gdouble       scale_y;
  gdouble       trans_x;
  gdouble       trans_y;

  width     = drawable->width;
  height    = drawable->height;
  rowstride = width * drawable->bpp;
  pixels    = g_new (guchar, height * rowstride);

  gimp_pixel_rgn_init (&region, drawable, 0, 0, width, height, FALSE, FALSE);

  gimp_pixel_rgn_get_rect (&region, pixels, 0, 0, width, height);

  gimp_drawable_detach (drawable);

  name = gimp_drawable_get_name (drawable_ID);
  gnome_print_beginpage (context, (const guchar *) name);
  g_free (name);

  print_fit_size (config, width, height,
                  &scale_x, &scale_y, &trans_x, &trans_y);

  gnome_print_gsave (context);
  gnome_print_scale (context, scale_x, scale_y);
  gnome_print_translate (context, trans_x, trans_y);

  gnome_print_moveto (context, 0, 0);
  gnome_print_lineto (context, 1, 0);
  gnome_print_lineto (context, 1, 1);
  gnome_print_lineto (context, 0, 1);
  gnome_print_lineto (context, 0, 0);
  gnome_print_stroke (context);

  switch (gimp_drawable_type (drawable_ID))
    {
    case GIMP_RGB_IMAGE:
      gnome_print_rgbimage (context, pixels, width, height, rowstride);
      break;

    case GIMP_RGBA_IMAGE:
      gnome_print_rgbaimage (context, pixels, width, height, rowstride);
      break;

    case GIMP_GRAY_IMAGE:
      gnome_print_grayimage (context, pixels, width, height, rowstride);
      break;

    default:
      g_warning ("drawable type not implemented");
      break;
    }

  g_free (pixels);

  gnome_print_grestore (context);

  gnome_print_showpage (context);
}

static void
print_fit_size (GnomePrintConfig *config,
                gint              width,
                gint              height,
                gdouble          *scale_x,
                gdouble          *scale_y,
                gdouble          *trans_x,
                gdouble          *trans_y)
{
  const GnomePrintUnit *base;
  const GnomePrintUnit *unit;
  gdouble  paper_width;
  gdouble  paper_height;
  gdouble  margin_bottom;
  gdouble  margin_right;
  gdouble  margin_left;
  gdouble  margin_top;


  base = gnome_print_unit_get_identity (GNOME_PRINT_UNIT_DIMENSIONLESS);

  gnome_print_config_get_length (config,
                                 (const guchar *) GNOME_PRINT_KEY_PAPER_WIDTH,
                                 &paper_width, &unit);
  gnome_print_convert_distance (&paper_width, unit, base);

  gnome_print_config_get_length (config,
                                 (const guchar *) GNOME_PRINT_KEY_PAPER_HEIGHT,
                                 &paper_height, &unit);
  gnome_print_convert_distance (&paper_height, unit, base);

  gnome_print_config_get_length (config,
                                 (const guchar *) GNOME_PRINT_KEY_PAPER_MARGIN_BOTTOM,
                                 &margin_bottom, &unit);
  gnome_print_convert_distance (&margin_bottom, unit, base);

  gnome_print_config_get_length (config,
                                 (const guchar *) GNOME_PRINT_KEY_PAPER_MARGIN_TOP,
                                 &margin_top, &unit);
  gnome_print_convert_distance (&margin_top, unit, base);

  gnome_print_config_get_length (config,
                                 (const guchar *) GNOME_PRINT_KEY_PAPER_MARGIN_LEFT,
                                 &margin_left, &unit);
  gnome_print_convert_distance (&margin_left, unit, base);

  gnome_print_config_get_length (config,
                                 (const guchar *) GNOME_PRINT_KEY_PAPER_MARGIN_RIGHT,
                                 &margin_right, &unit);
  gnome_print_convert_distance (&margin_right, unit, base);

  paper_width -= margin_left + margin_right;
  paper_height -= margin_top + margin_left;

  *scale_x = paper_width;
  *scale_y = paper_height;
  *trans_x = margin_left;
  *trans_y = margin_bottom;
}
