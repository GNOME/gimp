/**************************************************
 * file: emboss/emboss.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


enum
{
  FUNCTION_BUMPMAP = 0,
  FUNCTION_EMBOSS  = 1
};

typedef struct
{
  gint32 img;
  gint32 drw;
  gdouble azimuth;
  gdouble elevation;
  gint32 depth;
  gint32 embossp;
} piArgs;

struct embossFilter
{
  gdouble Lx;
  gdouble Ly;
  gdouble Lz;
  gdouble Nz;
  gdouble Nz2;
  gdouble NzLz;
  gdouble bg;
} Filter;

/*  preview stuff -- to be removed as soon as we have a real libgimp preview  */

typedef struct 
{
  gint     width;
  gint     height;
  gint     bpp;
  gdouble  scale;
  guchar  *bits;
} mwPreview;

#define PREVIEW_SIZE 100

static gint do_preview = TRUE;

static GtkWidget *mw_preview_new   (GtkWidget        *parent,
				    mwPreview *mwp);
static mwPreview *mw_preview_build (GimpDrawable        *drw);



static void query (void);
static void run   (const gchar      *name,
		   gint              nparam,
		   const GimpParam  *param,
		   gint             *nretvals,
		   GimpParam       **retvals);

static gint pluginCore        (piArgs *argp);
static gint pluginCoreIA      (piArgs *argp);

static void emboss_do_preview (GtkWidget     *preview);

static inline void EmbossInit (gdouble  azimuth,
			       gdouble  elevation,
			       gushort  width45);
static inline void EmbossRow  (guchar  *src,
			       guchar  *texture,
			       guchar  *dst,
			       guint    xSize,
			       guint    bypp,
			       gint     alpha);

#define DtoR(d) ((d)*(G_PI/(gdouble)180))

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init  */
  NULL,  /* quit  */
  query, /* query */
  run,   /* run   */
};

static mwPreview *thePreview;

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "img", "The Image" },
    { GIMP_PDB_DRAWABLE, "drw", "The Drawable" },
    { GIMP_PDB_FLOAT, "azimuth", "The Light Angle (degrees)" },
    { GIMP_PDB_FLOAT, "elevation", "The Elevation Angle (degrees)" },
    { GIMP_PDB_INT32, "depth", "The Filter Width" },
    { GIMP_PDB_INT32, "embossp", "Emboss or Bumpmap" }
  };

  gimp_install_procedure ("plug_in_emboss",
			  "Emboss filter",
			  "Emboss or Bumpmap the given drawable, specifying "
			  "the angle and elevation for the light source.",
			  "Eric L. Hernes, John Schlag",
			  "Eric L. Hernes",
			  "1997",
			  N_("<Image>/Filters/Distorts/_Emboss..."),
			  "RGB*",
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
  piArgs            args;
  GimpDrawable     *drw;

  *nretvals = 1;
  *retvals = rvals;

  INIT_I18N ();

  memset(&args, (int) 0, sizeof (piArgs));

  rvals[0].type = GIMP_PDB_STATUS;
  rvals[0].data.d_status = GIMP_PDB_SUCCESS;

  switch (param[0].data.d_int32)
    {
    case GIMP_RUN_INTERACTIVE:
      args.img = args.drw = 0;
      gimp_get_data ("plug_in_emboss", &args);
      if (args.img == 0 && args.drw == 0)
	{
	  /* initial conditions */
	  args.azimuth = 30.0;
	  args.elevation = 45.0;
	  args.depth = 20;
	  args.embossp = FUNCTION_EMBOSS;
	}
      args.img = param[1].data.d_image;
      args.drw = param[2].data.d_drawable;
      drw = gimp_drawable_get (args.drw);
      thePreview = mw_preview_build (drw);

      if (pluginCoreIA (&args) == -1)
	{
	  rvals[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
	}
      else
	{
	  gimp_set_data ("plug_in_emboss", &args, sizeof (piArgs));
	}

      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparam != 7)
	{
	  rvals[0].data.d_status = GIMP_PDB_CALLING_ERROR;
	  break;
	}

      args.img = param[1].data.d_image;
      args.drw = param[2].data.d_drawable;
      args.azimuth = param[3].data.d_float;
      args.elevation = param[4].data.d_float;
      args.depth = param[5].data.d_int32;
      args.embossp = param[6].data.d_int32;

      if (pluginCore(&args)==-1)
	{
	  rvals[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
	  break;
	}
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data("plug_in_emboss", &args);
      /* use this image and drawable, even with last args */
      args.img = param[1].data.d_image;
      args.drw = param[2].data.d_drawable;
      if (pluginCore(&args)==-1) {
        rvals[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
      }
    break;
  }
}

#define pixelScale 255.9

static void
EmbossInit (gdouble azimuth,
	    gdouble elevation,
	    gushort width45)
{
  /*
   * compute the light vector from the input parameters.
   * normalize the length to pixelScale for fast shading calculation.
   */
  Filter.Lx = cos (azimuth) * cos (elevation) * pixelScale;
  Filter.Ly = sin (azimuth) * cos (elevation) * pixelScale;
  Filter.Lz = sin (elevation) * pixelScale;

  /*
   * constant z component of image surface normal - this depends on the
   * image slope we wish to associate with an angle of 45 degrees, which
   * depends on the width of the filter used to produce the source image.
   */
  Filter.Nz = (6 * 255) / width45;
  Filter.Nz2 = Filter.Nz * Filter.Nz;
  Filter.NzLz = Filter.Nz * Filter.Lz;

  /* optimization for vertical normals: L.[0 0 1] */
  Filter.bg = Filter.Lz;
}


/*
 * ANSI C code from the article
 * "Fast Embossing Effects on Raster Image Data"
 * by John Schlag, jfs@kerner.com
 * in "Graphics Gems IV", Academic Press, 1994
 *
 *
 * Emboss - shade 24-bit pixels using a single distant light source.
 * Normals are obtained by differentiating a monochrome 'bump' image.
 * The unary case ('texture' == NULL) uses the shading result as output.
 * The binary case multiples the optional 'texture' image by the shade.
 * Images are in row major order with interleaved color components (rgbrgb...).
 * E.g., component c of pixel x,y of 'dst' is dst[3*(y*xSize + x) + c].
 *
 */

static inline void
EmbossRow (guchar *src,
	   guchar *texture,
	   guchar *dst,
	   guint   xSize,
	   guint   bypp,
	   gint    alpha)
{
  glong Nx, Ny, NdotL;
  guchar *s1, *s2, *s3;
  gint x, shade, b;
  gint bytes;

  /* mung pixels, avoiding edge pixels */
  s1 = src + bypp;
  s2 = s1 + (xSize * bypp);
  s3 = s2 + (xSize * bypp);
  dst += bypp;

  bytes = (alpha) ? bypp - 1 : bypp;

  if (texture)
    texture += bypp;

  for (x = 1; x < xSize - 1; x++, s1 += bypp, s2 += bypp, s3 += bypp)
    {
      /*
       * compute the normal from the src map. the type of the
       * expression before the cast is compiler dependent. in
       * some cases the sum is unsigned, in others it is
       * signed. ergo, cast to signed.
       */
      Nx = (int) (s1[-(int)bypp] + s2[-(int)bypp] + s3[-(int)bypp]
		  - s1[bypp] - s2[bypp] - s3[bypp]);
      Ny = (int) (s3[-(int)bypp] + s3[0] + s3[bypp] - s1[-(int)bypp]
		  - s1[0] - s1[bypp]);

      /* shade with distant light source */
      if ( Nx == 0 && Ny == 0 )
	shade = Filter.bg;
      else if ( (NdotL = Nx * Filter.Lx + Ny * Filter.Ly + Filter.NzLz) < 0 )
	shade = 0;
      else
	shade = NdotL / sqrt(Nx*Nx + Ny*Ny + Filter.Nz2);

      /* do something with the shading result */
      if (texture)
	{
	  for (b = 0; b < bytes; b++)
	    {
	      *dst++ = (*texture++ * shade) >> 8;
	    }
	  if (alpha)
	    {
	      *dst++ = s2[bytes]; /* preserve the alpha */
	      texture++;
	    }
	}
      else
	{
	  for (b = 0; b < bytes; b++)
	    {
	      *dst++ = shade;
	    }
	  if (alpha)
	    *dst++ = s2[bytes]; /* preserve the alpha */
	}
    }
  if (texture)
    texture += bypp;
}

static gint
pluginCore (piArgs *argp)
{
  GimpDrawable *drw;
  GimpPixelRgn src, dst;
  gint p_update;
  gint y;
  gint x1, y1, x2, y2;
  guint width, height;
  gint bypp, rowsize, has_alpha;
  guchar *srcbuf, *dstbuf;

  drw = gimp_drawable_get (argp->drw);

  gimp_drawable_mask_bounds (argp->drw, &x1, &y1, &x2, &y2);

  /* expand the bounds a little */
  x1 = MAX (0, x1 - argp->depth);
  y1 = MAX (0, y1 - argp->depth);
  x2 = MIN (drw->width, x2 + argp->depth);
  y2 = MIN (drw->height, y2 + argp->depth);

  width = x2 - x1;
  height = y2 - y1;
  bypp = drw->bpp;
  p_update = height / 20;
  rowsize = width * bypp;
  has_alpha = gimp_drawable_has_alpha (argp->drw);

  gimp_pixel_rgn_init (&src, drw, x1, y1, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dst, drw, x1, y1, width, height, TRUE, TRUE);

  srcbuf = g_new0 (guchar, rowsize * 3);
  dstbuf = g_new0 (guchar, rowsize);

  EmbossInit (DtoR(argp->azimuth), DtoR(argp->elevation), argp->depth);
  gimp_progress_init (_("Emboss"));

  gimp_tile_cache_ntiles ((width + gimp_tile_width () - 1) / gimp_tile_width ());

  /* first row */
  gimp_pixel_rgn_get_rect (&src, srcbuf, x1, y1, width, 3);
  memcpy (srcbuf, srcbuf + rowsize, rowsize);
  EmbossRow (srcbuf, argp->embossp ? (guchar *) 0 : srcbuf,
	     dstbuf, width, bypp, has_alpha);
  gimp_pixel_rgn_set_row (&dst, dstbuf, 0, 0, width);

  /* last row */
  gimp_pixel_rgn_get_rect (&src, srcbuf, x1, y2-3, width, 3);
  memcpy (srcbuf + rowsize * 2, srcbuf + rowsize, rowsize);
  EmbossRow (srcbuf, argp->embossp ? (guchar *) 0 : srcbuf,
	     dstbuf, width, bypp, has_alpha);
  gimp_pixel_rgn_set_row (&dst, dstbuf, x1, y2-1, width);

  for (y = 0; y < height - 2; y++)
    {
      if (y % p_update == 0)
	  gimp_progress_update ((gdouble) y / (gdouble) height);

      gimp_pixel_rgn_get_rect (&src, srcbuf, x1, y1+y, width, 3);
      EmbossRow (srcbuf, argp->embossp ? (guchar *) 0 : srcbuf,
		 dstbuf, width, bypp, has_alpha);
     gimp_pixel_rgn_set_row (&dst, dstbuf, x1, y1+y+1, width);
  }
  gimp_progress_update (1.0);

  g_free (srcbuf);
  g_free (dstbuf);

  gimp_drawable_flush (drw);
  gimp_drawable_merge_shadow (drw->drawable_id, TRUE);
  gimp_drawable_update (drw->drawable_id, x1, y1, width, height);
  gimp_displays_flush ();

  return 0;
}

static void
emboss_radio_button_callback (GtkWidget *widget,
			      gpointer   data)
{
  gimp_radio_button_update (widget, data);

  if (do_preview && GTK_TOGGLE_BUTTON (widget)->active)
    emboss_do_preview (NULL);
}

static void
emboss_float_adjustment_callback (GtkAdjustment *adj,
				  gpointer       data)
{
  gdouble *value;

  value = (gdouble *) data;

  *value = adj->value;

  if (do_preview)
    emboss_do_preview (NULL);
}

static void
emboss_int_adjustment_callback (GtkAdjustment *adj,
				gpointer       data)
{
  gint *value;

  value = (gint *) data;

  *value = (gint) adj->value;

  if (do_preview)
    emboss_do_preview (NULL);
}

static gint
pluginCoreIA (piArgs *argp)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *preview;
  GtkWidget *frame;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init ("emboss", TRUE);

  dlg = gimp_dialog_new (_("Emboss"), "emboss",
                         NULL, 0,
			 gimp_standard_help_func, "filters/emboss.html",

			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK,     GTK_RESPONSE_OK,

			 NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox,
		      TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  preview = mw_preview_new (hbox, thePreview);
  g_object_set_data (G_OBJECT (preview), "piArgs", argp);
  emboss_do_preview (preview);

  frame = gimp_radio_group_new2 (TRUE, _("Function"),
				 G_CALLBACK (emboss_radio_button_callback),
				 &argp->embossp, (gpointer) argp->embossp,

				 _("_Bumpmap"), (gpointer) FUNCTION_BUMPMAP, NULL,
				 _("_Emboss"), (gpointer) FUNCTION_EMBOSS, NULL,

				 NULL);

  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gimp_parameter_settings_new (main_vbox, 3, 3);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("_Azimuth:"), 100, 6,
			      argp->azimuth, 0.0, 360.0, 1.0, 10.0, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (emboss_float_adjustment_callback),
                    &argp->azimuth);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("E_levation:"), 100, 6,
			      argp->elevation, 0.0, 180.0, 1.0, 10.0, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (emboss_float_adjustment_callback),
                    &argp->elevation);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("_Depth:"), 100, 6,
			      argp->depth, 1.0, 100.0, 1.0, 5.0, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (emboss_int_adjustment_callback),
                    &argp->depth);

  gtk_widget_show (table);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  if (run)
    return pluginCore (argp);
  else
    return -1;
}

static void
emboss_do_preview (GtkWidget *w)
{
  static GtkWidget *theWidget = NULL;
  piArgs *ap;
  guchar *dst, *c;
  gint y, rowsize;

  if (theWidget == NULL)
    {
      theWidget = w;
    }

  ap = g_object_get_data (G_OBJECT (theWidget), "piArgs");
  rowsize = thePreview->width * thePreview->bpp;

  dst = g_malloc (rowsize);
  c = g_malloc (rowsize * 3);
  memcpy (c, thePreview->bits, rowsize);
  memcpy (c + rowsize, thePreview->bits, rowsize * 2);
  EmbossInit (DtoR (ap->azimuth), DtoR (ap->elevation), ap->depth);

  EmbossRow (c, ap->embossp ? (guchar *) 0 : c,
             dst, thePreview->width, thePreview->bpp, FALSE);
  gtk_preview_draw_row (GTK_PREVIEW (theWidget),
                        dst, 0, 0, thePreview->width);

  memcpy (c,
	  thePreview->bits + ((thePreview->height-2) * rowsize),
	  rowsize * 2);
  memcpy (c + (rowsize * 2),
	  thePreview->bits + ((thePreview->height - 1) * rowsize),
          rowsize);
  EmbossRow (c, ap->embossp ? (guchar *) 0 : c,
             dst, thePreview->width, thePreview->bpp, FALSE);
  gtk_preview_draw_row (GTK_PREVIEW (theWidget),
                        dst, 0, thePreview->height - 1, thePreview->width);
  g_free (c);

  for (y = 0, c = thePreview->bits; y<thePreview->height - 2; y++, c += rowsize)
    {
      EmbossRow (c, ap->embossp ? (guchar *) 0 : c,
		 dst, thePreview->width, thePreview->bpp, FALSE);
      gtk_preview_draw_row (GTK_PREVIEW (theWidget),
			    dst, 0, y, thePreview->width);
    }

  gtk_widget_queue_draw (theWidget);

  g_free (dst);
}

static void
mw_preview_toggle_callback (GtkWidget *widget,
                            gpointer   data)
{
  gimp_toggle_button_update (widget, data);

  if (do_preview)
    emboss_do_preview (NULL);
}

static mwPreview *
mw_preview_build_virgin (GimpDrawable *drw)
{
  mwPreview *mwp;

  mwp = g_new (mwPreview, 1);

  if (drw->width > drw->height)
    {
      mwp->scale  = (gdouble) drw->width / (gdouble) PREVIEW_SIZE;
      mwp->width  = PREVIEW_SIZE;
      mwp->height = drw->height / mwp->scale;
    }
  else
    {
      mwp->scale  = (gdouble) drw->height / (gdouble) PREVIEW_SIZE;
      mwp->height = PREVIEW_SIZE;
      mwp->width  = drw->width / mwp->scale;
    }

  mwp->bpp  = 3;
  mwp->bits = NULL;

  return mwp;
}

static mwPreview *
mw_preview_build (GimpDrawable *drw)
{
  mwPreview *mwp;
  gint x, y, b;
  guchar *bc;
  guchar *drwBits;
  GimpPixelRgn pr;

  mwp = mw_preview_build_virgin (drw);

  gimp_pixel_rgn_init (&pr, drw, 0, 0, drw->width, drw->height, FALSE, FALSE);
  drwBits = g_new (guchar, drw->width * drw->bpp);

  bc = mwp->bits = g_new (guchar, mwp->width * mwp->height * mwp->bpp);
  for (y = 0; y < mwp->height; y++)
    {
      gimp_pixel_rgn_get_row (&pr, drwBits, 0, (int)(y*mwp->scale), drw->width);

      for (x = 0; x < mwp->width; x++)
        {
          for (b = 0; b < mwp->bpp; b++)
            *bc++ = *(drwBits +
                      ((gint) (x * mwp->scale) * drw->bpp) + b % drw->bpp);
        }
    }
  g_free (drwBits);

  return mwp;
}

static GtkWidget *
mw_preview_new (GtkWidget        *parent,
                mwPreview *mwp)
{
  GtkWidget *preview;
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

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), mwp->width, mwp->height);
  gtk_container_add (GTK_CONTAINER (pframe), preview);
  gtk_widget_show (preview);

  button = gtk_check_button_new_with_mnemonic (_("Do _Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), do_preview);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (mw_preview_toggle_callback),
                    &do_preview);

  return preview;
}
