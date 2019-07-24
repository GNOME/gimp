
#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpwidgets-private.h"

#include "shadow.h"
#include "units.h"
#include "widgets.h"


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
take_window_shot (GtkWidget *widget)
{
  GtkAllocation    allocation;
  cairo_surface_t *surface;
  cairo_t         *cr;
  GdkPixbuf       *pixbuf;

  allocation.x = 0;
  allocation.y = 0;
  gtk_widget_get_preferred_width  (widget, NULL, &allocation.width);
  gtk_widget_get_preferred_height (widget, NULL, &allocation.height);

  gtk_widget_size_allocate (widget, &allocation);
  gtk_widget_get_allocation (widget, &allocation);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        allocation.width,
                                        allocation.height);
  cr = cairo_create (surface);
  gtk_widget_draw (widget, cr);
  cairo_destroy (cr);

  pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0,
                                        allocation.width,
                                        allocation.height);

  cairo_surface_destroy (surface);

  if (GTK_IS_OFFSCREEN_WINDOW (widget))
    {
      GdkPixbuf *tmp;

      tmp = add_border_to_shot (pixbuf);

      g_object_unref (pixbuf);

      pixbuf = create_shadowed_pixbuf (tmp);

      g_object_unref (tmp);
    }

  return pixbuf;
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
main (int    argc,
      char **argv)
{
  GList *toplevels;
  GList *node;

  g_set_application_name ("GIMP documentation shooter");

  /* If there's no DISPLAY, we silently error out.
   * We don't want to break headless builds.
   */
  if (! gtk_init_check (&argc, &argv))
    return EXIT_SUCCESS;

  units_init ();

  gimp_widgets_init (shooter_standard_help,
                     shooter_get_foreground,
                     shooter_get_background,
                     shooter_ensure_modules);

  toplevels = get_all_widgets ();

  for (node = toplevels; node; node = g_list_next (node))
    {
      WidgetInfo *info       = node->data;
      GdkPixbuf  *screenshot = NULL;

      gtk_widget_show (info->widget);
      gtk_widget_queue_draw (info->widget);

      while (gtk_events_pending ())
        gtk_main_iteration ();

      screenshot = take_window_shot (info->widget);

      if (screenshot)
        {
          gchar *filename;

          filename = g_strdup_printf ("%s.png", info->name);
          gdk_pixbuf_save (screenshot, filename, "png", NULL, NULL);
          g_free(filename);

          g_object_unref (screenshot);
        }

      gtk_widget_hide (info->widget);
    }

  return EXIT_SUCCESS;
}
