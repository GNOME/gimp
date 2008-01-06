/* Convolution Matrix plug-in for GIMP -- Version 0.1
 * Copyright (C) 1997 Lauri Alanko <la@iki.fi>
 *
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
 * The GNU General Public License is also available from
 * http://www.fsf.org/copyleft/gpl.html
 *
 *
 * CHANGELOG:
 * v0.14        21.09.2006                   (gg <gg at catking the net>)
 *      Replace numerical consts by named const, much variable renaming for maintainability
 *      Generalisation of code w.r.t. matrix dimension with aim to support 7x7
 *      Some minor bug fixes.
 *
 * v0.13        15.12.2000
 *      Made the PDB interface actually work.   (Simon Budig <simon@gimp.org>)
 *
 * v0.12        15.9.1997
 *      Got rid of the unportable snprintf. Also made some _tiny_ GUI fixes.
 *
 * v0.11        20.7.1997
 *      Negative values in the matrix are now abs'ed when used to weight
 *      alpha. Embossing effects should work properly now. Also fixed a
 *      totally idiotic bug with embossing.
 *
 * v0.1         2.7.1997
 *      Initial release. Works... kinda.
 *
 *
 * TODO:
 *
 * - remove channels selector (that's what the channels dialog is for)
 * - remove idiotic slowdowns
 * - clean up code
 * - optimize properly
 * - save & load matrices
 * - spiffy frontend for designing matrices
 *
 * What else?
 *
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-convmatrix"
#define PLUG_IN_BINARY "convmatrix"
#define RESPONSE_RESET 1


#define BIG_MATRIX  /* toggle for 11x11 matrix code experimental*/
#undef BIG_MATRIX


#ifndef BIG_MATRIX
#define MATRIX_SIZE   (5)
#else
#define MATRIX_SIZE   (11)
#endif

#define HALF_WINDOW   (MATRIX_SIZE/2)
#define MATRIX_CELLS  (MATRIX_SIZE*MATRIX_SIZE)
#define DEST_ROWS     (MATRIX_SIZE/2 + 1)
#define CHANNELS      (5)
#define BORDER_MODES  (3)



typedef enum
{
  EXTEND,
  WRAP,
  CLEAR
} BorderMode;

static gchar * const channel_labels[] =
{
  N_("Gr_ey"),
  N_("Re_d"),
  N_("_Green"),
  N_("_Blue"),
  N_("_Alpha")
};

static gchar * const bmode_labels[] =
{
  N_("E_xtend"),
  N_("_Wrap"),
  N_("Cro_p")
};

/* Declare local functions. */
static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

static gboolean  convolve_image_dialog (GimpDrawable  *drawable);

static void      convolve_image        (GimpDrawable  *drawable,
                                        GimpPreview   *preview);

static void      check_config          (GimpDrawable  *drawable);

static gfloat    convolve_pixel        (guchar       **src_row,
                                        gint           x_offset,
                                        gint           channel,
                                        GimpDrawable  *drawable);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,   /* quit_proc  */
  query,  /* query_proc */
  run,    /* run_proc   */
};

static gboolean run_flag = FALSE;

typedef struct
{
  gfloat     matrix[MATRIX_SIZE][MATRIX_SIZE];
  gfloat     divisor;
  gfloat     offset;
  gint       alpha_weighting;
  BorderMode bmode;
  gboolean   channels[CHANNELS];
  gboolean   autoset;
} config_struct;

#ifndef BIG_MATRIX
static const config_struct default_config =
{
  {
    { 0.0, 0.0, 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 1.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.0, 0.0, 0.0 }
  },                 /* matrix */
  1,                 /* divisor */
  0,                 /* offset */
  1,                 /* Alpha-handling algorithm */
  CLEAR,             /* border-mode */
  { TRUE, TRUE, TRUE, TRUE, TRUE }, /* Channels mask */
  FALSE,              /* autoset */
};

#else

static const config_struct default_config =
{
  {
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0  },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0  },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0  },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0  },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0  },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0  },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0  },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0  },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0  },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0  },
    { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0  }
  },                 /* matrix */
  1,                 /* divisor */
  0,                 /* offset */
  1,                 /* Alpha-handling algorithm */
  CLEAR,             /* border-mode */
  { TRUE, TRUE, TRUE, TRUE, TRUE }, /* Channels mask */
  FALSE,              /* autoset */
};

#endif


static config_struct config;

struct
{
  GtkWidget *matrix[MATRIX_SIZE][MATRIX_SIZE];
  GtkWidget *divisor;
  GtkWidget *offset;
  GtkWidget *alpha_weighting;
  GtkWidget *bmode[BORDER_MODES];
  GtkWidget *channels[CHANNELS];
  GtkWidget *autoset;
} widget_set;


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,      "run-mode",    "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,      "image",       "Input image (unused)" },
    { GIMP_PDB_DRAWABLE,   "drawable",    "Input drawable" },
    { GIMP_PDB_INT32,      "argc-matrix", "The number of elements in the following array. Should be always 25." },
    { GIMP_PDB_FLOATARRAY, "matrix",      "The 5x5 convolution matrix" },
    { GIMP_PDB_INT32,      "alpha-alg",   "Enable weighting by alpha channel" },
    { GIMP_PDB_FLOAT,      "divisor",     "Divisor" },
    { GIMP_PDB_FLOAT,      "offset",      "Offset" },

    { GIMP_PDB_INT32,      "argc-channels", "The number of elements in following array. Should be always 5." },
    { GIMP_PDB_INT32ARRAY, "channels",      "Mask of the channels to be filtered" },
    { GIMP_PDB_INT32,      "bmode",         "Mode for treating image borders" },
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Apply a generic 5x5 convolution matrix"),
                          "",
                          "Lauri Alanko",
                          "Lauri Alanko",
                          "1997",
                          N_("_Convolution Matrix..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Generic");
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
  gint               x, y;
  GimpDrawable      *drawable;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals = values;

  run_mode = param[0].data.d_int32;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  The plug-in is not able to handle images smaller than 3x3 pixels  */
  if (drawable->width < 3 || drawable->height < 3)
    {
      g_message (_("Convolution does not work on layers "
                   "smaller than 3x3 pixels."));
      status = GIMP_PDB_EXECUTION_ERROR;
      values[0].type = GIMP_PDB_STATUS;
      values[0].data.d_status = status;
      return;
    }

  config = default_config;
  if (run_mode == GIMP_RUN_NONINTERACTIVE)
    {
      if ((nparams != 11) && (nparams != 12))
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          if (param[3].data.d_int32 != MATRIX_CELLS)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              for (y = 0; y < MATRIX_SIZE; y++)
                for (x = 0; x < MATRIX_SIZE; x++)
                  config.matrix[x][y]
                    = param[4].data.d_floatarray[x * MATRIX_SIZE + y];
            }

          config.alpha_weighting = param[5].data.d_int32;
          config.divisor   = param[6].data.d_float;
          config.offset    = param[7].data.d_float;

          if (param[8].data.d_int32 != CHANNELS)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              for (y = 0; y < CHANNELS; y++)
                config.channels[y] = param[9].data.d_int32array[y];
            }

          config.bmode     = param[10].data.d_int32;

          check_config (drawable);
        }
    }
  else
    {
      gimp_get_data (PLUG_IN_PROC, &config);

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          /*  Oh boy. We get to do a dialog box, because we can't really
           *  expect the user to set us up with the right values using gdb.
           */
          check_config (drawable);

          if (! convolve_image_dialog (drawable))
            {
              /* The dialog was closed, or something similarly evil happened. */
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id) ||
          gimp_drawable_is_gray (drawable->drawable_id))
        {
          gimp_progress_init (_("Applying convolution"));
          gimp_tile_cache_ntiles (2 * (drawable->width /
                                  gimp_tile_width () + 1));
          convolve_image (drawable, NULL);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC, &config, sizeof (config));
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }

      gimp_drawable_detach (drawable);
    }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}


/*  A generic wrapper to gimp_pixel_rgn_get_row which handles unlimited
 *  wrapping or gives the transparent regions outside the image
 *  fills additional bytes before and after image row to provide border modes.
 */

static void
my_get_row (GimpPixelRgn *PR,
            guchar       *dest,
            gint          x,
            gint          y,
            gint          w)
{
  gint width, height, bpp;
  gint i;

  width  = PR->drawable->width;
  height = PR->drawable->height;
  bpp  = PR->drawable->bpp;

  /* Y-wrappings */
  switch (config.bmode)
    {
    case WRAP:
      /* Wrapped, so we get the proper row from the other side */
      while (y < 0) /* This is the _sure_ way to wrap. :) */
        y += height;
      while (y >= height)
        y -= height;
      break;

    case CLEAR:
      /* Beyond borders, so set full transparent. */
      if (y < 0 || y >= height)
        {
          memset (dest, 0, w * bpp);
          return; /* Done, so back. */
        }

    case EXTEND:
      y = CLAMP (y , 0 , height - 1);
      break;
    }


  /* X-wrappings */
  switch (config.bmode)
    {
    case CLEAR:
      if (x < 0)
        {
          i = MIN (w, -x);
          memset (dest, 0, i * bpp);
          dest += i * bpp;
          w -= i;
          x += i;
        }
      if (w)
        {
          i = MIN (w, width);
          gimp_pixel_rgn_get_row (PR, dest, x, y, i);
          dest += i * bpp;
          w -= i;
          x += i;
        }
      if (w)
        memset (dest, 0, w * bpp);
      break;

    case WRAP:
      while (x < 0)
        x += width;
      i = MIN (w, width - x);
      gimp_pixel_rgn_get_row (PR, dest, x, y, i);
      w -= i;
      dest += i * bpp;
      x = 0;
      while (w)
        {
          i = MIN (w, width);
          gimp_pixel_rgn_get_row (PR, dest, x, y, i);
          w -= i;
          dest += i * bpp;
        }
      break;

    case EXTEND:
      if (x < 0)
        {
          gimp_pixel_rgn_get_pixel (PR, dest, 0, y);
          x++;
          w--;
          dest += bpp;

          while (x < 0 && w)
            {
              for (i = 0; i < bpp; i++)
                {
                  *dest = *(dest - bpp);
                  dest++;
                }
              x++;
              w--;
            }
        }
      if (w && width - x > 0)
        {
          i = MIN (w, width - x);
          gimp_pixel_rgn_get_row (PR, dest, x, y, i);
          w -= i;
          dest += i * bpp;
        }
      while (w)
        {
          for (i = 0; i < bpp; i++)
            {
              *dest= *(dest - bpp);
              dest++;
            }
          x++;
          w--;
        }
      break;

    }
}

static gfloat
convolve_pixel (guchar       **src_row,
                gint           x_offset,
                gint           channel,
                GimpDrawable  *drawable)
{
  static gfloat matrixsum = 0; /* FIXME: this certainly breaks the preview */
  static gint bpp         = 0;

  gfloat sum              = 0;
  gfloat alphasum         = 0;
  gfloat temp;
  gint   x, y;
  gint   alpha_channel;

  if (!bpp)
    {
      bpp = drawable->bpp;

      for (y = 0; y < MATRIX_SIZE; y++)
        for (x = 0; x < MATRIX_SIZE; x++)
          {
            temp = config.matrix[x][y];
            matrixsum += ABS (config.matrix[x][y]);
          }
    }

  alpha_channel = bpp - 1;

  for (y = 0; y < MATRIX_SIZE; y++)
    for (x = 0; x < MATRIX_SIZE; x++)
      {
        temp = config.matrix[x][y];

        if (channel != alpha_channel && config.alpha_weighting == 1)
          {
            temp *= src_row[y][x_offset + x * bpp + alpha_channel - channel];
            alphasum += ABS (temp);
          }

        temp *= src_row[y][x_offset + x * bpp];
        sum += temp;
      }

  sum /= config.divisor;

  if (channel != alpha_channel && config.alpha_weighting == 1)
    {
      if (alphasum != 0)
        sum = sum * matrixsum / alphasum;
      else
        sum = 0;
    }

  sum += config.offset;

  return sum;
}

static void
convolve_image (GimpDrawable *drawable,
                GimpPreview  *preview)
{
  GimpPixelRgn  srcPR, destPR;
  gint          width, height, row, col;
  gint          src_w, src_row_w, src_h, i;
  gint          src_x1, src_y1, src_x2, src_y2;
  gint          x1, x2, y1, y2;
  guchar       *dest_row[DEST_ROWS];
  guchar       *src_row[MATRIX_SIZE];
  guchar       *tmp_row;
  gfloat        sum;
  gint          x_offset;
  gboolean      chanmask[CHANNELS - 1];
  gint          bpp;
  gint          alpha_channel;

  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  if (preview)
    {
      gimp_preview_get_position (preview, &src_x1, &src_y1);
      gimp_preview_get_size (preview, &src_w, &src_h);
      src_x2 = src_x1 + src_w;
      src_y2 = src_y1 + src_h;
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id,
                                 &src_x1, &src_y1, &src_x2, &src_y2);
      src_w = src_x2 - src_x1;
      src_h = src_y2 - src_y1;
    }

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width  = drawable->width;
  height = drawable->height;
  bpp  = drawable->bpp;
  alpha_channel = bpp - 1;

  if (gimp_drawable_is_rgb (drawable->drawable_id))
    {
      for (i = 0; i < CHANNELS - 1; i++)
        chanmask[i] = config.channels[i + 1];
    }
  else /* Grayscale */
    {
      chanmask[0] = config.channels[0];
    }

  if (gimp_drawable_has_alpha (drawable->drawable_id))
    chanmask[alpha_channel] = config.channels[4];

  src_row_w = src_w + HALF_WINDOW + HALF_WINDOW;

  for (i = 0; i < MATRIX_SIZE; i++)
    src_row[i] = g_new (guchar, src_row_w * bpp);

  for (i = 0; i < DEST_ROWS; i++)
    dest_row[i]= g_new (guchar, src_w * bpp);

  /*  initialize the pixel regions  */
  x1 = MAX (src_x1 - HALF_WINDOW, 0);
  y1 = MAX (src_y1 - HALF_WINDOW, 0);
  x2 = MIN (src_x2 + HALF_WINDOW, width);
  y2 = MIN (src_y2 + HALF_WINDOW, height);
  gimp_pixel_rgn_init (&srcPR, drawable,
                       x1, y1, x2 - x1, y2 - y1, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable,
                       src_x1, src_y1, src_w, src_h,
                       preview == NULL, TRUE);

  /* initialize source arrays */
  for (i = 0; i < MATRIX_SIZE; i++)
    my_get_row (&srcPR, src_row[i], src_x1 - HALF_WINDOW,
                src_y1 - HALF_WINDOW + i , src_row_w);

  for (row = src_y1; row < src_y2; row++)
    {
      gint channel;

      x_offset = 0;

      for (col = src_x1; col < src_x2; col++)
        for (channel = 0; channel < bpp; channel++)
          {
            if (chanmask[channel])
              sum = convolve_pixel(src_row, x_offset, channel, drawable);
            else
              sum = src_row[HALF_WINDOW][x_offset + HALF_WINDOW * bpp];  /* copy unmodified px */

            dest_row[HALF_WINDOW][x_offset] = (guchar) CLAMP (sum, 0, 255);
            x_offset++;
          }

      if (row >= src_y1 + HALF_WINDOW)
        gimp_pixel_rgn_set_row (&destPR, dest_row[0], src_x1, row - HALF_WINDOW, src_w);

      if (row < src_y2 - 1)
        {
          tmp_row = dest_row[0];
          for (i = 0; i < DEST_ROWS - 1; i++)
            dest_row[i] = dest_row[i + 1];

          dest_row[DEST_ROWS - 1] = tmp_row;

          tmp_row = src_row[0];
          for (i = 0; i < MATRIX_SIZE - 1; i++)
            src_row[i] = src_row[i + 1];
          src_row[MATRIX_SIZE-1] = tmp_row;

          my_get_row (&srcPR, src_row[MATRIX_SIZE - 1],
                      src_x1 - HALF_WINDOW, row + HALF_WINDOW + 1, src_row_w);
        }

      if ((row % 10 == 0) && !preview)
        gimp_progress_update ((double) (row - src_y1) / src_h);
    }

  /* put the remaining rows in the buffer in place */
  for (i = 1; i <  DEST_ROWS; i++)
    gimp_pixel_rgn_set_row (&destPR, dest_row[i],
                            src_x1, src_y2 + i - 1 - HALF_WINDOW, src_w);


  /*  update the region  */
  if (preview)
    {
      gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),
                                         &destPR);
    }
  else
    {
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, src_x1, src_y1, src_x2 - src_x1, src_y2 - src_y1);
    }
  for (i = 0; i < MATRIX_SIZE; i++)
    g_free (src_row[i]);
  for (i = 0; i < DEST_ROWS; i++)
    g_free (dest_row[i]);
}

/***************************************************
 * GUI stuff
 */

static void
redraw_matrix (void)
{
  gint  x, y;
  gchar buffer[12];

  for (y = 0; y < MATRIX_SIZE; y++)
    for (x = 0; x < MATRIX_SIZE; x++)
      {
        g_snprintf (buffer, sizeof (buffer), "%g", config.matrix[x][y]);
        gtk_entry_set_text (GTK_ENTRY (widget_set.matrix[x][y]), buffer);
      }
}

static void
redraw_channels (void)
{
  gint i;

  for (i = 0; i < CHANNELS; i++)
    if (widget_set.channels[i])
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget_set.channels[i]),
                                    config.channels[i]);
}

static void
redraw_autoset (void)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget_set.autoset),
                                config.autoset);
}

static void
redraw_alpha_weighting (void)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget_set.alpha_weighting),
                                config.alpha_weighting > 0);
}

static void
redraw_off_and_div (void)
{
  gchar buffer[12];

  g_snprintf (buffer, sizeof (buffer), "%g", config.divisor);
  gtk_entry_set_text (GTK_ENTRY (widget_set.divisor), buffer);

  g_snprintf (buffer, sizeof (buffer), "%g", config.offset);
  gtk_entry_set_text (GTK_ENTRY (widget_set.offset), buffer);
}

static void
redraw_bmode (void)
{
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (widget_set.bmode[config.bmode]), TRUE);
}

static void
redraw_all (void)
{
  redraw_matrix ();
  redraw_off_and_div ();
  redraw_autoset ();
  redraw_alpha_weighting ();
  redraw_bmode ();
  redraw_channels ();
}

static void
check_matrix (void)
{
  gint      x, y;
  gboolean  valid = FALSE;
  gfloat    sum   = 0.0;

  for (y = 0; y < MATRIX_SIZE; y++)
    for (x = 0; x < MATRIX_SIZE; x++)
      {
        sum += config.matrix[x][y];
        if (config.matrix[x][y] != 0.0)
          valid = TRUE;
      }

  if (config.autoset)
    {
      if (sum > 0)
        {
          config.offset = 0;
          config.divisor = sum;
        }
      else if (sum < 0)
        {
          config.offset = 255;
          config.divisor = -sum;
        }
      else
        {
          config.offset = 128;
          /* The sum is 0, so this is probably some sort of
           * embossing filter. Should divisor be autoset to 1
           * or left undefined, ie. for the user to define? */
          config.divisor = 1;
        }
      redraw_off_and_div ();
    }
}

static void
response_callback (GtkWidget    *widget,
                   gint          response_id,
                   GimpDrawable *drawable)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      config = default_config;
      check_config (drawable);
      redraw_all ();
      break;

    case GTK_RESPONSE_OK:
      run_flag = TRUE;

    default:
      gtk_widget_destroy (GTK_WIDGET (widget));
      break;
    }
}

/* Checks that the configuration is valid for the image type */
static void
check_config (GimpDrawable *drawable)
{
  config.alpha_weighting = 0;

  if (!gimp_drawable_has_alpha (drawable->drawable_id))
    {
      config.alpha_weighting = -1;
      config.bmode           = EXTEND;
    }
}

static void
entry_callback (GtkWidget *widget,
                gpointer   data)
{
  gfloat *value = (gfloat *) data;

  *value = atof (gtk_entry_get_text (GTK_ENTRY (widget)));

  if (widget == widget_set.divisor)
    gtk_dialog_set_response_sensitive (GTK_DIALOG (gtk_widget_get_toplevel (widget)),
                                       GTK_RESPONSE_OK,
                                       (*value != 0.0));
  else if (widget != widget_set.offset)
    check_matrix ();
}


static void
my_toggle_callback (GtkWidget *widget,
                    gboolean  *data)
{
  gint val = GTK_TOGGLE_BUTTON (widget)->active;

  *data = val;

  if (widget == widget_set.alpha_weighting)
    {
      gtk_widget_set_sensitive (widget_set.bmode[CLEAR], val);
      if (val == 0 && config.bmode == CLEAR)
        {
          config.bmode = EXTEND;
          redraw_bmode ();
        }
    }
  else if (widget == widget_set.autoset)
    {
      gtk_widget_set_sensitive (widget_set.divisor, !val);
      gtk_widget_set_sensitive (widget_set.offset, !val);
      check_matrix ();
    }
}

static void
my_bmode_callback (GtkWidget *widget,
                   gpointer   data)
{
  config.bmode = GPOINTER_TO_INT (data) - 1;
}

static gboolean
convolve_image_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *main_hbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *button;
  GtkWidget *box;
  GtkWidget *inbox;
  GtkWidget *vbox;
  GtkWidget *frame;
  gint       x, y, i;
  GSList    *group;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Convolution Matrix"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GIMP_STOCK_RESET, RESPONSE_RESET,
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
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
                            G_CALLBACK (convolve_image),
                            drawable);

  main_hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_hbox, FALSE, FALSE, 0);
  gtk_widget_show (main_hbox),

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);

  frame = gimp_frame_new (_("Matrix"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  inbox = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (frame), inbox);

  table = gtk_table_new (MATRIX_SIZE, MATRIX_SIZE, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (inbox), table, TRUE, TRUE, 0);

  for (y = 0; y < MATRIX_SIZE; y++)
    for (x = 0; x < MATRIX_SIZE; x++)
      {
        widget_set.matrix[x][y] = entry = gtk_entry_new ();
        gtk_widget_set_size_request (entry, 40, -1);
        gtk_table_attach (GTK_TABLE (table), entry, x, x+1, y, y+1,
                          GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
        gtk_widget_show (entry);

        g_signal_connect (entry, "changed",
                          G_CALLBACK (entry_callback),
                          &config.matrix[x][y]);
        g_signal_connect_swapped (entry, "changed",
                                  G_CALLBACK (gimp_preview_invalidate),
                                  preview);
      }

  gtk_widget_show (table);

  box = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (inbox), box, FALSE, FALSE, 0);


  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (box), table, TRUE, FALSE, 0);

  label = gtk_label_new_with_mnemonic (_("D_ivisor:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_widget_show (label);

  widget_set.divisor = entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 40, -1);
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 0, 1);
  gtk_widget_show (entry);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (entry_callback),
                    &config.divisor);
  g_signal_connect_swapped (entry, "changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (table);



  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (box), table, TRUE, FALSE, 0);

  label = gtk_label_new_with_mnemonic (_("O_ffset:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_widget_show (label);

  widget_set.offset = entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 40, -1);
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 0, 1);
  gtk_widget_show (entry);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (entry_callback),
                    &config.offset);
  g_signal_connect_swapped (entry, "changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (table);

  gtk_widget_show (box);

  gtk_widget_show (inbox);
  gtk_widget_show (frame);

  box = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);

  widget_set.autoset = button =
    gtk_check_button_new_with_mnemonic (_("N_ormalise"));
  gtk_box_pack_start (GTK_BOX (box), button, TRUE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (my_toggle_callback),
                    &config.autoset);
  g_signal_connect_swapped (button, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  widget_set.alpha_weighting = button =
    gtk_check_button_new_with_mnemonic (_("A_lpha-weighting"));
  if (config.alpha_weighting == -1)
    gtk_widget_set_sensitive (button, FALSE);
  gtk_box_pack_start (GTK_BOX (box), button, TRUE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (my_toggle_callback),
                    &config.alpha_weighting);
  g_signal_connect_swapped (button, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (box);
  gtk_widget_show (vbox);

  inbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_hbox), inbox, FALSE, FALSE, 0);

  frame = gimp_frame_new (_("Border"));
  gtk_box_pack_start (GTK_BOX (inbox), frame, FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), box);

  group = NULL;

  for (i = 0; i < BORDER_MODES; i++)
    {
      widget_set.bmode[i] = button =
        gtk_radio_button_new_with_mnemonic (group, gettext (bmode_labels[i]));
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_signal_connect (button, "toggled",
                        G_CALLBACK (my_bmode_callback),
                        GINT_TO_POINTER (i + 1));
      g_signal_connect_swapped (button, "toggled",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);
    }

  gtk_widget_show (box);
  gtk_widget_show (frame);

  frame = gimp_frame_new (_("Channels"));
  gtk_box_pack_start (GTK_BOX (inbox), frame, FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), box);

  for (i = 0; i < CHANNELS; i++)
    {
      if ((gimp_drawable_is_gray (drawable->drawable_id) && i==0) ||
          (gimp_drawable_is_rgb  (drawable->drawable_id) && i>=1 && i<=3) ||
          (gimp_drawable_has_alpha (drawable->drawable_id) && i==4))
        {
          widget_set.channels[i] = button =
            gtk_check_button_new_with_mnemonic (gettext (channel_labels[i]));

          gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
          gtk_widget_show (button);

          g_signal_connect (button, "toggled",
                            G_CALLBACK (my_toggle_callback),
                            &config.channels[i]);
          g_signal_connect_swapped (button, "toggled",
                                    G_CALLBACK (gimp_preview_invalidate),
                                    preview);
        }
      else
        {
          widget_set.channels[i] = NULL;
        }
    }

  gtk_widget_show (box);
  gtk_widget_show (frame);

  gtk_widget_show (inbox);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (response_callback),
                    drawable);
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  gtk_widget_show (dialog);
  redraw_all ();

  gtk_widget_set_sensitive (widget_set.bmode[CLEAR],
                            (config.alpha_weighting > 0));

  gtk_main ();

  return run_flag;
}
