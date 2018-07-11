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

/* Declare local functions. */

static void            query        (void);
static void            run          (const gchar      *name,
                                     gint              nparams,
                                     const GimpParam  *param,
                                     gint             *nreturn_vals,
                                     GimpParam       **return_vals);

static inline gboolean colors_equal (const gfloat     *col1,
                                     const gfloat     *col2,
                                     gint              components,
                                     gboolean          has_alpha);
static void            do_zcrop     (gint32    drawable_id,
                                     gint32    image_id);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image"    },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Autocrop unused space from edges and middle"),
                          "",
                          "Adam D. Moss",
                          "Adam D. Moss",
                          "1997",
                          N_("_Zealous Crop"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Image/Crop");
}

static void
run (const gchar      *name,
     gint              n_params,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint32             drawable_id;
  gint32             image_id;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals = 1;
  *return_vals  = values;

  run_mode = param[0].data.d_int32;

  if (run_mode == GIMP_RUN_NONINTERACTIVE)
    {
      if (n_params != 3)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Get the specified drawable  */
      image_id    = param[1].data.d_int32;
      drawable_id = param[2].data.d_int32;

      /*  Make sure that the drawable is gray or RGB or indexed  */
      if (gimp_drawable_is_rgb (drawable_id) ||
          gimp_drawable_is_gray (drawable_id) ||
          gimp_drawable_is_indexed (drawable_id))
        {
          gimp_progress_init (_("Zealous cropping"));

          do_zcrop (drawable_id, image_id);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  gegl_exit ();
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
do_zcrop (gint32  drawable_id,
          gint32  image_id)
{
  GeglBuffer  *drawable_buffer;
  GeglBuffer  *shadow_buffer;
  gfloat      *linear_buf;
  const Babl  *format;

  gint         x, y, width, height;
  gint         components;
  gint8       *killrows;
  gint8       *killcols;
  gint32       livingrows, livingcols, destrow, destcol;
  gint32       selection_copy_id;
  gboolean     has_alpha;

  drawable_buffer = gimp_drawable_get_buffer (drawable_id);
  shadow_buffer   = gimp_drawable_get_shadow_buffer (drawable_id);

  width  = gegl_buffer_get_width (drawable_buffer);
  height = gegl_buffer_get_height (drawable_buffer);
  has_alpha = gimp_drawable_has_alpha (drawable_id);

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

  gimp_image_undo_group_start (image_id);

  selection_copy_id = gimp_selection_save (image_id);
  gimp_selection_none (image_id);

  gegl_buffer_flush (shadow_buffer);
  gimp_drawable_merge_shadow (drawable_id, TRUE);
  gegl_buffer_flush (drawable_buffer);

  gimp_image_select_item (image_id, GIMP_CHANNEL_OP_REPLACE, selection_copy_id);
  gimp_image_remove_channel (image_id, selection_copy_id);

  gimp_image_crop (image_id, livingcols, livingrows, 0, 0);

  gimp_image_undo_group_end (image_id);

  g_object_unref (shadow_buffer);
  g_object_unref (drawable_buffer);

  g_free (linear_buf);
  g_free (killrows);
  g_free (killcols);
}
