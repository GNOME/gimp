/*
 * This is a plug-in for the GIMP.
 *
 * Copyright (C) 1997 Brent Burton & the Edward Blevins
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

#include "config.h"

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define SPIN_BUTTON_WIDTH 8

/* Variables set in dialog box */
typedef struct data
{
  gint   mode;
  gint   size;
} CheckVals;

typedef struct
{
  gboolean run;
} CheckInterface;

static CheckInterface cint =
{
  FALSE
};

static void      query  (void);
static void      run    (const gchar       *name,
			 gint               nparams,
			 const GimpParam   *param,
			 gint              *nreturn_vals,
			 GimpParam        **return_vals);

static void      do_checkerboard_pattern    (GimpDrawable *drawable);
static gint      inblock                    (gint          pos,
                                             gint          size);

static gboolean	 do_checkerboard_dialog     (gint32        image_ID,
                                             GimpDrawable *drawable);
static void      check_ok_callback          (GtkWidget    *widget,
                                             gpointer      data);
static void      check_size_update_callback (GtkWidget    *widget,
                                             gpointer       data);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static CheckVals cvals =
{
  0,
  10
};

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_INT32, "check_mode", "Regular or Psychobilly" },
    { GIMP_PDB_INT32, "check_size", "Size of the checks" }
  };

  gimp_install_procedure ("plug_in_checkerboard",
			  "Adds a checkerboard pattern to an image",
			  "More here later",
			  "Brent Burton & the Edward Blevins",
			  "Brent Burton & the Edward Blevins",
			  "1997",
			  N_("<Image>/Filters/Render/Pattern/_Checkerboard..."),
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
  gint32             image_ID;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;
  image_ID = param[1].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data ("plug_in_checkerboard", &cvals);
      if (! do_checkerboard_dialog(image_ID, drawable))
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 5)
	status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS)
	{
	  cvals.mode = param[3].data.d_int32;
	  cvals.size = param[4].data.d_int32;
	}
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data ("plug_in_checkerboard", &cvals);
      break;

    default:
      break;
    }

  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id))
    {
      gimp_progress_init (_("Adding Checkerboard..."));

      do_checkerboard_pattern(drawable);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      if (run_mode == GIMP_RUN_INTERACTIVE)
	gimp_set_data ("plug_in_checkerboard", &cvals, sizeof (CheckVals));
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}


static void
do_checkerboard_pattern (GimpDrawable *drawable)
{
  GimpPixelRgn dest_rgn;
  guchar   *dest_row;
  guchar   *dest;
  gint      row, col;
  gint      progress, max_progress;
  gint      x1, y1, x2, y2, x, y;
  GimpRGB   foreground;
  GimpRGB   background;
  guchar    fg[4];
  guchar    bg[4];
  gint      bp;
  gpointer  pr;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
  gimp_pixel_rgn_init (&dest_rgn, drawable,
		       x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  progress     = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  /* Get the foreground and background colors */

  gimp_palette_get_foreground (&foreground);
  gimp_palette_get_background (&background);

  switch (gimp_drawable_type (drawable->drawable_id))
    {
    case GIMP_RGBA_IMAGE:
      fg[3] = 255;
      bg[3] = 255;
    case GIMP_RGB_IMAGE:
      gimp_rgb_get_uchar (&foreground, &fg[0], &fg[1], &fg[2]);
      gimp_rgb_get_uchar (&background, &bg[0], &bg[1], &bg[2]);
      break;

    case GIMP_GRAYA_IMAGE:
      fg[1] = 255;
      bg[1] = 255;
    case GIMP_GRAY_IMAGE:
      fg[0] = gimp_rgb_intensity_uchar (&foreground);
      bg[0] = gimp_rgb_intensity_uchar (&background);
      break;

    default:
      break;
    }


  if (cvals.size < 1)
    {
      /* make size 1 to prevent division by zero */
      cvals.size = 1;
    }

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      y = dest_rgn.y;

      dest_row = dest_rgn.data;
      for ( row = 0; row < dest_rgn.h; row++)
	{
	  dest = dest_row;
	  x = dest_rgn.x;

	  for (col = 0; col < dest_rgn.w; col++)
	    {
	      gint val, xp, yp;

	      if (cvals.mode)
		{
		  /* Psychobilly Mode */
		  val = ((inblock (x, cvals.size) == inblock (y, cvals.size))
                         ? 0 : 1);
		}
	      else
		{
		  /* Normal, regular checkerboard mode.
		   * Determine base factor (even or odd) of block
		   * this x/y position is in.
		   */
		  xp = x / cvals.size;
		  yp = y / cvals.size;

		  /* if both even or odd, color sqr */
		  val = ( (xp&1) == (yp&1) ) ? 0 : 1;
		}

	      for (bp = 0; bp < dest_rgn.bpp; bp++)
		dest[bp] = val ? fg[bp] : bg[bp];

	      dest += dest_rgn.bpp;
	      x++;
	    }

	  dest_row += dest_rgn.rowstride;
	  y++;
	}

      progress += dest_rgn.w * dest_rgn.h;
      gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
    }

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
}

static gint
inblock (gint pos, 
	 gint size)
{
  static gint *in = NULL;	/* initialized first time */
  gint         len;

  /* avoid a FP exception */
  if (size == 1)
    size = 2;

  len = size*size;
  /*
   * Initialize the array; since we'll be called thousands of
   * times with the same size value, precompute the array.
   */
  if (in == NULL)
    {
      gint cell = 1;	/* cell value */
      gint i, j, k;

      in = g_new (gint, len);

      /*
       * i is absolute index into in[]
       * j is current number of blocks to fill in with a 1 or 0.
       * k is just counter for the j cells.
       */
      i=0;
      for (j=1; j<=size; j++ )
	{ /* first half */
	  for (k=0; k<j; k++ )
	    {
	      in[i++] = cell;
	    }
	  cell = !cell;
	}
      for ( j=size-1; j>=1; j--)
	{ /* second half */
	  for (k=0; k<j; k++ )
	    {
	      in[i++] = cell;
	    }
	  cell = !cell;
	}
    }

  /* place pos within 0..(len-1) grid and return the value. */
  return in[ pos % (len-1) ];
}

static gboolean
do_checkerboard_dialog (gint32        image_ID,
			GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *toggle;
  GtkWidget *size_entry;
  gint	     size, width, height;
  GimpUnit   unit;
  gdouble    xres;
  gdouble    yres;

  gimp_ui_init ("checkerboard", FALSE);

  dlg = gimp_dialog_new (_("Checkerboard"), "checkerboard",
			 gimp_standard_help_func, "filters/checkerboard.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 GTK_STOCK_CANCEL, gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 GTK_STOCK_OK, check_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,

			 NULL);

  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  /*  Get the image resolution and unit  */
  gimp_image_get_resolution (image_ID, &xres, &yres);
  unit = gimp_image_get_unit (image_ID);

  width  = gimp_drawable_width (drawable->drawable_id);
  height = gimp_drawable_height (drawable->drawable_id);
  size   = MIN (width, height);

  /*  parameter settings  */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  toggle = gtk_check_button_new_with_mnemonic (_("_Psychobilly"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), cvals.mode);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &cvals.mode);

  size_entry = gimp_size_entry_new (1, unit, "%a",
                                    TRUE, TRUE, FALSE, SPIN_BUTTON_WIDTH,
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_table_set_col_spacing (GTK_TABLE (size_entry), 0, 4);
  gtk_table_set_col_spacing (GTK_TABLE (size_entry), 1, 4);

  /*  set the unit back to pixels, since most times we will want pixels */
  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (size_entry), GIMP_UNIT_PIXEL);

  /*  set the resolution to the image resolution  */
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (size_entry), 0, xres, TRUE);

  /*  set the size (in pixels) that will be treated as 0% and 100%  */
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (size_entry), 0, 0.0, size);

  /*  set upper and lower limits (in pixels)  */
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (size_entry), 0,
                                         1.0, size);

  /*  initialize the values  */
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (size_entry), 0, cvals.size);

  /*  attach labels  */
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (size_entry),
                                _("_Size:"), 1, 0, 0.0);
   
  g_signal_connect (size_entry, "value_changed",
                    G_CALLBACK (check_size_update_callback),
                    &cvals.size);

  gtk_container_add (GTK_CONTAINER (vbox), size_entry);
  gtk_widget_show (size_entry);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return cint.run;
}

static void 
check_size_update_callback(GtkWidget * widget, gpointer data)
{
  cvals.size = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
}


static void
check_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  cint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
