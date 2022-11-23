/******************************************************/
/* Apply mapping and shading on the whole input image */
/******************************************************/

#include "config.h"

#include <sys/types.h>

#include <libligma/ligma.h>

#include "lighting-main.h"
#include "lighting-image.h"
#include "lighting-shade.h"
#include "lighting-apply.h"

#include "libligma/stdplugins-intl.h"


/*************/
/* Main loop */
/*************/

void
compute_image (void)
{
  gint         xcount, ycount;
  LigmaRGB      color;
  glong        progress_counter = 0;
  LigmaVector3  p;
  LigmaImage   *new_image = NULL;
  LigmaLayer   *new_layer = NULL;
  gint32       index;
  guchar      *row = NULL;
  guchar       obpp;
  gboolean     has_alpha;
  get_ray_func ray_func;

  if (mapvals.create_new_image == TRUE ||
      (mapvals.transparent_background == TRUE &&
       ! ligma_drawable_has_alpha (input_drawable)))
    {
      /* Create a new image */
      /* ================== */

      new_image = ligma_image_new (width, height, LIGMA_RGB);

      if (mapvals.transparent_background == TRUE)
        {
          /* Add a layer with an alpha channel */
          /* ================================= */

          new_layer = ligma_layer_new (new_image, "Background",
                                      width, height,
                                      LIGMA_RGBA_IMAGE,
                                      100.0,
                                      ligma_image_get_default_new_layer_mode (new_image));
        }
      else
        {
          /* Create a "normal" layer */
          /* ======================= */

          new_layer = ligma_layer_new (new_image, "Background",
                                      width, height,
                                      LIGMA_RGB_IMAGE,
                                      100.0,
                                      ligma_image_get_default_new_layer_mode (new_image));
        }

      ligma_image_insert_layer (new_image, new_layer, NULL, 0);
      output_drawable = LIGMA_DRAWABLE (new_layer);
    }

  if (mapvals.bump_mapped == TRUE && mapvals.bumpmap_id != -1)
    {
      bumpmap_setup (ligma_drawable_get_by_id (mapvals.bumpmap_id));
    }

  precompute_init (width, height);

  if (! mapvals.env_mapped || mapvals.envmap_id == -1)
    {
      ray_func = get_ray_color;
    }
  else
    {
      envmap_setup (ligma_drawable_get_by_id (mapvals.envmap_id));

      ray_func = get_ray_color_ref;
    }

  dest_buffer = ligma_drawable_get_shadow_buffer (output_drawable);

  has_alpha = ligma_drawable_has_alpha (output_drawable);

  /* FIXME */
  obpp = has_alpha ? 4 : 3; //ligma_drawable_get_bpp (output_drawable);

  row = g_new (guchar, obpp * width);

  ligma_progress_init (_("Lighting Effects"));

  /* Init the first row */
  if (mapvals.bump_mapped == TRUE && mapvals.bumpmap_id != -1 && height >= 2)
    interpol_row (0, width, 0);

  for (ycount = 0; ycount < height; ycount++)
    {
      if (mapvals.bump_mapped == TRUE && mapvals.bumpmap_id != -1)
	precompute_normals (0, width, ycount);

      index = 0;

      for (xcount = 0; xcount < width; xcount++)
	{
	  p = int_to_pos (xcount, ycount);
	  color = (* ray_func) (&p);

	  row[index++] = (guchar) (color.r * 255.0);
	  row[index++] = (guchar) (color.g * 255.0);
	  row[index++] = (guchar) (color.b * 255.0);

	  if (has_alpha)
	    row[index++] = (guchar) (color.a * 255.0);

	  progress_counter++;
	}

      ligma_progress_update ((gdouble) progress_counter /
                            (gdouble) maxcounter);

      gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (0, ycount, width, 1), 0,
                       has_alpha ?
                       babl_format ("R'G'B'A u8") : babl_format ("R'G'B' u8"),
                       row,
                       GEGL_AUTO_ROWSTRIDE);
    }

  ligma_progress_update (1.0);

  g_free (row);

  g_object_unref (dest_buffer);

  ligma_drawable_merge_shadow (output_drawable, TRUE);
  ligma_drawable_update (output_drawable, 0, 0, width, height);

  if (new_image)
    {
      ligma_display_new (new_image);
      ligma_displays_flush ();
    }
}
