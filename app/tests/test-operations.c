/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 2010 Øyvind Kolås <pippin@gimp.org>
 *               2012 Ville Sokk   <ville.sokk@gmail.com>
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

#include <glib.h>
#include <gegl.h>
#include <gegl-plugin.h>
#include <math.h>
#include <string.h>

#include "app/operations/gimp-operations.h"

#define DATA_DIR "files"
#define OUTPUT_DIR "operations-output"


static inline gfloat
square (gfloat x)
{
  return x * x;
}

/*
 * image comparison function from GEGL
 */
static gboolean
image_compare (gchar *composition_path,
               gchar *reference_path)
{
  GeglBuffer *bufferA   = NULL;
  GeglBuffer *bufferB   = NULL;
  GeglBuffer *debug_buf = NULL;
  gboolean    result    = TRUE;

  {
    GeglNode *graph, *sink;
    graph = gegl_graph (sink=gegl_node ("gegl:buffer-sink", "buffer", &bufferA, NULL,
                                        gegl_node ("gegl:load", "path", composition_path, NULL)));
    gegl_node_process (sink);
    g_object_unref (graph);
    if (!bufferA)
      {
        g_printerr ("\nFailed to open %s\n", composition_path);
        return FALSE;
      }

    graph = gegl_graph (sink=gegl_node ("gegl:buffer-sink", "buffer", &bufferB, NULL,
                                        gegl_node ("gegl:load", "path", reference_path, NULL)));
    gegl_node_process (sink);
    g_object_unref (graph);
    if (!bufferB)
      {
        g_printerr ("\nFailed to open %s\n", reference_path);
        return FALSE;
      }
  }

  if (gegl_buffer_get_width (bufferA) != gegl_buffer_get_width (bufferB) ||
      gegl_buffer_get_height (bufferA) != gegl_buffer_get_height (bufferB))
    {
      g_printerr ("\nBuffers differ in size\n");
      g_printerr ("  %ix%i vs %ix%i\n",
                  gegl_buffer_get_width (bufferA), gegl_buffer_get_height (bufferA),
                  gegl_buffer_get_width (bufferB), gegl_buffer_get_height (bufferB));

      return FALSE;
    }

  debug_buf = gegl_buffer_new (gegl_buffer_get_extent (bufferA), babl_format ("R'G'B' u8"));

  {
     gfloat  *bufA, *bufB;
     gfloat  *a, *b;
     guchar  *debug, *d;
     gint     rowstrideA, rowstrideB, dRowstride;
     gint     pixels;
     gint     wrong_pixels = 0;
     gint     i;
     gdouble  diffsum = 0.0;
     gdouble  max_diff = 0.0;

     pixels = gegl_buffer_get_pixel_count (bufferA);

     bufA = (void*)gegl_buffer_linear_open (bufferA, NULL, &rowstrideA,
                                            babl_format ("CIE Lab float"));
     bufB = (void*)gegl_buffer_linear_open (bufferB, NULL, &rowstrideB,
                                            babl_format ("CIE Lab float"));
     debug = (void*)gegl_buffer_linear_open (debug_buf, NULL, &dRowstride,
                                             babl_format ("R'G'B' u8"));

     a = bufA;
     b = bufB;
     d = debug;

     for (i=0; i<pixels; i++)
       {
         gdouble diff = sqrt (square (a[0]-b[0])+
                              square (a[1]-b[1])+
                              square (a[2]-b[2])
                              /*+square (a[3]-b[3])*/);
         if (diff >= 0.01)
           {
             wrong_pixels++;
             diffsum += diff;
             if (diff > max_diff)
               max_diff = diff;
             d[0] = (diff/100.0*255);
             d[1] = 0;
             d[2] = a[0]/100.0*255;
           }
         else
           {
             d[0] = a[0]/100.0*255;
             d[1] = a[0]/100.0*255;
             d[2] = a[0]/100.0*255;
           }
         a += 3;
         b += 3;
         d += 3;
       }

     a = bufA;
     b = bufB;
     d = debug;

     if (wrong_pixels)
       for (i = 0; i < pixels; i++)
         {
           gdouble diff = sqrt (square (a[0]-b[0])+
                                square (a[1]-b[1])+
                                square (a[2]-b[2])
                                /*+square (a[3]-b[3])*/);
           if (diff >= 0.01)
             {
               d[0] = (100-a[0])/100.0*64+32;
               d[1] = (diff/max_diff * 255);
               d[2] = 0;
             }
           else
             {
               d[0] = a[0]/100.0*255;
               d[1] = a[0]/100.0*255;
               d[2] = a[0]/100.0*255;
             }
           a += 3;
           b += 3;
           d += 3;
         }

     gegl_buffer_linear_close (bufferA, bufA);
     gegl_buffer_linear_close (bufferB, bufB);
     gegl_buffer_linear_close (debug_buf, debug);

     if (max_diff >= 0.1)
       {
         g_print ("\nBuffers differ\n"
                  "  wrong pixels   : %i/%i (%2.2f%%)\n"
                  "  max Δe         : %2.3f\n"
                  "  avg Δe (wrong) : %2.3f(wrong) %2.3f(total)\n",
                  wrong_pixels, pixels, (wrong_pixels*100.0/pixels),
                  max_diff,
                  diffsum/wrong_pixels,
                  diffsum/pixels);

         result = FALSE;

         if (max_diff > 1.5)
           {
             GeglNode *graph, *sink;
             gchar    *debug_path = g_malloc (strlen (composition_path)+16);
             gint      ext_length = strlen (strrchr (composition_path, '.'));

             memcpy (debug_path, composition_path, strlen (composition_path)+1);
             memcpy (debug_path + strlen(composition_path)-ext_length, "-diff.png", 11);
             graph = gegl_graph (sink=gegl_node ("gegl:png-save",
                                                 "path", debug_path, NULL,
                                                 gegl_node ("gegl:buffer-source",
                                                            "buffer", debug_buf, NULL)));
             gegl_node_process (sink);
             g_object_unref (graph);
             g_object_unref (debug_buf);
           }
       }
  }

  g_object_unref (debug_buf);
  g_object_unref (bufferA);
  g_object_unref (bufferB);

  return result;
}

static gboolean
process_operations (GType type)
{
  GType    *operations;
  gboolean  result = TRUE;
  guint     count;
  gint      i;

  operations = g_type_children (type, &count);

  if (!operations)
    {
      g_free (operations);
      return TRUE;
    }

  for (i = 0; i < count; i++)
    {
      GeglOperationClass *operation_class;
      const gchar        *image, *xml;

      operation_class = g_type_class_ref (operations[i]);
      image = gegl_operation_class_get_key (operation_class, "reference-image");
      xml = gegl_operation_class_get_key (operation_class, "reference-composition");

      if (image && xml)
        {
          gchar    *root        = g_get_current_dir ();
          gchar    *image_path  = g_build_path (G_DIR_SEPARATOR_S, root, DATA_DIR, image, NULL);
          gchar    *xml_path    = g_build_path (G_DIR_SEPARATOR_S, root, DATA_DIR, xml, NULL);
          gchar    *output_path = g_build_path (G_DIR_SEPARATOR_S, root, OUTPUT_DIR, image, NULL);
          GeglNode *composition, *output;

          g_printf ("%s: ", gegl_operation_class_get_key (operation_class, "name"));

          if (!g_file_test (xml_path, G_FILE_TEST_EXISTS))
            {
              g_printerr ("\nCan't locate %s\n", xml_path);
              result = FALSE;
            }

          composition = gegl_node_new_from_file (xml_path);
          if (!composition)
            {
              g_printerr ("\nComposition graph is flawed\n");
              result = FALSE;
            }
          else
            {
              output = gegl_node_new_child (composition,
                                            "operation", "gegl:save",
                                            "path", output_path,
                                            NULL);
              gegl_node_connect_to (composition, "output", output, "input");
              gegl_node_process (output);

              if (image_compare (output_path, image_path))
                {
                  g_printf ("PASS\n");
                  result = TRUE;
                }
              else
                {
                  g_printf ("FAIL\n");
                  result = FALSE;
                }
            }

          g_object_unref (composition);
          g_free (root);
          g_free (image_path);
          g_free (xml_path);
          g_free (output_path);
        }

      result = result && process_operations(operations[i]);
    }

  g_free (operations);

  return result;
}

/**
 * test_operations:
 *
 * Test GIMP's GEGL operations that supply a reference image
 * and composition xml.
 **/
static void
test_operations (void)
{
  gint result;

  putchar ('\n');
  result = process_operations (GEGL_TYPE_OPERATION);
  g_assert_cmpint (result, ==, TRUE);
}

gint
main (gint     argc,
      gchar ** argv)
{
  gint  result;

  g_thread_init (NULL);
  gegl_init (&argc, &argv);
  gimp_operations_init ();
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gimp-operations", test_operations);

  result = g_test_run ();

  gegl_exit ();

  return result;
}

