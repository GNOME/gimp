/*  
 * TWAIN Plug-in
 * Copyright (C) 1999 Craig Setera
 * Craig Setera <setera@home.com>
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

#include <windows.h>
#include "twain.h"
#include "tw_func.h"
#include "tw_util.h"

/* The DLL to be loaded for TWAIN support */
#define TWAIN_DLL_NAME "TWAIN_32.DLL"

/*
 * Twain error code to string mappings
 */
static int twainErrorCount = 0;
static char *twainErrors[] = {
  "No error",
  "Failure due to unknown causes",
  "Not enough memory to perform operation",
  "No Data Source",
  "DS is connected to max possible apps",
  "DS or DSM reported error, application shouldn't",
  "Unknown capability",
  "Unrecognized MSG DG DAT combination",
  "Data parameter out of range",
  "DG DAT MSG out of expected sequence",
  "Unknown destination App/Src in DSM_Entry",
  "Capability not supported by source",
  "Operation not supported by capability",
  "Capability has dependency on other capability",
  "File System operation is denied (file is protected)",
  "Operation failed because file already exists.",
  "File not found",
  "Operation failed because directory is not empty",
  "The feeder is jammed",
  "The feeder detected multiple pages",
  "Error writing the file (disk full?)",
  "The device went offline prior to or during this operation",
  NULL
};

/* Storage for the DLL handle */
static HINSTANCE hDLL = NULL;

/* Storage for the entry point into the DSM */
static DSMENTRYPROC dsmEntryPoint = NULL;

/*
 * FloatToFix32
 *
 * Convert a floating point value into a FIX32.
 */
TW_FIX32 FloatToFIX32(float floater)
{
  TW_FIX32 Fix32_value;
  TW_INT32 value = (TW_INT32) (floater * 65536.0 + 0.5);
  Fix32_value.Whole = value >> 16;
  Fix32_value.Frac = value & 0x0000ffffL;
  return (Fix32_value);
}

/*
 * Fix32ToFloat
 *
 * Convert a FIX32 value into a floating point value.
 */
float FIX32ToFloat(TW_FIX32 fix32)
{
  float floater;
  floater = (float) fix32.Whole + (float) fix32.Frac / 65536.0;
  return floater;
}

/*
 * callDSM
 *
 * Call the specified function on the data source manager.
 */
TW_UINT16
callDSM(pTW_IDENTITY pOrigin,
	pTW_IDENTITY pDest,
	TW_UINT32    DG,
	TW_UINT16    DAT,
	TW_UINT16    MSG,
	TW_MEMREF    pData)
{
  /* Call the function */
  return (*dsmEntryPoint) (pOrigin, pDest, DG, DAT, MSG, pData);
}

/*
 * twainError
 * 
 * Return the TWAIN error message associated
 * with the specified error code.
 */
char *
twainError(int errorCode) 
{
  /* Check whether we've counted */
  if (twainErrorCount == 0)
    while (twainErrors[twainErrorCount++]) {}

  /* Check out of bounds */
  if (errorCode >= twainErrorCount)
    return "Unknown TWAIN Error Code";
  else
    return twainErrors[errorCode];
}

/*
 * currentTwainError
 *
 * Return the current TWAIN error message.
 */
char *
currentTwainError(pTW_SESSION twSession)
{
  TW_STATUS twStatus;

  /* Get the current status code from the DSM */
  twSession->twRC = callDSM(APP_IDENTITY(twSession), DS_IDENTITY(twSession),
			    DG_CONTROL, DAT_STATUS, MSG_GET,
			    (TW_MEMREF) &twStatus);

  /* Return the mapped error code */
  return twainError(twStatus.ConditionCode);
}

/*
 * twainIsAvailable
 *
 * Return boolean indicating whether TWAIN is available
 */
int
twainIsAvailable(void)
{
  /* Already loaded? */
  if (dsmEntryPoint) {
    return TRUE;
  }

  /* Attempt to load the library */
  hDLL = LoadLibrary(TWAIN_DLL_NAME);
  if (hDLL == NULL)
    return FALSE;

  /* Look up the entry point for use */
  dsmEntryPoint = (DSMENTRYPROC) GetProcAddress(hDLL, "DSM_Entry");
  if (dsmEntryPoint == NULL)
    return FALSE;

  return TRUE;
}

/*
 * getImage
 * 
 * This is a "high-level" function that can be called in order
 * to take all of the steps necessary to kick off an image-transfer
 * from a user-specified TWAIN datasource.  The data will be passed
 * back to the callback function specified in the session structure.
 */
int
getImage(pTW_SESSION twSession)
{
  /* Do some sanity checking first and bail
   * if necessary.
   */
  if (!twainIsAvailable()) {
    LogMessage("TWAIN is not available for image capture\n");
    return FALSE;
  }

  /* One step at a time */
  if (!openDSM(twSession)) {
    LogMessage("Unable to open data source manager\n");
    return FALSE;
  }

  if (!selectDS(twSession)) {
    LogMessage("Data source not selected\n");
    return FALSE;
  }

  if (!openDS(twSession)) {
    LogMessage("Unable to open datasource\n");
    return FALSE;
  }
       
  requestImageAcquire(twSession, TRUE);

  return TRUE;
}

/*
 * openDSM
 *
 * Open the data source manager
 */
int
openDSM(pTW_SESSION twSession)
{
  /* Make sure that we aren't already open */
  if (DSM_IS_OPEN(twSession))
    return TRUE;

  /* Open the data source manager */
  twSession->twRC = callDSM(APP_IDENTITY(twSession), NULL,
			    DG_CONTROL, DAT_PARENT, MSG_OPENDSM,
			    (TW_MEMREF) &(twSession->hwnd));

  /* Check the return code */
  switch (twSession->twRC) {
    case TWRC_SUCCESS:
      /* We are now at state 3 */
      twSession->twainState = 3;
      return TRUE;
      break;
	
    case TWRC_FAILURE:
    default:
      LogMessage("OpenDSM failure\n");
      break;
  }

  return FALSE;
}

/*
 * selectDS
 *
 * Select a datasource using the TWAIN user
 * interface.
 */
int
selectDS(pTW_SESSION twSession)
{
  /* The datasource manager must be open */
  if (DSM_IS_CLOSED(twSession)) {
    LogMessage("Can't select data source with closed source manager\n");
    return FALSE;
  }

  /* Ask TWAIN to present the source select dialog */
  twSession->twRC = callDSM(APP_IDENTITY(twSession), NULL,
			    DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT,
			    (TW_MEMREF) DS_IDENTITY(twSession));
  
  /* Check the return to determine what the user decided
   * to do.
   */
  switch (twSession->twRC) {
  case TWRC_SUCCESS:
    LogMessage("Data source %s selected\n", DS_IDENTITY(twSession)->ProductName);
    return TRUE;
    break;

  case TWRC_CANCEL:
    LogMessage("User cancelled TWAIN source selection\n");
    break;

  case TWRC_FAILURE:	        
  default:
    LogMessage("Error \"%s\" during TWAIN source selection\n", 
	       currentTwainError(twSession));
    break;
  }

  return FALSE;
}

/*
 * selectDefaultDS
 *
 * Select the default datasource.
 */
int
selectDefaultDS(pTW_SESSION twSession)
{
  /* The datasource manager must be open */
  if (DSM_IS_CLOSED(twSession)) {
    LogMessage("Can't select data source with closed source manager\n");
    return FALSE;
  }

  /* Ask TWAIN to present the source select dialog */
  twSession->twRC = callDSM(APP_IDENTITY(twSession), NULL,
			    DG_CONTROL, DAT_IDENTITY, MSG_GETDEFAULT,
			    (TW_MEMREF) DS_IDENTITY(twSession));

  /* Check the return code */
  return (twSession->twRC == TWRC_SUCCESS);
}

/*
 * openDS
 *
 * Open a data source using the TWAIN user interface.
 */
int
openDS(pTW_SESSION twSession)
{
  TW_IDENTITY *dsIdentity;

  /* The datasource manager must be open */
  if (DSM_IS_CLOSED(twSession)) {
    LogMessage("openDS: Cannot open data source... manager closed\n");
    return FALSE;
  }

  /* Is the data source already open? */
  if (DS_IS_OPEN(twSession)) {
    LogMessage("openDS: Data source already open\n");
    return TRUE;
  }

  /* Open the TWAIN datasource */
  dsIdentity = DS_IDENTITY(twSession);
  twSession->twRC = callDSM(APP_IDENTITY(twSession), NULL,
			    DG_CONTROL, DAT_IDENTITY, MSG_OPENDS,
			    (TW_MEMREF) dsIdentity);
  
  /* Check the return to determine what the user decided
   * to do.
	 */
  switch (twSession->twRC) {
  case TWRC_SUCCESS:
    /* We are now in TWAIN state 4 */
    twSession->twainState = 4;
    LogMessage("Data source %s opened\n", DS_IDENTITY(twSession)->ProductName);
    LogMessage("\tVersion.MajorNum = %d\n", dsIdentity->Version.MajorNum);
    LogMessage("\tVersion.MinorNum = %d\n", dsIdentity->Version.MinorNum);
    LogMessage("\tVersion.Info = %s\n", dsIdentity->Version.Info);
    LogMessage("\tProtocolMajor = %d\n", dsIdentity->ProtocolMajor);
    LogMessage("\tProtocolMinor = %d\n", dsIdentity->ProtocolMinor);
    LogMessage("\tManufacturer = %s\n", dsIdentity->Manufacturer);
    LogMessage("\tProductFamily = %s\n", dsIdentity->ProductFamily);
    return TRUE;
    break;

  default:
    LogMessage("Error \"%s\" opening data source\n", currentTwainError(twSession));
    break;
  }

  return FALSE;
}

/*
 * setBufferedXfer
 */
static int
setBufferedXfer(pTW_SESSION twSession)
{
  TW_CAPABILITY bufXfer;
  pTW_ONEVALUE pvalOneValue;

  /* Make sure the data source is open first */
  if (DS_IS_CLOSED(twSession))
    return FALSE;

  /* Create the capability information */
  bufXfer.Cap = ICAP_XFERMECH;
  bufXfer.ConType = TWON_ONEVALUE;
  bufXfer.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
  pvalOneValue = (pTW_ONEVALUE) GlobalLock(bufXfer.hContainer);
  pvalOneValue->ItemType = TWTY_UINT16;
  pvalOneValue->Item = TWSX_MEMORY;
  GlobalUnlock(bufXfer.hContainer);

  /* Make the call to the source manager */
  twSession->twRC = callDSM(APP_IDENTITY(twSession), DS_IDENTITY(twSession),
			    DG_CONTROL, DAT_CAPABILITY, MSG_SET,
			    (TW_MEMREF) &bufXfer);

  /* Free the container */
  GlobalFree(bufXfer.hContainer);

  /* Let the caller know what happened */
  return (twSession->twRC==TWRC_SUCCESS);
}

/*
 * requestImageAcquire
 *
 * Request that the acquire user interface
 * be displayed.  This may or may not cause
 * an image to actually be transferred.
 */
int
requestImageAcquire(pTW_SESSION twSession, BOOL showUI)
{
  /* Make sure in the correct state */
  if (DS_IS_CLOSED(twSession)) {
    LogMessage("Can't acquire image with closed datasource\n");
    return FALSE;
  }

  /* Set the transfer mode */
  if (setBufferedXfer(twSession)) {
    TW_USERINTERFACE ui;

    /* Set the UI information */
    ui.ShowUI = TRUE;
    ui.ModalUI = TRUE;
    ui.hParent = twSession->hwnd;

    /* Make the call to the source manager */
    twSession->twRC = callDSM(APP_IDENTITY(twSession), DS_IDENTITY(twSession),
			      DG_CONTROL, DAT_USERINTERFACE, MSG_ENABLEDS,
			      (TW_MEMREF) &ui);

    if (twSession->twRC == TWRC_SUCCESS) {
      /* We are now at a new twain state */
      twSession->twainState = 5;

      return TRUE;
    } else {
      LogMessage("Error during data source enable\n");
      return FALSE;
    }
  } else {
    LogMessage("Unable to set buffered transfer mode: %s\n", 
	       currentTwainError(twSession));
    return FALSE;
  }
}

/*
 * disableDS
 *
 * Disable the datasource associated with twSession.
 */
int
disableDS(pTW_SESSION twSession)
{
    TW_USERINTERFACE ui;

	/* Verify the datasource is enabled */
	if (DS_IS_DISABLED(twSession)) {
		LogMessage("disableDS: Data source not enabled\n");
		return TRUE;
	}

    /* Set the UI information */
    ui.ShowUI = TRUE;
    ui.ModalUI = TRUE;
    ui.hParent = twSession->hwnd;

    /* Make the call to the source manager */
    twSession->twRC = callDSM(APP_IDENTITY(twSession), DS_IDENTITY(twSession),
			      DG_CONTROL, DAT_USERINTERFACE, MSG_DISABLEDS,
			      (TW_MEMREF) &ui);

    if (twSession->twRC == TWRC_SUCCESS) {
      /* We are now at a new twain state */
      twSession->twainState = 4;

      return TRUE;
    } else {
      LogMessage("Error during data source disable\n");
      return FALSE;
    }
}

/*
 * closeDS
 *
 * Close the datasource associated with the 
 * specified session.
 */
int
closeDS(pTW_SESSION twSession)
{
  /* Can't close a closed data source */
  if (DS_IS_CLOSED(twSession)) {
    LogMessage("closeDS: Data source already closed\n");
    return TRUE;
  }

  /* Open the TWAIN datasource */
  twSession->twRC = callDSM(APP_IDENTITY(twSession), NULL,
			    DG_CONTROL, DAT_IDENTITY, MSG_CLOSEDS,
			    (TW_MEMREF) DS_IDENTITY(twSession));
  
  /* Check the return to determine what the user decided
   * to do.
	 */
  switch (twSession->twRC) {
  case TWRC_SUCCESS:
    /* We are now in TWAIN state 3 */
    twSession->twainState = 3;
    LogMessage("Data source %s closed\n", DS_IDENTITY(twSession)->ProductName);
    return TRUE;
    break;

  default:
    LogMessage("Error \"%s\" closing data source\n", currentTwainError(twSession));
    break;
  }

  return FALSE;
}

/*
 * closeDSM
 *
 * Close the data source manager
 */
int
closeDSM(pTW_SESSION twSession)
{
  if (DSM_IS_CLOSED(twSession)) {
    LogMessage("closeDSM: Source Manager not open\n");
    return FALSE;
  } else {
    if (DS_IS_OPEN(twSession)) {
      LogMessage("closeDSM: Can't close source manager with open source\n");
      return FALSE;
    } else {
      twSession->twRC = callDSM(APP_IDENTITY(twSession), NULL,
				DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM,
				&(twSession->hwnd));
				
      if (twSession->twRC != TWRC_SUCCESS) {
				LogMessage("CloseDSM failure -- %s\n", currentTwainError(twSession));
      }
      else {

				/* We are now in state 2 */
				twSession->twainState = 2;
      }
    }
  }

  /* Let the caller know what happened */
  return (twSession->twRC==TWRC_SUCCESS);
}

/*
 * unloadTwainLibrary
 *
 * Unload the TWAIN dynamic link library
 */
int
unloadTwainLibrary(pTW_SESSION twSession)
{
  /* Explicitly free the SM library */
  if (hDLL) {        
    FreeLibrary(hDLL);
    hDLL=NULL;
  }

  /* the data source id will no longer be valid after
   * twain is killed.  If the id is left around the
   * data source can not be found or opened
	 */
  DS_IDENTITY(twSession)->Id = 0;  

	/* We are now back at state 1 */
  twSession->twainState = 1;
  LogMessage("Source Manager successfully closed\n");

  return TRUE;
}

/*
 * beginImageTransfer
 *
 * Begin an image transfer.
 */
static int
beginImageTransfer(pTW_SESSION twSession, pTW_IMAGEINFO imageInfo)
{
  /* Clear our structures */
  memset(imageInfo, 0, sizeof(TW_IMAGEINFO));

  /* Query the image information */
  twSession->twRC = callDSM(
			    APP_IDENTITY(twSession), DS_IDENTITY(twSession),
			    DG_IMAGE, DAT_IMAGEINFO, MSG_GET,
			    (TW_MEMREF) imageInfo);

  /* Check the return code */
  if (twSession->twRC != TWRC_SUCCESS) {
    LogMessage("Get Image Info failure - %s\n", currentTwainError(twSession));
    
    return FALSE;
  }

  /* Call the begin transfer callback if registered */
  if (twSession->transferFunctions->txfrBeginCb)
    if (!(*twSession->transferFunctions->txfrBeginCb)(imageInfo, twSession->clientData))
      return FALSE;

  /* We should continue */
  return TRUE;
}

/*
 * transferImage
 *
 * The Source indicated it is ready to transfer data. It is 
 * waiting for the application to inquire about the image details, 
 * initiate the actual transfer, and, hence, transition the session 
 * from State 6 to 7.  Return the reason for exiting the transfer.
 */
static void
transferImage(pTW_SESSION twSession, pTW_IMAGEINFO imageInfo)
{
  TW_SETUPMEMXFER setupMemXfer;
  TW_IMAGEMEMXFER imageMemXfer;
  char *buffer;
	
  /* Clear our structures */
  memset(&setupMemXfer, 0, sizeof(TW_SETUPMEMXFER));
  memset(&imageMemXfer, 0, sizeof(TW_IMAGEMEMXFER));
	
  /* Find out how the source would like to transfer... */
  twSession->twRC = callDSM(APP_IDENTITY(twSession), DS_IDENTITY(twSession),
			    DG_CONTROL, DAT_SETUPMEMXFER, MSG_GET,
			    (TW_MEMREF) &setupMemXfer);
	
  /* Allocate the buffer for the transfer */
  buffer = g_new (char, setupMemXfer.Preferred);
  imageMemXfer.Memory.Flags = TWMF_APPOWNS | TWMF_POINTER;
  imageMemXfer.Memory.Length = setupMemXfer.Preferred;
  imageMemXfer.Memory.TheMem = (TW_MEMREF) buffer;
	
  /* Get the data */
  do {
    /* Setup for the memory transfer */
    imageMemXfer.Compression = TWON_DONTCARE16;
    imageMemXfer.BytesPerRow = TWON_DONTCARE32;
    imageMemXfer.Columns = TWON_DONTCARE32;
    imageMemXfer.Rows = TWON_DONTCARE32;
    imageMemXfer.XOffset = TWON_DONTCARE32;
    imageMemXfer.YOffset = TWON_DONTCARE32;
    imageMemXfer.BytesWritten = TWON_DONTCARE32;
		
    /* Get the next block of memory */
    twSession->twRC = callDSM(APP_IDENTITY(twSession), DS_IDENTITY(twSession),
			      DG_IMAGE, DAT_IMAGEMEMXFER, MSG_GET,
			      (TW_MEMREF) &imageMemXfer);
		
    if ((twSession->twRC == TWRC_SUCCESS) ||
	(twSession->twRC == TWRC_XFERDONE)) {
      /* Call the callback function */
      if (!(*twSession->transferFunctions->txfrDataCb) (
							imageInfo, 
							&imageMemXfer,	
							twSession->clientData)) {
	/* Callback function requested to cancel */
	twSession->twRC = TWRC_CANCEL;
	break;
      }
    }
  } while (twSession->twRC == TWRC_SUCCESS);

  /* Free the memory buffer */
  g_free (imageMemXfer.Memory.TheMem);
}

/*
 * endPendingTransfer
 *
 * Cancel the currently pending transfer.
 * Return the count of pending transfers.
 */
static int
endPendingTransfer(pTW_SESSION twSession)
{
  TW_PENDINGXFERS pendingXfers;

  twSession->twRC = callDSM(APP_IDENTITY(twSession), DS_IDENTITY(twSession),
			    DG_CONTROL, DAT_PENDINGXFERS, MSG_ENDXFER,
			    (TW_MEMREF) &pendingXfers);

  if (!pendingXfers.Count)
    twSession->twainState = 5;

  return pendingXfers.Count;
}

/*
 * cancelPendingTransfers
 *
 * Cancel all pending image transfers.
 */
void
cancelPendingTransfers(pTW_SESSION twSession)
{
  TW_PENDINGXFERS pendingXfers;

  twSession->twRC = callDSM(APP_IDENTITY(twSession), DS_IDENTITY(twSession),
			      DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET,
			      (TW_MEMREF) &pendingXfers);
}

/*
 * endImageTransfer
 *
 * Finish transferring an image.  Return the count
 * of pending images.
 */
static int
endImageTransfer(pTW_SESSION twSession, int *pendingCount)
{
  BOOL continueTransfers;
  int exitCode = twSession->twRC;

  /* Have now exited the transfer for some reason... Figure out
   * why and what to do about it
   */
  switch (twSession->twRC) {
  case TWRC_XFERDONE:
  case TWRC_CANCEL:
    LogMessage("Xfer done received\n");
    *pendingCount = endPendingTransfer(twSession);
    break;

  case TWRC_FAILURE:
    LogMessage("Failure received\n");
    *pendingCount = endPendingTransfer(twSession);
    break;
  }

  /* Call the end transfer callback */
  if (twSession->transferFunctions->txfrEndCb)
    continueTransfers = 
      (*twSession->transferFunctions->txfrEndCb)(exitCode, 
						 *pendingCount, 
						 twSession->clientData);

  return (*pendingCount && continueTransfers);
}

/*
 * transferImages
 *
 * Transfer all of the images that are available from the
 * datasource.
 */
static void
transferImages(pTW_SESSION twSession)
{
  TW_IMAGEINFO imageInfo;
  int pendingCount;

  /* Check the image transfer callback function
   * before even attempting to do the transfer
   */
  if (!twSession->transferFunctions || !twSession->transferFunctions->txfrDataCb) {
    LogMessage("Attempting image transfer without callback function\n");
    return;
  }

  /*
   * Inform our application that we are getting ready
   * to transfer images.
   */
  if (twSession->transferFunctions->preTxfrCb)
    (*twSession->transferFunctions->preTxfrCb)(twSession->clientData);

  /* Loop through the available images */
  do {
    /* Move to the new state */
    twSession->twainState = 6;
		
    /* Begin the image transfer */
    if (!beginImageTransfer(twSession, &imageInfo))
      continue;
		
    /* Call the image transfer function */
    transferImage(twSession, &imageInfo);
		
  } while (endImageTransfer(twSession, &pendingCount));

  /*
   * Inform our application that we are done
   * transferring images.
   */
  if (twSession->transferFunctions->postTxfrCb)
    (*twSession->transferFunctions->postTxfrCb)(pendingCount,
						twSession->clientData);
}

/*
 * TwainProcessMessage
 *
 * Returns TRUE if the application should process message as usual.
 * Returns FALSE if the application should skip processing of this message
 */
int
TwainProcessMessage(LPMSG lpMsg, pTW_SESSION twSession)
{
  TW_EVENT   twEvent;
	
  twSession->twRC = TWRC_NOTDSEVENT;
	
  /* Only ask Source Manager to process event if there is a Source connected. */
  if (DSM_IS_OPEN(twSession) && DS_IS_OPEN(twSession)) {
		/*
		 * A Source provides a modeless dialog box as its user interface.
		 * The following call relays Windows messages down to the Source's
		 * UI that were intended for its dialog box.  It also retrieves TWAIN
		 * messages sent from the Source to our Application.
		 */
		twEvent.pEvent = (TW_MEMREF) lpMsg;
		twSession->twRC = callDSM(APP_IDENTITY(twSession), DS_IDENTITY(twSession),
			DG_CONTROL, DAT_EVENT, MSG_PROCESSEVENT, 
			(TW_MEMREF) &twEvent);
		
		/* Check the return code */
		if (twSession->twRC == TWRC_NOTDSEVENT) {
			return FALSE;
		}
		
		/* Process the message as necessary */
		switch (twEvent.TWMessage) {
		case MSG_XFERREADY:
			LogMessage("Source says that data is ready\n");
			transferImages(twSession);
			break;
			
		case MSG_CLOSEDSREQ:
			/* Disable the datasource, Close the Data source
			 * and close the data source manager
			 */
			LogMessage("CloseDSReq\n");
			disableDS(twSession);
			closeDS(twSession);
			closeDSM(twSession);
			break;
			
			/* No message from the Source to the App break;
			 * possible new message
			 */
		case MSG_NULL:
		default:
			break;
		}   
  } 
	
  /* tell the caller what happened */
  return (twSession->twRC == TWRC_DSEVENT);
} 

/*
 * twainMessageLoop
 *
 * Process Win32 window messages and provide special handling
 * of TWAIN specific messages.  This loop will not exit until
 * the application exits.
 */
int
twainMessageLoop(pTW_SESSION twSession) 
{
  MSG msg;
	
  while (GetMessage(&msg, NULL, 0, 0)) {
    if (DS_IS_CLOSED(twSession) || !TwainProcessMessage(&msg, twSession)) {
      TranslateMessage((LPMSG)&msg);
      DispatchMessage(&msg);
    }
  }
	
  return msg.wParam;
}

/**********************************************************************
 * Session related functions
 **********************************************************************/

/*
 * newSession
 *
 * Create a new TWAIN session.
 */
pTW_SESSION
newSession(pTW_IDENTITY appIdentity) {
  /* Create the structure */
  pTW_SESSION session = g_new (TW_SESSION, 1);

  /* Set the structure fields */
  session->hwnd = 0;
  session->twRC = TWRC_SUCCESS;
  session->appIdentity = appIdentity;
  session->dsIdentity = g_new (TW_IDENTITY, 1);
  session->dsIdentity->Id = 0;
  session->dsIdentity->ProductName[0] = '\0';
  session->transferFunctions = NULL;

  if (twainIsAvailable())
    session->twainState = 2;
  else
    session->twainState = 0;

  return session;
}

/*
 * registerWindowHandle
 *
 * Register the window handle to be used for this
 * session.
 */
void
registerWindowHandle(pTW_SESSION session, HWND hwnd)
{
  session->hwnd = hwnd;
}

/*
 * registerTransferCallback
 *
 * Register the callback to use when transferring
 * image data.
 */
void 
registerTransferCallbacks(pTW_SESSION session, 
			  pTXFR_CB_FUNCS txfrFuncs, 
			  void *clientData)
{
  session->transferFunctions = txfrFuncs;
  session->clientData = clientData;
}

/*
 * setClientData
 *
 * Set the client data associated with the specified
 * TWAIN session.
 */
void
setClientData(pTW_SESSION session, void *clientData)
{
  session->clientData = clientData;
}
