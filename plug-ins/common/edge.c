/* edge filter for the GIMP
 *  -Peter Mattis
 *
 * This filter performs edge detection on the input image.
 *  The code for this filter is based on "pgmedge", a program
 *  that is part of the netpbm package.
 */

/* pgmedge.c - edge-detect a portable graymap
**
** Copyright (C) 1989 by Jef Poskanzer.
*/

/*
 *  Ported to GIMP Plug-in API 1.0
 *  version 1.07
 *  This version requires GIMP v0.99.10 or above.
 *
 *  This plug-in performs edge detection. The code is based on edge.c
 *  for GIMP 0.54 by Peter Mattis.
 *
 *      Eiichi Takamori <taka@ma1.seikyou.ne.jp>
 *      http://ha1.seikyou.ne.jp/home/taka/gimp/
 *
 *  Tips: you can enter arbitrary value into entry.
 *      (not bounded between 1.0 and 10.0)
 *
 *  Changes from version 1.06 to version 1.07:
 *  - Added entry
 *  - Cleaned up code a bit
 *
 *  Differences from Peter Mattis's original `edge' plug-in:
 *    - Added Wrapmode. (useful for tilable images)
 *    - Enhanced speed in this version.
 *    - It works with the alpha channel.
 */

/*  29 July 2003   Dave Neary  <bolsh@gimp.org>
 *  Added more edge detection routines, from the thin_line
 *  plug-in by iccii
 */

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#ifdef RCSID
static gchar rcsid[] = "$Id$";
#endif

/* Some useful macros */

#define TILE_CACHE_SIZE 48

enum
{
  SOBEL,
  PREWITT,
  GRADIENT,
  ROBERTS,
  DIFFERENTIAL,
  LAPLACE,
};

typedef struct
{
  gdouble  amount;
  gint     edgemode;
  gint     wrapmode;
} EdgeVals;

/*
 * Function prototypes.
 */

static void      query       (void);
static void      run         (const gchar      *name,
                              gint              nparams,
                              const GimpParam  *param,
                              gint             *nreturn_vals,
                              GimpParam       **return_vals);

static void      edge        (GimpDrawable     *drawable);
static gboolean  edge_dialog (GimpDrawable     *drawable);

static gint edge_detect  (guchar *data);
static gint prewitt      (guchar *data);
static gint gradient     (guchar *data);
static gint roberts      (guchar *data);
static gint differential (guchar *data);
static gint laplace      (guchar *data);
static gint sobel        (guchar *data);

/***** Local vars *****/

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init  */
  NULL,  /* quit  */
  query, /* query */
  run,   /* run   */
};

static EdgeVals evals =
{
  2.0,         			/* amount */
  SOBEL,       			/* Edge detection algorithm */
  GIMP_PIXEL_FETCHER_EDGE_SMEAR /* wrapmode */
};

/***** Functions *****/

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_FLOAT, "amount", "Edge detection amount" },
    { GIMP_PDB_INT32, "wrapmode", "Edge detection behavior: { WRAP (0), SMEAR (1), BLACK (2) }" },
    { GIMP_PDB_INT32, "edgemode", "Edge detection algorithm: { SOBEL (0), PREWITT (1), GRADIENT (2), ROBERTS (3),  DIFFERENTIAL (4), LAPLACE (5) }" }
  };

  const gchar *help_string =
    "Perform edge detection on the contents of the specified drawable."
    "AMOUNT is an arbitrary constant, WRAPMODE is like displace plug-in "
    "(useful for tilable image). EDGEMODE sets the kind of matrix "
    "transform applied to the pixels, SOBEL was the method used in older "
    "versions.";

  gimp_install_procedure ("plug_in_edge",
                          "Perform edge detection on the contents of the specified drawable",
                          help_string,
                          "Peter Mattis & (ported to 1.0 by) Eiichi Takamori",
                          "Peter Mattis",
                          "1996",
                          N_("_Edge..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register ("plug_in_edge",
                             N_("<Image>/Filters/Edge-Detect"));
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

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_edge", &evals);

      /*  First acquire information with a dialog  */
      if (! edge_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 5 && nparams != 6)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      if (status == GIMP_PDB_SUCCESS)
        {
          evals.amount   = param[3].data.d_float;
          evals.wrapmode = param[4].data.d_int32;
          evals.edgemode = nparams > 5 ? param[5].data.d_int32 : SOBEL;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_edge", &evals);
      break;

    default:
      break;
    }

  /* make sure the drawable exist and is not indexed */
  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id))
    {
      gimp_progress_init (_("Edge Detection..."));

      /*  set the tile cache size  */
      gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

      /*  run the edge effect  */
      edge (drawable);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data ("plug_in_edge", &evals, sizeof (EdgeVals));
    }
  else
    {
      /* gimp_message ("edge: cannot operate on indexed color images"); */
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

/********************************************************/
/*              Edge Detection main                     */
/********************************************************/

static void
edge (GimpDrawable *drawable)
{
  /*
   * this function is too long, so I must split this into a few
   * functions later ...  -- taka
   */
  GimpPixelRgn src_rgn, dest_rgn;
  gpointer pr;
  GimpPixelFetcher *pft;
  guchar *srcrow, *src;
  guchar *destrow, *dest;
  guchar pix00[4], pix01[4], pix02[4];
  guchar pix10[4], pix11[4], pix12[4];
  guchar pix20[4], pix21[4], pix22[4];
  glong width, height;
  gint alpha, has_alpha, chan;
  gint x, y;
  gint x1, y1, x2, y2;
  gint maxval;
  gint cur_progress;
  gint max_progress;

  if (evals.amount < 1.0)
    evals.amount = 1.0;

  pft = gimp_pixel_fetcher_new (drawable, FALSE);
  gimp_pixel_fetcher_set_edge_mode (pft, evals.wrapmode);

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  width = gimp_drawable_width (drawable->drawable_id);
  height = gimp_drawable_height (drawable->drawable_id);
  alpha = gimp_drawable_bpp (drawable->drawable_id);
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  if (has_alpha)
    alpha--;

  maxval = 255;

  cur_progress = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, x2-x1, y2-y1, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, x2-x1, y2-y1, TRUE, TRUE);

  for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      srcrow  = src_rgn.data;
      destrow = dest_rgn.data;

      for (y = dest_rgn.y;
           y < (dest_rgn.y + dest_rgn.h);
           y++, srcrow += src_rgn.rowstride, destrow += dest_rgn.rowstride)
        {
          src  = srcrow;
          dest = destrow;

          for (x = dest_rgn.x;
               x < (dest_rgn.x + dest_rgn.w);
               x++,  src += src_rgn.bpp, dest += dest_rgn.bpp)
            {
              if (dest_rgn.x < x &&  x < dest_rgn.x + dest_rgn.w - 2 &&
                  dest_rgn.y < y &&  y < dest_rgn.y + dest_rgn.h - 2)
                {
                  /*
                  ** 3x3 kernel is inside of the tile -- do fast
                  ** version
                  */
                  for (chan = 0; chan < alpha; chan++)
                    {
                      /* get the 3x3 kernel into a guchar array,
                       * and send it to edge_detect */
                      guchar kernel[9];
                      gint   i,j;

#define PIX(X,Y)  src[ (Y-1)*(int)src_rgn.rowstride + (X-1)*(int)src_rgn.bpp + chan ]
                      /* make convolution */
                      for(i = 0; i < 3; i++)
                        for(j = 0; j < 3; j++)
                          kernel[3*i + j] = PIX(i,j);

#undef  PIX

                      dest[chan] = edge_detect(kernel);
                    }
                }
              else
                {
                  /*
                  ** The kernel is not inside of the tile -- do slow
                  ** version
                  */

                  gimp_pixel_fetcher_get_pixel (pft, x-1, y-1, pix00);
                  gimp_pixel_fetcher_get_pixel (pft, x  , y-1, pix10);
                  gimp_pixel_fetcher_get_pixel (pft, x+1, y-1, pix20);
                  gimp_pixel_fetcher_get_pixel (pft, x-1, y  , pix01);
                  gimp_pixel_fetcher_get_pixel (pft, x  , y  , pix11);
                  gimp_pixel_fetcher_get_pixel (pft, x+1, y  , pix21);
                  gimp_pixel_fetcher_get_pixel (pft, x-1, y+1, pix02);
                  gimp_pixel_fetcher_get_pixel (pft, x  , y+1, pix12);
                  gimp_pixel_fetcher_get_pixel (pft, x+1, y+1, pix22);

                  for (chan = 0; chan < alpha; chan++)
                    {
                      guchar kernel[9];

                      kernel[0] = pix00[chan];
                      kernel[1] = pix01[chan];
                      kernel[2] = pix02[chan];
                      kernel[3] = pix10[chan];
                      kernel[4] = pix11[chan];
                      kernel[5] = pix12[chan];
                      kernel[6] = pix20[chan];
                      kernel[7] = pix21[chan];
                      kernel[8] = pix22[chan];

                      dest[chan] = edge_detect (kernel);
                    }
                }
              if (has_alpha)
                dest[alpha] = src[alpha];
            }
        }
      cur_progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((double) cur_progress / (double) max_progress);
    }

  gimp_pixel_fetcher_destroy (pft);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
}

/* ***********************   Edge Detection   ******************** */


/*
 * Edge detect switcher function
 */

static gint
edge_detect (guchar *data)
{
  gint ret;
  switch (evals.edgemode)
    {
       case SOBEL:
         ret = sobel (data);
         break;
       case PREWITT:
         ret = prewitt (data);
         break;
       case GRADIENT:
         ret = gradient (data);
         break;
       case ROBERTS:
         ret = roberts (data);
         break;
       case DIFFERENTIAL:
         ret = differential (data);
         break;
       case LAPLACE:
         ret = laplace (data);
         break;
       default:
         ret = -1;
         break;
    }
  return ret;
}


/*
 * Sobel Edge detector
 */
static gint
sobel (guchar *data)
{
  gint    v_kernel[9] = {-1,  0,  1,
                         -2,  0,  2,
                         -1,  0,  1};
  gint    h_kernel[9] = {-1, -2, -1,
                          0,  0,  0,
                          1,  2,  1};

  gint i;
  gint    v_grad, h_grad;

  v_grad = 0;
  h_grad = 0;

  for (i = 0; i < 9; i++)
    {
      v_grad += v_kernel[i] * data[i];
      h_grad += h_kernel[i] * data[i];
    }
  return CLAMP(sqrt (v_grad * v_grad * evals.amount +
        h_grad * h_grad * evals.amount), 0, 255);
}

/*
 * Edge detector via template matting
 *   -- Prewitt
 */
static gint
prewitt (guchar *data)
{
  gint    k, max;
  gint    m[8];

  m[0] =   data [0] +   data [1] + data [2]
         + data [3] - 2*data [4] + data [5]
         - data [6] -   data [7] - data [8];
  m[1] =   data [0] +   data [1] + data [2]
         + data [3] - 2*data [4] - data [5]
         + data [6] -   data [7] - data [8];
  m[2] =   data [0] +   data [1] - data [2]
         + data [3] - 2*data [4] - data [5]
         + data [6] +   data [7] - data [8];
  m[3] =   data [0] -   data [1] - data [2]
         + data [3] - 2*data [4] - data [5]
         + data [6] +   data [7] + data [8];
  m[4] = - data [0] -   data [1] - data [2]
         + data [3] - 2*data [4] + data [5]
         + data [6] +   data [7] + data [8];
  m[5] = - data [0] -   data [1] + data [2]
         - data [3] - 2*data [4] + data [5]
         + data [6] +   data [7] + data [8];
  m[6] = - data [0] +   data [1] + data [2]
         - data [3] - 2*data [4] + data [5]
         - data [6] +   data [7] + data [8];
  m[7] =   data [0] +   data [1] + data [2]
         - data [3] - 2*data [4] + data [5]
         - data [6] -   data [7] + data [8];

  max = 0;
  for (k = 0; k < 8; k++)
    if (max < m[k])
      max = m[k];

  return CLAMP(evals.amount * max, 0, 255);
}


/*
 * Gradient Edge detector
 */
static gint
gradient (guchar *data)
{
  gint    v_kernel[9] = { 0,  0,  0,
                          0,  4, -4,
                          0,  0,  0};
  gint    h_kernel[9] = { 0,  0,  0,
                          0, -4,  0,
                          0,  4,  0};

  gint i;
  gint    v_grad, h_grad;

  v_grad = 0;
  h_grad = 0;

  for (i = 0; i < 9; i++)
    {
      v_grad += v_kernel[i] * data[i];
      h_grad += h_kernel[i] * data[i];
    }

  return  CLAMP(sqrt (v_grad * v_grad * evals.amount +
                      h_grad * h_grad * evals.amount), 0, 255);
}



/*
 * Roberts Edge detector
 */
static gint
roberts (guchar *data)
{
  gint    v_kernel[9] = {0,  0,  0,
                         0,  4,  0,
                         0,  0, -4};
  gint    h_kernel[9] = {0,  0,  0,
                         0,  0,  4,
                         0, -4,  0};

  gint i;
  gint    v_grad, h_grad;

  v_grad = 0;
  h_grad = 0;

  for (i = 0; i < 9; i++)
    {
      v_grad += v_kernel[i] * data[i];
      h_grad += h_kernel[i] * data[i];
    }

  return  CLAMP(sqrt (v_grad * v_grad * evals.amount +
                      h_grad * h_grad * evals.amount), 0, 255);

}


/*
 * Differential Edge detector
 */
static gint
differential (guchar *data)
{
  gint    v_kernel[9] = { 0,  0,  0,
                          0,  2, -2,
                          0,  2, -2};
  gint    h_kernel[9] = { 0,  0,  0,
                          0, -2, -2,
                          0,  2,  2};


  gint i;
  gint    v_grad, h_grad;

  v_grad = 0;
  h_grad = 0;

  for (i = 0; i < 9; i++)
    {
      v_grad += v_kernel[i] * data[i];
      h_grad += h_kernel[i] * data[i];
    }

  return  CLAMP(sqrt (v_grad * v_grad * evals.amount +
                      h_grad * h_grad * evals.amount), 0, 255);
}


/*
 * Laplace Edge detector
 */
static gint
laplace (guchar *data)
{
  gint    kernel[9] = { 1,  1,  1,
                        1, -8,  1,
                        1,  1,  1};

  gint i;
  gint    grad;

  grad = 0;

  for (i = 0; i < 9; i++)
    {
      grad += kernel[i] * data[i];
    }

  return  CLAMP(grad * evals.amount, 0, 255);

}


/*******************************************************/
/*                    Dialog                           */
/*******************************************************/

static gboolean
edge_dialog (GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *hbox;
  GtkWidget *toggle;
  GtkObject *scale_data;
  GSList    *group = NULL;
  gboolean   run;

  gboolean use_wrap  = (evals.wrapmode == GIMP_PIXEL_FETCHER_EDGE_WRAP);
  gboolean use_smear = (evals.wrapmode == GIMP_PIXEL_FETCHER_EDGE_SMEAR);
  gboolean use_black = (evals.wrapmode == GIMP_PIXEL_FETCHER_EDGE_BLACK);

  gimp_ui_init ("edge", FALSE);

  dlg = gimp_dialog_new (_("Edge Detection"), "edge",
                         NULL, 0,
                         gimp_standard_help_func, "plug-in-edge",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox);
  gtk_widget_show (vbox);

  /*  compression  */
  frame = gimp_int_radio_group_new (TRUE, _("Algorithm"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &evals.edgemode, evals.edgemode,

                                    _("_Sobel"),        SOBEL,        NULL,
                                    _("_Prewitt"),      PREWITT,      NULL,
                                    _("_Gradient"),     GRADIENT,     NULL,
                                    _("_Roberts"),      ROBERTS,      NULL,
                                    _("_Differential"), DIFFERENTIAL, NULL,
                                    _("_Laplace"),      LAPLACE,      NULL,

                                    NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  parameter settings  */

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  Label, scale, entry for evals.amount  */
  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                     _("_Amount:"), 100, 0,
                                     evals.amount, 1.0, 10.0, 0.1, 1.0, 1,
                                     TRUE, 0, 0,
                                     NULL, NULL);

  g_signal_connect (scale_data, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &evals.amount);

  /*  Radio buttons WRAP, SMEAR, BLACK  */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 3, 1, 2,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  toggle = gtk_radio_button_new_with_mnemonic (group, _("_Wrap"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), use_wrap);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &use_wrap);

  toggle = gtk_radio_button_new_with_mnemonic (group, _("_Smear"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), use_smear);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &use_smear);

  toggle = gtk_radio_button_new_with_mnemonic (group, _("_Black"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), use_black);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &use_black);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  if (use_wrap)
    evals.wrapmode = GIMP_PIXEL_FETCHER_EDGE_WRAP;
  else if (use_smear)
    evals.wrapmode = GIMP_PIXEL_FETCHER_EDGE_SMEAR;
  else if (use_black)
    evals.wrapmode = GIMP_PIXEL_FETCHER_EDGE_BLACK;

  return run;
}
