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

/* JPEG loading and saving file filter for the GIMP
 *  -Peter Mattis
 *
 * This filter is heavily based upon the "example.c" file in libjpeg.
 * In fact most of the loading and saving code was simply cut-and-pasted
 *  from that file. The filter, therefore, also uses libjpeg.
 */

/* 11-JAN-99 - Added support for JPEG comments and Progressive saves.
 *  -pete whiting <pwhiting@sprint.net>
 *
 * Comments of size up to 32k can be stored in the header of jpeg
 * files.  (They are not compressed.)  The JPEG specs and libraries
 * support the storing of multiple comments.  The behavior of this
 * code is to merge all comments in a loading image into a single
 * comment (putting \n between each) and attach that string as a
 * parasite - gimp-comment - to the image.  When saving, the image is
 * checked to see if it has the gimp-comment parasite - if so, that is
 * used as the default comment in the save dialog.  Further, the other
 * jpeg parameters (quaility, smoothing, compression and progressive)
 * are attached to the image as a parasite.  This allows the
 * parameters to remain consistent between saves.  I was not able to
 * figure out how to determine the quaility, smoothing or compression
 * values of an image as it is being loaded, but the code is there to
 * support it if anyone knows how.  Progressive mode is a method of
 * saving the image such that as a browser (or other app supporting
 * progressive loads - gimp doesn't) loads the image it first gets a
 * low res version displayed and then the image is progressively
 * enhanced until you get the final version.  It doesn't add any size
 * to the image (actually it often results in smaller file size) - the
 * only draw backs are that progressive jpegs are not supported by some
 * older viewers/browsers, and some might find it annoying.
 */

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <jerror.h>
#include "config.h"    /* configure cares about HAVE_PROGRESSIVE_JPEG */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#define SCALE_WIDTH 125

/* if you are not compiling this from inside the gimp tree, you have to  */
/* take care yourself if your JPEG library supports progressive mode     */
/* #undef HAVE_PROGRESSIVE_JPEG   if your library don't support it       */
/* #define HAVE_PROGRESSIVE_JPEG  if your library knows how to handle it */

#define DEFAULT_QUALITY 0.75
#define DEFAULT_SMOOTHING 0.0
#define DEFAULT_OPTIMIZE 1
#define DEFAULT_PROGRESSIVE 0
#define DEFAULT_COMMENT "Created with The GIMP"

typedef struct
{
  gdouble quality;
  gdouble smoothing;
  gint optimize;
  gint progressive;
} JpegSaveVals;

typedef struct
{
  gint  run;
} JpegSaveInterface;

/* Declare local functions.
 */
static void   query      (void);
static void   run        (char    *name,
			  int      nparams,
			  GParam  *param,
			  int     *nreturn_vals,
			  GParam **return_vals);
static gint32 load_image (char   *filename);
static gint   save_image (char   *filename,
			  gint32  image_ID,
			  gint32  drawable_ID);

static gint   save_dialog ();

static void   save_close_callback  (GtkWidget *widget,
				    gpointer   data);
static void   save_ok_callback     (GtkWidget *widget,
				    gpointer   data);
static void   save_scale_update    (GtkAdjustment *adjustment,
				    double        *scale_val);
static void   save_optimize_update (GtkWidget *widget,
				    gpointer   data);
static void   save_progressive_toggle (GtkWidget *widget,
				       gpointer   data);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static JpegSaveVals jsvals =
{
  DEFAULT_QUALITY,
  DEFAULT_SMOOTHING,
  DEFAULT_OPTIMIZE,
  DEFAULT_PROGRESSIVE
};

static JpegSaveInterface jsint =
{
  FALSE   /*  run  */
};

char *image_comment=NULL;

MAIN ()

static void
query ()
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name of the file to load" },
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename", "The name of the file to save the image in" },
    { PARAM_FLOAT, "quality", "Quality of saved image (0 <= quality <= 1)" },
    { PARAM_FLOAT, "smoothing", "Smoothing factor for saved image (0 <= smoothing <= 1)" },
    { PARAM_INT32, "optimize", "Optimization of entropy encoding parameters" },
    { PARAM_INT32, "progressive", "Enable progressive jpeg image loading - ignored if not compiled with HAVE_PROGRESSIVE_JPEG" },
    { PARAM_STRING, "comment", "Image comment" }
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_jpeg_load",
                          "loads files of the jpeg file format",
			  "FIXME: write help for jpeg_load",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1995-1996",
			  "<Load>/Jpeg",
			  NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_jpeg_save",
                          "saves files in the jpeg file format",
			  "FIXME: write help for jpeg_save",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1995-1996",
                          "<Save>/Jpeg",
			  "RGB*, GRAY*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_jpeg_load", "jpg,jpeg", "", "6,string,JFIF");
  gimp_register_save_handler ("file_jpeg_save", "jpg,jpeg", "");
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gint32 image_ID;
#ifdef GIMP_HAVE_PARASITES
  Parasite *parasite;
#endif /* GIMP_HAVE_PARASITES */

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_jpeg_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
	{
	  *nreturn_vals = 2;
	  values[0].data.d_status = STATUS_SUCCESS;
	  values[1].type = PARAM_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  values[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
    }
  else if (strcmp (name, "file_jpeg_save") == 0)
    {
      image_ID = param[1].data.d_int32;
      if(image_comment) {
	g_free(image_comment);
	image_comment=NULL;
      }
#ifdef GIMP_HAVE_PARASITES
      parasite = gimp_image_find_parasite(image_ID, "gimp-comment");
      if (parasite) {
        image_comment = g_strdup(parasite->data);
	parasite_free(parasite);
      }
#endif /* GIMP_HAVE_PARASITES */
      if (!image_comment) image_comment = g_strdup(DEFAULT_COMMENT);
      jsvals.quality = DEFAULT_QUALITY;
      jsvals.smoothing = DEFAULT_SMOOTHING;
      jsvals.optimize = DEFAULT_OPTIMIZE;
      jsvals.progressive = DEFAULT_PROGRESSIVE;
      
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_jpeg_save", &jsvals);

#ifdef GIMP_HAVE_PARASITES
	  /* load up the previously used values */
	  parasite = gimp_image_find_parasite(image_ID, "jpeg-save-options");
	  if (parasite)
	  {
	    jsvals.quality     = ((JpegSaveVals *)parasite->data)->quality;
	    jsvals.smoothing   = ((JpegSaveVals *)parasite->data)->smoothing;
	    jsvals.optimize    = ((JpegSaveVals *)parasite->data)->optimize;
	    jsvals.progressive = ((JpegSaveVals *)parasite->data)->progressive;
	    parasite_free(parasite);
	  }
#endif /* GIMP_HAVE_PARASITES */
	  
	  /*  First acquire information with a dialog  */
	  if (! save_dialog ())
	    return;
	  break;

	case RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  /*  pw - added two more progressive and comment */
	  if (nparams != 10)
	    status = STATUS_CALLING_ERROR;
	  if (status == STATUS_SUCCESS)
	    {
	      jsvals.quality = param[5].data.d_float;
	      jsvals.smoothing = param[6].data.d_float;
	      jsvals.optimize = param[7].data.d_int32;
#ifdef HAVE_PROGRESSIVE_JPEG
	      jsvals.progressive = param[8].data.d_int32;
#endif /* HAVE_PROGRESSIVE_JPEG */
	      /* free up the default -- wasted some effort earlier */
	      if(image_comment) g_free(image_comment);
	      image_comment=g_strdup(param[9].data.d_string);
	    }
	  if (status == STATUS_SUCCESS &&
	      (jsvals.quality < 0.0 || jsvals.quality > 1.0))
	    status = STATUS_CALLING_ERROR;
	  if (status == STATUS_SUCCESS &&
	      (jsvals.smoothing < 0.0 || jsvals.smoothing > 1.0))
	    status = STATUS_CALLING_ERROR;
	  break;

	case RUN_WITH_LAST_VALS:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_jpeg_save", &jsvals);
#ifdef GIMP_HAVE_PARASITES
	  parasite = gimp_image_find_parasite(image_ID, "jpeg-save-options");
	  if (parasite)
	  {
	    jsvals.quality     = ((JpegSaveVals *)parasite->data)->quality;
	    jsvals.smoothing   = ((JpegSaveVals *)parasite->data)->smoothing;
	    jsvals.optimize    = ((JpegSaveVals *)parasite->data)->optimize;
	    jsvals.progressive = ((JpegSaveVals *)parasite->data)->progressive;
	    parasite_free(parasite);
	  }
#endif /* GIMP_HAVE_PARASITES */
	  break;

	default:
	  break;
	}

      *nreturn_vals = 1;
      if (save_image (param[3].data.d_string,
		      param[1].data.d_int32,
		      param[2].data.d_int32))
	{
	  /*  Store mvals data  */
	  gimp_set_data ("file_jpeg_save", &jsvals, sizeof (JpegSaveVals));

	  values[0].data.d_status = STATUS_SUCCESS;
	}
      else
	values[0].data.d_status = STATUS_EXECUTION_ERROR;

#ifdef GIMP_HAVE_PARASITES

      /* pw - now we need to change the defaults to be whatever
       * was used to save this image.  Dump the old parasites
       * and add new ones. */
      
      gimp_image_detach_parasite(image_ID,"gimp-comment");
      if(strlen(image_comment)) {
	parasite=parasite_new("gimp-comment",
			      PARASITE_PERSISTENT,
			      strlen(image_comment)+1,
			      image_comment);
	gimp_image_attach_parasite(image_ID,parasite);
	parasite_free(parasite);
      }
      gimp_image_detach_parasite(image_ID, "jpeg-save-options");
      
      parasite=parasite_new("jpeg-save-options",0,sizeof(jsvals),&jsvals);
      gimp_image_attach_parasite(image_ID,parasite);
      parasite_free(parasite);
    
#endif /* Have Parasites */  
      

      
    }
}




/* Read next byte */
static unsigned int
jpeg_getc (j_decompress_ptr cinfo)
{
  struct jpeg_source_mgr * datasrc = cinfo->src;

  if (datasrc->bytes_in_buffer == 0) {
    if (! (*datasrc->fill_input_buffer) (cinfo))
      ERREXIT(cinfo, JERR_CANT_SUSPEND);
  }
  datasrc->bytes_in_buffer--;
  return *datasrc->next_input_byte++;
}

/* We need our own marker parser, since early versions of libjpeg
 * don't keep a conventient list of the marker's that have been
 * seen. */

/*
 * Marker processor for COM markers.
 * This replaces the library's built-in processor, which just skips the marker.
 * Note this code relies on a non-suspending data source.
 */
static GString *local_image_comments=NULL;

static boolean
COM_handler (j_decompress_ptr cinfo)
{
  int length;
  unsigned int ch;

  length = jpeg_getc (cinfo) << 8;
  length += jpeg_getc (cinfo);
  if (length < 2)
    return FALSE;
  length -= 2;			/* discount the length word itself */

  if (!local_image_comments)
    local_image_comments = g_string_new (NULL);
  else
    g_string_append_c (local_image_comments, '\n');

  while (length-- > 0)
  {
    ch = jpeg_getc (cinfo);
    g_string_append_c (local_image_comments, ch);
  }

  return TRUE;
}


typedef struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
} *my_error_ptr;


/*
 * Here's the routine that will replace the standard error_exit method:
 */

static void
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp (myerr->setjmp_buffer, 1);
}

static gint32
load_image (char *filename)
{
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  gint32 volatile image_ID;
  gint32 layer_ID;
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;
  FILE *infile;
  guchar *buf;
  guchar **rowbuf;
  char *name;
  int image_type;
  int layer_type;
  int tile_height;
  int scanlines;
  int i, start, end;

#ifdef GIMP_HAVE_PARASITES
  JpegSaveVals local_save_vals;
  Parasite *comment_parasite;
  Parasite *vals_parasite;
#endif /* GIMP_HAVE_PARASITES */

  
  /* We set up the normal JPEG error routines. */
  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  if ((infile = fopen (filename, "rb")) == NULL)
    {
      g_warning ("can't open \"%s\"\n", filename);
      gimp_quit ();
    }

  name = malloc (strlen (filename) + 12);
  sprintf (name, "Loading %s:", filename);
  gimp_progress_init (name);
  free (name);

  image_ID = -1;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp (jerr.setjmp_buffer))
    {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, close the input file, and return.
       */
      jpeg_destroy_decompress (&cinfo);
      if (infile)
	fclose (infile);
      if (image_ID != -1)
	gimp_image_delete (image_ID);
      gimp_quit ();
    }
  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress (&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src (&cinfo, infile);

  /* pw - step 2.1 let the lib know we want the comments. */

  jpeg_set_marker_processor (&cinfo, JPEG_COM, COM_handler);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header (&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

#ifdef GIMP_HAVE_PARASITES

  /* if we had any comments then make a parasite for them */
  if(local_image_comments) {
    char *string=local_image_comments->str;
    g_string_free(local_image_comments,FALSE);
    comment_parasite = parasite_new("gimp-comment", PARASITE_PERSISTENT,
				    strlen(string)+1,string);
  } else {
    comment_parasite=NULL;
  }
  
  /* pw - figuring out what the saved values were is non-trivial.
   * They don't seem to be in the cinfo structure. For now, I will
   * just use the defaults, but if someone figures out how to derive
   * them this is the place to store them. */

  local_save_vals.quality=DEFAULT_QUALITY;
  local_save_vals.smoothing=DEFAULT_SMOOTHING;
  local_save_vals.optimize=DEFAULT_OPTIMIZE;
  
#ifdef HAVE_PROGRESSIVE_JPEG 
  local_save_vals.progressive=cinfo.progressive_mode;
#else
  local_save_vals.progressive=0;
#endif /* HAVE_PROGRESSIVE_JPEG */
  
  vals_parasite = parasite_new("jpeg-save-options", 0,
			       sizeof(local_save_vals), &local_save_vals);
  
#endif /* GIMP_HAVE_PARASITES */
  
  
  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */

  jpeg_start_decompress (&cinfo);

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */
  /* temporary buffer */
  tile_height = gimp_tile_height ();
  buf = g_new (guchar, tile_height * cinfo.output_width * cinfo.output_components);
  rowbuf = g_new (guchar*, tile_height);

  for (i = 0; i < tile_height; i++)
    rowbuf[i] = buf + cinfo.output_width * cinfo.output_components * i;

  /* Create a new image of the proper size and associate the filename with it.
   */
  switch (cinfo.output_components)
    {
    case 1:
      image_type = GRAY;
      layer_type = GRAY_IMAGE;
      break;
    case 3:
      image_type = RGB;
      layer_type = RGB_IMAGE;
      break;
    default:
      g_message ("don't know how to load JPEGs\nwith %d color channels",
		 cinfo.output_components);
      gimp_quit ();
    }

  image_ID = gimp_image_new (cinfo.output_width, cinfo.output_height, image_type);
  gimp_image_set_filename (image_ID, filename);

  layer_ID = gimp_layer_new (image_ID, "Background",
			     cinfo.output_width,
			     cinfo.output_height,
			     layer_type, 100, NORMAL_MODE);
  gimp_image_add_layer (image_ID, layer_ID, 0);

  drawable = gimp_drawable_get (layer_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, FALSE);

  /* Step 5.1: if the file had resolution information, set it on the
   * image */
#ifdef GIMP_HAVE_RESOLUTION_INFO
  if (cinfo.saw_JFIF_marker)
  {
    float xresolution;
    float yresolution;
    float asymmetry;

    xresolution = cinfo.X_density;
    yresolution = cinfo.Y_density;

    switch (cinfo.density_unit) {
    case 0: /* unknown */
	asymmetry = xresolution / yresolution;
	xresolution = 72 * asymmetry;
	yresolution = 72;
	break;

    case 1: /* dots per inch */
	break;

    case 2: /* dots per cm */
	xresolution *= 2.54;
	yresolution *= 2.54;
	break;

    default:
	g_message ("unknown density unit %d\nassuming dots per inch",
		   cinfo.density_unit);
	break;
      }

    if (xresolution > 1e-5 && yresolution > 1e-5)
      gimp_image_set_resolution (image_ID, xresolution, yresolution);
  }
#endif /* GIMP_HAVE_RESOLUTION_INFO */


  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < cinfo.output_height)
    {
      start = cinfo.output_scanline;
      end = cinfo.output_scanline + tile_height;
      end = MIN (end, cinfo.output_height);
      scanlines = end - start;

      for (i = 0; i < scanlines; i++)
	jpeg_read_scanlines (&cinfo, (JSAMPARRAY) &rowbuf[i], 1);

      /*
      for (i = start; i < end; i++)
	gimp_pixel_rgn_set_row (&pixel_rgn, tilerow[i - start], 0, i, drawable->width);
	*/

      gimp_pixel_rgn_set_rect (&pixel_rgn, buf, 0, start, drawable->width, scanlines);

      gimp_progress_update ((double) cinfo.output_scanline / (double) cinfo.output_height);
    }

  /* Step 7: Finish decompression */

  jpeg_finish_decompress (&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress (&cinfo);

  /* free up the temporary buffers */
  g_free (rowbuf);
  g_free (buf);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  fclose (infile);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.num_warnings is nonzero).
   */

  /* Tell the GIMP to display the image.
   */
  gimp_drawable_flush (drawable);

  /* pw - Last of all, attach the parasites (couldn't do it earlier -
     there was no image. */
  
  if(comment_parasite){
    gimp_image_attach_parasite(image_ID, comment_parasite);
    parasite_free(comment_parasite);
  }
  if(vals_parasite){
    gimp_image_attach_parasite(image_ID, vals_parasite);
    parasite_free(vals_parasite);
  }   

  return image_ID;
}

static gint
save_image (char   *filename,
	    gint32  image_ID,
	    gint32  drawable_ID)
{
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  GDrawableType drawable_type;
  struct jpeg_compress_struct cinfo;
  struct my_error_mgr jerr;
  FILE * volatile outfile;
  guchar *temp, *t;
  guchar *data;
  guchar *src, *s;
  char *name;
  int has_alpha;
  int rowstride, yend;
  int i, j;

  drawable = gimp_drawable_get (drawable_ID);
  drawable_type = gimp_drawable_type (drawable_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

  name = malloc (strlen (filename) + 11);

  sprintf (name, "Saving %s:", filename);
  gimp_progress_init (name);
  free (name);

  /* Step 1: allocate and initialize JPEG compression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error (&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  outfile = NULL;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp (jerr.setjmp_buffer))
    {
      /* If we get here, the JPEG code has signaled an error.
       * We need to clean up the JPEG object, close the input file, and return.
       */
      jpeg_destroy_compress (&cinfo);
      if (outfile)
	fclose (outfile);
      if (drawable)
	gimp_drawable_detach (drawable);

      return FALSE;
    }

  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress (&cinfo);

  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */

  /* Here we use the library-supplied code to send compressed data to a
   * stdio stream.  You can also write your own code to do something else.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to write binary files.
   */
  if ((outfile = fopen (filename, "wb")) == NULL)
    {
      g_message ("can't open %s\n", filename);
      return FALSE;
    }
  jpeg_stdio_dest (&cinfo, outfile);

  /* Get the input image and a pointer to its data.
   */
  switch (drawable_type)
    {
    case RGB_IMAGE:
    case GRAY_IMAGE:
      /* # of color components per pixel */
      cinfo.input_components = drawable->bpp;
      has_alpha = 0;
      break;
    case RGBA_IMAGE:
    case GRAYA_IMAGE:
      /*gimp_message ("jpeg: image contains a-channel info which will be lost");*/
      /* # of color components per pixel (minus the GIMP alpha channel) */
      cinfo.input_components = drawable->bpp - 1;
      has_alpha = 1;
      break;
    case INDEXED_IMAGE:
      /*gimp_message ("jpeg: cannot operate on indexed color images");*/
      return FALSE;
      break;
    default:
      /*gimp_message ("jpeg: cannot operate on unknown image types");*/
      return FALSE;
      break;
    }

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  /* image width and height, in pixels */
  cinfo.image_width = drawable->width;
  cinfo.image_height = drawable->height;
  /* colorspace of input image */
  cinfo.in_color_space = (drawable_type == RGB_IMAGE ||
			  drawable_type == RGBA_IMAGE)
    ? JCS_RGB : JCS_GRAYSCALE;
  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults (&cinfo);
  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  jpeg_set_quality (&cinfo, (int) (jsvals.quality * 100), TRUE /* limit to baseline-JPEG values */);
  cinfo.smoothing_factor = (int) (jsvals.smoothing * 100);
  cinfo.optimize_coding = jsvals.optimize;

#ifdef HAVE_PROGRESSIVE_JPEG
  if(jsvals.progressive) {
    jpeg_simple_progression(&cinfo);
  }
#endif /* HAVE_PROGRESSIVE_JPEG */

#ifdef GIMP_HAVE_RESOLUTION_INFO
  {
    float xresolution;
    float yresolution;

    gimp_image_get_resolution (image_ID, &xresolution, &yresolution);

    if (xresolution > 1e-5 && yresolution > 1e-5)
    {
      cinfo.density_unit = 1;  /* dots per inch */
      cinfo.X_density = xresolution;
      cinfo.Y_density = yresolution;
    }
  }
#endif /* GIMP_HAVE_RESOLUTION_INFO */
  
  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress (&cinfo, TRUE);

  /* Step 4.1: Write the comment out - pw */
  if(image_comment) {
    jpeg_write_marker(&cinfo,
		      JPEG_COM,
		      image_comment,
		      strlen(image_comment));
  }
    
  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  /* JSAMPLEs per row in image_buffer */
  rowstride = drawable->bpp * drawable->width;
  temp = (guchar *) malloc (cinfo.image_width * cinfo.input_components);
  data = (guchar *) malloc (rowstride * gimp_tile_height ());

  /* fault if cinfo.next_scanline isn't initially a multiple of
   * gimp_tile_height */
  src = NULL;

  while (cinfo.next_scanline < cinfo.image_height)
    {
      if ((cinfo.next_scanline % gimp_tile_height ()) == 0)
	{
	  yend = cinfo.next_scanline + gimp_tile_height ();
	  yend = MIN (yend, cinfo.image_height);
	  gimp_pixel_rgn_get_rect (&pixel_rgn, data, 0, cinfo.next_scanline, cinfo.image_width,
				   (yend - cinfo.next_scanline));
	  src = data;
	}

      t = temp;
      s = src;
      i = cinfo.image_width;

      while (i--)
	{
	  for (j = 0; j < cinfo.input_components; j++)
	    *t++ = *s++;
	  if (has_alpha)  /* ignore alpha channel */
	    s++;
	}

      src += rowstride;
      jpeg_write_scanlines (&cinfo, (JSAMPARRAY) &temp, 1);

      if ((cinfo.next_scanline % 5) == 0)
	gimp_progress_update ((double) cinfo.next_scanline / (double) cinfo.image_height);
    }

  /* Step 6: Finish compression */
  jpeg_finish_compress (&cinfo);
  /* After finish_compress, we can close the output file. */
  fclose (outfile);

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress (&cinfo);
  /* free the temporary buffer */
  free (temp);
  free (data);

  /* And we're done! */
  /*gimp_do_progress (1, 1);*/

  gimp_drawable_detach (drawable);

  return TRUE;
}

static gint
save_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *scale;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *toggle;
  GtkObject *scale_data;

  GtkWidget *progressive;
  
  GtkWidget *text;
  GtkWidget *com_frame;
  GtkWidget *com_table;
  GtkWidget *vscrollbar;
  
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("save");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Save as Jpeg");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) save_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) save_ok_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new ("Parameter Settings");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  table = gtk_table_new (4, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  label = gtk_label_new ("Quality");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 0);
  scale_data = gtk_adjustment_new (jsvals.quality, 0.0, 1.0, 0.01, 0.01, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) save_scale_update,
		      &jsvals.quality);
  gtk_widget_show (label);
  gtk_widget_show (scale);

  label = gtk_label_new ("Smoothing");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 0);
  scale_data = gtk_adjustment_new (jsvals.smoothing, 0.0, 1.0, 0.01, 0.01, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) save_scale_update,
		      &jsvals.smoothing);
  gtk_widget_show (label);
  gtk_widget_show (scale);

  toggle = gtk_check_button_new_with_label("Optimize");
  gtk_table_attach(GTK_TABLE(table), toggle, 0, 2, 2, 3, GTK_FILL, 0, 0, 0);
  gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
                     (GtkSignalFunc)save_optimize_update, NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), jsvals.optimize);
  gtk_widget_show(toggle);


  progressive = gtk_check_button_new_with_label("Progressive");
  gtk_table_attach(GTK_TABLE(table), progressive, 0, 2, 3, 4,
		   GTK_FILL, 0, 0, 0);
  gtk_signal_connect(GTK_OBJECT(progressive), "toggled",
                     (GtkSignalFunc)save_progressive_toggle, NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(progressive),
			       jsvals.progressive);
  gtk_widget_show(progressive);

#ifndef HAVE_PROGRESSIVE_JPEG
  gtk_widget_set_sensitive(progressive,FALSE);
#endif
  
  com_frame = gtk_frame_new ("Image Comments");
  gtk_frame_set_shadow_type (GTK_FRAME (com_frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (com_frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), com_frame, TRUE, TRUE, 0
);
  com_table = gtk_table_new (1, 1, FALSE);
  gtk_container_border_width (GTK_CONTAINER (com_table), 10);
  gtk_container_add (GTK_CONTAINER (com_frame), com_table);

  text = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (text), TRUE);
  if(image_comment) 
    gtk_text_insert(GTK_TEXT(text),NULL,NULL,NULL,image_comment,-1);
  gtk_table_attach (GTK_TABLE (com_table), text, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

    /* Add a vertical scrollbar to the GtkText widget */
  vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
  gtk_table_attach (GTK_TABLE (com_table), vscrollbar, 1, 2, 0, 1,
                    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (vscrollbar);

  /* pw - mild hack here.  I didn't like redoing the comment string
   * each time a character was typed, so I associated the text area
   * with the dialog.  That way, just before the dialog destroys
   * itself (once the ok button is hit) it can save whatever was in
   * the comment text area to the comment string.  See the
   * save-ok-callback for more details.  */
  
  gtk_object_set_data((GtkObject *)dlg,"text",text); 
  
  gtk_widget_show (text);
  gtk_widget_show (com_frame);
  gtk_widget_show (com_table);
 
  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return jsint.run;
}


/*  Save interface functions  */

static void
save_close_callback (GtkWidget *widget,
		     gpointer   data)
{
  gtk_main_quit ();
}

static void
save_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  GtkWidget *text;
  jsint.run = TRUE;

  /* pw - get the comment text object and grab it's data */
  text=gtk_object_get_data(GTK_OBJECT(data),"text");
  if(image_comment!=NULL) {
    g_free(image_comment);
    image_comment=NULL;
  }
  
  /* pw - gtk_editable_get_chars allocates a copy of the string, so
   * don't worry about the gtk_widget_destroy killing it.  */

  image_comment=gtk_editable_get_chars(GTK_EDITABLE (text),0,-1);
  
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
save_scale_update (GtkAdjustment *adjustment,
		   double        *scale_val)
{
  *scale_val = adjustment->value;
}

static void
save_optimize_update(GtkWidget *widget,
		     gpointer  data)
{
  jsvals.optimize = GTK_TOGGLE_BUTTON(widget)->active;
}

static void
save_progressive_toggle(GtkWidget *widget,
			gpointer  data)
{
  jsvals.progressive = GTK_TOGGLE_BUTTON(widget)->active;
}


