/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Colorify. Changes the pixel's luminosity to a specified color
 * Copyright (C) 1997 Francisco Bustamante
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_NAME    "plug_in_colorify"
#define PLUG_IN_VERSION "1.1"
#define HELP_ID         "plug-in-colorify"

#define COLOR_SIZE 30

static void      query (void);
static void      run   (const gchar      *name,
                        gint              nparams,
                        const GimpParam  *param,
                        gint             *nreturn_vals,
                        GimpParam       **return_vals);

static void      colorify                  (GimpDrawable *drawable,
                                            GimpPreview  *preview);
static gboolean  colorify_dialog           (GimpDrawable *drawable);
static void      predefined_color_callback (GtkWidget    *widget,
                                            gpointer      data);

typedef struct
{
  GimpRGB  color;
  gboolean preview;
} ColorifyVals;

static ColorifyVals cvals =
{
  { 1.0, 1.0, 1.0, 1.0 },
  TRUE
};

static GimpRGB button_color[] =
{
  { 1.0, 0.0, 0.0, 1.0 },
  { 1.0, 1.0, 0.0, 1.0 },
  { 0.0, 1.0, 0.0, 1.0 },
  { 0.0, 1.0, 1.0, 1.0 },
  { 0.0, 0.0, 1.0, 1.0 },
  { 1.0, 0.0, 1.0, 1.0 },
  { 1.0, 1.0, 1.0, 1.0 },
};

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  query,
  run,
};

static GtkWidget *custom_color_button = NULL;

static gint lum_red_lookup[256];
static gint lum_green_lookup[256];
static gint lum_blue_lookup[256];
static gint final_red_lookup[256];
static gint final_green_lookup[256];
static gint final_blue_lookup[256];


MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_COLOR,    "color",    "Color to apply"}
  };

  gimp_install_procedure (PLUG_IN_NAME,
                          "Similar to the \"Color\" mode for layers.",
                          "Makes an average of the RGB channels and uses it "
                          "to set the color",
                          "Francisco Bustamante",
                          "Francisco Bustamante",
                          PLUG_IN_VERSION,
                          N_("_Colorify..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_NAME, "<Image>/Filters/Colors");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpPDBStatusType  status;
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;

  INIT_I18N ();

  status = GIMP_PDB_SUCCESS;
  run_mode = param[0].data.d_int32;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals = values;

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_NAME, &cvals);
      if (!colorify_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 4)
        status = GIMP_PDB_CALLING_ERROR;

      if (status == GIMP_PDB_SUCCESS)
        cvals.color = param[3].data.d_color;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_NAME, &cvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      gimp_progress_init (_("Colorifying..."));

      colorify (drawable, NULL);

      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_NAME, &cvals, sizeof (ColorifyVals));

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }

  gimp_drawable_detach (drawable);

  values[0].data.d_status = status;
}

static void
colorify_func (const guchar *src,
               guchar       *dest,
               gint          bpp,
               gpointer      data)
{
  gint lum;

  lum = lum_red_lookup[src[0]]   +
        lum_green_lookup[src[1]] +
        lum_blue_lookup[src[2]];

  dest[0] = final_red_lookup[lum];
  dest[1] = final_green_lookup[lum];
  dest[2] = final_blue_lookup[lum];

  if (bpp == 4)
    dest[3] = src[3];
}

static void
colorify (GimpDrawable *drawable,
          GimpPreview  *preview)
{
  gint  i;

  for (i = 0; i < 256; i ++)
    {
      lum_red_lookup[i]     = i * GIMP_RGB_INTENSITY_RED;
      lum_green_lookup[i]   = i * GIMP_RGB_INTENSITY_GREEN;
      lum_blue_lookup[i]    = i * GIMP_RGB_INTENSITY_BLUE;
      final_red_lookup[i]   = i * cvals.color.r;
      final_green_lookup[i] = i * cvals.color.g;
      final_blue_lookup[i]  = i * cvals.color.b;
    }

  if (preview)
    {
      gint    width, height, bytes;
      guchar *src;

      gimp_preview_get_size (preview, &width, &height);
      src = gimp_drawable_get_thumbnail_data (drawable->drawable_id,
                                              &width, &height, &bytes);
      for (i = 0; i < width * height; i++)
        colorify_func (src + i * bytes, src + i * bytes, bytes, NULL);

      gimp_preview_draw_buffer (preview, src, width * bytes);
      g_free (src);
    }
  else
    {
      gimp_rgn_iterate2 (drawable, 0 /* unused */, colorify_func, NULL);
    }
}

static gboolean
colorify_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *table;
  GtkWidget *color_area;
  gint       i;
  gboolean   run;

  gimp_ui_init ("colorify", TRUE);

  dialog = gimp_dialog_new (_("Colorify"), "colorify",
                            NULL, 0,
                            gimp_standard_help_func, HELP_ID,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_aspect_preview_new (drawable, &cvals.preview);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (colorify),
                            drawable);

  table = gtk_table_new (2, 7, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  label = gtk_label_new (_("Custom Color:"));
  gtk_table_attach (GTK_TABLE (table), label, 4, 6, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  custom_color_button = gimp_color_button_new (_("Colorify Custom Color"),
                                               COLOR_SIZE, COLOR_SIZE,
                                               &cvals.color,
                                               GIMP_COLOR_AREA_FLAT);
  g_signal_connect (custom_color_button, "color_changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &cvals.color);
  g_signal_connect_swapped (custom_color_button, "color_changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_table_attach (GTK_TABLE (table), custom_color_button, 6, 7, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (custom_color_button);

  for (i = 0; i < 7; i++)
    {
      button = gtk_button_new ();
      color_area = gimp_color_area_new (&button_color[i],
                                        GIMP_COLOR_AREA_FLAT,
                                        GDK_BUTTON2_MASK);
      gtk_widget_set_size_request (GTK_WIDGET (color_area),
                                   COLOR_SIZE, COLOR_SIZE);
      gtk_container_add (GTK_CONTAINER (button), color_area);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (predefined_color_callback),
                        &button_color[i]);
      gtk_widget_show (color_area);

      gtk_table_attach (GTK_TABLE (table), button, i, i + 1, 1, 2,
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (button);
    }

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
predefined_color_callback (GtkWidget *widget,
                           gpointer   data)
{
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (custom_color_button),
                               (GimpRGB *) data);
}
