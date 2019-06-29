/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Screenshot plug-in
 * Copyright 1998-2007 Sven Neumann <sven@gimp.org>
 * Copyright 2003      Henrik Brix Andersen <brix@gimp.org>
 * Copyright 2012      Simone Karin Lehmann - OS X patches
 * Copyright 2018      Gil Eliyahu <gileli121@gmail.com>
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
 */

#include "config.h"

#include <glib.h>

#ifdef G_OS_WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Necessary in order to have SetProcessDPIAware() defined.
 * This value of _WIN32_WINNT corresponds to Windows 7, which is our
 * minimum supported platform.
 */
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0601
#include <windows.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "screenshot.h"
#include "screenshot-win32.h"
#include "screenshot-win32-resource.h"

#include "libgimp/stdplugins-intl.h"
#include "screenshot-win32-magnification-api.h"
#include "screenshot-win32-dwm-api.h"

/*
 * Application definitions
 */
#define SELECT_FRAME    0
#define SELECT_CLIENT   1
#define SELECT_WINDOW   2

#define SHOW_WINDOW     FALSE
#define APP_NAME        "plug_in_screenshot_win"
#define WM_DOCAPTURE    (WM_USER + 100)

/* Prototypes */
void setCaptureType  (int       capType);
BOOL InitApplication (HINSTANCE hInstance);
BOOL InitInstance    (HINSTANCE hInstance,
                      int       nCmdShow);
int  winsnapWinMain  (void);

/* File variables */
static int             captureType;
static char            buffer[512];
static guchar         *capBytes = NULL;
static HWND            mainHwnd = NULL;
static HINSTANCE       hInst = NULL;
static HCURSOR         selectCursor = 0;
static ICONINFO        iconInfo;
static MAGIMAGEHEADER  returnedSrcheader;
static int             rectScreensCount = 0;

static gint32    *image_id;

static void sendBMPToGimp                      (HBITMAP         hBMP,
                                                HDC             hDC,
                                                RECT            rect);
static int      doWindowCapture                    (void);
static gboolean doCapture                          (HWND            selectedHwnd);
static BOOL     isWindowIsAboveCaptureRegion       (HWND            hwndWindow,
                                                    RECT            rectCapture);
static gboolean doCaptureMagnificationAPI          (HWND            selectedHwnd,
                                                    RECT            rect);
static void     doCaptureMagnificationAPI_callback (HWND            hwnd,
                                                    void           *srcdata,
                                                    MAGIMAGEHEADER  srcheader,
                                                    void           *destdata,
                                                    MAGIMAGEHEADER  destheader,
                                                    RECT            unclipped,
                                                    RECT            clipped,
                                                    HRGN            dirty);
static gboolean doCaptureBitBlt                    (HWND            selectedHwnd,
                                                    RECT            rect);

BOOL CALLBACK dialogProc (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

/* Data structure holding data between runs */
typedef struct {
  gint root;
  guint delay;
  gint decor;
} WinSnapValues;

/* Default WinSnap values */
static WinSnapValues winsnapvals =
{
  FALSE,
  0,
  TRUE,
};

/* The dialog information */
typedef struct {
#ifdef CAN_SET_DECOR
  GtkWidget *decor_button;
#endif
  GtkWidget *single_button;
  GtkWidget *root_button;
  GtkWidget *delay_spinner;
} WinSnapInterface;

/* We create a DIB section to hold the grabbed area. The scanlines in
 * DIB sections are aligned on a LONG (four byte) boundary. Its pixel
 * data is in RGB (BGR actually) format, three bytes per pixel.
 *
 * GIMP uses no alignment for its pixel regions. The GIMP image we
 * create is of type RGB, i.e. three bytes per pixel, too. Thus in
 * order to be able to quickly transfer all of the image at a time, we
 * must use a DIB section and pixel region the scanline width in
 * bytes of which is evenly divisible with both 3 and 4. I.e. it must
 * be a multiple of 12 bytes, or in pixels, a multiple of four pixels.
 */

#define ROUND4(width) (((width-1)/4+1)*4)


gboolean
screenshot_win32_available (void)
{
  return TRUE;
}

ScreenshotCapabilities
screenshot_win32_get_capabilities (void)
{
  return (SCREENSHOT_CAN_SHOOT_DECORATIONS |
          SCREENSHOT_CAN_SHOOT_WINDOW);
}

GimpPDBStatusType
screenshot_win32_shoot (ScreenshotValues  *shootvals,
                        GdkScreen         *screen,
                        gint32            *image_ID,
                        GError           **error)
{
  GimpPDBStatusType status = GIMP_PDB_EXECUTION_ERROR;

  /* leave "shootvals->monitor" alone until somebody patches the code
   * to be able to get a monitor's color profile
   */

  image_id = image_ID;

  winsnapvals.delay = shootvals->screenshot_delay;

  if (shootvals->shoot_type == SHOOT_ROOT)
    {
      doCapture (0);

      status = GIMP_PDB_SUCCESS;
    }
  else if (shootvals->shoot_type == SHOOT_WINDOW)
    {
      status = 0 == doWindowCapture () ? GIMP_PDB_CANCEL : GIMP_PDB_SUCCESS;
    }
  else if (shootvals->shoot_type == SHOOT_REGION)
    {
      /* FIXME */
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpColorProfile *profile;

      /* XXX No idea if the "monitor" value is right at all, especially
       * considering above comment. Just make so that it at least
       * compiles!
       */
      profile = gimp_screen_get_color_profile (screen, shootvals->monitor);

      if (profile)
        {
          gimp_image_set_color_profile (*image_ID, profile);
          g_object_unref (profile);
        }
    }

  return status;
}





/******************************************************************
 * GIMP format and transfer functions
 ******************************************************************/

/*
 * flipRedAndBlueBytes
 *
 * Microsoft has chosen to provide us a very nice (not!)
 * interface for retrieving bitmap bits.  DIBSections have
 * RGB information as BGR instead.  So, we have to swap
 * the silly red and blue bytes before sending to the
 * GIMP.
 */
static void
flipRedAndBlueBytes (int width,
                     int height)
{
  int      i, j;
  guchar  *bufp;
  guchar   temp;

  j = 0;
  while (j < height) {
    i = width;
    bufp = capBytes + j*ROUND4(width)*3;
    while (i--) {
      temp = bufp[2];
      bufp[2] = bufp[0];
      bufp[0] = temp;
      bufp += 3;
    }
    j++;
  }
}

/*
 * rgbaToRgbBytes
 *
 * Convert rgba array to rgb array
 */
static void
rgbaToRgbBytes (guchar *rgbBufp,
                guchar *rgbaBufp,
                int     rgbaBufSize)
{
  int rgbPoint = 0, rgbaPoint;

  for (rgbaPoint = 0; rgbaPoint < rgbaBufSize; rgbaPoint += 4)
    {
      rgbBufp[rgbPoint++] = rgbaBufp[rgbaPoint];
      rgbBufp[rgbPoint++] = rgbaBufp[rgbaPoint + 1];
      rgbBufp[rgbPoint++] = rgbaBufp[rgbaPoint + 2];
    }
}

/*
 * sendBMPToGIMP
 *
 * Take the captured data and send it across
 * to GIMP.
 */
static void
sendBMPToGimp (HBITMAP hBMP,
               HDC     hDC,
               RECT    rect)
{
  int            width, height;
  int            imageType, layerType;
  gint32         new_image_id;
  gint32         layer_id;
  GeglBuffer    *buffer;
  GeglRectangle *rectangle;

  /* Our width and height */
  width = (rect.right - rect.left);
  height = (rect.bottom - rect.top);

  /* Check that we got the memory */
  if (!capBytes)
    {
      g_message (_("No data captured"));
      return;
    }

  /* Flip the red and blue bytes */
  flipRedAndBlueBytes (width, height);

  /* Set up the image and layer types */
  imageType = GIMP_RGB;
  layerType = GIMP_RGB_IMAGE;

  /* Create the GIMP image and layers */
  new_image_id = gimp_image_new (width, height, imageType);
  layer_id = gimp_layer_new (new_image_id, _("Background"),
                             ROUND4 (width), height,
                             layerType,
                             100,
                             gimp_image_get_default_new_layer_mode (new_image_id));
  gimp_image_insert_layer (new_image_id, layer_id, -1, 0);

  /* make rectangle */
  rectangle = g_new (GeglRectangle, 1);
  rectangle->x = 0;
  rectangle->y = 0;
  rectangle->width = ROUND4(width);
  rectangle->height = height;

  /* get the buffer */
  buffer = gimp_drawable_get_buffer (layer_id);

  /* fill the buffer */
  gegl_buffer_set (buffer, rectangle, 0, NULL, (guchar *) capBytes,
                   GEGL_AUTO_ROWSTRIDE);

  /* flushing data */
  gegl_buffer_flush (buffer);

  /* Now resize the layer down to the correct size if necessary. */
  if (width != ROUND4 (width))
    {
      gimp_layer_resize (layer_id, width, height, 0, 0);
      gimp_image_resize (new_image_id, width, height, 0, 0);
    }

  *image_id = new_image_id;

  return;
}

/*
 * doNonRootWindowCapture
 *
 * Capture a selected window.
 * ENTRY POINT FOR WINSNAP NONROOT
 *
 */
static int
doWindowCapture (void)
{
  /* Start up a standard Win32
   * message handling loop for
   * selection of the window
   * to be captured
   */
  return winsnapWinMain ();
}

/******************************************************************
 * Debug stuff
 ******************************************************************/

/*
 * formatWindowsError
 *
 * Format the latest Windows error message into
 * a readable string.  Store in the provided
 * buffer.
 */
static void
formatWindowsError (char *buffer,
                    int   buf_size)
{
  LPVOID lpMsgBuf;

  /* Format the message */
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPTSTR) &lpMsgBuf, 0, NULL );

  /* Copy to the buffer */
  strncpy(buffer, lpMsgBuf, buf_size - 1);

  LocalFree(lpMsgBuf);
}

/******************************************************************
 * Bitmap capture and handling
 ******************************************************************/

/*
 * primDoWindowCapture
 *
 * The primitive window capture functionality.  Accepts
 * the two device contexts and the rectangle to be
 * captured.
 */
static HBITMAP
primDoWindowCapture (HDC  hdcWindow,
                     HDC  hdcCompat,
                     RECT rect)
{
  HBITMAP hbmCopy;
  HGDIOBJ oldObject;
  BITMAPINFO  bmi;

  int width = (rect.right - rect.left);
  int height = (rect.bottom - rect.top);

  /* Create the bitmap info header */
  bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = ROUND4(width);
  bmi.bmiHeader.biHeight = -height;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 24;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = 0;
  bmi.bmiHeader.biXPelsPerMeter =
  bmi.bmiHeader.biYPelsPerMeter = 0;
  bmi.bmiHeader.biClrUsed = 0;
  bmi.bmiHeader.biClrImportant = 0;

  /* Create the bitmap storage space */
  hbmCopy = CreateDIBSection (hdcCompat,
                              (BITMAPINFO *) &bmi,
                              DIB_RGB_COLORS,
                              (void **)&capBytes, NULL, 0);
  if (!hbmCopy)
    {
      formatWindowsError(buffer, sizeof buffer);
      g_error("Error creating DIB section: %s", buffer);
      return NULL;
    }

  /* Select the bitmap into the compatible DC. */
  oldObject = SelectObject (hdcCompat, hbmCopy);
  if (!oldObject)
    {
      formatWindowsError (buffer, sizeof buffer);
      g_error ("Error selecting object: %s", buffer);
      return NULL;
    }

  /* Copy the data from the application to the bitmap.  Even if we did
   * round up the width, BitBlt only the actual data.
   */
  if (!BitBlt(hdcCompat, 0,0,
              width, height,
              hdcWindow, rect.left, rect.top,
              SRCCOPY))
    {
      formatWindowsError (buffer, sizeof buffer);
      g_error ("Error copying bitmap: %s", buffer);
      return NULL;
    }

  /* Restore the original object */
  SelectObject (hdcCompat, oldObject);

  return hbmCopy;
}

static BOOL CALLBACK
GetAccurateWindowRect_MonitorEnumProc (HMONITOR hMonitor,
                                       HDC      hdcMonitor,
                                       LPRECT   lprcMonitor,
                                       LPARAM   dwData)
{
  RECT **rectScreens = (RECT**) dwData;

  if (!lprcMonitor) return FALSE;

  if (! (*rectScreens))
    *rectScreens = (RECT*) g_malloc (sizeof (RECT)*(rectScreensCount+1));
  else
    *rectScreens = (RECT*) g_realloc (rectScreens,
                                      sizeof (RECT)*(rectScreensCount+1));

  if (*rectScreens == NULL)
    {
      rectScreensCount = 0;
      return FALSE;
    }

  (*rectScreens)[rectScreensCount] = *lprcMonitor;

  rectScreensCount++;

  return TRUE;
}

static BOOL
GetAccurateWindowRect (HWND hwndTarget,
                       RECT *outRect)
{
  HRESULT dwmResult;
  WINDOWPLACEMENT windowplacment;

  ZeroMemory (outRect, sizeof (RECT));

  /* First we try to get the rect using the dwm api. it will also avoid us from doing more ugly fixes when the window is maximized */
  if (LoadRequiredDwmFunctions ())
    {
      dwmResult = DwmGetWindowAttribute (hwndTarget, DWMWA_EXTENDED_FRAME_BOUNDS, (PVOID) outRect, sizeof (RECT));
      UnloadRequiredDwmFunctions ();
      if (dwmResult == S_OK) return TRUE;
    }

  /* In this case, we did not got the rect from the dwm api so we try to get the rect with the normal function */
  if (GetWindowRect (hwndTarget, outRect))
    {
      /* If the window is maximized then we need and can fix the rect variable 
       * (we need to do this if the rect isn't coming from dwm api) 
       */
      ZeroMemory (&windowplacment, sizeof (WINDOWPLACEMENT));
      if (GetWindowPlacement (hwndTarget, &windowplacment) && windowplacment.showCmd == SW_SHOWMAXIMIZED)
        {
          RECT *rectScreens = NULL;

          /* If this is not the first time we call this function for some
           * reason then we reset the rectScreens count
           */
          if (rectScreensCount)
            rectScreensCount = 0;

          /* Get the screens rects */
          EnumDisplayMonitors (NULL, NULL, GetAccurateWindowRect_MonitorEnumProc,
                               (LPARAM) &rectScreens);

          /* If for some reason the array size is 0 then we fill it with the desktop rect */
          if (! rectScreensCount)
            {
              rectScreens = (RECT*) g_malloc (sizeof (RECT));
              if (! GetWindowRect (GetDesktopWindow (), rectScreens))
                {
                  /* error: could not get rect screens */
                  g_free (rectScreens);
                  return FALSE;
                }

              rectScreensCount = 1;
            }

          int xCenter = outRect->left + (outRect->right - outRect->left) / 2;
          int yCenter = outRect->top + (outRect->bottom - outRect->top) / 2;

          /* find on which screen the window exist */
          int i;
          for (i = 0; i < rectScreensCount; i++)
            if (xCenter > rectScreens[i].left && xCenter < rectScreens[i].right &&
                yCenter > rectScreens[i].top && yCenter < rectScreens[i].bottom)
              break;

          if (i == rectScreensCount)
            /* Error: did not found on which screen the window exist */
            return FALSE;

          if (rectScreens[i].left > outRect->left) outRect->left = rectScreens[i].left;
          if (rectScreens[i].right < outRect->right) outRect->right = rectScreens[i].right;
          if (rectScreens[i].top > outRect->top) outRect->top = rectScreens[i].top;
          if (rectScreens[i].bottom < outRect->bottom) outRect->bottom = rectScreens[i].bottom;

          g_free (rectScreens);
        }

      return TRUE;
    }

  return FALSE;
}

/*
 * doCapture
 *
 * Do the capture.  Accepts the window
 * handle to be captured or the NULL value
 * to specify the root window.
 */
static gboolean
doCapture (HWND selectedHwnd)
{
  RECT rect;

  /* Try and get everything out of the way before the
   * capture.
   */
  Sleep (500 + winsnapvals.delay * 1000);

  /* Are we capturing a window or the whole screen */
  if (selectedHwnd)
    {
      if (! GetAccurateWindowRect (selectedHwnd, &rect))
      /* For some reason it works only if we return here TRUE. strange... */
          return TRUE;

      /* First try to capture the window with the magnification api.
      *  This will solve the bug https://bugzilla.gnome.org/show_bug.cgi?id=793722/
      */

      if (! doCaptureMagnificationAPI (selectedHwnd, rect))
        {
          /* If for some reason this capture method failed then
          *  capture the window with the normal method:
          */
          HWND     previousHwnd = GetForegroundWindow ();
          gboolean success;

          SetForegroundWindow (selectedHwnd);
          BringWindowToTop (selectedHwnd);

          success = doCaptureBitBlt (selectedHwnd, rect);

          if (previousHwnd)
            SetForegroundWindow (previousHwnd);

          return success;
        }

      return TRUE;

    }
  else
    {
      /* Get the screen's rectangle */
      rect.top    = GetSystemMetrics (SM_YVIRTUALSCREEN);
      rect.bottom = GetSystemMetrics (SM_YVIRTUALSCREEN) + GetSystemMetrics (SM_CYVIRTUALSCREEN);
      rect.left   = GetSystemMetrics (SM_XVIRTUALSCREEN);
      rect.right  = GetSystemMetrics (SM_XVIRTUALSCREEN) + GetSystemMetrics (SM_CXVIRTUALSCREEN);

      return doCaptureBitBlt (selectedHwnd, rect);

    }

  return FALSE; /* we should never get here... */
}

static gboolean
doCaptureBitBlt (HWND selectedHwnd,
                 RECT rect)
{

  HDC     hdcSrc;
  HDC     hdcCompat;
  HBITMAP hbm;

  /* Get the device context for the whole screen
   * even if we just want to capture a window.
   * this will allow to capture applications that
   * don't render to their main window's device
   * context (e.g. browsers).
  */
  hdcSrc = CreateDC (TEXT("DISPLAY"), NULL, NULL, NULL);

  if (!hdcSrc)
    {
      formatWindowsError(buffer, sizeof buffer);
      g_error ("Error getting device context: %s", buffer);
      return FALSE;
    }
  hdcCompat = CreateCompatibleDC (hdcSrc);
  if (!hdcCompat)
    {
      formatWindowsError (buffer, sizeof buffer);
      g_error ("Error getting compat device context: %s", buffer);
      return FALSE;
    }

  /* Do the window capture */
  hbm = primDoWindowCapture (hdcSrc, hdcCompat, rect);
  if (!hbm)
    return FALSE;

  /* Release the device context */
  ReleaseDC(selectedHwnd, hdcSrc);

  if (hbm == NULL) return FALSE;

  sendBMPToGimp (hbm, hdcCompat, rect);

  return TRUE;

}

static void
doCaptureMagnificationAPI_callback (HWND            hwnd,
                                    void           *srcdata,
                                    MAGIMAGEHEADER  srcheader,
                                    void           *destdata,
                                    MAGIMAGEHEADER  destheader,
                                    RECT            unclipped,
                                    RECT            clipped,
                                    HRGN            dirty)
{
  if (!srcdata) return;
  capBytes = (guchar*) malloc (sizeof (guchar)*srcheader.cbSize);
  rgbaToRgbBytes (capBytes, srcdata, srcheader.cbSize);
  returnedSrcheader = srcheader;
}

static BOOL
isWindowIsAboveCaptureRegion (HWND hwndWindow,
                              RECT rectCapture)
{
  RECT rectWindow;
  ZeroMemory (&rectWindow, sizeof (RECT));
  if (!GetWindowRect (hwndWindow, &rectWindow)) return FALSE;
  if (
      (
       (rectWindow.left >= rectCapture.left && rectWindow.left < rectCapture.right)
       ||
       (rectWindow.right <= rectCapture.right && rectWindow.right > rectCapture.left)
       ||
       (rectWindow.left <= rectCapture.left && rectWindow.right >= rectCapture.right)
      )
      &&
      (
       (rectWindow.top >= rectCapture.top && rectWindow.top < rectCapture.bottom)
       ||
       (rectWindow.bottom <= rectCapture.bottom && rectWindow.bottom > rectCapture.top)
       ||
       (rectWindow.top <= rectCapture.top && rectWindow.bottom >= rectCapture.bottom)
      )
  )
    return TRUE;
  else
    return FALSE;
}

static gboolean
doCaptureMagnificationAPI (HWND selectedHwnd,
                           RECT rect)
{
  HWND            hwndMag;
  HWND            hwndHost;
  HWND            nextWindow;
  HWND            excludeWins[24];
  RECT            round4Rect;
  int             excludeWinsCount = 0;

  if (!LoadMagnificationLibrary ()) return FALSE;

  if (!MagInitialize ()) return FALSE;

  round4Rect = rect;
  round4Rect.right = round4Rect.left + ROUND4(round4Rect.right - round4Rect.left);

  /* Create the host window that will store the mag child window */
  hwndHost = CreateWindowEx (0x08000000 | 0x080000 | 0x80 | 0x20, APP_NAME, NULL, 0x80000000,
                             0, 0, 0, 0, NULL, NULL, GetModuleHandle (NULL), NULL);

  if (!hwndHost)
    {
      MagUninitialize ();
      return FALSE;
    }

  SetLayeredWindowAttributes (hwndHost, (COLORREF)0, (BYTE)255, (DWORD)0x02);

  /* Create the mag child window inside the host window */
  hwndMag = CreateWindow (WC_MAGNIFIER, TEXT ("MagnifierWindow"),
                          WS_CHILD /*| MS_SHOWMAGNIFIEDCURSOR*/  /*| WS_VISIBLE*/,
                          0, 0, round4Rect.right - round4Rect.left, round4Rect.bottom - round4Rect.top,
                          hwndHost, NULL, GetModuleHandle (NULL), NULL);

  /* Set the callback function that will be called by the api to get the pixels */
  if (!MagSetImageScalingCallback (hwndMag, (MagImageScalingCallback)doCaptureMagnificationAPI_callback))
    {
      DestroyWindow (hwndHost);
      MagUninitialize ();
      return FALSE;
    }

  /* Add only windows that above the target window */
  for (nextWindow = GetNextWindow (selectedHwnd, GW_HWNDPREV); nextWindow != NULL; nextWindow = GetNextWindow (nextWindow, GW_HWNDPREV))
    if (isWindowIsAboveCaptureRegion (nextWindow, rect))
    {
      excludeWins[excludeWinsCount++] = nextWindow;
      /* This api can't work with more than 24 windows. we stop on the 24 window */
      if (excludeWinsCount >= 24) break;
    }

  if (excludeWinsCount)
    MagSetWindowFilterList (hwndMag, MW_FILTERMODE_EXCLUDE, excludeWinsCount, excludeWins);

  /* Call the api to capture the window */
  capBytes = NULL;

  if (!MagSetWindowSource (hwndMag, round4Rect) || !capBytes)
    {
      DestroyWindow (hwndHost);
      MagUninitialize ();
      return FALSE;
    }

  /* Send it to Gimp */
  sendBMPToGimp (NULL, NULL, rect);

  DestroyWindow (hwndHost);
  MagUninitialize ();

  return TRUE;
}

/******************************************************************
 * Win32 entry point and setup...
 ******************************************************************/

#define DINV 3

/*
 * highlightWindowFrame
 *
 * Highlight (or unhighlight) the specified
 * window handle's frame.
 */
static void
highlightWindowFrame (HWND hWnd)
{
  HDC  hdc;
  RECT rc;

  if (!IsWindow (hWnd))
    return;

  hdc = GetWindowDC (hWnd);
  GetWindowRect (hWnd, &rc);
  OffsetRect (&rc, -rc.left, -rc.top);

  if (!IsRectEmpty (&rc))
    {
      PatBlt (hdc, rc.left, rc.top, rc.right-rc.left, DINV, DSTINVERT);
      PatBlt (hdc, rc.left, rc.bottom-DINV, DINV, -(rc.bottom-rc.top-2*DINV),
              DSTINVERT);
      PatBlt (hdc, rc.right-DINV, rc.top+DINV, DINV, rc.bottom-rc.top-2*DINV,
              DSTINVERT);
      PatBlt (hdc, rc.right, rc.bottom-DINV, -(rc.right-rc.left), DINV,
              DSTINVERT);
    }

  ReleaseDC (hWnd, hdc);
  UpdateWindow (hWnd);
}

/*
 * setCaptureType
 *
 * Set the capture type.  Should be one of:
 * SELECT_FRAME
 * SELECT_CLIENT
 * SELECT_WINDOW
 */
void
setCaptureType (int capType)
{
  captureType = capType;
}

/*
 * myWindowFromPoint
 *
 * Map to the appropriate window from the
 * specified point.  The chosen window is
 * based on the current capture type.
 */
static HWND
myWindowFromPoint (POINT pt)
{
  HWND myHwnd;
  HWND nextHwnd;

  switch (captureType)
    {
    case SELECT_FRAME:
    case SELECT_CLIENT:
      nextHwnd = WindowFromPoint (pt);

      do {
          myHwnd = nextHwnd;
          nextHwnd = GetParent (myHwnd);
      } while (nextHwnd);

      return myHwnd;
      break;

    case SELECT_WINDOW:
      return WindowFromPoint (pt);
      break;
    }

  return WindowFromPoint (pt);
}

/*
 * dialogProc
 *
 * The window procedure for the window
 * selection dialog box.
 */
BOOL CALLBACK
dialogProc (HWND   hwndDlg,
            UINT   msg,
            WPARAM wParam,
            LPARAM lParam)
{
  static int     mouseCaptured;
  static int     buttonDown;
  static HCURSOR oldCursor;
  static RECT    bitmapRect;
  static HWND    highlightedHwnd = NULL;

  switch (msg)
    {
    case WM_INITDIALOG:
        {
          int    nonclientHeight;
          HWND   hwndGroup;
          RECT   dlgRect;
          RECT   clientRect;
          RECT   groupRect;
          BITMAP bm;

          /* Set the mouse capture flag */
          buttonDown = 0;
          mouseCaptured = 0;

          /* Calculate the bitmap dimensions */
          GetObject (iconInfo.hbmMask, sizeof(BITMAP), (VOID *)&bm);

          /* Calculate the dialog window dimensions */
          GetWindowRect (hwndDlg, &dlgRect);

          /* Calculate the group box dimensions */
          hwndGroup = GetDlgItem(hwndDlg, IDC_GROUP);
          GetWindowRect (hwndGroup, &groupRect);
          OffsetRect (&groupRect, -dlgRect.left, -dlgRect.top);

          /* The client's rectangle */
          GetClientRect (hwndDlg, &clientRect);

          /* The non-client height */
          nonclientHeight = (dlgRect.bottom - dlgRect.top) -
            (clientRect.bottom - clientRect.top);

          /* Calculate the bitmap rectangle */
          bitmapRect.top = ((groupRect.top + groupRect.bottom) / 2) -
            (bm.bmHeight / 2);
          bitmapRect.top -= nonclientHeight;
          bitmapRect.bottom = bitmapRect.top + bm.bmHeight;
          bitmapRect.left = ((groupRect.left + groupRect.right) / 2) - (bm.bmWidth / 2);
          bitmapRect.right = bitmapRect.left + bm.bmWidth;
        }
      break;

    case WM_LBUTTONDOWN:
      /* Track the button down state */
      buttonDown = 1;
      break;

    case WM_LBUTTONUP:
      buttonDown = 0;

      /* If we have mouse captured
       * we do this stuff.
       */
      if (mouseCaptured)
        {
          HWND  selectedHwnd;
          POINT cursorPos;

          /* Release the capture */
          mouseCaptured = 0;
          SetCursor (oldCursor);
          ReleaseCapture ();

          /* Remove the highlight */
          if (highlightedHwnd)
            highlightWindowFrame (highlightedHwnd);
          RedrawWindow (hwndDlg, NULL, NULL, RDW_INVALIDATE);

          /* Return the selected window */
          GetCursorPos (&cursorPos);
          selectedHwnd = myWindowFromPoint (cursorPos);
          EndDialog (hwndDlg, (INT_PTR) selectedHwnd);
        }
      break;

    case WM_MOUSEMOVE:
      /* If the mouse is captured, show
       * the window which is tracking
       * under the mouse position.
       */
      if (mouseCaptured)
        {
          HWND  currentHwnd;
          POINT cursorPos;

          /* Get the window */
          GetCursorPos (&cursorPos);
          currentHwnd = myWindowFromPoint (cursorPos);

          /* Do the highlighting */
          if (highlightedHwnd != currentHwnd)
            {
              if (highlightedHwnd)
                highlightWindowFrame (highlightedHwnd);
              if (currentHwnd)
                highlightWindowFrame (currentHwnd);
              highlightedHwnd = currentHwnd;
            }
          /* If the mouse has not been captured,
           * try to figure out if we should capture
           * the mouse.
           */
      }
      else if (buttonDown)
        {
          POINT cursorPos;

          /* Get the current client position */
          GetCursorPos (&cursorPos);
          ScreenToClient (hwndDlg, &cursorPos);

          /* Check if within the rectangle formed
           * by the bitmap
           */
          if (PtInRect (&bitmapRect, cursorPos)) {
              mouseCaptured = 1;
              oldCursor = SetCursor (selectCursor);
              SetCapture (hwndDlg);
              RedrawWindow (hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
          }
        }

      break;

    case WM_PAINT:
        {
          HDC          hDC;
          PAINTSTRUCT  ps;

          /* If the mouse is not captured draw
           * the cursor image
           */
          if (!mouseCaptured)
            {
              hDC = BeginPaint (hwndDlg, &ps);
              DrawIconEx (hDC, bitmapRect.left, bitmapRect.top, selectCursor,
                          0, 0, 0, NULL, DI_NORMAL);
              EndPaint (hwndDlg, &ps);
            }
        }
      break;

    case WM_COMMAND:
      /* Handle the cancel button */
      switch (LOWORD (wParam))
        {
        case IDCANCEL:
          EndDialog (hwndDlg, 0);
          return TRUE;
          break;
        }

    }

  return FALSE;
}

///* Don't use the normal WinMain from gimp.h */
//#define WinMain WinMain_no_thanks
//MAIN()
//#undef WinMain

/*
 * WinMain
 *
 * The standard gimp plug-in WinMain won't quite cut it for
 * this plug-in.
 */
//int APIENTRY
//WinMain(HINSTANCE hInstance,
//  HINSTANCE hPrevInstance,
//  LPSTR     lpCmdLine,
//  int       nCmdShow)
//{
//  /*
//   * Normally, we would do all of the Windows-ish set up of
//   * the window classes and stuff here in WinMain.  But,
//   * the only time we really need the window and message
//   * queues is during the plug-in run.  So, all of that will
//   * be done during run().  This avoids all of the Windows
//   * setup stuff for the query().  Stash the instance handle now
//   * so it is available from the run() procedure.
//   */
//  hInst = hInstance;
//
//  /*
//   * Now, call gimp_main... This is what the normal WinMain()
//   * would do.
//   */
////  return gimp_main(&PLUG_IN_INFO, __argc, __argv);
//}

/*
 * InitApplication
 *
 * Initialize window data and register the window class
 */
BOOL
InitApplication (HINSTANCE hInstance)
{
  WNDCLASS wc;
  BOOL     retValue;

  /* Get some resources */
#ifdef _MSC_VER
  /* For some reason this works only with MSVC */
  selectCursor = LoadCursor (hInstance, MAKEINTRESOURCE(IDC_SELECT));
#else
  selectCursor = LoadCursor (NULL, IDC_CROSS);
#endif
  GetIconInfo (selectCursor, &iconInfo);

  /*
   * Fill in window class structure with parameters to describe
   * the main window.
   */
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = (WNDPROC) WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon (NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor (NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
  wc.lpszClassName = APP_NAME;
  wc.lpszMenuName = NULL;

  /* Register the window class and stash success/failure code. */
  retValue = RegisterClass (&wc);

  /* Log error */
  if (!retValue)
    {
      formatWindowsError (buffer, sizeof buffer);
      g_error ("Error registering class: %s", buffer);
      return retValue;
    }

  return retValue;
}

/*
 * InitInstance
 *
 * Create the main window for the application.
 */
BOOL
InitInstance (HINSTANCE hInstance,
              int       nCmdShow)
{
  HINSTANCE User32Library = LoadLibrary ("user32.dll");

  if (User32Library)
    {
      typedef BOOL (WINAPI* SET_PROC_DPI_AWARE)();
      SET_PROC_DPI_AWARE SetProcessDPIAware;

      /* This line fix bug: https://bugzilla.gnome.org/show_bug.cgi?id=796121#c4 */
      SetProcessDPIAware = (SET_PROC_DPI_AWARE) GetProcAddress (User32Library,
                                                                "SetProcessDPIAware");
      if (SetProcessDPIAware)
        SetProcessDPIAware();

      FreeLibrary (User32Library);
    }

  /* Create our window */
  mainHwnd = CreateWindow (APP_NAME, APP_NAME, WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                           NULL, NULL, hInstance, NULL);

  if (!mainHwnd)
    {
      return (FALSE);
    }

  ShowWindow (mainHwnd, nCmdShow);
  UpdateWindow (mainHwnd);

  return TRUE;
}

/*
 * winsnapWinMain
 *
 * This is the function that represents the code that
 * would normally reside in WinMain (see above).  This
 * function will get called during run() in order to set
 * up the windowing environment necessary for WinSnap to
 * operate.
 */
int
winsnapWinMain (void)
{
  MSG msg;

  /* Perform instance initialization */
  if (!InitApplication (hInst))
    return (FALSE);

  /* Perform application initialization */
  if (!InitInstance (hInst, SHOW_WINDOW))
    return (FALSE);

  /* Main message loop */
  while (GetMessage (&msg, NULL, 0, 0))
    {
      TranslateMessage (&msg);
      DispatchMessage (&msg);
    }

  return (msg.wParam);
}

/*
 * WndProc
 *
 * Process window message for the main window.
 */
LRESULT CALLBACK
WndProc (HWND   hwnd,
         UINT   message,
         WPARAM wParam,
         LPARAM lParam)
{
  HWND selectedHwnd;

  switch (message)
    {

    case WM_CREATE:
      /* The window is created... Send the capture message */
      PostMessage (hwnd, WM_DOCAPTURE, 0, 0);
      break;

    case WM_DOCAPTURE:
      /* Get the selected window handle */
      selectedHwnd = (HWND) DialogBox (hInst, MAKEINTRESOURCE(IDD_SELECT),
                                       hwnd, (DLGPROC) dialogProc);
      if (selectedHwnd)
        doCapture (selectedHwnd);

      PostQuitMessage (selectedHwnd != NULL);

      break;

    case WM_DESTROY:
      PostQuitMessage (0);
      break;

    default:
      return (DefWindowProc (hwnd, message, wParam, lParam));
    }

  return 0;
}

#endif /* G_OS_WIN32 */
