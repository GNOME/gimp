/* borderaverage 0.01 - image processing plug-in for GIMP.
 *
 * Copyright (C) 1998 Philipp Klaus (webmaster@access.ch)
 *
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


#define PLUG_IN_PROC   "plug-in-borderaverage"
#define PLUG_IN_BINARY "border-average"
#define PLUG_IN_ROLE   "gimp-border-average"


/* Declare local functions.
 */
static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);


static void      borderaverage        (GeglBuffer   *buffer,
                                       gint32        drawable_id,
                                       GimpRGB      *result);

static gboolean  borderaverage_dialog (gint32        image_ID,
                                       gint32        drawable_id);

static void      add_new_color        (const guchar *buffer,
                                       gint         *cube,
                                       gint          bucket_expo);

static void      thickness_callback   (GtkWidget    *widget,
                                       gpointer      data);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init  */
  NULL,  /* quit  */
  query, /* query */
  run,   /* run   */
};

static gint  borderaverage_thickness       = 3;
static gint  borderaverage_bucket_exponent = 4;

struct borderaverage_data
{
  gint  thickness;
  gint  bucket_exponent;
}

static borderaverage_data =
{
  3,
  4
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",        "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",           "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",        "Input drawable" },
    { GIMP_PDB_INT32,    "thickness",       "Border size to take in count" },
    { GIMP_PDB_INT32,    "bucket-exponent", "Bits for bucket size (default=4: 16 Levels)" },
  };
  static const GimpParamDef return_vals[] =
  {
    { GIMP_PDB_COLOR,    "borderaverage",   "The average color of the specified border." },
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Set foreground to the average color of the image border"),
                          "",
                          "Philipp Klaus",
                          "Internet Access AG",
                          "1998",
                          N_("_Border Average..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args),
                          G_N_ELEMENTS (return_vals),
                          args, return_vals);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Colors/Info");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[3];
  gint32             image_ID;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRGB            result_color = { 0.0, };
  GimpRunMode        run_mode;
  gint32             drawable_id;
  GeglBuffer        *buffer;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;
  image_ID = param[1].data.d_int32;
  drawable_id = param[2].data.d_drawable;

  buffer = gimp_drawable_get_buffer (drawable_id);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &borderaverage_data);
      borderaverage_thickness       = borderaverage_data.thickness;
      borderaverage_bucket_exponent = borderaverage_data.bucket_exponent;
      if (! borderaverage_dialog (image_ID, drawable_id))
        status = GIMP_PDB_EXECUTION_ERROR;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 5)
        status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS)
        {
          borderaverage_thickness       = param[3].data.d_int32;
          borderaverage_bucket_exponent = param[4].data.d_int32;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &borderaverage_data);
      borderaverage_thickness       = borderaverage_data.thickness;
      borderaverage_bucket_exponent = borderaverage_data.bucket_exponent;
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is RGB color  */
      if (gimp_drawable_is_rgb (drawable_id))
        {
          gimp_progress_init ( _("Border Average"));
          borderaverage (buffer, drawable_id, &result_color);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            {
              gimp_context_set_foreground (&result_color);
            }
          if (run_mode == GIMP_RUN_INTERACTIVE)
            {
              borderaverage_data.thickness       = borderaverage_thickness;
              borderaverage_data.bucket_exponent = borderaverage_bucket_exponent;
              gimp_set_data (PLUG_IN_PROC,
                             &borderaverage_data, sizeof (borderaverage_data));
            }
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  *nreturn_vals = 3;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  values[1].type         = GIMP_PDB_COLOR;
  values[1].data.d_color = result_color;

  g_object_unref (buffer);
}


static void
borderaverage (GeglBuffer   *buffer,
               gint32        drawable_id,
               GimpRGB      *result)
{
  gint            x, y, width, height;
  gint            max;
  guchar          r, g, b;
  gint            bucket_num, bucket_expo, bucket_rexpo;
  gint           *cube;
  gint            i, j, k;
  GeglRectangle   border[4];

  if (! gimp_drawable_mask_intersect (drawable_id, &x, &y, &width, &height))
    {
      gimp_rgba_set_uchar (result, 0, 0, 0, 255);
      return;
    }

  /* allocate and clear the cube before */
  bucket_expo = borderaverage_bucket_exponent;
  bucket_rexpo = 8 - bucket_expo;
  cube = g_new (gint, 1 << (bucket_rexpo * 3));
  bucket_num = 1 << bucket_rexpo;

  for (i = 0; i < bucket_num; i++)
    {
      for (j = 0; j < bucket_num; j++)
        {
          for (k = 0; k < bucket_num; k++)
            {
              cube[(i << (bucket_rexpo << 1)) + (j << bucket_rexpo) + k] = 0;
            }
        }
    }

  /* Top */
  border[0].x =       x;
  border[0].y =       y;
  border[0].width =   width;
  border[0].height =  borderaverage_thickness;

  /* Bottom */
  border[1].x =       x;
  border[1].y =       y + height - borderaverage_thickness;
  border[1].width =   width;
  border[1].height =  borderaverage_thickness;

  /* Left */
  border[2].x =       x;
  border[2].y =       y + borderaverage_thickness;
  border[2].width =   borderaverage_thickness;
  border[2].height =  height - 2 * borderaverage_thickness;

  /* Right */
  border[3].x =       x + width - borderaverage_thickness;
  border[3].y =       y + borderaverage_thickness;
  border[3].width =   borderaverage_thickness;
  border[3].height =  height - 2 * borderaverage_thickness;

  /* Fill the cube */
  for (i = 0; i < 4; i++)
    {
      if (border[i].width > 0 && border[i].height > 0)
        {
          GeglBufferIterator *gi;

          gi = gegl_buffer_iterator_new (buffer, &border[i], 0, babl_format ("R'G'B' u8"),
                                         GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 1);

          while (gegl_buffer_iterator_next (gi))
            {
              guint   k;
              guchar *data;

              data = (guchar*) gi->items[0].data;

              for (k = 0; k < gi->length; k++)
                {
                  add_new_color (data + k * 3,
                                 cube,
                                 bucket_expo);
                }
            }
        }
    }

  max = 0; r = 0; g = 0; b = 0;

  /* get max of cube */
  for (i = 0; i < bucket_num; i++)
    {
      for (j = 0; j < bucket_num; j++)
        {
          for (k = 0; k < bucket_num; k++)
            {
              if (cube[(i << (bucket_rexpo << 1)) +
                      (j << bucket_rexpo) + k] > max)
                {
                  max = cube[(i << (bucket_rexpo << 1)) +
                             (j << bucket_rexpo) + k];
                  r = (i<<bucket_expo) + (1<<(bucket_expo - 1));
                  g = (j<<bucket_expo) + (1<<(bucket_expo - 1));
                  b = (k<<bucket_expo) + (1<<(bucket_expo - 1));
                }
            }
        }
    }

  /* return the color */
  gimp_rgba_set_uchar (result, r, g, b, 255);

  g_free (cube);
}

static void
add_new_color (const guchar *buffer,
               gint         *cube,
               gint          bucket_expo)
{
  guchar r, g, b;
  gint   bucket_rexpo;

  bucket_rexpo = 8 - bucket_expo;
  r = buffer[0] >> bucket_expo;
  g = buffer[1] >> bucket_expo;
  b = buffer[2] >> bucket_expo;
  cube[(r << (bucket_rexpo << 1)) + (g << bucket_rexpo) + b]++;
}

static gboolean
borderaverage_dialog (gint32        image_ID,
                      gint32        drawable_id)
{
  GtkWidget    *dialog;
  GtkWidget    *frame;
  GtkWidget    *main_vbox;
  GtkWidget    *hbox;
  GtkWidget    *label;
  GtkWidget    *size_entry;
  GimpUnit      unit;
  GtkWidget    *combo;
  GtkSizeGroup *group;
  gboolean      run;
  gdouble       xres, yres;
  GeglBuffer   *buffer = NULL;

  const gchar *labels[] =
    { "1", "2", "4", "8", "16", "32", "64", "128", "256" };

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Border Average"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  frame = gimp_frame_new (_("Border Size"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Thickness:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  gtk_size_group_add_widget (group, label);
  g_object_unref (group);

  /*  Get the image resolution and unit  */
  gimp_image_get_resolution (image_ID, &xres, &yres);
  unit = gimp_image_get_unit (image_ID);

  size_entry = gimp_size_entry_new (1, unit, "%a", TRUE, TRUE, FALSE, 4,
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_box_pack_start (GTK_BOX (hbox), size_entry, FALSE, FALSE, 0);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (size_entry), GIMP_UNIT_PIXEL);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (size_entry), 0, xres, TRUE);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  buffer = gimp_drawable_get_buffer (drawable_id);
  if (buffer)
    gimp_size_entry_set_size (GIMP_SIZE_ENTRY (size_entry), 0, 0.0,
                              MIN (gegl_buffer_get_width (buffer),
                                   gegl_buffer_get_height (buffer)));

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (size_entry), 0,
                                         1.0, 256.0);
  gtk_table_set_col_spacing (GTK_TABLE (size_entry), 0, 4);
  gtk_table_set_col_spacing (GTK_TABLE (size_entry), 2, 12);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (size_entry), 0,
                              (gdouble) borderaverage_thickness);
  g_signal_connect (size_entry, "value-changed",
                    G_CALLBACK (thickness_callback),
                    NULL);
  gtk_widget_show (size_entry);

  frame = gimp_frame_new (_("Number of Colors"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Bucket size:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_size_group_add_widget (group, label);

  combo = gimp_int_combo_box_new_array (G_N_ELEMENTS (labels), labels);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                 borderaverage_bucket_exponent);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_int_combo_box_get_active),
                    &borderaverage_bucket_exponent);

  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
thickness_callback (GtkWidget *widget,
                    gpointer   data)
{
  borderaverage_thickness =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
}
