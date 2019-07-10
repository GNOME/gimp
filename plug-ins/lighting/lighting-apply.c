/******************************************************/
/* Apply mapping and shading on the whole input image */
/******************************************************/

#include "config.h"

#include <sys/types.h>

#include <libgimp/gimp.h>

#include "lighting-main.h"
#include "lighting-image.h"
#include "lighting-shade.h"
#include "lighting-apply.h"

#include "libgimp/stdplugins-intl.h"


/*************/
/* Main loop */
/*************/

void
compute_image (void)
{
  gint         xcount, ycount;
  GimpRGB      color;
  glong        progress_counter = 0;
  GimpVector3  p;
  gint32       new_image_id = -1;
  gint32       new_layer_id = -1;
  gint32       index;
  guchar      *row = NULL;
  guchar       obpp;
  gboolean     has_alpha;
  get_ray_func ray_func;

  if (mapvals.create_new_image == TRUE ||
      (mapvals.transparent_background == TRUE &&
       ! gimp_drawable_has_alpha (input_drawable_id)))
    {
      /* Create a new image */
      /* ================== */

      new_image_id = gimp_image_new (width, height, GIMP_RGB);

      if (mapvals.transparent_background == TRUE)
        {
          /* Add a layer with an alpha channel */
          /* ================================= */

          new_layer_id = gimp_layer_new (new_image_id, "Background",
					 width, height,
					 GIMP_RGBA_IMAGE,
					 100.0,
					 gimp_image_get_default_new_layer_mode (new_image_id));
        }
      else
        {
          /* Create a "normal" layer */
          /* ======================= */

          new_layer_id = gimp_layer_new (new_image_id, "Background",
					 width, height,
					 GIMP_RGB_IMAGE,
					 100.0,
					 gimp_image_get_default_new_layer_mode (new_image_id));
        }

      gimp_image_insert_layer (new_image_id, new_layer_id, -1, 0);
      output_drawable_id = new_layer_id;
    }

  if (mapvals.bump_mapped == TRUE && mapvals.bumpmap_id != -1)
    {
      bumpmap_setup (mapvals.bumpmap_id);
    }

  precompute_init (width, height);

  if (! mapvals.env_mapped || mapvals.envmap_id == -1)
    {
      ray_func = get_ray_color;
    }
  else
    {
      envmap_setup (mapvals.envmap_id);

      ray_func = get_ray_color_ref;
    }

  dest_buffer = gimp_drawable_get_shadow_buffer (output_drawable_id);

  has_alpha = gimp_drawable_has_alpha (output_drawable_id);

  /* FIXME */
  obpp = has_alpha ? 4 : 3; //gimp_drawable_bpp (output_drawable_id);

  row = g_new (guchar, obpp * width);

  gimp_progress_init (_("Lighting Effects"));

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

      gimp_progress_update ((gdouble) progress_counter /
                            (gdouble) maxcounter);

      gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (0, ycount, width, 1), 0,
                       has_alpha ?
                       babl_format ("R'G'B'A u8") : babl_format ("R'G'B' u8"),
                       row,
                       GEGL_AUTO_ROWSTRIDE);
    }

  gimp_progress_update (1.0);

  g_free (row);

  g_object_unref (dest_buffer);

  gimp_drawable_merge_shadow (output_drawable_id, TRUE);
  gimp_drawable_update (output_drawable_id, 0, 0, width, height);

  if (new_image_id!=-1)
    {
      gimp_display_new (new_image_id);
      gimp_displays_flush ();
    }

}
