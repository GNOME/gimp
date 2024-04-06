/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppickbutton-win32.c
 * Copyright (C) 2022 Luca Bacci <luca.bacci@outlook.com>
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"
#include "gimppickbutton.h"
#include "gimppickbutton-private.h"
#include "gimppickbutton-win32.h"

#include <sdkddkver.h>
#include <windows.h>
#include <windowsx.h>

#include <stdint.h>

/*
 * NOTES:
 *
 * This implementation is based on gtk/gtkcolorpickerwin32.c from GTK.
 *
 * We install a low-level mouse hook so that color picking continues
 * even when switching active windows.
 *
 * Beyond that, we also create keep-above, input-only HWNDs and place
 * them on the screen to cover each monitor. This is done to show our
 * custom cursor and to avoid giving input to other windows: that way
 * the desktop appears "frozen" while picking the color.
 *
 * Finally, we also set up a low-level keyboard hook to dismiss color-
 * picking mode whenever the user presses <ESC>.
 *
 * Note that low-level hooks for mouse and keyboard do not use any DLL
 * injection and are thus non-invasive.
 *
 * For GTK4: consider using an appropriate GDK surface for input-only
 * windows. This'd enable us to also get Wintab input when CXO_SYSTEM
 * is not set.
 */

typedef struct
{
  HMONITOR handle;
  wchar_t *device;
  HDC      hdc;
  POINT    screen_origin;
  int      logical_width;
  int      logical_height;
  int      physical_width;
  int      physical_height;
} MonitorData;

GList  *pickers;
HHOOK   mouse_hook;
HHOOK   keyboard_hook;

ATOM    notif_window_class;
HWND    notif_window_handle;
GArray *monitors;

ATOM    input_window_class;
GArray *input_window_handles;

/* Utils */
static HMODULE            this_module                      (void);
static MonitorData *      monitors_find_for_logical_point  (POINT           logical);
static MonitorData *      monitors_find_for_physical_point (POINT           physical);
static POINT              logical_to_physical              (MonitorData    *data,
                                                            POINT           logical);
static void               destroy_window                   (gpointer        ptr);

/* Mouse cursor */
static HCURSOR            create_cursor_from_rgba32_pixbuf (GdkPixbuf      *pixbuf,
                                                            POINT           hotspot);
static HCURSOR            create_cursor                    (void);

/* Low-level mouse hook */
static LRESULT CALLBACK   mouse_procedure                  (int             nCode,
                                                            WPARAM          wParam,
                                                            LPARAM          lParam);
static                    gboolean ensure_mouse_hook       (void);
static void               remove_mouse_hook                (void);

/* Low-level keyboard hook */
static LRESULT CALLBACK   keyboard_procedure               (int             nCode,
                                                            WPARAM          wParam,
                                                            LPARAM          lParam);
static gboolean           ensure_keyboard_hook             (void);
static void               remove_keyboard_hook             (void);

/* Input-only window */
static LRESULT CALLBACK   input_window_procedure           (HWND            hwnd,
                                                            UINT            uMsg,
                                                            WPARAM          wParam,
                                                            LPARAM          lParam);
static gboolean           ensure_input_window_class        (void);
static void               remove_input_window_class        (void);
static HWND               create_input_window              (POINT           origin,
                                                            int             width,
                                                            int             height);

/* Hidden notification window */
static LRESULT CALLBACK   notif_window_procedure           (HWND            hwnd,
                                                            UINT            uMsg,
                                                            WPARAM          wParam,
                                                            LPARAM          lParam);
static gboolean           ensure_notif_window_class        (void);
static void               remove_notif_window_class        (void);
static gboolean           ensure_notif_window              (void);
static void               remove_notif_window              (void);

/* Monitor enumeration and discovery */
static void               monitor_data_free                (gpointer        ptr);
static BOOL CALLBACK      enum_monitor_callback            (HMONITOR        hMonitor,
                                                            HDC             hDC,
                                                            RECT           *pRect,
                                                            LPARAM          lParam);
static GArray*            enumerate_monitors               (void);

/* GimpPickButtonWin32 */
static void               ensure_input_windows             (void);
static void               remove_input_windows             (void);
static void               ensure_monitors                  (void);
static void               remove_monitors                  (void);
static void               ensure_screen_data               (void);
static void               remove_screen_data               (void);
static void               screen_changed                   (void);
static void               ensure_screen_tracking           (void);
static void               remove_screen_tracking           (void);
static GeglColor *        pick_color_with_gdi              (POINT           physical_point);
static void               user_picked                      (MonitorData    *monitor,
                                                            POINT           physical_point);
void                      _gimp_pick_button_win32_pick     (GimpPickButton *button);
static void                stop_picking                    (void);

/* {{{ Utils */

/* Gets a handle to the module containing this code.
 * Works regardless if we're building a shared or
 * static library */

static HMODULE
this_module (void)
{
  extern IMAGE_DOS_HEADER __ImageBase;
  return (HMODULE) &__ImageBase;
}

static MonitorData*
monitors_find_for_logical_point (POINT logical)
{
  HMONITOR monitor_handle;
  guint    i;

  monitor_handle = MonitorFromPoint (logical, MONITOR_DEFAULTTONULL);
  if (!monitor_handle)
    return NULL;

  ensure_monitors ();

  for (i = 0; i < monitors->len; i++)
    {
      MonitorData *data = &g_array_index (monitors, MonitorData, i);

      if (data->handle == monitor_handle)
        return data;
    }

  return NULL;
}

static MonitorData*
monitors_find_for_physical_point (POINT physical)
{
  guint i;

  ensure_monitors ();

  for (i = 0; i < monitors->len; i++)
    {
      MonitorData *data = &g_array_index (monitors, MonitorData, i);
      RECT         physical_rect;

      physical_rect.left   = data->screen_origin.x;
      physical_rect.top    = data->screen_origin.y;
      physical_rect.right  = physical_rect.left + data->physical_width;
      physical_rect.bottom = physical_rect.top + data->physical_height;

      /* TODO: tolerance */
      if (PtInRect (&physical_rect, physical))
        return data;
    }

  return NULL;
}

static POINT
logical_to_physical (MonitorData *data,
                     POINT        logical)
{
  POINT physical = logical;

  if (data &&
      (data->logical_width != data->physical_width) &&
      data->logical_width > 0 && data->physical_width > 0)
    {
      double dpi_scale = (double) data->physical_width /
                         (double) data->logical_width;

      physical.x = logical.x * dpi_scale;
      physical.y = logical.y * dpi_scale;
    }

  return physical;
}

static void
destroy_window (gpointer ptr)
{
  HWND *hwnd = (HWND*) ptr;
  DestroyWindow (*hwnd);
}

/* }}}
 * {{{ Mouse cursor */

static HCURSOR
create_cursor_from_rgba32_pixbuf (GdkPixbuf *pixbuf,
                                  POINT      hotspot)
{
  struct {
    BITMAPV5HEADER header;
    RGBQUAD        colors[2];
  }                info;
  HBITMAP          bitmap      = NULL;
  uint8_t         *bitmap_data = NULL;
  HBITMAP          mask        = NULL;
  uint8_t         *mask_data   = NULL;
  unsigned int     mask_stride;
  HDC              hdc         = NULL;
  int              width;
  int              height;
  int              size;
  int              stride;
  const uint8_t   *pixbuf_data = NULL;
  int              i_offset;
  int              j_offset;
  int              i;
  int              j;
  ICONINFO         icon_info;
  HICON            icon        = NULL;

  if (gdk_pixbuf_get_n_channels (pixbuf) != 4)
    goto cleanup;

  hdc = GetDC (NULL);
  if (!hdc)
    goto cleanup;

  width       = gdk_pixbuf_get_width (pixbuf);
  height      = gdk_pixbuf_get_height (pixbuf);
  stride      = gdk_pixbuf_get_rowstride (pixbuf);
  size        = MAX (width, height);
  pixbuf_data = gdk_pixbuf_read_pixels (pixbuf);

  memset (&info, 0, sizeof (info));
  info.header.bV5Size        = sizeof (info.header);
  info.header.bV5Width       = size;
  info.header.bV5Height      = size;
  /* Since Windows XP the OS supports mouse cursors as 32bpp
   * bitmaps with alpha channel that are laid out as BGRA32
   * (assuming little-endian) */
  info.header.bV5Planes      = 1;
  info.header.bV5BitCount    = 32;
  info.header.bV5Compression = BI_BITFIELDS;
  info.header.bV5BlueMask    = 0x000000FF;
  info.header.bV5GreenMask   = 0x0000FF00;
  info.header.bV5RedMask     = 0x00FF0000;
  info.header.bV5AlphaMask   = 0xFF000000;

  bitmap = CreateDIBSection (hdc, (BITMAPINFO*) &info, DIB_RGB_COLORS,
                             (void**) &bitmap_data, NULL, 0);
  if (!bitmap || !bitmap_data)
    {
      g_warning ("CreateDIBSection failed with error code %u",
                 (unsigned) GetLastError ());
      goto cleanup;
    }

  /* We also need to provide a proper mask HBITMAP, otherwise
   * CreateIconIndirect() fails with ERROR_INVALID_PARAMETER.
   * This mask bitmap is a bitfield indicating whether the */
  memset (&info, 0, sizeof (info));
  info.header.bV5Size        = sizeof (info.header);
  info.header.bV5Width       = size;
  info.header.bV5Height      = size;
  info.header.bV5Planes      = 1;
  info.header.bV5BitCount    = 1;
  info.header.bV5Compression = BI_RGB;
  info.colors[0]             = (RGBQUAD){0x00, 0x00, 0x00};
  info.colors[1]             = (RGBQUAD){0xFF, 0xFF, 0xFF};

  mask = CreateDIBSection (hdc, (BITMAPINFO*) &info, DIB_RGB_COLORS,
                           (void**) &mask_data, NULL, 0);
  if (!mask || !mask_data)
    {
      g_warning ("CreateDIBSection failed with error code %u",
                 (unsigned) GetLastError ());
      goto cleanup;
    }

  /* MSDN says mask rows are aligned to LONG boundaries */
  mask_stride = (((size + 31) & ~31) >> 3);

  if (width > height)
    {
      i_offset = 0;
      j_offset = (width - height) / 2;
    }
  else
    {
      i_offset = (height - width) / 2;
      j_offset = 0;
    }

  for (j = 0; j < height; j++) /* loop over all the bitmap rows */
    {
      uint8_t       *bitmap_row = bitmap_data + 4 * ((j + j_offset) * size + i_offset);
      uint8_t       *mask_byte  = mask_data + (j + j_offset) * mask_stride + i_offset / 8;
      unsigned int   mask_bit   = (0x80 >> (i_offset % 8));
      const uint8_t *pixbuf_row = pixbuf_data + (height - j - 1) * stride;

      for (i = 0; i < width; i++) /* loop over the current bitmap row */
        {
          uint8_t       *bitmap_pixel = bitmap_row + 4 * i;
          const uint8_t *pixbuf_pixel = pixbuf_row + 4 * i;

          /* Assign to destination pixel from source pixel
           * by also swapping channels as appropriate */
          bitmap_pixel[0] = pixbuf_pixel[2];
          bitmap_pixel[1] = pixbuf_pixel[1];
          bitmap_pixel[2] = pixbuf_pixel[0];
          bitmap_pixel[3] = pixbuf_pixel[3];

          if (pixbuf_pixel[3] == 0)
            mask_byte[0] |= mask_bit;    /* turn ON bit */
          else
            mask_byte[0] &= ~mask_bit;   /* turn OFF bit */

          mask_bit >>= 1;
          if (mask_bit == 0)
            {
              mask_bit = 0x80;
              mask_byte++;
            }
        }
    }

  memset (&icon_info, 0, sizeof (icon_info));
  icon_info.fIcon    = FALSE;
  icon_info.xHotspot = hotspot.x;
  icon_info.yHotspot = hotspot.y;
  icon_info.hbmMask  = mask;
  icon_info.hbmColor = bitmap;

  icon = CreateIconIndirect (&icon_info);
  if (!icon)
    {
      g_warning ("CreateIconIndirect failed with error code %u",
                 (unsigned) GetLastError ());
      goto cleanup;
    }

cleanup:
  if (mask)
    DeleteObject (mask);

  if (bitmap)
    DeleteObject (bitmap);

  if (hdc)
    ReleaseDC (NULL, hdc);

  return (HCURSOR) icon;
}

static HCURSOR
create_cursor (void)
{
  GdkPixbuf *pixbuf  = NULL;
  GError    *error   = NULL;
  HCURSOR    cursor  = NULL;

  pixbuf = gdk_pixbuf_new_from_resource ("/org/gimp/color-picker-cursors/cursor-color-picker.png",
                                         &error);
  if (!pixbuf)
    {
      g_critical ("gdk_pixbuf_new_from_resource failed: %s",
                  error ? error->message : "unknown error");
      goto cleanup;
    }
  g_clear_error (&error);

  cursor = create_cursor_from_rgba32_pixbuf (pixbuf, (POINT){ 1, 30 });

cleanup:
  g_clear_error (&error);
  g_clear_object (&pixbuf);

  return cursor;
}

/* }}}
 * {{{ Low-level mouse hook */

/* This mouse procedure can detect clicks made on any
 * application window. Countrary to mouse capture, this
 * method continues to work even after switching active
 * windows. */

static LRESULT CALLBACK
mouse_procedure (int    nCode,
                 WPARAM wParam,
                 LPARAM lParam)
{
  if (nCode == HC_ACTION)
    {
      MSLLHOOKSTRUCT *info = (MSLLHOOKSTRUCT*) lParam;

      switch (wParam)
        {
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_XBUTTONDOWN:
          {
            POINT        physical;
            MonitorData *data;

            if (pickers == NULL)
              break;

            /* A low-level mouse hook always receives points in
             * per-monitor DPI-aware screen coordinates, regardless of
             * the DPI awareness setting of the application. */
            physical = info->pt;

            data = monitors_find_for_physical_point (physical);
            if (!data)
              g_message ("Captured point (%ld, %ld) doesn't belong to any monitor",
                         (long) physical.x, (long) physical.y);

            user_picked (data, physical);

            /* It's safe to remove a hook from within its callback.
             * Anyway this can even be called from an idle callback,
             * as the hook does nothing if there are no pickers.
             * (In that case also the ensure functions have to be
             * scheduled in an idle callback) */
            stop_picking ();

            return (wParam == WM_XBUTTONDOWN) ? TRUE : 0;
          }
          break;
        default:
          break;
        }
    }

  return CallNextHookEx (NULL, nCode, wParam, lParam);
}

static gboolean
ensure_mouse_hook (void)
{
  if (!mouse_hook)
    {
      mouse_hook = SetWindowsHookExW (WH_MOUSE_LL, mouse_procedure, this_module (), 0);
      if (!mouse_hook)
        {
          g_warning ("SetWindowsHookEx failed with error code %u",
                     (unsigned) GetLastError ());
          return FALSE;
        }
    }

  return TRUE;
}

static void
remove_mouse_hook (void)
{
  if (mouse_hook)
    {
      if (!UnhookWindowsHookEx (mouse_hook))
        {
          g_warning ("UnhookWindowsHookEx failed with error code %u",
                     (unsigned) GetLastError ());
          return;
        }

      mouse_hook = NULL;
    }
}

/* }}}
 * {{{ Low-level keyboard hook */

/* This is used to stop picking anytime the user presses ESC */

static LRESULT CALLBACK
keyboard_procedure (int    nCode,
                    WPARAM wParam,
                    LPARAM lParam)
{
  if (nCode == HC_ACTION)
    {
      KBDLLHOOKSTRUCT *info = (KBDLLHOOKSTRUCT*) lParam;

      switch (wParam)
        {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
          if (info->vkCode == VK_ESCAPE)
            {
              stop_picking ();
              return 1;
            }
          break;
        default:
          break;
        }
    }

  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static gboolean
ensure_keyboard_hook (void)
{
  if (!keyboard_hook)
    {
      keyboard_hook = SetWindowsHookExW (WH_KEYBOARD_LL, keyboard_procedure, this_module (), 0);
      if (!keyboard_hook)
        {
          g_warning ("SetWindowsHookEx failed with error code %u",
                     (unsigned) GetLastError ());
          return FALSE;
        }
    }

  return TRUE;
}

static void
remove_keyboard_hook (void)
{
  if (keyboard_hook)
    {
      if (!UnhookWindowsHookEx (keyboard_hook))
        {
          g_warning ("UnhookWindowsHookEx failed with error code %u",
                     (unsigned) GetLastError ());
          return;
        }

      keyboard_hook = NULL;
    }
}

/* }}}
 * {{{ Input-only windows */

/* Those input-only windows are placed to cover each monitor on
 * the screen and serve two purposes: display our custom mouse
 * cursor and freeze the desktop by avoiding interaction of the
 * mouse with other applications */

static LRESULT CALLBACK
input_window_procedure (HWND   hwnd,
                        UINT   uMsg,
                        WPARAM wParam,
                        LPARAM lParam)
{
  switch (uMsg)
    {
    case WM_NCCREATE:
      /* The shell automatically hides the taskbar when a window
       * covers the entire area of a monitor (fullscreened). In
       * order to avoid that, we can set a special property on
       * the window
       * See the docs for ITaskbarList2::MarkFullscreenWindow()
       * on MSDN for more informations */

      if (!SetPropW (hwnd, L"NonRudeHWND", (HANDLE) TRUE))
        g_warning_once ("SetPropW failed with error code %u",
                        (unsigned) GetLastError ());
      break;

    case WM_NCDESTROY:
      /* We have to remove window properties manually before the
       * window gets destroyed */

      if (!RemovePropW (hwnd, L"NonRudeHWND"))
        g_warning_once ("SetPropW failed with error code %u",
                        (unsigned) GetLastError ());
      break;

    /* Avoid any drawing at all. Here we block processing of the
     * default window procedure, although we set the NULL_BRUSH
     * as the window class's background brush, so even the default
     * window procedure wouldn't draw anything at all */

    case WM_ERASEBKGND:
      return 1;
    case WM_PAINT:
      {
        #if 1
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        #else
        UINT flags = RDW_VALIDATE |
                     RDW_NOFRAME |
                     RDW_NOERASE;
        RedrawWindow (hwnd, NULL, NULL, flags);
        #endif
      }
      return 0;
    #if 0
    case WM_SYNCPAINT:
      return 0;
    #endif

    case WM_MOUSEACTIVATE:
      /* This can be useful in case we want to avoid
       * using a mouse hook */
      return MA_NOACTIVATE;

    /* Anytime we detect a mouse click, pick the color. Note that
     * this is rarely used as the mouse hook would process the
     * clicks before that */

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_XBUTTONDOWN:
      {
        POINT        logical  = (POINT){GET_X_LPARAM (lParam), GET_Y_LPARAM (lParam)};
        MonitorData *data     = monitors_find_for_logical_point (logical);
        POINT        physical = logical_to_physical (data, logical);

        if (!data)
          g_message ("Captured point (%ld, %ld) doesn't belong to any monitor",
                     (long) logical.x, (long) logical.y);

        user_picked (data, physical);

        stop_picking ();
      }

      return (uMsg == WM_XBUTTONDOWN) ? TRUE : 0;
    default:
      break;
    }

  return DefWindowProcW (hwnd, uMsg, wParam, lParam);
}

static gboolean
ensure_input_window_class (void)
{
  if (!input_window_class)
    {
      WNDCLASSEXW wndclassex;
      HCURSOR     cursor     = create_cursor ();

      memset (&wndclassex, 0, sizeof (wndclassex));
      wndclassex.cbSize        = sizeof (wndclassex);
      wndclassex.hInstance     = this_module ();
      wndclassex.lpszClassName = L"GimpPickButtonInputWindowClass";
      wndclassex.lpfnWndProc   = input_window_procedure;
      wndclassex.hbrBackground = GetStockObject (NULL_BRUSH);
      wndclassex.hCursor       = cursor ?
                                 cursor :
                                 LoadImageW (NULL,
                                             (LPCWSTR)(guintptr)IDC_CROSS,
                                             IMAGE_CURSOR, 0, 0,
                                             LR_DEFAULTSIZE | LR_SHARED);

      input_window_class = RegisterClassExW (&wndclassex);
      if (input_window_class == 0)
        {
          g_warning ("RegisterClassExW failed with error code %u",
                     (unsigned) GetLastError ());

          if (cursor)
            DestroyCursor (cursor);

          return FALSE;
        }
    }

  return TRUE;
}

static void
remove_input_window_class (void)
{
  if (input_window_class)
    {
      LPCWSTR name = (LPCWSTR)(guintptr)input_window_class;
      UnregisterClassW (name, this_module ());
      input_window_class = 0;
    }
}

/* create_input_window expects logical screen coordinates */

static HWND
create_input_window (POINT origin,
                     int width,
                     int height)
{
  DWORD   stylex  = WS_EX_NOACTIVATE | WS_EX_TOPMOST;
  LPCWSTR wclass;
  LPCWSTR title   = L"Gimp Input Window";
  DWORD   style   = WS_POPUP;
  HWND    hwnd;

  if (!ensure_input_window_class ())
    return FALSE;

  /* This must go after the ensure_input_window_class */
  wclass = (LPCWSTR)(guintptr)input_window_class;

  hwnd = CreateWindowExW (stylex, wclass, title, style,
                          origin.x, origin.y, width, height,
                          NULL, NULL, this_module (), NULL);
  if (hwnd == NULL)
    {
      g_warning_once ("CreateWindowExW failed with error code %u",
                      (unsigned) GetLastError ());
      return FALSE;
    }

  ShowWindow (hwnd, SW_SHOWNOACTIVATE);

  return hwnd;
}

/* }}}
 * {{{ Hidden notification window */

/* We setup a hidden window to listen for WM_DISPLAYCAHNGE
 * messages and reposition the input-only windows on the
 * screen */

static LRESULT CALLBACK
notif_window_procedure (HWND   hwnd,
                        UINT   uMsg,
                        WPARAM wParam,
                        LPARAM lParam)
{
  switch (uMsg)
    {
    case WM_DISPLAYCHANGE:
      screen_changed ();
      break;
    default:
      break;
    }

  return DefWindowProcW (hwnd, uMsg, wParam, lParam);
}

static gboolean
ensure_notif_window_class (void)
{
  if (!notif_window_class)
    {
      WNDCLASSEXW wndclassex;

      memset (&wndclassex, 0, sizeof (wndclassex));
      wndclassex.cbSize        = sizeof (wndclassex);
      wndclassex.hInstance     = this_module ();
      wndclassex.lpszClassName = L"GimpPickButtonNotifWindowClass";
      wndclassex.lpfnWndProc   = notif_window_procedure;

      notif_window_class = RegisterClassExW (&wndclassex);
      if (notif_window_class == 0)
        {
          g_warning ("RegisterClassExW failed with error code %u",
                     (unsigned) GetLastError ());
          return FALSE;
        }
    }

  return TRUE;
}

static void
remove_notif_window_class (void)
{
  if (notif_window_class)
    {
      LPCWSTR name = (LPCWSTR)(guintptr)notif_window_class;
      UnregisterClassW (name, this_module ());
      notif_window_class = 0;
    }
}

static gboolean
ensure_notif_window (void)
{
  if (!ensure_notif_window_class ())
    return FALSE;

  if (!notif_window_handle)
    {
      DWORD   stylex = 0;
      LPCWSTR wclass = (LPCWSTR)(guintptr)notif_window_class;
      LPCWSTR title  = L"Gimp Notifications Window";
      DWORD   style  = WS_POPUP;

      notif_window_handle = CreateWindowExW (stylex, wclass,
                                             title, style,
                                             0, 0, 1, 1,
                                             NULL, NULL,
                                             this_module (),
                                             NULL);
      if (notif_window_handle == NULL)
        {
          g_warning_once ("CreateWindowExW failed with error code %u",
                          (unsigned) GetLastError ());
          return FALSE;
        }
    }

  return TRUE;
}

static void
remove_notif_window (void)
{
  if (notif_window_handle)
    {
      DestroyWindow (notif_window_handle);
      notif_window_handle = NULL;
    }

  remove_notif_window_class ();
}

/* }}}
 * {{{ Monitor enumeration and discovery */

static void
monitor_data_free (gpointer ptr)
{
  MonitorData *data = (MonitorData*) ptr;

  if (data->device)
    free (data->device);
}

static BOOL CALLBACK
enum_monitor_callback (HMONITOR  hMonitor,
                       HDC       hDC,
                       RECT     *pRect,
                       LPARAM    lParam)
{
  GArray         *result  = (GArray*) lParam;
  MonitorData     data;
  MONITORINFOEXW  info;
  DEVMODEW        devmode;

  const BOOL CALLBACK_CONTINUE = TRUE;
  const BOOL CALLBACK_STOP = FALSE;

  if (!pRect)
    return CALLBACK_CONTINUE;

  if (IsRectEmpty (pRect))
    return CALLBACK_CONTINUE;

  memset (&data, 0, sizeof (data));
  data.handle          = hMonitor;
  data.hdc             = hDC;
  data.screen_origin.x = pRect->left;
  data.screen_origin.y = pRect->top;
  data.logical_width   = pRect->right - pRect->left;
  data.logical_height  = pRect->bottom - pRect->top;

  memset (&info, 0, sizeof (info));
  info.cbSize = sizeof (info);
  if (!GetMonitorInfoW (hMonitor, (MONITORINFO*) &info))
    {
      g_warning_once ("GetMonitorInfo failed with error code %u",
                      (unsigned) GetLastError ());
      return CALLBACK_CONTINUE;
    }

  data.device = _wcsdup (info.szDevice);

  memset (&devmode, 0, sizeof (devmode));
  devmode.dmSize = sizeof (devmode);
  if (!EnumDisplaySettingsExW (info.szDevice,
                               ENUM_CURRENT_SETTINGS,
                               &devmode, EDS_ROTATEDMODE))
    {
      g_warning_once ("EnumDisplaySettingsEx failed with error code %u",
                      (unsigned) GetLastError ());
      return CALLBACK_CONTINUE;
    }

  if (devmode.dmPelsWidth)
    data.physical_width = devmode.dmPelsWidth;

  if (devmode.dmPelsHeight)
    data.physical_height = devmode.dmPelsHeight;

  g_array_append_val (result, data);

  if (result->len >= 100)
    return CALLBACK_STOP;

  return CALLBACK_CONTINUE;
}

static GArray*
enumerate_monitors (void)
{
  int     count_monitors;
  guint   length_hint;
  GArray *result;

  count_monitors = GetSystemMetrics (SM_CMONITORS);

  if (count_monitors > 0 && count_monitors < 100)
    length_hint = (guint) count_monitors;
  else
    length_hint = 1;

  result = g_array_sized_new (FALSE, TRUE, sizeof (MonitorData), length_hint);
  g_array_set_clear_func (result, monitor_data_free);

  if (!EnumDisplayMonitors (NULL, NULL, enum_monitor_callback, (LPARAM) result))
    {
      g_warning ("EnumDisplayMonitors failed with error code %u",
                 (unsigned) GetLastError ());
    }

  return result;
}

static void
ensure_input_windows (void)
{
  ensure_monitors ();

  if (!input_window_handles)
    {
      guint i;

      input_window_handles = g_array_sized_new (FALSE, TRUE,
                                                sizeof (HWND),
                                                monitors->len);
      g_array_set_clear_func (input_window_handles, destroy_window);

      for (i = 0; i < monitors->len; i++)
        {
          MonitorData *data = &g_array_index (monitors, MonitorData, i);
          HWND         hwnd = create_input_window (data->screen_origin,
                                                   data->logical_width,
                                                   data->logical_height);

          if (hwnd)
            g_array_append_val (input_window_handles, hwnd);
        }
    }
}

static void
remove_input_windows (void)
{
  if (input_window_handles)
    {
      g_array_free (input_window_handles, TRUE);
      input_window_handles = NULL;
    }
}

static void
ensure_monitors (void)
{
  if (!monitors)
    monitors = enumerate_monitors ();
}

static void
remove_monitors (void)
{
  if (monitors)
    {
      g_array_free (monitors, TRUE);
      monitors = NULL;
    }
}

static void
ensure_screen_data (void)
{
  ensure_monitors ();
  ensure_input_windows ();
}

static void
remove_screen_data (void)
{
  remove_input_windows ();
  remove_monitors ();
}

static void
screen_changed (void)
{
  remove_screen_data ();
  ensure_screen_data ();
}

static void
ensure_screen_tracking (void)
{
  ensure_notif_window ();
  screen_changed ();
}

static void
remove_screen_tracking (void)
{
  remove_notif_window ();
  remove_screen_data ();
}

/* }}}
 * {{{ GimpPickButtonWin32 */

/* pick_color_with_gdi is based on the GDI GetPixel() function.
 * Note that GDI only returns 8bit per-channel color data, but
 * as of today there's no documented method to retrieve colors
 * at higher bit depths. */

static GeglColor *
pick_color_with_gdi (POINT physical_point)
{
  GeglColor *rgb = gegl_color_new ("black");
  COLORREF   color;
  HDC        hdc;
  guchar     temp_rgb[3];

  hdc = GetDC (HWND_DESKTOP);

  if (!(GetDeviceCaps (hdc, RASTERCAPS) & RC_BITBLT))
    g_warning_once ("RC_BITBLT capability missing from device context");

  color = GetPixel (hdc, physical_point.x, physical_point.y);

  temp_rgb[0] = GetRValue (color);
  temp_rgb[1] = GetGValue (color);
  temp_rgb[2] = GetBValue (color);

  gegl_color_set_pixel (rgb, babl_format ("R'G'B' u8"), temp_rgb);

  ReleaseDC (HWND_DESKTOP, hdc);

  return rgb;
}

static void
user_picked (MonitorData *monitor,
             POINT        physical_point)
{
  GeglColor *rgb;
  GList     *l;

  /* Currently unused */
  (void) monitor;

  rgb = pick_color_with_gdi (physical_point);

  for (l = pickers; l != NULL; l = l->next)
    {
      GimpPickButton *button = GIMP_PICK_BUTTON (l->data);
      g_signal_emit_by_name (button, "color-picked", rgb);
      g_object_unref (rgb);
    }
}

/* entry point to this file, called from gimppickbutton.c */
void
_gimp_pick_button_win32_pick (GimpPickButton *button)
{
  ensure_mouse_hook ();
  ensure_keyboard_hook ();
  ensure_screen_tracking ();

  if (g_list_find (pickers, button))
    return;

  pickers = g_list_prepend (pickers,
                            g_object_ref (button));
}

static void
stop_picking (void)
{
  remove_screen_tracking ();
  remove_keyboard_hook ();
  remove_mouse_hook ();

  g_list_free_full (pickers, g_object_unref);
  pickers = NULL;
}
