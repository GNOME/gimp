/*
 * Color To Alpha plug-in v1.0 by Seth Burgess, sjburges@gimp.org 1999/05/14
 *  with algorithm by clahey
 */

/* The GIMP -- an image manipulation program
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PRV_WIDTH  40
#define PRV_HEIGHT 20


typedef struct
{
  GimpRGB  color;
} C2AValues;


/* Declare local functions.
 */
static void        query               (void);
static void        run                 (const gchar       *name,
                                        gint               nparams,
                                        const GimpParam   *param,
                                        gint              *nreturn_vals,
                                        GimpParam        **return_vals);

static void inline colortoalpha        (GimpRGB           *src,
                                        const GimpRGB     *color);
static void        toalpha             (GimpDrawable      *drawable);

/* UI stuff */
static gboolean    colortoalpha_dialog (GimpDrawable      *drawable);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static C2AValues pvals =
{
  { 1.0, 1.0, 1.0, 1.0 } /* white default */
};


MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_COLOR,    "color",    "Color to remove" }
  };

  gimp_install_procedure ("plug_in_colortoalpha",
			  "Convert the color in an image to alpha",
			  "This replaces as much of a given color as possible "
			  "in each pixel with a corresponding amount of alpha, "
			  "then readjusts the color accordingly.",
			  "Seth Burgess",
			  "Seth Burgess <sjburges@gimp.org>",
			  "7th Aug 1999",
			  N_("Color to _Alpha..."),
			  "RGB*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), 0,
			  args, NULL);

  gimp_plugin_menu_register ("plug_in_colortoalpha",
                             "<Image>/Filters/Colors");
  gimp_plugin_menu_register ("plug_in_colortoalpha",
                             "<Image>/Layer/Transparency/Modify");
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
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  INIT_I18N ();

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  image_ID = param[1].data.d_image;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data ("plug_in_colortoalpha", &pvals);
      gimp_context_get_foreground (&pvals.color);
      if (! colortoalpha_dialog (drawable))
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 4)
	status = GIMP_PDB_CALLING_ERROR;

      if (status == GIMP_PDB_SUCCESS)
	pvals.color = param[3].data.d_color;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data ("plug_in_colortoalpha", &pvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      gimp_image_undo_group_start (image_ID);

      /*  Add alpha if not present */
      gimp_layer_add_alpha (drawable->drawable_id);
      drawable = gimp_drawable_get (drawable->drawable_id);

      /*  Make sure that the drawable is RGB color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id) &&
	  gimp_drawable_is_layer (drawable->drawable_id))
	{
          gimp_progress_init (_("Removing color..."));
	  toalpha (drawable);
	}

      gimp_drawable_detach (drawable);

      gimp_image_undo_group_end (image_ID);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }
  else
    {
      gimp_drawable_detach (drawable);
    }

  if (run_mode == GIMP_RUN_INTERACTIVE)
    gimp_set_data ("plug_in_colortoalpha", &pvals, sizeof (pvals));

  values[0].data.d_status = status;
}

static inline void
colortoalpha (GimpRGB       *src,
	      const GimpRGB *color)
{
  GimpRGB alpha;

  alpha.a = src->a;

  if (color->r < 0.0001)
    alpha.r = src->r;
  else if (src->r > color->r)
    alpha.r = (src->r - color->r) / (1.0 - color->r);
  else if (src->r < color->r)
    alpha.r = (color->r - src->r) / color->r;
  else alpha.r = 0.0;

  if (color->g < 0.0001)
    alpha.g = src->g;
  else if (src->g > color->g)
    alpha.g = (src->g - color->g) / (1.0 - color->g);
  else if (src->g < color->g)
    alpha.g = (color->g - src->g) / (color->g);
  else alpha.g = 0.0;

  if (color->b < 0.0001)
    alpha.b = src->b;
  else if (src->b > color->b)
    alpha.b = (src->b - color->b) / (1.0 - color->b);
  else if (src->b < color->b)
    alpha.b = (color->b - src->b) / (color->b);
  else alpha.b = 0.0;

  if (alpha.r > alpha.g)
    {
      if (alpha.r > alpha.b)
	{
	  src->a = alpha.r;
	}
      else
	{
	  src->a = alpha.b;
	}
    }
  else if (alpha.g > alpha.b)
    {
      src->a = alpha.g;
    }
  else
    {
      src->a = alpha.b;
    }

  if (src->a < 0.0001)
    return;

  src->r = (src->r - color->r) / src->a + color->r;
  src->g = (src->g - color->g) / src->a + color->g;
  src->b = (src->b - color->b) / src->a + color->b;

  src->a *= alpha.a;
}

/*
  <clahey>   so if a1 > c1, a2 > c2, and a3 > c2 and a1 - c1 > a2-c2, a3-c3,
             then a1 = b1 * alpha + c1 * (1-alpha)
             So, maximizing alpha without taking b1 above 1 gives
	     a1 = alpha + c1(1-alpha) and therefore alpha = (a1-c1) / (1-c1).
  <sjburges> clahey: btw, the ordering of that a2, a3 in the white->alpha didn't
             matter
  <clahey>   sjburges: You mean that it could be either a1, a2, a3 or a1, a3, a2?
  <sjburges> yeah
  <sjburges> because neither one uses the other
  <clahey>   sjburges: That's exactly as it should be.  They are both just getting
             reduced to the same amount, limited by the the darkest color.
  <clahey>   Then a2 = b2 * alpha + c2 * (1- alpha).  Solving for b2 gives
             b2 = (a1-c2)/alpha + c2.
  <sjburges> yeah
  <clahey>   That gives us are formula for if the background is darker than the
             foreground? Yep.
  <clahey>   Next if a1 < c1, a2 < c2, a3 < c3, and c1-a1 > c2-a2, c3-a3, and by our
             desired result a1 = b1 * alpha + c1 * (1-alpha), we maximize alpha
             without taking b1 negative gives alpha = 1 - a1 / c1.
  <clahey>   And then again, b2 = (a2-c2) / alpha + c2 by the same formula.
             (Actually, I think we can use that formula for all cases, though it
             may possibly introduce rounding error.
  <clahey>   sjburges: I like the idea of using floats to avoid rounding error.
             Good call.
*/

static void
toalpha_func (const guchar *src,
              guchar       *dest,
              gint          bpp,
              gpointer      data)
{
  GimpRGB color;

  gimp_rgba_set_uchar (&color, src[0], src[1], src[2], src[3]);
  colortoalpha (&color, &pvals.color);
  gimp_rgba_get_uchar (&color, &dest[0], &dest[1], &dest[2], &dest[3]);
}

static void
toalpha (GimpDrawable *drawable)
{
  gimp_rgn_iterate2 (drawable, 0 /* unused */, toalpha_func, NULL);
}

static gboolean
colortoalpha_dialog (GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *label;
  gboolean   run;

  gimp_ui_init ("colortoalpha", TRUE);

  dlg = gimp_dialog_new (_("Color to Alpha"), "colortoalpha",
                         NULL, 0,
			 gimp_standard_help_func, "plug-in-colortoalpha",

			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK,     GTK_RESPONSE_OK,

			 NULL);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
		      hbox, FALSE, FALSE, 0);

  gtk_widget_show (hbox);

  label = gtk_label_new (_("From:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  button = gimp_color_button_new (_("Color to Alpha Color Picker"),
				  PRV_WIDTH, PRV_HEIGHT,
				  &pvals.color,
				  GIMP_COLOR_AREA_FLAT);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "color_changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &pvals.color);

  label = gtk_label_new (_("to alpha"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}
