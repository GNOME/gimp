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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>

#include <libgimp/gimp.h>

#include <gtk/gtk.h>

#include <plug-ins/megawidget/megawidget.h>
static mw_preview_t emboss_do_preview;

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
  gint32 img;
  gint32 drw;
  gdouble azimuth;
  gdouble elevation;
  gint32 depth;
  gint32 embossp;
};

struct embossFilter {
   gdouble Lx;
   gdouble Ly;
   gdouble Lz;
   gdouble Nz;
   gdouble Nz2;
   gdouble NzLz;
   gdouble bg;
} Filter;

static void query(void);
static void run(gchar *name, gint nparam, GParam *param,
                gint *nretvals, GParam **retvals);

int pluginCore(struct piArgs *argp);
int pluginCoreIA(struct piArgs *argp);


static inline void EmbossInit(gdouble azimuth, gdouble elevation,
                              gushort width45);
static inline void EmbossRow(guchar *src, guchar *texture, guchar *dst,
                             guint xSize, guint bypp, gint alpha);

#define DtoR(d) ((d)*(M_PI/(gdouble)180))

GPlugInInfo PLUG_IN_INFO = {
  NULL, /* init */
  NULL, /* quit */
  query, /* query */
  run, /* run */
};

static struct mwPreview *thePreview;

MAIN()

static void
query(void){
  static GParamDef args[] = {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "img", "The Image" },
    { PARAM_DRAWABLE, "drw", "The Drawable" },
    { PARAM_FLOAT, "azimuth", "The Light Angle (degrees)" },
    { PARAM_FLOAT, "elevation", "The Elevation Angle (degrees)" },
    { PARAM_INT32, "depth", "The Filter Width" },
    { PARAM_INT32, "embossp", "Emboss or Bumpmap" },
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  static GParamDef *rets = NULL;
  static gint nrets = 0;

  gimp_install_procedure("plug_in_emboss",
                         "Emboss filter",
                         "Emboss or Bumpmap the given drawable, specifying the angle and elevation for the light source.",
                         "Eric L. Hernes, John Schlag",
                         "Eric L. Hernes",
                         "1997",
                         "<Image>/Filters/Distorts/Emboss",
                         "RGB",
                         PROC_PLUG_IN,
                         nargs, nrets,
                         args, rets);
}

static void
run(gchar *name, gint nparam, GParam *param,
    gint *nretvals, GParam **retvals)
{
  static GParam rvals[1];
  struct piArgs args;
  GDrawable *drw;

  *nretvals = 1;
  *retvals = rvals;

  /* bzero(&args, sizeof(struct piArgs)); */
  memset(&args,(int)0,sizeof(struct piArgs));

  rvals[0].type = PARAM_STATUS;
  rvals[0].data.d_status = STATUS_SUCCESS;
  switch (param[0].data.d_int32)
    {
    case RUN_INTERACTIVE:
      gimp_get_data("plug_in_emboss", &args);
      if (args.img == 0 && args.drw == 0)
	{
	  /* initial conditions */
	  args.azimuth = 30.0;
	  args.elevation = 45.0;
	  args.depth = 20;
	  args.embossp = 1;
	}

      args.img = param[1].data.d_image;
      args.drw = param[2].data.d_drawable;
      drw = gimp_drawable_get(args.drw);
      thePreview = mw_preview_build(drw);

      if (pluginCoreIA(&args)==-1)
	{
	  rvals[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
      else
	{
	  gimp_set_data("plug_in_emboss", &args, sizeof(struct piArgs));
	}

      break;

    case RUN_NONINTERACTIVE:
      if (nparam != 7)
	{
	  rvals[0].data.d_status = STATUS_CALLING_ERROR;
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
	  rvals[0].data.d_status = STATUS_EXECUTION_ERROR;
	  break;
	}
    break;

    case RUN_WITH_LAST_VALS:
      gimp_get_data("plug_in_emboss", &args);
      /* use this image and drawable, even with last args */
      args.img = param[1].data.d_image;
      args.drw = param[2].data.d_drawable;
      if (pluginCore(&args)==-1) {
        rvals[0].data.d_status = STATUS_EXECUTION_ERROR;
      }
    break;
  }
}

#define pixelScale 255.9
void
EmbossInit(gdouble azimuth, gdouble elevation, gushort width45) {
       /*
        * compute the light vector from the input parameters.
        * normalize the length to pixelScale for fast shading calculation.
        */
   Filter.Lx = cos(azimuth) * cos(elevation) * pixelScale;
   Filter.Ly = sin(azimuth) * cos(elevation) * pixelScale;
   Filter.Lz = sin(elevation) * pixelScale;

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

void
EmbossRow(guchar *src, guchar *texture, guchar *dst,
          guint xSize, guint bypp, gint alpha) {
   glong Nx, Ny, NdotL;
   guchar *s1, *s2, *s3;
   gint x, shade, b;
   gint bytes;

       /* mung pixels, avoiding edge pixels */
   s1 = src + bypp;
   s2 = s1 + (xSize*bypp);
   s3 = s2 + (xSize*bypp);
   dst += bypp;

   bytes = (alpha) ? bypp - 1 : bypp;

   if (texture) texture += bypp;
   for (x = 1; x < xSize-1; x++, s1+=bypp, s2+=bypp, s3+=bypp) {
          /*
           * compute the normal from the src map. the type of the
           * expression before the cast is compiler dependent. in
           * some cases the sum is unsigned, in others it is
           * signed. ergo, cast to signed.
           */
      Nx = (int)(s1[-bypp] + s2[-bypp] + s3[-bypp]
                 - s1[bypp] - s2[bypp] - s3[bypp]);
      Ny = (int)(s3[-bypp] + s3[0] + s3[bypp] - s1[-bypp]
                 - s1[0] - s1[bypp]);

          /* shade with distant light source */
      if ( Nx == 0 && Ny == 0 )
         shade = Filter.bg;
      else if ( (NdotL = Nx*Filter.Lx + Ny*Filter.Ly + Filter.NzLz) < 0 )
         shade = 0;
      else
         shade = NdotL / sqrt(Nx*Nx + Ny*Ny + Filter.Nz2);

          /* do something with the shading result */
      if ( texture ) {
         for(b=0;b<bytes;b++) {
            *dst++ = (*texture++ * shade) >> 8;
         }
         if (alpha) {
            *dst++=s2[bytes]; /* preserve the alpha */
            texture++;
         }
      }
      else {
         for(b=0;b<bytes;b++) {
            *dst++ = shade;
         }
         if (alpha) *dst++=s2[bytes]; /* preserve the alpha */

      }
   }
   if (texture) texture += bypp;
}

int pluginCore(struct piArgs *argp) {
  GDrawable *drw;
  GPixelRgn src, dst;
  gint p_update;
  gint y;
  guint width, height;
  gint bypp, rowsize, has_alpha;
  guchar *srcbuf, *dstbuf;

  drw=gimp_drawable_get(argp->drw);

  width=drw->width;
  height=drw->height;
  bypp=drw->bpp;
  p_update = height/20;
  rowsize = width * bypp;
  has_alpha = gimp_drawable_has_alpha (argp->drw);

  gimp_pixel_rgn_init (&src, drw, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dst, drw, 0, 0, width, height, TRUE, TRUE);

  srcbuf=(guchar*)malloc(rowsize*3);
  dstbuf=(guchar*)malloc(rowsize);

  /* bzero(srcbuf, rowsize*3); */
  memset(srcbuf,(int)0,(size_t)(rowsize*3));
  /* bzero(dstbuf, rowsize); */
  memset(dstbuf,(int)0,(size_t)rowsize);

  EmbossInit(DtoR(argp->azimuth), DtoR(argp->elevation), argp->depth);
  gimp_progress_init("Emboss");

  gimp_tile_cache_ntiles((width + gimp_tile_width() - 1) / gimp_tile_width());

  /* first row */
  gimp_pixel_rgn_get_rect(&src, srcbuf, 0, 0, width, 3);
  bcopy(srcbuf+rowsize, srcbuf, rowsize);
  EmbossRow(srcbuf, argp->embossp ? (guchar *)0 : srcbuf,
            dstbuf, width, bypp, has_alpha);
  gimp_pixel_rgn_set_row(&dst, dstbuf, 0, 0, width);

  /* last row */
  gimp_pixel_rgn_get_rect(&src, srcbuf, 0, height-3, width, 3);
  bcopy(srcbuf+rowsize, srcbuf+rowsize*2, rowsize);
  EmbossRow(srcbuf, argp->embossp ? (guchar *)0 : srcbuf,
            dstbuf, width, bypp, has_alpha);
  gimp_pixel_rgn_set_row(&dst, dstbuf, 0, height-1, width);

  for(y=0 ;y<height-2; y++){
     if (y%p_update==0) gimp_progress_update((gdouble)y/(gdouble)height);
     gimp_pixel_rgn_get_rect(&src, srcbuf, 0, y, width, 3);
     EmbossRow(srcbuf, argp->embossp ? (guchar *)0 : srcbuf,
               dstbuf, width, bypp, has_alpha);
     gimp_pixel_rgn_set_row(&dst, dstbuf, 0, y+1, width);
  }

  free(srcbuf);
  free(dstbuf);

  gimp_drawable_flush(drw);
  gimp_drawable_merge_shadow (drw->id, TRUE);
  gimp_drawable_update(drw->id, 0, 0, width, height);
  gimp_displays_flush();

  return 0;
}

int pluginCoreIA(struct piArgs *argp) {
  GtkWidget *dlg;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *preview;
  GtkWidget *frame;

  gint runp;
  struct mwRadioGroup functions[] = {
     { "Emboss", 0 },
     { "Bumpmap", 0 },
     { NULL, 0 },
  };
  gchar **argv;
  gint argc;
 
  /* Set args */
  argc = 1;
  argv = g_new(gchar *, 1);
  argv[0] = g_strdup("emboss");
  gtk_init(&argc, &argv);
  gtk_rc_parse(gimp_gtkrc ());

  functions[!argp->embossp].var = 1;

  dlg = mw_app_new("plug_in_emboss", "Emboss", &runp);

  hbox = gtk_hbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(hbox), 5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);

  preview = mw_preview_new(hbox, thePreview, &emboss_do_preview);
  gtk_object_set_data(GTK_OBJECT(preview), "piArgs", argp);
  gtk_object_set_data(GTK_OBJECT(preview), "mwRadioGroup", &functions);
  emboss_do_preview(preview);

  mw_radio_group_new(hbox, "Function", functions);

  frame = gtk_frame_new("Parameters");
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);

  table = gtk_table_new(4, 2, FALSE);
  gtk_container_border_width(GTK_CONTAINER (table), 5);
  gtk_container_add(GTK_CONTAINER(frame), table);


  mw_fscale_entry_new(table, "Azimuth", 0.0, 360.0, 1.0, 10.0, 0.0,
                      0, 1, 1, 2, &argp->azimuth);
  mw_fscale_entry_new(table, "Elevation", 0.0, 180.0, 1.0, 10.0, 0.0,
                      0, 1, 2, 3, &argp->elevation);
  mw_iscale_entry_new(table, "Depth", 1, 100, 1, 5, 0,
                      0, 1, 3, 4, &argp->depth);
  gtk_widget_show(table);

  gtk_widget_show(table);
  gtk_widget_show(dlg);
  gtk_main();
  gdk_flush();

  argp->embossp = !mw_radio_result(functions);
  if(runp){
    return pluginCore(argp);
  } else {
    return -1;
  }
}



static void
emboss_do_preview(GtkWidget *w) {
   static GtkWidget *theWidget = NULL;
   struct piArgs *ap;
   struct mwRadioGroup *rgp;
   guchar *dst, *c;
   gint y, rowsize;

   if(theWidget==NULL){
      theWidget=w;
   }

   ap = gtk_object_get_data(GTK_OBJECT(theWidget), "piArgs");
   rgp = gtk_object_get_data(GTK_OBJECT(theWidget), "mwRadioGroup");
   ap->embossp = !mw_radio_result(rgp);
   rowsize=thePreview->width*thePreview->bpp;

   dst = malloc(rowsize);
   c = malloc(rowsize*3);
   bcopy(thePreview->bits, c, rowsize);
   bcopy(thePreview->bits, c+rowsize, rowsize*2);
   EmbossInit(DtoR(ap->azimuth), DtoR(ap->elevation), ap->depth);

   EmbossRow(c, ap->embossp ? (guchar *)0 : c,
             dst, thePreview->width, thePreview->bpp, FALSE);
   gtk_preview_draw_row(GTK_PREVIEW(theWidget),
                        dst, 0, 0, thePreview->width);

   bcopy(thePreview->bits+((thePreview->height-2)*rowsize), c, rowsize*2);
   bcopy(thePreview->bits+((thePreview->height-1)*rowsize),
         c+(rowsize*2), rowsize);
   EmbossRow(c, ap->embossp ? (guchar *)0 : c,
             dst, thePreview->width, thePreview->bpp, FALSE);
   gtk_preview_draw_row(GTK_PREVIEW(theWidget),
                        dst, 0, thePreview->height-1, thePreview->width);
   free(c);

   for(y=0, c=thePreview->bits;y<thePreview->height-2; y++, c+=rowsize){
      EmbossRow(c, ap->embossp ? (guchar *)0 : c,
                dst, thePreview->width, thePreview->bpp, FALSE);
      gtk_preview_draw_row(GTK_PREVIEW(theWidget),
                           dst, 0, y, thePreview->width);
   }

   gtk_widget_draw(theWidget, NULL);
   gdk_flush();
   free(dst);
}

/*
 * Local Variables:
 * mode: C
 * c-indent-level: 2
 *
 * End:
 */

/* end of file: emboss/emboss.c */
