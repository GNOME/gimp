/*
 * file: hot/hot.c
 *
 * $Id$
 */

/*
 * hot.c - Scan an image for pixels with RGB values that will give
 *	"unsafe" values of chrominance signal or composite signal
 *	amplitude when encoded into an NTSC or PAL colour signal.
 *	(This happens for certain high-intensity high-saturation colours
 *	that are rare in real scenes, but can easily be present
 *	in synthetic images.)
 *
 * 	Such pixels can be flagged so the user may then choose other
 *	colours.  Or, the offending pixels can be made "safe"
 *	in a manner that preserves hue.
 *
 *	There are two reasonable ways to make a pixel "safe":
 *	We can reduce its intensity (luminance) while leaving
 *	hue and saturation the same.  Or, we can reduce saturation
 *	while leaving hue and luminance the same.  A #define selects
 *	which strategy to use.
 *
 * Note to the user: You must add your own read_pixel() and write_pixel()
 *	routines.  You may have to modify pix_decode() and pix_encode().
 *	MAXPIX, WID, and HGT are likely to need modification.
 */

/*
 * Originally written as "ikNTSC.c" by Alan Wm Paeth,
 *	University of Waterloo, August, 1985
 * Updated by Dave Martindale, Imax Systems Corp., December 1990
 */

/*
 * Compile time options:
 *
 *
 * CHROMA_LIM is the limit (in IRE units) of the overall
 *	chrominance amplitude; it should be 50 or perhaps
 *	very slightly higher.
 * 
 * COMPOS_LIM is the maximum amplitude (in IRE units) allowed for
 *	the composite signal.  A value of 100 is the maximum
 *	monochrome white, and is always safe.  120 is the absolute
 *	limit for NTSC broadcasting, since the transmitter's carrier
 *	goes to zero with 120 IRE input signal.  Generally, 110
 *	is a good compromise - it allows somewhat brighter colours
 *	than 100, while staying safely away from the hard limit.
 */

/*
 * run-time options:
 *
 * Define either NTSC or PAL as 1 to select the colour system.
 * Define the other one as zero, or leave it undefined.
 *
 * Define FLAG_HOT as 1 if you want "hot" pixels set to black
 *	to identify them.  Otherwise they will be made safe.
 *
 * Define REDUCE_SAT as 1 if you want hot pixels to be repaired by
 *	reducing their saturation.  By default, luminance is reduced.
 *
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgimp/gimp.h>
#include <gtk/gtk.h>
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
  gint32 image;
  gint32 drawable;
  gint32 mode;
  gint32 action;
  gint32 new_layerp;
};

typedef enum {
  act_lredux = 0,
  act_sredux = 1,
  act_flag = 2
} hotAction;

typedef enum {
  mode_ntsc = 0,
  mode_pal = 1
} hotModes;

#define	CHROMA_LIM      50.0		/* chroma amplitude limit */
#define	COMPOS_LIM      110.0		/* max IRE amplitude */

/*
 * RGB to YIQ encoding matrix.
 */

struct {
  double pedestal;
  double gamma;
  double code[3][3];
} mode[2] = {
  {
     7.5,
     2.2,
     {
        { 0.2989,  0.5866,  0.1144 },
        { 0.5959, -0.2741, -0.3218 },
        { 0.2113, -0.5227,  0.3113 }
     }
  },
  {
     0.0,
     2.8,
     {
        { 0.2989,  0.5866,  0.1144 },
        { -0.1473, -0.2891,  0.4364 },
        { 0.6149, -0.5145, -0.1004 }
     }
  }
};


#define SCALE	8192            /* scale factor: do floats with int math */
#define MAXPIX	 255            /* white value */

int	tab[3][3][MAXPIX+1];   /* multiply lookup table */
double	chroma_lim;            /* chroma limit */
double	compos_lim;            /* composite amplitude limit */
long	ichroma_lim2;          /* chroma limit squared (scaled integer) */
int	icompos_lim;           /* composite amplitude limit (scaled integer) */

static void query(void);
static void run(char *name, int nparam, GParam *param,
                int *nretvals, GParam **retvals);

gint pluginCore(struct piArgs *argp);
gint pluginCoreIA(struct piArgs *argp);
static gint hotp(register guint8 r, register guint8 g, register guint8 b);
static void build_tab(int m);

/*
 * gc: apply the gamma correction specified for this video standard.
 * inv_gc: inverse function of gc.
 *
 * These are generally just a call to pow(), but be careful!
 * Future standards may use more complex functions.
 * (e.g. SMPTE 240M's "electro-optic transfer characteristic").
 */
#define gc(x,m) pow(x, 1.0 / mode[m].gamma)
#define inv_gc(x,m) pow(x, mode[m].gamma)

/*
 * pix_decode: decode an integer pixel value into a floating-point
 *	intensity in the range [0, 1].
 *
 * pix_encode: encode a floating-point intensity into an integer
 *	pixel value.
 *
 * The code given here assumes simple linear encoding; you must change
 * these routines if you use a different pixel encoding technique.
 */
#define pix_decode(v)  ((double)v / (double)MAXPIX)
#define pix_encode(v)  ((int)(v * (double)MAXPIX + 0.5))

GPlugInInfo PLUG_IN_INFO = {
  NULL, /* init */
  NULL, /* quit */
  query, /* query */
  run, /* run */
};

MAIN()

static void
query(void){
  static GParamDef args[] = {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "The Image" },
    { PARAM_DRAWABLE, "drawable", "The Drawable" },
    { PARAM_INT32, "mode", "Mode -- NTSC/PAL" },
    { PARAM_INT32, "action", "The action to perform" },
    { PARAM_INT32, "new_layerp", "Create a new layer iff True" },
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  static GParamDef *rets = NULL;
  static int nrets = 0;

  gimp_install_procedure("plug_in_hot",
                         "Look for hot NTSC or PAL pixels ",
                         "hot scans an image for pixels that will give "
			 "unsave values of chrominance or composite "
			 "signale amplitude when encoded into an NTSC "
			 "or PAL signal.  Three actions can be performed on these ``hot'' "
			 "pixels. (0) reduce luminance, (1) reduce saturation, or (2) Blacken.",
                         "Eric L. Hernes, Alan Wm Paeth",
                         "Eric L. Hernes",
                         "1997",
                         "<Image>/Filters/Colors/Hot...",
                         "RGB",
                         PROC_PLUG_IN,
                         nargs, nrets,
                         args, rets);
}

static void
run(char *name, int nparam, GParam *param,
    int *nretvals, GParam **retvals){
  static GParam rvals[1];

  struct piArgs args;

  *nretvals = 1;
  *retvals = rvals;

  memset(&args,(int)0,sizeof(struct piArgs));
  args.mode=-1;

  gimp_get_data("plug_in_hot", &args);

  args.image = param[1].data.d_image;
  args.drawable = param[2].data.d_drawable;

  rvals[0].type = PARAM_STATUS;
  rvals[0].data.d_status = STATUS_SUCCESS;
  switch (param[0].data.d_int32) {
    case RUN_INTERACTIVE:
      /* XXX: add code here for interactive running */
      if(args.mode == -1) {
         args.mode = mode_ntsc;
         args.action =   act_lredux;
         args.new_layerp =   1;
      }
      
      if (pluginCoreIA(&args)==-1) {
        rvals[0].data.d_status = STATUS_EXECUTION_ERROR;
      }
      gimp_set_data("plug_in_hot", &args, sizeof(struct piArgs));
      
    break;

    case RUN_NONINTERACTIVE:
      /* XXX: add code here for non-interactive running */
      if (nparam != 6) {
        rvals[0].data.d_status = STATUS_CALLING_ERROR;
        break;
      }
      args.mode = param[3].data.d_int32;
      args.action = param[4].data.d_int32;
      args.new_layerp = param[5].data.d_int32;

      if (pluginCore(&args)==-1) {
        rvals[0].data.d_status = STATUS_EXECUTION_ERROR;
        break;
      }
    break;

    case RUN_WITH_LAST_VALS:
      /* XXX: add code here for last-values running */
      if (pluginCore(&args)==-1) {
        rvals[0].data.d_status = STATUS_EXECUTION_ERROR;
      }
    break;

  }

}

gint
pluginCore(struct piArgs *argp) {
  GDrawable *drw, *ndrw=NULL;
  GPixelRgn srcPr, dstPr;
  gint retval = 0;
  gint nl=0;
  gint y, x, i;
  gint Y, I, Q;
  guint width, height, Bpp;
  gint prog_interval;
  guchar *src, *s, *dst, *d;
  guchar r, prev_r=0, new_r=0;
  guchar g, prev_g=0, new_g=0;
  guchar b, prev_b=0, new_b=0;
  gdouble fy, fc, t, scale;
  gdouble pr, pg, pb;
  gdouble py;
  
  drw = gimp_drawable_get(argp->drawable);

  width = drw->width;
  height = drw->height;
  Bpp = drw->bpp;
  if(argp->new_layerp) {
    char name[40];
    char *mode_names[] = {
      "ntsc",
      "pal",
    };
    char *action_names[] = {
      "lum redux",
      "sat redux",
      "flag",
    };
      
    sprintf(name, "hot mask (%s, %s)", mode_names[argp->mode],
            action_names[argp->action]);
	     
    nl=gimp_layer_new(argp->image, name, width, height,
                      RGBA_IMAGE, (gdouble)100, NORMAL_MODE);
    ndrw = gimp_drawable_get(nl);
    gimp_drawable_fill(nl, TRANS_IMAGE_FILL);
    gimp_image_add_layer(argp->image, nl, 0);
  }

  src = (guchar*)malloc(width*height*Bpp);
  dst = (guchar*)malloc(width*height*4);
  gimp_pixel_rgn_init (&srcPr, drw, 0, 0, width, height, FALSE, FALSE);
  if (argp->new_layerp) {
    gimp_pixel_rgn_init (&dstPr, ndrw, 0, 0, width, height, FALSE, FALSE);
  } else {
    gimp_pixel_rgn_init (&dstPr, drw, 0, 0, width, height, TRUE, TRUE);
  }

  gimp_pixel_rgn_get_rect(&srcPr, src, 0, 0, width, height);

  s=src;
  d=dst;

  build_tab(argp->mode);

  gimp_progress_init("Hot");
  prog_interval=height/10;
  
  for(y=0;y<height;y++) {
    if (y % prog_interval == 0) gimp_progress_update((double)y/(double)height);
    for(x=0;x<width;x++) {
      if (hotp(r=*(s+0),g=*(s+1),b=*(s+2))) {
         if (argp->action == act_flag) {
            for(i=0;i<3;i++)
               *d++=0;
            s+=3;
            if (Bpp==4) *d++=*s++; else if (argp->new_layerp) *d++=255;
         } else {
                /*
                 * Optimization: cache the last-computed hot pixel.
                 */
            if (r == prev_r && g == prev_g && b == prev_b) {
               *d++ = new_r;
               *d++ = new_g;
               *d++ = new_b;
               s+=3;
               if (Bpp==4) *d++=*s++; else if (argp->new_layerp) *d++=255;
            } else {
               
               Y = tab[0][0][r] + tab[0][1][g] + tab[0][2][b];
               I = tab[1][0][r] + tab[1][1][g] + tab[1][2][b];
               Q = tab[2][0][r] + tab[2][1][g] + tab[2][2][b];
               
               prev_r = r;
               prev_g = g;
               prev_b = b;
                   /*
                    * Get Y and chroma amplitudes in floating point.
                    *
                    * If your C library doesn't have hypot(), just use
                    * hypot(a,b) = sqrt(a*a, b*b);
                    *
                    * Then extract linear (un-gamma-corrected)
                    * floating-point pixel RGB values.
                    */
               fy = (double)Y / (double)SCALE;
               fc = hypot((double)I / (double)SCALE,
                          (double)Q / (double)SCALE);
               
               pr = (double)pix_decode(r);
               pg = (double)pix_decode(g);
               pb = (double)pix_decode(b);
               
                   /*
                    * Reducing overall pixel intensity by scaling R,
                    * G, and B reduces Y, I, and Q by the same factor.
                    * This changes luminance but not saturation, since
                    * saturation is determined by the chroma/luminance
                    * ratio.
                    *
                    * On the other hand, by linearly interpolating
                    * between the original pixel value and a grey
                    * pixel with the same luminance (R=G=B=Y), we
                    * change saturation without affecting luminance.
                    */
               if(argp->action == act_lredux) {
                      /*
                       * Calculate a scale factor that will bring the pixel
                       * within both chroma and composite limits, if we scale
                       * luminance and chroma simultaneously.
                       *
                       * The calculated chrominance reduction applies
                       * to the gamma-corrected RGB values that are
                       * the input to the RGB-to-YIQ operation.
                       * Multiplying the original un-gamma-corrected
                       * pixel values by the scaling factor raised to
                       * the "gamma" power is equivalent, and avoids
                       * calling gc() and inv_gc() three times each.  */
                  scale = chroma_lim / fc;
                  t = compos_lim / (fy + fc);
                  if (t < scale)
                     scale = t;
                  scale = pow(scale, mode[argp->mode].gamma);
                  
                  
                  r = (guint8)pix_encode(scale * pr);
                  g = (guint8)pix_encode(scale * pg);
                  b = (guint8)pix_encode(scale * pb);
                  
               } else { /* act_sredux hopefully */
                      /*
                       * Calculate a scale factor that will bring the
                       * pixel within both chroma and composite
                       * limits, if we scale chroma while leaving
                       * luminance unchanged.
                       *
                       * We have to interpolate gamma-corrected RGB
                       * values, so we must convert from linear to
                       * gamma-corrected before interpolation and then
                       * back to linear afterwards.
                       */
                  scale = chroma_lim / fc;
                  t = (compos_lim - fy) / fc;
                  if (t < scale)
                     scale = t;
                  
                  pr = gc(pr,argp->mode);
                  pg = gc(pg,argp->mode);
                  pb = gc(pb,argp->mode);
                  py = pr * mode[argp->mode].code[0][0] + pg * 
                     mode[argp->mode].code[0][1] + pb *
                     mode[argp->mode].code[0][2];
                  r = pix_encode(inv_gc(py + scale * (pr - py), argp->mode));
                  g = pix_encode(inv_gc(py + scale * (pg - py), argp->mode));
                  b = pix_encode(inv_gc(py + scale * (pb - py), argp->mode));
               }
               *d++ = new_r = r;
               *d++ = new_g = g;
               *d++ = new_b = b;
               s+=3;
               if (Bpp==4) *d++=*s++; else if (argp->new_layerp) *d++=255;
            }
         }
      } else {
         if (!argp->new_layerp) {
            for(i=0;i<Bpp;i++)
               *d++=*s++;
         } else {
            s+=Bpp;
            d+=4;
         }
      }
    }
  }
  gimp_pixel_rgn_set_rect(&dstPr, dst, 0, 0, width, height);
  
  free(src);
  free(dst);
  
  if(argp->new_layerp) {
     gimp_drawable_flush(ndrw);
     gimp_drawable_update(nl, 0, 0, width, height);
  } else {
     gimp_drawable_flush(drw);
     gimp_drawable_merge_shadow (drw->id, TRUE);
     gimp_drawable_update(drw->id, 0, 0, width, height);
  }
  
  gimp_displays_flush();
  
  return retval;
}

gint
pluginCoreIA(struct piArgs *argp) {
  GtkWidget *dlg;
  GtkWidget *hbox;
  GtkWidget *vbox;
  gint runp;
  struct mwRadioGroup modes[] = {
     { "NTSC", 1 },
     { "PAL", 0 },
     { NULL, 0 }
  };
  struct mwRadioGroup actions[] = {
     { "Reduce Luminance", 0 },
     { "Reduce Saturation", 0 },
     { "Blacken (flag)", 0 },
     { NULL, 0}
  };
  gchar **argv;
  gint argc;

  /* Set args */
  argc = 1;
  argv = g_new(gchar *, 1);
  argv[0] = g_strdup("hot");
  gtk_init(&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());

  actions[argp->action].var = 1;

  dlg = mw_app_new("plug_in_hot", "Hot", &runp);
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(hbox), 5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);

  vbox = gtk_vbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show(vbox);

  mw_toggle_button_new(vbox, NULL, "Create New Layer", &argp->new_layerp);
  mw_radio_group_new(vbox, "Mode", modes);

  mw_radio_group_new(hbox, "Action", actions);

  gtk_widget_show(dlg);
  gtk_main();
  gdk_flush();

  argp->mode = mw_radio_result(modes);
  argp->action = mw_radio_result(actions);
  
  if (runp) {
     return pluginCore(argp);
  } else {
     return -1;
  }
}

/*
 * build_tab: Build multiply lookup table.
 *
 * For each possible pixel value, decode value into floating-point
 * intensity.  Then do gamma correction required by the video
 * standard.  Scale the result by our fixed-point scale factor.
 * Then calculate 9 lookup table entries for this pixel value.
 *
 * We also calculate floating-point and scaled integer versions
 * of our limits here.  This prevents evaluating expressions every pixel
 * when the compiler is too stupid to evaluate constant-valued
 * floating-point expressions at compile time.
 *
 * For convenience, the limits are #defined using IRE units.
 * We must convert them here into the units in which YIQ
 * are measured.  The conversion from IRE to internal units
 * depends on the pedestal level in use, since as Y goes from
 * 0 to 1, the signal goes from the pedestal level to 100 IRE.
 * Chroma is always scaled to remain consistent with Y.
 */
static void
build_tab(int m) {
  register double f;
  register int pv;

  for (pv = 0; pv <= MAXPIX; pv++) {
    f = (double)SCALE * (double)gc((double)pix_decode(pv),m);
    tab[0][0][pv] = (int)(f * mode[m].code[0][0] + 0.5);
    tab[0][1][pv] = (int)(f * mode[m].code[0][1] + 0.5);
    tab[0][2][pv] = (int)(f * mode[m].code[0][2] + 0.5);
    tab[1][0][pv] = (int)(f * mode[m].code[1][0] + 0.5);
    tab[1][1][pv] = (int)(f * mode[m].code[1][1] + 0.5);
    tab[1][2][pv] = (int)(f * mode[m].code[1][2] + 0.5);
    tab[2][0][pv] = (int)(f * mode[m].code[2][0] + 0.5);
    tab[2][1][pv] = (int)(f * mode[m].code[2][1] + 0.5);
    tab[2][2][pv] = (int)(f * mode[m].code[2][2] + 0.5);
  }

  chroma_lim = (double)CHROMA_LIM / (100.0 - mode[m].pedestal);
  compos_lim = ((double)COMPOS_LIM - mode[m].pedestal) /
    (100.0 - mode[m].pedestal);

  ichroma_lim2 = (int)(chroma_lim * SCALE + 0.5);
  ichroma_lim2 *= ichroma_lim2;
  icompos_lim = (int)(compos_lim * SCALE + 0.5);
}

static int
hotp(register guint8 r, register guint8 g, register guint8 b) {
  register int	y, i, q;
  register long	y2, c2;

  /*  fprintf(stderr, "\tr: %d, g: %d, b: %d\n", r, g, b);*/
  
  /*
   * Pixel decoding, gamma correction, and matrix multiplication
   * all done by lookup table.
   *
   * "i" and "q" are the two chrominance components;
   * they are I and Q for NTSC.
   * For PAL, "i" is U (scaled B-Y) and "q" is V (scaled R-Y).
   * Since we only care about the length of the chroma vector,
   * not its angle, we don't care which is which.
   */
  y = tab[0][0][r] + tab[0][1][g] + tab[0][2][b];
  i = tab[1][0][r] + tab[1][1][g] + tab[1][2][b];
  q = tab[2][0][r] + tab[2][1][g] + tab[2][2][b];

  /*
   * Check to see if the chrominance vector is too long or the
   * composite waveform amplitude is too large.
   *
   * Chrominance is too large if
   *
   *	sqrt(i^2, q^2)  >  chroma_lim.
   *
   * The composite signal amplitude is too large if
   *
   *	y + sqrt(i^2, q^2)  >  compos_lim.
   *
   * We avoid doing the sqrt by checking
   *
   *	i^2 + q^2  >  chroma_lim^2
   * and
   *	y + sqrt(i^2 + q^2)  >  compos_lim
   *	sqrt(i^2 + q^2)  >  compos_lim - y
   *	i^2 + q^2  >  (compos_lim - y)^2
   *
   */

  c2 = (long)i * i + (long)q * q;
  y2 = (long)icompos_lim - y;
  y2 *= y2;
  /*  fprintf(stderr, "hotp: c2: %d; ichroma_lim2: %d; y2: %d; ",
	  c2, ichroma_lim2, y2);*/
  
  if (c2 <= ichroma_lim2 && c2 <= y2) {	/* no problems */
    /*    fprintf(stderr, "nope\n");*/
    return 0;
  }

  /*  fprintf(stderr, "yup\n");*/
  return 1;
}

/*
 * Local Variables:
 * mode: C
 * End:
 */

/* end of file: hot/hot.c */
