/* $Id$ */
/* :PREAMBLE: Glace.h
 *
 * Main header file for GLACE programs:
 *    NOTE: This is designed to load your program-specific headers for you
 *
 *     For PBMPLUS and Tk/Tcl-based and GIMP versions
 * #define GLACE_PNM or GLACE_TK or GLACE_GIMP before inclusion
 * Automatically includes pgm.h or tk.h, or gimp headers as appropriate.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.  See the file COPYING for details.
 *
 */

/* :DEFS AND SUCH:
 */

#ifdef _PGM_H_
#error DO NOT LOAD PBMPLUS (OR DERIVATIVE) FIRST
#endif

#ifndef _GLACE_H_
#define _GLACE_H_


#ifndef BUFSIZ
#include <stdio.h>
#endif



#ifdef GLACE_PNM
/* pbmpluss.h deals with WATCOMC */
#  include "pbmpluss.h"
#  include "pnm.h"
#endif

#ifdef __WATCOMC__
#  define __WIN32__
/*#  define  DllEntryPoint LibMain*/
#  include "glace.wcp"
#endif

#ifdef GLACE_TK
#  include "tk.h"
#endif

#ifdef GLACE_GIMP
#  include <glib.h>
#  include <libgimp/gimp.h>
#endif

/* This isn't really the right place for this, but... */
#define BYTE_SPLIT(x,hb,lb) \
lb = (Glace_Gray) (((unsigned int) (x+0.5)) & 0377);\
hb = (Glace_Gray) ((((unsigned int) (x+0.5))>>8))
/*lb = ((Glace_Gray) x);*/
/*lb = ((unsigned int) x) - (hb<<8);*/


/*
 * There is also an internal header file glaceInt.h
 */

#ifdef _OVAR_
#define GLACE_OVAR(A) A __attribute__ ((unused))
#else
#define GLACE_OVAR(A) A
#endif

/**********************************************************************/

/* :*** Simple Things ***:
 *
 * :+++++++: #defines
 */
#define GLACE_PI ((double) 3.141592653589793)

#define GLACE_TRUE 1
#define GLACE_FALSE 0
#define GLACE_BOOL int

/* avoiding 0, 1, etc. */
#define GLACE_ERROR 99
#define GLACE_OK 0

#define GLACE_STRMAX 100

#define GLACE_MIDGRAY ((float) 127.5)
#define GLACE_MAXMAXGRAY 255

/*--------------------------------------------------------------------*/
/*
 * :+++++++: Grays and colours typedefs
 */


typedef signed long Glace_BigGray;
typedef signed short Glace_MidGray;
#ifndef _PGM_H_
typedef unsigned char Glace_Gray;



#undef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#undef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#undef abs
#define abs(a) ((a) >= 0 ? (a) : -(a))
#undef odd
#define odd(n) ((n) & 1)

#else

#define Glace_Gray gray

#endif



typedef struct
    {
    Glace_Gray r, g, b;
    } Glace_Pixel;
#define GLACE_GETR(p) ((p).r)
#define GLACE_GETG(p) ((p).g)
#define GLACE_GETB(p) ((p).b)

/************* added definitions *****************/
#define GLACE_PUTR(p,red) ((p).r = (red))
#define GLACE_PUTG(p,grn) ((p).g = (grn))
#define GLACE_PUTB(p,blu) ((p).b = (blu))
/**************************************************/

#define GLACE_ASSIGN(p,red,grn,blu) do { (p).r = (red); (p).g = (grn); (p).b = (blu); } while ( 0 )



/**********************************************************************/

/* :*** Structures ***:
 *
 * :+++++++: Glace_WData.       Wrapper data structure (dummy typedef)
 */

typedef char *Glace_WData;
/*typedef char Glace_WErrorInfo;*/


/*--------------------------------------------------------------------*/
/*
 * :+++++++: Glace_ImgArrays.   Image arrays structure
 *
 * This is a structure that holds the pointers and defining parameters of
 * the image arrays: input, reference, filtering, accumulation, and
 * temporary.  The output is done row-by-row.
 */

/*
typedef enum {
  GLACE_TMP_IMG_BIG,  GLACE_TMP_IMG_MID
} Glace_TmpImgGraySize;
*/

typedef char *Glace_ImageHandle;

/*
 * Structure definition
 */

typedef struct Glace_ImgArrays {
  Glace_ImageHandle   inputImageHandle, refImageHandle,
    ddHImageHandle, ddVImageHandle, outputImageHandle;
  
  Glace_Gray *ddHImgPtr,
    *ddVImgPtr;

  long inImgSize; /* Allocated size for input image,
		   taking into account * pixel planes passed on (1 to 5) */
  Glace_Gray *inImgPtr;
  /*long inImgBSize;*/ /*  for input gray bytes. 0: not alloc or use inImgPtr
		    *   0: not alloc or use inImgPtr
		    *  *2 if LB+HB (ie doublep)
		    */
  Glace_Gray *inImgHBPtr;
  Glace_Gray *inImgLBPtr;

  /* Some color methods take RGB, others take Yxy. */
  /* (The C is to remind you that x,y are coordinates in color space,
     and not cartesian pixel coordinates.) */

  Glace_Gray *inImgCxPtr;
  Glace_Gray *inImgCyPtr;
  Glace_Gray *inImgCYMaxPtr;

  Glace_Gray *inImgRPtr;
  Glace_Gray *inImgGPtr;
  Glace_Gray *inImgBPtr;

  int pixelSize;
  int pixelBytePad;  /* Set by Glace_WPutImgRowStart or before */

  long refImgSize;
  Glace_Gray *refImgPtr;
  /*  long refImgBSize;*/
  Glace_Gray *refImgHBPtr;
  Glace_Gray *refImgLBPtr;


  long accImgSize;
  Glace_MidGray *accImgPtr;
  Glace_MidGray *outAccImgPtr; /* a dummy pointer that refers to the 
				   accumulator array to be output.  This
				   can be other than the true accumulator
				   if a diagnostic image is being
				   generated. */
  int cols;
  int rows;
  long putImgSize; /* The wrapper can use this how it likes, eg size
		    of a single row or of a whole output image. */
  Glace_Gray *putImgRowPtr;

  long tmpImgSize;
  /* basically internal; in size_t. If zero, then tmp is unallocated. */

  int tmpImgRPad, tmpImgCPad;  /*added to rows and cols*/
  /*Glace_TmpImgGraySize tmpImgGraySize;*/
  size_t tmpImgGraySize;

  char *tmpImgPtr;
  /* Glace_BigGray *bgTmpImgPtr;
  Glace_MidGray *mgTmpImgPtr;*/

  Glace_WData wData;
} Glace_ImgArrays;



/*--------------------------------------------------------------------*/
/*
 * :+++++++: Glace_CfgInfo.     Configuration information
 */

#define GLACE_CFG_DEFAULT_NUM_TERMS 240
#define GLACE_CFG_DEFAULT_A_WITH_F GLACE_NONE
/*#define GLACE_CFG_DEFAULT_A_WITH_F GLACE_LOCALMEAN*/
#define GLACE_TOL_DOUBLEP 0.00001
#define GLACE_TOL_SINGLEP 0.0025


/* Default value is indicated by zero throughout */
typedef enum {
    GLACE_AUTO = 0, GLACE_MANUAL
} Glace_Modes;

typedef enum {
  GLACE_MISSING = 0, GLACE_LISTFILE, GLACE_COMMANDLINE
} Glace_DimSrcs;

typedef enum {
  GLACE_NORMAL = 0, GLACE_COSRAW, GLACE_COSFILT, GLACE_SINRAW, GLACE_SINFILT
} Glace_OutputMethods;

typedef enum {
  GLACE_STANDARD = 0, GLACE_SERIES, GLACE_FACTOR
} Glace_HeTypes;

typedef enum {
  GLACE_CC = 0, GLACE_CG, GLACE_GG
} Glace_ChromeTypes;

typedef enum {
  GLACE_WINDOW = 0, GLACE_DIRDIFF
} Glace_FiltMethods;

typedef enum {
  GLACE_INPUT = 0, GLACE_SEPARATE
} Glace_RefImageModes;

typedef enum {
  GLACE_NONE = 0, GLACE_ZEROINPUT, GLACE_LOCALMEAN
} Glace_AddbackTypes;

typedef enum {
  GLACE_PNM_VER = 0, GLACE_TK_VER, GLACE_GIMP_VER
} Glace_WrapTypes;

typedef enum
{
  GLACE_COLOR_Yxy = 0, GLACE_COLOR_LUMIN
} Glace_ColorMethods;

typedef struct Glace_CfgInfo
{
  FILE *wListfile, *dListfile, *sListfile;

  Glace_OutputMethods outputMethod;
  int numTerms, firstTerm, termsSerialised;
  unsigned int activeTerms;
  GLACE_BOOL verbose;
  GLACE_BOOL hammingCwind;
  GLACE_BOOL gaussCwind;
  GLACE_BOOL addbackCwind;
  GLACE_BOOL doClip;

  GLACE_BOOL doublep;
  GLACE_BOOL doubleout;
  Glace_ChromeTypes chrome;
  
  
  Glace_HeTypes heType;

  long currentSeriesAllocation;
  double *heseriesSeries, *cwindSeries;
  double *addbackSeries;
  int *windCWSeries, *windCHSeries, *windSWSeries, *windSHSeries;

  Glace_FiltMethods filtMethod;
  int windBaseW, windBaseH;
  float addbackFactor;
  /* if dd filtering is used, then we remap arrays */
#define GLACE_CFG_COSDIM windCWSeries
#define GLACE_CFG_COSBV  windCHSeries
#define GLACE_CFG_SINDIM windSWSeries
#define GLACE_CFG_SINBV  windSHSeries

 
  Glace_DimSrcs dimensionSrc;
  
  Glace_Modes prescaleMode;
  Glace_Modes inoffsetMode;
  Glace_Modes addbackMode;
  Glace_AddbackTypes addbackType;
  float passthruFactor;
  float prescaleVal;
  float heFactor;
  float inoffsetVal;
  float gaussCwindWidth;
  float coeffTol;

  Glace_ColorMethods colorMethod;

  Glace_RefImageModes refimageMode;

  GLACE_BOOL genCFunc;
  float cFuncMin, cFuncMax;
  int cFuncPoints;

  Glace_WData wData;
  Glace_WrapTypes wrapType;
  GLACE_BOOL plainGlace;
} Glace_CfgInfo;



/*--------------------------------------------------------------------*/
/*
 * :+++++++: Glace_TableInfo.   Term-wise and table information
 */

typedef enum {
  GLACE_SIN, GLACE_COS
} Glace_TermTypes;

typedef double Glace_FpSeries;

typedef struct Glace_TableInfo
{
  int filtShifts, notFiltShifts;
  float filtFactor, notFiltFactor;
  Glace_TermTypes termType; /* which one is being filtered */
   unsigned int termNum;

  /*  long *notFiltTable, *filtTable;*/
  long *seriesTable;
   int   accShift;

 Glace_FpSeries *seriesAH, *seriesAL, *seriesBH, *seriesBL;
  
  float abFactor, abShifts;

  double waveFactor;

  Glace_WData wData;
} Glace_TableInfo;



/*--------------------------------------------------------------------*/
/*
 * :+++++++: Glace_ClientData.  Umbrella clientdata
 */


typedef struct Glace_ClientData
{
  Glace_WData wData;
  Glace_CfgInfo *cfgInfoPtr;
  Glace_TableInfo *tableInfoPtr;
  Glace_ImgArrays *imgArraysPtr;
} Glace_ClientData;



#if defined(GLACE_IMG)
#undef GLACE_IMG
#endif
#define GLACE_IMG(A) (imgArraysPtr->A)



#if defined(GLACE_TRM)
#undef GLACE_TRM
#endif
#define GLACE_TRM(A) (tableInfoPtr->A)



#if defined(GLACE_CFG)
#undef GLACE_CFG
#endif
#define GLACE_CFG(A) (cfgInfoPtr->A)



#if defined(GLACE_CDATA)
#undef GLACE_CDATA
#endif
#define GLACE_CDATA(A) (((Glace_ClientData *) clientData)->A)




/**********************************************************************/

/* :*** Procedures ***:
 *
 * :+++++++: Dummy tests
 *
 * These were originally used as dummy operations (a kind of NOP)
 * that used a variable or structure pointer such that the error
 * should never occur.  This keeps the compiler happy.  Pointers
 * to structures are often unused in some versions, or if they are
 * included for future expansion.
 *
 * Note that these tests could be used in anger, but that the message
 * would be non-specific.
 */

/*

#define GLACE_NULL_TEST(wData,ptr) if (ptr==NULL) \
               Glace_WError( wData,"Pointer " \
	       "null test failed");
#define GLACE_TEST(wData,T) if (T) Glace_WError( wData,\
						      "Test failure");
*/


#define GLACE_ERROR_EXIT(wData) if (Glace_WIsError(wData)) \
                                       exit(1);
#define GLACE_ERROR_CHECK(wData) if (Glace_WIsError(wData)) \
                                       return;
#define GLACE_ERROR_VALUE(wData) if (Glace_WIsError(wData)) \
                                       return Glace_WErrorValue( wData );


/*--------------------------------------------------------------------*/
/*
 * :+++++++: Cfg.       Configuration information
 */

/*
 * CONFIG
 *
 */

void
Glace_CfgInit (
   Glace_CfgInfo *cfgInfoPtr);

void
Glace_CfgAllocSeriesVectors (
   Glace_CfgInfo *cfgInfoPtr
   );

void
Glace_CfgBeginToHeseries (
   Glace_CfgInfo *cfgInfoPtr
   );

void
Glace_CfgHeseriesToAddback (
   Glace_CfgInfo *cfgInfoPtr
   );

void
Glace_CfgAddbackToEnd (
   Glace_CfgInfo *cfgInfoPtr
   );

void
Glace_CfgFreeSeriesVectors (
   Glace_CfgInfo *cfgInfoPtr
   );


/*--------------------------------------------------------------------*/
/*
 * :+++++++: W.         Wrapper-specifics
 *
 * The following are implemented differently for each wrapper.
 * PGM   system:  glaceP.c and 
 * TK/TCL system: glaceT.c
 */




/**********************************************************************/
/*
 * WRAPPER:
 *
 */


Glace_WData
Glace_WDataAlloc();

void
Glace_WInit(
	    Glace_WData wData
	    );

Glace_WrapTypes
Glace_WWrapTell();

void
Glace_WMessage(
	     Glace_WData wData, char* messageStr );

int /* Sets error and returns error flag (but may exit) */
Glace_WError(
		  Glace_WData wData,
	     char* argErrStr
	     );

int /* Returns error flag */
Glace_WErrorValue(
		  Glace_WData wData
	     );

int /* Returns boolean */
Glace_WIsError(
		  Glace_WData wData
	     );



/*----------*/
void
Glace_WPutImgStart(
		   Glace_WData wData,
	      Glace_CfgInfo *cfgInfoPtr,
		   Glace_ImgArrays *imgArraysPtr
		   );
void
Glace_WPutImgRowStart(
		  Glace_WData wData,
	      Glace_CfgInfo *cfgInfoPtr,
		Glace_ImgArrays *imgArraysPtr,
		int row
		);
void
Glace_WPutImgRowFinish(
		  Glace_WData wData,
	      Glace_CfgInfo *cfgInfoPtr,
		Glace_ImgArrays *imgArraysPtr,
		int row
		);
void
Glace_WPutImgFinish(
		  Glace_WData wData,
	      Glace_CfgInfo *cfgInfoPtr,
		Glace_ImgArrays *imgArraysPtr
		);
int
Glace_WKeyMatch(
		char* str,
		char* keyWord,
		int minChars
		);

void
Glace_WUsage(
	     Glace_WData wData,
    char* usage
);

void
Glace_WOpenImage(
		  Glace_WData wData,
	      Glace_CfgInfo *cfgInfoPtr,
		 char *name,
		 Glace_ImageHandle *handlePtr
);


/*--------------------------------------------------------------------*/
/*
 * :+++++++: Term-wise and table info
 */


/* TABLE INFO
 *
 * This supplies the lookup tables and other related information that is
 * specific to the current term being processed.
 */


void
Glace_SetForTerm (
		 Glace_TableInfo *tableInfoPtr,
		 int k,
		 Glace_TermTypes type /* which one is being filtered */
		 );

void
Glace_TermlyReport (
	       Glace_CfgInfo *cfgInfoPtr,
		 Glace_TableInfo *tableInfoPtr
);

void
Glace_AllocTables (
		 Glace_CfgInfo *cfgInfoPtr,
		 Glace_TableInfo *tableInfoPtr
		 );

void
Glace_FreeTables (
		 Glace_CfgInfo *cfgInfoPtr,
		 Glace_TableInfo *tableInfoPtr
		 );

void
Glace_SetTables (
		 Glace_CfgInfo *cfgInfoPtr,
		 Glace_TableInfo *tableInfoPtr
		 );

void
Glace_FillTableForFilt (
		 Glace_CfgInfo *cfgInfoPtr,
		 Glace_TableInfo *tableInfoPtr,
		 float scale
		 );

void
Glace_FillTableForNotFilt (
		 Glace_CfgInfo *cfgInfoPtr,
		 Glace_TableInfo *tableInfoPtr
		 );

/*--------------------------------------------------------------------*/
/*
 * :+++++++: CFunc.     Cumulation function
 */

int
Glace_CFuncGen (
		 Glace_CfgInfo *cfgInfoPtr
		 );
float
Glace_CFuncPoint (
		Glace_CfgInfo *cfgInfoPtr,
		float x
		 );
float
Glace_CFuncIdealPoint (
		Glace_CfgInfo *cfgInfoPtr,
		float x
		 );



/*--------------------------------------------------------------------*/
/*
 * :+++++++: Allocation and reallocation
 */

/*
 * Etc.
 */
/**************************************************************/
char *
Glace_CallocReallocFree(
			void *p,
			long *nObjPtr,
			long newNObj,
			size_t size,
			float minBound
);

void
Glace_FreeImgArrays (
		 Glace_CfgInfo *cfgInfoPtr,
	     Glace_ImgArrays *imgArraysPtr
		 );

void
Glace_SetTmpImg (
		 Glace_ImgArrays *imgArraysPtr,
		 size_t graySize,
		 int rowPad,
		 int colPad
		 );

void
Glace_DefaultTmpImg (
		 Glace_CfgInfo *cfgInfoPtr,
	     Glace_ImgArrays *imgArraysPtr
		 );
void
Glace_AllocImgArrays (
		 Glace_CfgInfo *cfgInfoPtr,
	     Glace_ImgArrays *imgArraysPtr
		 );

void
Glace_AllocInputImgArrays (
		 Glace_CfgInfo *cfgInfoPtr,
	     Glace_ImgArrays *imgArraysPtr
		 );

void
Glace_FreeInputImgArrays (
	     Glace_ImgArrays *imgArraysPtr
		 );


Glace_ClientData *
Glace_AllocClientData ();

void
Glace_FreeClientData (
		       Glace_ClientData *clientData
		       );

/*--------------------------------------------------------------------*/
/*
 * :+++++++: Process procedures
 */

void 
Glace_Process (
	       Glace_WData wData,
	      Glace_CfgInfo *cfgInfoPtr,
	      Glace_TableInfo *tableInfoPtr,
	    Glace_ImgArrays *imgArraysPtr
	      );

/*
void
GlaceArgError (
   const char *message,
   char *argv[],
   int numArgs,
   int problemArg
   );
   */
int
Glace_ParseArgs (
   Glace_CfgInfo *cfgInfoPtr,
		Glace_ImgArrays *imgArraysPtr,
   int argc,
   char *argv[]);


void
Glace_WindChk( 
	     Glace_CfgInfo *cfgInfoPtr,
	     int rows,
	     int cols
	     );

int
Glace_Output (
	       Glace_WData wData,
	  Glace_CfgInfo *cfgInfoPtr,
	      Glace_ImgArrays *imgArraysPtr
	  );
void 
Glace_InitAccIm (
	       Glace_CfgInfo *cfgInfoPtr,
	     Glace_ImgArrays *imgArraysPtr);

void 
Glace_GenGen (
	       Glace_CfgInfo *cfgInfoPtr,
		 Glace_TableInfo *tableInfoPtr,
	     Glace_ImgArrays *imgArraysPtr);

void
Glace_DiagGen (
	       Glace_CfgInfo *cfgInfoPtr,
	      Glace_TableInfo *tableInfoPtr,
	      Glace_ImgArrays *imgArraysPtr
	      );
void
Glace_DiagOutput (
 	       Glace_WData wData,
	      Glace_CfgInfo *cfgInfoPtr,
	      Glace_ImgArrays *imgArraysPtr
	      );



void 
Glace_CallFilt (
	       Glace_CfgInfo *cfgInfoPtr,
		 Glace_TableInfo *tableInfoPtr,
	     Glace_ImgArrays *imgArraysPtr);


/*--------------------------------------------------------------------*/
/*
 * :+++++++: Filt.      Filtering
 */

/**********************************************************************/
/*
 * FILT
 *
 * Image array filtering
 * Implemented in glaceFilt.c
 */

void 
Glace_FiltWind(
   Glace_CfgInfo *cfgInfoPtr,
	       Glace_Gray * inImgHBPtr,
	       Glace_Gray * inImgLBPtr, Glace_BigGray * outImgPtr,
	       /*signed long *lTable,*/
	      int cols, int rows,
	      Glace_TableInfo *tableInfoPtr,
	      int width, int height
	      );

void 
Glace_FiltDD(
   Glace_CfgInfo *cfgInfoPtr,
	       Glace_Gray * inImgPtr, Glace_MidGray * outImgPtr,
	       /*signed long *lTable,*/
	      int cols, int rows,
	      Glace_TableInfo *tableInfoPtr,
	    Glace_Gray *hBiasArray, Glace_Gray *vBiasArray,
	    int numPasses
	    );

/*--------------------------------------------------------------------*/
/*
 * :+++++++: Heseries.  Generating signed power-law series
 */

/**********************************************************************/
/*
 * HESERIES
 *
 * HE Fourier series generation
 * Implemented in glaceHeseries.c
 */

/* Internal structure.
 *
 * A dummy definition is used for this structure.
 * The caller must preallocate it.
 */

/* An unreasonable value for the series, used to flag errors */
#define GLACE_HESERIES_ERRORFLAG (-1000000.0)

#define GLACE_HESERIES_RESERVE 100
typedef struct Glace_HeseriesInfo
{
char dummy[GLACE_HESERIES_RESERVE];
} Glace_HeseriesInfo;

void
Glace_HeseriesInit (Glace_HeseriesInfo *heseriesPtr, float heFactor);
double
Glace_HeseriesVal (Glace_HeseriesInfo *heseriesPtr, int k);



/**********************************************************************/

/* :*** Wrapper specific data ***:
 */

#  if defined(WRAPPER)
#    undef WRAPPER
#  endif

/*--------------------------------------------------------------------*/
/*
 * :+++++++: GLACE_PNM
 */

#ifdef GLACE_PNM
#
typedef struct GlacePnmData
{
   Glace_Gray **ddHImgPtrPtr, **ddVImgPtrPtr;
  Glace_Gray **inImgPtrPtr;
  Glace_Gray **refImgPtrPtr;

  int errorValue;
} GlacePnmData;
#
#
#  define WRAPPER(A) (((GlacePnmData *) wData)->A)
#endif

/*--------------------------------------------------------------------*/
/*
 * :+++++++: GLACE_GIMP
 */

#ifdef GLACE_GIMP
#
typedef struct GlaceGimpData
{
  /*   Glace_Gray **ddHImgPtrPtr, **ddVImgPtrPtr; */
  /*  Glace_Gray **inImgPtrPtr;   / No, I don't have a clue as to how     */
  /*  Glace_Gray **refImgPtrPtr;  / much of this is used, or where. [kmt] */
  /* [jas]: These are used in PNM to remember pointers to arrays */

  Glace_Gray *dest_row;
  
  /* Is this a good place to store these things? */
  /* [jas]: yes, this is exactly the place! */
  GPixelRgn dest_rgn_ptr;
  GDrawable *drawable_ptr;
  gint32 gimp_x0, gimp_y0;

  guchar *gimpImgPtr;

  int errorValue;
} GlaceGimpData;
#
#
#  define WRAPPER(A) (((GlaceGimpData *) wData)->A)
#endif

/*--------------------------------------------------------------------*/
/*
 * :+++++++: GLACE_TK
 */


#ifdef GLACE_TK
#
typedef struct GlaceTkData
{
  Tk_PhotoImageBlock outputImageBlock;

  Glace_Gray *outImgPtr;

  Tcl_Interp *interp;
  int errorValue;
} GlaceTkData;
#
#
#  define WRAPPER(A) (((GlaceTkData *) wData)->A)
#  define INTERP(A) ((GlaceTkData *) A)->interp
#endif


#endif
