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

#ifndef _TW_FUNC_H
#define _TW_FUNC_H
#include "twain.h"

/*
 * Pre-image transfer function type
 *
 * Sent to the caller before any of the
 * images are transferred to the application.
 */
typedef void (* TW_PRE_TXFR_CB)(void *);

/*
 * Image transfer begin function type
 *
 * Sent to the caller when an image transfer
 * is about to begin.  The caller may return
 * FALSE if the transfer should not continue.
 * Otherwise, the function should return a
 * TRUE value.
 */
typedef int (* TW_TXFR_BEGIN_CB)(pTW_IMAGEINFO, void *);

/*
 * Image transfer callback function type
 *
 * Expected to return true if the image transfer
 * should continue.  False if the transfer should
 * be cancelled.
 */
typedef int (* TW_TXFR_DATA_CB)(pTW_IMAGEINFO, pTW_IMAGEMEMXFER, void *);

/*
 * Image transfer end function type
 *
 * Sent to the caller when an image transfer
 * is completed.  The caller will be handed
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
typedef int (* TW_TXFR_END_CB)(int, int, void *);

/*
 * Post-image transfer callback
 *
 * This callback function is called after all
 * of the possible images have been transferred
 * from the datasource.
 */
typedef void (* TW_POST_TXFR_CB)(int, void *);

/*
 * The following structure defines the
 * three callback functions that are called
 * while an image is being transferred.
 * The types of these functions are defined
 * above.  Any function that is NULL will just
 * not be called.
 */
typedef struct _TXFR_CB_FUNCS {
  /* Pre-transfer function */
  TW_PRE_TXFR_CB preTxfrCb;

  /* Begin function */
  TW_TXFR_BEGIN_CB txfrBeginCb;

  /* Data transfer */
  TW_TXFR_DATA_CB txfrDataCb;

  /* End function */
  TW_TXFR_END_CB txfrEndCb;

  /* Post-transfer function */
  TW_POST_TXFR_CB postTxfrCb;
} TXFR_CB_FUNCS, *pTXFR_CB_FUNCS;

/*
 * Data representing a TWAIN
 * application to data source
 * session.
 */
typedef struct _TWAIN_SESSION {
  /* The window handle related to the TWAIN application */
  HWND hwnd;

  /* The current TWAIN return code */
  TW_UINT16 twRC;

  /* The application's TWAIN identity */
  pTW_IDENTITY appIdentity;

  /* The datasource's TWAIN identity */
  pTW_IDENTITY dsIdentity;

  /* The image data transfer functions */
  pTXFR_CB_FUNCS transferFunctions;

  /* Client data that is associated with the image
   * transfer callback
   */
  void *clientData;

  /*
   * The following variable tracks the current state
   * as related to the TWAIN engine.  The states are:
   *
   * 1) Pre-session: The DSM has not been loaded
   * 2) Source Manager Loaded (not opened)
   * 3) Source Manager Opened
   * 4) Source Open
   * 5) Source Enabled
   * 6) Transfer ready
   * 7) Transferring
   */
  int twainState;

} TW_SESSION, *pTW_SESSION;

/* Session structure access
 * macros
 */
/* #define pAPP_IDENTITY(tw_session) (&(tw_session->appIdentity)) */
#define APP_IDENTITY(tw_session) (tw_session->appIdentity)
/* #define pDS_IDENTITY(tw_session) (&(tw_session->dsIdentity)) */
#define DS_IDENTITY(tw_session) (tw_session->dsIdentity)

/* State macros... Each expects
 * a Twain Session pointer
 */
#define TWAIN_LOADED(tw_session) (tw_session->twainState >= 2)
#define TWAIN_UNLOADED(tw_session) (tw_session->twainState < 2)
#define DSM_IS_OPEN(tw_session) (tw_session->twainState >= 3)
#define DSM_IS_CLOSED(tw_session) (tw_session->twainState < 3)
#define DS_IS_OPEN(tw_session) (tw_session->twainState >= 4)
#define DS_IS_CLOSED(tw_session) (tw_session->twainState < 4)
#define DS_IS_ENABLED(tw_session) (tw_session->twainState >= 5)
#define DS_IS_DISABLED(tw_session) (tw_session->twainState < 5)

/* Function declarations */
TW_UINT16 callDSM(pTW_IDENTITY, pTW_IDENTITY, 
		  TW_UINT32, TW_UINT16, 
		  TW_UINT16, TW_MEMREF);
char *twainError(int);
char *currentTwainError(pTW_SESSION);
int twainIsAvailable(void);
int getImage(pTW_SESSION);
int loadTwainLibrary(pTW_SESSION);
int openDSM(pTW_SESSION);
int selectDS(pTW_SESSION);
int selectDefaultDS(pTW_SESSION);
int openDS(pTW_SESSION);
int requestImageAcquire(pTW_SESSION, BOOL);
int disableDS(pTW_SESSION);
int closeDS(pTW_SESSION);
int closeDSM(pTW_SESSION);
int unloadTwainLibrary(pTW_SESSION);
int twainMessageLoop(pTW_SESSION);
void cancelPendingTransfers(pTW_SESSION);

TW_FIX32 FloatToFix32(float);
float FIX32ToFloat(TW_FIX32);

pTW_SESSION newSession(pTW_IDENTITY);
void registerWindowHandle(pTW_SESSION, HWND);
void registerTransferCallbacks(pTW_SESSION, pTXFR_CB_FUNCS, void *);
void setClientData(pTW_SESSION session, void *clientData);

#endif // _TW_FUNC_H
