/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Plugin to load MPEG movies. (C) 1997-99 Adam D. Moss
 *
 * v1.1 - by Adam D. Moss, adam@gimp.org, adam@foxbox.org
 * Requires mpeg_lib by Gregory P. Ward.  See notes below for
 * obtaining and patching mpeg_lib.
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

/*******************************************************************
 * USING MPEG_LIB WITH THIS PLUGIN:  mpeg_lib 1.2.1 can be found   *
 * at ftp://ftp.mni.mcgill.ca/pub/mpeg/ - however, mpeg_lib 1.2.x  *
 * contains a bug in end-of-stream reporting, which will cause it  *
 * to crash in conjunction with this plugin.  I enclose a simple   *
 * patch below which fixes the problem (or at least the symptom.;))*
 *******************************************************************
 *    Addendum: mpeg_lib 1.3.0 is now released and much better!    *
 *******************************************************************/

/*******************************************************************
*** wrapper.c   Tue Oct 10 22:08:39 1995
--- ../mpeg_lib2/wrapper.c      Sat Sep 20 20:29:46 1997
***************
*** 392,394 ****
            "copying from %08X to %08X\n", CurrentImage, Frame);
!    memcpy (Frame, CurrentImage, ImageInfo.Size);
     return (!MovieDone);
--- 392,397 ----
            "copying from %08X to %08X\n", CurrentImage, Frame);
!
!    if (!MovieDone)
!       memcpy (Frame, CurrentImage, ImageInfo.Size);
!
     return (!MovieDone);
*******************************************************************/

/*
 * Changelog:
 *
 * 99/05/31
 * v1.1: Endianness fix.
 *
 * 97/09/21
 * v1.0: Initial release. [Adam]
 */

/*
 * TODO:
 *
 * More robustness for broken streams (how much co-operation
 *   from mpeg_lib?)
 *
 * Saving (eventually...)
 *
 * Crop images properly (e.g. height&~7 ?)
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "mpeg.h"

#include "libgimp/stdplugins-intl.h"


static void   query      (void);
static void   run        (gchar   *name,
                          gint     nparams,
                          GParam  *param,
                          gint    *nreturn_vals,
                          GParam **return_vals);
static gint32 load_image (char   *filename);


GPlugInInfo PLUG_IN_INFO =
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
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name entered" }
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" }
  };
  static gint nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static gint nload_return_vals = (sizeof (load_return_vals) /
				   sizeof (load_return_vals[0]));

  gimp_install_procedure ("file_mpeg_load",
                          "Loads MPEG movies",
                          "FIXME: write help for mpeg_load",
                          "Adam D. Moss",
                          "Adam D. Moss",
                          "1997",
                          "<Load>/MPEG",
			  NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_register_magic_load_handler ("file_mpeg_load",
				    "mpg,mpeg",
				    "",
				    "0,long,0x000001b3,0,long,0x000001ba");
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GRunModeType  run_mode;
  GStatusType   status = STATUS_SUCCESS;
  gint32 image_ID;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_EXECUTION_ERROR;

  INIT_I18N();

  if (strcmp (name, "file_mpeg_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
	{
	  *nreturn_vals = 2;
	  values[1].type         = PARAM_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  status = STATUS_EXECUTION_ERROR;
	}
    }
  else
    {
      status = STATUS_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}


static gint
MPEG_frame_period_ms (gint mpeg_rate_code)
{
  switch(mpeg_rate_code)
    {
    case 1: return 44; /* ~23 fps */
    case 2: return 42; /*  24 fps */
    case 3: return 40; /*  25 fps */
    case 4: return 33; /* ~30 fps */
    case 5: return 33; /*  30 fps */
    case 6: return 20; /*  50 fps */
    case 7: return 17; /* ~60 fps */
    case 8: return 17; /*  60 fps */
    case 0: /* ? */
    default:
      printf("mpeg: warning - this MPEG has undefined timing.\n");
      return 0;
    }
}


static gint32
load_image (gchar *filename)
{
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  gint32 image_ID;
  gint32 layer_ID;
  gchar *temp;
  Boolean moreframes;
  gint i, framenumber, delay;
  FILE *fp;
  guchar *data;
  gint wwidth, wheight;

  /* mpeg structure */
  ImageDesc img;

  gchar *layername;  /* FIXME? */

  temp = g_malloc (strlen (filename) + 16);
  if (!temp) gimp_quit ();
  g_free (temp);
  
  gimp_progress_init ( _("Loading MPEG movie..."));

  fp = fopen(filename,"rb");
  if (fp == NULL)
    {
      fprintf (stderr, "mpeg: fopen failed\n");
      exit (4);      
    }

  SetMPEGOption (MPEG_DITHER, FULL_COLOR_DITHER);
  if (!OpenMPEG (fp, &img))
    {
      fprintf (stderr, "mpeg: OpenMPEG failed\n");
      exit (1);
    }

  data = g_malloc(img.Size);

  delay = MPEG_frame_period_ms(img.PictureRate);

  /*
  printf("  <%d : %d>  %dx%d - d%d - bmp%d - ps%d - s%d\n",
	 img.PictureRate, MPEG_frame_period_ms(img.PictureRate),
	 img.Width,img.Height,img.Depth,img.BitmapPad,img.PixelSize,img.Size);
	 */

  if (img.PixelSize != 32)
    {
      printf("mpeg: Hmm, sorry, funny PixelSize (%d) in MPEG.  Aborting.\n",
	     img.PixelSize);
      exit(2);
    }

  if (img.BitmapPad != 32)
    {
      printf("mpeg: Hmm, sorry, funny BitmapPad (%d) in MPEG.  Aborting.\n",
	     img.BitmapPad);
      exit(3);
    }

  wwidth = img.Width;
  wheight = img.Height;
  
  image_ID = gimp_image_new (wwidth, wheight, RGB);
  gimp_image_set_filename (image_ID, filename);

  moreframes = TRUE;
  framenumber = 1;
  while (moreframes)
    {
      gimp_progress_update ((framenumber&1) ? 1.0 : 0.0);

      /* libmpeg - at least the version I have (1.2) - is faulty
         in its reporting of whether there are remaining frames...
	 the sample MPEG it comes with is the only one that it seems
	 able to detect the end of, and even then it doesn't notice
	 until it's too late, so we may lose the last frame, or
	 crash... sigh.

	 A patch to mpeg_lib 1.2.x is included in the header of this
	 plugin.

	 */
      moreframes = GetMPEGFrame((char *)data);
      if (!moreframes) break;

      if (delay > 0)
	layername = g_strdup_printf( _("Frame %d (%dms)"),
		framenumber, delay);
      else
	layername = g_strdup_printf( _("Frame %d"),
		framenumber);

      layer_ID = gimp_layer_new (image_ID, layername,
				 wwidth,
				 wheight,
				 RGBA_IMAGE, 100, NORMAL_MODE);
      g_free(layername);
      gimp_image_add_layer (image_ID, layer_ID, 0);
      drawable = gimp_drawable_get (layer_ID);


      /* conveniently for us, mpeg_lib returns pixels padded
	 to 32-bit boundaries (is that true for all archs?).  So we
	 only have to fill in the alpha channel. */
      for (i=(wwidth*wheight)-1; i>=0; i--)
	{
#if G_BYTE_ORDER == G_BIG_ENDIAN
          unsigned char r,g,b;
          r = data[i*4+3 ];
          g = data[i*4+2 ];
          b = data[i*4+1 ];
          data[i*4+2]= b;
          data[i*4+1]= g;
          data[i*4 ] = r;
#endif
          data[i*4+3] = 255;
	}

      gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
			   drawable->width, drawable->height, TRUE, FALSE);
      gimp_pixel_rgn_set_rect (&pixel_rgn, data, 0, 0,
			       wwidth, wheight);
      
      gimp_drawable_flush (drawable);
      gimp_drawable_detach (drawable);

      framenumber++;
    }

  CloseMPEG();
  fclose(fp);
      
  gimp_progress_update (1.0);

  g_free (data);

  return image_ID;
}
