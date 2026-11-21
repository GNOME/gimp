/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * macOS Scanner plug-in (ported from GimpScan)
 *
 * Copyright (C) 2020 Simul Piscator
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

#include <AvailabilityMacros.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <Quartz/Quartz.h>
#import <ImageCaptureCore/ImageCaptureCore.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_NAME "ik-acquire"

typedef struct _Scanner      Scanner;
typedef struct _ScannerClass ScannerClass;

struct _Scanner
{
  GimpPlugIn      parent_instance;
};

struct _ScannerClass
{
  GimpPlugInClass parent_class;
};


#define SCANNER_TYPE  (scanner_get_type ())
#define SCANNER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SCANNER_TYPE, Scanner))

GType                   scanner_get_type          (void);

static GList          * scanner_query_procedures  (GimpPlugIn           *plug_in);
static GimpProcedure  * scanner_create_procedure  (GimpPlugIn           *plug_in,
                                                   const gchar          *name);

static GimpValueArray * scanner_run               (GimpProcedure        *procedure,
                                                   GimpRunMode           run_mode,
                                                   GimpImage            *image,
                                                   GimpDrawable        **drawables,
                                                   GimpProcedureConfig  *config,
                                                   gpointer              run_data);

G_DEFINE_TYPE (Scanner, scanner, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (SCANNER_TYPE)
DEFINE_STD_SET_I18N


#if MAC_OS_X_VERSION_MIN_REQUIRED >= 101400 /* NSAppearanceNameDarkAqua */
static void
scanner_update_appearance (void)
{
  GFile    *file;
  gchar    *contents;
  gboolean  prefer_dark_theme = FALSE;

  file = gimp_directory_file ("theme.css", NULL);
  if (g_file_load_contents (file, NULL, &contents, NULL, NULL, NULL))
    {
      prefer_dark_theme = strstr (contents, "/* prefer-dark-theme */") != NULL;
      g_free (contents);
    }
  g_object_unref (file);

  if (prefer_dark_theme)
    [NSApp setAppearance: [NSAppearance appearanceNamed: NSAppearanceNameDarkAqua]];
  else
    [NSApp setAppearance: [NSAppearance appearanceNamed: NSAppearanceNameAqua]];
}
#endif

static NSString *
scanner_next_document_name (void)
{
  NSString *doc_name = [NSString stringWithFormat: @"Scan-%@", [[NSProcessInfo processInfo] globallyUniqueString]];

  [[NSUserDefaults standardUserDefaults] setObject: doc_name forKey: @"IK_documentName"];

  return doc_name;
}


/* macOS methods */
@interface GimpScannerDelegate : NSObject <NSApplicationDelegate, IKScannerDeviceViewDelegate, ICDeviceBrowserDelegate>
{
  ICDeviceBrowser     *_scanner_browser;
  IKScannerDeviceView *_scanner_container;
}
@property (retain) ICDeviceBrowser     *scanner_browser;
@property (assign) IKScannerDeviceView *scanner_container;
@end

@implementation GimpScannerDelegate
@synthesize scanner_browser   = _scanner_browser;
@synthesize scanner_container = _scanner_container;

- (void) applicationDidFinishLaunching: (NSNotification *)notification
{
  NSRect               frame;
  NSWindow            *scanner_window;
  IKScannerDeviceView *scanner_container;
  NSString            *doc_name;
  gchar               *title;

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 101400 /* NSAppearanceNameDarkAqua */
  scanner_update_appearance ();
#endif

  /* reset persisted ImageKit name defaults before creating view */
  doc_name = scanner_next_document_name ();

  /* discover local and network scanners via ImageCaptureCore */
  self.scanner_browser = [[ICDeviceBrowser alloc] init];
  self.scanner_browser.delegate = self;
  self.scanner_browser.browsedDeviceTypeMask =
    (ICDeviceTypeMask) ((NSUInteger) ICDeviceTypeMaskScanner       |
                        (NSUInteger) ICDeviceLocationTypeMaskLocal |
                        (NSUInteger) ICDeviceLocationTypeMaskShared |
                        (NSUInteger) ICDeviceLocationTypeMaskBonjour |
                        (NSUInteger) ICDeviceLocationTypeMaskBluetooth);
  [self.scanner_browser start];

  /* create native window with ImageKit widgets */
  frame  = NSMakeRect (0, 0, 640, 480);
  scanner_window = [[NSWindow alloc] initWithContentRect: frame
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 101200
                                               styleMask: NSWindowStyleMaskTitled   |
                                                          NSWindowStyleMaskClosable |
                                                          NSWindowStyleMaskResizable
#else
                                               styleMask: NSTitledWindowMask   |
                                                          NSClosableWindowMask |
                                                          NSResizableWindowMask
#endif
                                                 backing: NSBackingStoreBuffered
                                                   defer: NO];
  title = gimp_strip_uline (_("_Scanner/Camera..."));
  if (g_str_has_suffix (title, "...") ||
      g_str_has_suffix (title, "\xe2\x80\xa6"))
    title[strlen (title) - 3] = '\0';
  [scanner_window setTitle: [NSString stringWithUTF8String: title]];
  g_free (title);
  [scanner_window setLevel: NSFloatingWindowLevel];
  [scanner_window center];
  scanner_container = [[IKScannerDeviceView alloc] initWithFrame: scanner_window.contentView.bounds];
  scanner_container.autoresizingMask       = NSViewWidthSizable | NSViewHeightSizable;

  /* set displaysDownloadsDirectoryControl otherwise we get 'folder is not writable' error */
  scanner_container.documentName                      = doc_name;
  scanner_container.displaysDownloadsDirectoryControl = YES;
  scanner_container.postProcessApplication            = nil;
  scanner_container.delegate                          = self;
  /* enable display modes before setting mode, otherwise it stays None */
  scanner_container.hasDisplayModeSimple              = YES;
  scanner_container.hasDisplayModeAdvanced            = YES;
  scanner_container.mode                              = IKScannerDeviceViewDisplayModeAdvanced;
  scanner_container.transferMode                      = IKScannerDeviceViewTransferModeFileBased;
  self.scanner_container                              = scanner_container;

  [scanner_window.contentView addSubview: scanner_container];
  [scanner_window makeKeyAndOrderFront: nil];
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 140000
  if (@available(macOS 14.0, *))
    {
      [NSApp activate];
    }
  else
#endif
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      [NSApp activateIgnoringOtherApps: YES];
#pragma clang diagnostic pop
    }
}

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 101400 /* NSAppearanceNameDarkAqua */
- (void) applicationDidBecomeActive: (NSNotification *)notification
{
  scanner_update_appearance ();
}
#endif

- (void) deviceBrowser: (ICDeviceBrowser *)browser
          didAddDevice: (ICDevice *)device
            moreComing: (BOOL)moreComing
{
  /* auto-select the first discovered scanner device */
  if (self.scanner_container != nil               &&
      self.scanner_container.scannerDevice == nil &&
      [device isKindOfClass: [ICScannerDevice class]])
    {
      self.scanner_container.scannerDevice = (ICScannerDevice *) device;
    }
}

- (void) deviceBrowser: (ICDeviceBrowser *)browser
       didRemoveDevice: (ICDevice *)device
             moreGoing: (BOOL)moreGoing
{
  /* clear active scanner if it gets disconnected */
  if (self.scanner_container != nil &&
      self.scanner_container.scannerDevice == (ICScannerDevice *) device)
    {
      self.scanner_container.scannerDevice = nil;
    }
}

- (BOOL) applicationShouldTerminateAfterLastWindowClosed: (NSApplication *)app
{
  return YES;
}

- (NSApplicationTerminateReply) applicationShouldTerminate: (NSApplication *)app
{
  [app stop: nil];
  [app postEvent: [NSEvent otherEventWithType:
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 101200
                                                NSEventTypeApplicationDefined
#else
                                                NSApplicationDefined
#endif
                                      location: NSZeroPoint
                                 modifierFlags: 0
                                     timestamp: 0
                                  windowNumber: 0
                                       context: nil
                                       subtype: 0
                                         data1: 0
                                         data2: 0]
         atStart: YES];

  return NSTerminateCancel;
}

- (void) scannerDeviceView: (IKScannerDeviceView *)scannerDeviceView
              didScanToURL: (NSURL *)url
                  fileData: (NSData *)data
                     error: (NSError *)error
{
  if (url)
    {
      const char *filename = [[url path] UTF8String];
      GFile      *file     = g_file_new_for_path (filename);
      GimpImage  *image    = gimp_file_load (GIMP_RUN_NONINTERACTIVE, file);

      g_object_unref (file);

      if (image)
        gimp_display_new (image);

      g_unlink (filename);
      gimp_progress_end ();

      /* reset document name for the next scan */
      scannerDeviceView.documentName = scanner_next_document_name ();

      /* bring GIMP back to the foreground after importing the scan (bad workflow?)
      gimp_pid = gimp_getpid ();
      gimp_app = [NSRunningApplication runningApplicationWithProcessIdentifier: gimp_pid];
      if (gimp_app)
        {
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 140000
          if (@available(macOS 14.0, *))
            {
              [[NSApplication sharedApplication] yieldActivationToApplication:gimp_app];
              [gimp_app activateFromApplication:[NSRunningApplication currentApplication] options:0];
            }
          else
#endif
            {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
              [gimp_app activateWithOptions:NSApplicationActivateIgnoringOtherApps];
#pragma clang diagnostic pop
            }
        }*/
    }
}
@end


static void
scanner_class_init (ScannerClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = scanner_query_procedures;
  plug_in_class->create_procedure = scanner_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
scanner_init (Scanner *scanner)
{
}

static GList *
scanner_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_NAME));
}

static GimpProcedure *
scanner_create_procedure (GimpPlugIn  *plug_in,
                          const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_NAME))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            scanner_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE     |
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES    |
                                           GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES |
                                           GIMP_PROCEDURE_SENSITIVE_NO_IMAGE);

      gimp_procedure_set_menu_label (procedure, _("_Scanner/Camera..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/File/Create");

      gimp_procedure_set_documentation (procedure, NULL, NULL, name);
      gimp_procedure_set_attribution (procedure,
                                      "Simul Piscator",
                                      "Simul Piscator",
                                      "2020");
    }

  return procedure;
}

static GimpValueArray *
scanner_run (GimpProcedure        *procedure,
             GimpRunMode           run_mode,
             GimpImage            *image,
             GimpDrawable        **drawables,
             GimpProcedureConfig  *config,
             gpointer              run_data)
{
  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_NONINTERACTIVE)
    /* Currently, we don't do non-interactive calls.
     * Bail if someone tries to call us non-interactively
     */
    return gimp_procedure_new_return_values (procedure, GIMP_PDB_CALLING_ERROR, NULL);

  /* run as an accessory app on the main thread so the window can be focused */
  @autoreleasepool
    {
      NSApplication       *app      = [NSApplication sharedApplication];
      GimpScannerDelegate *delegate = [GimpScannerDelegate new];

      [app setActivationPolicy: NSApplicationActivationPolicyAccessory];
      [app setDelegate: delegate];
      [app run];
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}
