/* flame - cosmic recursive fractal flames
 * Copyright (C) 1997  Scott Draves <spot@cs.cmu.edu>
 *
 * GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <time.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "flame.h"

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC      "plug-in-flame"
#define PLUG_IN_BINARY    "flame"
#define PLUG_IN_ROLE      "gimp-flame"

#define VARIATION_SAME    (-2)

#define BUFFER_SIZE       10000

#define SCALE_WIDTH       150
#define PREVIEW_SIZE      150
#define EDIT_PREVIEW_SIZE 85
#define NMUTANTS          9

#define BLACK_DRAWABLE    (-2)
#define GRADIENT_DRAWABLE (-3)
#define TABLE_DRAWABLE    (-4)


struct
{
  gint          randomize;  /* superseded */
  gint          variation;
  gint32        cmap_drawable;
  control_point cp;
} config;


/* Declare local functions. */

static void      query             (void);
static void      run               (const gchar      *name,
                                    gint              nparams,
                                    const GimpParam  *param,
                                    gint             *nreturn_vals,
                                    GimpParam       **return_vals);
static void      flame             (GimpDrawable     *drawable);

static gboolean  flame_dialog      (void);
static void      set_flame_preview (void);
static void      load_callback     (GtkWidget        *widget,
                                    gpointer          data);
static void      save_callback     (GtkWidget        *widget,
                                    gpointer          data);
static void      set_edit_preview  (void);
static void      combo_callback    (GtkWidget        *widget,
                                    gpointer          data);
static void      init_mutants      (void);


static gchar      buffer[BUFFER_SIZE];
static GtkWidget *cmap_preview;
static GtkWidget *flame_preview;
static gint       preview_width, preview_height;
static GtkWidget *dialog;
static GtkWidget *load_button = NULL;
static GtkWidget *save_button = NULL;
static GtkWidget *file_dialog = NULL;
static gint       load_save;

static GtkWidget *edit_dialog = NULL;

static control_point  edit_cp;
static control_point  mutants[NMUTANTS];
static GtkWidget     *edit_previews[NMUTANTS];
static gdouble        pick_speed = 0.2;

static frame_spec f = { 0.0, &config.cp, 1, 0.0 };


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
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (unused)"         },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"               }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Create cosmic recursive fractal flames"),
                          "Create cosmic recursive fractal flames",
                          "Scott Draves",
                          "Scott Draves",
                          "1997",
                          N_("_Flame..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Render/Fractals");
}

static void
maybe_init_cp (void)
{
  if (0 == config.cp.spatial_oversample)
    {
      config.randomize     = 0;
      config.variation     = VARIATION_SAME;
      config.cmap_drawable = GRADIENT_DRAWABLE;

      random_control_point (&config.cp, variation_random);

      config.cp.center[0]             = 0.0;
      config.cp.center[1]             = 0.0;
      config.cp.pixels_per_unit       = 100;
      config.cp.spatial_oversample    = 2;
      config.cp.gamma                 = 2.0;
      config.cp.contrast              = 1.0;
      config.cp.brightness            = 1.0;
      config.cp.spatial_filter_radius = 0.75;
      config.cp.sample_density        = 5.0;
      config.cp.zoom                  = 0.0;
      config.cp.nbatches              = 1;
      config.cp.white_level           = 200;
      config.cp.cmap_index            = 72;
      /* cheating */
      config.cp.width                 = 256;
      config.cp.height                = 256;
    }
}

static void
run (const gchar      *name,
     gint              n_params,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpDrawable     *drawable = NULL;
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  if (run_mode == GIMP_RUN_NONINTERACTIVE)
    {
      status = GIMP_PDB_CALLING_ERROR;
    }
  else
    {
      gimp_get_data (PLUG_IN_PROC, &config);
      maybe_init_cp ();

      drawable = gimp_drawable_get (param[2].data.d_drawable);
      config.cp.width  = drawable->width;
      config.cp.height = drawable->height;

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          if (! flame_dialog ())
            {
              gimp_drawable_detach (drawable);

              status = GIMP_PDB_CANCEL;
            }
        }
      else
        {
          /*  reusing a drawable_ID from the last run is a bad idea
              since the drawable might have vanished  (bug #37761)   */
          if (config.cmap_drawable >= 0)
            config.cmap_drawable = GRADIENT_DRAWABLE;
        }
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (gimp_drawable_is_rgb (drawable->drawable_id))
        {
          gimp_progress_init (_("Drawing flame"));
          gimp_tile_cache_ntiles (2 * (drawable->width /
                                       gimp_tile_width () + 1));

          flame (drawable);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          gimp_set_data (PLUG_IN_PROC, &config, sizeof (config));
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }

      gimp_drawable_detach (drawable);
    }

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static void
drawable_to_cmap (control_point *cp)
{
  gint          i, j;
  GimpPixelRgn  pr;
  GimpDrawable *d;
  guchar       *p;

  if (TABLE_DRAWABLE >= config.cmap_drawable)
    {
      i = TABLE_DRAWABLE - config.cmap_drawable;
      get_cmap (i, cp->cmap, 256);
    }
  else if (BLACK_DRAWABLE == config.cmap_drawable)
    {
      for (i = 0; i < 256; i++)
        for (j = 0; j < 3; j++)
          cp->cmap[i][j] = 0.0;
    }
  else if (GRADIENT_DRAWABLE == config.cmap_drawable)
    {
      gchar   *name = gimp_context_get_gradient ();
      gint     num;
      gdouble *g;

      /* FIXME: "reverse" hardcoded to FALSE. */
      gimp_gradient_get_uniform_samples (name, 256, FALSE,
                                         &num, &g);

      g_free (name);

      for (i = 0; i < 256; i++)
        for (j = 0; j < 3; j++)
          cp->cmap[i][j] = g[i*4 + j];
      g_free (g);
    }
  else
    {
      d = gimp_drawable_get (config.cmap_drawable);
      p = g_new (guchar, d->bpp);
      gimp_pixel_rgn_init (&pr, d, 0, 0,
                           d->width, d->height, FALSE, FALSE);
      for (i = 0; i < 256; i++)
        {
          gimp_pixel_rgn_get_pixel (&pr, p, i % d->width,
                                    (i / d->width) % d->height);
          for (j = 0; j < 3; j++)
            cp->cmap[i][j] =
              (d->bpp >= 3) ? (p[j] / 255.0) : (p[0]/255.0);
        }
      g_free (p);
    }
}

static void
flame (GimpDrawable *drawable)
{
  gint    width, height;
  guchar *tmp;
  gint    bytes;

  width  = drawable->width;
  height = drawable->height;
  bytes  = drawable->bpp;

  if (3 != bytes && 4 != bytes)
    {
      g_message (_("Flame works only on RGB drawables."));
      return;
    }

  tmp = g_new (guchar, width * height * 4);

  /* render */
  config.cp.width = width;
  config.cp.height = height;
  if (config.randomize)
    random_control_point (&config.cp, config.variation);
  drawable_to_cmap (&config.cp);
  render_rectangle (&f, tmp, width, field_both, 4,
                    gimp_progress_update);
  gimp_progress_update (1.0);

  /* update destination */
  if (4 == bytes)
    {
      GimpPixelRgn pr;
      gimp_pixel_rgn_init (&pr, drawable, 0, 0, width, height,
                           TRUE, TRUE);
      gimp_pixel_rgn_set_rect (&pr, tmp, 0, 0, width, height);
    }
  else if (3 == bytes)
    {
      gint       i, j;
      GimpPixelRgn  src_pr, dst_pr;
      guchar    *sl;

      sl = g_new (guchar, 3 * width);

      gimp_pixel_rgn_init (&src_pr, drawable,
                           0, 0, width, height, FALSE, FALSE);
      gimp_pixel_rgn_init (&dst_pr, drawable,
                           0, 0, width, height, TRUE, TRUE);
      for (i = 0; i < height; i++)
        {
          guchar *rr = tmp + 4 * i * width;
          guchar *sld = sl;

          gimp_pixel_rgn_get_rect (&src_pr, sl, 0, i, width, 1);
          for (j = 0; j < width; j++)
            {
              gint k, alpha = rr[3];

              for (k = 0; k < 3; k++)
                {
                  gint t = (rr[k] + ((sld[k] * (256-alpha)) >> 8));

                  if (t > 255) t = 255;
                  sld[k] = t;
                }
              rr += 4;
              sld += 3;
            }
          gimp_pixel_rgn_set_rect (&dst_pr, sl, 0, i, width, 1);
        }
      g_free (sl);
    }

  g_free (tmp);
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, 0, 0, width, height);
}

static void
file_response_callback (GtkFileChooser *chooser,
                        gint            response_id,
                        gpointer        data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *filename = gtk_file_chooser_get_filename (chooser);

      if (load_save)
        {
          FILE  *f;
          gint   i, c;
          gchar *ss;

          if (!g_file_test (filename, G_FILE_TEST_IS_REGULAR))
            {
              g_message (_("'%s' is not a regular file"),
                         gimp_filename_to_utf8 (filename));
              g_free (filename);
              return;
            }

          f = g_fopen (filename, "rb");

          if (f == NULL)
            {
              g_message (_("Could not open '%s' for reading: %s"),
                         gimp_filename_to_utf8 (filename), g_strerror (errno));
              g_free (filename);
              return;
            }

          i = 0;
          ss = buffer;
          do
            {
              c = getc (f);
              if (EOF == c)
                break;
              ss[i++] = c;
            }
          while (i < BUFFER_SIZE && ';' != c);
          parse_control_point (&ss, &config.cp);
          fclose (f);
          /* i want to update the existing dialogue, but it's
             too painful */
          gimp_set_data ("plug_in_flame", &config, sizeof (config));
          /* gtk_widget_destroy(dialog); */
          set_flame_preview ();
          set_edit_preview ();
        }
      else
        {
          FILE *f = g_fopen (filename, "wb");

          if (NULL == f)
            {
              g_message (_("Could not open '%s' for writing: %s"),
                         gimp_filename_to_utf8 (filename), g_strerror (errno));
              g_free (filename);
              return;
            }

          print_control_point (f, &config.cp, 0);
          fclose (f);
        }

      g_free (filename);
    }

  gtk_widget_destroy (GTK_WIDGET (chooser));

  if (! gtk_widget_get_sensitive (load_button))
    gtk_widget_set_sensitive (load_button, TRUE);

  if (! gtk_widget_get_sensitive (save_button))
    gtk_widget_set_sensitive (save_button, TRUE);
}

static void
make_file_dialog (const gchar *title,
                  GtkWidget   *parent)
{
  file_dialog = gtk_file_chooser_dialog_new (title, GTK_WINDOW (parent),
                                             load_save ?
                                             GTK_FILE_CHOOSER_ACTION_OPEN :
                                             GTK_FILE_CHOOSER_ACTION_SAVE,

                                             _("_Cancel"), GTK_RESPONSE_CANCEL,
                                             load_save ?
                                             _("_Open") : _("_Save"),
                                             GTK_RESPONSE_OK,

                                             NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (file_dialog), GTK_RESPONSE_OK);
  gimp_dialog_set_alternative_button_order (GTK_DIALOG (file_dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  if (! load_save)
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (file_dialog),
                                                    TRUE);

  g_object_add_weak_pointer (G_OBJECT (file_dialog), (gpointer) &file_dialog);

  gtk_window_set_destroy_with_parent (GTK_WINDOW (file_dialog), TRUE);

  g_signal_connect (file_dialog, "delete-event",
                    G_CALLBACK (gtk_true),
                    NULL);
  g_signal_connect (file_dialog, "response",
                    G_CALLBACK (file_response_callback),
                    NULL);
}

static void
randomize_callback (GtkWidget *widget,
                    gpointer   data)
{
  random_control_point (&edit_cp, config.variation);
  init_mutants ();
  set_edit_preview ();
}

static void
edit_response (GtkWidget *widget,
               gint       response_id,
               gpointer   data)
{
  gtk_widget_hide (widget);

  if (response_id == GTK_RESPONSE_OK)
    {
      config.cp = edit_cp;
      set_flame_preview ();
    }
}

static void
init_mutants (void)
{
  gint i;

  for (i = 0; i < NMUTANTS; i++)
    {
      mutants[i] = edit_cp;
      random_control_point (mutants + i, config.variation);
      if (VARIATION_SAME == config.variation)
        copy_variation (mutants + i, &edit_cp);
    }
}

static void
set_edit_preview (void)
{
  gint           i, j;
  guchar        *b;
  control_point  pcp;
  gint           nbytes = EDIT_PREVIEW_SIZE * EDIT_PREVIEW_SIZE * 3;

  static frame_spec pf = { 0.0, 0, 1, 0.0 };

  if (NULL == edit_previews[0])
    return;

  b = g_new (guchar, nbytes);
  maybe_init_cp ();
  drawable_to_cmap (&edit_cp);
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      {
        gint mut = i*3 + j;

        pf.cps = &pcp;
        if (1 == i && 1 == j)
          {
            pcp = edit_cp;
          }
        else
          {
            control_point ends[2];
            ends[0] = edit_cp;
            ends[1] = mutants[mut];
            ends[0].time = 0.0;
            ends[1].time = 1.0;
            interpolate (ends, 2, pick_speed, &pcp);
          }
        pcp.pixels_per_unit =
          (pcp.pixels_per_unit * EDIT_PREVIEW_SIZE) / pcp.width;
        pcp.width = EDIT_PREVIEW_SIZE;
        pcp.height = EDIT_PREVIEW_SIZE;

        pcp.sample_density = 1;
        pcp.spatial_oversample = 1;
        pcp.spatial_filter_radius = 0.5;

        drawable_to_cmap (&pcp);

        render_rectangle (&pf, b, EDIT_PREVIEW_SIZE, field_both, 3, NULL);

        gimp_preview_area_draw (GIMP_PREVIEW_AREA (edit_previews[mut]),
                                0, 0, EDIT_PREVIEW_SIZE, EDIT_PREVIEW_SIZE,
                                GIMP_RGB_IMAGE,
                                b,
                                EDIT_PREVIEW_SIZE * 3);
      }
  g_free (b);
}

static void
edit_preview_size_allocate (GtkWidget *widget)
{
  set_edit_preview ();
}

static void
preview_clicked (GtkWidget *widget,
                 gpointer   data)
{
  gint mut = GPOINTER_TO_INT (data);

  if (mut == 4)
    {
      control_point t = edit_cp;
      init_mutants ();
      edit_cp = t;
    }
  else
    {
      control_point ends[2];
      ends[0] = edit_cp;
      ends[1] = mutants[mut];
      ends[0].time = 0.0;
      ends[1].time = 1.0;
      interpolate (ends, 2, pick_speed, &edit_cp);
    }
  set_edit_preview ();
}

static void
edit_callback (GtkWidget *widget,
               GtkWidget *parent)
{
  edit_cp = config.cp;

  if (edit_dialog == NULL)
    {
      GtkWidget     *main_vbox;
      GtkWidget     *frame;
      GtkWidget     *grid;
      GtkWidget     *vbox;
      GtkWidget     *hbox;
      GtkWidget     *button;
      GtkWidget     *combo;
      GtkWidget     *label;
      GtkAdjustment *adj;
      gint           i, j;

      edit_dialog = gimp_dialog_new (_("Edit Flame"), PLUG_IN_ROLE,
                                     parent, GTK_DIALOG_DESTROY_WITH_PARENT,
                                     gimp_standard_help_func, PLUG_IN_PROC,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_OK"),     GTK_RESPONSE_OK,

                                     NULL);

      gimp_dialog_set_alternative_button_order (GTK_DIALOG (edit_dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_signal_connect (edit_dialog, "response",
                        G_CALLBACK (edit_response),
                        edit_dialog);

      main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
      gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (edit_dialog))),
                          main_vbox, FALSE, FALSE, 0);

      frame = gimp_frame_new (_("Directions"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      grid = gtk_grid_new ();
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_show (grid);

      for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
          {
            gint mut = i * 3 + j;

            edit_previews[mut] = gimp_preview_area_new ();
            gtk_widget_set_size_request (edit_previews[mut],
                                         EDIT_PREVIEW_SIZE,
                                         EDIT_PREVIEW_SIZE);
            button = gtk_button_new ();
            gtk_widget_set_hexpand (button, TRUE);
            gtk_widget_set_halign (button, GTK_ALIGN_CENTER);
            gtk_container_add (GTK_CONTAINER(button), edit_previews[mut]);
            gtk_grid_attach (GTK_GRID (grid), button, i, j, 1, 1);
            gtk_widget_show (edit_previews[mut]);

            gtk_widget_show (button);

            g_signal_connect (button, "clicked",
                              G_CALLBACK (preview_clicked),
                              GINT_TO_POINTER (mut));
          }

      g_signal_connect (edit_previews[0], "size-allocate",
                        G_CALLBACK (edit_preview_size_allocate),
                        NULL);

      frame = gimp_frame_new (_("Controls"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      grid = gtk_grid_new ();
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
      gtk_widget_show(grid);

      adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 0,
                                  _("_Speed:"), SCALE_WIDTH, 0,
                                  pick_speed,
                                  0.05, 0.5, 0.01, 0.1, 2,
                                  TRUE, 0, 0,
                                  NULL, NULL);

      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (gimp_double_adjustment_update),
                        &pick_speed);
      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (set_edit_preview),
                        NULL);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      button = gtk_button_new_with_mnemonic( _("_Randomize"));
      g_object_set (gtk_bin_get_child (GTK_BIN (button)),
                    "margin-start", 2,
                    "margin-end",   2,
                    NULL);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_signal_connect_swapped (button, "clicked",
                                G_CALLBACK (randomize_callback),
                                NULL);

      combo = gimp_int_combo_box_new (_("Same"),         VARIATION_SAME,
                                      _("Random"),       variation_random,
                                      _("Linear"),       0,
                                      _("Sinusoidal"),   1,
                                      _("Spherical"),    2,
                                      _("Swirl"),        3,
                                      _("Horseshoe"),    4,
                                      _("Polar"),        5,
                                      _("Bent"),         6,
                                      _("Handkerchief"), 7,
                                      _("Heart"),        8,
                                      _("Disc"),         9,
                                      _("Spiral"),       10,
                                      _("Hyperbolic"),   11,
                                      _("Diamond"),      12,
                                      _("Ex"),           13,
                                      _("Julia"),        14,
                                      _("Waves"),        15,
                                      _("Fisheye"),      16,
                                      _("Popcorn"),      17,
                                      _("Exponential"),  18,
                                      _("Power"),        19,
                                      _("Cosine"),       20,
                                      _("Rings"),        21,
                                      _("Fan"),          22,
                                      _("Eyefish"),      23,
                                      _("Bubble"),       24,
                                      _("Cylinder"),     25,
                                      _("Noise"),        26,
                                      _("Blur"),         27,
                                      _("Gaussian"),     28,
                                      NULL);

      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                     VARIATION_SAME);

      g_signal_connect (combo, "changed",
                        G_CALLBACK (combo_callback),
                        &config.variation);

      gtk_box_pack_end (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
      gtk_widget_show (combo);

      label = gtk_label_new_with_mnemonic (_("_Variation:"));
      gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
      gtk_widget_show (label);

      gtk_widget_show (main_vbox);

      init_mutants ();
    }

  set_edit_preview ();

  gtk_window_present (GTK_WINDOW (edit_dialog));
}

static void
load_callback (GtkWidget *widget,
               gpointer   data)
{
  if (! file_dialog)
    {
      load_save = 1;
      make_file_dialog (_("Load Flame"), gtk_widget_get_toplevel (widget));

      gtk_widget_set_sensitive (save_button, FALSE);
    }

  gtk_window_present (GTK_WINDOW (file_dialog));
}

static void
save_callback (GtkWidget *widget,
               gpointer   data)
{
  if (! file_dialog)
    {
      load_save = 0;
      make_file_dialog (_("Save Flame"), gtk_widget_get_toplevel (widget));

      gtk_widget_set_sensitive (load_button, FALSE);
    }

  gtk_window_present (GTK_WINDOW (file_dialog));
}

static void
combo_callback (GtkWidget *widget,
                gpointer   data)
{
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), (gint *) data);

  if (VARIATION_SAME != config.variation)
    random_control_point (&edit_cp, config.variation);

  init_mutants ();
  set_edit_preview ();
}

static void
set_flame_preview (void)
{
  guchar *b;
  control_point pcp;

  static frame_spec pf = {0.0, 0, 1, 0.0};

  if (NULL == flame_preview)
    return;

  b = g_new (guchar, preview_width * preview_height * 3);

  maybe_init_cp ();
  drawable_to_cmap (&config.cp);

  pf.cps = &pcp;
  pcp = config.cp;
  pcp.pixels_per_unit =
    (pcp.pixels_per_unit * preview_width) / pcp.width;
  pcp.width = preview_width;
  pcp.height = preview_height;
  pcp.sample_density = 1;
  pcp.spatial_oversample = 1;
  pcp.spatial_filter_radius = 0.1;
  render_rectangle (&pf, b, preview_width, field_both, 3, NULL);

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (flame_preview),
                          0, 0, preview_width, PREVIEW_SIZE,
                          GIMP_RGB_IMAGE,
                          b,
                          preview_width * 3);
  g_free (b);
}

static void
flame_preview_size_allocate (GtkWidget *preview)
{
  set_flame_preview ();
}

static void
set_cmap_preview (void)
{
  gint i, x, y;
  guchar b[96];
  guchar *cmap_buffer, *ptr;

  if (NULL == cmap_preview)
    return;

  drawable_to_cmap (&config.cp);

  cmap_buffer = g_new (guchar, 32*32*3);
  ptr = cmap_buffer;
  for (y = 0; y < 32; y += 4)
    {
      for (x = 0; x < 32; x++)
        {
          gint j;

          i = x + (y/4) * 32;
          for (j = 0; j < 3; j++)
            b[x*3+j] = config.cp.cmap[i][j]*255.0;
        }

      memcpy (ptr, b, 32*3);
      ptr += 32*3;
      memcpy (ptr, b, 32*3);
      ptr += 32*3;
      memcpy (ptr, b, 32*3);
      ptr += 32*3;
      memcpy (ptr, b, 32*3);
      ptr += 32*3;
    }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (cmap_preview),
                          0, 0, 32, 32,
                          GIMP_RGB_IMAGE,
                          cmap_buffer,
                          32 * 3);
  g_free (cmap_buffer);
}

static void
cmap_callback (GtkWidget *widget,
               gpointer   data)
{
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget),
                                 &config.cmap_drawable);

  set_cmap_preview ();
  set_flame_preview ();
  /* set_edit_preview(); */
}

static gboolean
cmap_constrain (gint32   image_id,
                gint32   drawable_id,
                gpointer data)
{
  return ! gimp_drawable_is_indexed (drawable_id);
}


static gboolean
flame_dialog (void)
{
  GtkWidget     *main_vbox;
  GtkWidget     *notebook;
  GtkWidget     *label;
  GtkWidget     *frame;
  GtkWidget     *abox;
  GtkWidget     *button;
  GtkWidget     *grid;
  GtkWidget     *box;
  GtkAdjustment *adj;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Flame"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  abox = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (box), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), frame);
  gtk_widget_show (frame);

  flame_preview = gimp_preview_area_new ();
  {
    gdouble aspect = config.cp.width / (double) config.cp.height;

    if (aspect > 1.0)
      {
        preview_width = PREVIEW_SIZE;
        preview_height = PREVIEW_SIZE / aspect;
      }
    else
      {
        preview_width = PREVIEW_SIZE * aspect;
        preview_height = PREVIEW_SIZE;
      }
  }
  gtk_widget_set_size_request (flame_preview, preview_width, preview_height);
  gtk_container_add (GTK_CONTAINER (frame), flame_preview);
  gtk_widget_show (flame_preview);
  g_signal_connect (flame_preview, "size-allocate",
                    G_CALLBACK (flame_preview_size_allocate), NULL);

  {
    GtkWidget *vbox;
    GtkWidget *vbbox;

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start (GTK_BOX (box), vbox, FALSE, FALSE, 0);
    gtk_widget_show (vbox);

    vbbox= gtk_button_box_new (GTK_ORIENTATION_VERTICAL);
    gtk_box_set_homogeneous (GTK_BOX (vbbox), FALSE);
    gtk_box_set_spacing (GTK_BOX (vbbox), 6);
    gtk_box_pack_start (GTK_BOX (vbox), vbbox, FALSE, FALSE, 0);
    gtk_widget_show (vbbox);

    button = gtk_button_new_with_mnemonic (_("_Edit"));
    gtk_box_pack_start (GTK_BOX (vbbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    g_signal_connect (button, "clicked",
                      G_CALLBACK (edit_callback),
                      dialog);

    load_button = button = gtk_button_new_with_mnemonic (_("_Open"));
    gtk_box_pack_start (GTK_BOX (vbbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    g_signal_connect (button, "clicked",
                      G_CALLBACK (load_callback),
                      NULL);

    save_button = button = gtk_button_new_with_mnemonic (_("_Save"));
    gtk_box_pack_start (GTK_BOX (vbbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    g_signal_connect (button, "clicked",
                      G_CALLBACK (save_callback),
                      NULL);
  }

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 12);
  label = gtk_label_new_with_mnemonic(_("_Rendering"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);
  gtk_widget_show (box);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (box), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 0,
                              _("_Brightness:"), SCALE_WIDTH, 5,
                              config.cp.brightness,
                              0, 5, 0.1, 1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &config.cp.brightness);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (set_flame_preview),
                    NULL);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 1,
                              _("Co_ntrast:"), SCALE_WIDTH, 5,
                              config.cp.contrast,
                              0, 5, 0.1, 1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &config.cp.contrast);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (set_flame_preview),
                    NULL);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 2,
                              _("_Gamma:"), SCALE_WIDTH, 5,
                              config.cp.gamma,
                              1, 5, 0.1, 1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  gtk_widget_set_margin_bottom (gtk_grid_get_child_at (GTK_GRID (grid), 0, 2), 6);
  gtk_widget_set_margin_bottom (gtk_grid_get_child_at (GTK_GRID (grid), 1, 2), 6);
  gtk_widget_set_margin_bottom (gtk_grid_get_child_at (GTK_GRID (grid), 2, 2), 6);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &config.cp.gamma);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (set_flame_preview),
                    NULL);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 3,
                              _("Sample _density:"), SCALE_WIDTH, 5,
                              config.cp.sample_density,
                              0.1, 20, 1, 5, 2,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &config.cp.sample_density);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 4,
                              _("Spa_tial oversample:"), SCALE_WIDTH, 5,
                              config.cp.spatial_oversample,
                              1, 4, 0.01, 0.1, 0,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &config.cp.spatial_oversample);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 5,
                              _("Spatial _filter radius:"), SCALE_WIDTH, 5,
                              config.cp.spatial_filter_radius,
                              0, 4, 0.2, 1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &config.cp.spatial_filter_radius);

  {
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *combo;

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    label = gtk_label_new_with_mnemonic (_("Color_map:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    combo = gimp_drawable_combo_box_new (cmap_constrain, NULL);

    gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

#if 0
    gimp_int_combo_box_prepend (GIMP_INT_COMBO_BOX (combo),
                                GIMP_INT_STORE_VALUE, BLACK_DRAWABLE,
                                GIMP_INT_STORE_LABEL, _("Black"),
                                -1);
#endif

    {
      static const gchar *names[] =
      {
        "sunny harvest",
        "rose",
        "calcoast09",
        "klee insula-dulcamara",
        "ernst anti-pope",
        "gris josette"
      };
      static const gint good[] = { 10, 20, 68, 79, 70, 75 };

      gint i;

      for (i = 0; i < G_N_ELEMENTS (good); i++)
        {
          gint value = TABLE_DRAWABLE - good[i];

          gimp_int_combo_box_prepend (GIMP_INT_COMBO_BOX (combo),
                                      GIMP_INT_STORE_VALUE, value,
                                      GIMP_INT_STORE_LABEL, names[i],
                                      -1);
        }
    }

    gimp_int_combo_box_prepend (GIMP_INT_COMBO_BOX (combo),
                                GIMP_INT_STORE_VALUE,     GRADIENT_DRAWABLE,
                                GIMP_INT_STORE_LABEL,     _("Custom gradient"),
                                GIMP_INT_STORE_ICON_NAME, GIMP_ICON_GRADIENT,
                                -1);

    gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                                config.cmap_drawable,
                                G_CALLBACK (cmap_callback),
                                NULL);

    gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
    gtk_widget_show (combo);

    cmap_preview = gimp_preview_area_new ();
    gtk_widget_set_size_request (cmap_preview, 32, 32);

    gtk_box_pack_end (GTK_BOX (hbox), cmap_preview, FALSE, FALSE, 0);
    gtk_widget_show (cmap_preview);

    set_cmap_preview ();
  }

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);

  label = gtk_label_new_with_mnemonic(_("C_amera"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), grid, label);
  gtk_widget_show (grid);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 0,
                              _("_Zoom:"), SCALE_WIDTH, 0,
                              config.cp.zoom,
                              -4, 4, 0.5, 1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &config.cp.zoom);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (set_flame_preview),
                    NULL);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 1,
                              _("_X:"), SCALE_WIDTH, 0,
                              config.cp.center[0],
                              -2, 2, 0.5, 1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &config.cp.center[0]);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (set_flame_preview),
                    NULL);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 2,
                              _("_Y:"), SCALE_WIDTH, 0,
                              config.cp.center[1],
                              -2, 2, 0.5, 1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &config.cp.center[1]);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (set_flame_preview),
                    NULL);

  set_flame_preview ();

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
