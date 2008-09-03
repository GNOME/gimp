/*
 * (c) Adam D. Moss : 1998-2000 : adam@gimp.org : adam@foxbox.org
 *
 * This is part of the GIMP package and is released under the GNU
 * Public License.
 */

/*
 * Version 1.06 : 2000-12-12
 *
 * 1.06:
 * "Out of hyding" remix.  Dr Jekyll is still suspiciously
 * absent from the fine bogey tale until Chapter Three.
 *
 * 1.05:
 * Sub-pixel jitter is now less severe and less coarse.
 *
 * 1.04:
 * Wigglyness and button-click fun.
 *
 * 1.03:
 * Fix for pseudocolor displays w/gdkrgb.
 *
 * 1.02:
 * Massive speedup if you have a very recent version of GTK 1.1.
 * Removed possible div-by-0 errors, took the plugin out
 * of hiding (guess we need a new easter-egg for GIMP 1.2!)
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-the-old-egg"
#define PLUG_IN_BINARY "gee-zoom"


/* Declare local functions. */
static void       query (void);
static void       run   (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static void       do_fun                   (void);

static void       window_response_callback (GtkWidget      *widget,
                                            gint            response_id,
                                            gpointer        data);
static gboolean   do_iteration             (void);
static gboolean   toggle_feedbacktype      (GtkWidget      *widget,
                                            GdkEventButton *bevent);

static void       render_frame             (void);
static void       init_preview_misc        (void);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


static const guint  width  = 256;
static const guint  height = 256;
static GRand       *gr;


#define LUTSIZE 512
#define LUTSIZEMASK ((LUTSIZE)-1)
static gint wigglelut[LUTSIZE];

#define LOWAMP 2
#define HIGHAMP 11
static gint wiggleamp = LOWAMP;


/* Global widgets'n'stuff */
static guchar     *seed_data;
static guchar     *preview_data1;
static guchar     *preview_data2;

static GtkWidget  *drawing_area;

static gint32      image_id;
static GimpDrawable      *drawable;
static GimpImageBaseType  imagetype;
static guchar     *palette;
static gint        ncolours;

static guint      idle_tag;
static gboolean   feedbacktype = FALSE;
static gboolean   wiggly       = TRUE;
static gboolean   rgb_mode;


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "Must be interactive (1)" },
    { GIMP_PDB_IMAGE,    "image",    "Input Image"             },
    { GIMP_PDB_DRAWABLE, "drawable", "Input Drawable"          }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("A big hello from the GIMP team!"),
                          "Hay-ulp",
                          "Adam D. Moss <adam@gimp.org>",
                          "Adam D. Moss <adam@gimp.org>",
                          "1998",
                          N_("Gee Zoom"),
                          "RGB*, INDEXED*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (const gchar      *name,
     gint              n_params,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  gr = g_rand_new ();

  *nreturn_vals = 1;
  *return_vals = values;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  if (run_mode == GIMP_RUN_NONINTERACTIVE ||
      n_params != 3)
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      image_id = param[1].data.d_image;
      drawable = gimp_drawable_get (param[2].data.d_drawable);

      if (drawable)
	do_fun();
      else
	status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  g_rand_free (gr);
}


static void
build_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *frame;
  gchar     *tmp;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dlg = gimp_dialog_new (_("Gee Zoom"), PLUG_IN_BINARY,
                         NULL, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,
                         NULL);

  button = gtk_dialog_add_button (GTK_DIALOG (dlg),
                                  _("Thank You for Choosing GIMP"),
                                  GTK_RESPONSE_OK);

  g_signal_connect (dlg, "response",
                    G_CALLBACK (window_response_callback),
                    NULL);


  tmp = g_strdup_printf (_("An obsolete creation by %s"),
                         "Adam D. Moss / adam@gimp.org / adam@foxbox.org "
                         "/ 1998-2000");
  gimp_help_set_help_data (button, tmp, NULL);
  g_free (tmp);
  /* The 'fun' half of the dialog */

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
                      frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawing_area, width, height);
  gtk_container_add (GTK_CONTAINER (frame), drawing_area);
  gtk_widget_show (drawing_area);

  gtk_widget_add_events (drawing_area,
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  g_signal_connect (drawing_area, "button-release-event",
                    G_CALLBACK (toggle_feedbacktype),
                    NULL);

  gtk_widget_show (dlg);

  idle_tag = g_idle_add_full (G_PRIORITY_LOW,
                              (GSourceFunc) do_iteration,
                              NULL,
                              NULL);
}


static void
init_lut (void)
{
  gint i;

  for (i=0; i<LUTSIZE; i++)
    {
      wigglelut[i] = RINT((double)(wiggleamp<<11))*(sin((double)(i) /
					    ((double)LUTSIZEMASK /
					     10 * G_PI)));
    }
}


static void
do_fun (void)
{
  imagetype = gimp_image_base_type(image_id);

  if (imagetype == GIMP_INDEXED)
    palette = gimp_image_get_colormap (image_id, &ncolours);

  /* cache hint */
  gimp_tile_cache_ntiles (1);

  init_preview_misc ();
  build_dialog ();

  init_lut();

  render_frame();

  gtk_main ();
}


/* Rendering Functions */

/* Adam's silly algorithm. */
static void
domap1 (unsigned char *src, unsigned char *dest,
	int bx, int by, int cx, int cy)
{
  unsigned int dy;
  signed int bycxmcybx;
  signed int bx2,by2;
  signed int cx2,cy2;
  unsigned int basesx;
  unsigned int basesy;

  static unsigned int grrr=0;

  grrr++;

  if ((cx+bx) == 0)
    cx++;

  if ((cy+by) == 0)
    by++;

  bycxmcybx = (by*cx-cy*bx);

  if (bycxmcybx == 0)
    bycxmcybx = 1;

  /* A little sub-pixel jitter to liven things up. */
  basesx = ((g_rand_int_range (gr, 0, 29<<19)/bycxmcybx)) +
    ((-128-((128*256)/(cx+bx)))<<11);
  basesy = ((g_rand_int_range (gr, 0, 29<<19)/bycxmcybx)) +
    ((-128-((128*256)/(cy+by)))<<11);

  bx2 = ((bx)<<19)/bycxmcybx;
  cx2 = ((cx)<<19)/bycxmcybx;
  by2 = ((by)<<19)/bycxmcybx;
  cy2 = ((cy)<<19)/bycxmcybx;

  for (dy=0;dy<256;dy++)
    {
      unsigned int sx;
      unsigned int sy;
      unsigned int dx;

      sy = (basesy+=cx2);
      sx = (basesx-=bx2);

      if (wiggly)
	{
	  sx += wigglelut[(((basesy)>>11)+grrr) & LUTSIZEMASK];
	  sy += wigglelut[(((basesx)>>11)+(grrr/3)) & LUTSIZEMASK];
	}

      dx = 256;
      do
	{
	  *dest++ = (*(src +
		   (
		    (
		     ((255&(
			    (sx>>11)
			    )))
		     |
		     ((((255&(
			      (sy>>11)
			      ))<<8)))
		     )
		    )));
	  ;
	  sx += by2;
	  sy -= cy2;
	}
      while (--dx);
    }
}

/* 3bypp variant */
static void
domap3(unsigned char *src, unsigned char *dest,
       int bx, int by, int cx, int cy)
{
  unsigned int dy;
  signed int bycxmcybx;
  signed int bx2,by2;
  signed int cx2,cy2;
  unsigned int basesx;
  unsigned int basesy;

  static unsigned int grrr=0;

  grrr++;

  if ((cx+bx) == 0)
    cx++;

  if ((cy+by) == 0)
    by++;

  bycxmcybx = (by*cx-cy*bx);

  if (bycxmcybx == 0)
    bycxmcybx = 1;

  /* A little sub-pixel jitter to liven things up. */
  basesx = ((g_rand_int_range (gr, 0, 29<<19)/bycxmcybx)) +
    ((-128-((128*256)/(cx+bx)))<<11);
  basesy = ((g_rand_int_range (gr, 0, 29<<19)/bycxmcybx)) +
    ((-128-((128*256)/(cy+by)))<<11);

  bx2 = ((bx)<<19)/bycxmcybx;
  cx2 = ((cx)<<19)/bycxmcybx;
  by2 = ((by)<<19)/bycxmcybx;
  cy2 = ((cy)<<19)/bycxmcybx;

  for (dy=0;dy<256;dy++)
    {
      unsigned int sx;
      unsigned int sy;
      unsigned int dx;

      sy = (basesy+=cx2);
      sx = (basesx-=bx2);

      if (wiggly)
	{
	  sx += wigglelut[(((basesy)>>11)+grrr) & LUTSIZEMASK];
	  sy += wigglelut[(((basesx)>>11)+(grrr/3)) & LUTSIZEMASK];
	}

      dx = 256;

      do
	{
	  unsigned char* addr;

	  addr = src + 3*
	    (
	     (
	      ((255&(
		     (sx>>11)
		     )))
	      |
	      ((((255&(
		       (sy>>11)
		       ))<<8)))
	      )
	     );

	  *dest++ = *(addr);
	  *dest++ = *(addr+1);
	  *dest++ = *(addr+2);

	  sx += by2;
	  sy -= cy2;
	}
      while (--dx);
    }
}


static void
render_frame (void)
{
  GtkStyle *style;
  int i;
  static int frame = 0;
  unsigned char* tmp;
  static gint xp=128, yp=128;
  gint rxp, ryp;
  GdkModifierType mask;
  gint pixels;

  if (! GTK_WIDGET_DRAWABLE (drawing_area))
    return;

  style = gtk_widget_get_style (drawing_area);

  pixels = width*height*(rgb_mode?3:1);

  tmp = preview_data2;
  preview_data2 = preview_data1;
  preview_data1 = tmp;

  if (frame==0)
    {
      for (i=0;i<pixels;i++)
	{
	  preview_data2[i] =
	    preview_data1[i] =
	    seed_data[i];
	}
    }

  gdk_window_get_pointer (drawing_area->window, &rxp, &ryp, &mask);

  if ((abs(rxp)>60)||(abs(ryp)>60))
    {
      xp = rxp;
      yp = ryp;
    }

  if (rgb_mode)
    {
      domap3(preview_data2, preview_data1,
	     -(yp-xp)/2, xp+yp
	     ,
	     xp+yp, (yp-xp)/2
	     );

      gdk_draw_rgb_image (drawing_area->window,
                          style->white_gc,
			  0, 0, width, height,
			  GDK_RGB_DITHER_NORMAL,
			  preview_data1, width * 3);

      /*      memcpy(preview_data1, seed_data, 256*256*3); */

      if (frame != 0)
	{
	  if (feedbacktype)
	    {
	      for (i=0;i<pixels;i++)
		{
		  gint t;
		  t = preview_data1[i] + seed_data[i] - 128;
		  preview_data1[i] = /*CLAMP(t,0,255);*/
		    (t&256)? (~(t>>10)) : t; /* Quick specialized clamp */
		}
	    }
	  else/* if (0) */
	    {
	      gint pixwords = pixels/sizeof(gint32);
	      gint32* seedwords = (gint32*) seed_data;
	      gint32* prevwords = (gint32*) preview_data1;

	      for (i=0;i<pixwords;i++)
		{
		  /*preview_data1[i] = (preview_data1[i]*2 +
		    seed_data[i]) /3;*/

		  /* mod'd version of the below for a 'deeper' mix */
		  prevwords[i] =
		    ((prevwords[i] >> 1) & 0x7f7f7f7f) +
		    ((prevwords[i] >> 2) & 0x3f3f3f3f) +
		    ((seedwords[i] >> 2) & 0x3f3f3f3f);
		  /* This is from Raph L... it should be a fast 50%/50%
		     blend, though I don't know if 50%/50% is as nice as
		     the old ratio. */
		  /*
		    prevwords[i] =
		    ((prevwords[i] >> 1) & 0x7f7f7f7f) +
		    ((seedwords[i] >> 1) & 0x7f7f7f7f) +
		    (prevwords[i] & seedwords[i] & 0x01010101); */
		}
	    }
	}
    }
  else /* GRAYSCALE */
    {
      domap1(preview_data2, preview_data1,
	     -(yp-xp)/2, xp+yp
	     ,
	     xp+yp, (yp-xp)/2
	     );

      gdk_draw_gray_image (drawing_area->window,
                           style->white_gc,
			   0, 0, width, height,
			   GDK_RGB_DITHER_NORMAL,
			   preview_data1, width);
      if (frame != 0)
	{
	  if (feedbacktype)
	    {
	      for (i=0;i<pixels;i++)
		{
		  int t;
		  t = preview_data1[i] + seed_data[i] - 128;
		  preview_data1[i] = /*CLAMP(t,0,255);*/
		    (t&256)? (~(t>>10)) : t; /* Quick specialized clamp */
		}
	    }
	  else
	    {
	      gint pixwords = pixels/sizeof(gint32);
	      gint32* seedwords = (gint32*) seed_data;
	      gint32* prevwords = (gint32*) preview_data1;

	      for (i=0;i<pixwords;i++)
		{

		  /* mod'd version of the below for a 'deeper' mix */
		  prevwords[i] =
		    ((prevwords[i] >> 1) & 0x7f7f7f7f) +
		    ((prevwords[i] >> 2) & 0x3f3f3f3f) +
		    ((seedwords[i] >> 2) & 0x3f3f3f3f);
		  /* This is from Raph L... it should be a fast 50%/50%
		     blend, though I don't know if 50%/50% is as nice as
		     the old ratio. */
		  /*
		    prevwords[i] =
		    ((prevwords[i] >> 1) & 0x7f7f7f7f) +
		    ((seedwords[i] >> 1) & 0x7f7f7f7f) +
		    (prevwords[i] & seedwords[i] & 0x01010101); */
		}
	    }
	}
    }

  frame++;
}

static void
init_preview_misc (void)
{
  GimpPixelRgn pixel_rgn;
  gint i;
  gboolean has_alpha;

  if ((imagetype == GIMP_RGB) || (imagetype == GIMP_INDEXED))
    rgb_mode = TRUE;
  else
    rgb_mode = FALSE;

  has_alpha = gimp_drawable_has_alpha(drawable->drawable_id);

  seed_data = g_malloc(width*height*4);
  preview_data1 = g_malloc(width*height*(rgb_mode?3:1));
  preview_data2 = g_malloc(width*height*(rgb_mode?3:1));

  if ((drawable->width<256) || (drawable->height<256))
    {
      for (i=0;i<256;i++)
	{
	  if (i < drawable->height)
	    {
	      gimp_pixel_rgn_init (&pixel_rgn,
				   drawable,
				   drawable->width>256?
				   (drawable->width/2-128):0,
				   (drawable->height>256?
				   (drawable->height/2-128):0)+i,
				   MIN(256,drawable->width),
				   1,
				   FALSE,
				   FALSE);
	      gimp_pixel_rgn_get_rect (&pixel_rgn,
				       &seed_data[(256*i +
						 (
						  (
						   drawable->width<256 ?
						   (256-drawable->width)/2 :
						   0
						   )
						  +
						  (
						   drawable->height<256 ?
						   (256-drawable->height)/2 :
						   0
						   ) * 256
						  )) *
						 gimp_drawable_bpp
						 (drawable->drawable_id)
				       ],
				       drawable->width>256?
				       (drawable->width/2-128):0,
				       (drawable->height>256?
				       (drawable->height/2-128):0)+i,
				       MIN(256,drawable->width),
				       1);
	    }
	}
    }
  else
    {
      gimp_pixel_rgn_init (&pixel_rgn,
			   drawable,
			   drawable->width>256?(drawable->width/2-128):0,
			   drawable->height>256?(drawable->height/2-128):0,
			   MIN(256,drawable->width),
			   MIN(256,drawable->height),
			   FALSE,
			   FALSE);
      gimp_pixel_rgn_get_rect (&pixel_rgn,
			       seed_data,
			       drawable->width>256?(drawable->width/2-128):0,
			       drawable->height>256?(drawable->height/2-128):0,
			       MIN(256,drawable->width),
			       MIN(256,drawable->height));
    }

  gimp_drawable_detach(drawable);


  /* convert the image data of varying types into flat grey or rgb. */
  switch (imagetype)
    {
    case GIMP_INDEXED:
      if (has_alpha)
	{
	  for (i=width*height;i>0;i--)
	    {
	      seed_data[3*(i-1)+2] =
		((palette[3*(seed_data[(i-1)*2])+2]*seed_data[(i-1)*2+1])/255)
		+ ((255-seed_data[(i-1)*2+1]) * g_rand_int_range (gr, 0, 256))/255;
	      seed_data[3*(i-1)+1] =
		((palette[3*(seed_data[(i-1)*2])+1]*seed_data[(i-1)*2+1])/255)
		+ ((255-seed_data[(i-1)*2+1]) * g_rand_int_range (gr, 0, 256))/255;
	      seed_data[3*(i-1)+0] =
		((palette[3*(seed_data[(i-1)*2])+0]*seed_data[(i-1)*2+1])/255)
		+ ((255-seed_data[(i-1)*2+1]) * g_rand_int_range (gr, 0, 256))/255;
	    }
	}
      else
	{
	  for (i=width*height;i>0;i--)
	    {
	      seed_data[3*(i-1)+2] = palette[3*(seed_data[i-1])+2];
	      seed_data[3*(i-1)+1] = palette[3*(seed_data[i-1])+1];
	      seed_data[3*(i-1)+0] = palette[3*(seed_data[i-1])+0];
	    }
	}
      break;

    case GIMP_GRAY:
      if (has_alpha)
	{
	  for (i=0;i<width*height;i++)
	    {
	      seed_data[i] =
		(seed_data[i*2]*seed_data[i*2+1])/255
		+ ((255-seed_data[i*2+1]) * g_rand_int_range (gr, 0, 256))/255;
	    }
	}
      break;

    case GIMP_RGB:
      if (has_alpha)
	{
	  for (i=0;i<width*height;i++)
	    {
	      seed_data[i*3+2] =
		(seed_data[i*4+2]*seed_data[i*4+3])/255
		+ ((255-seed_data[i*4+3]) * g_rand_int_range (gr, 0, 256))/255;
	      seed_data[i*3+1] =
		(seed_data[i*4+1]*seed_data[i*4+3])/255
		+ ((255-seed_data[i*4+3]) * g_rand_int_range (gr, 0, 256))/255;
	      seed_data[i*3+0] =
		(seed_data[i*4+0]*seed_data[i*4+3])/255
		+ ((255-seed_data[i*4+3]) * g_rand_int_range (gr, 0, 256))/255;
	    }
	}
      break;

    default:
      break;
    }
}



/* Util. */

static gboolean
do_iteration (void)
{
  render_frame ();

  return TRUE;
}



/*  Callbacks  */

static void
window_response_callback (GtkWidget *widget,
                          gint       response_id,
                          gpointer   data)
{
  g_source_remove (idle_tag);
  idle_tag = 0;

  gtk_widget_destroy (widget);
}

static gboolean
toggle_feedbacktype (GtkWidget      *widget,
		     GdkEventButton *bevent)
{
  if (bevent->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK))
    {
      wiggleamp = bevent->x/5;

      wiggly = TRUE;
      init_lut();

      return TRUE;
    }

  if (bevent->state & GDK_BUTTON1_MASK)
    feedbacktype = !feedbacktype;

  if (bevent->state & GDK_BUTTON2_MASK)
    wiggly = !wiggly;

  if (bevent->state & GDK_BUTTON3_MASK)
    {
      if (wiggleamp == LOWAMP)
	wiggleamp = HIGHAMP;
      else
	wiggleamp = LOWAMP;

      wiggly = TRUE;
      init_lut ();
    }

  return TRUE;
}
