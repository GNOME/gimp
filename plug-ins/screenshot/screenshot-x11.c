/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Screenshot plug-in
 * Copyright 1998-2007 Sven Neumann <sven@gimp.org>
 * Copyright 2003      Henrik Brix Andersen <brix@gimp.org>
 * Copyright 2012      Simone Karin Lehmann - OS X patches
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
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#ifdef GDK_WINDOWING_X11

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>

#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
#include <X11/extensions/shape.h>
#endif

#ifdef HAVE_X11_XMU_WINUTIL_H
#include <X11/Xmu/WinUtil.h>
#endif

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

#include "screenshot.h"
#include "screenshot-x11.h"

#include "libgimp/stdplugins-intl.h"


static guint32    select_window        (ScreenshotValues *shootvals,
                                        GdkMonitor       *monitor);
static gint32     create_image         (cairo_surface_t  *surface,
                                        cairo_region_t   *shape,
                                        const gchar      *name);


/* Allow the user to select a window or a region with the mouse */

static guint32
select_window (ScreenshotValues *shootvals,
               GdkMonitor       *monitor)
{
  Display      *x_dpy    = GDK_DISPLAY_XDISPLAY (gdk_monitor_get_display (monitor));
  gint          x_scr    = 0;
  Window        x_root   = RootWindow (x_dpy, x_scr);
  Window        x_win    = None;
  GC            x_gc     = NULL;
  Cursor        x_cursor = XCreateFontCursor (x_dpy, GDK_CROSSHAIR);
  GdkKeymap    *keymap;
  GdkKeymapKey *keys     = NULL;
  gint          status;
  gint          num_keys;
  gint          i;
  gint          buttons  = 0;
  gint          mask     = ButtonPressMask | ButtonReleaseMask;
  gboolean      cancel   = FALSE;

  if (shootvals->shoot_type == SHOOT_REGION)
    mask |= PointerMotionMask;

  status = XGrabPointer (x_dpy, x_root, False,
                         mask, GrabModeSync, GrabModeAsync,
                         x_root, x_cursor, CurrentTime);

  if (status != GrabSuccess)
    {
      gint  x, y;
      guint xmask;

      /* if we can't grab the pointer, return the window under the pointer */
      XQueryPointer (x_dpy, x_root, &x_root, &x_win, &x, &y, &x, &y, &xmask);

      if (x_win == None || x_win == x_root)
        g_message (_("Error selecting the window"));
    }

  if (shootvals->shoot_type == SHOOT_REGION)
    {
      XGCValues gc_values;

      gc_values.function           = GXxor;
      gc_values.plane_mask         = AllPlanes;
      gc_values.foreground         = WhitePixel (x_dpy, x_scr);
      gc_values.background         = BlackPixel (x_dpy, x_scr);
      gc_values.line_width         = 0;
      gc_values.line_style         = LineSolid;
      gc_values.fill_style         = FillSolid;
      gc_values.cap_style          = CapButt;
      gc_values.join_style         = JoinMiter;
      gc_values.graphics_exposures = FALSE;
      gc_values.clip_x_origin      = 0;
      gc_values.clip_y_origin      = 0;
      gc_values.clip_mask          = None;
      gc_values.subwindow_mode     = IncludeInferiors;

      x_gc = XCreateGC (x_dpy, x_root,
                        GCFunction | GCPlaneMask | GCForeground | GCLineWidth |
                        GCLineStyle | GCCapStyle | GCJoinStyle |
                        GCGraphicsExposures | GCBackground | GCFillStyle |
                        GCClipXOrigin | GCClipYOrigin | GCClipMask |
                        GCSubwindowMode,
                        &gc_values);
    }

  keymap = gdk_keymap_get_for_display (gdk_monitor_get_display (monitor));

  if (gdk_keymap_get_entries_for_keyval (keymap, GDK_KEY_Escape,
                                         &keys, &num_keys))
    {
      gdk_error_trap_push ();

#define X_GRAB_KEY(index, modifiers) \
      XGrabKey (x_dpy, keys[index].keycode, modifiers, x_root, False, \
                GrabModeAsync, GrabModeAsync)

      for (i = 0; i < num_keys; i++)
        {
          X_GRAB_KEY (i, 0);
          X_GRAB_KEY (i, LockMask);            /* CapsLock              */
          X_GRAB_KEY (i, Mod2Mask);            /* NumLock               */
          X_GRAB_KEY (i, Mod5Mask);            /* ScrollLock            */
          X_GRAB_KEY (i, LockMask | Mod2Mask); /* CapsLock + NumLock    */
          X_GRAB_KEY (i, LockMask | Mod5Mask); /* CapsLock + ScrollLock */
          X_GRAB_KEY (i, Mod2Mask | Mod5Mask); /* NumLock  + ScrollLock */
          X_GRAB_KEY (i, LockMask | Mod2Mask | Mod5Mask); /* all        */
        }

#undef X_GRAB_KEY

      gdk_display_flush (gdk_monitor_get_display (monitor));

      if (gdk_error_trap_pop ())
        {
          /* ignore errors */
        }
    }

  while (! cancel && ((x_win == None) || (buttons != 0)))
    {
      XEvent x_event;
      gint   x, y, w, h;

      XAllowEvents (x_dpy, SyncPointer, CurrentTime);
      XWindowEvent (x_dpy, x_root, mask | KeyPressMask, &x_event);

      switch (x_event.type)
        {
        case ButtonPress:
          if (x_win == None)
            {
              x_win = x_event.xbutton.subwindow;

              if (x_win == None)
                x_win = x_root;
#ifdef HAVE_X11_XMU_WINUTIL_H
              else if (! shootvals->decorate)
                x_win = XmuClientWindow (x_dpy, x_win);
#endif

              shootvals->x2 = shootvals->x1 = x_event.xbutton.x_root;
              shootvals->y2 = shootvals->y1 = x_event.xbutton.y_root;
            }

          buttons++;
          break;

        case ButtonRelease:
          if (buttons > 0)
            buttons--;

          if (! buttons && shootvals->shoot_type == SHOOT_REGION)
            {
              x = MIN (shootvals->x1, shootvals->x2);
              y = MIN (shootvals->y1, shootvals->y2);
              w = ABS (shootvals->x2 - shootvals->x1);
              h = ABS (shootvals->y2 - shootvals->y1);

              if (w > 0 && h > 0)
                XDrawRectangle (x_dpy, x_root, x_gc, x, y, w, h);

              shootvals->x2 = x_event.xbutton.x_root;
              shootvals->y2 = x_event.xbutton.y_root;
            }
          break;

        case MotionNotify:
          if (buttons > 0)
            {
              x = MIN (shootvals->x1, shootvals->x2);
              y = MIN (shootvals->y1, shootvals->y2);
              w = ABS (shootvals->x2 - shootvals->x1);
              h = ABS (shootvals->y2 - shootvals->y1);

              if (w > 0 && h > 0)
                XDrawRectangle (x_dpy, x_root, x_gc, x, y, w, h);

              shootvals->x2 = x_event.xmotion.x_root;
              shootvals->y2 = x_event.xmotion.y_root;

              x = MIN (shootvals->x1, shootvals->x2);
              y = MIN (shootvals->y1, shootvals->y2);
              w = ABS (shootvals->x2 - shootvals->x1);
              h = ABS (shootvals->y2 - shootvals->y1);

              if (w > 0 && h > 0)
                XDrawRectangle (x_dpy, x_root, x_gc, x, y, w, h);
            }
          break;

        case KeyPress:
          {
            guint *keyvals;
            gint   n;

            if (gdk_keymap_get_entries_for_keycode (NULL, x_event.xkey.keycode,
                                                    NULL, &keyvals, &n))
              {
                gint i;

                for (i = 0; i < n && ! cancel; i++)
                  if (keyvals[i] == GDK_KEY_Escape)
                    cancel = TRUE;

                g_free (keyvals);
              }
          }
          break;

        default:
          break;
        }
    }

  if (keys)
    {
#define X_UNGRAB_KEY(index, modifiers) \
      XUngrabKey (x_dpy, keys[index].keycode, modifiers, x_root)

      for (i = 0; i < num_keys; i++)
        {
          X_UNGRAB_KEY (i, 0);
          X_UNGRAB_KEY (i, LockMask);            /* CapsLock              */
          X_UNGRAB_KEY (i, Mod2Mask);            /* NumLock               */
          X_UNGRAB_KEY (i, Mod5Mask);            /* ScrollLock            */
          X_UNGRAB_KEY (i, LockMask | Mod2Mask); /* CapsLock + NumLock    */
          X_UNGRAB_KEY (i, LockMask | Mod5Mask); /* CapsLock + ScrollLock */
          X_UNGRAB_KEY (i, Mod2Mask | Mod5Mask); /* NumLock  + ScrollLock */
          X_UNGRAB_KEY (i, LockMask | Mod2Mask | Mod5Mask); /* all        */
        }
#undef X_UNGRAB_KEY

      g_free (keys);
    }

  if (status == GrabSuccess)
    XUngrabPointer (x_dpy, CurrentTime);

  XFreeCursor (x_dpy, x_cursor);

  if (x_gc != NULL)
    XFreeGC (x_dpy, x_gc);

  return x_win;
}

static gchar *
window_get_utf8_property (GdkDisplay  *display,
                          guint32      window,
                          const gchar *name)
{
  gchar   *retval = NULL;
  Atom     utf8_string;
  Atom     type   = None;
  guchar  *val    = NULL;
  gulong   nitems = 0;
  gulong   after  = 0;
  gint     format = 0;

  utf8_string = gdk_x11_get_xatom_by_name_for_display (display, "UTF8_STRING");

  XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display), window,
                      gdk_x11_get_xatom_by_name_for_display (display, name),
                      0, G_MAXLONG, False, utf8_string,
                      &type, &format, &nitems, &after, &val);

  if (type != utf8_string || format != 8 || nitems == 0)
    {
      if (val)
        XFree (val);
      return NULL;
    }

  if (g_utf8_validate ((const gchar *) val, nitems, NULL))
    retval = g_strndup ((const gchar *) val, nitems);

  XFree (val);

  return retval;
}

static gchar *
window_get_title (GdkDisplay *display,
                  guint       window)
{
#ifdef HAVE_X11_XMU_WINUTIL_H
  window = XmuClientWindow (GDK_DISPLAY_XDISPLAY (display), window);
#endif

  return window_get_utf8_property (display, window, "_NET_WM_NAME");
}

static cairo_region_t *
window_get_shape (GdkMonitor *monitor,
                  guint32     window)
{
  cairo_region_t *shape = NULL;

#if defined(HAVE_X11_EXTENSIONS_SHAPE_H)
  XRectangle *rects;
  gint        rect_count;
  gint        rect_order;

  rects = XShapeGetRectangles (GDK_DISPLAY_XDISPLAY (gdk_monitor_get_display (monitor)),
                               window,
                               ShapeBounding,
                               &rect_count, &rect_order);

  if (rects)
    {
      if (rect_count > 1)
        {
          gint i;

          shape = cairo_region_create ();

          for (i = 0; i < rect_count; i++)
            {
              cairo_rectangle_int_t rect = { rects[i].x,
                                             rects[i].y,
                                             rects[i].width,
                                             rects[i].height };

              cairo_region_union_rectangle (shape, &rect);
            }
        }

      XFree (rects);
    }
#endif

  return shape;
}

static void
image_select_shape (gint32          image,
                    cairo_region_t *shape)
{
  gint num_rects;
  gint i;

  gimp_selection_none (image);

  num_rects = cairo_region_num_rectangles (shape);

  for (i = 0; i < num_rects; i++)
    {
      cairo_rectangle_int_t rect;

      cairo_region_get_rectangle (shape, i, &rect);

      gimp_image_select_rectangle (image, GIMP_CHANNEL_OP_ADD,
                                   rect.x, rect.y,
                                   rect.width, rect.height);
    }

  gimp_selection_invert (image);
}


/* Create a GimpImage from a GdkPixbuf */

static gint32
create_image (cairo_surface_t *surface,
              cairo_region_t  *shape,
              const gchar     *name)
{
  gint32     image;
  gint32     layer;
  gdouble    xres, yres;
  gint       width, height;

  gimp_progress_init (_("Importing screenshot"));

  width  = cairo_image_surface_get_width (surface);
  height = cairo_image_surface_get_height (surface);

  image = gimp_image_new (width, height, GIMP_RGB);
  gimp_image_undo_disable (image);

  gimp_get_monitor_resolution (&xres, &yres);
  gimp_image_set_resolution (image, xres, yres);

  layer = gimp_layer_new_from_surface (image,
                                       name ? name : _("Screenshot"),
                                       surface,
                                       0.0, 1.0);
  gimp_image_insert_layer (image, layer, -1, 0);

  if (shape && ! cairo_region_is_empty (shape))
    {
      image_select_shape (image, shape);

      if (! gimp_selection_is_empty (image))
        {
          gimp_layer_add_alpha (layer);
          gimp_drawable_edit_clear (layer);
          gimp_selection_none (image);
        }
    }

  gimp_image_undo_enable (image);

  return image;
}

static void
add_cursor_image (gint32      image,
                  GdkDisplay *display)
{
#ifdef HAVE_XFIXES
  XFixesCursorImage  *cursor;
  GeglBuffer         *buffer;
  GeglBufferIterator *iter;
  GeglRectangle      *roi;
  gint32              layer;
  gint32              active;

  cursor = XFixesGetCursorImage (GDK_DISPLAY_XDISPLAY (display));

  if (!cursor)
    return;

  active = gimp_image_get_active_layer (image);

  layer = gimp_layer_new (image, _("Mouse Pointer"),
                          cursor->width, cursor->height,
                          GIMP_RGBA_IMAGE,
                          100.0,
                          gimp_image_get_default_new_layer_mode (image));

  buffer = gimp_drawable_get_buffer (layer);

  iter = gegl_buffer_iterator_new (buffer,
                                   GEGL_RECTANGLE (0, 0,
                                                   gimp_drawable_width  (layer),
                                                   gimp_drawable_height (layer)),
                                   0, babl_format ("R'G'B'A u8"),
                                   GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);
  roi = &iter->roi[0];

  while (gegl_buffer_iterator_next (iter))
    {
      const gulong *src  = cursor->pixels + roi->y * cursor->width + roi->x;
      guchar       *dest = iter->data[0];
      gint          x, y;

      for (y = 0; y < roi->height; y++)
        {
          const gulong *s = src;
          guchar       *d = dest;

          for (x = 0; x < roi->width; x++)
            {
              /*  the cursor pixels are pre-multiplied ARGB  */
              guint a = (*s >> 24) & 0xff;
              guint r = (*s >> 16) & 0xff;
              guint g = (*s >> 8)  & 0xff;
              guint b = (*s >> 0)  & 0xff;

              d[0] = a ? (r * 255) / a : r;
              d[1] = a ? (g * 255) / a : g;
              d[2] = a ? (b * 255) / a : b;
              d[3] = a;

              s++;
              d += 4;
            }

          src  += cursor->width;
          dest += 4 * roi->width;
        }
    }

  g_object_unref (buffer);

  gimp_image_insert_layer (image, layer, -1, -1);
  gimp_layer_set_offsets (layer,
                          cursor->x - cursor->xhot, cursor->y - cursor->yhot);

  gimp_image_set_active_layer (image, active);
#endif
}


/* The main Screenshot function */

gboolean
screenshot_x11_available (void)
{
  return TRUE;
}

ScreenshotCapabilities
screenshot_x11_get_capabilities (void)
{
  ScreenshotCapabilities capabilities = SCREENSHOT_CAN_PICK_NONINTERACTIVELY;

#ifdef HAVE_X11_XMU_WINUTIL_H
  capabilities |= SCREENSHOT_CAN_SHOOT_DECORATIONS;
#endif

#ifdef HAVE_XFIXES
  capabilities |= SCREENSHOT_CAN_SHOOT_POINTER;
#endif

  capabilities |= SCREENSHOT_CAN_SHOOT_REGION |
                  SCREENSHOT_CAN_SHOOT_WINDOW |
                  SCREENSHOT_CAN_PICK_WINDOW  |
                  SCREENSHOT_CAN_DELAY_WINDOW_SHOT;

  return capabilities;
}

GimpPDBStatusType
screenshot_x11_shoot (ScreenshotValues  *shootvals,
                      GdkMonitor        *monitor,
                      gint32            *image_ID,
                      GError           **error)
{
  GdkDisplay       *display;
  GdkWindow        *window;
  cairo_surface_t  *screenshot;
  cairo_region_t   *shape = NULL;
  cairo_t          *cr;
  GimpColorProfile *profile;
  GdkRectangle      rect;
  GdkRectangle      screen_rect;
  gchar            *name  = NULL;
  gint              screen_x;
  gint              screen_y;
  gint              x, y;

  /* use default screen if we are running non-interactively */
  if (monitor == NULL)
    monitor = gdk_display_get_monitor (gdk_display_get_default (), 0);

  if (shootvals->shoot_type != SHOOT_ROOT && ! shootvals->window_id)
    {
      if (shootvals->select_delay > 0)
        screenshot_delay (shootvals->select_delay);

      shootvals->window_id = select_window (shootvals, monitor);

      if (! shootvals->window_id)
        return GIMP_PDB_CANCEL;
    }

  if (shootvals->screenshot_delay > 0)
    screenshot_delay (shootvals->screenshot_delay);

  display = gdk_monitor_get_display (monitor);

  screen_rect.x      = 0;
  screen_rect.y      = 0;
  screen_rect.width  = gdk_screen_get_width (gdk_display_get_default_screen (display));
  screen_rect.height = gdk_screen_get_height (gdk_display_get_default_screen (display));

  if (shootvals->shoot_type == SHOOT_REGION)
    {
      rect.x      = MIN (shootvals->x1, shootvals->x2);
      rect.y      = MIN (shootvals->y1, shootvals->y2);
      rect.width  = ABS (shootvals->x2 - shootvals->x1);
      rect.height = ABS (shootvals->y2 - shootvals->y1);

      monitor = gdk_display_get_monitor_at_point (display,
                                                  rect.x + rect.width  / 2,
                                                  rect.y + rect.height / 2);
    }
  else
    {
      if (shootvals->shoot_type == SHOOT_ROOT)
        {
          window = gdk_screen_get_root_window (gdk_display_get_default_screen (display));

          /* FIXME: figure monitor */
        }
      else
        {
          window = gdk_x11_window_foreign_new_for_display (display,
                                                           shootvals->window_id);

          monitor = gdk_display_get_monitor_at_window (display, window);
        }

      if (! window)
        {
          g_set_error_literal (error, 0, 0, _("Specified window not found"));
          return GIMP_PDB_EXECUTION_ERROR;
        }

      rect.width  = gdk_window_get_width (window);
      rect.height = gdk_window_get_height (window);
      gdk_window_get_origin (window, &x, &y);

      rect.x = x;
      rect.y = y;
    }

  if (! gdk_rectangle_intersect (&rect, &screen_rect, &rect))
    return GIMP_PDB_EXECUTION_ERROR;

  window = gdk_screen_get_root_window (gdk_display_get_default_screen (display));
  gdk_window_get_origin (window, &screen_x, &screen_y);

  screenshot = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
                                           rect.width, rect.height);

  cr = cairo_create (screenshot);

  gdk_cairo_set_source_window (cr, window,
                               - (rect.x - screen_x),
                               - (rect.y - screen_y));
  cairo_paint (cr);

  cairo_destroy (cr);

  gdk_display_beep (display);

  if (shootvals->shoot_type == SHOOT_WINDOW)
    {
      name = window_get_title (display, shootvals->window_id);

      shape = window_get_shape (monitor, shootvals->window_id);

      if (shape)
        cairo_region_translate (shape, x - rect.x, y - rect.y);
    }

  *image_ID = create_image (screenshot, shape, name);

  cairo_surface_destroy (screenshot);

  if (shape)
    cairo_region_destroy (shape);

  g_free (name);

  /* FIXME: Some time might have passed until we get here.
   *        The cursor image should be grabbed together with the screenshot.
   */
  if ((shootvals->shoot_type == SHOOT_ROOT ||
       shootvals->shoot_type == SHOOT_WINDOW) && shootvals->show_cursor)
    add_cursor_image (*image_ID, display);

  profile = gimp_monitor_get_color_profile (monitor);

  if (profile)
    {
      gimp_image_set_color_profile (*image_ID, profile);
      g_object_unref (profile);
    }

  return GIMP_PDB_SUCCESS;
}

#endif /* GDK_WINDOWING_X11 */
