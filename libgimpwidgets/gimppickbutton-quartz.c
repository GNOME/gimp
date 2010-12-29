/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppickbutton-quartz.c
 * Copyright (C) 2015 Kristian Rietveld <kris@loopnest.org>
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"
#include "gimppickbutton.h"
#include "gimppickbutton-private.h"

#include "cursors/gimp-color-picker-cursors.c"

#ifdef GDK_WINDOWING_QUARTZ
#import <AppKit/AppKit.h>
#include <Carbon/Carbon.h>  /* For virtual key codes ... */
#include <ApplicationServices/ApplicationServices.h>
#endif


void   _gimp_pick_button_quartz_pick (GimpPickButton *button);


@interface GimpPickWindowController : NSObject
{
  GimpPickButton *button;
  NSMutableArray *windows;
}

@property (nonatomic, assign) BOOL firstBecameKey;
@property (readonly, retain) NSCursor *cursor;

- (id)initWithButton:(GimpPickButton *)_button;
- (void)updateKeyWindow;
- (void)shutdown;
@end

@interface GimpPickView : NSView
{
  GimpPickButton *button;
  GimpPickWindowController *controller;
}

@property (readonly,assign) NSTrackingArea *area;

- (id)initWithButton:(GimpPickButton *)_button controller:(GimpPickWindowController *)controller;
@end

@implementation GimpPickView

@synthesize area;

- (id)initWithButton:(GimpPickButton *)_button controller:(GimpPickWindowController *)_controller
{
  self = [super init];

  if (self)
    {
      button = _button;
      controller = _controller;
    }

  return self;
}

- (void)dealloc
{
  [self removeTrackingArea:self.area];

  [super dealloc];
}

- (void)viewDidMoveToWindow
{
  NSTrackingAreaOptions options;

  [super viewDidMoveToWindow];

  if ([self window] == nil)
    return;

  options = NSTrackingMouseEnteredAndExited |
            NSTrackingMouseMoved |
            NSTrackingActiveAlways;

  /* Add assume inside if mouse pointer is above this window */
  if (NSPointInRect ([NSEvent mouseLocation], self.window.frame))
    options |= NSTrackingAssumeInside;

  area = [[NSTrackingArea alloc] initWithRect:self.bounds
                                 options:options
                                 owner:self
                                 userInfo:nil];
  [self addTrackingArea:self.area];
}

- (void)mouseEntered:(NSEvent *)event
{
  /* We handle the mouse cursor manually, see also the comment in
   * [GimpPickWindow windowDidBecomeMain below].
   */
  if (controller.cursor)
    [controller.cursor push];
}

- (void)mouseExited:(NSEvent *)event
{
  if (controller.cursor)
    [controller.cursor pop];

  [controller updateKeyWindow];
}

- (void)mouseMoved:(NSEvent *)event
{
  [self pickColor:event];
}

- (void)mouseUp:(NSEvent *)event
{
  [self pickColor:event];

  [controller shutdown];
}

- (void)rightMouseUp:(NSEvent *)event
{
  [self mouseUp:event];
}

- (void)otherMouseUp:(NSEvent *)event
{
  [self mouseUp:event];
}

- (void)keyDown:(NSEvent *)event
{
  if (event.keyCode == kVK_Escape)
    [controller shutdown];
}

- (void)pickColor:(NSEvent *)event
{
  CGImageRef    root_image_ref;
  CFDataRef     pixel_data;
  const guchar *data;
  GimpRGB       rgb;
  NSPoint       point;

  /* The event gives us a point in Cocoa window coordinates. The function
   * CGWindowListCreateImage expects a rectangle in screen coordinates
   * with the origin in the upper left (contrary to Cocoa). The origin is
   * on the screen showing the menu bar (this is the screen at index 0 in the
   * screens array). So, after converting the rectangle to Cocoa screen
   * coordinates, we use the height of this particular screen to translate
   * to the coordinate space expected by CGWindowListCreateImage.
   */
  point = event.locationInWindow;
  NSRect rect = NSMakeRect (point.x, point.y,
                            1, 1);
  rect = [self.window convertRectToScreen:rect];
  rect.origin.y = [[[NSScreen screens] objectAtIndex:0] frame].size.height - rect.origin.y;

  root_image_ref = CGWindowListCreateImage (rect,
                                            kCGWindowListOptionOnScreenOnly,
                                            kCGNullWindowID,
                                            kCGWindowImageDefault);
  pixel_data = CGDataProviderCopyData (CGImageGetDataProvider (root_image_ref));
  data = CFDataGetBytePtr (pixel_data);

  gimp_rgba_set_uchar (&rgb, data[2], data[1], data[0], 255);

  CGImageRelease (root_image_ref);
  CFRelease (pixel_data);

  g_signal_emit_by_name (button, "color-picked", &rgb);
}
@end


@interface GimpPickWindow : NSWindow <NSWindowDelegate>
{
  GimpPickWindowController *controller;
}

- (id)initWithButton:(GimpPickButton *)button forScreen:(NSScreen *)screen withController:(GimpPickWindowController *)_controller;
@end

@implementation GimpPickWindow
- (id)initWithButton:(GimpPickButton *)button forScreen:(NSScreen *)screen withController:(GimpPickWindowController *)_controller
{
  self = [super initWithContentRect:screen.frame
                styleMask:NSBorderlessWindowMask
                backing:NSBackingStoreBuffered
                defer:NO];

  if (self)
    {
      GimpPickView *view;

      controller = _controller;

      [self setDelegate:self];

      [self setAlphaValue:0.0];
#if 0
      /* Useful for debugging purposes */
      [self setBackgroundColor:[NSColor redColor]];
      [self setAlphaValue:0.2];
#endif
      [self setIgnoresMouseEvents:NO];
      [self setAcceptsMouseMovedEvents:YES];
      [self setHasShadow:NO];
      [self setOpaque:NO];

      /* Set the highest level, so on top of everything */
      [self setLevel:NSScreenSaverWindowLevel];

      view = [[GimpPickView alloc] initWithButton:button controller:controller];
      [self setContentView:view];
      [self makeFirstResponder:view];
      [view release];

      [self disableCursorRects];
    }

  return self;
}

/* Borderless windows cannot become key/main by default, so we force it
 * to make it so. We need this to receive events.
 */
- (BOOL)canBecomeKeyWindow
{
  return YES;
}

- (BOOL)canBecomeMainWindow
{
  return YES;
}

- (void)windowDidBecomeKey:(NSNotification *)aNotification
{
  /* We cannot use the usual Cocoa method for handling cursor updates,
   * since the GDK Quartz backend is interfering. Additionally, because
   * one of the screen-spanning windows pops up under the mouse pointer this
   * view will not receive a MouseEntered event. So, we synthesize such
   * an event here and the view can set the mouse pointer in response to
   * this. So, this event only has to be synthesized once and only for
   * the window that pops up under the mouse cursor. Synthesizing multiple
   * times messes up the mouse cursor stack.
   *
   * We cannot set the mouse pointer at this moment, because the GDK window
   * will still receive an MouseExited event in which turn it will modify
   * the cursor. So, with this synthesized event we also ensure we set
   * the mouse cursor *after* the GDK window has manipulated the cursor.
   */
  NSEvent *event;

  if (!controller.firstBecameKey ||
      !NSPointInRect ([NSEvent mouseLocation], self.frame))
    return;

  controller.firstBecameKey = NO;

  event = [NSEvent enterExitEventWithType:NSMouseEntered
                   location:[self mouseLocationOutsideOfEventStream]
                   modifierFlags:0
                   timestamp:[[NSApp currentEvent] timestamp]
                   windowNumber:self.windowNumber
                   context:nil
                   eventNumber:0
                   trackingNumber:(NSInteger)[[self contentView] area]
                   userData:nil];

  [NSApp postEvent:event atStart:NO];
}
@end


/* To properly handle multi-monitor setups we need to create a
 * GimpPickWindow for each monitor (NSScreen). This is necessary because
 * a window on Mac OS X (tested on 10.9) cannot span more than one
 * monitor, so any approach that attempts to create one large window
 * spanning all monitors cannot work. So, we have to create multiple
 * windows in case of multi-monitor setups and these different windows
 * are managed by GimpPickWindowController.
 */
@implementation GimpPickWindowController

@synthesize firstBecameKey;
@synthesize cursor;

- (id)initWithButton:(GimpPickButton *)_button;
{
  self = [super init];

  if (self)
    {
      firstBecameKey = YES;
      button = _button;
      cursor = [GimpPickWindowController makePickCursor];

      windows = [[NSMutableArray alloc] init];

      for (NSScreen *screen in [NSScreen screens])
        {
          GimpPickWindow *window;

          window = [[GimpPickWindow alloc] initWithButton:button
                                           forScreen:screen
                                           withController:self];

          [window orderFrontRegardless];
          [window makeMainWindow];

          [windows addObject:window];
        }

      [self updateKeyWindow];
    }

  return self;
}

- (void)updateKeyWindow
{
  for (GimpPickWindow *window in windows)
    {
      if (NSPointInRect ([NSEvent mouseLocation], window.frame))
        [window makeKeyWindow];
    }
}

- (void)shutdown
{
  GtkWidget *window;

  for (GimpPickWindow *window in windows)
    [window close];

  [windows release];

  if (cursor)
    [cursor release];

  /* Give focus back to the window containing the pick button */
  window = gtk_widget_get_toplevel (GTK_WIDGET (button));
  gtk_window_present_with_time (GTK_WINDOW (window), GDK_CURRENT_TIME);

  [self release];
}

+ (NSCursor *)makePickCursor
{
  GBytes    *bytes = NULL;
  GError    *error = NULL;

  bytes = g_resources_lookup_data ("/org/gimp/color-picker-cursors-raw/cursor-color-picker.png",
                                   G_RESOURCE_LOOKUP_FLAGS_NONE, &error);

  if (bytes)
    {
      NSData   *data = [NSData dataWithBytes:g_bytes_get_data (bytes, NULL)
                               length:g_bytes_get_size (bytes)];
      NSImage  *image = [[NSImage alloc] initWithData:data];
      NSCursor *cursor = [[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(1, 30)];

      [image release];
      g_bytes_unref (bytes);

      return [cursor retain];
    }
  else
    {
      g_critical ("Failed to create cursor image: %s", error->message);
      g_clear_error (&error);
    }

  return NULL;
}
@end

/* entry point to this file, called from gimppickbutton.c */
void
_gimp_pick_button_quartz_pick (GimpPickButton *button)
{
  GimpPickWindowController *controller;
  NSAutoreleasePool        *pool;

  pool = [[NSAutoreleasePool alloc] init];

  controller = [[GimpPickWindowController alloc] initWithButton:button];

  [pool release];
}
