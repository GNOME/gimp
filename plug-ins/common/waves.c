/**************************************************
 * file: waves/waves.c
 *
 * Copyright (c) 1997 Eric L. Hernes (erich@rrnet.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */
#include "config.h"

#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <plug-ins/megawidget/megawidget.h>

struct Grgb {
  guint8 red;
  guint8 green;
  guint8 blue;
};

struct GRegion {
  gint32 x;
  gint32 y;
  gint32 width;
  gint32 height;
};

struct piArgs {
  gdouble amplitude;
  gdouble phase;
  gdouble wavelength;
  gint32 type;
  gint32 reflective;
};

struct mwPreview *mwp;

static void query (void);
static void run   (char *name, int nparam, GParam *param,
		   int *nretvals, GParam **retvals);

int pluginCore   (struct piArgs *argp, gint32 drawable);
int pluginCoreIA (struct piArgs *argp, gint32 drawable);

static mw_preview_t waves_do_preview;

static void wave (guchar *src, guchar *dest, gint width, gint height, gint bypp,
		  gdouble amplitude, gdouble wavelength, gdouble phase,
		  gint smear, gint reflective, gint verbose);

static guchar bilinear (gdouble x, gdouble y, guchar *v);
#define WITHIN(a, b, c) ((((a) <= (b)) && ((b) <= (c))) ? 1 : 0)

GPlugInInfo PLUG_IN_INFO =
{
  NULL, /* init */
  NULL, /* quit */
  query, /* query */
  run, /* run */
};

MAIN()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "The Image" },
    { PARAM_DRAWABLE, "drawable", "The Drawable" },
    { PARAM_FLOAT, "amplitude", "The Amplitude of the Waves" },
    { PARAM_FLOAT, "phase", "The Phase of the Waves" },
    { PARAM_FLOAT, "wavelength", "The Wavelength of the Waves" },
    { PARAM_INT32, "type", "Type of waves, black/smeared" },
    { PARAM_INT32, "reflective", "Use Reflection" },
  };
  static int nargs = 8;

  static GParamDef *rets = NULL;
  static int nrets = 0;

  gimp_install_procedure ("plug_in_waves",
			  "Distort the image with waves",
			  "none yet",
			  "Eric L. Hernes, Stephen Norris",
			  "Stephen Norris",
			  "1997",
			  "<Image>/Filters/Distorts/Waves...",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nrets,
			  args, rets);
}

static void
run (char    *name,
     int      nparam,
     GParam  *param,
     int     *nretvals,
     GParam **retvals)
{
  static GParam rvals[1];

  struct piArgs args;

  *nretvals = 1;
  *retvals = rvals;

  memset (&args, (int)0, sizeof (struct piArgs));
  args.type=-1;
  gimp_get_data ("plug_in_waves", &args);

  rvals[0].type = PARAM_STATUS;
  rvals[0].data.d_status = STATUS_SUCCESS;
  switch (param[0].data.d_int32)
    {
      GDrawable *drw;

    case RUN_INTERACTIVE:
      /* XXX: add code here for interactive running */
      if (args.type == -1)
	{
	  args.amplitude = 10.0;
	  args.wavelength = 10;
	  args.phase = 0.0;
	  args.type = 0;
	  args.reflective = 0;
	}

      drw = gimp_drawable_get (param[2].data.d_drawable);
      mwp=mw_preview_build (drw);

      if (pluginCoreIA(&args, param[2].data.d_drawable)==-1)
	{
	  rvals[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
      else
	{
	  gimp_set_data ("plug_in_waves", &args, sizeof (struct piArgs));
	}

    break;

    case RUN_NONINTERACTIVE:
      /* XXX: add code here for non-interactive running */
      if (nparam != 8)
	{
	  rvals[0].data.d_status = STATUS_CALLING_ERROR;
	  break;
	}
      args.amplitude = param[3].data.d_float;
      args.phase = param[4].data.d_float;
      args.wavelength = param[5].data.d_float;
      args.type = param[6].data.d_int32;
      args.reflective = param[7].data.d_int32;

      if (pluginCore(&args, param[2].data.d_drawable)==-1)
	{
	  rvals[0].data.d_status = STATUS_EXECUTION_ERROR;
	  break;
	}
    break;

    case RUN_WITH_LAST_VALS:
      /* XXX: add code here for last-values running */
      if (pluginCore(&args, param[2].data.d_drawable)==-1)
	{
	  rvals[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
      break;
    }
}

gint
pluginCore (struct piArgs *argp,
	    gint32         drawable)
{
  gint retval=0;
  GDrawable *drw;
  GPixelRgn srcPr, dstPr;
  guchar *src, *dst;
  guint width, height, Bpp;

  drw = gimp_drawable_get (drawable);

  width = drw->width;
  height = drw->height;
  Bpp = drw->bpp;

  src = g_new (guchar, width * height * Bpp);
  dst = g_new (guchar, width * height * Bpp);
  gimp_pixel_rgn_init (&srcPr, drw, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dstPr, drw, 0, 0, width, height, TRUE, TRUE);
  gimp_pixel_rgn_get_rect (&srcPr, src, 0, 0, width, height);

  wave (src, dst, width, height, Bpp, argp->amplitude, argp->wavelength,
        argp->phase, argp->type==0, argp->reflective, 1);
  gimp_pixel_rgn_set_rect (&dstPr, dst, 0, 0, width, height);

  g_free (src);
  g_free (dst);

  gimp_drawable_flush (drw);
  gimp_drawable_merge_shadow (drw->id, TRUE);
  gimp_drawable_update (drw->id, 0, 0, width, height);

  gimp_displays_flush ();

  return retval;
}

static gboolean run_flag = FALSE;

static void
waves_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  run_flag = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

gint
pluginCoreIA (struct piArgs *argp,
	      gint32         drawable)
{
  gint r=-1; /* default to error return */
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *preview;
  gchar **argv;
  gint    argc;

  static struct mwRadioGroup mode[] = {
    { "Smear", 1 },
    { "Blacken", 0 },
    { NULL, 0 },
  };

  /* Set args */
  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("waves");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
  
  dlg = gimp_dialog_new ("Waves", "waves",
			 gimp_plugin_help_func, "filters/waves.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 "OK", waves_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 "Cancel", gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_border_width (GTK_CONTAINER (hbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  preview = mw_preview_new (vbox, mwp, &waves_do_preview);
  gtk_object_set_data (GTK_OBJECT (preview), "piArgs", argp);
  gtk_object_set_data (GTK_OBJECT (preview), "mwRadioGroup", mode);
  waves_do_preview (preview);

  mw_toggle_button_new (vbox, NULL, "Reflective", &argp->reflective);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new ("Parameters");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (4, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  mw_fscale_entry_new (table, "Amplitude:", 0.0, 101.0, 1.0, 5.0, 0.0,
		       0, 1, 1, 2, &argp->amplitude);
  mw_fscale_entry_new (table, "Phase:", 0.0, 360.0, 2.0, 5.0, 0.0,
		       0, 1, 2, 3, &argp->phase);
  mw_fscale_entry_new (table, "Wavelength:", 0.1, 50.0, 1.0, 5.0, 0.0,
		       0, 1, 3, 4, &argp->wavelength);
  gtk_widget_show (table);

  mw_radio_group_new (vbox, "Mode", mode);

  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  argp->type = mw_radio_result(mode);

  if (run_flag)
    {
#if 0
      fprintf(stderr, "running:\n");
      /*fprintf(stderr, "\t(image %d)\n", argp->image);*/
      fprintf(stderr, "\t(drawable %d)\n", argp->drawable);
      fprintf(stderr, "\t(amplitude %f)\n", argp->amplitude);
      fprintf(stderr, "\t(phase %f)\n", argp->phase);
      fprintf(stderr, "\t(wavelength %f)\n", argp->wavelength);
      fprintf(stderr, "\t(type %d)\n", argp->type);
      fprintf(stderr, "\t(reflective %d)\n", argp->reflective);
#endif
      return pluginCore (argp, drawable);
    }
  else
    {
      return r;
    }
}

static void
waves_do_preview (GtkWidget *w)
{
  static GtkWidget *theWidget = NULL;
  struct piArgs *argp;
  struct mwRadioGroup *rgp;
  guchar *dst;
  gint y;

  if (theWidget==NULL)
    {
      theWidget=w;
    }

  argp = gtk_object_get_data(GTK_OBJECT(theWidget), "piArgs");
  rgp = gtk_object_get_data(GTK_OBJECT(theWidget), "mwRadioGroup");
  argp->type = mw_radio_result(rgp);
  dst = g_new (guchar, mwp->width * mwp->height * mwp->bpp);

  wave (mwp->bits, dst, mwp->width, mwp->height,
	mwp->bpp, (argp->amplitude/mwp->scale), (argp->wavelength/mwp->scale),
	argp->phase, argp->type==0, argp->reflective, 0);

  for (y = 0; y < mwp->height; y++)
    {
      gtk_preview_draw_row (GTK_PREVIEW (theWidget),
			    dst+(y*mwp->width*mwp->bpp), 0, y,
			    mwp->width);
    }

  gtk_widget_draw (theWidget, NULL);
  gdk_flush ();
  g_free (dst);
}


static void
wave (guchar  *src,
      guchar  *dst,
      gint     width,
      gint     height,
      gint     bypp,
      gdouble  amplitude,
      gdouble  wavelength,
      gdouble  phase,
      gint     smear,
      gint     reflective,
      gint     verbose)
{
  long   rowsiz;
  guchar *p;
  guchar *dest;
  gint    x1, y1, x2, y2;
  gint    x, y;
  gint    prog_interval=0;
  gint    x1_in, y1_in, x2_in, y2_in;

  gdouble cen_x, cen_y;       /* Center of wave */
  gdouble xhsiz, yhsiz;       /* Half size of selection */
  gdouble radius, radius2;    /* Radius and radius^2 */
  gdouble amnt, d;
  gdouble needx, needy;
  gdouble dx, dy;
  gdouble xscale, yscale;

  gint xi, yi;

  guchar  values[4];
  guchar  val;

  gint k;

  phase = phase*G_PI/180;
  rowsiz   = width * bypp;

  if (verbose)
    {
      gimp_progress_init ("Waves");
      prog_interval=height/10;
    }

  x1=y1=0;
  x2=width;
  y2=height;

  /* Wave the image.  For each pixel in the destination image
   * which is inside the selection, we compute the corresponding
   * waved location in the source image.  We use bilinear
   * interpolation to avoid ugly jaggies.
   *
   * Let's assume that we are operating on a circular area.
   * Every point within <radius> distance of the wave center is
   * waveed to its destination position.
   *
   * Basically, introduce a wave into the image. I made up the
   * forumla initially, but it looks good. Quartic added the
   * wavelength etc. to it while I was asleep :) Just goes to show
   * we should work with people in different time zones more.
   *
   */

  /* Center of selection */
  cen_x = (double) (x2 - 1 + x1) / 2.0;
  cen_y = (double) (y2 - 1 + y1) / 2.0;

  /* Compute wave radii (semiaxes) */
  xhsiz = (double) (x2 - x1) / 2.0;
  yhsiz = (double) (y2 - y1) / 2.0;

  /* These are the necessary scaling factors to turn the wave
     ellipse into a large circle */

  if (xhsiz < yhsiz)
    {
      xscale = yhsiz / xhsiz;
      yscale = 1.0;
    }
  else if (xhsiz > yhsiz)
    {
      xscale = 1.0;
      yscale = xhsiz / yhsiz;
    }
  else
    {
      xscale = 1.0;
      yscale = 1.0;
    }

  radius  = MAX(xhsiz, yhsiz);
  radius2 = radius * radius;

  /* Wave the image! */

  dst += y1 * rowsiz + x1 * bypp;

  wavelength = (wavelength * 2);

  for (y = y1; y < y2; y++)
    {
      dest = dst;

      if (verbose && (y % prog_interval == 0))
	gimp_progress_update((double)y/(double)height);

      for (x = x1; x < x2; x++)
	{
	  /* Distance from current point to wave center, scaled */
	  dx = (x - cen_x) * xscale;
	  dy = (y - cen_y) * yscale;

	  /* Distance^2 to center of *circle* (our scaled ellipse) */
	  d = sqrt(dx * dx + dy * dy);

	  /* Use the formula described above. */

	  /* Calculate waved point and scale again to ellipsify */

	  /*
	   * Reflective waves are strange - the effect is much
	   * more like a mirror which is in the shape of
	   * the wave than anything else.
	   */

	  if (reflective)
	    {
	      amnt = amplitude * fabs (sin (((d / wavelength)
					     * (2.0 * G_PI) + phase)));

	      needx = (amnt * dx) / xscale + cen_x;
	      needy = (amnt * dy) / yscale + cen_y;
	    }
	  else
	    {
	      amnt = amplitude * sin (((d / wavelength)
				       * (2.0 * G_PI) + phase));

	      needx = (amnt + dx) / xscale + cen_x;
	      needy = (amnt + dy) / yscale + cen_y;
	    }

	  /* Calculations complete; now copy the proper pixel */

	  xi = needx;
	  yi = needy;

	  if (smear)
	    {
	      if (xi > width - 2)
		{
		  xi = width - 2;
		}
	      else if (xi < 0)
		{
		  xi = 0;
		}
	      if (yi > height - 2)
		{
		  yi = height - 2;
		}
	      else if (yi < 0)
		{
		  yi = 0;
		}
	    }

	  p = src + rowsiz * yi + xi * bypp;

	  x1_in = WITHIN (0, xi, width - 1);
	  y1_in = WITHIN (0, yi, height - 1);
	  x2_in = WITHIN (0, xi + 1, width - 1);
	  y2_in = WITHIN (0, yi + 1, height - 1);

	  for (k = 0; k < bypp; k++)
	    {
	      if (x1_in && y1_in)
		values[0] = *(p + k);
	      else
		values[0] = 0;

	      if (x2_in && y1_in)
		values[1] = *(p + bypp + k);
	      else
		values[1] = 0;

	      if (x1_in && y2_in)
		values[2] = *(p + rowsiz + k);
	      else
		values[2] = 0;

	      if (x2_in)
		{
		  if (y2_in)
		    values[3] = *(p + bypp + k + rowsiz);
		  else
		    values[3] = 0;
		}
	      else
		values[3] = 0;

	      val = bilinear (needx, needy, values);

	      *dest++ = val;
	    }
	}

      dst += rowsiz;
    }

  if (verbose)
    gimp_progress_update (1.0);

}

static guchar
bilinear (gdouble  x,
	  gdouble  y,
	  guchar  *v)
{
  double m0, m1;
  x = fmod (x, 1.0);
  y = fmod (y, 1.0);

  if (x < 0)
    x += 1.0;
  if (y < 0)
    y += 1.0;

  m0 = (1.0 - x) * v[0] + x * v[1];
  m1 = (1.0 - x) * v[2] + x * v[3];

  return (guchar) ((1.0 - y) * m0 + y * m1);
}
