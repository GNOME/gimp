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


typedef struct _Gimpressionist      Gimpressionist;
typedef struct _GimpressionistClass GimpressionistClass;

struct _Gimpressionist
{
  GimpPlugIn parent_instance;
};

struct _GimpressionistClass
{
  GimpPlugInClass parent_class;
};


#define GIMPRESSIONIST_TYPE  (gimpressionist_get_type ())
#define GIMPRESSIONIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMPRESSIONIST_TYPE, Gimpressionist))

GType                   gimpressionist_get_type         (void) G_GNUC_CONST;

static GList          * gimpressionist_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * gimpressionist_create_procedure (GimpPlugIn           *plug_in,
                                                         const gchar          *name);

static GimpValueArray * gimpressionist_run              (GimpProcedure        *procedure,
                                                         GimpRunMode           run_mode,
                                                         GimpImage            *image,
                                                         gint                  n_drawables,
                                                         GimpDrawable        **drawables,
                                                         GimpProcedureConfig  *config,
                                                         gpointer              run_data);

static void             gimpressionist_main             (void);


G_DEFINE_TYPE (Gimpressionist, gimpressionist, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GIMPRESSIONIST_TYPE)
DEFINE_STD_SET_I18N


static GimpDrawable *drawable;
static ppm_t         infile  = { 0, 0, NULL };
static ppm_t         inalpha = { 0, 0, NULL };


static void
gimpressionist_class_init (GimpressionistClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = gimpressionist_query_procedures;
  plug_in_class->create_procedure = gimpressionist_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
gimpressionist_init (Gimpressionist *gimpressionist)
{
}

static GList *
gimpressionist_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
gimpressionist_create_procedure (GimpPlugIn  *plug_in,
                                 const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            gimpressionist_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_GIMPressionist..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Artistic");

      gimp_procedure_set_documentation (procedure,
                                        _("Performs various artistic "
                                          "operations"),
                                        _("Performs various artistic operations "
                                          "on an image"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Vidar Madsen <vidar@prosalg.no>",
                                      "Vidar Madsen",
                                      PLUG_IN_VERSION);

      gimp_procedure_add_string_argument (procedure, "preset",
                                          _("Preset"),
                                          _("Preset Name"),
                                          NULL,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_bytes_aux_argument (procedure, "settings-data",
                                             "Settings data",
                                             "TODO: eventually we must implement proper args for every settings",
                                             GIMP_PARAM_READWRITE);
    }

  return procedure;
}

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

static GimpValueArray *
gimpressionist_run (GimpProcedure        *procedure,
                    GimpRunMode           run_mode,
                    GimpImage            *image,
                    gint                  n_drawables,
                    GimpDrawable        **drawables,
                    GimpProcedureConfig  *config,
                    gpointer              run_data)
{
  GBytes *settings_bytes = NULL;
  gchar  *preset_name    = NULL;

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   gimp_procedure_get_name (procedure));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  img_has_alpha = gimp_drawable_has_alpha (drawable);

  random_generator = g_rand_new ();

  {
    gint x1, y1, width, height;

    if (! gimp_drawable_mask_intersect (drawable,
                                        &x1, &y1, &width, &height))
      {
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_SUCCESS,
                                                 NULL);
      }
  }

  restore_default_values ();

  /* Temporary code replacing legacy gimp_[gs]et_data() using an AUX argument.
   * This doesn't actually fix the "Reset to initial values|factory defaults"
   * features, but at least makes per-run value storage work.
   * TODO: eventually we want proper separate arguments as a complete fix.
   */
  g_object_get (config, "settings-data", &settings_bytes, NULL);
  if (settings_bytes != NULL && g_bytes_get_size (settings_bytes) == sizeof (gimpressionist_vals_t))
    pcvals = *((gimpressionist_vals_t *) g_bytes_get_data (settings_bytes, NULL));
  g_bytes_unref (settings_bytes);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      if (! create_gimpressionist (procedure, config))
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      g_object_get (config,
                    "preset", &preset_name,
                    NULL);

      if (select_preset (preset_name))
        {
          g_free (preset_name);
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_EXECUTION_ERROR,
                                                   NULL);
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      break;
    }

  /* It seems that the value of the run variable is stored in the
   * preset. I don't know if it's a bug or a feature, but I just work
   * here and am anxious to get a working version.  So I'm setting it
   * to the correct value here.
   *
   * It also seems that defaultpcvals have this erroneous value as
   * well, so it gets set to FALSE as well. Thus it is always set to
   * TRUE upon a non-interactive run.  -- Shlomi Fish
   */
  pcvals.run = TRUE;

  if (gimp_drawable_is_rgb  (drawable) ||
      gimp_drawable_is_gray (drawable))
    {
      gimpressionist_main ();

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }
  else
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  settings_bytes = g_bytes_new (&pcvals, sizeof (gimpressionist_vals_t));
  g_object_set (config, "settings-data", settings_bytes, NULL);
  g_bytes_unref (settings_bytes);

  /* Resources Cleanup */
  g_free (preset_name);
  g_rand_free (random_generator);
  free_parsepath_cache ();
  brush_reload (NULL, NULL);
  preview_free_resources ();
  brush_free ();
  preset_free ();
  orientation_map_free_resources ();
  size_map_free_resources ();

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
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
