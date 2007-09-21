/*
 * This is the Glass Tile plug-in for GIMP 1.2
 * Version 1.02
 *
 * Copyright (C) 1997 Karl-Johan Andersson (t96kja@student.tdb.uu.se)
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
 */

/*
 * This filter divide the image into square "glass"-blocks in which
 * the image is refracted.
 *
 * The alpha-channel is left unchanged.
 *
 * Please send any comments or suggestions to
 * Karl-Johan Andersson (t96kja@student.tdb.uu.se)
 *
 * May 2000 - tim copperfield [timecop@japan.co.jp]
 * Added preview mode.
 * Noticed there is an issue with the algorithm if odd number of rows or
 * columns is requested.  Dunno why.  I am not a graphics expert :(
 *
 * May 2000 alt@gimp.org Made preview work and removed some boundary
 * conditions that caused "streaks" to appear when using some tile spaces.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-glasstile"
#define PLUG_IN_BINARY "glasstile"


/* --- Typedefs --- */
typedef struct
{
  gint     xblock;
  gint     yblock;
  /* interface only */
  gint     constrain;
} GlassValues;

typedef struct
{
  GlassValues *gval;
  GtkObject   *xadj;
  GtkObject   *yadj;
} GlassChainedValues;

/* --- Declare local functions --- */
static void      query                   (void);
static void      run                     (const gchar      *name,
                                          gint              nparams,
                                          const GimpParam  *param,
                                          gint             *nreturn_vals,
                                          GimpParam       **return_vals);

static gboolean  glasstile_dialog        (GimpDrawable     *drawable);

static void      glasstile_size_changed  (GtkObject        *adj,
                                          gpointer          data);
static void      glasstile_chain_toggled (GtkWidget        *widget,
                                          gboolean         *value);

static void      glasstile                (GimpDrawable    *drawable,
                                           GimpPreview     *preview);


/* --- Variables --- */
const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static GlassValues gtvals =
{
  20,    /* tile width  */
  20,    /* tile height */
  /* interface only */
  TRUE   /* constrained */
};

/* --- Functions --- */

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (unused)"         },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"               },
    { GIMP_PDB_INT32,    "tilex",    "Tile width (10 - 50)"         },
    { GIMP_PDB_INT32,    "tiley",    "Tile height (10 - 50)"        }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Simulate distortion caused by square glass tiles"),
                          "Divide the image into square glassblocks in "
                          "which the image is refracted.",
                          "Karl-Johan Andersson", /* Author */
                          "Karl-Johan Andersson", /* Copyright */
                          "May 2000",
                          N_("_Glass Tile..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC,
                             "<Image>/Filters/Light and Shadow/Glass");
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

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  gimp_tile_cache_ntiles (2 * (drawable->ntile_cols));

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &gtvals);

      /*  First acquire information with a dialog  */
      if (! glasstile_dialog (drawable))
        {
          gimp_drawable_detach (drawable);
          return;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
        status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS)
        {
          gtvals.xblock = (gint) param[3].data.d_int32;
          gtvals.yblock = (gint) param[4].data.d_int32;
        }
      if (gtvals.xblock < 10 || gtvals.xblock > 50)
        status = GIMP_PDB_CALLING_ERROR;
      if (gtvals.yblock < 10 || gtvals.yblock > 50)
        status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &gtvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id) ||
          gimp_drawable_is_gray (drawable->drawable_id))
        {
          gimp_progress_init (_("Glass Tile"));

          glasstile (drawable, NULL);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();
          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            {
              gimp_set_data (PLUG_IN_PROC, &gtvals, sizeof (GlassValues));
            }
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static gboolean
glasstile_dialog (GimpDrawable *drawable)
{
  GlassChainedValues *gv;
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *table;
  GtkWidget *chainbutton;
  gboolean   run;

  gv = g_new (GlassChainedValues, 1);
  gv->gval = &gtvals;
  gtvals.constrain = TRUE;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Glass Tile"), PLUG_IN_BINARY,
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

  preview = gimp_drawable_preview_new (drawable, NULL);
  gtk_box_pack_start_defaults (GTK_BOX (main_vbox), preview);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (glasstile),
                            drawable);

  /*  Parameter settings  */
  table = gtk_table_new (2, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacing (GTK_TABLE (table), 2, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Horizontal scale - Width */
  gv->xadj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                   _("Tile _width:"), 150, 0,
                                   gtvals.xblock, 10, 50, 2, 10, 0,
                                   TRUE, 0, 0,
                                   NULL, NULL);

  g_signal_connect (gv->xadj, "value-changed",
                    G_CALLBACK (glasstile_size_changed),
                    gv);
  g_signal_connect_swapped (gv->xadj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* Horizontal scale - Height */
  gv->yadj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                                   _("Tile _height:"), 150, 0,
                                   gtvals.yblock, 10, 50, 2, 10, 0,
                                   TRUE, 0, 0,
                                   NULL, NULL);

  g_signal_connect (gv->yadj, "value-changed",
                    G_CALLBACK (glasstile_size_changed),
                    gv);
  g_signal_connect_swapped (gv->yadj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  chainbutton = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chainbutton),
                                gtvals.constrain);
  gtk_table_attach_defaults (GTK_TABLE(table), chainbutton, 3, 4, 0, 2);
  g_signal_connect (chainbutton, "toggled",
                    G_CALLBACK (glasstile_chain_toggled),
                    &gtvals.constrain);
  gtk_widget_show (chainbutton);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
glasstile_size_changed (GtkObject *adj,
                        gpointer   data)
{
  GlassChainedValues *gv = data;

  if (adj == gv->xadj)
    {
      gimp_int_adjustment_update(GTK_ADJUSTMENT (gv->xadj), &gv->gval->xblock);
      if (gv->gval->constrain)
        gtk_adjustment_set_value(GTK_ADJUSTMENT (gv->yadj),
                                 (gdouble) gv->gval->xblock);
    }
  else if (adj == gv->yadj)
    {
      gimp_int_adjustment_update(GTK_ADJUSTMENT (gv->yadj), &gv->gval->yblock);
      if (gv->gval->constrain)
        gtk_adjustment_set_value(GTK_ADJUSTMENT (gv->xadj),
                                 (gdouble) gv->gval->yblock);
    }
}

static void
glasstile_chain_toggled (GtkWidget *widget,
                         gboolean  *value)
{
  *value = gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (widget));
}

/*  -  Filter function  -  I wish all filter functions had a pmode :) */
static void
glasstile (GimpDrawable *drawable,
           GimpPreview  *preview)
{
  GimpPixelRgn  srcPR, destPR;
  gint          width, height;
  gint          bytes;
  guchar       *dest, *d;
  guchar       *cur_row;
  gint          row, col, i;
  gint          x1, y1, x2, y2;

  /* Translations of variable names from Maswan
   * rutbredd = grid width
   * ruthojd = grid height
   * ymitt = y middle
   * xmitt = x middle
   */

  gint rutbredd, xpixel1, xpixel2;
  gint ruthojd , ypixel2;
  gint xhalv, xoffs, xmitt, xplus;
  gint yhalv, yoffs, ymitt, yplus;

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
      width  = x2 - x1;
      height = y2 - y1;
    }
  bytes  = drawable->bpp;

  cur_row = g_new (guchar, width * bytes);
  dest    = g_new (guchar, width * bytes);

  /* initialize the pixel regions, set grid height/width */
  gimp_pixel_rgn_init (&srcPR, drawable,
                        x1, y1, width, height,
                        FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable,
                       x1, y1, width, height,
                       preview == NULL, TRUE);

  rutbredd = gtvals.xblock;
  ruthojd  = gtvals.yblock;

  xhalv = rutbredd / 2;
  yhalv = ruthojd  / 2;

  xplus = rutbredd % 2;
  yplus = ruthojd  % 2;

  ymitt = y1;
  yoffs = 0;

  /*  Loop through the rows */
  for (row = y1; row < y2; row++)
    {
      d = dest;

      ypixel2 = ymitt + yoffs * 2;
      ypixel2 = CLAMP (ypixel2, 0, y2 - 1);

      gimp_pixel_rgn_get_row (&srcPR, cur_row, x1, ypixel2, width);
      yoffs++;

      /* if current offset = half, do a displacement next time around */
      if (yoffs == yhalv)
        {
          ymitt += ruthojd;
          yoffs = - (yhalv + yplus);
        }

      xmitt = 0;
      xoffs = 0;

      for (col = 0; col < x2 - x1; col++) /* one pixel */
        {
          xpixel1 = (xmitt + xoffs) * bytes;
          xpixel2 = (xmitt + xoffs * 2) * bytes;

          if (xpixel2 < (x2 - x1) * bytes)
            {
              if (xpixel2 < 0)
                xpixel2 = 0;
              for (i = 0; i < bytes; i++)
                d[xpixel1 + i] = cur_row[xpixel2 + i];
            }
          else
            {
              for (i = 0; i < bytes; i++)
                d[xpixel1 + i] = cur_row[xpixel1 + i];
            }

          xoffs++;

          if (xoffs == xhalv)
            {
              xmitt += rutbredd;
              xoffs = - (xhalv + xplus);
            }
        }

      /*  Store the dest  */
      gimp_pixel_rgn_set_row (&destPR, dest, x1, row, width);

      if (!preview && ((row % 5) == 0))
        {
          gimp_progress_update ((gdouble) row / (gdouble) height);
        }
    }

  /*  Update region  */
  if (preview)
    {
      gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),
                                         &destPR);
    }
  else
    {
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id,
                            x1, y1, width, height);
    }

  g_free (cur_row);
  g_free (dest);
}
