/* Warp  --- image filter plug-in for GIMP
 * Copyright (C) 1997 John P. Beale
 * Much of the 'warp' is from the Displace plug-in: 1996 Stephen Robert Norris
 * Much of the 'displace' code taken in turn  from the pinch plug-in
 *   which is by 1996 Federico Mena Quintero
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
 *
 * You can contact me (the warp author) at beale@best.com
 * Please send me any patches or enhancements to this code.
 * You can contact the original GIMP authors at gimp@xcf.berkeley.edu
 *
 * --------------------------------------------------------------------
 * Warp Program structure: after running the user interface and setting the
 * parameters, warp generates a brand-new image (later to be deleted
 * before the user ever sees it) which contains two grayscale layers,
 * representing the X and Y gradients of the "control" image. For this
 * purpose, all channels of the control image are summed for a scalar
 * value at each pixel coordinate for the gradient operation.
 *
 * The X,Y components of the calculated gradient are then used to
 * displace pixels from the source image into the destination
 * image. The displacement vector is rotated a user-specified amount
 * first. This displacement operation happens iteratively, generating
 * a new displaced image from each prior image.
 * -------------------------------------------------------------------
 *
 * Revision History:
 * Version 0.37  12/19/98 Fixed Tooltips and freeing memory
 * Version 0.36  11/9/97  Changed XY vector layers  back to own image
 *               fixed 'undo' problem (hopefully)
 *
 * Version 0.35  11/3/97  Added vector-map, mag-map, grad-map to
 *               diff vector instead of separate operation
 *               further futzing with drawable updates
 *               starting adding tooltips
 *
 * Version 0.34  10/30/97   'Fixed' drawable update problem
 *               Added 16-bit resolution to differential map
 *               Added substep increments for finer control
 *
 * Version 0.33  10/26/97   Added 'angle increment' to user interface
 *
 * Version 0.32  10/25/97   Added magnitude control map (secondary control)
 *               Changed undo behavior to be one undo-step per warp call.
 *
 * Version 0.31  10/25/97   Fixed src/dest pixregions so program works
 *               with multiple-layer images. Still don't know
 *               exactly what I did to fix it :-/  Also, added 'color' option
 *               for border pixels to use the current selected foreground color.
 *
 * Version 0.3   10/20/97  Initial release for Gimp 0.99.xx
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Some useful macros */

#define PLUG_IN_PROC    "plug-in-warp"
#define PLUG_IN_BINARY  "warp"
#define PLUG_IN_ROLE    "gimp-warp"
#define ENTRY_WIDTH     75
#define MIN_ARGS         6  /* minimum number of arguments required */

enum
{
  WRAP,
  SMEAR,
  BLACK,
  COLOR
};

typedef struct
{
  gdouble amount;
  gint    warp_map;
  gint    iter;
  gdouble dither;
  gdouble angle;
  gint    wrap_type;
  gint    mag_map;
  gint    mag_use;
  gint    substeps;
  gint    grad_map;
  gdouble grad_scale;
  gint    vector_map;
  gdouble vector_scale;
  gdouble vector_angle;
} WarpVals;


/*
 * Function prototypes.
 */

static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static void      blur16           (gint32        drawable_id);

static void      diff             (gint32        drawable_id,
                                   gint32       *xl_id,
                                   gint32       *yl_id);

static void      diff_prepare_row (GeglBuffer   *buffer,
                                   const Babl   *format,
                                   guchar       *data,
                                   gint          x,
                                   gint          y,
                                   gint          w);

static void      warp_one         (gint32        draw_id,
                                   gint32        new_id,
                                   gint32        map_x_id,
                                   gint32        map_y_id,
                                   gint32        mag_draw_id,
                                   gboolean      first_time,
                                   gint          step);

static void      warp        (gint32        drawable_id);

static gboolean  warp_dialog (gint32        drawable_id);
static void      warp_pixel  (GeglBuffer   *buffer,
                              const Babl   *format,
                              gint          width,
                              gint          height,
                              gint          x1,
                              gint          y1,
                              gint          x2,
                              gint          y2,
                              gint          x,
                              gint          y,
                              guchar       *pixel);

static gboolean  warp_map_constrain       (gint32     image_id,
                                           gint32     drawable_id,
                                           gpointer   data);
static gdouble   warp_map_mag_give_value  (guchar    *pt,
                                           gint       alpha,
                                           gint       bytes);

/* -------------------------------------------------------------------------- */
/*   Variables global over entire plug-in scope                               */
/* -------------------------------------------------------------------------- */

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static WarpVals dvals =
{
  10.0,   /* amount       */
  -1,     /* warp_map     */
  5,      /* iterations   */
  0.0,    /* dither       */
  90.0,   /* angle        */
  WRAP,   /* wrap_type    */
  -1,     /* mag_map      */
  FALSE,  /* mag_use      */
  1,      /* substeps     */
  -1,     /* grad_map     */
  0.0,    /* grad_scale   */
  -1,     /* vector_map   */
  0.0,    /* vector_scale */
  0.0     /* vector_angle */
};

/* -------------------------------------------------------------------------- */

static gint         progress = 0;              /* progress indicator bar      */
static GimpRunMode  run_mode;                  /* interactive, non-, etc.     */
static guchar       color_pixel[4] = {0, 0, 0, 255};  /* current fg color     */

/* -------------------------------------------------------------------------- */

/***** Functions *****/

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",        "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Input drawable" },
    { GIMP_PDB_FLOAT,    "amount",       "Pixel displacement multiplier" },
    { GIMP_PDB_DRAWABLE, "warp-map",     "Displacement control map" },
    { GIMP_PDB_INT32,    "iter",         "Iteration count (last required argument)" },
    { GIMP_PDB_FLOAT,    "dither",       "Random dither amount (first optional argument)" },
    { GIMP_PDB_FLOAT,    "angle",        "Angle of gradient vector rotation" },
    { GIMP_PDB_INT32,    "wrap-type",    "Edge behavior: { WRAP (0), SMEAR (1), BLACK (2), COLOR (3) }" },
    { GIMP_PDB_DRAWABLE, "mag-map",      "Magnitude control map" },
    { GIMP_PDB_INT32,    "mag-use",      "Use magnitude map: { FALSE (0), TRUE (1) }" },
    { GIMP_PDB_INT32,    "substeps",     "Substeps between image updates" },
    { GIMP_PDB_INT32,    "grad-map",     "Gradient control map" },
    { GIMP_PDB_FLOAT,    "grad-scale",   "Scaling factor for gradient map (0=don't use)" },
    { GIMP_PDB_INT32,    "vector-map",   "Fixed vector control map" },
    { GIMP_PDB_FLOAT,    "vector-scale", "Scaling factor for fixed vector map (0=don't use)" },
    { GIMP_PDB_FLOAT,    "vector-angle", "Angle for fixed vector map" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Twist or smear image in many different ways"),
                          "Smears an image along vector paths calculated as "
                          "the gradient of a separate control matrix. The "
                          "effect can look like brushstrokes of acrylic or "
                          "watercolor paint, in some cases.",
                          "John P. Beale",
                          "John P. Beale",
                          "1997",
                          N_("_Warp..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Map");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            drawable_id;
  GimpRGB           color;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  /* get currently selected foreground pixel color */
  gimp_context_get_foreground (&color);
  gimp_rgb_get_uchar (&color,
                      &color_pixel[0],
                      &color_pixel[1],
                      &color_pixel[2]);

  run_mode    = param[0].data.d_int32;
  drawable_id = param[2].data.d_drawable;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &dvals);

      /*  First acquire information with a dialog  */
      if (! warp_dialog (drawable_id))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure minimum args
       *  (mode, image, draw, amount, warp_map, iter) are there
       */
      if (nparams < MIN_ARGS)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          gint  pcnt = MIN_ARGS;

          dvals.amount   = param[3].data.d_float;
          dvals.warp_map = param[4].data.d_int32;
          dvals.iter     = param[5].data.d_int32;

          if (nparams > pcnt++) dvals.dither       = param[6].data.d_float;
          if (nparams > pcnt++) dvals.angle        = param[7].data.d_float;
          if (nparams > pcnt++) dvals.wrap_type    = param[8].data.d_int32;
          if (nparams > pcnt++) dvals.mag_map      = param[9].data.d_int32;
          if (nparams > pcnt++) dvals.mag_use      = param[10].data.d_int32;
          if (nparams > pcnt++) dvals.substeps     = param[11].data.d_int32;
          if (nparams > pcnt++) dvals.grad_map     = param[12].data.d_int32;
          if (nparams > pcnt++) dvals.grad_scale   = param[13].data.d_float;
          if (nparams > pcnt++) dvals.vector_map   = param[14].data.d_int32;
          if (nparams > pcnt++) dvals.vector_scale = param[15].data.d_float;
          if (nparams > pcnt++) dvals.vector_angle = param[16].data.d_float;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &dvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  run the warp effect  */
      warp (drawable_id);

      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &dvals, sizeof (WarpVals));
    }

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  values[0].data.d_status = status;
}

static gboolean
warp_dialog (gint32 drawable_id)
{
  GtkWidget    *dlg;
  GtkWidget    *vbox;
  GtkWidget    *label;
  GtkWidget    *toggle;
  GtkWidget    *toggle_hbox;
  GtkWidget    *frame;
  GtkWidget    *table;
  GtkWidget    *spinbutton;
  GtkObject    *adj;
  GtkWidget    *combo;
  GtkSizeGroup *label_group;
  GtkSizeGroup *spin_group;
  GSList       *group = NULL;
  gboolean      run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dlg = gimp_dialog_new (_("Warp"), PLUG_IN_ROLE,
                         NULL, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,

                         NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame = gimp_frame_new (_("Basic Options"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 12);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  spin_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  label_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /*  amount, iter */
  spinbutton = gimp_spin_button_new (&adj, dvals.amount,
                                     -1000, 1000, /* ??? */
                                     1, 10, 0, 1, 2);
  gtk_size_group_add_widget (spin_group, spinbutton);
  g_object_unref (spin_group);

  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                     _("Step size:"), 0.0, 0.5,
                                     spinbutton, 1, FALSE);
  gtk_size_group_add_widget (label_group, label);
  g_object_unref (label_group);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &dvals.amount);

  spinbutton = gimp_spin_button_new (&adj, dvals.iter,
                                     1, 100, 1, 5, 0, 1, 0);
  gtk_size_group_add_widget (spin_group, spinbutton);

  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                                     _("Iterations:"), 0.0, 0.5,
                                     spinbutton, 1, FALSE);
  gtk_size_group_add_widget (label_group, label);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &dvals.iter);

  /*  Displacement map menu  */
  label = gtk_label_new (_("Displacement map:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_yalign (GTK_LABEL (label), 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  combo = gimp_drawable_combo_box_new (warp_map_constrain,
                                       GINT_TO_POINTER (drawable_id));
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), dvals.warp_map,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &dvals.warp_map);

  gtk_table_attach (GTK_TABLE (table), combo, 2, 3, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  /* ======================================================================= */

  /*  Displacement Type  */
  label = gtk_label_new (_("On edges:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  toggle_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_table_attach (GTK_TABLE (table), toggle_hbox, 1, 3, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (toggle_hbox);

  toggle = gtk_radio_button_new_with_label (group, _("Wrap"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_object_set_data (G_OBJECT (toggle), "gimp-item-data",
                     GINT_TO_POINTER (WRAP));

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_radio_button_update),
                    &dvals.wrap_type);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                dvals.wrap_type == WRAP);

  toggle = gtk_radio_button_new_with_label (group, _("Smear"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_object_set_data (G_OBJECT (toggle), "gimp-item-data",
                     GINT_TO_POINTER (SMEAR));

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_radio_button_update),
                    &dvals.wrap_type);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                dvals.wrap_type == SMEAR);

  toggle = gtk_radio_button_new_with_label (group, _("Black"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_object_set_data (G_OBJECT (toggle), "gimp-item-data",
                     GINT_TO_POINTER (BLACK));

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_radio_button_update),
                    &dvals.wrap_type);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                dvals.wrap_type == BLACK);

  toggle = gtk_radio_button_new_with_label (group, _("Foreground color"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_object_set_data (G_OBJECT (toggle), "gimp-item-data",
                     GINT_TO_POINTER (COLOR));

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_radio_button_update),
                    &dvals.wrap_type);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                dvals.wrap_type == COLOR);



  /* -------------------------------------------------------------------- */
  /* ---------    The secondary table         --------------------------  */

  frame = gimp_frame_new (_("Advanced Options"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 12);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  spinbutton = gimp_spin_button_new (&adj, dvals.dither,
                                     0, 100, 1, 10, 0, 1, 2);
  gtk_size_group_add_widget (spin_group, spinbutton);

  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                     _("Dither size:"), 0.0, 0.5,
                                     spinbutton, 1, FALSE);
  gtk_size_group_add_widget (label_group, label);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &dvals.dither);

  spinbutton = gimp_spin_button_new (&adj, dvals.angle,
                                     0, 360, 1, 15, 0, 1, 1);
  gtk_size_group_add_widget (spin_group, spinbutton);

  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                                     _("Rotation angle:"), 0.0, 0.5,
                                     spinbutton, 1, FALSE);
  gtk_size_group_add_widget (label_group, label);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &dvals.angle);

  spinbutton = gimp_spin_button_new (&adj, dvals.substeps,
                                     1, 100, 1, 5, 0, 1, 0);
  gtk_size_group_add_widget (spin_group, spinbutton);

  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                                     _("Substeps:"), 0.0, 0.5,
                                     spinbutton, 1, FALSE);
  gtk_size_group_add_widget (label_group, label);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &dvals.substeps);

  /*  Magnitude map menu  */
  label = gtk_label_new (_("Magnitude map:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_yalign (GTK_LABEL (label), 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  combo = gimp_drawable_combo_box_new (warp_map_constrain,
                                       GINT_TO_POINTER (drawable_id));
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), dvals.mag_map,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &dvals.mag_map);

  gtk_table_attach (GTK_TABLE (table), combo, 2, 3, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  /*  Magnitude Usage  */
  toggle_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_set_border_width (GTK_CONTAINER (toggle_hbox), 1);
  gtk_table_attach (GTK_TABLE (table), toggle_hbox, 2, 3, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (toggle_hbox);

  toggle = gtk_check_button_new_with_label (_("Use magnitude map"));
  gtk_box_pack_start (GTK_BOX (toggle_hbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), dvals.mag_use);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &dvals.mag_use);


  /* -------------------------------------------------------------------- */
  /* ---------    The "other" table         --------------------------  */

  frame = gimp_frame_new (_("More Advanced Options"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 12);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  spinbutton = gimp_spin_button_new (&adj, dvals.grad_scale,
                                     -1000, 1000, /* ??? */
                                     0.01, 0.1, 0, 1, 3);
  gtk_size_group_add_widget (spin_group, spinbutton);

  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                     _("Gradient scale:"), 0.0, 0.5,
                                     spinbutton, 1, FALSE);
  gtk_size_group_add_widget (label_group, label);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &dvals.grad_scale);

  /* ---------  Gradient map menu ----------------  */

  combo = gimp_drawable_combo_box_new (warp_map_constrain,
                                       GINT_TO_POINTER (drawable_id));
  gtk_table_attach (GTK_TABLE (table), combo, 2, 3, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), dvals.grad_map,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &dvals.grad_map);

  gimp_help_set_help_data (combo, _("Gradient map selection menu"), NULL);

  /* ---------------------------------------------- */

  spinbutton = gimp_spin_button_new (&adj, dvals.vector_scale,
                                     -1000, 1000, /* ??? */
                                     0.01, 0.1, 0, 1, 3);
  gtk_size_group_add_widget (spin_group, spinbutton);

  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                                     _("Vector mag:"), 0.0, 0.5,
                                     spinbutton, 1, FALSE);
  gtk_size_group_add_widget (label_group, label);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &dvals.vector_scale);

  /* -------------------------------------------------------- */

  spinbutton = gimp_spin_button_new (&adj, dvals.vector_angle,
                                     0, 360, 1, 15, 0, 1, 1);
  gtk_size_group_add_widget (spin_group, spinbutton);

  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                                     _("Angle:"), 0.0, 0.5,
                                     spinbutton, 1, FALSE);
  gtk_size_group_add_widget (label_group, label);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &dvals.vector_angle);

  /* ---------  Vector map menu ----------------  */
  combo = gimp_drawable_combo_box_new (warp_map_constrain,
                                       GINT_TO_POINTER (drawable_id));
  gtk_table_attach (GTK_TABLE (table), combo, 2, 3, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), dvals.vector_map,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &dvals.vector_map);

  gimp_help_set_help_data (combo,
                           _("Fixed-direction-vector map selection menu"),
                           NULL);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}

/* ---------------------------------------------------------------------- */

static const Babl *
get_u8_format (gint32 drawable_id)
{
  if (gimp_drawable_is_rgb (drawable_id))
    {
      if (gimp_drawable_has_alpha (drawable_id))
        return babl_format ("R'G'B'A u8");
      else
        return babl_format ("R'G'B' u8");
    }
  else
    {
      if (gimp_drawable_has_alpha (drawable_id))
        return babl_format ("Y'A u8");
      else
        return babl_format ("Y' u8");
    }
}

static void
blur16 (gint32 drawable_id)
{
  /*  blur a 2-or-more byte-per-pixel drawable,
   *  1st 2 bytes interpreted as a 16-bit height field.
   */
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *format;
  gint width, height;
  gint src_bytes;
  gint dest_bytes;
  gint dest_bytes_inc;
  gint offb, off1;

  guchar *dest, *d;  /* pointers to rows of X and Y diff. data */
  guchar *prev_row, *pr;
  guchar *cur_row, *cr;
  guchar *next_row, *nr;
  guchar *tmp;
  gint row, col;  /* relating to indexing into pixel row arrays */
  gint x1, y1, x2, y2;
  gdouble pval;          /* average pixel value of pixel & neighbors */

  /* --------------------------------------- */

  if (! gimp_drawable_mask_intersect (drawable_id,
                                      &x1, &y1, &width, &height))
    return;

  x2 = x1 + width;
  y2 = y1 + height;

  width  = gimp_drawable_width  (drawable_id);     /* size of input drawable*/
  height = gimp_drawable_height (drawable_id);

  format = get_u8_format (drawable_id);

  /* bytes per pixel in SOURCE drawable, must be 2 or more  */
  src_bytes = babl_format_get_bytes_per_pixel (format);

  dest_bytes = src_bytes;  /* bytes per pixel in SOURCE drawable, >= 2  */
  dest_bytes_inc = dest_bytes - 2;  /* this is most likely zero, but I guess it's more conservative... */

  /*  allocate row buffers for source & dest. data  */

  prev_row = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  cur_row  = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  next_row = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  dest     = g_new (guchar, (x2 - x1) * src_bytes);

  /* initialize the pixel regions (read from source, write into dest)  */
  src_buffer   = gimp_drawable_get_buffer (drawable_id);
  dest_buffer  = gimp_drawable_get_shadow_buffer (drawable_id);

  pr = prev_row + src_bytes;   /* row arrays are prepared for indexing to -1 (!) */
  cr = cur_row  + src_bytes;
  nr = next_row + src_bytes;

  diff_prepare_row (src_buffer, format, pr, x1, y1, (x2 - x1));
  diff_prepare_row (src_buffer, format, cr, x1, y1+1, (x2 - x1));

  /*  loop through the rows, applying the smoothing function  */
  for (row = y1; row < y2; row++)
    {
      /*  prepare the next row  */
      diff_prepare_row (src_buffer, format, nr, x1, row + 1, (x2 - x1));

      d = dest;
      for (col = 0; col < (x2 - x1); col++) /* over columns of pixels */
        {
          offb = col*src_bytes;    /* base of byte pointer offset */
          off1 = offb+1;                 /* offset into row arrays */

          pval = (256.0 * pr[offb - src_bytes] + pr[off1 - src_bytes] +
                  256.0 * pr[offb] + pr[off1] +
                  256.0 * pr[offb + src_bytes] + pr[off1 + src_bytes] +
                  256.0 * cr[offb - src_bytes] + cr[off1 - src_bytes] +
                  256.0 * cr[offb]  + cr[off1] +
                  256.0 * cr[offb + src_bytes] + cr[off1 + src_bytes] +
                  256.0 * nr[offb - src_bytes] + nr[off1 - src_bytes] +
                  256.0 * nr[offb] + nr[off1] +
                  256.0 * nr[offb + src_bytes]) + nr[off1 + src_bytes];

          pval /= 9.0;  /* take the average */
          *d++ = (guchar) (((gint) pval) >> 8);   /* high-order byte */
          *d++ = (guchar) (((gint) pval) % 256);  /* low-order byte  */
          d += dest_bytes_inc;       /* move data pointer on to next destination pixel */
       }

      /*  store the dest  */
      gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (x1, row, (x2 - x1), 1), 0,
                       format, dest,
                       GEGL_AUTO_ROWSTRIDE);

      /*  shuffle the row pointers  */
      tmp = pr;
      pr = cr;
      cr = nr;
      nr = tmp;

      if ((row % 8) == 0)
        gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  gimp_progress_update (1.0);

  gimp_drawable_merge_shadow (drawable_id, TRUE);
  gimp_drawable_update (drawable_id, x1, y1, (x2 - x1), (y2 - y1));

  g_free (prev_row);  /* row buffers allocated at top of fn. */
  g_free (cur_row);
  g_free (next_row);
  g_free (dest);

}


/* ====================================================================== */
/* Get one row of pixels from the PixelRegion and put them in 'data'      */

static void
diff_prepare_row (GeglBuffer *buffer,
                  const Babl *format,
                  guchar     *data,
                  gint        x,
                  gint        y,
                  gint        w)
{
  gint bpp = babl_format_get_bytes_per_pixel (format);
  gint b;

  /* y = CLAMP (y, 0, pixel_rgn->h - 1); FIXME? */

  gegl_buffer_get (buffer, GEGL_RECTANGLE (x, y, w, 1), 1.0,
                   format, data,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  /*  Fill in edge pixels  */
  for (b = 0; b < bpp; b++)
    {
      data[b - (gint) bpp] = data[b];
      data[w * bpp + b] = data[(w - 1) * bpp + b];
    }
}

/* -------------------------------------------------------------------------- */
/*  'diff' combines the input drawables to prepare the two                    */
/*  16-bit (X,Y) vector displacement maps                                     */
/* -------------------------------------------------------------------------- */

static void
diff (gint32  drawable_id,
      gint32 *xl_id,
      gint32 *yl_id)
{
  gint32      draw_xd_id;
  gint32      draw_yd_id; /* vector disp. drawables */
  gint32      mdraw_id;
  gint32      vdraw_id;
  gint32      gdraw_id;
  gint32      image_id;              /* image holding X and Y diff. arrays */
  gint32      new_image_id;          /* image holding X and Y diff. layers */
  gint32      layer_active;          /* currently active layer */
  gint32      xlayer_id, ylayer_id;  /* individual X and Y layer ID numbers */
  GeglBuffer *src_buffer;
  GeglBuffer *destx_buffer;
  const Babl *destx_format;
  GeglBuffer *desty_buffer;
  const Babl *desty_format;
  GeglBuffer *vec_buffer;
  GeglBuffer *mag_buffer = NULL;
  GeglBuffer *grad_buffer;
  gint        width, height;
  const Babl *src_format;
  gint        src_bytes;
  const Babl *mformat = NULL;
  gint        mbytes  = 0;
  const Babl *vformat = NULL;
  gint        vbytes  = 0;
  const Babl *gformat = NULL;
  gint        gbytes  = 0;   /* bytes-per-pixel of various source drawables */
  const Babl *dest_format;
  gint        dest_bytes;
  gint        dest_bytes_inc;
  gint        do_gradmap = FALSE; /* whether to add in gradient of gradmap to final diff. map */
  gint        do_vecmap = FALSE; /* whether to add in a fixed vector scaled by the vector map */
  gint        do_magmap = FALSE; /* whether to multiply result by the magnitude map */

  guchar *destx, *dx, *desty, *dy;  /* pointers to rows of X and Y diff. data */
  guchar *tmp;
  guchar *prev_row, *pr;
  guchar *cur_row, *cr;
  guchar *next_row, *nr;
  guchar *prev_row_g, *prg = NULL;  /* pointers to gradient map data */
  guchar *cur_row_g, *crg = NULL;
  guchar *next_row_g, *nrg = NULL;
  guchar *cur_row_v, *crv = NULL;   /* pointers to vector map data */
  guchar *cur_row_m, *crm = NULL;   /* pointers to magnitude map data */
  gint row, col, offb, off, bytes;  /* relating to indexing into pixel row arrays */
  gint x1, y1, x2, y2;
  gint dvalx, dvaly;                /* differential value at particular pixel */
  gdouble tx, ty;                   /* temporary x,y differential value increments from gradmap, etc. */
  gdouble rdx, rdy;                 /* x,y differential values: real #s */
  gdouble rscalefac;                /* scaling factor for x,y differential of 'curl' map */
  gdouble gscalefac;                /* scaling factor for x,y differential of 'gradient' map */
  gdouble r, theta, dtheta;         /* rectangular<-> spherical coordinate transform for vector rotation */
  gdouble scale_vec_x, scale_vec_y; /* fixed vector X,Y component scaling factors */

  /* ----------------------------------------------------------------------- */

  if (dvals.grad_scale != 0.0)
    do_gradmap = TRUE;              /* add in gradient of gradmap if scale != 0.000 */

  if (dvals.vector_scale != 0.0)    /* add in gradient of vectormap if scale != 0.000 */
    do_vecmap = TRUE;

  do_magmap = (dvals.mag_use == TRUE); /* multiply by magnitude map if so requested */

  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  if (! gimp_drawable_mask_intersect (drawable_id,
                                      &x1, &y1, &width, &height))
    return;

  x2 = x1 + width;
  y2 = y1 + height;

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width  = gimp_drawable_width  (drawable_id);
  height = gimp_drawable_height (drawable_id);

  src_format = get_u8_format (drawable_id);
  src_bytes  = babl_format_get_bytes_per_pixel (src_format);

  /* -- Add two layers: X and Y Displacement vectors -- */
  /* -- I'm using a RGB  drawable and using the first two bytes for a
        16-bit pixel value. This is either clever, or a kluge,
        depending on your point of view.  */

  image_id     = gimp_item_get_image (drawable_id);
  layer_active = gimp_image_get_active_layer (image_id);

  /* create new image for X,Y diff */
  new_image_id = gimp_image_new (width, height, GIMP_RGB);

  xlayer_id = gimp_layer_new (new_image_id, "Warp_X_Vectors",
                              width, height,
                              GIMP_RGB_IMAGE,
                              100.0,
                              gimp_image_get_default_new_layer_mode (new_image_id));

  ylayer_id = gimp_layer_new (new_image_id, "Warp_Y_Vectors",
                              width, height,
                              GIMP_RGB_IMAGE,
                              100.0,
                              gimp_image_get_default_new_layer_mode (new_image_id));

  draw_yd_id = ylayer_id;
  draw_xd_id = xlayer_id;

  gimp_image_insert_layer (new_image_id, xlayer_id, -1, 1);
  gimp_image_insert_layer (new_image_id, ylayer_id, -1, 1);
  gimp_drawable_fill (xlayer_id, GIMP_FILL_BACKGROUND);
  gimp_drawable_fill (ylayer_id, GIMP_FILL_BACKGROUND);
  gimp_image_set_active_layer (image_id, layer_active);

  dest_format = get_u8_format (draw_xd_id);
  dest_bytes  = babl_format_get_bytes_per_pixel (dest_format);
  /* for a GRAYA drawable, I would expect this to be two bytes; any more would be excess */
  dest_bytes_inc = dest_bytes - 2;

  /*  allocate row buffers for source & dest. data  */

  prev_row = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  cur_row  = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  next_row = g_new (guchar, (x2 - x1 + 2) * src_bytes);

  prev_row_g = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  cur_row_g  = g_new (guchar, (x2 - x1 + 2) * src_bytes);
  next_row_g = g_new (guchar, (x2 - x1 + 2) * src_bytes);

  cur_row_v = g_new (guchar, (x2 - x1 + 2) * src_bytes);  /* vector map */
  cur_row_m = g_new (guchar, (x2 - x1 + 2) * src_bytes);  /* magnitude map */

  destx = g_new (guchar, (x2 - x1) * dest_bytes);
  desty = g_new (guchar, (x2 - x1) * dest_bytes);

  /*  initialize the source and destination pixel regions  */

  /* 'curl' vector-rotation input */
  src_buffer = gimp_drawable_get_buffer (drawable_id);

  /* destination: X diff output */
  destx_buffer = gimp_drawable_get_buffer (draw_xd_id);
  destx_format = get_u8_format (draw_xd_id);

  /* Y diff output */
  desty_buffer = gimp_drawable_get_buffer (draw_yd_id);
  desty_format = get_u8_format (draw_yd_id);

  pr = prev_row + src_bytes;
  cr = cur_row  + src_bytes;
  nr = next_row + src_bytes;

  diff_prepare_row (src_buffer, src_format, pr, x1, y1, (x2 - x1));
  diff_prepare_row (src_buffer, src_format, cr, x1, y1+1, (x2 - x1));

 /* fixed-vector (x,y) component scale factors */
  scale_vec_x = (dvals.vector_scale *
                 cos ((90 - dvals.vector_angle) * G_PI / 180.0) * 256.0 / 10);
  scale_vec_y = (dvals.vector_scale *
                 sin ((90 - dvals.vector_angle) * G_PI / 180.0) * 256.0 / 10);

  if (do_vecmap)
    {
      vdraw_id = dvals.vector_map;

      /* bytes per pixel in SOURCE drawable */
      vformat = get_u8_format (vdraw_id);
      vbytes  = babl_format_get_bytes_per_pixel (vformat);

      /* fixed-vector scale-map */
      vec_buffer = gimp_drawable_get_buffer (vdraw_id);

      crv = cur_row_v + vbytes;
      diff_prepare_row (vec_buffer, vformat, crv, x1, y1, (x2 - x1));
    }

  if (do_gradmap)
    {
      gdraw_id = dvals.grad_map;

      gformat = get_u8_format (gdraw_id);
      gbytes  = babl_format_get_bytes_per_pixel (gformat);

      /* fixed-vector scale-map */
      grad_buffer = gimp_drawable_get_buffer (gdraw_id);

      prg = prev_row_g + gbytes;
      crg = cur_row_g + gbytes;
      nrg = next_row_g + gbytes;
      diff_prepare_row (grad_buffer, gformat, prg, x1, y1 - 1, (x2 - x1));
      diff_prepare_row (grad_buffer, gformat, crg, x1, y1, (x2 - x1));
    }

  if (do_magmap)
    {
      mdraw_id = dvals.mag_map;

      mformat = get_u8_format (mdraw_id);
      mbytes  = babl_format_get_bytes_per_pixel (mformat);

      /* fixed-vector scale-map */
      mag_buffer = gimp_drawable_get_buffer (mdraw_id);

      crm = cur_row_m + mbytes;
      diff_prepare_row (mag_buffer, mformat, crm, x1, y1, (x2 - x1));
    }

  dtheta = dvals.angle * G_PI / 180.0;
  /* note that '3' is rather arbitrary here. */
  rscalefac = 256.0 / (3 * src_bytes);
  /* scale factor for gradient map components */
  gscalefac = dvals.grad_scale * 256.0 / (3 * gbytes);

  /*  loop through the rows, applying the differential convolution  */
  for (row = y1; row < y2; row++)
    {
      /*  prepare the next row  */
      diff_prepare_row (src_buffer, src_format, nr, x1, row + 1, (x2 - x1));

      if (do_magmap)
        diff_prepare_row (mag_buffer, mformat, crm, x1, row + 1, (x2 - x1));
      if (do_vecmap)
        diff_prepare_row (vec_buffer, vformat, crv, x1, row + 1, (x2 - x1));
      if (do_gradmap)
        diff_prepare_row (grad_buffer, gformat, crg, x1, row + 1, (x2 - x1));

      dx = destx;
      dy = desty;

      for (col = 0; col < (x2 - x1); col++) /* over columns of pixels */
        {
          rdx = 0.0;
          rdy = 0.0;
          ty = 0.0;
          tx = 0.0;

          offb = col * src_bytes;    /* base of byte pointer offset */
          for (bytes=0; bytes < src_bytes; bytes++) /* add all channels together */
            {
              off = offb+bytes;                 /* offset into row arrays */
              rdx += ((gint) -pr[off - src_bytes]   + (gint) pr[off + src_bytes] +
                      (gint) -2*cr[off - src_bytes] + (gint) 2*cr[off + src_bytes] +
                      (gint) -nr[off - src_bytes]   + (gint) nr[off + src_bytes]);

              rdy += ((gint) -pr[off - src_bytes] - (gint)2*pr[off] - (gint) pr[off + src_bytes] +
                      (gint) nr[off - src_bytes] + (gint)2*nr[off] + (gint) nr[off + src_bytes]);
            }

          rdx *= rscalefac;   /* take average, then reduce. Assume max. rdx now 65535 */
          rdy *= rscalefac;   /* take average, then reduce */

          theta = atan2(rdy,rdx);          /* convert to polar, then back to rectang. coords */
          r = sqrt(rdy*rdy + rdx*rdx);
          theta += dtheta;              /* rotate gradient vector by this angle (radians) */
          rdx = r * cos(theta);
          rdy = r * sin(theta);

          if (do_gradmap)
            {
              offb = col*gbytes;     /* base of byte pointer offset into pixel values (R,G,B,Alpha, etc.) */
              for (bytes=0; bytes < src_bytes; bytes++) /* add all channels together */
                {
                  off = offb+bytes;                 /* offset into row arrays */
                  tx += ((gint) -prg[off - gbytes]   + (gint) prg[off + gbytes] +
                         (gint) -2*crg[off - gbytes] + (gint) 2*crg[off + gbytes] +
                         (gint) -nrg[off - gbytes]   + (gint) nrg[off + gbytes]);

                  ty += ((gint) -prg[off - gbytes] - (gint)2*prg[off] - (gint) prg[off + gbytes] +
                         (gint) nrg[off - gbytes] + (gint)2*nrg[off] + (gint) nrg[off + gbytes]);
                }
              tx *= gscalefac;
              ty *= gscalefac;

              rdx += tx;         /* add gradient component in to the other one */
              rdy += ty;

            } /* if (do_gradmap) */

          if (do_vecmap)
            {  /* add in fixed vector scaled by  vec. map data */
              tx = (gdouble) crv[col*vbytes];       /* use first byte only */
              rdx += scale_vec_x * tx;
              rdy += scale_vec_y * tx;
            } /* if (do_vecmap) */

          if (do_magmap)
            {  /* multiply result by mag. map data */
              tx = (gdouble) crm[col*mbytes];
              rdx = (rdx * tx)/(255.0);
              rdy = (rdy * tx)/(255.0);
            } /* if do_magmap */

          dvalx = rdx + (2<<14);         /* take zero point to be 2^15, since this is two bytes */
          dvaly = rdy + (2<<14);

          if (dvalx < 0)
            dvalx = 0;

          if (dvalx > 65535)
            dvalx = 65535;

          *dx++ = (guchar) (dvalx >> 8);    /* store high order byte in value channel */
          *dx++ = (guchar) (dvalx % 256);   /* store low order byte in alpha channel */
          dx += dest_bytes_inc;       /* move data pointer on to next destination pixel */

          if (dvaly < 0)
            dvaly = 0;

          if (dvaly > 65535)
            dvaly = 65535;

          *dy++ = (guchar) (dvaly >> 8);
          *dy++ = (guchar) (dvaly % 256);
          dy += dest_bytes_inc;

        } /* ------------------------------- for (col...) ----------------  */

      /*  store the dest  */
      gegl_buffer_set (destx_buffer,
                       GEGL_RECTANGLE (x1, row, (x2 - x1), 1), 0,
                       destx_format, destx,
                       GEGL_AUTO_ROWSTRIDE);

      gegl_buffer_set (desty_buffer,
                       GEGL_RECTANGLE (x1, row, (x2 - x1), 1), 0,
                       desty_format, desty,
                       GEGL_AUTO_ROWSTRIDE);

      /*  swap around the pointers to row buffers  */
      tmp = pr;
      pr = cr;
      cr = nr;
      nr = tmp;

      if (do_gradmap)
        {
          tmp = prg;
          prg = crg;
          crg = nrg;
          nrg = tmp;
        }

      if ((row % 8) == 0)
        gimp_progress_update ((gdouble) row / (gdouble) (y2 - y1));

    } /* for (row..) */

  gimp_progress_update (1.0);

  g_object_unref (src_buffer);
  g_object_unref (destx_buffer);
  g_object_unref (desty_buffer);

  gimp_drawable_update (draw_xd_id, x1, y1, (x2 - x1), (y2 - y1));
  gimp_drawable_update (draw_yd_id, x1, y1, (x2 - x1), (y2 - y1));

  gimp_displays_flush ();  /* make sure layer is visible */

  gimp_progress_init (_("Smoothing X gradient"));
  blur16 (draw_xd_id);

  gimp_progress_init (_("Smoothing Y gradient"));
  blur16 (draw_yd_id);

  g_free (prev_row);  /* row buffers allocated at top of fn. */
  g_free (cur_row);
  g_free (next_row);
  g_free (prev_row_g);  /* row buffers allocated at top of fn. */
  g_free (cur_row_g);
  g_free (next_row_g);
  g_free (cur_row_v);
  g_free (cur_row_m);

  g_free (destx);
  g_free (desty);

  *xl_id = xlayer_id;  /* pass back the X and Y layer ID numbers */
  *yl_id = ylayer_id;
}

/* -------------------------------------------------------------------------- */
/*            The Warp displacement is done here.                             */
/* -------------------------------------------------------------------------- */

static void
warp (gint32 orig_draw_id)
{
  gint32    disp_map_id;    /* Displacement map, ie, control array */
  gint32    mag_draw_id;    /* Magnitude multiplier factor map */
  gint32    map_x_id = -1;
  gint32    map_y_id = -1;
  gboolean  first_time = TRUE;
  gint      width;
  gint      height;
  gint      x1, y1, x2, y2;
  gint32    image_ID;

  /* index var. over all "warp" Displacement iterations */
  gint          warp_iter;

  disp_map_id = dvals.warp_map;
  mag_draw_id = dvals.mag_map;

  /* calculate new X,Y Displacement image maps */

  gimp_progress_init (_("Finding XY gradient"));

  /* Get selection area */
  if (! gimp_drawable_mask_intersect (orig_draw_id,
                                      &x1, &y1, &width, &height))
    return;

  x2 = x1 + width;
  y2 = y1 + height;

  width  = gimp_drawable_width  (orig_draw_id);
  height = gimp_drawable_height (orig_draw_id);

  /* generate x,y differential images (arrays) */
  diff (disp_map_id, &map_x_id, &map_y_id);

  for (warp_iter = 0; warp_iter < dvals.iter; warp_iter++)
    {
      gimp_progress_init_printf (_("Flow step %d"), warp_iter+1);
      progress = 0;

      warp_one (orig_draw_id, orig_draw_id,
                map_x_id, map_y_id, mag_draw_id,
                first_time, warp_iter);

      gimp_drawable_update (orig_draw_id,
                            x1, y1, (x2 - x1), (y2 - y1));

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      first_time = FALSE;
    }

  image_ID = gimp_item_get_image (map_x_id);

  gimp_image_delete (image_ID);
}

/* -------------------------------------------------------------------------- */

static void
warp_one (gint32   draw_id,
          gint32   new_id,
          gint32   map_x_id,
          gint32   map_y_id,
          gint32   mag_draw_id,
          gboolean first_time,
          gint     step)
{
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  GeglBuffer *map_x_buffer;
  GeglBuffer *map_y_buffer;
  GeglBuffer *mag_buffer = NULL;

  GeglBufferIterator *iter;

  gint        width;
  gint        height;

  const Babl *src_format;
  gint        src_bytes;
  const Babl *dest_format;
  gint        dest_bytes;

  guchar  pixel[4][4];
  gint    x1, y1, x2, y2;
  gint    x, y;
  gint    max_progress;

  gdouble needx, needy;
  gdouble xval=0;      /* initialize to quiet compiler grumbles */
  gdouble yval=0;      /* interpolated vector displacement */
  gdouble scalefac;        /* multiplier for vector displacement scaling */
  gdouble dscalefac;       /* multiplier for incremental displacement vectors */
  gint    xi, yi;
  gint    substep;         /* loop variable counting displacement vector substeps */

  guchar  values[4];
  guint32 ivalues[4];
  guchar  val;

  gint k;

  gdouble dx, dy;           /* X and Y Displacement, integer from GRAY map */

  const Babl *map_x_format;
  gint        map_x_bytes;
  const Babl *map_y_format;
  gint        map_y_bytes;
  const Babl *mag_format;
  gint        mag_bytes = 1;
  gboolean    mag_alpha = FALSE;

  GRand  *gr;

  gr = g_rand_new (); /* Seed Pseudo Random Number Generator */

  /* ================ Outer Loop calculation ================================ */

  /* Get selection area */

  if (! gimp_drawable_mask_intersect (draw_id,
                                      &x1, &y1, &width, &height))
    return;

  x2 = x1 + width;
  y2 = y1 + height;

  width  = gimp_drawable_width  (draw_id);
  height = gimp_drawable_height (draw_id);


  max_progress = (x2 - x1) * (y2 - y1);


  /*  --------- Register the (many) pixel regions ----------  */

  src_buffer = gimp_drawable_get_buffer (draw_id);

  src_format = get_u8_format (draw_id);
  src_bytes  = babl_format_get_bytes_per_pixel (src_format);

  iter = gegl_buffer_iterator_new (src_buffer,
                                   GEGL_RECTANGLE (x1, y1, (x2 - x1), (y2 - y1)),
                                   0, src_format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 5);


  dest_buffer = gimp_drawable_get_shadow_buffer (new_id);

  dest_format = get_u8_format (new_id);
  dest_bytes  = babl_format_get_bytes_per_pixel (dest_format);

  gegl_buffer_iterator_add (iter, dest_buffer,
                            GEGL_RECTANGLE (x1, y1, (x2 - x1), (y2 - y1)),
                            0, dest_format,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);


  map_x_buffer = gimp_drawable_get_buffer (map_x_id);

  map_x_format = get_u8_format (map_x_id);
  map_x_bytes  = babl_format_get_bytes_per_pixel (map_x_format);

  gegl_buffer_iterator_add (iter, map_x_buffer,
                            GEGL_RECTANGLE (x1, y1, (x2 - x1), (y2 - y1)),
                            0, map_x_format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);


  map_y_buffer = gimp_drawable_get_buffer (map_y_id);

  map_y_format = get_u8_format (map_y_id);
  map_y_bytes  = babl_format_get_bytes_per_pixel (map_y_format);

  gegl_buffer_iterator_add (iter, map_y_buffer,
                            GEGL_RECTANGLE (x1, y1, (x2 - x1), (y2 - y1)),
                            0, map_y_format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);


  if (dvals.mag_use)
    {
      mag_buffer = gimp_drawable_get_buffer (mag_draw_id);

      mag_format = get_u8_format (mag_draw_id);
      mag_bytes  = babl_format_get_bytes_per_pixel (mag_format);

      mag_alpha = gimp_drawable_has_alpha (mag_draw_id);

      gegl_buffer_iterator_add (iter, mag_buffer,
                                GEGL_RECTANGLE (x1, y1, (x2 - x1), (y2 - y1)),
                                0, mag_format,
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
    }

  /* substep displacement vector scale factor */
  dscalefac = dvals.amount / (256 * 127.5 * dvals.substeps);

  while (gegl_buffer_iterator_next (iter))
    {
      GeglRectangle  roi     = iter->items[1].roi;
      guchar        *srcrow  = iter->items[0].data;
      guchar        *destrow = iter->items[1].data;
      guchar        *mxrow   = iter->items[2].data;
      guchar        *myrow   = iter->items[3].data;
      guchar        *mmagrow = NULL;

      if (dvals.mag_use)
        mmagrow = iter->items[4].data;

      /* loop over destination pixels */
      for (y = roi.y; y < (roi.y + roi.height); y++)
        {
          guchar *dest = destrow;
          guchar *mx   = mxrow;
          guchar *my   = myrow;
          guchar *mmag = NULL;

          if (dvals.mag_use == TRUE)
            mmag = mmagrow;

          for (x = roi.x; x < (roi.x + roi.width); x++)
            {
              /* ----- Find displacement vector (amnt_x, amnt_y) ------------ */

              dx = dscalefac * ((256.0 * mx[0]) + mx[1] -32768);  /* 16-bit values */
              dy = dscalefac * ((256.0 * my[0]) + my[1] -32768);

              if (dvals.mag_use)
                {
                  scalefac = warp_map_mag_give_value (mmag,
                                                      mag_alpha,
                                                      mag_bytes) / 255.0;
                  dx *= scalefac;
                  dy *= scalefac;
                }

              if (dvals.dither != 0.0)
                {       /* random dither is +/- dvals.dither pixels */
                  dx += g_rand_double_range (gr, -dvals.dither, dvals.dither);
                  dy += g_rand_double_range (gr, -dvals.dither, dvals.dither);
                }

              if (dvals.substeps != 1)
                {   /* trace (substeps) iterations of displacement vector */
                  for (substep = 1; substep < dvals.substeps; substep++)
                    {
                      /* In this (substep) loop, (x,y) remain fixed. (dx,dy) vary each step. */
                      needx = x + dx;
                      needy = y + dy;

                      if (needx >= 0.0)
                        xi = (gint) needx;
                      else
                        xi = -((gint) -needx + 1);

                      if (needy >= 0.0)
                        yi = (gint) needy;
                      else
                        yi = -((gint) -needy + 1);

                      /* get 4 neighboring DX values from DiffX drawable for linear interpolation */
                      warp_pixel (map_x_buffer, map_x_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi, yi,
                                  pixel[0]);
                      warp_pixel (map_x_buffer, map_x_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi + 1, yi,
                                  pixel[1]);
                      warp_pixel (map_x_buffer, map_x_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi, yi + 1,
                                  pixel[2]);
                      warp_pixel (map_x_buffer, map_x_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi + 1, yi + 1,
                                  pixel[3]);

                      ivalues[0] = 256 * pixel[0][0] + pixel[0][1];
                      ivalues[1] = 256 * pixel[1][0] + pixel[1][1];
                      ivalues[2] = 256 * pixel[2][0] + pixel[2][1];
                      ivalues[3] = 256 * pixel[3][0] + pixel[3][1];

                      xval = gimp_bilinear_32 (needx, needy, ivalues);

                      /* get 4 neighboring DY values from DiffY drawable for linear interpolation */
                      warp_pixel (map_y_buffer, map_y_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi, yi,
                                  pixel[0]);
                      warp_pixel (map_y_buffer, map_y_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi + 1, yi,
                                  pixel[1]);
                      warp_pixel (map_y_buffer, map_y_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi, yi + 1,
                                  pixel[2]);
                      warp_pixel (map_y_buffer, map_y_format,
                                  width, height,
                                  x1, y1, x2, y2,
                                  xi + 1, yi + 1,
                                  pixel[3]);

                      ivalues[0] = 256 * pixel[0][0] + pixel[0][1];
                      ivalues[1] = 256 * pixel[1][0] + pixel[1][1];
                      ivalues[2] = 256 * pixel[2][0] + pixel[2][1];
                      ivalues[3] = 256 * pixel[3][0] + pixel[3][1];

                      yval = gimp_bilinear_32 (needx, needy, ivalues);

                      /* move displacement vector to this new value */
                      dx += dscalefac * (xval - 32768);
                      dy += dscalefac * (yval - 32768);

                    } /* for (substep) */
                } /* if (substeps != 0) */

              /* --------------------------------------------------------- */

              needx = x + dx;
              needy = y + dy;

              mx += map_x_bytes;         /* pointers into x,y displacement maps */
              my += map_y_bytes;

              if (dvals.mag_use == TRUE)
                mmag += mag_bytes;

              /* Calculations complete; now copy the proper pixel */

              if (needx >= 0.0)
                xi = (gint) needx;
              else
                xi = -((gint) -needx + 1);

              if (needy >= 0.0)
                yi = (gint) needy;
              else
                yi = -((gint) -needy + 1);

              /* get 4 neighboring pixel values from source drawable
               * for linear interpolation
               */
              warp_pixel (src_buffer, src_format,
                          width, height,
                          x1, y1, x2, y2,
                          xi, yi,
                          pixel[0]);
              warp_pixel (src_buffer, src_format,
                          width, height,
                          x1, y1, x2, y2,
                          xi + 1, yi,
                          pixel[1]);
              warp_pixel (src_buffer, src_format,
                          width, height,
                          x1, y1, x2, y2,
                          xi, yi + 1,
                          pixel[2]);
              warp_pixel (src_buffer, src_format,
                          width, height,
                          x1, y1, x2, y2,
                          xi + 1, yi + 1,
                          pixel[3]);

              for (k = 0; k < dest_bytes; k++)
                {
                  values[0] = pixel[0][k];
                  values[1] = pixel[1][k];
                  values[2] = pixel[2][k];
                  values[3] = pixel[3][k];

                  val = gimp_bilinear_8 (needx, needy, values);

                  *dest++ = val;
                }
            }

          /*      srcrow += src_rgn.rowstride; */
          srcrow  += src_bytes   * roi.width;
          destrow += dest_bytes  * roi.width;
          mxrow   += map_x_bytes * roi.width;
          myrow   += map_y_bytes * roi.width;

          if (dvals.mag_use == TRUE)
            mmagrow += mag_bytes * roi.width;
        }

      progress += (roi.width * roi.height);
      gimp_progress_update ((double) progress / (double) max_progress);
    }

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);
  g_object_unref (map_x_buffer);
  g_object_unref (map_y_buffer);

  if (dvals.mag_use == TRUE)
    g_object_unref (mag_buffer);

  gimp_progress_update (1.0);

  gimp_drawable_merge_shadow (draw_id, first_time);

  g_rand_free (gr);
}

/* ------------------------------------------------------------------------- */

static gdouble
warp_map_mag_give_value (guchar *pt,
                         gint    alpha,
                         gint    bytes)
{
  gdouble ret, val_alpha;

  if (bytes >= 3)
    ret =  (pt[0] + pt[1] + pt[2])/3.0;
  else
    ret = (gdouble) *pt;

  if (alpha)
    {
      val_alpha = pt[bytes - 1];
      ret = (ret * val_alpha / 255.0);
    };

  return (ret);
}


static void
warp_pixel (GeglBuffer *buffer,
            const Babl *format,
            gint        width,
            gint        height,
            gint        x1,
            gint        y1,
            gint        x2,
            gint        y2,
            gint        x,
            gint        y,
            guchar     *pixel)
{
  static guchar  empty_pixel[4] = { 0, 0, 0, 0 };
  guchar        *data;

  /* Tile the image. */
  if (dvals.wrap_type == WRAP)
    {
      if (x < 0)
        x = width - (-x % width);
      else
        x %= width;

      if (y < 0)
        y = height - (-y % height);
      else
        y %= height;
    }
  /* Smear out the edges of the image by repeating pixels. */
  else if (dvals.wrap_type == SMEAR)
    {
      if (x < 0)
        x = 0;
      else if (x > width - 1)
        x = width - 1;

      if (y < 0)
        y = 0;
      else if (y > height - 1)
        y = height - 1;
    }

  if (x >= x1 && y >= y1 && x < x2 && y < y2)
    {
      gegl_buffer_sample (buffer, x, y, NULL, pixel, format,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
    }
  else
    {
      gint bpp = babl_format_get_bytes_per_pixel (format);
      gint b;

      if (dvals.wrap_type == BLACK)
        data = empty_pixel;
      else
        data = color_pixel;      /* must have selected COLOR type */

      for (b = 0; b < bpp; b++)
        pixel[b] = data[b];
    }
}

/*  Warp interface functions  */

static gboolean
warp_map_constrain (gint32     image_id,
                    gint32     drawable_id,
                    gpointer   data)
{
  gint32 d_id = GPOINTER_TO_INT (data);

  return (gimp_drawable_width (drawable_id)  == gimp_drawable_width  (d_id) &&
          gimp_drawable_height (drawable_id) == gimp_drawable_height (d_id));
}
