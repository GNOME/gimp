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

/*  Mosaic is a filter which transforms an image into
 *  what appears to be a mosaic, composed of small primitives,
 *  each of constant color and of an approximate size.
 *        Copyright (C) 1996  Spencer Kimball
 *        Speedups by Elliot Lee
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC    "plug-in-mosaic"
#define PLUG_IN_BINARY  "mosaic"

#define SCALE_WIDTH     150

#define HORIZONTAL        0
#define VERTICAL          1
#define SUPERSAMPLE       3
#define MAG_THRESHOLD     7.5
#define COUNT_THRESHOLD   0.1
#define MAX_POINTS        12

typedef enum
{
  SQUARES   = 0,
  HEXAGONS  = 1,
  OCTAGONS  = 2,
  TRIANGLES = 3
} TileType;

#define SMOOTH   0
#define ROUGH    1

#define BW       0
#define FG_BG    1


typedef struct
{
  gdouble x, y;
} Vertex;

typedef struct
{
  guint  npts;
  Vertex pts[MAX_POINTS];
} Polygon;

typedef struct
{
  gdouble base_x, base_y;
  gdouble norm_x, norm_y;
  gdouble light;
} SpecVec;

typedef struct
{
  gdouble  tile_size;
  gdouble  tile_height;
  gdouble  tile_spacing;
  gdouble  tile_neatness;
  gboolean tile_allow_split;
  gdouble  light_dir;
  gdouble  color_variation;
  gboolean antialiasing;
  gint     color_averaging;
  TileType tile_type;
  gint     tile_surface;
  gint     grout_color;
} MosaicVals;


/* Declare local functions.
 */
static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);
static void      mosaic (GimpDrawable     *drawable,
                         GimpPreview      *preview);

/*  user interface functions  */
static gboolean  mosaic_dialog     (GimpDrawable *drawable);

/*  gradient finding machinery  */
static void      find_gradients    (GimpDrawable *drawable,
                                    gdouble       std_dev,
                                    gint          x1,
                                    gint          y1,
                                    gint          width,
                                    gint          height,
                                    GimpPreview  *preview);
static void      find_max_gradient (GimpPixelRgn *src_rgn,
                                    GimpPixelRgn *dest_rgn);

/*  gaussian & 1st derivative  */
static void      gaussian_deriv    (GimpPixelRgn *src_rgn,
                                    GimpPixelRgn *dest_rgn,
                                    gint          direction,
                                    gdouble       std_dev,
                                    gint         *prog,
                                    gint          max_prog,
                                    gint          ith_prog,
                                    gint          x1,
                                    gint          y1,
                                    gint          x2,
                                    gint          y2,
                                    GimpPreview  *preview);
static void      make_curve        (gint         *curve,
                                    gint         *sum,
                                    gdouble       std_dev,
                                    gint          length);
static void      make_curve_d      (gint         *curve,
                                    gint         *sum,
                                    gdouble       std_dev,
                                    gint          length);

/*  grid creation and localization machinery  */
static gdouble   fp_rand               (gdouble val);
static void      grid_create_squares   (gint x1,
                                        gint y1,
                                        gint x2,
                                        gint y2);
static void      grid_create_hexagons  (gint x1,
                                        gint y1,
                                        gint x2,
                                        gint y2);
static void      grid_create_octagons  (gint x1,
                                        gint y1,
                                        gint x2,
                                        gint y2);
static void      grid_create_triangles (gint x1,
                                        gint y1,
                                        gint x2,
                                        gint y2);
static void      grid_localize         (gint x1,
                                        gint y1,
                                        gint x2,
                                        gint y2);

static void      grid_render           (GimpDrawable *drawable,
                                        gint          x1,
                                        gint          y1,
                                        gint          x2,
                                        gint          y2,
                                        GimpPreview  *preview);
static void      split_poly            (Polygon      *poly,
                                        GimpDrawable *drawable,
                                        guchar       *col,
                                        gdouble      *dir,
                                        gdouble       color_vary,
                                        gint          x1,
                                        gint          y1,
                                        gint          x2,
                                        gint          y2,
                                        guchar       *dest);
static void      clip_poly             (gdouble      *vec,
                                        gdouble      *pt,
                                        Polygon      *poly,
                                        Polygon      *new_poly);
static void      clip_point            (gdouble      *dir,
                                        gdouble      *pt,
                                        gdouble       x1,
                                        gdouble       y1,
                                        gdouble       x2,
                                        gdouble       y2,
                                        Polygon      *poly);
static void      process_poly          (Polygon      *poly,
                                        gboolean      allow_split,
                                        GimpDrawable *drawable,
                                        guchar       *col,
                                        gboolean      vary,
                                        gint          x1,
                                        gint          y1,
                                        gint          x2,
                                        gint          y2,
                                        guchar       *dest);
static void      render_poly           (Polygon      *poly,
                                        GimpDrawable *drawable,
                                        guchar       *col,
                                        gdouble       vary,
                                        gint          x1,
                                        gint          y1,
                                        gint          x2,
                                        gint          y2,
                                        guchar       *dest);
static void      find_poly_dir         (Polygon      *poly,
                                        guchar       *m_gr,
                                        guchar       *h_gr,
                                        guchar       *v_gr,
                                        gdouble      *dir,
                                        gdouble      *loc,
                                        gint          x1,
                                        gint          y1,
                                        gint          x2,
                                        gint          y2);
static void      find_poly_color       (Polygon      *poly,
                                        GimpDrawable *drawable,
                                        guchar       *col,
                                        double        vary,
                                        gint          x1,
                                        gint          y1,
                                        gint          x2,
                                        gint          y2);
static void      scale_poly            (Polygon      *poly,
                                        gdouble       cx,
                                        gdouble       cy,
                                        gdouble       scale);
static void      fill_poly_color       (Polygon      *poly,
                                        GimpDrawable *drawable,
                                        guchar       *col,
                                        gint          x1,
                                        gint          y1,
                                        gint          x2,
                                        gint          y2,
                                        guchar       *dest);
static void      fill_poly_image       (Polygon      *poly,
                                        GimpDrawable *drawable,
                                        gdouble       vary,
                                        gint          x1,
                                        gint          y1,
                                        gint          x2,
                                        gint          y2,
                                        guchar       *dest);

static void      calc_spec_vec         (SpecVec      *vec,
                                        gint          xs,
                                        gint          ys,
                                        gint          xe,
                                        gint          ye);
static gdouble   calc_spec_contrib     (SpecVec      *vec,
                                        gint          n,
                                        gdouble       x,
                                        gdouble       y);
static void      convert_segment       (gint          x1,
                                        gint          y1,
                                        gint          x2,
                                        gint          y2,
                                        gint          offset,
                                        gint         *min,
                                        gint         *max);
static void      polygon_add_point     (Polygon      *poly,
                                        gdouble       x,
                                        gdouble       y);
static gboolean  polygon_find_center   (Polygon      *poly,
                                        gdouble      *x,
                                        gdouble      *y);
static void      polygon_translate     (Polygon      *poly,
                                        gdouble       tx,
                                        gdouble       ty);
static void      polygon_scale         (Polygon      *poly,
                                        gdouble       scale);
static gboolean  polygon_extents       (Polygon      *poly,
                                        gdouble      *min_x,
                                        gdouble      *min_y,
                                        gdouble      *max_x,
                                        gdouble      *max_y);
static void      polygon_reset         (Polygon      *poly);

/*
 *  Some static variables
 */
static gdouble  std_dev = 1.0;
static gdouble  light_x;
static gdouble  light_y;
static gdouble  scale;
static guchar  *h_grad;
static guchar  *v_grad;
static guchar  *m_grad;
static Vertex  *grid;
static gint     grid_rows;
static gint     grid_cols;
static gint     grid_row_pad;
static gint     grid_col_pad;
static gint     grid_multiple;
static gint     grid_rowstride;
static guchar   back[4];
static guchar   fore[4];
static SpecVec  vecs[MAX_POINTS];


static MosaicVals mvals =
{
  15.0,        /* tile_size */
  4.0,         /* tile_height */
  1.0,         /* tile_spacing */
  0.65,        /* tile_neatness */
  TRUE,        /* tile_allow_split */
  135,         /* light_dir */
  0.2,         /* color_variation */
  TRUE,        /* antialiasing */
  1,           /* color_averaging  */
  HEXAGONS,    /* tile_type */
  SMOOTH,      /* tile_surface */
  BW           /* grout_color */
};

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
    { GIMP_PDB_INT32,    "run-mode",        "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",            "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",         "Input drawable" },
    { GIMP_PDB_FLOAT,    "tile-size",        "Average diameter of each tile (in pixels)" },
    { GIMP_PDB_FLOAT,    "tile-height",      "Apparent height of each tile (in pixels)" },
    { GIMP_PDB_FLOAT,    "tile-spacing",     "Inter-tile spacing (in pixels)" },
    { GIMP_PDB_FLOAT,    "tile-neatness",    "Deviation from perfectly formed tiles (0.0 - 1.0)" },
    { GIMP_PDB_INT32,    "tile-allow-split", "Allows splitting tiles at hard edges" },
    { GIMP_PDB_FLOAT,    "light-dir",        "Direction of light-source (in degrees)" },
    { GIMP_PDB_FLOAT,    "color-variation",  "Magnitude of random color variations (0.0 - 1.0)" },
    { GIMP_PDB_INT32,    "antialiasing",     "Enables smoother tile output at the cost of speed" },
    { GIMP_PDB_INT32,    "color-averaging",  "Tile color based on average of subsumed pixels" },
    { GIMP_PDB_INT32,    "tile-type",        "Tile geometry: { SQUARES (0), HEXAGONS (1), OCTAGONS (2), TRIANGLES (3) }" },
    { GIMP_PDB_INT32,    "tile-surface",     "Surface characteristics: { SMOOTH (0), ROUGH (1) }" },
    { GIMP_PDB_INT32,    "grout-color",      "Grout color (black/white or fore/background): { BW (0), FG_BG (1) }" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Convert the image into irregular tiles"),
                          "Help not yet written for this plug-in",
                          "Spencer Kimball",
                          "Spencer Kimball & Peter Mattis",
                          "1996",
                          N_("_Mosaic..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Distorts");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpDrawable      *drawable;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the active drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  set the tile cache size so that the gaussian blur works well  */
  gimp_tile_cache_ntiles (2 * (MAX (drawable->width,
                                    drawable->height) /
                                   gimp_tile_width () + 1));

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &mvals);

      /*  First acquire information with a dialog  */
      if (! mosaic_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 15)
        status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS)
        {
          mvals.tile_size = param[3].data.d_float;
          mvals.tile_height = param[4].data.d_float;
          mvals.tile_spacing = param[5].data.d_float;
          mvals.tile_neatness = param[6].data.d_float;
          mvals.tile_allow_split = (param[7].data.d_int32) ? TRUE : FALSE;
          mvals.light_dir = param[8].data.d_float;
          mvals.color_variation = param[9].data.d_float;
          mvals.antialiasing = (param[10].data.d_int32) ? TRUE : FALSE;
          mvals.color_averaging = (param[11].data.d_int32) ? TRUE : FALSE;
          mvals.tile_type = param[12].data.d_int32;
          mvals.tile_surface = param[13].data.d_int32;
          mvals.grout_color = param[14].data.d_int32;
        }
      if (status == GIMP_PDB_SUCCESS &&
          (mvals.tile_type < SQUARES || mvals.tile_type > TRIANGLES))
        status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS &&
          (mvals.tile_surface < SMOOTH || mvals.tile_surface > ROUGH))
        status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS &&
          (mvals.grout_color < BW || mvals.grout_color > FG_BG))
        status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &mvals);
      break;

    default:
      break;
    }

  /*  Create the mosaic  */
  if ((status == GIMP_PDB_SUCCESS) &&
      (gimp_drawable_is_rgb (drawable->drawable_id) ||
       gimp_drawable_is_gray (drawable->drawable_id)))
    {
      /*  run the effect  */
      mosaic (drawable, NULL);

      /*  If the run mode is interactive, flush the displays  */
      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /*  Store mvals data  */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &mvals, sizeof (MosaicVals));
    }
  else if (status == GIMP_PDB_SUCCESS)
    {
      /* gimp_message ("mosaic: cannot operate on indexed color images"); */
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
mosaic (GimpDrawable *drawable,
        GimpPreview  *preview)
{
  gint     x1, y1, x2, y2;
  gint     width, height;
  gint     alpha;
  GimpRGB  color;

  /*  Find the mask bounds  */
  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);
      x2 = x1 + width;
      y2 = y1 + height;
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
      width  = (x2 - x1);
      height = (y2 - y1);

      /*  progress bar for gradient finding  */
      gimp_progress_init (_("Finding edges"));
    }

  /*  Find the gradients  */
  find_gradients (drawable, std_dev, x1, y1, width, height, preview);

  /*  Create the tile geometry grid  */
  switch (mvals.tile_type)
    {
    case SQUARES:
      grid_create_squares (x1, y1, x2, y2);
      break;
    case HEXAGONS:
      grid_create_hexagons (x1, y1, x2, y2);
      break;
    case OCTAGONS:
      grid_create_octagons (x1, y1, x2, y2);
      break;
    case TRIANGLES:
      grid_create_triangles(x1, y1, x2, y2);
      break;
    default:
      break;
    }

  /*  Deform the tiles based on image content  */
  grid_localize (x1, y1, x2, y2);

  switch (mvals.grout_color)
    {
    case BW:
      fore[0] = fore[1] = fore[2] = 255;
      back[0] = back[1] = back[2] = 0;
      break;

    case FG_BG:
      gimp_context_get_foreground (&color);
      gimp_drawable_get_color_uchar (drawable->drawable_id, &color, fore);

      gimp_context_get_background (&color);
      gimp_drawable_get_color_uchar (drawable->drawable_id, &color, back);
      break;
    }

  alpha = drawable->bpp - 1;

  light_x = -cos (mvals.light_dir * G_PI / 180.0);
  light_y =  sin (mvals.light_dir * G_PI / 180.0);
  scale = (mvals.tile_spacing > mvals.tile_size / 2.0) ?
    0.5 : 1.0 - mvals.tile_spacing / mvals.tile_size;

  if (!preview)
    {
      /*  Progress bar for rendering tiles  */
      gimp_progress_init (_("Rendering tiles"));
    }

  /*  Render the tiles  */
  grid_render (drawable, x1, y1, x2, y2, preview);

  if (!preview)
    {
      /*  merge the shadow, update the drawable  */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1,
                            (x2 - x1), (y2 - y1));
    }
}

static gboolean
mosaic_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *toggle;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *combo;
  GtkWidget *table;
  GtkObject *scale_data;
  gint       row = 0;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Mosaic"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  /* A preview */
  preview = gimp_drawable_preview_new (drawable, NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (mosaic),
                            drawable);

  /*  The hbox -- splits the scripts and the info vbox  */
  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  table = gtk_table_new (7, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_widget_show (table);

  combo = gimp_int_combo_box_new (_("Squares"),            SQUARES,
                                  _("Hexagons"),           HEXAGONS,
                                  _("Octagons & squares"), OCTAGONS,
                                  _("Triangles"),          TRIANGLES,
                                  NULL);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              mvals.tile_type,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &mvals.tile_type);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("_Tiling primitives:"), 0.0, 0.5,
                             combo, 2, FALSE);

  g_signal_connect_swapped (combo, "changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                                     _("Tile _size:"), SCALE_WIDTH, 5,
                                     mvals.tile_size, 5.0, 100.0, 1.0, 10.0, 1,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mvals.tile_size);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                                     _("Tile _height:"), SCALE_WIDTH, 5,
                                     mvals.tile_height, 1.0, 50.0, 1.0, 10.0, 1,
                                     TRUE, 0, 0,
                                     NULL, NULL);

  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mvals.tile_height);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                                     _("Til_e spacing:"), SCALE_WIDTH, 5,
                                     mvals.tile_spacing, 1.0, 50.0, 1.0, 10.0, 1,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mvals.tile_spacing);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                                     _("Tile _neatness:"), SCALE_WIDTH, 5,
                                     mvals.tile_neatness,
                                     0.0, 1.0, 0.10, 0.1, 2,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mvals.tile_neatness);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                                     _("Light _direction:"), SCALE_WIDTH, 5,
                                     mvals.light_dir, 0.0, 360.0, 1.0, 15.0, 1,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mvals.light_dir);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                                     _("Color _variation:"), SCALE_WIDTH, 5,
                                     mvals.color_variation,
                                     0.0, 1.0, 0.01, 0.1, 2,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mvals.color_variation);
  g_signal_connect_swapped (scale_data, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /*  the vertical box and its toggle buttons  */
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  toggle = gtk_check_button_new_with_mnemonic (_("_Antialiasing"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), mvals.antialiasing);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mvals.antialiasing);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  toggle = gtk_check_button_new_with_mnemonic ( _("Co_lor averaging"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                mvals.color_averaging);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mvals.color_averaging);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  toggle = gtk_check_button_new_with_mnemonic ( _("Allo_w tile splitting"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                mvals.tile_allow_split);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mvals.tile_allow_split);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  toggle = gtk_check_button_new_with_mnemonic ( _("_Pitted surfaces"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                (mvals.tile_surface == ROUGH));
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mvals.tile_surface);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  toggle = gtk_check_button_new_with_mnemonic ( _("_FG/BG lighting"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                (mvals.grout_color == FG_BG));
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mvals.grout_color);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

/*
 *  Gradient finding machinery
 */

static void
find_gradients (GimpDrawable *drawable,
                gdouble       std_dev,
                gint          x1,
                gint          y1,
                gint          width,
                gint          height,
                GimpPreview  *preview)
{
  GimpPixelRgn  src_rgn;
  GimpPixelRgn  dest_rgn;
  gint          bytes;
  gint          i, j;
  guchar       *gr, *dh, *dv;
  gint          hmax, vmax;
  gint          row, rows;
  gint          ith_row;

  bytes = drawable->bpp;

  /*  allocate the gradient maps  */
  h_grad = g_new (guchar, width * height);
  v_grad = g_new (guchar, width * height);
  m_grad = g_new (guchar, width * height);

  /*  Calculate total number of rows to be processed  */
  rows = width * 2 + height * 2;
  ith_row = rows / 256;
  if (!ith_row)
    ith_row = 1;
  row = 0;

  /*  Get the horizontal derivative  */
  gimp_pixel_rgn_init (&src_rgn, drawable,
                       x1, y1, width, height,
                       FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable,
                       x1, y1, width, height,
                       preview == NULL, TRUE);
  gaussian_deriv (&src_rgn, &dest_rgn,
                  HORIZONTAL, std_dev, &row, rows, ith_row,
                  x1, y1, x1 + width, y1 + height, preview);

  gimp_pixel_rgn_init (&src_rgn, drawable,
                       x1, y1, width, height,
                       FALSE, TRUE);
  dest_rgn.x = dest_rgn.y = 0;
  dest_rgn.w = width;
  dest_rgn.h = height;
  dest_rgn.bpp = 1;
  dest_rgn.rowstride = width;
  dest_rgn.data = h_grad;
  find_max_gradient (&src_rgn, &dest_rgn);

  /*  Get the vertical derivative  */
  gimp_pixel_rgn_init (&src_rgn, drawable,
                       x1, y1, width, height,
                       FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable,
                       x1, y1, width, height,
                       preview == NULL, TRUE);
  gaussian_deriv (&src_rgn, &dest_rgn,
                  VERTICAL, std_dev, &row, rows, ith_row,
                  x1, y1, x1 + width, y1 + height, preview);

  gimp_pixel_rgn_init (&src_rgn, drawable,
                        x1, y1, width, height,
                        FALSE, TRUE);
  dest_rgn.x = dest_rgn.y = 0;
  dest_rgn.w = width;
  dest_rgn.h = height;
  dest_rgn.bpp = 1;
  dest_rgn.rowstride = width;
  dest_rgn.data = v_grad;
  find_max_gradient (&src_rgn, &dest_rgn);

  if (!preview)
    gimp_progress_update (1.0);

  /*  fill in the gradient map  */
  gr = m_grad;
  dh = h_grad;
  dv = v_grad;

  for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++, dh++, dv++, gr++)
        {
          /*  Find the gradient  */
          if (!j || !i || (j == width - 1) || (i == height - 1))
            {
              *gr = MAG_THRESHOLD;
            }
          else
            {
              hmax = *dh - 128;
              vmax = *dv - 128;

              *gr = (guchar) sqrt (SQR (hmax) + SQR (vmax));
            }
        }
    }
}


static void
find_max_gradient (GimpPixelRgn *src_rgn,
                   GimpPixelRgn *dest_rgn)
{
  guchar   *s, *d, *s_iter, *s_end;
  gpointer  pr;
  gint      i, j;
  gint      val;
  gint      max;

  /*  Find the maximum value amongst intensity channels  */
  pr = gimp_pixel_rgns_register (2, src_rgn, dest_rgn);
  while (pr)
    {
      s = src_rgn->data;
      d = dest_rgn->data;

      for (i = 0; i < src_rgn->h; i++)
        {
          for (j = 0; j < src_rgn->w; j++)
            {
              max = 0;
#ifndef SLOW_CODE
#define ABSVAL(x) ((x) >= 0 ? (x) : -(x))

              for (s_iter = s, s_end = s + src_rgn->bpp;
                   s_iter < s_end; s_iter++) {
                val = *s;
                if (ABSVAL(val) > ABSVAL(max))
                  max = val;
              }
              *d++ = max;
#else
              for (b = 0; b < src_rgn->bpp; b++)
                {
                  val = (gint) s[b] - 128;
                  if (abs (val) > abs (max))
                    max = val;
                }
              *d++ = (max + 128);
#endif
              s += src_rgn->bpp;
            }

          s += (src_rgn->rowstride - src_rgn->w * src_rgn->bpp);
          d += (dest_rgn->rowstride - dest_rgn->w);
        }

      pr = gimp_pixel_rgns_process (pr);
    }
}


/*********************************************/
/*   Functions for gaussian convolutions     */
/*********************************************/


static void
gaussian_deriv (GimpPixelRgn *src_rgn,
                GimpPixelRgn *dest_rgn,
                gint          type,
                gdouble       std_dev,
                gint         *prog,
                gint          max_prog,
                gint          ith_prog,
                gint          x1,
                gint          y1,
                gint          x2,
                gint          y2,
                GimpPreview  *preview)
{
  guchar *dest, *dp;
  guchar *src, *sp, *s;
  guchar *data;
  gint   *buf, *b;
  gint    chan;
  gint    i, row, col;
  gint    start, end;
  gint    curve_array [9];
  gint    sum_array [9];
  gint   *curve;
  gint   *sum;
  gint    bytes;
  gint    val;
  gint    total;
  gint    length;
  gint    initial_p[4], initial_m[4];

  bytes = src_rgn->bpp;

  /*  allocate buffers for get/set pixel region rows/cols  */
  length = MAX (src_rgn->w, src_rgn->h) * bytes;
  data = g_new (guchar, length * 2);
  src = data;
  dest = data + length;

#ifdef UNOPTIMIZED_CODE
  length = 3;    /*  static for speed  */
#else
  /* badhack :) */
# define length 3
#endif

  /*  initialize  */
  curve = curve_array + length;
  sum = sum_array + length;
  buf = g_new (gint, MAX ((x2 - x1), (y2 - y1)) * bytes);

  if (type == VERTICAL)
    {
      make_curve_d (curve, sum, std_dev, length);
      total = sum[0] * -2;
    }
  else
    {
      make_curve (curve, sum, std_dev, length);
      total = sum[length] + curve[length];
    }

  for (col = x1; col < x2; col++)
    {
      gimp_pixel_rgn_get_col (src_rgn, src, col, y1, (y2 - y1));

      sp = src;
      dp = dest;
      b = buf;

      for (chan = 0; chan < bytes; chan++)
        {
          initial_p[chan] = sp[chan];
          initial_m[chan] = sp[(y2 - y1 - 1) * bytes + chan];
        }

      for (row = y1; row < y2; row++)
        {
          start = ((row - y1) < length) ? (y1 - row) : -length;
          end = ((y2 - row - 1) < length) ? (y2 - row - 1) : length;

          for (chan = 0; chan < bytes; chan++)
            {
              s = sp + (start * bytes) + chan;
              val = 0;
              i = start;

              if (start != -length)
                val += initial_p[chan] * (sum[start] - sum[-length]);

              while (i <= end)
                {
                  val += *s * curve[i++];
                  s += bytes;
                }

              if (end != length)
                val += initial_m[chan] * (sum[length] + curve[length] - sum[end+1]);

              *b++ = val / total;
            }

          sp += bytes;
        }

      b = buf;
      if (type == VERTICAL)
        for (row = y1; row < y2; row++)
          {
            for (chan = 0; chan < bytes; chan++)
              {
                b[chan] += 128;
                dp[chan] = CLAMP0255 (b[chan]);
              }
            b += bytes;
            dp += bytes;
          }
      else
        for (row = y1; row < y2; row++)
          {
            for (chan = 0; chan < bytes; chan++)
              {
                dp[chan] = CLAMP0255 (b[chan]);
              }
            b += bytes;
            dp += bytes;
          }

      gimp_pixel_rgn_set_col (dest_rgn, dest, col, y1, (y2 - y1));

      if (! ((*prog)++ % ith_prog) && !preview)
        gimp_progress_update ((gdouble) *prog / (gdouble) max_prog);
    }

  if (type == HORIZONTAL)
    {
      make_curve_d (curve, sum, std_dev, length);
      total = sum[0] * -2;
    }
  else
    {
      make_curve (curve, sum, std_dev, length);
      total = sum[length] + curve[length];
    }

  for (row = y1; row < y2; row++)
    {
      gimp_pixel_rgn_get_row (dest_rgn, src, x1, row, (x2 - x1));

      sp = src;
      dp = dest;
      b = buf;

      for (chan = 0; chan < bytes; chan++)
        {
          initial_p[chan] = sp[chan];
          initial_m[chan] = sp[(x2 - x1 - 1) * bytes + chan];
        }

      for (col = x1; col < x2; col++)
        {
          start = ((col - x1) < length) ? (x1 - col) : -length;
          end = ((x2 - col - 1) < length) ? (x2 - col - 1) : length;

          for (chan = 0; chan < bytes; chan++)
            {
              s = sp + (start * bytes) + chan;
              val = 0;
              i = start;

              if (start != -length)
                val += initial_p[chan] * (sum[start] - sum[-length]);

              while (i <= end)
                {
                  val += *s * curve[i++];
                  s += bytes;
                }

              if (end != length)
                val += initial_m[chan] * (sum[length] + curve[length] - sum[end+1]);

              *b++ = val / total;
            }

          sp += bytes;
        }

      b = buf;
      if (type == HORIZONTAL)
        for (col = x1; col < x2; col++)
          {
            for (chan = 0; chan < bytes; chan++)
              {
                b[chan] += 128;
                dp[chan] = CLAMP0255 (b[chan]);
              }
            b += bytes;
            dp += bytes;
          }
      else
        for (col = x1; col < x2; col++)
          {
            for (chan = 0; chan < bytes; chan++)
              {
                dp[chan] = CLAMP0255 (b[chan]);
              }
            b += bytes;
            dp += bytes;
          }

      gimp_pixel_rgn_set_row (dest_rgn, dest, x1, row, (x2 - x1));

      if (! ((*prog)++ % ith_prog) && !preview)
        gimp_progress_update ((gdouble) *prog / (gdouble) max_prog);
    }

  g_free (buf);
  g_free (data);
#ifndef UNOPTIMIZED_CODE
  /* end bad hack */
#undef length
#endif
}

/*
 * The equations: g(r) = exp (- r^2 / (2 * sigma^2))
 *                   r = sqrt (x^2 + y ^2)
 */

static void
make_curve (gint    *curve,
            gint    *sum,
            gdouble  sigma,
            gint     length)
{
  gdouble sigma2;
  gint i;

  sigma2 = sigma * sigma;

  curve[0] = 255;
  for (i = 1; i <= length; i++)
    {
      curve[i] = (gint) (exp (- (i * i) / (2 * sigma2)) * 255);
      curve[-i] = curve[i];
    }

  sum[-length] = 0;
  for (i = -length+1; i <= length; i++)
    sum[i] = sum[i-1] + curve[i-1];
}


/*
 * The equations: d_g(r) = -r * exp (- r^2 / (2 * sigma^2)) / sigma^2
 *                   r = sqrt (x^2 + y ^2)
 */

static void
make_curve_d (gint    *curve,
              gint    *sum,
              gdouble  sigma,
              gint     length)
{
  gdouble sigma2;
  gint    i;

  sigma2 = sigma * sigma;

  curve[0] = 0;
  for (i = 1; i <= length; i++)
    {
      curve[i] = (gint) ((i * exp (- (i * i) / (2 * sigma2)) / sigma2) * 255);
      curve[-i] = -curve[i];
    }

  sum[-length] = 0;
  sum[0] = 0;
  for (i = 1; i <= length; i++)
    {
      sum[-length + i] = sum[-length + i - 1] + curve[-length + i - 1];
      sum[i] = sum[i - 1] + curve[i - 1];
    }
}


/*********************************************/
/*   Functions for grid manipulation         */
/*********************************************/

static gdouble
fp_rand (gdouble val)
{
  return g_random_double () * val;
}

static void
grid_create_squares (gint x1,
                     gint y1,
                     gint x2,
                     gint y2)
{
  gint    rows, cols;
  gint    width, height;
  gint    i, j;
  gint    size = (gint) mvals.tile_size;
  Vertex *pt;

  width  = x2 - x1;
  height = y2 - y1;
  rows = (height + size - 1) / size;
  cols = (width + size - 1) / size;

  grid = g_new (Vertex, (cols + 2) * (rows + 2));
  grid += (cols + 2) + 1;

  for (i = -1; i <= rows; i++)
    for (j = -1; j <= cols; j++)
      {
        pt = grid + (i * (cols + 2) + j);

        pt->x = x1 + j * size + size / 2;
        pt->y = y1 + i * size + size / 2;
      }

  grid_rows = rows;
  grid_cols = cols;
  grid_row_pad = 1;
  grid_col_pad = 1;
  grid_multiple = 1;
  grid_rowstride = cols + 2;
}

static void
grid_create_hexagons (gint x1,
                      gint y1,
                      gint x2,
                      gint y2)
{
  gint     rows, cols;
  gint     width, height;
  gint     i, j;
  gdouble  hex_l1, hex_l2, hex_l3;
  gdouble  hex_width;
  gdouble  hex_height;
  Vertex  *pt;

  width  = x2 - x1;
  height = y2 - y1;
  hex_l1 = mvals.tile_size / 2.0;
  hex_l2 = hex_l1 * 2.0 / sqrt (3.0);
  hex_l3 = hex_l1 / sqrt (3.0);
  hex_width = 6 * hex_l1 / sqrt (3.0);
  hex_height = mvals.tile_size;
  rows = ((height + hex_height - 1) / hex_height);
  cols = ((width + hex_width * 2 - 1) / hex_width);

  grid = g_new (Vertex, (cols + 2) * 4 * (rows + 2));
  grid += (cols + 2) * 4 + 4;

  for (i = -1; i <= rows; i++)
    for (j = -1; j <= cols; j++)
      {
        pt = grid + (i * (cols + 2) * 4 + j * 4);

        pt[0].x = x1 + hex_width * j + hex_l3;
        pt[0].y = y1 + hex_height * i;
        pt[1].x = pt[0].x + hex_l2;
        pt[1].y = pt[0].y;
        pt[2].x = pt[1].x + hex_l3;
        pt[2].y = pt[1].y + hex_l1;
        pt[3].x = pt[0].x - hex_l3;
        pt[3].y = pt[0].y + hex_l1;
      }

  grid_rows = rows;
  grid_cols = cols;
  grid_row_pad = 1;
  grid_col_pad = 1;
  grid_multiple = 4;
  grid_rowstride = (cols + 2) * 4;
}


static void
grid_create_octagons (gint x1,
                      gint y1,
                      gint x2,
                      gint y2)
{
  gint     rows, cols;
  gint     width, height;
  gint     i, j;
  gdouble  ts, side, leg;
  gdouble  oct_size;
  Vertex  *pt;

  width = x2 - x1;
  height = y2 - y1;

  ts = mvals.tile_size;
  side = ts / (sqrt (2.0) + 1.0);
  leg = side * sqrt (2.0) * 0.5;
  oct_size = ts + side;

  rows = ((height + oct_size - 1) / oct_size);
  cols = ((width + oct_size * 2 - 1) / oct_size);

  grid = g_new (Vertex, (cols + 2) * 8 * (rows + 2));
  grid += (cols + 2) * 8 + 8;

  for (i = -1; i < rows + 1; i++)
    for (j = -1; j < cols + 1; j++)
      {
        pt = grid + (i * (cols + 2) * 8 + j * 8);

        pt[0].x = x1 + oct_size * j;
        pt[0].y = y1 + oct_size * i;
        pt[1].x = pt[0].x + side;
        pt[1].y = pt[0].y;
        pt[2].x = pt[0].x + leg + side;
        pt[2].y = pt[0].y + leg;
        pt[3].x = pt[2].x;
        pt[3].y = pt[0].y + leg + side;
        pt[4].x = pt[1].x;
        pt[4].y = pt[0].y + 2 * leg + side;
        pt[5].x = pt[0].x;
        pt[5].y = pt[4].y;
        pt[6].x = pt[0].x - leg;
        pt[6].y = pt[3].y;
        pt[7].x = pt[6].x;
        pt[7].y = pt[2].y;
      }

  grid_rows = rows;
  grid_cols = cols;
  grid_row_pad = 1;
  grid_col_pad = 1;
  grid_multiple = 8;
  grid_rowstride = (cols + 2) * 8;
}


static void
grid_create_triangles (gint x1,
                       gint y1,
                       gint x2,
                       gint y2)
{
  gint     rows, cols;
  gint     width, height;
  gint     i, j;
  gdouble  tri_mid, tri_height;
  Vertex  *pt;

  width  = x2 - x1;
  height = y2 - y1;
  tri_mid    = mvals.tile_size / 2.0;              /* cos 60 */
  tri_height = mvals.tile_size / 2.0 * sqrt (3.0); /* sin 60 */

  rows = (height + 2 * tri_height - 1) / (2 * tri_height);
  cols = (width + mvals.tile_size - 1) / mvals.tile_size;

  grid = g_new (Vertex, (cols + 2) * 2 * (rows + 2));
  grid += (cols + 2) * 2 + 2;

  for (i = -1; i <= rows; i++)
    for (j = -1; j <= cols; j++)
      {
        pt = grid + (i * (cols + 2) * 2 + j * 2);

        pt[0].x = x1 + mvals.tile_size * j;
        pt[0].y = y1 + (tri_height*2) * i;
        pt[1].x = pt[0].x + tri_mid;
        pt[1].y = pt[0].y + tri_height;
      }

  grid_rows = rows;
  grid_cols = cols;
  grid_row_pad = 1;
  grid_col_pad = 1;
  grid_multiple = 2;
  grid_rowstride = (cols + 2) * 2;
}


static void
grid_localize (gint x1,
               gint y1,
               gint x2,
               gint y2)
{
  gint     width, height;
  gint     i, j;
  gint     k, l;
  gint     x3, y3, x4, y4;
  gint     size;
  gint     max_x, max_y;
  gint     max;
  guchar  *data;
  gdouble  rand_localize;
  Vertex  *pt;

  width  = x2 - x1;
  height = y2 - y1;
  size = (gint) mvals.tile_size;
  rand_localize = size * (1.0 - mvals.tile_neatness);

  for (i = -grid_row_pad; i < grid_rows + grid_row_pad; i++)
    for (j = -grid_col_pad * grid_multiple; j < (grid_cols + grid_col_pad) * grid_multiple; j++)
      {
        pt = grid + (i * grid_rowstride + j);

        max_x = pt->x + (gint) (fp_rand (rand_localize) - rand_localize/2.0);
        max_y = pt->y + (gint) (fp_rand (rand_localize) - rand_localize/2.0);

        x3 = pt->x - (gint) (rand_localize / 2.0);
        y3 = pt->y - (gint) (rand_localize / 2.0);
        x4 = x3 + (gint) rand_localize;
        y4 = y3 + (gint) rand_localize;

        x3 = CLAMP (x3, x1, x2 - 1);
        y3 = CLAMP (y3, y1, y2 - 1);
        x4 = CLAMP (x4, x1, x2 - 1);
        y4 = CLAMP (y4, y1, y2 - 1);

        max = *(m_grad + (y3 - y1) * width + (x3 - x1));
        data = m_grad + width * (y3 - y1);

        for (k = y3; k <= y4; k++)
          {
            for (l = x3; l <= x4; l++)
              {
                if (data[l - x1] > max)
                  {
                    max_y = k;
                    max_x = l;
                    max = data[l - x1];
                  }
              }
            data += width;
          }

        pt->x = max_x;
        pt->y = max_y;
      }
}

static void
grid_render (GimpDrawable *drawable,
             gint          x1,
             gint          y1,
             gint          x2,
             gint          y2,
             GimpPreview  *preview)
{
  GimpPixelRgn  src_rgn;
  gint          i, j, k;
  guchar       *dest = NULL, *d;
  guchar        col[4];
  gint          bytes;
  gint          size, frac_size;
  gint          count;
  gint          index;
  gint          vary;
  Polygon       poly;
  gpointer      pr;

  bytes = drawable->bpp;

  if (preview)
    {
      dest = g_new (guchar, bytes * (x2 - x1) * (y2 - y1));

      d = dest;
      for (i = 0; i < (x2 - x1) * (y2 - y1); i++, d += bytes)
        memcpy (d, back, bytes);
    }
  else
    {
      /*  Fill the image with the background color  */
      gimp_pixel_rgn_init (&src_rgn, drawable,
                           x1, y1, (x2 - x1), (y2 - y1),
                           TRUE, TRUE);
      for (pr = gimp_pixel_rgns_register (1, &src_rgn);
           pr != NULL;
           pr = gimp_pixel_rgns_process (pr))
        {
          size = src_rgn.w * src_rgn.h;
          dest = src_rgn.data;

          for (i = 0; i < src_rgn.h ; i++)
            {
              d = dest;
              for (j = 0; j < src_rgn.w ; j++)
                {
                  for (k = 0; k < bytes; k++)
                    d[k] = back[k];
                  d += bytes;
                }
              dest += src_rgn.rowstride;
            }
        }
      dest = NULL;
    }

  size = (grid_rows + grid_row_pad) * (grid_cols + grid_col_pad);
  frac_size = (gint) (size * mvals.color_variation);
  count = 0;

  for (i = -grid_row_pad; i < grid_rows; i++)
    for (j = -grid_col_pad; j < grid_cols; j++)
      {
        vary = ((g_random_int_range (0, size)) < frac_size) ? 1 : 0;

        index = i * grid_rowstride + j * grid_multiple;

        switch (mvals.tile_type)
          {
          case SQUARES:
            polygon_reset (&poly);
            polygon_add_point (&poly,
                               grid[index].x,
                               grid[index].y);
            polygon_add_point (&poly,
                               grid[index + 1].x,
                               grid[index + 1].y);
            polygon_add_point (&poly,
                               grid[index + grid_rowstride + 1].x,
                               grid[index + grid_rowstride + 1].y);
            polygon_add_point (&poly,
                               grid[index + grid_rowstride].x,
                               grid[index + grid_rowstride].y);

            process_poly (&poly, mvals.tile_allow_split, drawable, col, vary,
                          x1, y1, x2, y2, dest);
            break;

          case HEXAGONS:
            /*  The main hexagon  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                               grid[index].x,
                               grid[index].y);
            polygon_add_point (&poly,
                               grid[index + 1].x,
                               grid[index + 1].y);
            polygon_add_point (&poly,
                               grid[index + 2].x,
                               grid[index + 2].y);
            polygon_add_point (&poly,
                               grid[index + grid_rowstride + 1].x,
                               grid[index + grid_rowstride + 1].y);
            polygon_add_point (&poly,
                               grid[index + grid_rowstride].x,
                               grid[index + grid_rowstride].y);
            polygon_add_point (&poly,
                               grid[index + 3].x,
                               grid[index + 3].y);
            process_poly (&poly, mvals.tile_allow_split, drawable, col, vary,
                          x1, y1, x2, y2, dest);

            /*  The auxillary hexagon  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                               grid[index + 2].x,
                               grid[index + 2].y);
            polygon_add_point (&poly,
                               grid[index + grid_multiple * 2 - 1].x,
                               grid[index + grid_multiple * 2 - 1].y);
            polygon_add_point (&poly,
                               grid[index + grid_rowstride + grid_multiple].x,
                               grid[index + grid_rowstride + grid_multiple].y);
            polygon_add_point (&poly,
                               grid[index + grid_rowstride + grid_multiple + 3].x,
                               grid[index + grid_rowstride + grid_multiple + 3].y);
            polygon_add_point (&poly,
                               grid[index + grid_rowstride + 2].x,
                               grid[index + grid_rowstride + 2].y);
            polygon_add_point (&poly,
                               grid[index + grid_rowstride + 1].x,
                               grid[index + grid_rowstride + 1].y);
            process_poly (&poly, mvals.tile_allow_split, drawable, col, vary,
                          x1, y1, x2, y2, dest);
            break;

          case OCTAGONS:
            /*  The main octagon  */
            polygon_reset (&poly);
            for (k = 0; k < 8; k++)
              polygon_add_point (&poly,
                                 grid[index + k].x,
                                 grid[index + k].y);
            process_poly (&poly, mvals.tile_allow_split, drawable, col, vary,
                          x1, y1, x2, y2, dest);

            /*  The auxillary octagon  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                               grid[index + 3].x,
                               grid[index + 3].y);
            polygon_add_point (&poly,
                               grid[index + grid_multiple * 2 - 2].x,
                               grid[index + grid_multiple * 2 - 2].y);
            polygon_add_point (&poly,
                               grid[index + grid_multiple * 2 - 3].x,
                               grid[index + grid_multiple * 2 - 3].y);
            polygon_add_point (&poly,
                               grid[index + grid_rowstride + grid_multiple].x,
                               grid[index + grid_rowstride + grid_multiple].y);
            polygon_add_point (&poly,
                               grid[index + grid_rowstride + grid_multiple * 2 - 1].x,
                               grid[index + grid_rowstride + grid_multiple * 2 - 1].y);
            polygon_add_point (&poly,
                               grid[index + grid_rowstride + 2].x,
                               grid[index + grid_rowstride + 2].y);
            polygon_add_point (&poly,
                               grid[index + grid_rowstride + 1].x,
                               grid[index + grid_rowstride + 1].y);
            polygon_add_point (&poly,
                               grid[index + 4].x,
                               grid[index + 4].y);
            process_poly (&poly, mvals.tile_allow_split, drawable, col, vary,
                          x1, y1, x2, y2, dest);

            /*  The main square  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                               grid[index + 2].x,
                               grid[index + 2].y);
            polygon_add_point (&poly,
                               grid[index + grid_multiple * 2 - 1].x,
                               grid[index + grid_multiple * 2 - 1].y);
            polygon_add_point (&poly,
                               grid[index + grid_multiple * 2 - 2].x,
                               grid[index + grid_multiple * 2 - 2].y);
            polygon_add_point (&poly,
                               grid[index + 3].x,
                               grid[index + 3].y);
            process_poly (&poly, FALSE, drawable, col, vary,
                          x1, y1, x2, y2, dest);

            /*  The auxillary square  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                               grid[index + 5].x,
                               grid[index + 5].y);
            polygon_add_point (&poly,
                               grid[index + 4].x,
                               grid[index + 4].y);
            polygon_add_point (&poly,
                               grid[index + grid_rowstride + 1].x,
                               grid[index + grid_rowstride + 1].y);
            polygon_add_point (&poly,
                               grid[index + grid_rowstride].x,
                               grid[index + grid_rowstride].y);
            process_poly (&poly, FALSE, drawable, col, vary,
                          x1, y1, x2, y2, dest);
            break;
          case TRIANGLES:
            /*  Lower left  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                                grid[index].x,
                                grid[index].y);
            polygon_add_point (&poly,
                                grid[index + grid_multiple].x,
                                grid[index + grid_multiple].y);
            polygon_add_point (&poly,
                                grid[index + 1].x,
                                grid[index + 1].y);
            process_poly (&poly, mvals.tile_allow_split, drawable, col, vary,
                          x1, y1, x2, y2, dest);

            /*  lower right  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                                grid[index + 1].x,
                                grid[index + 1].y);
            polygon_add_point (&poly,
                                grid[index + grid_multiple].x,
                                grid[index + grid_multiple].y);
            polygon_add_point (&poly,
                                grid[index + grid_multiple + 1].x,
                                grid[index + grid_multiple + 1].y);
            process_poly (&poly, mvals.tile_allow_split, drawable, col, vary,
                          x1, y1, x2, y2, dest);

            /*  upper left  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                                grid[index + 1].x,
                                grid[index + 1].y);
            polygon_add_point (&poly,
                                grid[index + grid_multiple + grid_rowstride].x,
                                grid[index + grid_multiple + grid_rowstride].y);
            polygon_add_point (&poly,
                                grid[index + grid_rowstride].x,
                                grid[index + grid_rowstride].y);
            process_poly (&poly, mvals.tile_allow_split, drawable, col, vary,
                           x1, y1, x2, y2, dest);

            /*  upper right  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                                grid[index + 1].x,
                                grid[index + 1].y);
            polygon_add_point (&poly,
                                grid[index + grid_multiple +1 ].x,
                                grid[index + grid_multiple +1 ].y);
            polygon_add_point (&poly,
                                grid[index + grid_multiple + grid_rowstride].x,
                                grid[index + grid_multiple + grid_rowstride].y);
            process_poly (&poly, mvals.tile_allow_split, drawable, col, vary,
                           x1, y1, x2, y2, dest);
            break;

          }

        if (!preview)
          gimp_progress_update ((double) count++ / (double) size);
      }

  if (preview)
    {
      gimp_preview_draw_buffer (preview,
                                dest,
                                (x2 - x1) * bytes);
    }
  else
    gimp_progress_update (1.0);
}

static void
process_poly (Polygon      *poly,
              gboolean      allow_split,
              GimpDrawable *drawable,
              guchar       *col,
              gboolean      vary,
              gint          x1,
              gint          y1,
              gint          x2,
              gint          y2,
              guchar       *dest)
{
  gdouble dir[2];
  gdouble loc[2];
  gdouble cx = 0.0, cy = 0.0;
  gdouble magnitude;
  gdouble distance;
  gdouble color_vary;

  /*  determine the variation of tile color based on tile number  */
  color_vary = vary ? fp_rand (mvals.color_variation) : 0;
  color_vary = (g_random_int_range (0, 2)) ? color_vary * 127 : -color_vary * 127;

  /*  Determine direction of edges inside polygon, if any  */
  find_poly_dir (poly, m_grad, h_grad, v_grad, dir, loc, x1, y1, x2, y2);
  magnitude = sqrt (SQR (dir[0] - 128) + SQR (dir[1] - 128));

  /*  Find the center of the polygon  */
  polygon_find_center (poly, &cx, &cy);
  distance = sqrt (SQR (loc[0] - cx) + SQR (loc[1] - cy));

  /*  If the magnitude of direction inside the polygon is greater than
   *  THRESHOLD, split the polygon into two new polygons
   */
  if (magnitude > MAG_THRESHOLD && (2 * distance / mvals.tile_size) < 0.5 && allow_split)
    split_poly (poly, drawable, col, dir, color_vary, x1, y1, x2, y2, dest);
  /*  Otherwise, render the original polygon
   */
  else
    render_poly (poly, drawable, col, color_vary, x1, y1, x2, y2, dest);
}

static void
render_poly (Polygon      *poly,
             GimpDrawable *drawable,
             guchar       *col,
             gdouble       vary,
             gint          x1,
             gint          y1,
             gint          x2,
             gint          y2,
             guchar       *dest)
{
  gdouble cx = 0.0, cy = 0.0;

  polygon_find_center (poly, &cx, &cy);

  if (mvals.color_averaging)
    find_poly_color (poly, drawable, col, vary, x1, y1, x2, y2);

  scale_poly (poly, cx, cy, scale);

  if (mvals.color_averaging)
    fill_poly_color (poly, drawable, col, x1, y1, x2, y2, dest);
  else
    fill_poly_image (poly, drawable, vary, x1, y1, x2, y2, dest);
}

static void
split_poly (Polygon      *poly,
            GimpDrawable *drawable,
            guchar       *col,
            gdouble      *dir,
            gdouble       vary,
            gint          x1,
            gint          y1,
            gint          x2,
            gint          y2,
            guchar       *dest)
{
  Polygon new_poly;
  gdouble spacing;
  gdouble cx = 0.0, cy = 0.0;
  gdouble magnitude;
  gdouble vec[2];
  gdouble pt[2];

  spacing = mvals.tile_spacing / (2.0 * scale);

  polygon_find_center (poly, &cx, &cy);
  polygon_translate (poly, -cx, -cy);

  magnitude = sqrt (SQR (dir[0] - 128) + SQR (dir[1] - 128));
  vec[0] = -(dir[1] - 128) / magnitude;
  vec[1] = (dir[0] - 128) / magnitude;
  pt[0] = -vec[1] * spacing;
  pt[1] = vec[0] * spacing;

  polygon_reset (&new_poly);
  clip_poly (vec, pt, poly, &new_poly);
  polygon_translate (&new_poly, cx, cy);

  if (new_poly.npts)
    {
      if (mvals.color_averaging)
        find_poly_color (&new_poly, drawable, col, vary, x1, y1, x2, y2);
      scale_poly (&new_poly, cx, cy, scale);
      if (mvals.color_averaging)
        fill_poly_color (&new_poly, drawable, col, x1, y1, x2, y2, dest);
      else
        fill_poly_image (&new_poly, drawable, vary, x1, y1, x2, y2, dest);
    }

  vec[0] = -vec[0];
  vec[1] = -vec[1];
  pt[0] = -pt[0];
  pt[1] = -pt[1];

  polygon_reset (&new_poly);
  clip_poly (vec, pt, poly, &new_poly);
  polygon_translate (&new_poly, cx, cy);

  if (new_poly.npts)
    {
      if (mvals.color_averaging)
        find_poly_color (&new_poly, drawable, col, vary, x1, y1, x2, y2);
      scale_poly (&new_poly, cx, cy, scale);
      if (mvals.color_averaging)
        fill_poly_color (&new_poly, drawable, col, x1, y1, x2, y2, dest);
      else
        fill_poly_image (&new_poly, drawable, vary, x1, y1, x2, y2, dest);
    }
}

static void
clip_poly (gdouble  *dir,
           gdouble  *pt,
           Polygon  *poly,
           Polygon  *poly_new)
{
  gint    i;
  gdouble x1, y1, x2, y2;

  for (i = 0; i < poly->npts; i++)
    {
      x1 = (i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x;
      y1 = (i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y;
      x2 = poly->pts[i].x;
      y2 = poly->pts[i].y;

      clip_point (dir, pt, x1, y1, x2, y2, poly_new);
    }
}


static void
clip_point (gdouble *dir,
            gdouble *pt,
            gdouble  x1,
            gdouble  y1,
            gdouble  x2,
            gdouble  y2,
            Polygon *poly_new)
{
  gdouble det, m11, m12, m21, m22;
  gdouble side1, side2;
  gdouble t;
  gdouble vec[2];

  x1 -= pt[0]; x2 -= pt[0];
  y1 -= pt[1]; y2 -= pt[1];

  side1 = x1 * -dir[1] + y1 * dir[0];
  side2 = x2 * -dir[1] + y2 * dir[0];

  /*  If both points are to be clipped, ignore  */
  if (side1 < 0.0 && side2 < 0.0)
    return;
  /*  If both points are non-clipped, set point  */
  else if (side1 >= 0.0 && side2 >= 0.0)
    {
      polygon_add_point (poly_new, x2 + pt[0], y2 + pt[1]);
      return;
    }
  /*  Otherwise, there is an intersection...  */
  else
    {
      vec[0] = x1 - x2;
      vec[1] = y1 - y2;
      det = dir[0] * vec[1] - dir[1] * vec[0];

      if (det == 0.0)
        {
          polygon_add_point (poly_new, x2 + pt[0], y2 + pt[1]);
          return;
        }

      m11 = vec[1] / det;
      m12 = -vec[0] / det;
      m21 = -dir[1] / det;
      m22 = dir[0] / det;

      t = m11 * x1 + m12 * y1;

      /*  If the first point is clipped, set intersection and point  */
      if (side1 < 0.0 && side2 > 0.0)
        {
          polygon_add_point (poly_new, dir[0] * t + pt[0], dir[1] * t + pt[1]);
          polygon_add_point (poly_new, x2 + pt[0], y2 + pt[1]);
        }
      else
        polygon_add_point (poly_new, dir[0] * t + pt[0], dir[1] * t + pt[1]);
    }
}


static void
find_poly_dir (Polygon *poly,
               guchar  *m_gr,
               guchar  *h_gr,
               guchar  *v_gr,
               gdouble *dir,
               gdouble *loc,
               gint     x1,
               gint     y1,
               gint     x2,
               gint     y2)
{
  gdouble dmin_x = 0.0, dmin_y = 0.0;
  gdouble dmax_x = 0.0, dmax_y = 0.0;
  gint    xs, ys;
  gint    xe, ye;
  gint    min_x, min_y;
  gint    max_x, max_y;
  gint    size_x, size_y;
  gint   *max_scanlines;
  gint   *min_scanlines;
  guchar *dm, *dv, *dh;
  gint    count, total;
  gint    rowstride;
  gint    i, j;

  rowstride = (x2 - x1);
  count = 0;
  total = 0;
  dir[0] = 0.0;
  dir[1] = 0.0;
  loc[0] = 0.0;
  loc[1] = 0.0;

  polygon_extents (poly, &dmin_x, &dmin_y, &dmax_x, &dmax_y);
  min_x = (gint) dmin_x;
  min_y = (gint) dmin_y;
  max_x = (gint) dmax_x;
  max_y = (gint) dmax_y;
  size_y = max_y - min_y;
  size_x = max_x - min_x;

  min_scanlines = g_new (gint, size_y);
  max_scanlines = g_new (gint, size_y);
  for (i = 0; i < size_y; i++)
    {
      min_scanlines[i] = max_x;
      max_scanlines[i] = min_x;
    }

  for (i = 0; i < poly->npts; i++)
    {
      xs = (gint) ((i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x);
      ys = (gint) ((i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y);
      xe = (gint) poly->pts[i].x;
      ye = (gint) poly->pts[i].y;

      convert_segment (xs, ys, xe, ye, min_y,
                       min_scanlines, max_scanlines);
    }

  for (i = 0; i < size_y; i++)
    {
      if ((i + min_y) >= y1 && (i + min_y) < y2)
        {
          dm = m_gr + (i + min_y - y1) * rowstride - x1;
          dh = h_gr + (i + min_y - y1) * rowstride - x1;
          dv = v_gr + (i + min_y - y1) * rowstride - x1;

          for (j = min_scanlines[i]; j < max_scanlines[i]; j++)
            {
              if (j >= x1 && j < x2)
                {
                  if (dm[j] > MAG_THRESHOLD)
                    {
                      dir[0] += dh[j];
                      dir[1] += dv[j];
                      loc[0] += j;
                      loc[1] += i + min_y;
                      count++;
                    }
                  total++;
                }
            }
        }
    }

  if (!total)
    return;

  if ((gdouble) count / (gdouble) total > COUNT_THRESHOLD)
    {
      dir[0] /= count;
      dir[1] /= count;
      loc[0] /= count;
      loc[1] /= count;
    }
  else
    {
      dir[0] = 128.0;
      dir[1] = 128.0;
      loc[0] = 0.0;
      loc[1] = 0.0;
    }

  g_free (min_scanlines);
  g_free (max_scanlines);
}


static void
find_poly_color (Polygon      *poly,
                 GimpDrawable *drawable,
                 guchar       *col,
                 gdouble       color_var,
                 gint          x1,
                 gint          y1,
                 gint          x2,
                 gint          y2)
{
  GimpPixelRgn  src_rgn;
  gdouble       dmin_x = 0.0, dmin_y = 0.0;
  gdouble       dmax_x = 0.0, dmax_y = 0.0;
  gint          xs, ys;
  gint          xe, ye;
  gint          min_x, min_y;
  gint          max_x, max_y;
  gint          size_x, size_y;
  gint         *max_scanlines;
  gint         *min_scanlines;
  gint          col_sum[4] = {0, 0, 0, 0};
  gint          bytes;
  gint          b, count;
  gint          i, j, y;

  count = 0;

  bytes = drawable->bpp;

  polygon_extents (poly, &dmin_x, &dmin_y, &dmax_x, &dmax_y);
  min_x = (gint) dmin_x;
  min_y = (gint) dmin_y;
  max_x = (gint) dmax_x;
  max_y = (gint) dmax_y;

  size_y = max_y - min_y;
  size_x = max_x - min_x;

  min_scanlines = g_new (int, size_y);
  max_scanlines = g_new (int, size_y);
  for (i = 0; i < size_y; i++)
    {
      min_scanlines[i] = max_x;
      max_scanlines[i] = min_x;
    }

  for (i = 0; i < poly->npts; i++)
    {
      xs = (gint) ((i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x);
      ys = (gint) ((i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y);
      xe = (gint) poly->pts[i].x;
      ye = (gint) poly->pts[i].y;

      convert_segment (xs, ys, xe, ye, min_y,
                       min_scanlines, max_scanlines);
    }

  gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0,
                       drawable->width, drawable->height,
                       FALSE, FALSE);
  for (i = 0; i < size_y; i++)
    {
      y = i + min_y;
      if (y >= y1 && y < y2)
        {
          for (j = min_scanlines[i]; j < max_scanlines[i]; j++)
            {
              if (j >= x1 && j < x2)
                {
                  gimp_pixel_rgn_get_pixel (&src_rgn, col, j, y);

                  for (b = 0; b < bytes; b++)
                    col_sum[b] += col[b];

                  count++;
                }
            }
        }
    }

  if (count)
    for (b = 0; b < bytes; b++)
      {
        col_sum[b] = (gint) (col_sum[b] / count + color_var);
        col[b] = CLAMP0255 (col_sum[b]);
      }

  g_free (min_scanlines);
  g_free (max_scanlines);
}

static void
scale_poly (Polygon *poly,
            gdouble  tx,
            gdouble  ty,
            gdouble  poly_scale)
{
  polygon_translate (poly, -tx, -ty);
  polygon_scale (poly, poly_scale);
  polygon_translate (poly, tx, ty);
}

static void
fill_poly_color (Polygon      *poly,
                 GimpDrawable *drawable,
                 guchar       *col,
                 gint          x1,
                 gint          y1,
                 gint          x2,
                 gint          y2,
                 guchar       *dest)
{
  GimpPixelRgn  src_rgn;
  gdouble       dmin_x = 0.0, dmin_y = 0.0;
  gdouble       dmax_x = 0.0, dmax_y = 0.0;
  gint          xs, ys;
  gint          xe, ye;
  gint          min_x, min_y;
  gint          max_x, max_y;
  gint          size_x, size_y;
  gint         *max_scanlines, *max_scanlines_iter;
  gint         *min_scanlines, *min_scanlines_iter;
  gint         *vals;
  gint          val;
  gint          pixel;
  gint          bytes;
  guchar        buf[4];
  gint          b, i, j, k, x, y;
  gdouble       contrib;
  gdouble       xx, yy;
  gint          supersample;
  gint          supersample2;
  Vertex       *pts_tmp;
  const gint    poly_npts = poly->npts;

  /*  Determine antialiasing  */
  if (mvals.antialiasing)
    {
      supersample = SUPERSAMPLE;
      supersample2 = SQR (supersample);
    }
  else
    {
      supersample = supersample2 = 1;
    }

  bytes = drawable->bpp;

  /* begin loop */
  if(poly_npts) {
    pts_tmp = poly->pts;
    xs = (gint) pts_tmp[poly_npts - 1].x;
    ys = (gint) pts_tmp[poly_npts - 1].y;
    xe = (gint) pts_tmp->x;
    ye = (gint) pts_tmp->y;

    calc_spec_vec (vecs, xs, ys, xe, ye);

    for (i = 1; i < poly_npts; i++)
      {
        xs = (gint) (pts_tmp->x);
        ys = (gint) (pts_tmp->y);
        pts_tmp++;
        xe = (gint) pts_tmp->x;
        ye = (gint) pts_tmp->y;

        calc_spec_vec (vecs+i, xs, ys, xe, ye);
      }
  }
  /* end loop */

  polygon_extents (poly, &dmin_x, &dmin_y, &dmax_x, &dmax_y);
  min_x = (gint) dmin_x;
  min_y = (gint) dmin_y;
  max_x = (gint) dmax_x;
  max_y = (gint) dmax_y;

  size_y = (max_y - min_y) * supersample;
  size_x = (max_x - min_x) * supersample;

  min_scanlines = min_scanlines_iter = g_new (gint, size_y);
  max_scanlines = max_scanlines_iter = g_new (gint, size_y);
  for (i = 0; i < size_y; i++)
    {
      min_scanlines[i] = max_x * supersample;
      max_scanlines[i] = min_x * supersample;
    }

  /* begin loop */
  if(poly_npts) {
    pts_tmp = poly->pts;
    xs = (gint) pts_tmp[poly_npts-1].x;
    ys = (gint) pts_tmp[poly_npts-1].y;
    xe = (gint) pts_tmp->x;
    ye = (gint) pts_tmp->y;

    xs *= supersample;
    ys *= supersample;
    xe *= supersample;
    ye *= supersample;

    convert_segment (xs, ys, xe, ye, min_y * supersample,
                     min_scanlines, max_scanlines);

    for (i = 1; i < poly_npts; i++)
      {
        xs = (gint) pts_tmp->x;
        ys = (gint) pts_tmp->y;
        pts_tmp++;
        xe = (gint) pts_tmp->x;
        ye = (gint) pts_tmp->y;

        xs *= supersample;
        ys *= supersample;
        xe *= supersample;
        ye *= supersample;

        convert_segment (xs, ys, xe, ye, min_y * supersample,
                         min_scanlines, max_scanlines);
      }
  }
  /* end loop */

  gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0,
                       drawable->width, drawable->height,
                       dest == NULL, TRUE);

  vals = g_new (gint, size_x);
  for (i = 0; i < size_y; i++, min_scanlines_iter++, max_scanlines_iter++)
    {
      if (! (i % supersample))
        memset (vals, 0, sizeof (gint) * size_x);

      yy = (gdouble) i / (gdouble) supersample + min_y;

      for (j = *min_scanlines_iter; j < *max_scanlines_iter; j++)
        {
          x = j - min_x * supersample;
          vals[x] += 255;
        }

      if (! ((i + 1) % supersample))
        {
          y = (i / supersample) + min_y;

          if (y >= y1 && y < y2)
            {
              for (j = 0; j < size_x; j += supersample)
                {
                  x = (j / supersample) + min_x;

                  if (x >= x1 && x < x2)
                    {
                      val = 0;
                      for (k = 0; k < supersample; k++)
                        val += vals[j + k];
                      val /= supersample2;

                      if (val > 0)
                        {

                          xx = (gdouble) j / (gdouble) supersample + min_x;
                          contrib = calc_spec_contrib (vecs, poly_npts, xx, yy);

                          for (b = 0; b < bytes; b++)
                            {
                              pixel = col[b] + (gint) (((contrib < 0.0)?(col[b] - back[b]):(fore[b] - col[b])) * contrib);

                              buf[b] = ((pixel * val) + (back[b] * (255 - val))) / 255;
                            }

                          if (dest)
                            memcpy (dest + ((y - y1) * (x2 - x1) + (x - x1)) * bytes, buf, bytes);
                          else
                            gimp_pixel_rgn_set_pixel (&src_rgn, buf, x, y);
                        }
                    }
                }
            }
        }
    }

  g_free (vals);
  g_free (min_scanlines);
  g_free (max_scanlines);
}

static void
fill_poly_image (Polygon      *poly,
                 GimpDrawable *drawable,
                 gdouble       vary,
                 gint          x1,
                 gint          y1,
                 gint          x2,
                 gint          y2,
                 guchar       *dest)
{
  GimpPixelRgn  src_rgn, dest_rgn;
  gdouble       dmin_x = 0.0, dmin_y = 0.0;
  gdouble       dmax_x = 0.0, dmax_y = 0.0;
  gint          xs, ys;
  gint          xe, ye;
  gint          min_x, min_y;
  gint          max_x, max_y;
  gint          size_x, size_y;
  gint         *max_scanlines;
  gint         *min_scanlines;
  gint         *vals;
  gint          val;
  gint          pixel;
  gint          bytes;
  guchar        buf[4];
  gint          b, i, j, k, x, y;
  gdouble       contrib;
  gdouble       xx, yy;
  gint          supersample;
  gint          supersample2;

  /*  Determine antialiasing  */
  if (mvals.antialiasing)
    {
      supersample = SUPERSAMPLE;
      supersample2 = SQR (supersample);
    }
  else
    {
      supersample = supersample2 = 1;
    }

  bytes = drawable->bpp;
  for (i = 0; i < poly->npts; i++)
    {
      xs = (gint) ((i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x);
      ys = (gint) ((i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y);
      xe = (gint) poly->pts[i].x;
      ye = (gint) poly->pts[i].y;

      calc_spec_vec (vecs+i, xs, ys, xe, ye);
    }

  polygon_extents (poly, &dmin_x, &dmin_y, &dmax_x, &dmax_y);
  min_x = (gint) dmin_x;
  min_y = (gint) dmin_y;
  max_x = (gint) dmax_x;
  max_y = (gint) dmax_y;

  size_y = (max_y - min_y) * supersample;
  size_x = (max_x - min_x) * supersample;

  min_scanlines = g_new (gint, size_y);
  max_scanlines = g_new (gint, size_y);
  for (i = 0; i < size_y; i++)
    {
      min_scanlines[i] = max_x * supersample;
      max_scanlines[i] = min_x * supersample;
    }

  for (i = 0; i < poly->npts; i++)
    {
      xs = (gint) ((i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x);
      ys = (gint) ((i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y);
      xe = (gint) poly->pts[i].x;
      ye = (gint) poly->pts[i].y;

      xs *= supersample;
      ys *= supersample;
      xe *= supersample;
      ye *= supersample;

      convert_segment (xs, ys, xe, ye, min_y * supersample,
                       min_scanlines, max_scanlines);
    }

  gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0,
                       drawable->width, drawable->height,
                       FALSE, FALSE);
  if (!dest)
    gimp_pixel_rgn_init (&dest_rgn, drawable, 0, 0,
                         drawable->width, drawable->height,
                         TRUE, TRUE);
  vals = g_new (gint, size_x);
  for (i = 0; i < size_y; i++)
    {
      if (! (i % supersample))
        memset (vals, 0, sizeof (gint) * size_x);

      yy = (gdouble) i / (gdouble) supersample + min_y;

      for (j = min_scanlines[i]; j < max_scanlines[i]; j++)
        {
          x = j - min_x * supersample;
          vals[x] += 255;
        }

      if (! ((i + 1) % supersample))
        {
          y = (i / supersample) + min_y;

          if (y >= y1 && y < y2)
            {
              for (j = 0; j < size_x; j += supersample)
                {
                  x = (j / supersample) + min_x;

                  if (x >= x1 && x < x2)
                    {
                      val = 0;
                      for (k = 0; k < supersample; k++)
                        val += vals[j + k];
                      val /= supersample2;

                      if (val > 0)
                        {
                          xx = (double) j / (double) supersample + min_x;
                          contrib = calc_spec_contrib (vecs, poly->npts, xx, yy);

                          gimp_pixel_rgn_get_pixel (&src_rgn, buf, x, y);

                          for (b = 0; b < bytes; b++)
                            {
                              if (contrib < 0.0)
                                pixel = buf[b] + (int) ((buf[b] - back[b]) * contrib);
                              else
                                pixel = buf[b] + (int) ((fore[b] - buf[b]) * contrib);

                              /*  factor in per-tile intensity variation  */
                              pixel += vary;
                              pixel = CLAMP (pixel, 0, 255);

                              buf[b] = ((back[b] << 8) + (pixel - back[b]) * val) >> 8;
                            }

                          if (dest)
                            memcpy (dest + ((y - y1) * (x2 - x1) + (x - x1)) * bytes, buf, bytes);
                          else
                            gimp_pixel_rgn_set_pixel (&dest_rgn, buf, x, y);
                        }
                    }
                }
            }
        }
    }

  g_free (vals);
  g_free (min_scanlines);
  g_free (max_scanlines);
}

static void
calc_spec_vec (SpecVec *vec,
               gint     x1,
               gint     y1,
               gint     x2,
               gint     y2)
{
  gdouble r;

  vec->base_x = x1;
  vec->base_y = y1;
  r = sqrt (SQR (x2 - x1) + SQR (y2 - y1));
  if (r > 0.0)
    {
      vec->norm_x = -(y2 - y1) / r;
      vec->norm_y = (x2 - x1) / r;
    }
  else
    {
      vec->norm_x = 0;
      vec->norm_y = 0;
    }

  vec->light = vec->norm_x * light_x + vec->norm_y * light_y;
}


static double
calc_spec_contrib (SpecVec *vecs,
                   gint     n,
                   gdouble  x,
                   gdouble  y)
{
  gint i;
  gdouble contrib = 0;

  for (i = 0; i < n; i++)
    {
      gdouble x_p, y_p;
      gdouble dist;

      x_p = x - vecs[i].base_x;
      y_p = y - vecs[i].base_y;

      dist = fabs (x_p * vecs[i].norm_x + y_p * vecs[i].norm_y);

      if (mvals.tile_surface == ROUGH)
        {
          /*  If the surface is rough, randomly perturb the distance  */
          dist -= dist * g_random_double ();
        }

      /*  If the distance to an edge is less than the tile_spacing, there
       *  will be no highlight as the tile blends to background here
       */
      if (dist < 1.0)
        contrib += vecs[i].light;
      else if (dist <= mvals.tile_height)
        contrib += vecs[i].light * (1.0 - (dist / mvals.tile_height));
    }

  return contrib / 4.0;
}

static void
convert_segment (gint  x1,
                 gint  y1,
                 gint  x2,
                 gint  y2,
                 gint  offset,
                 gint *min,
                 gint *max)
{
  gint    ydiff, y, tmp;
  gdouble xinc, xstart;

  if (y1 > y2)
    {
      tmp = y2; y2 = y1; y1 = tmp;
      tmp = x2; x2 = x1; x1 = tmp;
    }
  ydiff = y2 - y1;

  if (ydiff)
    {
      xinc = (gdouble) (x2 - x1) / (gdouble) ydiff;
      xstart = x1 + 0.5 * xinc;
      for (y = y1; y < y2; y++)
        {
          min[y - offset] = MIN (min[y - offset], xstart);
          max[y - offset] = MAX (max[y - offset], xstart);

          xstart += xinc;
        }
    }
}

static void
polygon_add_point (Polygon *poly,
                   gdouble  x,
                   gdouble  y)
{
  if (poly->npts < 12)
    {
      poly->pts[poly->npts].x = x;
      poly->pts[poly->npts].y = y;
      poly->npts++;
    }
  else
    g_print ( _("Unable to add additional point.\n"));
}

static gboolean
polygon_find_center (Polygon *poly,
                     gdouble *cx,
                     gdouble *cy)
{
  guint i;

  if (!poly->npts)
    return FALSE;

  *cx = 0.0;
  *cy = 0.0;

  for (i = 0; i < poly->npts; i++)
    {
      *cx += poly->pts[i].x;
      *cy += poly->pts[i].y;
    }

  *cx /= poly->npts;
  *cy /= poly->npts;

  return TRUE;
}

static void
polygon_translate (Polygon *poly,
                   gdouble  tx,
                   gdouble  ty)
{
  guint i;

  for (i = 0; i < poly->npts; i++)
    {
      poly->pts[i].x += tx;
      poly->pts[i].y += ty;
    }
}

static void
polygon_scale (Polygon *poly,
               gdouble  poly_scale)
{
  guint i;

  for (i = 0; i < poly->npts; i++)
    {
      poly->pts[i].x *= poly_scale;
      poly->pts[i].y *= poly_scale;
    }
}

static gboolean
polygon_extents (Polygon *poly,
                 gdouble *min_x,
                 gdouble *min_y,
                 gdouble *max_x,
                 gdouble *max_y)
{
  guint i;

  if (!poly->npts)
    return FALSE;

  *min_x = *max_x = poly->pts[0].x;
  *min_y = *max_y = poly->pts[0].y;

  for (i = 1; i < poly->npts; i++)
    {
      *min_x = MIN (*min_x, poly->pts[i].x);
      *max_x = MAX (*max_x, poly->pts[i].x);
      *min_y = MIN (*min_y, poly->pts[i].y);
      *max_y = MAX (*max_y, poly->pts[i].y);
    }

  return TRUE;
}

static void
polygon_reset (Polygon *poly)
{
  poly->npts = 0;
}
