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


static void      query  (void);
static void      run    (const gchar       *name,
			 gint               nparams,
			 const GimpParam   *param,
			 gint              *nreturn_vals,
			 GimpParam        **return_vals);

static void      do_checkerboard_pattern    (GimpDrawable *drawable);
static gint      inblock                    (gint          pos,
                                             gint          size);

static gboolean	 checkerboard_dialog        (gint32        image_ID,
                                             GimpDrawable *drawable);
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
			  N_("_Checkerboard..."),
			  "RGB*, GRAY*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), 0,
			  args, NULL);

  gimp_plugin_menu_register ("plug_in_checkerboard",
                             "<Image>/Filters/Render/Pattern");
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
      if (! checkerboard_dialog (image_ID, drawable))
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

typedef struct {
  guchar fg[4];
  guchar bg[4];
} CheckerboardParam_t;

static void
checkerboard_func (gint x,
		   gint y,
		   guchar *dest,
		   gint bpp,
		   gpointer data)
{
  CheckerboardParam_t *param = (CheckerboardParam_t*) data;
  gint val, xp, yp;
  gint b;

  if (cvals.mode)
    {
      /* Psychobilly Mode */
      val = (inblock (x, cvals.size) != inblock (y, cvals.size));
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
      val = ( (xp & 1) != (yp & 1) );
    }

  for (b = 0; b < bpp; b++)
    dest[b] = val ? param->fg[b] : param->bg[b];
}

static void
do_checkerboard_pattern (GimpDrawable *drawable)
{
  CheckerboardParam_t  param;
  GimpRgnIterator     *iter;
  GimpRGB              color;

  gimp_context_get_background (&color);
  gimp_drawable_get_color_uchar (drawable->drawable_id, &color, param.bg);

  gimp_context_get_foreground (&color);
  gimp_drawable_get_color_uchar (drawable->drawable_id, &color, param.fg);

  if (cvals.size < 1)
    {
      /* make size 1 to prevent division by zero */
      cvals.size = 1;
    }

  iter = gimp_rgn_iterator_new (drawable, 0);
  gimp_rgn_iterator_dest (iter, checkerboard_func, &param);
  gimp_rgn_iterator_free (iter);
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
checkerboard_dialog (gint32        image_ID,
                     GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *vbox;
  GtkWidget *toggle;
  GtkWidget *size_entry;
  gint	     size, width, height;
  GimpUnit   unit;
  gdouble    xres;
  gdouble    yres;
  gboolean   run;

  gimp_ui_init ("checkerboard", FALSE);

  dlg = gimp_dialog_new (_("Checkerboard"), "checkerboard",
                         NULL, 0,
			 gimp_standard_help_func, "plug-in-checkerboard",

			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK,     GTK_RESPONSE_OK,

			 NULL);

  /*  Get the image resolution and unit  */
  gimp_image_get_resolution (image_ID, &xres, &yres);
  unit = gimp_image_get_unit (image_ID);

  width  = gimp_drawable_width (drawable->drawable_id);
  height = gimp_drawable_height (drawable->drawable_id);
  size   = MIN (width, height);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox, TRUE, TRUE, 0);
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

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}

static void
check_size_update_callback (GtkWidget *widget,
                            gpointer   data)
{
  cvals.size = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
}
