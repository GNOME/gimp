/* gap_decode_mpeg.c */

/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Plugin to load MPEG movies. (C) 1997-99 Adam D. Moss
 *           GAP modifications (C) 1999    Wolfgang Hofer
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
 *******************************************************************
 *     mpeg_lib 1.3.1  is available at: 	                   *
 *     http://starship.python.net/~gward/mpeglib	           *
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
 * 2000/02/07 v1.1.16a:  hof: replaced sprintf by g_strdup_printf
 * 2000/01/06 v1.1.14a:  hof: save thumbnails .xvpics p_gimp_file_save_thumbnail
 *                       store framerate in video_info file
 * 1999/11/25 v1.1.11.b: Initial release. [hof] 
 *                       (based on plug-ins/common/mpeg.c v1.1 99/05/31 by Adam D. Moss)
 */




/* UNIX system includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* GIMP includes */
#include "gtk/gtk.h"
#include "config.h"
#include "libgimp/stdplugins-intl.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

/* GAP includes */
#include "gap_arr_dialog.h"
#include "gap_pdb_calls.h"

/* Includes for extra LIBS */
#include "mpeg.h"

int gap_debug; /* ==0  ... dont print debug infos */

/*  for i18n  */
static gchar G_GNUC_UNUSED *dummy_entries[] =
{
  N_("<Image>/Video/Split Video to Frames"),
  N_("<Toolbox>/Xtns/Split Video to Frames")
};

static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GimpParam  *param,
                          int     *nreturn_vals,
                          GimpParam **return_vals);
static gint32 load_image (char   *filename,
                          gint32  first_frame,
                          gint32  last_frame,
			  char    *basename,
			  gint32  autoload);
static gint32 load_range_dialog(gint32 *first_frame,
                                gint32 *last_frame,
		                char *filename,
		                gint32 len_filename,
				char *basename,
				gint32 len_basename,
				gint32 *autoload);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()

static void
query ()
{
  static GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "(unused)"},
    { GIMP_PDB_DRAWABLE, "drawable", "(unused)"},
    { GIMP_PDB_STRING, "filename", "The name of the file to load" },
    { GIMP_PDB_STRING, "raw_filename", "The name entered" },
    { GIMP_PDB_INT32, "first_frame", "1st frame to extract (starting at number 1)" },
    { GIMP_PDB_INT32, "last_frame", "last frame to extract (use 0 to load all remaining frames)" },
    { GIMP_PDB_STRING, "animframe_basename", "The name for the single frames _0001.xcf is added" },
    { GIMP_PDB_INT32, "autoload", "TRUE: load 1.st extracted frame on success" },
  };
  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" },
  };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

  static GimpParamDef ext_args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename", "The name of the file to load" },
    { GIMP_PDB_STRING, "raw_filename", "The name entered" },
    { GIMP_PDB_INT32, "first_frame", "1st frame to extract (starting at number 1)" },
    { GIMP_PDB_INT32, "last_frame", "last frame to extract (use 0 to load all remaining frames)" },
    { GIMP_PDB_STRING, "animframe_basename", "The name for the single frames _0001.xcf is added" },
    { GIMP_PDB_INT32, "autoload", "TRUE: load 1.st extracted frame on success" },
  };
  static int next_args = sizeof (ext_args) / sizeof (ext_args[0]);

  INIT_I18N();

  gimp_install_procedure ("plug_in_gap_decode_mpeg",
                          "Split MPEG1 movies into animframes and load 1st frame",
                          "Split MPEG1 movies into single frames (image files on disk) and load 1st frame. audio tracks are ignored",
                          "Wolfgang Hofer (hof@hotbot.com)",
                          "Wolfgang Hofer",
                          "2000/01/01",
                          N_("<Image>/Video/Split Video to Frames/MPEG1"),
			  NULL,
                          GIMP_PLUGIN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("extension_gap_decode_mpeg",
                          "Split MPEG1 movies into animframes and load 1st frame",
                          "Split MPEG1 movies into single frames (image files on disk) and load 1st frame. audio tracks are ignored",
                          "Wolfgang Hofer (hof@hotbot.com)",
                          "Wolfgang Hofer",
                          "2000/01/01",
                          N_("<Toolbox>/Xtns/Split Video to Frames/MPEG1"),
			  NULL,
                          GIMP_EXTENSION,
                          next_args, nload_return_vals,
                          ext_args, load_return_vals);
}

static void
run (char    *name,
     int      nparams,
     GimpParam  *param,
     int     *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[2];
  GimpRunModeType run_mode;
  gint32 image_ID;
  gint32 first_frame, last_frame;
  gint32 autoload;
  gint32 l_rc;
  char   l_frames_basename[500];
  char   l_filename[500];
  int    l_par;

  image_ID = -1;
  *nreturn_vals = 1;
  *return_vals = values;
  autoload = FALSE;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_CALLING_ERROR;

  run_mode = param[0].data.d_int32;

  if ((strcmp (name, "plug_in_gap_decode_mpeg") == 0) 
  ||  (strcmp (name, "extension_gap_decode_mpeg") == 0))
  {
    l_filename[0] = '\0';
    strcpy(&l_frames_basename[0], "frame_");
    
    l_par = 1;
    if(strcmp(name, "plug_in_gap_decode_mpeg") == 0)
    {
      l_par = 3;
    }

    if( nparams > l_par)
    {
      if(param[l_par + 0].data.d_string != NULL)
      {
	strncpy(l_filename, param[l_par + 0].data.d_string, sizeof(l_filename) -1);
	l_filename[sizeof(l_filename) -1] = '\0';
      }
    }

    l_rc = 0;
    if (run_mode == GIMP_RUN_NONINTERACTIVE)
    {
       l_filename[0] = '\0';
       
       if(nparams != (l_par + 6))
       {
             l_rc = 1;            /* calling error */
       }
       else
       {
         first_frame = param[l_par + 2].data.d_int32;
         last_frame  = param[l_par + 3].data.d_int32;
	 if(param[l_par + 4].data.d_string != NULL)
	 {
           strcpy(l_frames_basename, param[l_par + 4].data.d_string);
	 }
         autoload = param[l_par + 5].data.d_int32;
       }
    }
    else
    {
       l_rc = load_range_dialog(&first_frame, &last_frame,
                                &l_filename[0], sizeof(l_filename),
                                &l_frames_basename[0], sizeof(l_frames_basename),
                                &autoload);
    }


    if(l_rc == 0)
    {
       if( (last_frame > 0) && (last_frame < first_frame))
       {
         /* swap, because load_image works only from lower to higher frame number */
         image_ID = load_image (&l_filename[0],
                                last_frame, first_frame,
                                &l_frames_basename[0],
                                autoload);
       }
       else
       {
         image_ID = load_image (&l_filename[0],
                               first_frame, last_frame,
                               &l_frames_basename[0],
                               autoload);
       }
    }
    
    if (image_ID != -1)
    {
      *nreturn_vals = 2;
      values[0].data.d_status = GIMP_PDB_SUCCESS;
      values[1].type = GIMP_PDB_IMAGE;
      values[1].data.d_image = image_ID;
    }
    else
    {
      values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
    }
  }

}	/* end run */


int p_does_exist(char *fname)
{
  struct stat  l_stat_buf;

  if (0 != stat(fname, &l_stat_buf))
  {
    /* stat error (file does not exist) */
    return(0);
  }
      
  return(1);
}	/* end p_does_exist */

static gint
p_overwrite_dialog(char *filename, gint overwrite_mode)
{
  static  t_but_arg  l_argv[3];
  static  t_arr_arg  argv[1];

  if(p_does_exist(filename))
  {
    if (overwrite_mode < 1)
    {
       l_argv[0].but_txt  = _("Overwrite Frame");
       l_argv[0].but_val  = 0;
       l_argv[1].but_txt  = _("Overwrite All");
       l_argv[1].but_val  = 1;
       l_argv[2].but_txt  = _("Cancel");
       l_argv[2].but_val  = -1;

       p_init_arr_arg(&argv[0], WGT_LABEL);
       argv[0].label_txt = filename;
    
       return(p_array_std_dialog ( _("GAP Question"),
                                   _("File already exists"),
				   1, argv,
				   3, l_argv, -1));
    }
  }
  return (overwrite_mode);
}

static gint
MPEG_frame_period_ms(gint mpeg_rate_code, char *basename)
{
  gint    l_rc;
  gdouble l_framerate;
  t_video_info *vin_ptr;
  
  vin_ptr = p_get_video_info(basename);
  l_rc = 0;
  l_framerate = 24.0;
  switch(mpeg_rate_code)
  {
    case 1: l_rc = 44; l_framerate = 23.976; break;
    case 2: l_rc = 42; l_framerate = 24.0;   break;
    case 3: l_rc = 40; l_framerate = 25.0;   break;
    case 4: l_rc = 33; l_framerate = 29.97;  break;
    case 5: l_rc = 33; l_framerate = 30.0;   break;
    case 6: l_rc = 20; l_framerate = 50.0;   break;
    case 7: l_rc = 17; l_framerate = 59.94;  break;
    case 8: l_rc = 17; l_framerate = 60.0;   break;
    case 0: /* ? */
    default:
      printf("mpeg: warning - this MPEG has undefined timing.\n");
      break;
  }
  if(vin_ptr)
  {
    if(l_rc != 0)  vin_ptr->framerate = l_framerate;
    p_set_video_info(vin_ptr ,basename);
    g_free(vin_ptr);
  }
  return(l_rc);
}


static char *
p_build_gap_framename(gint32 frame_nr, char *basename, char *ext)
{
   char *framename;
   framename = g_strdup_printf("%s%04d.%s", basename, (int)frame_nr, ext);
   return(framename);
}

static gint32 
load_image (char   *filename,
            gint32  first_frame,
            gint32  last_frame,
	    char    *basename,
	    gint32   autoload)
{
  GimpPixelRgn pixel_rgn;
  GimpDrawable *drawable;
  gint32 first_image_ID;
  gint32 image_ID;
  gint32 layer_ID;
  gchar *temp;
  Boolean moreframes;
  gint i, framenumber, delay;
  FILE *fp;
  guchar *data;
  gint wwidth, wheight;
  gint       l_visible;
  gint       l_overwrite_mode;

  /* mpeg structure */
  ImageDesc img;

  gchar *layername = NULL;
  gchar *framename = NULL;
  first_image_ID = -1;
  l_overwrite_mode = 0;


  if(!p_does_exist(filename))
  {
     printf("Error: file %s not found\n", filename);
     return (-1);
  }
  framename = p_build_gap_framename(first_frame, basename, "xcf");

  temp = g_malloc (strlen (filename) + 16);
  if (!temp) gimp_quit ();
  g_free (temp);


  
  gimp_progress_init (_("Decoding MPEG Movie..."));

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

  delay = MPEG_frame_period_ms(img.PictureRate, basename);

  /*
  printf("  <%d : %d>  %dx%d - d%d - bmp%d - ps%d - s%d\n",
	 img.PictureRate, delay,
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

  
  l_visible = TRUE;   /* loaded layer should be visible */
  moreframes = TRUE;
  framenumber = 1;
  while (moreframes)
  {
    g_free(framename);
    framename = p_build_gap_framename(framenumber, basename, "xcf");
    if (last_frame > 0)
    {
       gimp_progress_update ((gdouble)framenumber / (gdouble)last_frame );
    }
    else
    {
       /* toggle progressbar, because we dont know how much frames to handle
        * until its all done
        */
       gimp_progress_update ((framenumber&1) ? 1.0 : 0.0);
    }

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

    /* stop if desired range is loaded 
     * (negative last_frame ==> load until last frame)
     */
    if ((framenumber > last_frame ) && (last_frame > 0)) break;

    /* ignore frames before first_frame */    
    if (framenumber >= first_frame )
    {
       image_ID = gimp_image_new (wwidth, wheight, GIMP_RGB);
       gimp_image_set_filename (image_ID, framename);

       if(framenumber == first_frame)
       {
         first_image_ID = image_ID;
       }
    
       if (delay > 0)
         layername = g_strdup_printf("Frame %d (%dms)",
                 framenumber, delay);
       else
         layername = g_strdup_printf("Frame %d",
                 framenumber);

       layer_ID = gimp_layer_new (image_ID, layername,
                                  wwidth,
                                  wheight,
                                  GIMP_RGBA_IMAGE, 100, GIMP_NORMAL_MODE);
       g_free(layername);
       gimp_image_add_layer (image_ID, layer_ID, 0);
       gimp_layer_set_visible(layer_ID, l_visible);
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

       /* save each image as frame to disk */       
       {
         GimpParam* l_params;
	 gint    l_retvals;

         l_overwrite_mode = p_overwrite_dialog(framename, l_overwrite_mode);
	 
         if (l_overwrite_mode < 0)
	 {
	    /* fake no more frames if CANCEL was pressed in the Overwrite dialog */
	    moreframes = FALSE;
	    autoload = FALSE;    /* drop image and do not open on CANCEL */
	 }
	 else
	 {
           l_params = gimp_run_procedure ("gimp_xcf_save",
			           &l_retvals,
			           GIMP_PDB_INT32,    GIMP_RUN_NONINTERACTIVE,
			           GIMP_PDB_IMAGE,    image_ID,
			           GIMP_PDB_DRAWABLE, 0,
			           GIMP_PDB_STRING, framename,
			           GIMP_PDB_STRING, framename, /* raw name ? */
			           GIMP_PDB_END);
           p_gimp_file_save_thumbnail(image_ID, framename);
	 }

       }

       if((first_image_ID != image_ID) || ( !autoload))
       {
           gimp_image_delete(image_ID);
       }

    }


    framenumber++;  

  }

  CloseMPEG();
  fclose(fp);
      
  gimp_progress_update (1.0);

  g_free (data);
  if(autoload)
  {
    gimp_display_new(first_image_ID);
    return first_image_ID;
  }
  return -1;

}

static gint32
load_range_dialog(gint32 *first_frame,
                  gint32 *last_frame,
		  char *filename,
		  gint32 len_filename,
		  char *basename,
		  gint32 len_basename,
		  gint32 *autoload)
{
#define ARGC_DIALOG 6
  static t_arr_arg  argv[ARGC_DIALOG];

  p_init_arr_arg(&argv[0], WGT_FILESEL);
  argv[0].label_txt = _("Video");
  argv[0].help_txt  = _("Name of the MPEG1 videofile to READ.\n"
                        "Frames are extracted from the videofile\n"
			"and written to seperate diskfiles.\n"
			"Audiotracks in the videofile are ignored.");
  argv[0].text_buf_len = len_filename;
  argv[0].text_buf_ret = filename;
  argv[0].entry_width = 250;

  p_init_arr_arg(&argv[1], WGT_INT_PAIR);
  argv[1].label_txt = _("From:");
  argv[1].help_txt  = _("Framenumber of 1st frame to extract");
  argv[1].constraint = FALSE;
  argv[1].int_min    = 1;
  argv[1].int_max    = 9999;
  argv[1].int_ret    = 1;
  argv[1].umin       = 0;
  argv[1].entry_width = 80;
  
  p_init_arr_arg(&argv[2], WGT_INT_PAIR);
  argv[2].label_txt = _("To:");
  argv[2].help_txt  = _("Framenumber of last frame to extract");
  argv[2].constraint = FALSE;
  argv[2].int_min    = 1;
  argv[2].int_max    = 9999;
  argv[2].int_ret    = 9999;
  argv[2].umin       = 0;
  argv[2].entry_width = 80;
  
  p_init_arr_arg(&argv[3], WGT_FILESEL);
  argv[3].label_txt = _("Framenames:");
  argv[3].help_txt  = _("Basename for the AnimFrames to write on disk\n"
                        "(framenumber and .xcf is added)");
  argv[3].text_buf_len = len_basename;
  argv[3].text_buf_ret = basename;
  argv[3].entry_width = 250;
  
  p_init_arr_arg(&argv[4], WGT_TOGGLE);
  argv[4].label_txt = _("Open");
  argv[4].help_txt  = _("Open the 1st one of the extracted frames");
  argv[4].int_ret    = 1;
  
  p_init_arr_arg(&argv[5], WGT_LABEL_LEFT);
  argv[5].label_txt = _("\nWARNING: Do not attempt to split other files than MPEG1 videos.\n"
                         "Before you proceed, you should save all open images.");
  
  if(TRUE == p_array_dialog (_("Split MPEG1 Video to Frames"), 
			     _("Select Frame Range"), 
			     ARGC_DIALOG, argv))
  {
     *first_frame = (long)(argv[1].int_ret);
     *last_frame  = (long)(argv[2].int_ret);
     *autoload    = (long)(argv[4].int_ret);
     return (0);    /* OK */
  }
  else
  {
     return -1;     /* Cancel */
  }
   

}
