/*  
 * TWAIN Plug-in
 * Copyright (C) 1999 Craig Setera
 * Craig Setera, setera@infonet.isl.net
 * 03/31/1999
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
 *
 *
 * Based on (at least) the following plug-ins:
 * Screenshot
 * GIF
 * Randomize
 *
 * Any suggestions, bug-reports or patches are welcome.
 * 
 * This plug-in interfaces to the TWAIN support library in order
 * to capture images from TWAIN devices directly into GIMP images.
 * The plug-in is capable of acquiring the following type of
 * images:
 * - B/W (1 bit images translated to grayscale B/W)
 * - Grayscale up to 16 bits per pixel
 * - RGB up to 16 bits per sample (24, 30, 36, etc.)
 * - Paletted images (both Gray and RGB)
 *
 * Prerequisites:
 *  This plug-in will not compile on anything other than a Win32
 *  platform.  Although the TWAIN documentation implies that there
 *  is TWAIN support available on Macintosh, I neither have a 
 *  Macintosh nor the interest in porting this.  If anyone else
 *  has an interest, consult www.twain.org for more information on
 *  interfacing to TWAIN.
 *
 * Known problems:
 * - Multiple image transfers will hang the plug-in.  The current
 *   configuration compiles with a maximum of single image transfers.
 */

/* 
 * Revision history
 *  (02/07/99)  v0.1   First working version (internal)
 *  (02/09/99)  v0.2   First release to anyone other than myself
 *  (02/15/99)  v0.3   Added image dump and read support for debugging
 *  (03/31/99)  v0.5   Added support for multi-byte samples and paletted 
 *                     images.
 */
#include <glib.h>		/* Needed when compiling with gcc */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "twain.h"
#include "tw_func.h"
#include "tw_util.h"

#ifdef _DEBUG
#include "tw_dump.h"
#endif /* _DEBUG */

/*
 * Plug-in Definitions
 */
#define PLUG_IN_NAME        "twain-acquire"
#define PLUG_IN_DESCRIPTION "Capture an image from a TWAIN datasource"
#define PLUG_IN_HELP        "This plug-in will capture an image from a TWAIN datasource"
#define PLUG_IN_AUTHOR      "Craig Setera (setera@infonet.isl.net)"
#define PLUG_IN_COPYRIGHT   "Craig Setera"
#define PLUG_IN_VERSION     "v0.5 (03/31/1999)"
#define PLUG_IN_MENU_PATH   "<Toolbox>/File/Acquire/TWAIN..."

#ifdef _DEBUG
#define PLUG_IN_D_NAME        "twain-acquire-dump"
#define PLUG_IN_D_MENU_PATH   "<Toolbox>/File/Acquire/TWAIN (Dump)..."

#define PLUG_IN_R_NAME        "twain-acquire-read"
#define PLUG_IN_R_MENU_PATH   "<Toolbox>/File/Acquire/TWAIN (Read)..."
#endif /* _DEBUG */

/*
 * Application definitions
 */
#define APP_NAME "TWAIN"
#define MAX_IMAGES 1
#define SHOW_WINDOW 0
#define WM_TRANSFER_IMAGE (WM_USER + 100)

/*
 * Definition of the run states 
 */
#define RUN_STANDARD 0
#define RUN_DUMP 1
#define RUN_READDUMP 2

/* Global variables */
pTW_SESSION twSession = NULL;

static HWND        hwnd = NULL;
static HINSTANCE   hInst = NULL;
static char        *destBuf = NULL;
static int         twain_run_mode = RUN_STANDARD;

/* Forward declarations */
void preTransferCallback(void *);
int  beginTransferCallback(pTW_IMAGEINFO, void *);
int  dataTransferCallback(pTW_IMAGEINFO, pTW_IMAGEMEMXFER, void *);
int  endTransferCallback(int, int, void *);
void postTransferCallback(int, void *);

static void init(void);
static void quit(void);
static void query(void);
static void run(char *, int, GParam *, int *, GParam **);

/* This plug-in's functions */
GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};
	
extern void set_gimp_PLUG_IN_INFO_PTR(GPlugInInfo *);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

/* Data structure holding data between runs */
/* Currently unused... Eventually may be used
 * to track dialog data.
 */
typedef struct {
  gchar  sourceName[34];
  gfloat xResolution;
  gfloat yResolution;
  gint   xOffset;
  gint   yOffset;
  gint   width;
  gint   height;
  gint   imageType;
} TwainValues;

/* Default Twain values */
static TwainValues twainvals = 
{ 
  "",
  100.0, 100.0,
  0, 0,
  0, 0,
  TWPT_RGB
};

/* The standard callback functions */
TXFR_CB_FUNCS standardCbFuncs = {
  preTransferCallback,
  beginTransferCallback,
  dataTransferCallback,
  endTransferCallback,
  postTransferCallback };

/******************************************************************
 * Dump handling
 ******************************************************************/

#ifdef _DEBUG
/* The dumper callback functions */
TXFR_CB_FUNCS dumperCbFuncs = {
  dumpPreTransferCallback,
  dumpBeginTransferCallback,
  dumpDataTransferCallback,
  dumpEndTransferCallback,
  dumpPostTransferCallback };

static void
setRunMode(char *argv[])
{
  char *exeName = strrchr(argv[0], '\\') + 1;

  LogMessage("Executable name: %s\n", exeName);

  if (!_stricmp(exeName, "DTWAIN.EXE"))
    twain_run_mode = RUN_DUMP;

  if (!_stricmp(exeName, "RTWAIN.EXE"))
    twain_run_mode = RUN_READDUMP;
}
#endif /* _DEBUG */
	
/******************************************************************
 * Win32 entry point and setup...
 ******************************************************************/
	
/*
 * WinMain
 *
 * The standard gimp entry point won't quite cut it for
 * this plug-in.  This plug-in requires creation of a
 * standard Win32 window (hidden) in order to receive
 * and process window messages on behalf of the TWAIN
 * datasource.
 */
int APIENTRY 
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{
		
  /* 
   * Normally, we would do all of the Windows-ish set up of
   * the window classes and stuff here in WinMain.  But,
   * the only time we really need the window and message
   * queues is during the plug-in run.  So, all of that will
   * be done during run().  This avoids all of the Windows
   * setup stuff for the query().  Stash the instance handle now
   * so it is available from the run() procedure.
   */
  hInst = hInstance;

#ifdef _DEBUG
  /* When in debug version, we allow different run modes...
   * make sure that it is correctly set.
   */
  setRunMode(__argv);
#endif /* _DEBUG */

  /*
   * Now, call gimp_main... This is what the MAIN() macro
   * would usually do.
   */
  set_gimp_PLUG_IN_INFO_PTR(&PLUG_IN_INFO);
  return gimp_main(__argc, __argv);
}
	
/*
 * initTwainAppIdentity
 *
 * Initialize and return our application's identity for
 * the TWAIN runtime.
 */
static pTW_IDENTITY
getAppIdentity(void)
{
  pTW_IDENTITY appIdentity = (pTW_IDENTITY) malloc (sizeof(TW_IDENTITY));
		
  /* Set up the application identity */
  appIdentity->Id = 0;
  appIdentity->Version.MajorNum = 0;
  appIdentity->Version.MinorNum = 1;
  appIdentity->Version.Language = TWLG_USA;
  appIdentity->Version.Country = TWCY_USA;
  strcpy(appIdentity->Version.Info, "GIMP TWAIN 0.1");
  appIdentity->ProtocolMajor = TWON_PROTOCOLMAJOR;
  appIdentity->ProtocolMinor = TWON_PROTOCOLMINOR;
  appIdentity->SupportedGroups = DG_IMAGE;
  strcpy(appIdentity->Manufacturer, "Craig Setera");
  strcpy(appIdentity->ProductFamily, "GIMP");
  strcpy(appIdentity->ProductName, "GIMP for WIN32");
		
  return appIdentity;
}
	
/*
 * initializeTwain
 *
 * Do the necessary TWAIN initialization.  This sets up
 * our TWAIN session information.  The session stuff is
 * something built by me on top of the standard TWAIN
 * datasource manager calls.
 */
void
initializeTwain(void)
{
  pTW_IDENTITY appIdentity;
		
  /* Get our application's identity */
  appIdentity = getAppIdentity();
		
  /* Create a new session object */
  twSession = newSession(appIdentity);
		
  /* Register our image transfer callback functions */
#ifdef _DEBUG
  if (twain_run_mode == RUN_DUMP)
    registerTransferCallbacks(twSession, &dumperCbFuncs, NULL);
  else
#endif /* _DEBUG */
    registerTransferCallbacks(twSession, &standardCbFuncs, NULL);
}
	
/*
 * InitApplication
 *
 * Initialize window data and register the window class
 */
BOOL 
InitApplication(HINSTANCE hInstance)
{
  WNDCLASS wc;
  BOOL retValue;
		
  /*
   * Fill in window class structure with parameters to describe
   * the main window.
   */
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = (WNDPROC) WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
  wc.lpszClassName = APP_NAME;
  wc.lpszMenuName = NULL;
		
  /* Register the window class and stash success/failure code. */
  retValue = RegisterClass(&wc);
		
  /* Log error */
  if (!retValue)
    LogLastWinError();
		
  return retValue;
}
	
/*
 * InitInstance
 * 
 * Create the main window for the application.  Used to
 * interface with the TWAIN datasource.
 */
BOOL 
InitInstance(HINSTANCE hInstance, int nCmdShow)
{
  /* Create our window */
  hwnd = CreateWindow(APP_NAME, APP_NAME, WS_OVERLAPPEDWINDOW,
		      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
		      NULL, NULL, hInstance, NULL);
		
  if (!hwnd) {
    return (FALSE);
  }
		
  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);
		
  return TRUE;
}
	
/*
 * twainWinMain
 *
 * This is the function that represents the code that
 * would normally reside in WinMain (see above).  This
 * function will get called during run() in order to set
 * up the windowing environment necessary for TWAIN to
 * operate.
 */
int
twainWinMain(void) 
{
		
  /* Initialize the twain information */
  initializeTwain();
		
  /* Perform instance initialization */
  if (!InitApplication(hInst))
    return (FALSE);
		
  /* Perform application initialization */
  if (!InitInstance(hInst, SHOW_WINDOW))
    return (FALSE);
		
  /* 
   * Call the main message processing loop...
   * This call will not return until the application
   * exits.
   */
  return twainMessageLoop(twSession);
}
	
/*
 * WndProc
 *
 * Process window message for the main window.
 */
LRESULT CALLBACK 
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {
			
  case WM_CREATE:
    /* Register our window handle with the TWAIN
     * support.
     */
    registerWindowHandle(twSession, hWnd);
			
    /* Schedule the image transfer by posting a message */
    PostMessage(hWnd, WM_TRANSFER_IMAGE, 0, 0);
    break;
			
  case WM_TRANSFER_IMAGE:
    /* Get an image */
#ifdef _DEBUG
    if (twain_run_mode == RUN_READDUMP)
      readDumpedImage(twSession);
    else
#endif /* _DEBUG */
      getImage(twSession);
    break;
			
  case WM_DESTROY:
    LogMessage("Exiting application\n");
    PostQuitMessage(0);
    break;
			
  default:
    return (DefWindowProc(hWnd, message, wParam, lParam));
  }
  return 0;
}
	
/******************************************************************
 * GIMP Plug-in entry points
 ******************************************************************/
	
/*
 * Plug-in Parameter definitions
 */
#define NUMBER_IN_ARGS 1
#define IN_ARGS { PARAM_INT32, "run_mode", "Interactive, non-interactive" }
#define NUMBER_OUT_ARGS 2
#define OUT_ARGS \
	{ PARAM_INT32, "image_count", "Number of acquired images" }, \
	{ PARAM_INT32ARRAY, "image_ids", "Array of acquired image identifiers" }

	
/*
 * query
 *
 * The plug-in is being queried.  Install our procedure for
 * acquiring.
 */
static void 
query(void)
{
  static GParamDef args[] = { IN_ARGS };
  static GParamDef return_vals[] = { OUT_ARGS };

#ifdef _DEBUG
  if (twain_run_mode == RUN_DUMP)
    /* the installation of the plugin */
    gimp_install_procedure(PLUG_IN_D_NAME,
			   PLUG_IN_DESCRIPTION,
			   PLUG_IN_HELP,
			   PLUG_IN_AUTHOR,
			   PLUG_IN_COPYRIGHT,
			   PLUG_IN_VERSION,
			   PLUG_IN_D_MENU_PATH,
			   NULL,
			   PROC_EXTENSION,		
			   NUMBER_IN_ARGS,
			   NUMBER_OUT_ARGS,
			   args,
			   return_vals);

  else if (twain_run_mode == RUN_READDUMP)
    /* the installation of the plugin */
    gimp_install_procedure(PLUG_IN_R_NAME,
			   PLUG_IN_DESCRIPTION,
			   PLUG_IN_HELP,
			   PLUG_IN_AUTHOR,
			   PLUG_IN_COPYRIGHT,
			   PLUG_IN_VERSION,
			   PLUG_IN_R_MENU_PATH,
			   NULL,
			   PROC_EXTENSION,		
			   NUMBER_IN_ARGS,
			   NUMBER_OUT_ARGS,
			   args,
			   return_vals);
  else
#endif /* _DEBUG */
    /* the installation of the plugin */
    gimp_install_procedure(PLUG_IN_NAME,
			   PLUG_IN_DESCRIPTION,
			   PLUG_IN_HELP,
			   PLUG_IN_AUTHOR,
			   PLUG_IN_COPYRIGHT,
			   PLUG_IN_VERSION,
			   PLUG_IN_MENU_PATH,
			   NULL,
			   PROC_EXTENSION,		
			   NUMBER_IN_ARGS,
			   NUMBER_OUT_ARGS,
			   args,
			   return_vals);
}
	
/*
 * twainDisplayImage
 *
 * Display the image specified by the image id.
 */
static gint
twainDisplayImage(gint32 image)
{
  GParam *params;
  gint retval;
		
  params = gimp_run_procedure ("gimp_display_new",
			       &retval,
			       PARAM_IMAGE, image,
			       PARAM_END);
		
  return retval;
}
	
/* Return values storage */
static GParam values[3];

/*
 * run
 *
 * The plug-in is being requested to run.
 * Capture an image from a TWAIN datasource
 */
static void 
run(gchar *name,		/* name of plugin */
    gint nparams,		/* number of in-paramters */
    GParam *param,		/* in-parameters */
    gint *nreturn_vals,	        /* number of out-parameters */
    GParam **return_vals)	/* out-parameters */
{
  GRunModeType run_mode;

  /* Initialize the return values
   * Always return at least the status to the caller. 
   */
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_SUCCESS;
  *nreturn_vals = 1;
  *return_vals = values;

  /* Before we get any further, verify that we have
   * TWAIN and that there is actually a datasource
   * to be used in doing the acquire.
   */
  if (!twainIsAvailable()) {
    values[0].data.d_status = STATUS_EXECUTION_ERROR;
    return;
  }

  /* Get the runmode from the in-parameters */
  run_mode = param[0].data.d_int32;
		
  /* Set up the rest of the return parameters */
  values[1].type = PARAM_INT32;
  values[1].data.d_int32 = 0;
  values[2].type = PARAM_INT32ARRAY;
  values[2].data.d_int32array = (gint32 *) malloc (sizeof(gint32) * MAX_IMAGES);
		
  /* How are we running today? */
  switch (run_mode) {
  case RUN_INTERACTIVE:
    /* Retrieve values from the last run...
     * Currently ignored
     */
    gimp_get_data(PLUG_IN_NAME, &twainvals);
    break;
			
  case RUN_NONINTERACTIVE:
    /* Currently, we don't do non-interactive calls.
     * Bail if someone tries to call us non-interactively
     */
    values[0].data.d_status = STATUS_CALLING_ERROR;
    return;
			
  case RUN_WITH_LAST_VALS:
    /* Retrieve values from the last run...
     * Currently ignored
     */
    gimp_get_data(PLUG_IN_NAME, &twainvals);
    break;
			
  default:
    break;
  } /* switch */
		
  /* Have we succeeded so far? */
  if (values[0].data.d_status == STATUS_SUCCESS)
    twainWinMain();
		
  /* Check to make sure we got at least one valid
   * image.
   */
  if (values[1].data.d_int32 > 0) {
    /* An image was captured from the TWAIN
     * datasource.  Do final Interactive
     * steps.
     */
    if (run_mode == RUN_INTERACTIVE) {
      /* Store variable states for next run */
      gimp_set_data(PLUG_IN_NAME, &twainvals, sizeof (TwainValues));
    }
			
    /* Set return values */
    *nreturn_vals = 3;
  } else {
    values[0].data.d_status = STATUS_EXECUTION_ERROR;
  }
}
	
/***********************************************************************
 * Image transfer callback functions
 ***********************************************************************/
	
/* Data used to carry data between each of
 * the callback function calls.
 */
typedef struct {
  gint32 image_id;
  gint32 layer_id;
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  pTW_PALETTE8 paletteData;
  int totalPixels;
  int completedPixels;
} ClientDataStruct, *pClientDataStruct;

/*
 * preTransferCallback
 *
 * This callback function is called before any images
 * are transferred.  Set up the one time only stuff.
 */
void
preTransferCallback(void *clientData)
{
  /* Initialize our progress dialog */
  gimp_progress_init("Transferring TWAIN data");
}

/*
 * beginTransferCallback
 *
 * The following function is called at the beginning
 * of each image transfer.
 */
int
beginTransferCallback(pTW_IMAGEINFO imageInfo, void *clientData)
{
  int done = 0;
  int imageType, layerType;

  pClientDataStruct theClientData = 
    (pClientDataStruct) malloc (sizeof(ClientDataStruct));

#ifdef _DEBUG		
  logBegin(imageInfo, clientData);
#endif
		
  /* Decide on the image type */
  switch (imageInfo->PixelType) {
  case TWPT_BW:
  case TWPT_GRAY:
    /* Set up the image and layer types */
    imageType = GRAY;
    layerType = GRAY_IMAGE;
    break;
			
  case TWPT_RGB:
    /* Set up the image and layer types */
    imageType = RGB;
    layerType = RGB_IMAGE;
    break;

  case TWPT_PALETTE:
    /* Get the palette data */
    theClientData->paletteData = (pTW_PALETTE8) malloc (sizeof(TW_PALETTE8));
    twSession->twRC = callDSM(APP_IDENTITY(twSession), DS_IDENTITY(twSession),
			      DG_IMAGE, DAT_PALETTE8, MSG_GET,
			      (TW_MEMREF) theClientData->paletteData);
    if (twSession->twRC != TWRC_SUCCESS)
      return FALSE;

    switch (theClientData->paletteData->PaletteType) {
    case TWPA_RGB:
      /* Set up the image and layer types */
      imageType = RGB;
      layerType = RGB_IMAGE;
      break;

    case TWPA_GRAY:
      /* Set up the image and layer types */
      imageType = GRAY;
      layerType = GRAY_IMAGE;
      break;

    default:
      return FALSE;
    }
    break;

  default:
    /* We don't know how to deal with anything other than
     * the types listed above.  Bail for any other image
     * type.
     */
    return FALSE;
  }
		
  /* Create the GIMP image */
  theClientData->image_id = gimp_image_new(imageInfo->ImageWidth, 
					   imageInfo->ImageLength, imageType);
			
  /* Create a layer */
  theClientData->layer_id = gimp_layer_new(theClientData->image_id,
					   "Background",
					   imageInfo->ImageWidth, 
					   imageInfo->ImageLength,
					   layerType, 100, NORMAL_MODE);
		
  /* Add the layer to the image */
  gimp_image_add_layer(theClientData->image_id, 
		       theClientData->layer_id, 0);
		
  /* Update the progress dialog */
  theClientData->totalPixels = imageInfo->ImageWidth * imageInfo->ImageLength;
  theClientData->completedPixels = 0;
  gimp_progress_update((double) 0);
		
  /* Get our drawable */
  theClientData->drawable = gimp_drawable_get(theClientData->layer_id);
		
  /* Initialize a pixel region for writing to the image */
  gimp_pixel_rgn_init(&(theClientData->pixel_rgn), theClientData->drawable, 
		      0, 0, imageInfo->ImageWidth, imageInfo->ImageLength,
		      TRUE, FALSE);
		
  /* Store our client data for the data transfer callbacks */
  if (clientData)
    free(clientData);
  setClientData(twSession, (void *) theClientData);
		
  /* Make sure to return TRUE to continue the image
   * transfer
   */
  return TRUE;
}
	
/*
 * bitTransferCallback
 *
 * The following function is called for each memory
 * block that is transferred from the data source if
 * the image type is Black/White.
 *
 * Black and white data is unpacked from bit data
 * into byte data and written into a gray scale GIMP
 * image.
 */
static char bitMasks[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
static int 
bitTransferCallback(pTW_IMAGEINFO imageInfo,
		    pTW_IMAGEMEMXFER imageMemXfer,
		    void *clientData)
{
  int row, col, offset;
  char *srcBuf;
  int rows = imageMemXfer->Rows;
  int cols = imageMemXfer->Columns;
  pClientDataStruct theClientData = (pClientDataStruct) clientData;
		
  /* Allocate a buffer as necessary */
  if (!destBuf)
    destBuf = (char *) malloc (rows * cols);
		
  /* Unpack the image data from bits into bytes */
  srcBuf = (char *) imageMemXfer->Memory.TheMem;
  offset = 0;
  for (row = 0; row < rows; row++) {
    for (col = 0; col < cols; col++) {
      char byte = srcBuf[(row * imageMemXfer->BytesPerRow) + (col / 8)];
      destBuf[offset++] = ((byte & bitMasks[col % 8]) != 0) ? 255 : 0;
    }
  }
		
  /* Update the complete chunk */
  gimp_pixel_rgn_set_rect(&(theClientData->pixel_rgn), 
			  (guchar *) destBuf,
			  imageMemXfer->XOffset, imageMemXfer->YOffset,
			  cols, rows);
		
  /* Update the user on our progress */
  theClientData->completedPixels += (cols * rows);
  gimp_progress_update((double) theClientData->completedPixels / 
		       (double) theClientData->totalPixels);
		
  return TRUE;
}
	
/*
 * oneBytePerSampleTransferCallback
 *
 * The following function is called for each memory
 * block that is transferred from the data source if
 * the image type is Grayscale or RGB.  This transfer
 * mode is quicker than the modes that require translation
 * from a greater number of bits per sample down to the
 * 8 bits per sample understood by The GIMP.
 */
static int 
oneBytePerSampleTransferCallback(pTW_IMAGEINFO imageInfo,
		     pTW_IMAGEMEMXFER imageMemXfer,
		     void *clientData)
{
  int row;
  char *srcBuf;
  int bytesPerPixel = imageInfo->BitsPerPixel / 8;
  int rows = imageMemXfer->Rows;
  int cols = imageMemXfer->Columns;
  pClientDataStruct theClientData = (pClientDataStruct) clientData;
		
  /* Allocate a buffer as necessary */
  if (!destBuf)
    destBuf = (char *) malloc (rows * cols * bytesPerPixel);
		
  /* The bytes coming from the source may not be padded in
   * a way that The GIMP is terribly happy with.  It is
   * possible to transfer row by row, but that is particularly
   * expensive in terms of performance.  It is much cheaper
   * to rearrange the data and transfer it in one large chunk.
   * The next chunk of code rearranges the incoming data into
   * a non-padded chunk for The GIMP.
   */
  srcBuf = (char *) imageMemXfer->Memory.TheMem;
  for (row = 0; row < rows; row++) {
    /* Copy the current row */
    memcpy((destBuf + (row * bytesPerPixel * cols)),
	   (srcBuf + (row * imageMemXfer->BytesPerRow)),
	   (bytesPerPixel * cols));
  }
		
  /* Update the complete chunk */
  gimp_pixel_rgn_set_rect(&(theClientData->pixel_rgn), 
			  (guchar *) destBuf,
			  imageMemXfer->XOffset, imageMemXfer->YOffset,
			  cols, rows);
		
  /* Update the user on our progress */
  theClientData->completedPixels += (cols * rows);
  gimp_progress_update((double) theClientData->completedPixels / 
		       (double) theClientData->totalPixels);
		
  return TRUE;
}
	
/*
 * twoBytesPerSampleTransferCallback
 *
 * The following function is called for each memory
 * block that is transferred from the data source if
 * the image type is Grayscale or RGB.
 */ 
static int 
twoBytesPerSampleTransferCallback(pTW_IMAGEINFO imageInfo,
		     pTW_IMAGEMEMXFER imageMemXfer,
		     void *clientData)
{
  static float ratio = 0.00390625;
  int row, col, sample;
  char *srcBuf, *destByte;
  int rows = imageMemXfer->Rows;
  int cols = imageMemXfer->Columns;
  int bitsPerSample = imageInfo->BitsPerPixel / imageInfo->SamplesPerPixel;
  int bytesPerSample = bitsPerSample / 8;

  TW_UINT16 *samplePtr;

  pClientDataStruct theClientData = (pClientDataStruct) clientData;
		
  /* Allocate a buffer as necessary */
  if (!destBuf)
    destBuf = (char *) malloc (rows * cols * imageInfo->SamplesPerPixel);
		
  /* The bytes coming from the source may not be padded in
   * a way that The GIMP is terribly happy with.  It is
   * possible to transfer row by row, but that is particularly
   * expensive in terms of performance.  It is much cheaper
   * to rearrange the data and transfer it in one large chunk.
   * The next chunk of code rearranges the incoming data into
   * a non-padded chunk for The GIMP.  This function must also
   * reduce from multiple bytes per sample down to single byte
   * per sample.
   */
  /* Work through the rows */
  for (row = 0; row < rows; row++) {
    /* The start of this source row */
    samplePtr = (TW_UINT16 *) 
      ((char *) imageMemXfer->Memory.TheMem + (row * imageMemXfer->BytesPerRow));

    /* The start of this dest row */
    destByte = destBuf + (row * imageInfo->SamplesPerPixel * cols);

    /* Work through the columns */
    for (col = 0; col < cols; col++) {
      /* Finally, work through each of the samples */
      for (sample = 0; sample < imageInfo->SamplesPerPixel; sample++) {
				/* Get the value */
	TW_UINT16 value = *samplePtr;

				/* Move the sample pointer */
	samplePtr++;

				/* Place in the destination */
	*destByte = (char) ((float) value * (float) ratio);
	destByte++;
      }
    }
  }
		
  /* Send the complete chunk */
  gimp_pixel_rgn_set_rect(&(theClientData->pixel_rgn), 
			  (guchar *) destBuf,
			  imageMemXfer->XOffset, imageMemXfer->YOffset,
			  cols, rows);
		
  /* Update the user on our progress */
  theClientData->completedPixels += (cols * rows);
  gimp_progress_update((double) theClientData->completedPixels / 
		       (double) theClientData->totalPixels);
		
  return TRUE;
}
	
/*
 * palettedTransferCallback
 *
 * The following function is called for each memory
 * block that is transferred from the data source if
 * the image type is paletted.  This does not create
 * an indexed image type in The GIMP because for some
 * reason it does not allow creation of a specific
 * palette.  This function will create an RGB or Gray
 * image and use the palette to set the details of
 * the pixels.
 */ 
static int 
palettedTransferCallback(pTW_IMAGEINFO imageInfo,
			 pTW_IMAGEMEMXFER imageMemXfer,
			 void *clientData)
{
  int channelsPerEntry;
  int row, col;
  int rows = imageMemXfer->Rows;
  int cols = imageMemXfer->Columns;
  char *destPtr = NULL, *srcPtr = NULL;

  /* Get the client data */
  pClientDataStruct theClientData = (pClientDataStruct) clientData;
		
  /* Look up the palette entry size */
  channelsPerEntry = 
    (theClientData->paletteData->PaletteType == TWPA_RGB) ? 3 : 1;

  /* Allocate a buffer as necessary */
  if (!destBuf)
    destBuf = (char *) malloc (rows * cols * channelsPerEntry);

  /* Work through the rows */
  destPtr = destBuf;
  for (row = 0; row < rows; row++) {
    srcPtr = (char *) ((char *) imageMemXfer->Memory.TheMem + 
		       (row * imageMemXfer->BytesPerRow));

    /* Work through the columns */
    for (col = 0; col < cols; col++) {
      /* Get the palette index */
      int index = (unsigned char) *srcPtr;
      srcPtr++;

      switch (theClientData->paletteData->PaletteType) {
      case TWPA_GRAY:
	*destPtr = theClientData->paletteData->Colors[index].Channel1;
	destPtr++;
	break;

      case TWPA_RGB:
	*destPtr = theClientData->paletteData->Colors[index].Channel1;
	destPtr++;
	*destPtr = theClientData->paletteData->Colors[index].Channel2;
	destPtr++;
	*destPtr = theClientData->paletteData->Colors[index].Channel3;
	destPtr++;
      }
    }
  }
		
  /* Send the complete chunk */
  gimp_pixel_rgn_set_rect(&(theClientData->pixel_rgn), 
			  (guchar *) destBuf,
			  imageMemXfer->XOffset, imageMemXfer->YOffset,
			  cols, rows);
		
  /* Update the user on our progress */
  theClientData->completedPixels += (cols * rows);
  gimp_progress_update((double) theClientData->completedPixels / 
		       (double) theClientData->totalPixels);
		
  return TRUE;
}
	
/*
 * dataTransferCallback
 *
 * The following function is called for each memory
 * block that is transferred from the data source.
 */
int 
dataTransferCallback(pTW_IMAGEINFO imageInfo,
		     pTW_IMAGEMEMXFER imageMemXfer,
		     void *clientData)
{
#ifdef _DEBUG
  logData(imageInfo, imageMemXfer, clientData);
#endif
		
  /* Choose the appropriate transfer handler */
  switch (imageInfo->PixelType) {
  case TWPT_PALETTE:
    return palettedTransferCallback(imageInfo, imageMemXfer, clientData);

  case TWPT_BW:
    return bitTransferCallback(imageInfo, imageMemXfer, clientData);
			
  case TWPT_GRAY:
  case TWPT_RGB:
    switch (imageInfo->BitsPerPixel / imageInfo->SamplesPerPixel) {
    case 8:
      return oneBytePerSampleTransferCallback(imageInfo, imageMemXfer, clientData);
				
    case 16:
      return twoBytesPerSampleTransferCallback(imageInfo, imageMemXfer, clientData);

    default:
      return FALSE;
    }
			
  default:
    return FALSE;
  }
}
	
/*
 * endTransferCallback
 *
 * The following function is called at the end of the
 * image transfer.  The caller will be handed
 * the image transfer completion state.  The
 * following values (defined in twain.h) are
 * possible:
 *
 * TWRC_XFERDONE
 *  The transfer completed successfully
 * TWRC_CANCEL
 *  The transfer was completed by the user
 * TWRC_FAILURE
 *  The transfer failed.
 */
int
endTransferCallback(int completionState, int pendingCount, void *clientData)
{
  pClientDataStruct theClientData = (pClientDataStruct) clientData;
		
  LogMessage("endTransferCallback: CompState = %d, pending = %d\n",
	     completionState, pendingCount);

  /* Clean up and detach from the drawable */
  if (destBuf) {
    free(destBuf);
    destBuf = NULL;
  }
  gimp_drawable_flush(theClientData->drawable);
  gimp_drawable_detach(theClientData->drawable);

  /* Make sure to check our return code */
  if (completionState == TWRC_XFERDONE) {
    /* We have a completed image transfer */
    values[2].type = PARAM_INT32ARRAY;	
    values[2].data.d_int32array[values[1].data.d_int32++] =
      theClientData->image_id;
				
    /* Display the image */
    LogMessage("Displaying image %d\n", theClientData->image_id);
    twainDisplayImage(theClientData->image_id);
  } else {
    /* The transfer did not complete successfully */
    LogMessage("Deleting image\n");
    gimp_image_delete(theClientData->image_id);
  }
		
  /* Shut down if we have received all of the possible images */
  return (values[1].data.d_int32 < MAX_IMAGES);
}

/*
 * postTransferCallback
 *
 * This callback function will be called
 * after all possible images have been
 * transferred.
 */
void
postTransferCallback(int pendingCount, void *clientData)
{
  /* Shut things down. */
  if (pendingCount != 0)
    cancelPendingTransfers(twSession);

  /* This will close the datasource and datasource
   * manager.  Then the message queue will be shut
   * down and the run() procedure will finally be
   * able to finish.
   */
  disableDS(twSession);
  closeDS(twSession);
  closeDSM(twSession);
		
  /* Post a message to close up the application */
  PostQuitMessage(0);
}
