
#include "config.h"

#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gegl.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <X11/extensions/shape.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpwidgets-private.h"

#include "shadow.h"
#include "units.h"
#include "widgets.h"


static Window
find_toplevel_window (Display *display,
                      Window   xid)
{
  Window  root, parent, *children;
  guint   nchildren;

  do
    {
      if (XQueryTree (display, xid,
                      &root, &parent, &children, &nchildren) == 0)
        {
          g_warning ("Couldn't find window manager window");
          return 0;
        }

      if (root == parent)
        return xid;

      xid = parent;
    }
  while (TRUE);
}

static GdkPixbuf *
add_border_to_shot (GdkPixbuf *pixbuf)
{
  GdkPixbuf *retval;

  retval = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           gdk_pixbuf_get_width (pixbuf) + 2,
                           gdk_pixbuf_get_height (pixbuf) + 2);

  /* Fill with solid black */
  gdk_pixbuf_fill (retval, 0x000000FF);

  gdk_pixbuf_copy_area (pixbuf,
                        0, 0,
                        gdk_pixbuf_get_width (pixbuf),
                        gdk_pixbuf_get_height (pixbuf),
                        retval, 1, 1);

  return retval;
}

static GdkPixbuf *
remove_shaped_area (GdkPixbuf *pixbuf,
                    Window     window)
{
  Display    *display;
  GdkPixbuf  *retval;
  XRectangle *rectangles;
  gint        rectangle_count, rectangle_order;
  gint        i;

  retval = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           gdk_pixbuf_get_width (pixbuf),
                           gdk_pixbuf_get_height (pixbuf));

  gdk_pixbuf_fill (retval, 0);

  display = gdk_x11_display_get_xdisplay (gdk_display_get_default ());

  rectangles = XShapeGetRectangles (display, window, ShapeBounding,
                                    &rectangle_count, &rectangle_order);

  for (i = 0; i < rectangle_count; i++)
    {
      int y, x;

      for (y = rectangles[i].y;
           y < rectangles[i].y + rectangles[i].height;
           y++)
        {
          const guchar *src_pixels;
          guchar       *dest_pixels;

          src_pixels = gdk_pixbuf_get_pixels (pixbuf) +
            y * gdk_pixbuf_get_rowstride (pixbuf) +
            rectangles[i].x * (gdk_pixbuf_get_has_alpha (pixbuf) ? 4 : 3);

          dest_pixels = gdk_pixbuf_get_pixels (retval) +
            y * gdk_pixbuf_get_rowstride (retval) +
            rectangles[i].x * 4;

          for (x = rectangles[i].x;
               x < rectangles[i].x + rectangles[i].width;
               x++)
            {
              *dest_pixels++ = *src_pixels ++;
              *dest_pixels++ = *src_pixels ++;
              *dest_pixels++ = *src_pixels ++;
              *dest_pixels++ = 255;

              if (gdk_pixbuf_get_has_alpha (pixbuf))
                src_pixels++;
            }
        }
    }

  return retval;
}

static GdkPixbuf *
take_window_shot (Window   child,
                  gboolean include_decoration)
{
  GdkDisplay *display;
  GdkScreen  *screen;
  GdkWindow  *window;
  Window      xid;
  gint        x_orig, y_orig;
  gint        x = 0, y = 0;
  gint        width, height;
  GdkPixbuf  *tmp, *tmp2;
  GdkPixbuf  *retval;

  display = gdk_display_get_default ();
  screen = gdk_screen_get_default ();

  if (include_decoration)
    xid = find_toplevel_window (gdk_x11_display_get_xdisplay (display), child);
  else
    xid = child;

  window = gdk_x11_window_foreign_new_for_display (display, xid);

  width  = gdk_window_get_width  (window);
  height = gdk_window_get_height (window);
  gdk_window_get_origin (window, &x_orig, &y_orig);

  if (x_orig < 0)
    {
      x = - x_orig;
      width = width + x_orig;
      x_orig = 0;
    }

  if (y_orig < 0)
    {
      y = - y_orig;
      height = height + y_orig;
      y_orig = 0;
    }

  if (x_orig + width > gdk_screen_get_width (screen))
    width = gdk_screen_get_width (screen) - x_orig;

  if (y_orig + height > gdk_screen_get_height (screen))
    height = gdk_screen_get_height (screen) - y_orig;

  tmp = gdk_pixbuf_get_from_window (window,
                                    x, y, width, height);

  if (include_decoration)
    tmp2 = remove_shaped_area (tmp, xid);
  else
    tmp2 = add_border_to_shot (tmp);

  retval = create_shadowed_pixbuf (tmp2);

  g_object_unref (tmp);
  g_object_unref (tmp2);

  return retval;
}

static gboolean
shooter_get_foreground (GimpRGB *color)
{
  color->r = color->g = color->b = 0.0;
  color->a = 1.0;
  return TRUE;
}

static gboolean
shooter_get_background (GimpRGB *color)
{
  color->r = color->g = color->b = 1.0;
  color->a = 1.0;
  return TRUE;
}

static void
shooter_standard_help (const gchar *help_id,
                       gpointer     help_data)
{
}

static void
shooter_ensure_modules (void)
{
  static GimpModuleDB *module_db = NULL;

  if (! module_db)
    {
      gchar *config = gimp_config_build_plug_in_path ("modules");
      gchar *path   = gimp_config_path_expand (config, TRUE, NULL);

      module_db = gimp_module_db_new (FALSE);
      gimp_module_db_load (module_db, path);

      g_free (path);
      g_free (config);
    }
}

int
main (int argc, char **argv)
{
  GdkPixbuf *screenshot = NULL;
  GList     *toplevels;
  GList     *node;

  g_set_application_name ("GIMP documention shooter");

  /* If there's no DISPLAY, we silently error out.
   * We don't want to break headless builds.
   */
  if (! gtk_init_check (&argc, &argv))
    return EXIT_SUCCESS;

  gtk_rc_add_default_file (gimp_gtkrc ());

  units_init ();

  gimp_widgets_init (shooter_standard_help,
                     shooter_get_foreground,
                     shooter_get_background,
                     shooter_ensure_modules);

  toplevels = get_all_widgets ();

  for (node = toplevels; node; node = g_list_next (node))
    {
      GdkWindow  *window;
      WidgetInfo *info;
      XID         xid;
      gchar      *filename;

      info = node->data;

      gtk_widget_show (info->window);

      window = gtk_widget_get_window (info->window);

      gtk_widget_show_now (info->window);
      gtk_widget_queue_draw (info->window);

      while (gtk_events_pending ())
        {
          gtk_main_iteration ();
        }
      sleep (1);

      while (gtk_events_pending ())
        {
          gtk_main_iteration ();
        }

      xid = gdk_x11_window_get_xid (window);
      screenshot = take_window_shot (xid, info->include_decorations);

      filename = g_strdup_printf ("%s.png", info->name);
      gdk_pixbuf_save (screenshot, filename, "png", NULL, NULL);
      g_free(filename);

      gtk_widget_hide (info->window);
    }

  return EXIT_SUCCESS;
}
