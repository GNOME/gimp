/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
 * test-preview-area.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

/* This code is based on testrgb.c from GTK+ - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 */

#include <config.h>

#include <stdlib.h>

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimppreviewarea.h"


#define WIDTH      256
#define HEIGHT     256
#define NUM_ITERS  100


static void
test_run (GtkWidget *area,
          gboolean   visible)
{
  guchar        buf[WIDTH * HEIGHT * 8];
  gint          i, j;
  gint          num_iters = NUM_ITERS;
  guchar        val;
  gdouble       start_time, total_time;
  GTimer       *timer;
  GEnumClass   *enum_class;
  GEnumValue   *enum_value;

  if (! visible)
    num_iters *= 4;

  gtk_widget_realize (area);

  g_print ("\nPerformance tests for GimpPreviewArea "
           "(%d x %d, %s, %d iterations):\n\n",
           WIDTH, HEIGHT, visible ? "visible" : "hidden", num_iters);

  val = 0;
  for (j = 0; j < WIDTH * HEIGHT * 8; j++)
    {
      val = (val + ((val + (rand () & 0xff)) >> 1)) >> 1;
      buf[j] = val;
    }

  gimp_preview_area_set_colormap (GIMP_PREVIEW_AREA (area), buf, 256);

  /* Let's warm up the cache, and also wait for the window manager
     to settle. */
  for (i = 0; i < NUM_ITERS; i++)
    {
      gint offset = (rand () % (WIDTH * HEIGHT * 4)) & -4;

      gimp_preview_area_draw (GIMP_PREVIEW_AREA (area),
                              0, 0, WIDTH, HEIGHT,
                              GIMP_RGB_IMAGE,
                              buf + offset,
                              WIDTH * 4);
    }

  gdk_display_flush (gtk_widget_get_display (area));

  timer = g_timer_new ();

  enum_class = g_type_class_ref (GIMP_TYPE_IMAGE_TYPE);

  for (enum_value = enum_class->values; enum_value->value_name; enum_value++)
    {
      /* gimp_preview_area_draw */
      start_time = g_timer_elapsed (timer, NULL);

      for (i = 0; i < num_iters; i++)
        {
          gint offset = (rand () % (WIDTH * HEIGHT * 4)) & -4;

          gimp_preview_area_draw (GIMP_PREVIEW_AREA (area),
                                  0, 0, WIDTH, HEIGHT,
                                  enum_value->value,
                                  buf + offset,
                                  WIDTH * 4);
        }

      gdk_display_flush (gtk_widget_get_display (area));
      total_time = g_timer_elapsed (timer, NULL) - start_time;

      g_print ("%-20s "
               "draw  :  %5.2fs, %8.1f fps, %8.2f megapixels/s\n",
               enum_value->value_name,
               total_time,
               num_iters / total_time,
               num_iters * (WIDTH * HEIGHT * 1e-6) / total_time);

      /* gimp_preview_area_blend */
      start_time = g_timer_elapsed (timer, NULL);

      for (i = 0; i < num_iters; i++)
        {
          gint offset  = (rand () % (WIDTH * HEIGHT * 4)) & -4;
          gint offset2 = (rand () % (WIDTH * HEIGHT * 4)) & -4;

          gimp_preview_area_blend (GIMP_PREVIEW_AREA (area),
                                   0, 0, WIDTH, HEIGHT,
                                   enum_value->value,
                                   buf + offset,
                                   WIDTH * 4,
                                   buf + offset2,
                                   WIDTH * 4,
                                   rand () & 0xFF);
        }

      gdk_display_flush (gtk_widget_get_display (area));
      total_time = g_timer_elapsed (timer, NULL) - start_time;

      g_print ("%-20s "
               "blend :  %5.2fs, %8.1f fps, %8.2f megapixels/s\n",
               enum_value->value_name,
               total_time,
               num_iters / total_time,
               num_iters * (WIDTH * HEIGHT * 1e-6) / total_time);

      /* gimp_preview_area_mask */
      start_time = g_timer_elapsed (timer, NULL);

      for (i = 0; i < num_iters; i++)
        {
          gint offset  = (rand () % (WIDTH * HEIGHT * 4)) & -4;
          gint offset2 = (rand () % (WIDTH * HEIGHT * 4)) & -4;
          gint offset3 = (rand () % (WIDTH * HEIGHT * 4)) & -4;

          gimp_preview_area_mask (GIMP_PREVIEW_AREA (area),
                                  0, 0, WIDTH, HEIGHT,
                                  enum_value->value,
                                  buf + offset,
                                  WIDTH * 4,
                                  buf + offset2,
                                  WIDTH * 4,
                                  buf + offset3,
                                  WIDTH);
        }

      gdk_display_flush (gtk_widget_get_display (area));
      total_time = g_timer_elapsed (timer, NULL) - start_time;

      g_print ("%-20s "
               "mask  :  %5.2fs, %8.1f fps, %8.2f megapixels/s\n",
               enum_value->value_name,
               total_time,
               num_iters / total_time,
               num_iters * (WIDTH * HEIGHT * 1e-6) / total_time);
      g_print ("\n");
    }

  start_time = g_timer_elapsed (timer, NULL);

  for (i = 0; i < num_iters; i++)
    {
      guchar  r = rand () % 0xFF;
      guchar  g = rand () % 0xFF;
      guchar  b = rand () % 0xFF;

      gimp_preview_area_fill (GIMP_PREVIEW_AREA (area),
                              0, 0, WIDTH, HEIGHT,
                              r, g, b);
    }

  gdk_display_flush (gtk_widget_get_display (area));
  total_time = g_timer_elapsed (timer, NULL) - start_time;
  g_print ("%-20s "
           "fill  :  %5.2fs, %8.1f fps, %8.2f megapixels/s\n",
           "Color fill",
           total_time,
           num_iters / total_time,
           num_iters * (WIDTH * HEIGHT * 1e-6) / total_time);
  g_print ("\n");
}

static void
test_preview_area (void)
{
  GtkWidget *window;
  GtkWidget *area;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_accept_focus (GTK_WINDOW (window), FALSE);

  area = gimp_preview_area_new ();
  gtk_container_add (GTK_CONTAINER (window), area);
  gtk_widget_show (area);

  test_run (area, FALSE);

  gtk_widget_show (window);

  test_run (area, TRUE);
}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  test_preview_area ();

  return EXIT_SUCCESS;
}
