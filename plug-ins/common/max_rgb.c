/* max_rgb.c -- This is a plug-in for the GIMP
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 * Time-stamp: <2000-02-08 16:26:24 yasuhiro>
 * Version: 0.35
 *
 * Copyright (C) 1997 Shuji Narazaki <narazaki@InetQ.or.jp>
 *
 * May 2000 - tim copperfield [timecop@japan.co.jp]
 *
 * Added a preview mode.  After adding preview mode realised just exactly
 * how useless this plugin is :)
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

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Replace them with the right ones */
#define	PLUG_IN_NAME	     "plug_in_max_rgb"
#define SHORT_NAME	     "max_rgb"

static void	query	(void);
static void	run	(const gchar      *name,
			 gint              nparams,
			 const GimpParam  *param,
			 gint             *nreturn_vals,
			 GimpParam       **return_vals);

static GimpPDBStatusType main_function (GimpDrawable *drawable,
			                gboolean      preview_mode);

static gint	   dialog         (GimpDrawable *drawable);
static void        radio_callback (GtkWidget    *widget,
				   gpointer      data);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

enum
{
  MIN_CHANNELS = 0,
  MAX_CHANNELS = 1
};

typedef struct
{
  gint      max_p;
} ValueType;

static ValueType pvals =
{
  MAX_CHANNELS
};

static GimpRunMode       run_mode;
static GimpFixMePreview *preview;

MAIN ()

static void
query (void)
{
  static GimpParamDef args [] =
  {
    { GIMP_PDB_INT32,    "run_mode", "Interactive, non-interactive"},
    { GIMP_PDB_IMAGE,    "image",    "Input image (not used)"},
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"},
    { GIMP_PDB_INT32,    "max_p",    "1 for maximizing, 0 for minimizing"}
  };

  gimp_install_procedure (PLUG_IN_NAME,
			  "Return an image in which each pixel holds only "
			  "the channel that has the maximum value in three "
			  "(red, green, blue) channels, and other channels "
			  "are zero-cleared",
			  "the help is not yet written for this plug-in since none is needed.",
			  "Shuji Narazaki (narazaki@InetQ.or.jp)",
			  "Shuji Narazaki",
			  "May 2000",
                          N_("<Image>/Filters/Colors/_Max RGB..."),
			  "RGB*",
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
  GimpDrawable      *drawable;
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_EXECUTION_ERROR;

  run_mode = param[0].data.d_int32;
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_NAME, &pvals);
      /* Since a channel might be selected, we must check wheter RGB or not. */
      if (!gimp_drawable_is_rgb (drawable->drawable_id))
	{
	  g_message (_("Can only operate on RGB drawables."));
	  return;
	}
      if (! dialog (drawable))
	return;
      break;
    case GIMP_RUN_NONINTERACTIVE:
      /* You must copy the values of parameters to pvals or dialog variables. */
      pvals.max_p = param[3].data.d_int32;
      break;
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_NAME, &pvals);
      break;
    }

  status = main_function (drawable, FALSE);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();
  if (run_mode == GIMP_RUN_INTERACTIVE && status == GIMP_PDB_SUCCESS)
    gimp_set_data (PLUG_IN_NAME, &pvals, sizeof (ValueType));

  values[0].data.d_status = status;
}

typedef struct {
  gint init_value;
  gint flag;
  gboolean has_alpha;
} MaxRgbParam_t;

static void
max_rgb_func (const guchar *src,
	      guchar       *dest,
	      gint          bpp,
	      gpointer      data)
{
  MaxRgbParam_t *param = (MaxRgbParam_t*) data;
  gint   ch, max_ch = 0;
  guchar max, tmp_value;

  max = param->init_value;
  for (ch = 0; ch < 3; ch++)
    if (param->flag * max <= param->flag * (tmp_value = (*src++)))
      {
	if (max == tmp_value)
	  {
	    max_ch += 1 << ch;
	  }
	else
	  {
	    max_ch = 1 << ch; /* clear memories of old channels */
	    max = tmp_value;
	  }
      }

  dest[0] = (max_ch & (1 << 0)) ? max : 0;
  dest[1] = (max_ch & (1 << 1)) ? max : 0;
  dest[2] = (max_ch & (1 << 2)) ? max : 0;
  if (param->has_alpha)
    dest[3] = *src;
}

static GimpPDBStatusType
main_function (GimpDrawable *drawable,
	       gboolean      preview_mode)
{
  MaxRgbParam_t param;

  param.init_value = (pvals.max_p > 0) ? 0 : 255;
  param.flag = (0 < pvals.max_p) ? 1 : -1;
  param.has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  if (preview_mode)
    {
      gimp_fixme_preview_update (preview, max_rgb_func, &param);
    }
  else
    {
      gimp_progress_init ( _("Max RGB..."));

      gimp_rgn_iterate2 (drawable, run_mode, max_rgb_func, &param);

      gimp_drawable_detach (drawable);
    }

  return GIMP_PDB_SUCCESS;
}


/* dialog stuff */
static gint
dialog (GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *max;
  GtkWidget *min;
  gboolean   run;

  gimp_ui_init ("max_rgb", TRUE);

  dlg = gimp_dialog_new (_("Max RGB"), "max_rgb",
                         NULL, 0,
			 gimp_standard_help_func, "filters/max_rgb.html",

			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK,     GTK_RESPONSE_OK,

			 NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = gimp_fixme_preview_new (drawable, TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview->frame, FALSE, FALSE, 0);
  main_function (drawable, TRUE);
  gtk_widget_show (preview->widget);

  frame = gimp_radio_group_new2 (TRUE, _("Parameter Settings"),
				 G_CALLBACK (radio_callback),
				 &pvals.max_p,
                                 GINT_TO_POINTER (pvals.max_p),

				 _("_Hold the Maximal Channels"),
				 GINT_TO_POINTER (MAX_CHANNELS), &max,

				 _("Ho_ld the Minimal Channels"),
				 GINT_TO_POINTER (MIN_CHANNELS), &min,

				 NULL);

  g_object_set_data (G_OBJECT (max), "drawable", drawable);
  g_object_set_data (G_OBJECT (min), "drawable", drawable);

  gtk_container_add (GTK_CONTAINER (main_vbox), frame);
  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  run = (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}

static void
radio_callback (GtkWidget *widget,
		gpointer  data)
{
  gimp_radio_button_update (widget, data);

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      GimpDrawable *drawable;
      drawable = g_object_get_data (G_OBJECT (widget), "drawable");
      main_function (drawable, TRUE);
    }
}
