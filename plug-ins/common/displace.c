/* Displace --- image filter plug-in for The Gimp image manipulation program
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
 * You can contact the original The Gimp authors at gimp@xcf.berkeley.edu
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
 * and the possibility to use RGB/RGBA pictures where the intensity
 * of the pixel is taken into account
 *
 */

/* Version 1.12. */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Some useful macros */

#define ENTRY_WIDTH     75
#define TILE_CACHE_SIZE 48

typedef struct
{
  gdouble  amount_x;
  gdouble  amount_y;
  gint     do_x;
  gint     do_y;
  gint     displace_map_x;
  gint     displace_map_y;
  gint     displace_type;
  gboolean preview;
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

static gboolean  displace_map_constrain    (gint32     image_id,
                                            gint32     drawable_id,
                                            gpointer   data);
static gdouble   displace_map_give_value   (guchar    *ptr,
                                            gint       alpha,
                                            gint       bytes);

/***** Local vars *****/

GimpPlugInInfo PLUG_IN_INFO =
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
  TRUE
};


/***** Functions *****/

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",       "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",          "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",       "Input drawable" },
    { GIMP_PDB_FLOAT,    "amount_x",       "Displace multiplier for X direction" },
    { GIMP_PDB_FLOAT,    "amount_y",       "Displace multiplier for Y direction" },
    { GIMP_PDB_INT32,    "do_x",           "Displace in X direction?" },
    { GIMP_PDB_INT32,    "do_y",           "Displace in Y direction?" },
    { GIMP_PDB_DRAWABLE, "displace_map_x", "Displacement map for X direction" },
    { GIMP_PDB_DRAWABLE, "displace_map_y", "Displacement map for Y direction" },
    { GIMP_PDB_INT32,    "displace_type",  "Edge behavior: { WRAP (0), SMEAR (1), BLACK (2) }" }
  };

  gimp_install_procedure ("plug_in_displace",
                          "Displace the contents of the specified drawable",
                          "Displaces the contents of the specified drawable "
                          "by the amounts specified by 'amount_x' and "
                          "'amount_y' multiplied by the intensity of "
                          "corresponding pixels in the 'displace_map' "
                          "drawables.  Both 'displace_map' drawables must be "
                          "of type GIMP_GRAY_IMAGE for this operation to succeed.",
                          "Stephen Robert Norris & (ported to 1.0 by) "
                          "Spencer Kimball",
                          "Stephen Robert Norris",
                          "1996",
                          N_("_Displace..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register ("plug_in_displace", "<Image>/Filters/Map");
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
      gimp_get_data ("plug_in_displace", &dvals);

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
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_displace", &dvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS && (dvals.do_x || dvals.do_y))
    {
      gimp_progress_init (_("Displacing..."));

      /*  run the displace effect  */
      displace (drawable, NULL);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data ("plug_in_displace", &dvals, sizeof (DisplaceVals));
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static gboolean
displace_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *toggle;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;
  GtkWidget *combo;
  GtkWidget *frame;
  GtkWidget *wrap;
  GtkWidget *smear;
  GtkWidget *black;
  gboolean   run;

  gimp_ui_init ("displace", FALSE);

  dialog = gimp_dialog_new (_("Displace"), "displace",
                            NULL, 0,
                            gimp_standard_help_func, "plug-in-displace",

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

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

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 12);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);

  /*  X options  */
  toggle = gtk_check_button_new_with_mnemonic (_("_X displacement:"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), dvals.do_x);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &dvals.do_x);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  spinbutton = gimp_spin_button_new (&adj, dvals.amount_x,
                                     (gint) drawable->width * -2,
                                     drawable->width * 2,
                                     1, 10, 0, 1, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 1, 2, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &dvals.amount_x);
  g_signal_connect_swapped (adj, "value_changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_set_sensitive (spinbutton, dvals.do_x);
  g_object_set_data (G_OBJECT (toggle), "set_sensitive", spinbutton);
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
  toggle = gtk_check_button_new_with_mnemonic (_("_Y displacement:"));
  gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), dvals.do_y);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &dvals.do_y);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  spinbutton = gimp_spin_button_new (&adj, dvals.amount_y,
                                     (gint) drawable->height * -2,
                                     drawable->height * 2,
                                     1, 10, 0, 1, 2);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 1, 2, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);

  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &dvals.amount_y);
  g_signal_connect_swapped (adj, "value_changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_set_sensitive (spinbutton, dvals.do_y);
  g_object_set_data (G_OBJECT (toggle), "set_sensitive", spinbutton);
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

  frame = gimp_int_radio_group_new (TRUE, _("On Edges:"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &dvals.displace_type, dvals.displace_type,

                                    _("_Wrap"),  GIMP_PIXEL_FETCHER_EDGE_WRAP,
                                    &wrap,
                                    _("_Smear"), GIMP_PIXEL_FETCHER_EDGE_SMEAR,
                                    &smear,
                                    _("_Black"), GIMP_PIXEL_FETCHER_EDGE_BLACK,
                                    &black,

                                    NULL);
  gtk_table_attach (GTK_TABLE (table), frame, 0, 3, 2, 3,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (frame);

  g_signal_connect_swapped (wrap, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (smear, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  g_signal_connect_swapped (black, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

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
  GimpDrawable     *map_x;
  GimpDrawable     *map_y;
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
  gint              progress, max_progress;

  gdouble           amnt;
  gdouble           needx, needy;
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

  /* initialize */

  mxrow = NULL;
  myrow = NULL;

  pft = gimp_pixel_fetcher_new (drawable, FALSE);
  gimp_pixel_fetcher_set_edge_mode (pft, dvals.displace_type);

  bytes  = drawable->bpp;

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);
      x2 = x1 + width;
      y2 = y1 + height;
      buffer = g_new (guchar, width * height * bytes);
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id,
                                 &x1, &y1, &x2, &y2);
      width  = x2 - x1;
      height = y2 - y1;
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
      if (gimp_drawable_has_alpha(map_x->drawable_id))
        xm_alpha = 1;
      xm_bytes = gimp_drawable_bpp(map_x->drawable_id);
    }
  else
    map_x = NULL;

  if (dvals.displace_map_y != -1 && dvals.do_y)
    {
      map_y = gimp_drawable_get (dvals.displace_map_y);
      gimp_pixel_rgn_init (&map_y_rgn, map_y,
                           x1, y1, width, height, FALSE, FALSE);
      if (gimp_drawable_has_alpha(map_y->drawable_id))
        ym_alpha = 1;
      ym_bytes = gimp_drawable_bpp(map_y->drawable_id);
    }
  else
    map_y = NULL;

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
           * We could move the displacement image address calculation out of here,
           * but when we can have different sized displacement and destination
           * images we'd have to move it back anyway.
           */

          for (x = dest_rgn.x; x < (dest_rgn.x + dest_rgn.w); x++)
            {
              if (dvals.do_x)
                {
                  xm_val = displace_map_give_value(mx, xm_alpha, xm_bytes);
                  amnt = dvals.amount_x * (xm_val - 127.5) / 127.5;
                  needx = x + amnt;
                  mx += xm_bytes;
                }
              else
                needx = x;

              if (dvals.do_y)
                {
                  ym_val = displace_map_give_value(my, ym_alpha, ym_bytes);
                  amnt = dvals.amount_y * (ym_val - 127.5) / 127.5;
                  needy = y + amnt;
                  my += ym_bytes;
                }
              else
                needy = y;

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
    ret =  GIMP_RGB_INTENSITY (pt[0], pt[1], pt[2]);
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
