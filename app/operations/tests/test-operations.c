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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gegl-plugin.h>

#include "libgimpmath/gimpmath.h"

#include "app/operations/gimp-operations.h"


#define DATA_DIR   "data"
#define OUTPUT_DIR "output"


static inline gdouble
square (gdouble x)
{
  return x * x;
}

static gdouble
cie76 (gfloat *src1,
       gfloat *src2)
{
  return sqrt (square (src1[0] - src2[0]) +
               square (src1[1] - src2[1]) +
               square (src1[2] - src2[2]));
}

static gdouble
cie94 (gfloat* src1,
       gfloat* src2)
{
  gdouble L1, L2, a1, b1, a2, b2, C1, C2, dL, dC, dH, dE;

  L1 = src1[0];
  a1 = src1[1];
  b1 = src1[2];
  L2 = src2[0];
  a2 = src2[1];
  b2 = src2[2];
  dL = L1 - L2;
  C1 = sqrt (square (a1) + square (b1));
  C2 = sqrt (square (a2) + square (b2));
  dC = C1 - C2;
  dH = sqrt (square (a1 - a2) + square (b1 - b2) - square (dC));
  dE = sqrt (square (dL) + square (dC / (1 + 0.045 * C1)) + square (dH / (1 + 0.015 * C1)));

  return dE;
}

/*
 * CIE 2000 delta E color comparison
 */
static gdouble
delta_e (gfloat* src1,
         gfloat* src2)
{
  gdouble L1, L2, a1, a2, b1, b2, La_, C1, C2, Ca, G, a1_, a2_, C1_,
    C2_, Ca_, h1_, h2_, Ha_, T, dh_, dL_, dC_, dH_, Sl, Sc, Sh, dPhi,
    Rc, Rt, dE, tmp;

  L1 = src1[0];
  L2 = src2[0];
  a1 = src1[1];
  a2 = src2[1];
  b1 = src1[2];
  b2 = src2[2];

  La_ = (L1 + L2) / 2.0;
  C1 = sqrt (square (a1) + square (b1));
  C2 = sqrt (square (a2) + square (b2));
  Ca = (C1 + C2) / 2.0;
  tmp = pow (Ca, 7);
  G = (1 - sqrt (tmp / (tmp + pow (25, 7)))) / 2.0;
  a1_ = a1 * (1 + G);
  a2_ = a2 * (1 + G);
  C1_ = sqrt (square (a1_) + square (b1));
  C2_ = sqrt (square (a2_) + square (b2));
  Ca_ = (C1_ + C2_) / 2.0;
  tmp = atan2 (b1, a1_) * 180 / G_PI;
  h1_ = (tmp >= 0.0) ? tmp : tmp + 360;
  tmp = atan2 (b2, a2_) * 180 / G_PI;
  h2_ = (tmp >= 0) ? tmp: tmp + 360;
  tmp = abs (h1_ - h2_);
  Ha_ = (tmp > 180) ? (h1_ + h2_ + 360) / 2.0 : (h1_ + h2_) / 2.0;
  T = 1 - 0.17 * cos ((Ha_ - 30) * G_PI / 180) +
    0.24 * cos ((Ha_ * 2) * G_PI / 180) +
    0.32 * cos ((Ha_ * 3 + 6) * G_PI / 180) -
    0.2 * cos ((Ha_ * 4 - 63) * G_PI / 180);
  if (tmp <= 180)
    dh_ = h2_ - h1_;
  else if (tmp > 180 && h2_ <= h1_)
    dh_ = h2_ - h1_ + 360;
  else
    dh_ = h2_ - h1_ - 360;
  dL_ = L2 - L1;
  dC_ = C2_ - C1_;
  dH_ = 2 * sqrt (C1_ * C2_) * sin (dh_ / 2.0 * G_PI / 180);
  tmp = square (La_ - 50);
  Sl = 1 + 0.015 * tmp / sqrt (20 + tmp);
  Sc = 1 + 0.045 * Ca_;
  Sh = 1 + 0.015 * Ca_ * T;
  dPhi = 30 * exp (-square ((Ha_ - 275) / 25.0));
  tmp = pow (Ca_, 7);
  Rc = 2 * sqrt (tmp / (tmp + pow (25, 7)));
  Rt = -Rc * sin (2 * dPhi * G_PI / 180);

  dE = sqrt (square (dL_ / Sl) + square (dC_ / Sc) +
             square (dH_ / Sh) + Rt * dC_ * dH_ / Sc / Sh);

  return dE;
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

     for (i=0; i < pixels; i++)
       {
         gdouble diff = delta_e (a, b);

         if (diff >= 0.1)
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
           gdouble diff = delta_e (a, b);

           if (diff >= 0.1)
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

     if (max_diff > 1.5)
       {
         GeglNode *graph, *sink;
         gchar    *debug_path;
         gint      ext_length;

         g_print ("\nBuffers differ\n"
                  "  wrong pixels   : %i/%i (%2.2f%%)\n"
                  "  max Δe         : %2.3f\n"
                  "  avg Δe (wrong) : %2.3f(wrong) %2.3f(total)\n",
                  wrong_pixels, pixels, (wrong_pixels*100.0/pixels),
                  max_diff,
                  diffsum/wrong_pixels,
                  diffsum/pixels);

         debug_path = g_malloc (strlen (composition_path)+16);
         ext_length = strlen (strrchr (composition_path, '.'));

         memcpy (debug_path, composition_path, strlen (composition_path)+1);
         memcpy (debug_path + strlen(composition_path)-ext_length, "-diff.png", 11);
         graph = gegl_graph (sink=gegl_node ("gegl:png-save",
                                             "path", debug_path, NULL,
                                             gegl_node ("gegl:buffer-source",
                                                        "buffer", debug_buf, NULL)));
         gegl_node_process (sink);
         g_object_unref (graph);
         g_object_unref (debug_buf);

         result = FALSE;
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
          gchar    *xml_root    = g_build_path (G_DIR_SEPARATOR_S, root, DATA_DIR, NULL);
          gchar    *image_path  = g_build_path (G_DIR_SEPARATOR_S, root, DATA_DIR, image, NULL);
          gchar    *output_path = g_build_path (G_DIR_SEPARATOR_S, root, OUTPUT_DIR, image, NULL);
          GeglNode *composition, *output;

          g_printf ("%s: ", gegl_operation_class_get_key (operation_class, "name"));

          composition = gegl_node_new_from_xml (xml, xml_root);
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
              gegl_node_connect (composition, "output", output, "input");
              gegl_node_process (output);

              if (image_compare (output_path, image_path))
                {
                  g_printf ("PASS\n");
                  result = result && TRUE;
                }
              else
                {
                  g_printf ("FAIL\n");
                  result = result && FALSE;
                }
            }

          g_object_unref (composition);
          g_free (root);
          g_free (xml_root);
          g_free (image_path);
          g_free (output_path);
        }

      result = result && process_operations(operations[i]);
    }

  g_free (operations);

  return result;
}

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

  gegl_init (&argc, &argv);
  gimp_operations_init ();
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gimp-operations", test_operations);

  result = g_test_run ();

  gegl_exit ();

  return result;
}

