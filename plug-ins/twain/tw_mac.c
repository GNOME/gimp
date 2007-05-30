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

/*
 * Platform-specific code for Mac OS X
 */

#include "config.h"

#include <libgimp/gimp.h>

#include "tw_platform.h"
#include "tw_func.h"
#include "tw_util.h"
#include "tw_local.h"

TW_UINT16 twainCallback(pTW_IDENTITY pOrigin,
                         pTW_IDENTITY pDest,
                         TW_UINT32    DG,
                         TW_UINT16    DAT,
                         TW_UINT16    MSG,
                         TW_MEMREF    pData);
extern pTW_SESSION initializeTwain ();


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
  return DSM_Entry (pOrigin, pDest, DG, DAT, MSG, pData);
}

/*
 * twainIsAvailable
 *
 * Return boolean indicating whether TWAIN is available
 */
int
twainIsAvailable(void)
{
  return TRUE;
}

TW_HANDLE
twainAllocHandle(size_t size)
{
	return NewHandle(size);
}

TW_MEMREF
twainLockHandle (TW_HANDLE handle)
{
  return *handle;
}

void
twainUnlockHandle (TW_HANDLE handle)
{
  /* NOP */
}

void
twainFreeHandle (TW_HANDLE handle)
{
  DisposeHandle (handle);
}

gboolean
twainSetupCallback (pTW_SESSION twSession)
{
  TW_CALLBACK callback;
  /* We need to set up our callback to receive messages */
  callback.CallBackProc = (TW_MEMREF)twainCallback;
  callback.RefCon = (TW_UINT32)twSession;
  callback.Message = 0;
  twSession->twRC = callDSM(APP_IDENTITY(twSession), NULL,
                            DG_CONTROL, DAT_CALLBACK, MSG_REGISTER_CALLBACK,
                            (TW_MEMREF) &callback);
  if (twSession->twRC != TWRC_SUCCESS) {
    LogMessage("Couldn't set callback function (retval %d)", (int)(twSession->twRC));
    return FALSE;
  }
  return TRUE;
}

/*
 * unloadTwainLibrary
 *
 * Unload the TWAIN dynamic link library
 */
int
unloadTwainLibrary(pTW_SESSION twSession)
{
  /* We're statically linked; just clear the session */
  DS_IDENTITY(twSession)->Id = 0;

	/* We are now back at state 1 */
  twSession->twainState = 1;
  LogMessage("Source Manager successfully closed\n");

  return TRUE;
}

TW_UINT16 twainCallback(pTW_IDENTITY pOrigin,
                        pTW_IDENTITY pDest,
                        TW_UINT32    DG,
                        TW_UINT16    DAT,
                        TW_UINT16    MSG,
                        TW_MEMREF    pData)
{
  pTW_SESSION twSession = (pTW_SESSION)(((TW_CALLBACK *)pData)->RefCon);
  LogMessage("twainCallback! DG 0x%x, DAT 0x%x, MSG 0x%x\n",
    (int)DG, (int)DAT, (int)MSG);
  processTwainMessage (MSG, twSession);
  return TWRC_SUCCESS;
}

void twainQuitApplication ()
{
  QuitApplicationEventLoop();
}


/* main bits */

/* http://lists.wxwidgets.org/archive/wxPython-mac/msg00117.html */
extern OSErr CPSSetProcessName (ProcessSerialNumber *psn, char *processname);
extern OSErr CPSEnableForegroundOperation( ProcessSerialNumber *psn, UInt32 _arg2, UInt32 _arg3, UInt32 _arg4, UInt32 _arg5);

static void
doGetImage (EventLoopTimerRef inTimer, void *inUserData)
{
  if (! scanImage ())
    QuitApplicationEventLoop ();
}

static void twainSetupMacUI()
{
  ProcessSerialNumber psn;
  GetCurrentProcess (&psn);
  CPSSetProcessName (&psn, "GIMP TWAIN");

  /* Need to do some magic here to get the UI to work */
  CPSEnableForegroundOperation (&psn, 0x03, 0x3C, 0x2C, 0x1103);
  SetFrontProcess (&psn);

  /* We end up with the ugly console dock icon; let's override it */
  gchar *iconfile = g_build_filename (gimp_data_directory (),
                                      "twain",
                                      "gimp-twain.png",
                                      NULL);
  CFURLRef url = CFURLCreateFromFileSystemRepresentation (kCFAllocatorDefault,
                                                          iconfile,
                                                          strlen (iconfile),
                                                          FALSE);
  g_free (iconfile);

  CGDataProviderRef png = CGDataProviderCreateWithURL (url);
  CGImageRef icon = CGImageCreateWithPNGDataProvider (png, NULL, TRUE,
                                             kCGRenderingIntentDefault);

  /* Voodoo magic fix inspired by java_swt launcher */
  /* Without this the icon setting doesn't work about half the time. */
  CGrafPtr p = BeginQDContextForApplicationDockTile();
  EndQDContextForApplicationDockTile(p);

  SetApplicationDockTileImage (icon);
}

int
twainMain()
{
  EventLoopTimerRef timer;
  OSStatus err;

  twainSetupMacUI();

  /* Ok, back to work! */
  initializeTwain ();

  /* Set this up to run once the event loop is started */
  err = InstallEventLoopTimer (GetMainEventLoop (),
                               0, 0, /* Immediately, once only */
                               NewEventLoopTimerUPP (doGetImage),
                               NULL, &timer);
  if (err)
    {
      g_warning ("InstallEventLoopTimer returned error %d", (int)err);
      g_message ("Something went horribly awry.");
      return -1;
    }

  RunApplicationEventLoop();
  return 0;
}

