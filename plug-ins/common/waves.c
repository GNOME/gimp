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

#include "libgimp/stdplugins-intl.h"


enum
{
  MODE_SMEAR,
  MODE_BLACKEN
};

typedef struct
{
  gdouble amplitude;
  gdouble phase;
  gdouble wavelength;
  gint32  type;
  gint32  reflective;
} piArgs;

/*  preview stuff -- to be removed as soon as we have a real libgimp preview  */
static gint            do_preview = TRUE;
static GimpOldPreview *preview;

static GtkWidget *mw_preview_new (GtkWidget *parent,
                                  GimpDrawable *drawable);

static void query (void);
static void run   (const gchar      *name,
                   gint              nparam,
                   const GimpParam  *param,
                   gint             *nretvals,
                   GimpParam       **retvals);

static gint pluginCore       (piArgs *argp,
                              GimpDrawable *drawable);
static gint pluginCoreIA     (piArgs *argp,
                              GimpDrawable *drawable);

static void waves_do_preview (void);

static void wave (guchar  *src,
                  guchar  *dest,
                  gint     width,
                  gint     height,
                  gint     bypp,
                  gboolean has_alpha,
                  gdouble  amplitude,
                  gdouble  wavelength,
                  gdouble  phase,
                  gint     smear,
                  gint     reflective,
                  gint     verbose);

#define WITHIN(a, b, c) ((((a) <= (b)) && ((b) <= (c))) ? TRUE : FALSE)

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "The Image" },
    { GIMP_PDB_DRAWABLE, "drawable", "The Drawable" },
    { GIMP_PDB_FLOAT, "amplitude", "The Amplitude of the Waves" },
    { GIMP_PDB_FLOAT, "phase", "The Phase of the Waves" },
    { GIMP_PDB_FLOAT, "wavelength", "The Wavelength of the Waves" },
    { GIMP_PDB_INT32, "type", "Type of waves, black/smeared" },
    { GIMP_PDB_INT32, "reflective", "Use Reflection" }
  };

  gimp_install_procedure ("plug_in_waves",
                          "Distort the image with waves",
                          "none yet",
                          "Eric L. Hernes, Stephen Norris",
                          "Stephen Norris",
                          "1997",
                          N_("<Image>/Filters/Distorts/_Waves..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (const gchar      *name,
     gint              nparam,
     const GimpParam  *param,
     gint             *nretvals,
     GimpParam       **retvals)
{
  static GimpParam  rvals[1];
  GimpDrawable     *drawable;

  piArgs args;

  INIT_I18N ();

  *nretvals = 1;
  *retvals  = rvals;

  rvals[0].type          = GIMP_PDB_STATUS;
  rvals[0].data.d_status = GIMP_PDB_SUCCESS;

  memset (&args, (int) 0, sizeof (piArgs));
  args.type = -1;
  gimp_get_data ("plug_in_waves", &args);

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (param[0].data.d_int32)
    {

    case GIMP_RUN_INTERACTIVE:
      /* XXX: add code here for interactive running */
      if (args.type == -1)
        {
          args.amplitude  = 10.0;
          args.wavelength = 10;
          args.phase      = 0.0;
          args.type       = MODE_SMEAR;
          args.reflective = 0;
        }

      if (pluginCoreIA(&args, drawable) == -1)
        {
          rvals[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
        }
      else
        {
          gimp_set_data ("plug_in_waves", &args, sizeof (piArgs));
        }

    break;

    case GIMP_RUN_NONINTERACTIVE:
      /* XXX: add code here for non-interactive running */
      if (nparam != 8)
        {
          rvals[0].data.d_status = GIMP_PDB_CALLING_ERROR;
          break;
        }
      args.amplitude  = param[3].data.d_float;
      args.phase      = param[4].data.d_float;
      args.wavelength = param[5].data.d_float;
      args.type       = param[6].data.d_int32;
      args.reflective = param[7].data.d_int32;

      if (pluginCore (&args, drawable) == -1)
        {
          rvals[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
          break;
        }
    break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* XXX: add code here for last-values running */
      if (pluginCore (&args, drawable) == -1)
        {
          rvals[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
        }
      break;
    }
}

static gint
pluginCore (piArgs *argp,
            GimpDrawable *drawable)
{
  gint retval=0;
  GimpPixelRgn srcPr, dstPr;
  guchar *src, *dst;
  guint width, height, bpp, has_alpha;

  width = drawable->width;
  height = drawable->height;
  bpp = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  src = g_new (guchar, width * height * bpp);
  dst = g_new (guchar, width * height * bpp);
  gimp_pixel_rgn_init (&srcPr, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dstPr, drawable, 0, 0, width, height, TRUE, TRUE);
  gimp_pixel_rgn_get_rect (&srcPr, src, 0, 0, width, height);

  wave (src, dst, width, height, bpp, has_alpha,
        argp->amplitude, argp->wavelength, argp->phase,
        argp->type==0, argp->reflective, 1);
  gimp_pixel_rgn_set_rect (&dstPr, dst, 0, 0, width, height);

  g_free (src);
  g_free (dst);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, 0, 0, width, height);

  gimp_displays_flush ();

  return retval;
}

static void
waves_toggle_button_update (GtkWidget *widget,
                            gpointer   data)
{
  gimp_toggle_button_update (widget, data);
  waves_do_preview ();
}

static void
waves_radio_button_update (GtkWidget *widget,
                           gpointer   data)
{
  gimp_radio_button_update (widget, data);

  if (GTK_TOGGLE_BUTTON (widget)->active)
    waves_do_preview ();
}

static void
waves_double_adjustment_update (GtkAdjustment *adjustment,
                                gpointer   data)
{
  gimp_double_adjustment_update (adjustment, data);
  waves_do_preview ();
}

static gint
pluginCoreIA (piArgs *argp,
              GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *sep;
  GtkWidget *table;
  GtkWidget *preview;
  GtkWidget *toggle;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init ("waves", TRUE);

  dlg = gimp_dialog_new (_("Waves"), "waves",
                         NULL, 0,
                         gimp_standard_help_func, "filters/waves.html",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  preview = mw_preview_new (hbox, drawable);
  g_object_set_data (G_OBJECT (preview), "piArgs", argp);
  waves_do_preview ();

  frame = gimp_int_radio_group_new (TRUE, _("Mode"),
                                    G_CALLBACK (waves_radio_button_update),
                                    &argp->type, argp->type,

                                    _("_Smear"),   MODE_SMEAR,   NULL,
                                    _("_Blacken"), MODE_BLACKEN, NULL,

                                    NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = GTK_BIN (frame)->child;

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 2);
  gtk_widget_show (sep);

  toggle = gtk_check_button_new_with_mnemonic ( _("_Reflective"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), argp->reflective);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (waves_toggle_button_update),
                    &argp->reflective);

  frame = gtk_frame_new ( _("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("_Amplitude:"), 140, 6,
                              argp->amplitude, 0.0, 101.0, 1.0, 5.0, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (waves_double_adjustment_update),
                    &argp->amplitude);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("_Phase:"), 140, 6,
                              argp->phase, 0.0, 360.0, 2.0, 5.0, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (waves_double_adjustment_update),
                    &argp->phase);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                              _("_Wavelength:"), 140, 6,
                              argp->wavelength, 0.1, 50.0, 1.0, 5.0, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (waves_double_adjustment_update),
                    &argp->wavelength);

  gtk_widget_show (table);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run ? pluginCore (argp, drawable) : -1;
}

static void
waves_do_preview (void)
{
  piArgs *argp;
  guchar *dst;
  gint y;

  if (!do_preview)
    return;

  argp = g_object_get_data (G_OBJECT (preview->widget), "piArgs");
  dst = g_new (guchar, preview->width * preview->height * preview->bpp);

  wave (preview->cache, dst, preview->width, preview->height, preview->bpp,
        preview->bpp == 2 || preview->bpp == 4,
        argp->amplitude * preview->scale_x,
        argp->wavelength * preview->scale_x,
        argp->phase, argp->type == 0, argp->reflective, 0);

  for (y = 0; y < preview->height; y++)
    {
      gimp_old_preview_do_row (preview, y, preview->width,
                                 dst + y * preview->rowstride);
    }

  gtk_widget_queue_draw (preview->widget);

  g_free (dst);
}

static void
mw_preview_toggle_callback (GtkWidget *widget,
                            gpointer   data)
{
  gimp_toggle_button_update (widget, data);

  waves_do_preview ();
}

static GtkWidget *
mw_preview_new (GtkWidget *parent, GimpDrawable *drawable)
{
  GtkWidget *frame;
  GtkWidget *pframe;
  GtkWidget *vbox;
  GtkWidget *button;

  frame = gtk_frame_new (_("Preview"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (parent), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  pframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME(pframe), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), pframe, FALSE, FALSE, 0);
  gtk_widget_show (pframe);

  preview = gimp_old_preview_new (drawable, FALSE);
  /* FIXME: this forces gimp_old_preview to set its alpha correctly */
  gimp_old_preview_fill_scaled (preview, drawable);
  gtk_container_add (GTK_CONTAINER (pframe), preview->widget);
  gtk_widget_show (preview->widget);

  button = gtk_check_button_new_with_mnemonic (_("_Do Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), do_preview);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (mw_preview_toggle_callback),
                    &do_preview);

  return preview->widget;
}

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

static void
wave (guchar  *src,
      guchar  *dst,
      gint     width,
      gint     height,
      gint     bypp,
      gboolean has_alpha,
      gdouble  amplitude,
      gdouble  wavelength,
      gdouble  phase,
      gint     smear,
      gint     reflective,
      gint     verbose)
{
  glong   rowsiz;
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

  guchar *values[4];
  guchar  zeroes[4] = { 0, 0, 0, 0 };

  phase = phase * G_PI / 180;
  rowsiz   = width * bypp;

  if (verbose)
    {
      gimp_progress_init (_("Waving..."));
      prog_interval=height/10;
    }

  x1 = y1 = 0;
  x2 = width;
  y2 = height;

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

  radius  = MAX (xhsiz, yhsiz);
  radius2 = radius * radius;

  /* Wave the image! */

  dst += y1 * rowsiz + x1 * bypp;

  wavelength *= 2;

  for (y = y1; y < y2; y++)
    {
      dest = dst;

      if (verbose && (y % prog_interval == 0))
        gimp_progress_update ((double) y / (double) height);

      for (x = x1; x < x2; x++)
        {
          /* Distance from current point to wave center, scaled */
          dx = (x - cen_x) * xscale;
          dy = (y - cen_y) * yscale;

          /* Distance^2 to center of *circle* (our scaled ellipse) */
          d = sqrt (dx * dx + dy * dy);

          /* Use the formula described above. */

          /* Calculate waved point and scale again to ellipsify */

          /*
           * Reflective waves are strange - the effect is much
           * more like a mirror which is in the shape of
           * the wave than anything else.
           */

          if (reflective)
            {
              amnt = amplitude * fabs (sin (((d / wavelength) * (2.0 * G_PI) +
                                             phase)));

              needx = (amnt * dx) / xscale + cen_x;
              needy = (amnt * dy) / yscale + cen_y;
            }
          else
            {
              amnt = amplitude * sin (((d / wavelength) * (2.0 * G_PI) +
                                       phase));

              needx = (amnt + dx) / xscale + cen_x;
              needy = (amnt + dy) / yscale + cen_y;
            }

          /* Calculations complete; now copy the proper pixel */

          if (smear)
            {
              xi = CLAMP (needx, 0, width - 2);
              yi = CLAMP (needy, 0, height - 2);
            }
          else
            {
              xi = needx;
              yi = needy;
            }

          p = src + rowsiz * yi + xi * bypp;

          x1_in = WITHIN (0, xi, width - 1);
          y1_in = WITHIN (0, yi, height - 1);
          x2_in = WITHIN (0, xi + 1, width - 1);
          y2_in = WITHIN (0, yi + 1, height - 1);

          if (x1_in && y1_in)
            values[0] = p;
          else
            values[0] = zeroes;

          if (x2_in && y1_in)
            values[1] = p + bypp;
          else
            values[1] = zeroes;

          if (x1_in && y2_in)
            values[2] = p + rowsiz;
          else
            values[2] = zeroes;

          if (x2_in && y2_in)
            values[3] = p + bypp + rowsiz;
          else
            values[3] = zeroes;

          gimp_bilinear_pixels_8 (dest, needx, needy, bypp, has_alpha, values);
          dest += bypp;
        }

      dst += rowsiz;
    }

  if (verbose)
    gimp_progress_update (1.0);
}
