/* :PREAMBLE: GlaceG.c
 *
 * Wrapper-specific Code:
 *           GIMP plug-in version
 *
 * Based on glaceT.c and glaceP.c by J.Alex Stark
 * g_ified by Kevin M. Turner
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.  See the file COPYING for details.
 *
 */

/* :HEADERS:
 */
#ifndef GLACE_GIMP
#  define GLACE_GIMP
#endif

#include "glaceInt.h"

#include <ctype.h> /* for tolower in keymatch */

/* Other things we're implementing
 * Included first, because glace.h needs to know about it.
 */

/* :*** Internal Functions ***:
 * GlaceWMalloc,Realloc,Calloc, and Free
 * have been implemented as defines in an #ifdef block in glaceInt.h
 * Ugly, but true.
 */

/**************************************************************/

/* :*** Errors and Messages ***:

 * :FUNCTION: Glace_WError
 */
int				/* Sets error and returns error flag (but may exit) */
Glace_WError (
	       Glace_WData wData,
	       char *argErrStr
)
{
  if (wData == NULL)
    g_warning ("Fatal error (error rountine called"
	     " with invalid package data structure.");
  WRAPPER (errorValue) = GLACE_ERROR;

  g_warning (argErrStr);

  return GLACE_ERROR;
}

/*
 * :FUNCTION: Glace_WErrorValue
 */
int				/* Returns error flag */
Glace_WErrorValue (
		    Glace_WData wData
)
{
  return WRAPPER (errorValue);
}



/*
 * :FUNCTION: Glace_WIsError
 */
int				/* Returns boolean */
Glace_WIsError (
		 Glace_WData wData
)
{
  return (WRAPPER (errorValue) == GLACE_ERROR);
}



/*
 * :FUNCTION: Glace_WMessage
 */
void
Glace_WMessage (
		 GLACE_OVAR(Glace_WData wData),
		 char *messageStr)
{
  fprintf (stderr, "%s", messageStr);
}


/**************************************************************/
/* :*** Wrapper-specific Setup ***:

 * :FUNCTION: Glace_WInit
 */
void
Glace_WInit (
	     GLACE_OVAR(Glace_WData wData))
{
#ifdef __EMX__
  _fsetmode (stdin, "b");
  _fsetmode (stdout, "b");
#endif
}



/**************************************************************/
/*
 * :FUNCTION: Glace_WWrapTell
 */
Glace_WrapTypes
Glace_WWrapTell()
{
  return GLACE_GIMP_VER;
}

/**************************************************************/
/*
 * :FUNCTION: Glace_WDataAlloc
 */
Glace_WData
Glace_WDataAlloc ()
{
  return (Glace_WData) GlaceWMalloc (sizeof (GlaceGimpData));
}



/* :*** Argument Parsing and Warning ***:

 * :FUNCTION: Glace_WKeyMatch
 *
 * NB This is an independent rewrite of PBMPLUS routine,
 * just to be sure about copyright issues. [Alex]
 *
 * (Probably not necessary for GIMP version.  Feel free to substitute
 * pm_keymatch back in...) [kmt]
 */
int
Glace_WKeyMatch(
		char* str,
		char* keyword,
		int minchars
		)
{
  int i, slen;

  slen = strlen(str);

  if (slen < minchars)
    return 0;

  else if (slen > (int) strlen(keyword))
    return 0;

  else
    for (i=0; i<slen; i++)
      if (tolower(str[i]) != tolower(keyword[i]))
	return 0;
 
  return 1;
}

/**************************************************************/
/*
 * :FUNCTION: Glace_WUsage
 */
void
Glace_WUsage(
	     Glace_WData wData,
    char* usage
)
{
  Glace_WMessage(wData, usage);
  WRAPPER(errorValue) = GLACE_ERROR;
}

/* :*** Image Input and Output ***:

 * :FUNCTION: FIXME Glace_WOpenImage 
 */
void
Glace_WOpenImage(GLACE_OVAR(Glace_WData wData),
		 GLACE_OVAR(Glace_CfgInfo *cfgInfoPtr),
		 char *name,
		 Glace_ImageHandle *handlePtr
)
{
  /* g_print(" *** Glace_WOpenImage doesn't do jack! ***\n"); */
}

/**************************************************************/

/*
 * :FUNCTION: FIXME Glace_WPutImgStart
 * Here we initalize a pixel region to write to, 
 * and set the output pointer to the temp space.
 */
void
Glace_WPutImgStart(
		  GLACE_OVAR(Glace_WData wData),
	      GLACE_OVAR(Glace_CfgInfo *cfgInfoPtr),
		  Glace_ImgArrays *imgArraysPtr
		  )
{
  /* gimp_pixel_rgn_init (GPixelRgn *pr,
		     GDrawable *drawable,
		     int       x,
		     int       y,
		     int       width,
		     int       height,
		     int       dirty,
		     int       shadow) */

	/*  puts(" *** I am Glace_WPutImgStart, here me roar! ***"); */

  gimp_pixel_rgn_init(&WRAPPER(dest_rgn_ptr),
		      WRAPPER(drawable_ptr),
		      WRAPPER(gimp_x0), WRAPPER(gimp_y0),
		      GLACE_IMG(cols), GLACE_IMG(rows),
		      TRUE, TRUE);

  /* GLACE_IMG(putImgRowPtr)=(Glace_Gray *)GLACE_IMG(tmpImgPtr); */

  GLACE_IMG(putImgRowPtr) = WRAPPER(gimpImgPtr);

}

/**************************************************************/
/*
 * :FUNCTION: Glace_WPutImgRowStart
 * (we don't use it)
 */
void
Glace_WPutImgRowStart(
		  GLACE_OVAR(Glace_WData wData),
	      GLACE_OVAR(Glace_CfgInfo *cfgInfoPtr),
		GLACE_OVAR(Glace_ImgArrays *imgArraysPtr),
		GLACE_OVAR(int row)
		)
{

}

/**************************************************************/


 /*
 * :FUNCTION: FIXME Glace_WPutImgRowFinish
 * It *should* take the row of data that has been produced and
 * give it to the Gimp.  I think it does this, but the output
 * sure doesn't look right.
 */
void
Glace_WPutImgRowFinish (
			 GLACE_OVAR(Glace_WData wData),
	      Glace_CfgInfo *cfgInfoPtr,
			 Glace_ImgArrays * imgArraysPtr,
			 GLACE_OVAR(int row)
)
{
  /* If we were outputting by row, we'd do this: */
  /* gimp_pixel_rgn_set_row (GPixelRgn *pr,
                             guchar   *buf,
                             int       x,
			     int       y,
			     int       width) */

  /* gimp_pixel_rgn_set_row(&WRAPPER(dest_rgn_ptr),
	 		 GLACE_IMG(putImgRowPtr),
			 WRAPPER(gimp_x0),
			 WRAPPER(gimp_y0) + row,
			 GLACE_IMG(cols));*/

  /* But we're writing it all to a temp buffer instead... */
  /* Faster, and the temp buffer is already allocated. */

  GLACE_IMG(putImgRowPtr) += GLACE_IMG(cols) * WRAPPER(drawable_ptr)->bpp; 

}


/**************************************************************/


/*
 * :FUNCTION: Glace_WPutImgFinish
 */
void
Glace_WPutImgFinish (
		      GLACE_OVAR(Glace_WData wData),
	      GLACE_OVAR(Glace_CfgInfo *cfgInfoPtr),
		      GLACE_OVAR(Glace_ImgArrays * imgArraysPtr)
)
{
  /* g_print("*** Glace_WPutImgFinish coming through. ***\n"); */


  /* We could do something like this to write the entire image
     at one time instead of row by row. */

  gimp_pixel_rgn_set_rect  (&WRAPPER(dest_rgn_ptr),
			    WRAPPER(gimpImgPtr),
			    WRAPPER(gimp_x0), WRAPPER(gimp_y0),
			    GLACE_IMG(cols), GLACE_IMG(rows));

  gimp_drawable_flush (WRAPPER(drawable_ptr));
  gimp_drawable_merge_shadow (WRAPPER(drawable_ptr)->id, TRUE);
  gimp_drawable_update (WRAPPER(drawable_ptr)->id, 
			WRAPPER(gimp_x0), WRAPPER(gimp_y0),
			GLACE_IMG(cols), GLACE_IMG(rows));
  gimp_drawable_detach (WRAPPER(drawable_ptr));
  gimp_displays_flush();
}

/**************************************************************/

