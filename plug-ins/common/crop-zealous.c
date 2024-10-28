/*
 * ZealousCrop plug-in version 1.00
 * by Adam D. Moss <adam@foxbox.org>
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

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC "plug-in-zealouscrop"

#define EPSILON (1e-5)
#define FLOAT_IS_ZERO(value) (value > -EPSILON && value < EPSILON)
#define FLOAT_EQUAL(v1, v2)  ((v1 - v2) > -EPSILON && (v1 - v2) < EPSILON)


typedef struct _Crop      Crop;
typedef struct _CropClass CropClass;

struct _Crop
{
  GimpPlugIn      parent_instance;
};

struct _CropClass
{
  GimpPlugInClass parent_class;
};

#define CROP_TYPE  (crop_get_type ())
#define CROP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CROP_TYPE, Crop))

GType                   crop_get_type         (void) G_GNUC_CONST;

static GList          * crop_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * crop_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * crop_run              (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               GimpDrawable        **drawables,
                                               GimpProcedureConfig  *config,
                                               gpointer              run_data);

static inline gboolean  colors_equal          (const gfloat         *col1,
                                               const gfloat         *col2,
                                               gint                  components,
                                               gboolean              has_alpha);
static void             do_zcrop              (GimpDrawable         *drawable,
                                               GimpImage            *image);


G_DEFINE_TYPE (Crop, crop, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (CROP_TYPE)
DEFINE_STD_SET_I18N


static void
crop_class_init (CropClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = crop_query_procedures;
  plug_in_class->create_procedure = crop_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
crop_init (Crop *crop)
{
}

static GList *
crop_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
crop_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            crop_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Zealous Crop"));
      gimp_procedure_add_menu_path (procedure, "<Image>/Image/[Crop]");

      gimp_procedure_set_documentation (procedure,
                                        _("Autocrop unused space from "
                                          "edges and middle"),
                                        NULL,
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Adam D. Moss",
                                      "Adam D. Moss",
                                      "1997");
    }

  return procedure;
}

static GimpValueArray *
crop_run (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          GimpDrawable        **drawables,
          GimpProcedureConfig  *config,
          gpointer              run_data)
{
  GimpDrawable *drawable;

  gegl_init (NULL, NULL);

  gimp_progress_init (_("Zealous cropping"));

  if (gimp_core_object_array_get_length ((GObject **) drawables) != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  do_zcrop (drawable, image);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static inline gboolean
colors_equal (const gfloat   *col1,
              const gfloat   *col2,
              gint            components,
              gboolean        has_alpha)
{
  if (has_alpha &&
      FLOAT_IS_ZERO (col1[components - 1]) &&
      FLOAT_IS_ZERO (col2[components - 1]))
    {
      return TRUE;
    }
  else
    {
      gint b;

      for (b = 0; b < components; b++)
        {
          if (! FLOAT_EQUAL (col1[b], col2[b]))
            return FALSE;
        }

      return TRUE;
    }
}

static void
do_zcrop (GimpDrawable *drawable,
          GimpImage    *image)
{
  GeglBuffer   *drawable_buffer;
  GeglBuffer   *shadow_buffer;
  gfloat       *linear_buf;
  const Babl   *format;

  gint          x, y, width, height;
  gint          components;
  gint8        *killrows;
  gint8        *killcols;
  gint32        livingrows, livingcols, destrow, destcol;
  GimpChannel  *selection_copy;
  gboolean      has_alpha;

  drawable_buffer = gimp_drawable_get_buffer (drawable);
  shadow_buffer   = gimp_drawable_get_shadow_buffer (drawable);

  width  = gegl_buffer_get_width (drawable_buffer);
  height = gegl_buffer_get_height (drawable_buffer);
  has_alpha = gimp_drawable_has_alpha (drawable);

  if (has_alpha)
    format = babl_format ("R'G'B'A float");
  else
    format = babl_format ("R'G'B' float");

  components = babl_format_get_n_components (format);

  killrows = g_new (gint8, height);
  killcols = g_new (gint8, width);

  linear_buf = g_new (gfloat, (width > height ? width : height) * components);

  /* search which rows to remove */

  livingrows = 0;
  for (y = 0; y < height; y++)
    {
      gegl_buffer_get (drawable_buffer, GEGL_RECTANGLE (0, y, width, 1),
                       1.0, format, linear_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      killrows[y] = TRUE;

      for (x = components; x < width * components; x += components)
        {
          if (! colors_equal (linear_buf, &linear_buf[x], components, has_alpha))
            {
              livingrows++;
              killrows[y] = FALSE;
              break;
            }
        }
    }

  gimp_progress_update (0.25);

  /* search which columns to remove */

  livingcols = 0;
  for (x = 0; x < width; x++)
    {
      gegl_buffer_get (drawable_buffer, GEGL_RECTANGLE (x, 0, 1, height),
                       1.0, format, linear_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      killcols[x] = TRUE;

      for (y = components; y < height * components; y += components)
        {
          if (! colors_equal (linear_buf, &linear_buf[y], components, has_alpha))
            {
              livingcols++;
              killcols[x] = FALSE;
              break;
            }
        }
    }

  gimp_progress_update (0.5);

  if ((livingcols == 0 || livingrows == 0) ||
      (livingcols == width && livingrows == height))
    {
      g_message (_("Nothing to crop."));

      g_object_unref (shadow_buffer);
      g_object_unref (drawable_buffer);

      g_free (linear_buf);
      g_free (killrows);
      g_free (killcols);
      return;
    }

  /* restitute living rows */

  destrow = 0;
  for (y = 0; y < height; y++)
    {
      if (!killrows[y])
        {
          gegl_buffer_copy (drawable_buffer,
                            GEGL_RECTANGLE (0, y, width, 1),
                            GEGL_ABYSS_NONE,
                            shadow_buffer,
                            GEGL_RECTANGLE (0, destrow, width, 1));

          destrow++;
        }
    }

  gimp_progress_update (0.75);

  /* restitute living columns */

  destcol = 0;
  for (x = 0; x < width; x++)
    {
      if (!killcols[x])
        {
          gegl_buffer_copy (shadow_buffer,
                            GEGL_RECTANGLE (x, 0, 1, height),
                            GEGL_ABYSS_NONE,
                            shadow_buffer,
                            GEGL_RECTANGLE (destcol, 0, 1, height));

          destcol++;
        }
    }

  gimp_progress_update (1.00);

  gimp_image_undo_group_start (image);

  selection_copy = GIMP_CHANNEL (gimp_selection_save (image));
  gimp_selection_none (image);

  gegl_buffer_flush (shadow_buffer);
  gimp_drawable_merge_shadow (drawable, TRUE);
  gegl_buffer_flush (drawable_buffer);

  gimp_image_select_item (image, GIMP_CHANNEL_OP_REPLACE,
                          GIMP_ITEM (selection_copy));
  gimp_image_remove_channel (image, selection_copy);

  gimp_image_crop (image, livingcols, livingrows, 0, 0);

  gimp_image_undo_group_end (image);

  g_object_unref (shadow_buffer);
  g_object_unref (drawable_buffer);

  g_free (linear_buf);
  g_free (killrows);
  g_free (killcols);
}
