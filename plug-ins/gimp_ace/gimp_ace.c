/* :PREAMBLE: gimp_ace.c
 * Adaptive Contrast Enhancement plug-in for the GIMP (v1.0)
 * (A windowed histogram equalization using the Fourier-series based
 * fast algorithm.)
 * Based on J. Alex Stark's <stark@niss.org> GLACE system. 
 * GIMP Plug-in by Kevin Turner <kevint@poboxes.com> */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.  See the file COPYING for details.
 *
 */

/* :HEADERS: 
 */

#include "config.h" /* autostuff */

#ifndef GLACE_GIMP
#  define GLACE_GIMP
#endif

#include <libintl.h> /* i18n - gettext */
#define _(String) gettext (String)
#ifdef gettext_noop
# define N_(String) gettext_noop (String)
#else
# define N_(String) (String)
#endif

#include "gimp_ace.h" /* defines the AceValues struct */
#include "color.h" /* This has the RGB -> Y conversion factors.
		      for the SeperateChannels */


/* :*** Prototypes ***: 
 */

static void query (void);
static void run (gchar   *name,
		 gint     nparams,
		 GParam  *param,
		 gint    *nreturn_vals,
		 GParam **return_vals);

static gboolean do_ace(gint32 drawable_id);

static void GlaceGimp_ReadImgArrays(Glace_WData wData,
				    Glace_CfgInfo *cfgInfoPtr,
				    Glace_ImgArrays *imgArraysPtr,
				    GDrawable *drawable_ptr);

static int SeperateChannels (guchar *inbuf, const gint buflen,
			     const gboolean TypeRGB, const gboolean HasAlpha,
			     const Glace_ColorMethods color_method,
			     guchar **rbuf, guchar **gbuf, guchar **bbuf,
			     guchar **Graybuf, 
			     guchar **xbuf, guchar **ybuf,
			     guchar **ymaxbuf);

static void GlaceGimp_Process (
			       Glace_WData wData,
			       Glace_CfgInfo *cfgInfoPtr,
			       Glace_TableInfo *tableInfoPtr,
			       Glace_ImgArrays *imgArraysPtr);

/* static void GlaceGimp_ScanListfiles (Glace_CfgInfo *cfgInfoPtr); */

/* :*** Definitions ***:
 */

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static AceValues acevals =
{
	1.0, /* Strength (full) */
	0.0, /* brightness adjust (none) */
	0.0, /* foobarm (none) */
	0.0, /* smoothing (none) */
	10,  /* iterations */
	25,  /* window size */
	GLACE_COLOR_Yxy, /* Color method */
	TRUE /* link strength and brightness adjust in dialog. */
};

/* :*** MAIN ***: 
 */

MAIN()

/* :FUNCTION: query 
 */

static void
query ()
{
  static GParamDef params[] = {
    /* Required params. */
    { PARAM_INT32, "run_mode", "Interactive=0x0, Noninteractive=0x1" },
    { PARAM_IMAGE, "image_id", "(unused)" },
    { PARAM_DRAWABLE, "drawable_id", N_("Drawable to affect") },
    /* Our params. */
    { PARAM_INT32,  "ace_argc", "ace argument count" },
    { PARAM_STRINGARRAY, "ace_argv", "ace argument vector" },
  };
  const int nparams = sizeof (params) / sizeof (GParamDef);

  gimp_install_procedure ("plug_in_ace",
			  _("Adaptive Contrast Enhancement"),
			  _("FIXME: Write ACE help."),
			  "Alex Stark, Kevin Turner,",
			  "Alex Stark, Kevin Turner,",
			  "1998,1999",
			  "<Image>/Image/Colors/Auto/Adaptive Contrast",
			  "GRAY*, RGB*", /*FIXME*/
			  PROC_PLUG_IN,
			  nparams, /* # Params */
			  0,       /* # return values */
			  params,
			  NULL); /* GParamDef *return_vals */
}

/* :FUNCTION: run
 */

static void
run (gchar   *name, gint nparams,       GParam *param, 
                    gint *nreturn_vals, GParam **return_vals)
{
	static GParam retvals[1];
	gint needparams;
	GRunModeType run_mode;
	
	*return_vals=retvals;
	
	*nreturn_vals=1;
	retvals[0].type = PARAM_STATUS;  
	
	/* Guilty until proven innocent. */
	retvals[0].data.d_status = STATUS_CALLING_ERROR;
	
	run_mode=(GRunModeType) param[0].data.d_int32;

	switch(run_mode) {
        case RUN_NONINTERACTIVE:
		if (nparams < 3 ||
		    param[0].type != PARAM_INT32 || 
		    param[1].type != PARAM_IMAGE || 
		    param[2].type != PARAM_DRAWABLE) {
			g_warning("gimp_ace: Caller is on crack!  " 
				  "You're not passing me the parameters required for a plug-in.  "
				  "The first three types should be %d, %d, %d; but I'm reading "
				  "%d, %d, %d.\n",
				  PARAM_INT32, PARAM_IMAGE, PARAM_DRAWABLE,
				  param[0].type, param[1].type, param[2].type);
			return;
		}
		if (param[3].type != PARAM_INT32 ||
		    param[4].type != PARAM_STRING) {
			g_warning("gimp_ace: caller is passing ace_argc and ace_argv as "
				  "types %d and %d, should be %d and %d.\n",
				  param[3].type, param[4].type,
				  PARAM_INT32, PARAM_STRINGARRAY);
			return;
		} /* endif param.type */    
		/* FIXME: do something intelligent with the parameters. */
                break;
        case RUN_INTERACTIVE:
                retvals[0].data.d_status = STATUS_EXECUTION_ERROR;
                gimp_get_data ("plug_in_ace",&acevals);
                if(!ace_dialog (-1, &acevals)) {
                        /* No error, but dialog cancelled. */
                        retvals[0].data.d_status = STATUS_EXECUTION_ERROR;
                        return;
                } /* else continue on and do antialias below. */
                gimp_set_data ("plug_in_ace", &acevals, 
			       sizeof (AceValues)); 
                break;
        case RUN_WITH_LAST_VALS:
                gimp_get_data ("plug_in_ace", &acevals);
                break;
        }

	retvals[0].data.d_status = STATUS_EXECUTION_ERROR;

	retvals[0].data.d_status = do_ace(param[2].data.d_drawable)
		? STATUS_SUCCESS : STATUS_EXECUTION_ERROR;
}

/* :*** Support Functions ***: 
 */

/* :FUNCTION: do_ace(gint32 drawable_id)
 */
static gboolean
do_ace(gint32 drawable_id)
{
  gchar *ace_argv[] = { "-reset",
  			    "25", 
			    "25", 
			    "10", 
		       "-factor", 
			"0.0", /* 1.0 is identity, 0.0 is full effect. */
		     "-prescale", 
			   "1.0", 
			"-gauss", 
			     "0", 
                      "-hamming",
                           "-gg",
		     "-passthru", 
			   "1.0",
		     "-inoffset", 
			 "127.5", 
			  "-tol", 
			   "0.0" };
  gint ace_argc=18;

  /* -reset 25 25 10 -cumu -1.2 1.2 361 -factor 1.0 -prescale 1.0 -gauss
     0 -gg -passthru 1.0 -inoffset 127.5 -tol 0.0 -input inImg -output outImg1*/

  /* gchar *ace_argv[] =  { "-reset", "25", "25", "5", "-cumu", "-1.2", 
     "1.2", "361", "-factor", "0.0", "-prescale",
     "1.0", "-gauss", "0", "-gg", 
     "-passthru", "1.00", "-inoffset", "127.5", "-tol", "0.0"}; */

  Glace_ClientData *clientData;
  GDrawable *drawable_ptr = gimp_drawable_get(drawable_id);

  gimp_progress_init(_("Adaptive Contrast Enhancement:"));
#if 0
  g_message("gimp_ace PID: %d\n",getpid());
      kill(getpid(),19);
#endif

  /* I don't know what most of this does.
     It's copied out of pnmace.c */
  clientData = Glace_AllocClientData ();
  Glace_WInit(GLACE_CDATA(wData));

  Glace_CfgInit ( GLACE_CDATA(cfgInfoPtr) );

  Glace_ParseArgs ( GLACE_CDATA(cfgInfoPtr),
		    GLACE_CDATA(imgArraysPtr), 
		    ace_argc, ace_argv);

  clientData->cfgInfoPtr->heFactor = 1.0 - acevals.strength;
  clientData->cfgInfoPtr->passthruFactor = 1.0 - acevals.bradj;
  clientData->cfgInfoPtr->numTerms = acevals.iterations;
  clientData->cfgInfoPtr->windBaseW = acevals.wsize;
  clientData->cfgInfoPtr->windBaseH = acevals.wsize;
  clientData->cfgInfoPtr->coeffTol = acevals.coefftol;
  clientData->cfgInfoPtr->gaussCwindWidth = acevals.smoothing;
  clientData->cfgInfoPtr->colorMethod= acevals.color_method;

  GLACE_ERROR_EXIT( GLACE_CDATA(wData) );
  Glace_CfgAllocSeriesVectors ( GLACE_CDATA(cfgInfoPtr) );
  /* FIXME:  GlaceGimp_ScanListfiles ( GLACE_CDATA(cfgInfoPtr) ); */
  GLACE_ERROR_EXIT( GLACE_CDATA(wData) );
  
  Glace_CfgBeginToHeseries ( GLACE_CDATA(cfgInfoPtr) );
  GLACE_ERROR_EXIT( GLACE_CDATA(wData) );
  Glace_CfgHeseriesToAddback ( GLACE_CDATA(cfgInfoPtr) );

  Glace_CFuncGen ( GLACE_CDATA(cfgInfoPtr) );

  /* This also allocs the input/ref/ddH/ddV image arrays */
  /* FIXME */ 
  GlaceGimp_ReadImgArrays ( GLACE_CDATA(wData), GLACE_CDATA(cfgInfoPtr),
			    GLACE_CDATA(imgArraysPtr), drawable_ptr );
  GLACE_ERROR_EXIT( GLACE_CDATA(wData) );

  gimp_progress_update(0.1);

  Glace_CfgAddbackToEnd ( GLACE_CDATA(cfgInfoPtr) );

  Glace_AllocTables ( GLACE_CDATA(cfgInfoPtr), GLACE_CDATA(tableInfoPtr) );
  Glace_DefaultTmpImg ( GLACE_CDATA(cfgInfoPtr), GLACE_CDATA(imgArraysPtr) );
  Glace_AllocImgArrays ( GLACE_CDATA(cfgInfoPtr), GLACE_CDATA(imgArraysPtr) );

  GlaceGimp_Process ( GLACE_CDATA(wData), GLACE_CDATA(cfgInfoPtr),
		  GLACE_CDATA(tableInfoPtr), GLACE_CDATA(imgArraysPtr));
  GLACE_ERROR_EXIT( GLACE_CDATA(wData) );

  Glace_CfgFreeSeriesVectors ( GLACE_CDATA(cfgInfoPtr) );
  Glace_FreeImgArrays ( GLACE_CDATA(cfgInfoPtr), GLACE_CDATA(imgArraysPtr) );
  Glace_FreeTables ( GLACE_CDATA(cfgInfoPtr), GLACE_CDATA(tableInfoPtr) );
  
  Glace_FreeClientData ( clientData );

  /* GlaceGimp_CloseFile (stdout); */

  return TRUE;
}

/* :FUNCTION: FIXME GlaceGimp_ReadImgArrays
 * Assumptions: Input is one byte-per-pixel grayscale.
 */

static void 
GlaceGimp_ReadImgArrays(Glace_WData wData, Glace_CfgInfo *cfgInfoPtr,
			Glace_ImgArrays *imgArraysPtr, 
			GDrawable *drawable_ptr)
{
  guchar *gimp_buf=NULL;
  gint gimp_size;
  GPixelRgn src_rgn;
  gint x1, y1, x2, y2; /* Bounds of the selection */

  /* Nurb Nurb Oink. */

  GLACE_IMG(inImgPtr) =
	  GLACE_IMG(inImgCxPtr) =
	  GLACE_IMG(inImgCyPtr) =
	  GLACE_IMG(inImgCYMaxPtr) =
	  GLACE_IMG(inImgRPtr) =
	  GLACE_IMG(inImgGPtr) =
	  GLACE_IMG(inImgBPtr) = NULL;

  WRAPPER(drawable_ptr)=drawable_ptr;

  gimp_drawable_mask_bounds (drawable_ptr->id, &x1, &y1, &x2, &y2);

  (((GlaceGimpData *)wData)->gimp_x0 ) =x1;
  (((GlaceGimpData *)wData)->gimp_y0 ) =y1;
  (imgArraysPtr->cols ) =x2-x1;
  (imgArraysPtr->rows ) =y2-y1;

  gimp_pixel_rgn_init(&src_rgn,
		      WRAPPER(drawable_ptr),
		      WRAPPER(gimp_x0), WRAPPER(gimp_y0),
		      GLACE_IMG(cols), GLACE_IMG(rows),
		      FALSE, FALSE);

  /* skip some pixels on output so we can mix the alpha back in. */
  GLACE_IMG(pixelBytePad) = gimp_drawable_has_alpha(drawable_ptr->id) ? 1 : 0;

  /* This refers to precision, not number of channels. */
  GLACE_IMG(pixelSize) = 1; 

  gimp_size = GLACE_IMG(cols) * GLACE_IMG(rows) * drawable_ptr->bpp;
  WRAPPER(gimpImgPtr) = gimp_buf = g_new(guchar, gimp_size);

  gimp_pixel_rgn_get_rect(&src_rgn, gimp_buf,
			  WRAPPER(gimp_x0), WRAPPER(gimp_y0),
			  GLACE_IMG(cols), GLACE_IMG(rows)); 

  imgArraysPtr->inImgSize = 
    SeperateChannels(gimp_buf, gimp_size,
		     gimp_drawable_color(drawable_ptr->id),
		     gimp_drawable_has_alpha(drawable_ptr->id),
		     cfgInfoPtr->colorMethod,
		     &imgArraysPtr->inImgPtr,   /* Greybuf (to process) */
		     &imgArraysPtr->inImgRPtr,     /* rbuf */
		     &imgArraysPtr->inImgGPtr,     /* gbuf */
		     &imgArraysPtr->inImgBPtr,     /* bbuf */
		     &imgArraysPtr->inImgCxPtr,     /* xbuf */
		     &imgArraysPtr->inImgCyPtr,     /* ybuf */
		     &imgArraysPtr->inImgCYMaxPtr); /* YMaxbuf */

  imgArraysPtr->refImgPtr = imgArraysPtr->inImgPtr;

  imgArraysPtr->refImgLBPtr = imgArraysPtr->refImgHBPtr = 
  imgArraysPtr->inImgLBPtr = imgArraysPtr->inImgHBPtr = 
    imgArraysPtr->inImgPtr;

  GLACE_CFG(chrome)= gimp_drawable_color(drawable_ptr->id) 
    ? GLACE_CC : GLACE_GG;

  /* This is forced config stuff. */  
  /*  if (GLACE_CFG(filtMethod) == GLACE_WINDOW) */
  GLACE_CFG(filtMethod) = GLACE_WINDOW;
  GLACE_IMG(ddHImgPtr) = GLACE_IMG(ddVImgPtr) = NULL;

  GLACE_CFG(doublep)=GLACE_FALSE;
}

static int
SeperateChannels (guchar *inbuf, const gint buflen,
		  const gboolean TypeRGB, const gboolean HasAlpha,
		  const Glace_ColorMethods color_method,
		  guchar **Graybuf, 
		  guchar **rbuf, guchar **gbuf, guchar **bbuf,
		  guchar **xbuf, guchar **ybuf,
		  guchar **Ymaxbuf)
{
  int c=0, ic=0, bpp;
  guchar *Grayfoo;
  if(!TypeRGB && !HasAlpha) {
    *Graybuf = inbuf;
    return buflen;
  }
  if(!TypeRGB && HasAlpha) {
    if((buflen % 2) != 0)
      g_error("GRAYA with odd size %d?",buflen);
    if(*Graybuf == NULL)
      *Graybuf = g_new(guchar,buflen/2);

    Grayfoo=*Graybuf;
    while(ic < buflen) {
      Grayfoo[c] = inbuf[ic++];
      ic++;
      c++;
    }
    return c;
  } /* if GRAYA */
  if(TypeRGB) {
	  guchar R,G,B;
	  guchar *xfoo, *yfoo, *Ymaxfoo;
	  guchar *rfoo, *bfoo, *gfoo;

	  bpp = HasAlpha ? 4 : 3;
	  if((buflen % bpp) != 0) {
		  g_error("RGB: %d Alpha: %d.  I think bpp should be %d, but that"
			  "doesn't make sense with size %d, as R = %d",
			  TypeRGB, HasAlpha, bpp, buflen, buflen % bpp);
	  }


	  if(*Graybuf == NULL)
		  *Graybuf = g_new(guchar,buflen/bpp);

	  Grayfoo=*Graybuf;

	  if(color_method == GLACE_COLOR_Yxy) {
		  if(*xbuf == NULL)
			  *xbuf = g_new(guchar,buflen/bpp);
		  if(*ybuf == NULL)
			  *ybuf = g_new(guchar,buflen/bpp);
		  if(*Ymaxbuf == NULL)
			  *Ymaxbuf = g_new(guchar,buflen/bpp);

		  xfoo=*xbuf;    
		  yfoo=*ybuf;
		  Ymaxfoo=*Ymaxbuf;

		  puts("Seperating into Yxy");
	  } else {
		  if(*rbuf == NULL)
			  *rbuf = g_new(guchar,buflen/bpp);
				  
		  if(*gbuf == NULL)
			  *gbuf = g_new(guchar,buflen/bpp);
		  
		  if(*bbuf == NULL)
			  *bbuf = g_new(guchar,buflen/bpp);
		  
		  rfoo=*rbuf;
		  gfoo=*gbuf;
		  bfoo=*bbuf;

		  puts("Seperating into RGB");
	  }
	  	  	  
	  while(ic < buflen) {
		  R = inbuf[ic++];
		  G = inbuf[ic++];
		  B = inbuf[ic++];
		  if(HasAlpha) ic++; 

		  if(color_method != GLACE_COLOR_Yxy) {
			  rfoo[c]=R;
			  gfoo[c]=G;
			  bfoo[c]=B;
		  }

		  if((R==0) && (G==0) && (B==0)) {
			  /* Black is a special case.  Choose values
                             that won't make divide by 0 errors. */
			  Grayfoo[c] = 0;

			  if(color_method == GLACE_COLOR_Yxy) {
				  xfoo[c] = 0;
				  yfoo[c] = 255;
				  Ymaxfoo[c] = 255;
			  }
		  } else {
			  gfloat X1, Y1, Z1;
			  gfloat x,y,z;
			  gfloat rYmax, gYmax, bYmax;

			  /* Calculate XYZ coordinates for original image. */
			  Y1 = Y_r * R + Y_g * G + Y_b * B;

			  /* This is the channel which is processed. */
			  Grayfoo[c] = Y1;

			  if(color_method == GLACE_COLOR_Yxy) {
				  X1 = X_r * R + X_g * G + X_b * B;
				  Z1 = Z_r * R + Z_g * G + Z_b * B;
				  
				  /* Find chromacity xy coordinates. */
				  /* Multiplied by 255 because we're not storing
				     them as floats from 0-1 but in an 8-bit
				     data type. */
				  xfoo[c] = x = 255.0 * X1 / ( X1 + Y1 + Z1 );
				  yfoo[c] = y = 255.0 * Y1 / ( X1 + Y1 + Z1 );
				  
				  z = 255.0 - x - y;
				  
				  /* This calculates the maximum displayable
				     luminosity for this color. */
				  rYmax = 255.0 / ((1/y) * (R_x * x + R_z * z) + R_y);
				  gYmax = 255.0 / ((1/y) * (G_x * x + G_z * z) + G_y);
				  bYmax = 255.0 / ((1/y) * (B_x * x + B_z * z) + B_y);
				  
				  Ymaxfoo[c] = MIN(MIN(rYmax,gYmax),bYmax);
			  } /* endif Yxy */
		  } /* endif rgb != 0 */
		  c++;
	  } /* whend */
	  return c;
  } /* endif rgb */
  g_error("Fell through SeperateChannels.");
  return -1;
} /* SeperateChannels() */

/**************************************************************/
/*
 * :FUNCTION: Glace_Process
 */
void 
GlaceGimp_Process (
               Glace_WData wData,
               Glace_CfgInfo *cfgInfoPtr,
	       Glace_TableInfo *tableInfoPtr,
               Glace_ImgArrays *imgArraysPtr
)
{
	guint k;

	Glace_WindChk (cfgInfoPtr, GLACE_IMG(cols), GLACE_IMG(rows));
	GLACE_ERROR_CHECK( GLACE_CFG(wData) );
	
	Glace_InitAccIm (cfgInfoPtr, imgArraysPtr);
	
	if (GLACE_CFG(outputMethod) == GLACE_NORMAL) {
		for (k  = GLACE_CFG(firstTerm); 
		     k <= (unsigned) GLACE_CFG(numTerms); 
		     k++) {
			Glace_SetForTerm(tableInfoPtr, k, GLACE_COS);
			Glace_TermlyReport( cfgInfoPtr, tableInfoPtr);
			
			Glace_SetTables (cfgInfoPtr, tableInfoPtr);
			Glace_GenGen ( cfgInfoPtr, tableInfoPtr, imgArraysPtr);
			GLACE_ERROR_CHECK( GLACE_CFG(wData) );
			
			Glace_SetForTerm(tableInfoPtr, k, GLACE_SIN);
			
			Glace_SetTables (cfgInfoPtr, tableInfoPtr);
			Glace_GenGen ( cfgInfoPtr, tableInfoPtr, imgArraysPtr);
			GLACE_ERROR_CHECK( GLACE_CFG(wData) );
			
			Glace_WMessage( GLACE_CFG(wData), ";\n" );
			
			gimp_progress_update(0.10 + 0.90 * k/GLACE_CFG(numTerms));
		} 
	} else {
		Glace_DiagGen (cfgInfoPtr, tableInfoPtr, imgArraysPtr);
	}
	GLACE_ERROR_CHECK( GLACE_CFG(wData) );
	Glace_Output (wData, cfgInfoPtr, imgArraysPtr );
}

/* :FUNCTION: FIXME GlacePnm_ScanListfiles
 */

/*static void
GlaceGimp_ScanListfiles (Glace_CfgInfo *cfgInfoPtr)
{
  g_warning("GlaceGimp_ScanListFiles in gimp_ace.c not yet implemented.\n");
}*/

/*   GlaceGimp_CloseFile (stdout); */
