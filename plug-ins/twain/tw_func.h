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
 * Added for Win x64 support, changed data source selection.
 * Jens M. Plonka <jens.plonka@gmx.de>
 * 11/25/2011
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
 *  (11/25/11)  v0.7   Added Win x64 support, changed data source selection.
 */

#ifndef _TW_FUNC_H
#define _TW_FUNC_H
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
typedef void (* TW_TXFR_END_CB)(int, void *);

/*
 * Post-image transfer callback
 *
 * This callback function is called after all
 * of the possible images have been transferred
 * from the datasource.
 */
typedef void (* TW_POST_TXFR_CB)(void *);

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

typedef struct _TW_DATA_SOURCE {
  pTW_IDENTITY dsIdentity;
  struct _TW_DATA_SOURCE *dsNext;
} TW_DATA_SOURCE, *pTW_DATA_SOURCE;

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

  const gchar *name;
  /*
   * The available data sources for this session.
   * The list will be updated each time the plugin is invoked on query.
   */
  struct _TW_DATA_SOURCE *dataSources;

} TW_SESSION, *pTW_SESSION;

#define CB_XFER_PRE(s) if (s->transferFunctions->preTxfrCb) \
    (*s->transferFunctions->preTxfrCb) (s->clientData);

#define CB_XFER_BEGIN(s, i)   if (s->transferFunctions->txfrBeginCb) \
    if (!(*s->transferFunctions->txfrBeginCb) (imageInfo, s->clientData)) \
      return FALSE; \
  return TRUE;

#define CB_XFER_DATA(s, i, m) (*s->transferFunctions->txfrDataCb) \
    (i, &m, s->clientData)

#define CB_XFER_END(s)   if (s->transferFunctions->txfrEndCb) \
    (*s->transferFunctions->txfrEndCb) (s->twRC, s->clientData)

#define CB_XFER_POST(s) if (s->transferFunctions->postTxfrCb) \
    (*s->transferFunctions->postTxfrCb) (s->clientData)

/* Session structure access
 * macros
 */
#define APP_IDENTITY(s)   ((s == NULL) ? NULL : s->appIdentity)
#define DS_IDENTITY(s)    ((s == NULL) ? NULL : s->dsIdentity)

/* State macros... Each expects
 * a Twain Session pointer
 */
#define TWAIN_LOADED(s)   ((s == NULL) ? FALSE : s->twainState >= 2)
#define TWAIN_UNLOADED(s) ((s == NULL) ? TRUE  : s->twainState <  2)
#define DSM_IS_OPEN(s)    ((s == NULL) ? FALSE : s->twainState >= 3)
#define DSM_IS_CLOSED(s)  ((s == NULL) ? TRUE  : s->twainState <  3)
#define DS_IS_OPEN(s)     ((s == NULL) ? FALSE : s->twainState >= 4)
#define DS_IS_CLOSED(s)   ((s == NULL) ? TRUE  : s->twainState <  4)
#define DS_IS_ENABLED(s)  ((s == NULL) ? FALSE : s->twainState >= 5)
#define DS_IS_DISABLED(s) ((s == NULL) ? TRUE  : s->twainState <  5)

#define DSM_GET_STATUS(ses, sta)  callDSM(ses->appIdentity, ses->dsIdentity, \
          DG_CONTROL, DAT_STATUS, MSG_GET, (TW_MEMREF) sta)

#define DSM_OPEN(ses) callDSM(ses->appIdentity, NULL,\
          DG_CONTROL, DAT_PARENT, MSG_OPENDSM, (TW_MEMREF) &(ses->hwnd))

#define DSM_CLOSE(ses) callDSM(ses->appIdentity, NULL,\
          DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, (TW_MEMREF)&(ses->hwnd))

#define DSM_SET_CALLBACK(ses, cb) callDSM(ses->appIdentity, NULL,\
          DG_CONTROL, DAT_CALLBACK, MSG_REGISTER_CALLBACK, (TW_MEMREF) &cb)

#define DSM_PROCESS_EVENT(ses, ev) callDSM(ses->appIdentity, ses->dsIdentity,\
          DG_CONTROL, DAT_EVENT, MSG_PROCESSEVENT, (TW_MEMREF) &ev)

#define DSM_GET_PALETTE(ses, d) callDSM(ses->appIdentity, ses->dsIdentity,\
            DG_IMAGE, DAT_PALETTE8, MSG_GET, (TW_MEMREF) d->paletteData)

#define DSM_SELECT_USER(ses) callDSM(ses->appIdentity, NULL,\
          DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT, (TW_MEMREF) ses->dsIdentity)

#define DSM_SELECT_DEFAULT(ses) callDSM(ses->appIdentity, NULL,\
          DG_CONTROL, DAT_IDENTITY, MSG_GETDEFAULT, (TW_MEMREF) ses->dsIdentity)

#define DSM_OPEN_DS(ses) callDSM(ses->appIdentity, NULL,\
          DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, (TW_MEMREF) ses->dsIdentity)

#define DSM_CLOSE_DS(ses) callDSM(ses->appIdentity, NULL,\
          DG_CONTROL, DAT_IDENTITY, MSG_CLOSEDS, (TW_MEMREF) ses->dsIdentity)

#define DSM_ENABLE_DS(ses, ui) callDSM(ses->appIdentity, ses->dsIdentity,\
          DG_CONTROL, DAT_USERINTERFACE, MSG_ENABLEDS, (TW_MEMREF) &ui)

#define DSM_DISABLE_DS(ses, ui) callDSM(ses->appIdentity, ses->dsIdentity,\
          DG_CONTROL, DAT_USERINTERFACE, MSG_DISABLEDS, (TW_MEMREF) &ui)

#define DSM_GET_IMAGE(ses, i) callDSM(ses->appIdentity, ses->dsIdentity,\
          DG_IMAGE, DAT_IMAGEINFO, MSG_GET, (TW_MEMREF) i)

#define DSM_CAPABILITY_SET(ses, x) callDSM(ses->appIdentity, ses->dsIdentity,\
          DG_CONTROL, DAT_CAPABILITY, MSG_SET, (TW_MEMREF) &x)

#define DSM_XFER_START(ses, x) callDSM(ses->appIdentity, ses->dsIdentity,\
          DG_CONTROL, DAT_SETUPMEMXFER, MSG_GET, (TW_MEMREF) &x)

#define DSM_XFER_GET(ses, x) callDSM(ses->appIdentity, ses->dsIdentity,\
          DG_IMAGE, DAT_IMAGEMEMXFER, MSG_GET, (TW_MEMREF) &x)

#define DSM_XFER_STOP(ses, x) callDSM(ses->appIdentity, ses->dsIdentity,\
          DG_CONTROL, DAT_PENDINGXFERS, MSG_ENDXFER, (TW_MEMREF) &x)

#define DSM_XFER_RESET(ses, x) callDSM(ses->appIdentity, ses->dsIdentity,\
          DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, (TW_MEMREF) &x)

#define DSM_GET_FIRST_DS(ses, ds) callDSM (ses->appIdentity, NULL, \
          DG_CONTROL, DAT_IDENTITY, MSG_GETFIRST, (TW_MEMREF) ds);

#define DSM_GET_NEXT_DS(ses, ds) callDSM (ses->appIdentity, NULL, \
          DG_CONTROL, DAT_IDENTITY, MSG_GETNEXT, (TW_MEMREF) ds);

/* Function declarations */
char *          twainError (int errorCode);
char *          currentTwainError (pTW_SESSION twSession);
int             getImage (pTW_SESSION twSession);
int             loadTwainLibrary (pTW_SESSION twSession);
void            unloadTwainLibrary (pTW_SESSION twSession);
int             openDSM (pTW_SESSION twSession);
int             selectDS (pTW_SESSION twSession);
int             openDS (pTW_SESSION twSession);
int             requestImageAcquire (pTW_SESSION twSession, gboolean showUI);
void            disableDS (pTW_SESSION twSession);
void            closeDS (pTW_SESSION twSession);
void            closeDSM (pTW_SESSION twSession);
void            cancelPendingTransfers (pTW_SESSION twSession);
void            processTwainMessage (TW_UINT16 message, pTW_SESSION twSession);
pTW_SESSION     newSession (pTW_IDENTITY twSession);
pTW_DATA_SOURCE get_available_ds (pTW_SESSION twSession);
int             adjust_selected_data_source (pTW_SESSION twSession);
void            set_ds_capabilities (pTW_SESSION twSession);
void            set_ds_capability (pTW_SESSION twSession, TW_UINT16 cap, TW_UINT16 type, TW_UINT32 value);
#endif /* _TW_FUNC_H */
