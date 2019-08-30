/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "ppmtool.h"
#include "infile.h"
#include "gimpressionist.h"
#include "preview.h"
#include "brush.h"
#include "presets.h"
#include "random.h"
#include "orientmap.h"
#include "size.h"


#include "libgimp/stdplugins-intl.h"

static void query               (void);
static void run                 (const gchar      *name,
                                 gint              nparams,
                                 const GimpParam  *param,
                                 gint             *nreturn_vals,
                                 GimpParam       **return_vals);
static void gimpressionist_main (void);


const GimpPlugInInfo PLUG_IN_INFO = {
        NULL,   /* init_proc */
        NULL,   /* quit_proc */
        query,  /* query_proc */
        run     /* run_proc */
}; /* PLUG_IN_INFO */

static GimpDrawable *drawable;
static ppm_t         infile  = { 0, 0, NULL };
static ppm_t         inalpha = { 0, 0, NULL };


void
infile_copy_to_ppm (ppm_t * p)
{
  if (!PPM_IS_INITED (&infile))
    grabarea ();

  ppm_copy (&infile, p);
}

void
infile_copy_alpha_to_ppm (ppm_t * p)
{
  ppm_copy (&inalpha, p);
}

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",  "The run mode { RUN-INTERACTIVE (0) }"    },
    { GIMP_PDB_IMAGE,    "image",     "Input image"    },
    { GIMP_PDB_DRAWABLE, "drawable",  "Input drawable" },
    { GIMP_PDB_STRING,   "preset",    "Preset Name"    },
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Performs various artistic operations"),
                          "Performs various artistic operations on an image",
                          "Vidar Madsen <vidar@prosalg.no>",
                          "Vidar Madsen",
                          PLUG_IN_VERSION,
                          N_("_GIMPressionist..."),
                          "RGB*, GRAY*",
                          GIMP_PDB_PROC_TYPE_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Artistic");
}

static void
gimpressionist_get_data (void)
{
  restore_default_values ();
  gimp_get_data (PLUG_IN_PROC, &pcvals);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status;
  gboolean           with_specified_preset;
  gchar             *preset_name = NULL;

  status   = GIMP_PDB_SUCCESS;
  run_mode = param[0].data.d_int32;
  with_specified_preset = FALSE;

  if (nparams > 3)
    {
      preset_name = param[3].data.d_string;
      if (strcmp (preset_name, ""))
        {
          with_specified_preset = TRUE;
        }
    }

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  /* Get the active drawable info */

  drawable = GIMP_DRAWABLE (gimp_item_get_by_id (param[2].data.d_drawable));
  img_has_alpha = gimp_drawable_has_alpha (drawable);

  random_generator = g_rand_new ();

  /*
   * Check precondition before we open a dialog: Is there a selection
   * that intersects, OR is there no selection (use entire drawable.)
   */
  {
    gint x1, y1, width, height; /* Not used. */

    if (! gimp_drawable_mask_intersect (drawable,
                                        &x1, &y1, &width, &height))
      {
        values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
        *nreturn_vals           = 2;
        values[1].type          = GIMP_PDB_STRING;
        values[1].data.d_string = _("The selection does not intersect "
                                    "the active layer or mask.");

        return;
      }
  }


  switch (run_mode)
    {
        /*
         * Note: there's a limitation here. Running this plug-in before the
         * interactive plug-in was run will cause it to crash, because the
         * data is uninitialized.
         * */
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_NONINTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimpressionist_get_data ();
      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          if (!create_gimpressionist ())
              return;
        }
      break;
    default:
      status = GIMP_PDB_EXECUTION_ERROR;
      break;
    }
  if ((status == GIMP_PDB_SUCCESS) &&
      (gimp_drawable_is_rgb  (drawable) ||
       gimp_drawable_is_gray (drawable)))
    {

      if (with_specified_preset)
        {
          /* If select_preset fails - set to an error */
          if (select_preset (preset_name))
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }
      /* It seems that the value of the run variable is stored in
       * the preset. I don't know if it's a bug or a feature, but
       * I just work here and am anxious to get a working version.
       * So I'm setting it to the correct value here.
       *
       * It also seems that defaultpcvals have this erroneous
       * value as well, so it gets set to FALSE as well. Thus it
       * is always set to TRUE upon a non-interactive run.
       *        -- Shlomi Fish
       * */
      if (run_mode == GIMP_RUN_NONINTERACTIVE)
        {
          pcvals.run = TRUE;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          gimpressionist_main ();
          gimp_displays_flush ();

          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC,
                           &pcvals,
                           sizeof (gimpressionist_vals_t));
        }
    }
  else if (status == GIMP_PDB_SUCCESS)
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  /* Resources Cleanup */
  g_rand_free (random_generator);
  free_parsepath_cache ();
  brush_reload (NULL, NULL);
  preview_free_resources ();
  brush_free ();
  preset_free ();
  orientation_map_free_resources ();
  size_map_free_resources ();

  values[0].data.d_status = status;
}

static const Babl *
get_u8_format (GimpDrawable *drawable)
{
  if (gimp_drawable_is_rgb (drawable))
    {
      if (gimp_drawable_has_alpha (drawable))
        return babl_format ("R'G'B'A u8");
      else
        return babl_format ("R'G'B' u8");
    }
  else
    {
      if (gimp_drawable_has_alpha (drawable))
        return babl_format ("Y'A u8");
      else
        return babl_format ("Y' u8");
    }
}

void
grabarea (void)
{
  GeglBuffer         *src_buffer;
  GeglBufferIterator *iter;
  const Babl         *format;
  gint                bpp;
  ppm_t              *p;
  gint                x1, y1;
  gint                x, y;
  gint                width, height;
  gint                row, col;
  gint                rowstride;

  if (! gimp_drawable_mask_intersect (drawable,
                                      &x1, &y1, &width, &height))
    return;

  ppm_new (&infile, width, height);
  p = &infile;

  format = get_u8_format (drawable);
  bpp    = babl_format_get_bytes_per_pixel (format);

  if (gimp_drawable_has_alpha (drawable))
    ppm_new (&inalpha, width, height);

  rowstride = p->width * 3;

  src_buffer = gimp_drawable_get_buffer (drawable);

  iter = gegl_buffer_iterator_new (src_buffer,
                                   GEGL_RECTANGLE (x1, y1, width, height), 0,
                                   format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      GeglRectangle  roi = iter->items[0].roi;
      const guchar  *src = iter->items[0].data;

      switch (bpp)
        {
        case 1:
          for (y = 0, row = roi.y - y1; y < roi.height; y++, row++)
            {
              const guchar *s      = src;
              guchar       *tmprow = p->col + row * rowstride;

              for (x = 0, col = roi.x - x1; x < roi.width; x++, col++)
                {
                  gint k = col * 3;

                  tmprow[k + 0] = s[0];
                  tmprow[k + 1] = s[0];
                  tmprow[k + 2] = s[0];

                  s++;
                }

              src += bpp * roi.width;
            }
          break;

        case 2:
          for (y = 0, row = roi.y - y1; y < roi.height; y++, row++)
            {
              const guchar *s       = src;
              guchar       *tmprow  = p->col + row * rowstride;
              guchar       *tmparow = inalpha.col + row * rowstride;

              for (x = 0, col = roi.x - x1; x < roi.width; x++, col++)
                {
                  gint k = col * 3;

                  tmprow[k + 0] = s[0];
                  tmprow[k + 1] = s[0];
                  tmprow[k + 2] = s[0];
                  tmparow[k]    = 255 - s[1];

                  s += 2;
                }

              src += bpp * roi.width;
            }
          break;

        case 3:
          col = roi.x - x1;

          for (y = 0, row = roi.y - y1; y < roi.height; y++, row++)
            {
              memcpy (p->col + row * rowstride + col * 3, src, roi.width * 3);

              src += bpp * roi.width;
            }
          break;

        case 4:
          for (y = 0, row = roi.y - y1; y < roi.height; y++, row++)
            {
              const guchar *s       = src;
              guchar       *tmprow  = p->col + row * rowstride;
              guchar       *tmparow = inalpha.col + row * rowstride;

              for (x = 0, col = roi.x - x1; x < roi.width; x++, col++)
                {
                  gint k = col * 3;

                  tmprow[k + 0] = s[0];
                  tmprow[k + 1] = s[1];
                  tmprow[k + 2] = s[2];
                  tmparow[k]    = 255 - s[3];

                  s += 4;
                }

              src += bpp * roi.width;
            }
          break;
        }
    }

  g_object_unref (src_buffer);
}

static void
gimpressionist_main (void)
{
  GeglBuffer         *dest_buffer;
  GeglBufferIterator *iter;
  const Babl         *format;
  gint                bpp;
  ppm_t              *p;
  gint                x1, y1;
  gint                x, y;
  gint                width, height;
  gint                row, col;
  gint                rowstride;
  glong               done = 0;
  glong               total;

  if (! gimp_drawable_mask_intersect (drawable,
                                      &x1, &y1, &width, &height))
    return;

  total = width * height;

  format = get_u8_format (drawable);
  bpp    = babl_format_get_bytes_per_pixel (format);

  gimp_progress_init (_("Painting"));

  if (! PPM_IS_INITED (&infile))
    grabarea ();

  repaint (&infile, (img_has_alpha) ? &inalpha : NULL);

  p = &infile;

  rowstride = p->width * 3;

  dest_buffer = gimp_drawable_get_shadow_buffer (drawable);

  iter = gegl_buffer_iterator_new (dest_buffer,
                                   GEGL_RECTANGLE (x1, y1, width, height), 0,
                                   format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      GeglRectangle  roi  = iter->items[0].roi;
      guchar        *dest = iter->items[0].data;

      switch (bpp)
        {
        case 1:
          for (y = 0, row = roi.y - y1; y < roi.height; y++, row++)
            {
              guchar       *d       = dest;
              const guchar *tmprow  = p->col + row * rowstride;

              for (x = 0, col = roi.x - x1; x < roi.width; x++, col++)
                {
                  gint k = col * 3;

                  *d++ = GIMP_RGB_LUMINANCE (tmprow[k + 0],
                                             tmprow[k + 1],
                                             tmprow[k + 2]);
                }

              dest += bpp * roi.width;
            }
          break;

        case 2:
          for (y = 0, row = roi.y - y1; y < roi.height; y++, row++)
            {
              guchar       *d       = dest;
              const guchar *tmprow  = p->col + row * rowstride;
              const guchar *tmparow = inalpha.col + row * rowstride;

              for (x = 0, col = roi.x - x1; x < roi.width; x++, col++)
                {
                  gint k     = col * 3;
                  gint value = GIMP_RGB_LUMINANCE (tmprow[k + 0],
                                                   tmprow[k + 1],
                                                   tmprow[k + 2]);

                  d[0] = value;
                  d[1] = 255 - tmparow[k];

                  d += 2;
                }

              dest += bpp * roi.width;
            }
          break;

        case 3:
          col = roi.x - x1;

          for (y = 0, row = roi.y - y1; y < roi.height; y++, row++)
            {
              memcpy (dest, p->col + row * rowstride + col * 3, roi.width * 3);

              dest += bpp * roi.width;
            }
          break;

        case 4:
          for (y = 0, row = roi.y - y1; y < roi.height; y++, row++)
            {
              guchar       *d       = dest;
              const guchar *tmprow  = p->col + row * rowstride;
              const guchar *tmparow = inalpha.col + row * rowstride;

              for (x = 0, col = roi.x - x1; x < roi.width; x++, col++)
                {
                  gint k = col * 3;

                  d[0] = tmprow[k + 0];
                  d[1] = tmprow[k + 1];
                  d[2] = tmprow[k + 2];
                  d[3] = 255 - tmparow[k];

                  d += 4;
                }

              dest += bpp * roi.width;
            }
          break;
        }

      done += roi.width * roi.height;

      gimp_progress_update (0.8 + 0.2 * done / total);
    }

  g_object_unref (dest_buffer);

  gimp_progress_update (1.0);

  gimp_drawable_merge_shadow (drawable, TRUE);
  gimp_drawable_update (drawable, x1, y1, width, height);
}
