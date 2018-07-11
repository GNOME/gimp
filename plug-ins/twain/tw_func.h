/*
 * TWAIN Plug-in
 * Copyright (C) 1999 Craig Setera
 * Craig Setera <setera@home.com>
 * 03/31/1999
 *
 * Updated for Mac OS X support
 * Brion Vibber <brion@pobox.com>
 * 07/22/2004
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
 * Should compile and run on both Win32 and Mac OS X 10.3 (possibly
 * also on 10.2).
 *
 * Known problems:
 * - Multiple image transfers will hang the plug-in.  The current
 *   configuration compiles with a maximum of single image transfers.
 * - On Mac OS X, canceling doesn't always close things out fully.
 * - Epson TWAIN driver on Mac OS X crashes the plugin when scanning.
 */

/*
 * Revision history
 *  (02/07/99)  v0.1   First working version (internal)
 *  (02/09/99)  v0.2   First release to anyone other than myself
 *  (02/15/99)  v0.3   Added image dump and read support for debugging
 *  (03/31/99)  v0.5   Added support for multi-byte samples and paletted
 *                     images.
 *  (07/23/04)  v0.6   Added Mac OS X support.
 */

#ifndef _TW_FUNC_H
#define _TW_FUNC_H

#include "tw_platform.h"

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
  /* The window handle related to the TWAIN application on Win32 */
  TW_HANDLE hwnd;

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
char *twainError(int);
char *currentTwainError(pTW_SESSION);
int getImage(pTW_SESSION);
int loadTwainLibrary(pTW_SESSION);
int unloadTwainLibrary(pTW_SESSION twSession);
int openDSM(pTW_SESSION);
int selectDS(pTW_SESSION);
int selectDefaultDS(pTW_SESSION);
int openDS(pTW_SESSION);
int requestImageAcquire(pTW_SESSION, gboolean);
int disableDS(pTW_SESSION);
int closeDS(pTW_SESSION);
int closeDSM(pTW_SESSION);
void cancelPendingTransfers(pTW_SESSION);
int scanImage (void);

TW_FIX32 FloatToFIX32(float);
float FIX32ToFloat(TW_FIX32);

void processTwainMessage(TW_UINT16 message, pTW_SESSION twSession);

pTW_SESSION newSession(pTW_IDENTITY);
void registerWindowHandle(pTW_SESSION, TW_HANDLE);
void registerTransferCallbacks(pTW_SESSION, pTXFR_CB_FUNCS, void *);
void setClientData(pTW_SESSION session, void *clientData);
pTW_SESSION initializeTwain(void);

#ifdef G_OS_WIN32
void LogLastWinError(void);
BOOL InitApplication(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, pTW_SESSION twSession);
#endif

#endif /* _TW_FUNC_H */
