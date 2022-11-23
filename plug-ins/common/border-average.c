/* borderaverage 0.01 - image processing plug-in for LIGMA.
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-borderaverage"
#define PLUG_IN_BINARY "border-average"
#define PLUG_IN_ROLE   "ligma-border-average"


typedef struct _BorderAverage      BorderAverage;
typedef struct _BorderAverageClass BorderAverageClass;

struct _BorderAverage
{
  LigmaPlugIn      parent_instance;
};

struct _BorderAverageClass
{
  LigmaPlugInClass parent_class;
};


#define BORDER_AVERAGE_TYPE  (border_average_get_type ())
#define BORDER_AVERAGE (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BORDER_AVERAGE_TYPE, BorderAverage))


GType                   border_average_get_type               (void) G_GNUC_CONST;

static GList          * border_average_query_procedures       (LigmaPlugIn           *plug_in);
static LigmaProcedure  * border_average_create_procedure       (LigmaPlugIn           *plug_in,
                                                               const gchar          *name);

static LigmaValueArray * border_average_run                    (LigmaProcedure        *procedure,
                                                               LigmaRunMode           run_mode,
                                                               LigmaImage            *image,
                                                               gint                  n_drawables,
                                                               LigmaDrawable        **drawables,
                                                               const LigmaValueArray *args,
                                                               gpointer              run_data);


static void      borderaverage        (GeglBuffer   *buffer,
                                       LigmaDrawable *drawable,
                                       LigmaRGB      *result);

static gboolean  borderaverage_dialog (LigmaImage    *image,
                                       LigmaDrawable *drawable);

static void      add_new_color        (const guchar *buffer,
                                       gint         *cube,
                                       gint          bucket_expo);

static void      thickness_callback   (GtkWidget    *widget,
                                       gpointer      data);


G_DEFINE_TYPE (BorderAverage, border_average, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (BORDER_AVERAGE_TYPE)
DEFINE_STD_SET_I18N


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

static void
border_average_class_init (BorderAverageClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = border_average_query_procedures;
  plug_in_class->create_procedure = border_average_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
border_average_init (BorderAverage *film)
{
}

static GList *
border_average_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
border_average_create_procedure (LigmaPlugIn  *plug_in,
                               const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            border_average_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "RGB*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE);

      ligma_procedure_set_menu_label (procedure, _("_Border Average..."));
      ligma_procedure_add_menu_path (procedure, "<Image>/Colors/Info");

      ligma_procedure_set_documentation (procedure,
                                        _("Set foreground to the average color of the image border"),
                                        "",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Philipp Klaus",
                                      "Internet Access AG",
                                      "1998");

      LIGMA_PROC_ARG_INT (procedure, "thickness",
                         "Border size to take in count",
                         "Border size to take in count",
                         0, G_MAXINT, 3,
                         G_PARAM_READWRITE);
      LIGMA_PROC_ARG_INT (procedure, "bucket-exponent",
                         "Bits for bucket size (default=4: 16 Levels)",
                         "Bits for bucket size (default=4: 16 Levels)",
                         0, G_MAXINT, 4,
                         G_PARAM_READWRITE);

      LIGMA_PROC_VAL_RGB (procedure, "borderaverage",
                         "The average color of the specified border.",
                         "The average color of the specified border.",
                         TRUE, NULL,
                         G_PARAM_READWRITE);
    }

  return procedure;
}


static LigmaValueArray *
border_average_run (LigmaProcedure        *procedure,
                    LigmaRunMode           run_mode,
                    LigmaImage            *image,
                    gint                  n_drawables,
                    LigmaDrawable        **drawables,
                    const LigmaValueArray *args,
                    gpointer              run_data)
{
  LigmaDrawable      *drawable;
  LigmaValueArray    *return_vals = NULL;
  LigmaPDBStatusType  status = LIGMA_PDB_SUCCESS;
  LigmaRGB            result_color = { 0.0, };
  GeglBuffer        *buffer;

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, LIGMA_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  buffer = ligma_drawable_get_buffer (drawable);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
      ligma_get_data (PLUG_IN_PROC, &borderaverage_data);
      borderaverage_thickness       = borderaverage_data.thickness;
      borderaverage_bucket_exponent = borderaverage_data.bucket_exponent;
      if (! borderaverage_dialog (image, drawable))
        status = LIGMA_PDB_EXECUTION_ERROR;
      break;

    case LIGMA_RUN_NONINTERACTIVE:
      if (ligma_value_array_length (args) != 2)
        status = LIGMA_PDB_CALLING_ERROR;
      if (status == LIGMA_PDB_SUCCESS)
        {
          borderaverage_thickness       = LIGMA_VALUES_GET_INT (args, 0);
          borderaverage_bucket_exponent = LIGMA_VALUES_GET_INT (args, 1);
        }
      break;

    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_get_data (PLUG_IN_PROC, &borderaverage_data);
      borderaverage_thickness       = borderaverage_data.thickness;
      borderaverage_bucket_exponent = borderaverage_data.bucket_exponent;
      break;

    default:
      break;
    }

  if (status == LIGMA_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is RGB color  */
      if (ligma_drawable_is_rgb (drawable))
        {
          ligma_progress_init ( _("Border Average"));
          borderaverage (buffer, drawable, &result_color);

          if (run_mode != LIGMA_RUN_NONINTERACTIVE)
            {
              ligma_context_set_foreground (&result_color);
            }
          if (run_mode == LIGMA_RUN_INTERACTIVE)
            {
              borderaverage_data.thickness       = borderaverage_thickness;
              borderaverage_data.bucket_exponent = borderaverage_bucket_exponent;
              ligma_set_data (PLUG_IN_PROC,
                             &borderaverage_data, sizeof (borderaverage_data));
            }
        }
      else
        {
          status = LIGMA_PDB_EXECUTION_ERROR;
        }
    }

  g_object_unref (buffer);

  return_vals = ligma_procedure_new_return_values (procedure, status, NULL);

  if (status == LIGMA_PDB_SUCCESS)
    LIGMA_VALUES_SET_RGB (return_vals, 1, &result_color);

  return return_vals;
}


static void
borderaverage (GeglBuffer   *buffer,
               LigmaDrawable *drawable,
               LigmaRGB      *result)
{
  gint            x, y, width, height;
  gint            max;
  guchar          r, g, b;
  gint            bucket_num, bucket_expo, bucket_rexpo;
  gint           *cube;
  gint            i, j, k;
  GeglRectangle   border[4];

  if (! ligma_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    {
      ligma_rgba_set_uchar (result, 0, 0, 0, 255);
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
  ligma_rgba_set_uchar (result, r, g, b, 255);

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
borderaverage_dialog (LigmaImage    *image,
                      LigmaDrawable *drawable)
{
  GtkWidget    *dialog;
  GtkWidget    *frame;
  GtkWidget    *main_vbox;
  GtkWidget    *hbox;
  GtkWidget    *label;
  GtkWidget    *size_entry;
  LigmaUnit      unit;
  GtkWidget    *combo;
  GtkSizeGroup *group;
  gboolean      run;
  gdouble       xres, yres;
  GeglBuffer   *buffer = NULL;

  const gchar *labels[] =
    { "1", "2", "4", "8", "16", "32", "64", "128", "256" };

  ligma_ui_init (PLUG_IN_BINARY);

  dialog = ligma_dialog_new (_("Border Average"), PLUG_IN_ROLE,
                            NULL, 0,
                            ligma_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  ligma_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  frame = ligma_frame_new (_("Border Size"));
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
  ligma_image_get_resolution (image, &xres, &yres);
  unit = ligma_image_get_unit (image);

  size_entry = ligma_size_entry_new (1, unit, "%a", TRUE, TRUE, FALSE, 4,
                                    LIGMA_SIZE_ENTRY_UPDATE_SIZE);
  gtk_box_pack_start (GTK_BOX (hbox), size_entry, FALSE, FALSE, 0);

  ligma_size_entry_set_unit (LIGMA_SIZE_ENTRY (size_entry), LIGMA_UNIT_PIXEL);
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (size_entry), 0, xres, TRUE);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  buffer = ligma_drawable_get_buffer (drawable);
  if (buffer)
    ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (size_entry), 0, 0.0,
                              MIN (gegl_buffer_get_width (buffer),
                                   gegl_buffer_get_height (buffer)));

  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (size_entry), 0,
                                         1.0, 256.0);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (size_entry), 0,
                              (gdouble) borderaverage_thickness);
  g_signal_connect (size_entry, "value-changed",
                    G_CALLBACK (thickness_callback),
                    NULL);
  gtk_widget_show (size_entry);

  frame = ligma_frame_new (_("Number of Colors"));
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

  combo = ligma_int_combo_box_new_array (G_N_ELEMENTS (labels), labels);
  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (combo),
                                 borderaverage_bucket_exponent);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (ligma_int_combo_box_get_active),
                    &borderaverage_bucket_exponent);

  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  gtk_widget_show (dialog);

  run = (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
thickness_callback (GtkWidget *widget,
                    gpointer   data)
{
  borderaverage_thickness =
    ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (widget), 0);
}
