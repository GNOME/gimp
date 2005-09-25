/* Convolution Matrix plug-in for the GIMP -- Version 0.1
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
 * v0.13	15.12.2000
 * 	Made the PDB interface actually work.   (Simon Budig <simon@gimp.org>)
 *
 * v0.12	15.9.1997
 *	Got rid of the unportable snprintf. Also made some _tiny_ GUI fixes.
 *
 * v0.11	20.7.1997
 *	Negative values in the matrix are now abs'ed when used to weight
 *      alpha. Embossing effects should work properly now. Also fixed a
 *      totally idiotic bug with embossing.
 *
 * v0.1 	2.7.1997
 *	Initial release. Works... kinda.
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

static gboolean  convmatrix_dialog (GimpDrawable  *drawable);

static void      convmatrix        (GimpDrawable  *drawable,
                                    GimpPreview   *preview);
static void      check_config      (GimpDrawable  *drawable);

static gfloat    calcmatrix        (guchar       **srcrow,
                                    gint           xoff,
                                    gint           i,
                                    GimpDrawable  *drawable);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,   /* quit_proc  */
  query,  /* query_proc */
  run,    /* run_proc   */
};

static gboolean run_flag = FALSE;

typedef struct
{
  gfloat     matrix[5][5];
  gfloat     divisor;
  gfloat     offset;
  gint       alpha_alg;
  BorderMode bmode;
  gboolean   channels[5];
  gboolean   autoset;
} config;

static const config default_config =
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
  FALSE              /* autoset */
};

static config my_config;

struct
{
  GtkWidget *matrix[5][5];
  GtkWidget *divisor;
  GtkWidget *offset;
  GtkWidget *alpha_alg;
  GtkWidget *bmode[3];
  GtkWidget *channels[5];
  GtkWidget *autoset;
} my_widgets;


MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
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
    { GIMP_PDB_INT32,      "bmode",         "Mode for treating image borders" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          "A generic 5x5 convolution matrix",
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

  /*  The plug-in is not able to handle images smaller than 3 pixels  */
  if (drawable->width < 3 || drawable->height < 3)
    {
      g_message (_("Convolution Matrix does not work on layers "
                   "smaller than 3 pixels."));
      status = GIMP_PDB_EXECUTION_ERROR;
      values[0].type = GIMP_PDB_STATUS;
      values[0].data.d_status = status;
      return;
    }

  my_config = default_config;
  if (run_mode == GIMP_RUN_NONINTERACTIVE)
    {
      if (nparams != 11)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      else
        {
          if (param[3].data.d_int32 != 25)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              for (y = 0; y < 5; y++)
                for (x = 0; x < 5; x++)
                  my_config.matrix[x][y]=param[4].data.d_floatarray[y*5+x];
            }

          my_config.alpha_alg = param[5].data.d_int32;
          my_config.divisor   = param[6].data.d_float;
          my_config.offset    = param[7].data.d_float;


          if (param[8].data.d_int32 != 5)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              for (y = 0; y < 5; y++)
                my_config.channels[y] = param[9].data.d_int32array[y];
            }

          my_config.bmode     = param[10].data.d_int32;

          check_config (drawable);
        }
    }
  else
    {
      gimp_get_data (PLUG_IN_PROC, &my_config);

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          /*  Oh boy. We get to do a dialog box, because we can't really
           *  expect the user to set us up with the right values using gdb.
           */
          check_config (drawable);

          if (! convmatrix_dialog (drawable))
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
          convmatrix (drawable, NULL);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC, &my_config, sizeof (my_config));
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
 *  wrapping or gives you transparent regions outside the image
 */

static void
my_get_row (GimpPixelRgn *PR,
            guchar       *dest,
            gint          x,
            gint          y,
            gint          w)
{
  gint width, height, bytes;
  gint i;

  width  = PR->drawable->width;
  height = PR->drawable->height;
  bytes  = PR->drawable->bpp;

  /* Y-wrappings */

  switch (my_config.bmode)
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
          memset (dest, 0, w * bytes);
          return; /* Done, so back. */
        }

    case EXTEND:
      y = CLAMP (y , 0 , height - 1);
      break;
    }

  switch (my_config.bmode)
    {
    case CLEAR:
      if (x < 0)
        {
          i = MIN (w, -x);
          memset (dest, 0, i * bytes);
          dest += i * bytes;
          w -= i;
          x += i;
        }
      if (w)
        {
          i = MIN (w, width);
          gimp_pixel_rgn_get_row (PR, dest, x, y, i);
          dest += i * bytes;
          w -= i;
          x += i;
        }
      if (w)
        memset (dest, 0, w * bytes);
      break;

    case WRAP:
      while (x < 0)
        x += width;
      i = MIN (w, width - x);
      gimp_pixel_rgn_get_row (PR, dest, x, y, i);
      w -= i;
      dest += i * bytes;
      x = 0;
      while (w)
        {
          i = MIN (w, width);
          gimp_pixel_rgn_get_row (PR, dest, x, y, i);
          w -= i;
          dest += i * bytes;
        }
      break;

    case EXTEND:
      if (x < 0)
        {
          gimp_pixel_rgn_get_pixel (PR, dest, 0, y);
          x++;
          w--;
          dest += bytes;

          while (x < 0 && w)
            {
              for (i = 0; i < bytes; i++)
                {
                  *dest = *(dest - bytes);
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
          dest += i * bytes;
        }
      while (w)
        {
          for (i = 0; i < bytes; i++)
            {
              *dest= *(dest - bytes);
              dest++;
            }
          x++;
          w--;
        }
      break;

    }
}

static gfloat
calcmatrix (guchar       **srcrow,
            gint           xoff,
            gint           i,
            GimpDrawable  *drawable)
{
  static gfloat matrixsum = 0; /* FIXME: this certainly breaks the preview */
  static gint bytes       = 0;

  gfloat sum      = 0;
  gfloat alphasum = 0;
  gfloat temp;
  gint   x, y;

  if (!bytes)
    {
      bytes = drawable->bpp;
      for (y = 0; y < 5; y++)
        for (x = 0; x < 5; x++)
          {
            temp = my_config.matrix[x][y];
            matrixsum += ABS (temp);
          }
    }
  for (y = 0; y < 5; y++)
    for (x = 0; x < 5; x++)
      {
        temp = my_config.matrix[x][y];
        if (i != (bytes - 1) && my_config.alpha_alg == 1)
          {
            temp *= srcrow[y][xoff + x * bytes +bytes - 1 - i];
            alphasum += ABS (temp);
          }
        temp *= srcrow[y][xoff + x * bytes];
        sum += temp;
      }
  sum /= my_config.divisor;
  if (i != (bytes - 1) && my_config.alpha_alg == 1)
    {
      if (alphasum != 0)
        sum = sum * matrixsum / alphasum;
      else
        sum = 0;
      /* sum = srcrow[2][xoff + 2 * bytes] * my_config.matrix[2][2];*/
    }
  sum += my_config.offset;

  return sum;
}

static void
convmatrix (GimpDrawable *drawable,
            GimpPreview  *preview)
{
  GimpPixelRgn  srcPR, destPR;
  gint          width, height, row, col;
  gint          w, h, i;
  gint          sx1, sy1, sx2, sy2;
  gint          x1, x2, y1, y2;
  guchar       *destrow[3];
  guchar       *srcrow[5];
  guchar       *temprow;
  gfloat        sum;
  gint          xoff;
  gboolean      chanmask[4];
  gint          bytes;

  /* Get the input area. This is the bounding box of the selection in
   *  the image (or the entire image if there is no selection). Only
   *  operating on the input area is simply an optimization. It doesn't
   *  need to be done for correct operation. (It simply makes it go
   *  faster, since fewer pixels need to be operated on).
   */
  if (preview)
    {
      gimp_preview_get_position (preview, &sx1, &sy1);
      gimp_preview_get_size (preview, &w, &h);
      sx2 = sx1 + w;
      sy2 = sy1 + h;
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id, &sx1, &sy1, &sx2, &sy2);
      w = sx2 - sx1;
      h = sy2 - sy1;
    }

  /* Get the size of the input image. (This will/must be the same
   *  as the size of the output image.
   */
  width  = drawable->width;
  height = drawable->height;
  bytes  = drawable->bpp;

  if (gimp_drawable_is_rgb (drawable->drawable_id))
    for (i = 0; i <3; i++)
      chanmask[i] = my_config.channels[i + 1];
  else /* Grayscale */
    chanmask[0] = my_config.channels[0];

  if (gimp_drawable_has_alpha (drawable->drawable_id))
    chanmask[bytes - 1] = my_config.channels[4];

  for (i = 0; i < 5; i++)
    srcrow[i] = g_new (guchar, (w + 4) * bytes);
  for (i = 0; i < 3; i++)
    destrow[i]= g_new (guchar, w * bytes);

  /*  initialize the pixel regions  */
  x1 = MAX (sx1 - 2, 0);
  y1 = MAX (sy1 - 2, 0);
  x2 = MIN (sx2 + 2, width);
  y2 = MIN (sy2 + 2, height);
  gimp_pixel_rgn_init (&srcPR, drawable,
                       x1, y1, x2 - x1, y2 - y1, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable,
                       sx1, sy1, w, h,
                       preview == NULL, TRUE);

  /* initialize source arrays */
  for (i = 0; i < 5; i++)
    my_get_row (&srcPR, srcrow[i], sx1 - 2, sy1 + i - 2, w + 4);

  for (row = sy1; row < sy2; row++)
    {
      xoff = 0;

      for (col = sx1; col < sx2; col++)
        for (i = 0; i < bytes; i++)
          {
            if (chanmask[i])
              sum = calcmatrix(srcrow, xoff, i, drawable);
            else
              sum = srcrow[2][xoff + 2 * bytes];

            destrow[2][xoff]= (guchar) CLAMP (sum, 0, 255);
            xoff++;
          }

      if (row > sy1 + 1)
        gimp_pixel_rgn_set_row (&destPR, destrow[0], sx1, row - 2, w);

      temprow = destrow[0];
      destrow[0] = destrow[1];
      destrow[1] = destrow[2];
      destrow[2] = temprow;
      temprow = srcrow[0];

      for (i = 0; i < 4; i++)
        srcrow[i] = srcrow[i + 1];

      srcrow[4] = temprow;
      my_get_row (&srcPR, srcrow[4], sx1 - 2, row + 3, w + 4);

      if ((row % 10 == 0) && !preview)
        gimp_progress_update ((double) (row - sy1) / h);
    }

  /* put the final rows in the buffer in place */
  if (h < 3)
    gimp_pixel_rgn_set_row (&destPR, destrow[2], sx1, row - 3, w);
  if (h > 1)
    gimp_pixel_rgn_set_row (&destPR, destrow[0], sx1, row - 2, w);
  if (h > 2)
    gimp_pixel_rgn_set_row (&destPR, destrow[1], sx1, row - 1, w);

  /*  update the timred region  */
  if (preview)
    {
      gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),
                                         &destPR);
    }
  else
    {
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, sx1, sy1, sx2 - sx1, sy2 - sy1);
    }
  for (i = 0; i < 5; i++)
    g_free (srcrow[i]);
  for (i = 0; i < 3; i++)
    g_free (destrow[i]);
}

/***************************************************
 * GUI stuff
 */

static void
fprint (gfloat  f,
        gchar  *buffer,
        gsize   len)
{
  guint i, t;

  g_snprintf (buffer, len, "%.7f", f);
  buffer[len - 1] = '\0';

  for (t = 0; t < len - 1 && buffer[t] != '.'; t++)
      ;

  i = t + 1;

  while (buffer[i] != '\0')
    {
      if (buffer[i] != '0')
        t = i + 1;

      i++;
    }

  buffer[t] = '\0';
}

static void
redraw_matrix (void)
{
  gint  x, y;
  gchar buffer[12];

  for (y = 0; y < 5; y++)
    for (x = 0; x < 5; x++)
      {
        fprint (my_config.matrix[x][y], buffer, sizeof (buffer));
        gtk_entry_set_text (GTK_ENTRY (my_widgets.matrix[x][y]), buffer);
      }
}

static void
redraw_channels (void)
{
  gint i;

  for (i = 0; i < 5; i++)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (my_widgets.channels[i]),
                                  my_config.channels[i]);
}

static void
redraw_autoset (void)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (my_widgets.autoset),
                                my_config.autoset);
}

static void
redraw_alpha_alg (void)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (my_widgets.alpha_alg),
                                my_config.alpha_alg > 0);
}

static void
redraw_off_and_div (void)
{
  gchar buffer[12];

  fprint (my_config.divisor, buffer, sizeof (buffer));
  gtk_entry_set_text (GTK_ENTRY (my_widgets.divisor), buffer);

  fprint (my_config.offset, buffer, sizeof (buffer));
  gtk_entry_set_text (GTK_ENTRY (my_widgets.offset), buffer);
}

static void
redraw_bmode (void)
{
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (my_widgets.bmode[my_config.bmode]), TRUE);
}

static void
redraw_all (void)
{
  redraw_matrix ();
  redraw_off_and_div ();
  redraw_autoset ();
  redraw_alpha_alg ();
  redraw_bmode ();
  redraw_channels ();
}

static void
check_matrix (void)
{
  gint      x, y;
  gboolean  valid = FALSE;
  gfloat    sum   = 0.0;

  for (y = 0; y < 5; y++)
    for (x = 0; x < 5; x++)
      {
        sum += my_config.matrix[x][y];
        if (my_config.matrix[x][y] != 0.0)
          valid = TRUE;
      }

  if (my_config.autoset)
    {
      if (sum > 0)
        {
          my_config.offset = 0;
          my_config.divisor = sum;
        }
      else if (sum < 0)
        {
          my_config.offset = 255;
          my_config.divisor = -sum;
        }
      else
        {
          my_config.offset = 128;
          /* The sum is 0, so this is probably some sort of
           * embossing filter. Should divisor be autoset to 1
           * or left undefined, ie. for the user to define? */
          my_config.divisor = 1;
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
      my_config = default_config;
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
  gint i;

  for (i = 0; i < 5; i++)
    my_config.channels[i] = FALSE;
  my_config.alpha_alg = 0;

  if (!gimp_drawable_has_alpha (drawable->drawable_id))
    {
      my_config.alpha_alg   = -1;
      my_config.bmode       = EXTEND;
    }
}

static void
entry_callback (GtkWidget *widget,
                gpointer   data)
{
  gfloat *value = (gfloat *) data;

  *value = atof (gtk_entry_get_text (GTK_ENTRY (widget)));

  if (widget == my_widgets.divisor)
    gtk_dialog_set_response_sensitive (GTK_DIALOG (gtk_widget_get_toplevel (widget)),
                                       GTK_RESPONSE_OK, (*value != 0.0));
  else if (widget != my_widgets.offset)
    check_matrix ();
}

static void
my_toggle_callback (GtkWidget *widget,
                    gboolean  *data)
{
  gint val = GTK_TOGGLE_BUTTON (widget)->active;

  *data = val;

  if (widget == my_widgets.alpha_alg)
    {
      gtk_widget_set_sensitive (my_widgets.bmode[CLEAR], val);
      if (val == 0 && my_config.bmode == CLEAR)
        {
          my_config.bmode = EXTEND;
          redraw_bmode ();
        }
    }
  else if (widget == my_widgets.autoset)
    {
      gtk_widget_set_sensitive (my_widgets.divisor, !val);
      gtk_widget_set_sensitive (my_widgets.offset, !val);
      check_matrix ();
    }
}

static void
my_bmode_callback (GtkWidget *widget,
                   gpointer   data)
{
  my_config.bmode = GPOINTER_TO_INT (data) - 1;
}

static gboolean
convmatrix_dialog (GimpDrawable *drawable)
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
                            G_CALLBACK (convmatrix),
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

  table = gtk_table_new (5, 5, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (inbox), table, TRUE, TRUE, 0);

  for (y = 0; y < 5; y++)
    for (x = 0; x < 5; x++)
      {
        my_widgets.matrix[x][y] = entry = gtk_entry_new ();
        gtk_widget_set_size_request (entry, 40, -1);
        gtk_table_attach (GTK_TABLE (table), entry, x, x+1, y, y+1,
                          GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
        gtk_widget_show (entry);

        g_signal_connect (entry, "changed",
                          G_CALLBACK (entry_callback),
                          &my_config.matrix[x][y]);
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

  my_widgets.divisor = entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 40, -1);
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 0, 1);
  gtk_widget_show (entry);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (entry_callback),
                    &my_config.divisor);
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

  my_widgets.offset = entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 40, -1);
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 0, 1);
  gtk_widget_show (entry);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (entry_callback),
                    &my_config.offset);
  g_signal_connect_swapped (entry, "changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_widget_show (table);

  gtk_widget_show (box);

  gtk_widget_show (inbox);
  gtk_widget_show (frame);

  box = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);

  my_widgets.autoset = button =
    gtk_check_button_new_with_mnemonic (_("A_utomatic"));
  gtk_box_pack_start (GTK_BOX (box), button, TRUE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (my_toggle_callback),
                    &my_config.autoset);
  g_signal_connect_swapped (button, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  my_widgets.alpha_alg = button =
    gtk_check_button_new_with_mnemonic (_("A_lpha-weighting"));
  if (my_config.alpha_alg == -1)
    gtk_widget_set_sensitive (button, FALSE);
  gtk_box_pack_start (GTK_BOX (box), button, TRUE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (my_toggle_callback),
                    &my_config.alpha_alg);
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

  for (i = 0; i < 3; i++)
    {
      my_widgets.bmode[i] = button =
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

  for (i = 0; i < 5; i++)
    {
      if ((gimp_drawable_is_gray (drawable->drawable_id) && i==0) ||
          (gimp_drawable_is_rgb  (drawable->drawable_id) && i>=1 && i<=3) ||
          (gimp_drawable_has_alpha (drawable->drawable_id) && i==4))
        {
          my_widgets.channels[i] = button =
            gtk_check_button_new_with_mnemonic (gettext (channel_labels[i]));

          gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
          gtk_widget_show (button);

          g_signal_connect (button, "toggled",
                            G_CALLBACK (my_toggle_callback),
                            &my_config.channels[i]);
          g_signal_connect_swapped (button, "toggled",
                                    G_CALLBACK (gimp_preview_invalidate),
                                    preview);
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

  gtk_widget_set_sensitive (my_widgets.bmode[CLEAR],
                            (my_config.alpha_alg > 0));

  gtk_main ();

  return run_flag;
}
