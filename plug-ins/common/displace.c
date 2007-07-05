/* Displace --- image filter plug-in for GIMP
 * Copyright (C) 1996 Stephen Robert Norris
 * Much of the code taken from the pinch plug-in by 1996 Federico Mena Quintero
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
 *
 * You can contact me at srn@flibble.cs.su.oz.au.
 * Please send me any patches or enhancements to this code.
 * You can contact the original GIMP authors at gimp@xcf.berkeley.edu
 *
 * Extensive modifications to the dialog box, parameters, and some
 * legibility stuff in displace() by Federico Mena Quintero ---
 * federico@nuclecu.unam.mx.  If there are any bugs in these
 * changes, they are my fault and not Stephen's.
 *
 * JTL: May 29th 1997
 * Added (part of) the patch from Eiichi Takamori
 *    -- the part which removes the border artefacts
 * (http://ha1.seikyou.ne.jp/home/taka/gimp/displace/displace.html)
 * Added ability to use transparency as the identity transformation
 * (Full transparency is treated as if it was grey 0.5)
 * and the possibility to use RGB/RGBA pictures where the luminance
 * of the pixel is taken into account
 *
 * Joao S. O. Bueno, Dec. 2004:
 *
 * Added functionality to displace using polar coordinates -
 * For a plain, non neutral map, works like whirl and pinch
 */

/* Version 1.12. */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Some useful macros */

#define PLUG_IN_PROC    "plug-in-displace"
#define PLUG_IN_BINARY  "displace"

#define ENTRY_WIDTH     75
#define TILE_CACHE_SIZE 48


typedef enum
{
  CARTESIAN_MODE = 0,
  POLAR_MODE     = 1
} DisplaceMode;

typedef struct
{
  gdouble      amount_x;
  gdouble      amount_y;
  gint         do_x;
  gint         do_y;
  gint         displace_map_x;
  gint         displace_map_y;
  gint         displace_type;
  DisplaceMode mode;
  gboolean     preview;
} DisplaceVals;


/*
 * Function prototypes.
 */

static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static void      displace        (GimpDrawable *drawable,
                                  GimpPreview  *preview);
static gboolean  displace_dialog (GimpDrawable *drawable);

static void      displace_radio_update   (GtkWidget     *widget,
                                          gpointer       data);

static void      displace_set_labels     (void);
static gint      displace_get_label_size (void);

static gboolean  displace_map_constrain    (gint32     image_id,
                                            gint32     drawable_id,
                                            gpointer   data);
static gdouble   displace_map_give_value   (guchar    *ptr,
                                            gint       alpha,
                                            gint       bytes);

/***** Local vars *****/

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static DisplaceVals dvals =
{
  20.0,                         /* amount_x */
  20.0,                         /* amount_y */
  TRUE,                         /* do_x */
  TRUE,                         /* do_y */
  -1,                           /* displace_map_x */
  -1,                           /* displace_map_y */
  GIMP_PIXEL_FETCHER_EDGE_WRAP, /* displace_type */
  CARTESIAN_MODE,               /* mode */
  TRUE
};


/***** Variables *****/

static GtkWidget   *preview        = NULL;
static GtkWidget   *toggle_x       = NULL;
static GtkWidget   *toggle_y       = NULL;

static const gchar *mtext[][2] =
{
  { N_("_X displacement"),   N_("_Pinch") },
  { N_("_Y displacement"),   N_("_Whirl") }
};


/***** Functions *****/

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",       "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",          "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",       "Input drawable" },
    { GIMP_PDB_FLOAT,    "amount-x",       "Displace multiplier for X or radial direction" },
    { GIMP_PDB_FLOAT,    "amount-y",       "Displace multiplier for Y or tangent (degrees) direction" },
    { GIMP_PDB_INT32,    "do-x",           "Displace in X or radial direction?" },
    { GIMP_PDB_INT32,    "do-y",           "Displace in Y or tangent direction?" },
    { GIMP_PDB_DRAWABLE, "displace-map-x", "Displacement map for X or radial direction" },
    { GIMP_PDB_DRAWABLE, "displace-map-y", "Displacement map for Y or tangent direction" },
    { GIMP_PDB_INT32,    "displace-type",  "Edge behavior: { WRAP (0), SMEAR (1), BLACK (2) }" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Displace pixels as indicated by displacement maps"),
                          "Displaces the contents of the specified drawable "
                          "by the amounts specified by 'amount-x' and "
                          "'amount-y' multiplied by the luminance of "
                          "corresponding pixels in the 'displace-map' "
                          "drawables.",
                          "Stephen Robert Norris & (ported to 1.0 by) "
                          "Spencer Kimball",
                          "Stephen Robert Norris",
                          "1996",
                          N_("_Displace..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Map");

  gimp_install_procedure ("plug-in-displace-polar",
                          "Displace the contents of the specified drawable",
                          "Just like plug-in-displace but working in "
                          "polar coordinates. The drawable is whirled and "
                          "pinched according to the map.",
                          "Stephen Robert Norris & (ported to 1.0 by) "
                          "Spencer Kimball",
                          "Stephen Robert Norris",
                          "1996",
                          "Displace Polar",
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  set the tile cache size  */
  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

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
      if (! displace_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 10)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          dvals.amount_x       = param[3].data.d_float;
          dvals.amount_y       = param[4].data.d_float;
          dvals.do_x           = param[5].data.d_int32;
          dvals.do_y           = param[6].data.d_int32;
          dvals.displace_map_x = param[7].data.d_int32;
          dvals.displace_map_y = param[8].data.d_int32;
          dvals.displace_type  = param[9].data.d_int32;

          dvals.mode = (strcmp (name, "plug-in-displace-polar") == 0 ?
                        POLAR_MODE : CARTESIAN_MODE);
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
      if (dvals.displace_map_x != -1 &&
          (gimp_drawable_width (dvals.displace_map_x) != drawable->width ||
           gimp_drawable_height (dvals.displace_map_x) != drawable->height))
        status = GIMP_PDB_CALLING_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (dvals.displace_map_y != -1 &&
          (gimp_drawable_width (dvals.displace_map_y) != drawable->width ||
           gimp_drawable_height (dvals.displace_map_y) != drawable->height))
        status = GIMP_PDB_CALLING_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS && (dvals.do_x || dvals.do_y))
    {
      gimp_progress_init (_("Displacing"));

      /*  run the displace effect  */
      displace (drawable, NULL);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &dvals, sizeof (DisplaceVals));
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static gboolean
displace_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;
  GtkWidget *combo;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *wrap;
  GtkWidget *smear;
  GtkWidget *black;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Displace"), PLUG_IN_BINARY,
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

  preview = gimp_drawable_preview_new (drawable, &dvals.preview);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (displace),
                            drawable);

  /*  The main table  */

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 12);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);

  /*  X options  */
  toggle_x = gtk_check_button_new_with_mnemonic (_("_X displacement:"));
  gtk_table_attach (GTK_TABLE (table), toggle_x, 0, 1, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle_x), dvals.do_x);
  gtk_widget_show (toggle_x);

  g_signal_connect (toggle_x, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &dvals.do_x);
  g_signal_connect_swapped (toggle_x, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  spinbutton = gimp_spin_button_new (&adj, dvals.amount_x,
                                     (gint) drawable->width * -2,
                                     drawable->width * 2,
                                     1, 10, 0, 1, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 1, 2, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &dvals.amount_x);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_set_sensitive (spinbutton, dvals.do_x);
  g_object_set_data (G_OBJECT (toggle_x), "set_sensitive", spinbutton);
  gtk_widget_show (spinbutton);

  combo = gimp_drawable_combo_box_new (displace_map_constrain, drawable);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), dvals.displace_map_x,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &dvals.displace_map_x);
  g_signal_connect_swapped (combo, "changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_table_attach (GTK_TABLE (table), combo, 2, 3, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  gtk_widget_set_sensitive (combo, dvals.do_x);
  g_object_set_data (G_OBJECT (spinbutton), "set_sensitive", combo);

  /*  Y Options  */
  toggle_y = gtk_check_button_new_with_mnemonic (_("_Y displacement:"));
  gtk_table_attach (GTK_TABLE (table), toggle_y, 0, 1, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle_y), dvals.do_y);
  gtk_widget_show (toggle_y);

  g_signal_connect (toggle_y, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &dvals.do_y);
  g_signal_connect_swapped (toggle_y, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  spinbutton = gimp_spin_button_new (&adj, dvals.amount_y,
                                     (gint) drawable->height * -2,
                                     drawable->height * 2,
                                     1, 10, 0, 1, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 1, 2, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &dvals.amount_y);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_set_sensitive (spinbutton, dvals.do_y);
  g_object_set_data (G_OBJECT (toggle_y), "set_sensitive", spinbutton);
  gtk_widget_show (spinbutton);

  combo = gimp_drawable_combo_box_new (displace_map_constrain, drawable);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), dvals.displace_map_y,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &dvals.displace_map_y);
  g_signal_connect_swapped (combo, "changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_table_attach (GTK_TABLE (table), combo, 2, 3, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  gtk_widget_set_sensitive (combo, dvals.do_y);
  g_object_set_data (G_OBJECT (spinbutton), "set_sensitive", combo);

  hbox = gtk_hbox_new (FALSE, 24);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gimp_int_radio_group_new (TRUE, _("Displacement Mode"),
                                    G_CALLBACK (displace_radio_update),
                                    &dvals.mode, dvals.mode,
                                    _("_Cartesian"), CARTESIAN_MODE, NULL,
                                    _("_Polar"),     POLAR_MODE,     NULL,
                                    NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show  (frame);

  frame = gimp_int_radio_group_new (TRUE, _("Edge Behavior"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &dvals.displace_type, dvals.displace_type,

                                    _("_Wrap"),  GIMP_PIXEL_FETCHER_EDGE_WRAP,
                                    &wrap,
                                    _("_Smear"), GIMP_PIXEL_FETCHER_EDGE_SMEAR,
                                    &smear,
                                    _("_Black"), GIMP_PIXEL_FETCHER_EDGE_BLACK,
                                    &black,

                                    NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show  (frame);

  g_signal_connect_swapped (wrap, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (smear, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (black, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  displace_set_labels ();

  gtk_widget_show (table);
  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

/* The displacement is done here. */

static void
displace (GimpDrawable *drawable,
          GimpPreview  *preview)
{
  GimpDrawable     *map_x = NULL;
  GimpDrawable     *map_y = NULL;
  GimpPixelRgn      dest_rgn;
  GimpPixelRgn      map_x_rgn;
  GimpPixelRgn      map_y_rgn;
  gpointer          pr;
  GimpPixelFetcher *pft;

  gint              width;
  gint              height;
  gint              bytes;
  guchar           *destrow, *dest;
  guchar           *mxrow, *mx;
  guchar           *myrow, *my;
  guchar            pixel[4][4];
  gint              x1, y1, x2, y2;
  gint              x, y;
  gdouble           cx, cy;
  gint              progress, max_progress;

  gdouble           amnt;
  gdouble           needx, needy;
  gdouble           radius, d_alpha;
  gint              xi, yi;

  guchar            values[4];
  guchar            val;

  gint              k;

  gdouble           xm_val, ym_val;
  gint              xm_alpha = 0;
  gint              ym_alpha = 0;
  gint              xm_bytes = 1;
  gint              ym_bytes = 1;
  guchar           *buffer   = NULL;
  gdouble           pi;

  /* initialize */

  /* get rid of uninitialized warnings */
  cx = cy = needx = needy = radius = d_alpha = 0.0;

  pi = 4 * atan (1);

  mxrow = NULL;
  myrow = NULL;

  pft = gimp_pixel_fetcher_new (drawable, FALSE);
  gimp_pixel_fetcher_set_edge_mode (pft, dvals.displace_type);

  bytes  = drawable->bpp;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
  width  = x2 - x1;
  height = y2 - y1;

  if (dvals.mode == POLAR_MODE)
    {
      cx = x1 + width / 2.0;
      cy = y1 + height / 2.0;
    }

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);
      x2 = x1 + width;
      y2 = y1 + height;
      buffer = g_new (guchar, width * height * bytes);
    }

  progress     = 0;
  max_progress = width * height;

  /*
   * The algorithm used here is simple - see
   * http://the-tech.mit.edu/KPT/Tips/KPT7/KPT7.html for a description.
   */

  /* Get the drawables  */
  if (dvals.displace_map_x != -1 && dvals.do_x)
    {
      map_x = gimp_drawable_get (dvals.displace_map_x);
      gimp_pixel_rgn_init (&map_x_rgn, map_x,
                           x1, y1, width, height, FALSE, FALSE);

      if (gimp_drawable_has_alpha (map_x->drawable_id))
        xm_alpha = 1;

      xm_bytes = gimp_drawable_bpp (map_x->drawable_id);
    }

  if (dvals.displace_map_y != -1 && dvals.do_y)
    {
      map_y = gimp_drawable_get (dvals.displace_map_y);
      gimp_pixel_rgn_init (&map_y_rgn, map_y,
                           x1, y1, width, height, FALSE, FALSE);

      if (gimp_drawable_has_alpha (map_y->drawable_id))
        ym_alpha = 1;

      ym_bytes = gimp_drawable_bpp (map_y->drawable_id);
    }

  gimp_pixel_rgn_init (&dest_rgn, drawable,
                       x1, y1, width, height,
                       preview == NULL, preview == NULL);

  /*  Register the pixel regions  */
  if (dvals.do_x && dvals.do_y)
    pr = gimp_pixel_rgns_register (3, &dest_rgn, &map_x_rgn, &map_y_rgn);
  else if (dvals.do_x)
    pr = gimp_pixel_rgns_register (2, &dest_rgn, &map_x_rgn);
  else if (dvals.do_y)
    pr = gimp_pixel_rgns_register (2, &dest_rgn, &map_y_rgn);
  else
    pr = NULL;

  for (pr = pr; pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      destrow = dest_rgn.data;
      if (dvals.do_x)
        mxrow = map_x_rgn.data;
      if (dvals.do_y)
        myrow = map_y_rgn.data;

      for (y = dest_rgn.y; y < (dest_rgn.y + dest_rgn.h); y++)
        {
          if (preview)
            dest = buffer + ((y - y1) * width + (dest_rgn.x - x1)) * bytes;
          else
            dest = destrow;
          mx = mxrow;
          my = myrow;

          /*
           * We could move the displacement image address calculation
           * out of here, but when we can have different sized
           * displacement and destination images we'd have to move it
           * back anyway.
           */

          for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++)
            {
              if (dvals.do_x)
                {
                  xm_val = displace_map_give_value(mx, xm_alpha, xm_bytes);
                  amnt = dvals.amount_x * (xm_val - 127.5) / 127.5;
                  /* CARTESIAN_MODE == 0 - performance important here */
                  if (! dvals.mode)
                    {
                      needx = x + amnt;
                    }
                  else
                    {
                      radius = sqrt (SQR (x - cx) + SQR (y - cy)) + amnt;
                    }
                  mx += xm_bytes;
                }
              else
                {
                  if (! dvals.mode)
                    needx = x;
                  else
                    radius = sqrt ((x - cx) * (x - cx) + (y - cy) * (y - cy));
                }


              if (dvals.do_y)
                {
                  ym_val = displace_map_give_value(my, ym_alpha, ym_bytes);
                  amnt = dvals.amount_y * (ym_val - 127.5) / 127.5;
                  if (! dvals.mode)
                    {
                      needy = y + amnt;
                    }
                  else
                    {
                      d_alpha = atan2 (x - cx, y - cy) + (dvals.amount_y / 180)
                                * pi * (ym_val - 127.5) / 127.5;
                    }
                  my += ym_bytes;
                }
              else
                {
                  if (! dvals.mode)
                    needy = y;
                  else
                    d_alpha = atan2 (x - cx, y - cy);
                }
              if (dvals.mode)
                {
                   needx = cx + radius * sin (d_alpha);
                   needy = cy + radius * cos (d_alpha);
                }
              /* Calculations complete; now copy the proper pixel */

              if (needx >= 0.0)
                xi = (int) needx;
              else
                xi = -((int) -needx + 1);

              if (needy >= 0.0)
                yi = (int) needy;
              else
                yi = -((int) -needy + 1);

              gimp_pixel_fetcher_get_pixel (pft, xi, yi, pixel[0]);
              gimp_pixel_fetcher_get_pixel (pft, xi + 1, yi, pixel[1]);
              gimp_pixel_fetcher_get_pixel (pft, xi, yi + 1, pixel[2]);
              gimp_pixel_fetcher_get_pixel (pft, xi + 1, yi + 1, pixel[3]);

              for (k = 0; k < bytes; k++)
                {
                  values[0] = pixel[0][k];
                  values[1] = pixel[1][k];
                  values[2] = pixel[2][k];
                  values[3] = pixel[3][k];
                  val = gimp_bilinear_8 (needx, needy, values);

                  *dest++ = val;
                } /* for */
            }

          destrow += dest_rgn.rowstride;

          if (dvals.do_x)
            mxrow += map_x_rgn.rowstride;
          if (dvals.do_y)
            myrow += map_y_rgn.rowstride;
        }

      if (!preview)
        {
          progress += dest_rgn.w * dest_rgn.h;
          gimp_progress_update ((double) progress / (double) max_progress);
        }
    } /* for */

  gimp_pixel_fetcher_destroy (pft);

  /*  detach from the map drawables  */
  if (dvals.do_x)
    gimp_drawable_detach (map_x);
  if (dvals.do_y)
    gimp_drawable_detach (map_y);

  if (preview)
    {
/*      gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),
                                         &dest_rgn);*/
      gimp_preview_draw_buffer (preview, buffer, width * bytes);
      g_free (buffer);
    }
  else
    {
      /*  update the region  */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);
    }
}

static gdouble
displace_map_give_value (guchar *pt,
                         gint    alpha,
                         gint    bytes)
{
  gdouble ret, val_alpha;

  if (bytes >= 3)
    ret =  GIMP_RGB_LUMINANCE (pt[0], pt[1], pt[2]);
  else
    ret = (gdouble) *pt;

  if (alpha)
    {
      val_alpha = pt[bytes - 1];
      ret = ((ret - 127.5) * val_alpha / 255.0) + 127.5;
    }

  return ret;
}

/*  Displace interface functions  */

static gboolean
displace_map_constrain (gint32   image_id,
                        gint32   drawable_id,
                        gpointer data)
{
  GimpDrawable *drawable = data;

  return (gimp_drawable_width (drawable_id)  == drawable->width &&
          gimp_drawable_height (drawable_id) == drawable->height);
}

static void
displace_radio_update (GtkWidget *widget,
                        gpointer   data)
{
  gimp_radio_button_update (widget, data);

  displace_set_labels ();

  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void
displace_set_labels (void)
{
  /* get the max. possible size of both check-buttons */
  gint label_maxwidth = displace_get_label_size();

  gtk_button_set_label (GTK_BUTTON (toggle_x),
                        gettext (mtext[0][dvals.mode]));
  gtk_button_set_label (GTK_BUTTON (toggle_y),
                        gettext (mtext[1][dvals.mode]));

  /* "displace_get_label_size()" must be called before */
  gtk_widget_set_size_request (toggle_x, label_maxwidth, -1);
  gtk_widget_set_size_request (toggle_y, label_maxwidth, -1);
}

static gint
displace_get_label_size (void)
{
  static gint  label_maxwidth = 0;
         gint  i, j;

  if (!label_maxwidth)
    for (i = 0; i < 2; i++)
      for (j = 0; j < 2; j++)
        {
          GtkRequisition  requisition;
  
          gtk_button_set_label (GTK_BUTTON (toggle_x), gettext (mtext[i][j]));
          gtk_widget_size_request (toggle_x, &requisition);
  
          if (requisition.width > label_maxwidth)
            label_maxwidth = requisition.width;
        }
  return label_maxwidth;
}
