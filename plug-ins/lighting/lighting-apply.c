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
  GimpImage   *new_image = NULL;
  GimpLayer   *new_layer = NULL;
  gint32       index;
  guchar      *row = NULL;
  guchar       obpp;
  gboolean     has_alpha;
  get_ray_func ray_func;

  if (mapvals.create_new_image == TRUE ||
      (mapvals.transparent_background == TRUE &&
       ! gimp_drawable_has_alpha (input_drawable)))
    {
      /* Create a new image */
      /* ================== */

      new_image = gimp_image_new (width, height, GIMP_RGB);

      if (mapvals.transparent_background == TRUE)
        {
          /* Add a layer with an alpha channel */
          /* ================================= */

          new_layer = gimp_layer_new (new_image, "Background",
                                      width, height,
                                      GIMP_RGBA_IMAGE,
                                      100.0,
                                      gimp_image_get_default_new_layer_mode (new_image));
        }
      else
        {
          /* Create a "normal" layer */
          /* ======================= */

          new_layer = gimp_layer_new (new_image, "Background",
                                      width, height,
                                      GIMP_RGB_IMAGE,
                                      100.0,
                                      gimp_image_get_default_new_layer_mode (new_image));
        }

      gimp_image_insert_layer (new_image, new_layer, NULL, 0);
      output_drawable = GIMP_DRAWABLE (new_layer);
    }

  if (mapvals.bump_mapped == TRUE && mapvals.bumpmap_id != -1)
    {
      bumpmap_setup (gimp_drawable_get_by_id (mapvals.bumpmap_id));
    }

  precompute_init (width, height);

  if (! mapvals.env_mapped || mapvals.envmap_id == -1)
    {
      ray_func = get_ray_color;
    }
  else
    {
      envmap_setup (gimp_drawable_get_by_id (mapvals.envmap_id));

      ray_func = get_ray_color_ref;
    }

  dest_buffer = gimp_drawable_get_shadow_buffer (output_drawable);

  has_alpha = gimp_drawable_has_alpha (output_drawable);

  /* FIXME */
  obpp = has_alpha ? 4 : 3; //gimp_drawable_get_bpp (output_drawable);

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

  gimp_drawable_merge_shadow (output_drawable, TRUE);
  gimp_drawable_update (output_drawable, 0, 0, width, height);

  if (new_image)
    {
      gimp_display_new (new_image);
      gimp_displays_flush ();
    }
}

void
copy_from_config (GimpProcedureConfig *config)
{
  GimpDrawable *temp_bump_map_drawable = NULL;
  GimpDrawable *temp_env_map_drawable  = NULL;
  GeglColor    *color_1;
  GeglColor    *color_2;
  GeglColor    *color_3;
  GeglColor    *color_4;
  GeglColor    *color_5;
  GeglColor    *color_6;

  g_object_get (config,
                "distance", &mapvals.viewpoint.z,
                "do-bumpmap", &mapvals.bump_mapped,
                "do-envmap", &mapvals.env_mapped,
                "antialiasing", &mapvals.antialiasing,
                "new-image", &mapvals.create_new_image,
                "transparent-background", &mapvals.transparent_background,

                "light-color-1", &color_1,
                "light-intensity-1", &mapvals.lightsource[0].intensity,
                "light-position-x-1", &mapvals.lightsource[0].position.x,
                "light-position-y-1", &mapvals.lightsource[0].position.y,
                "light-position-z-1", &mapvals.lightsource[0].position.z,
                "light-direction-x-1", &mapvals.lightsource[0].direction.x,
                "light-direction-y-1", &mapvals.lightsource[0].direction.y,
                "light-direction-z-1", &mapvals.lightsource[0].direction.z,

                "isolate", &mapvals.light_isolated,
                "ambient-intensity", &mapvals.material.ambient_int,
                "diffuse-intensity", &mapvals.material.diffuse_int,
                "diffuse-reflectivity", &mapvals.material.diffuse_ref,
                "specular-reflectivity", &mapvals.material.specular_ref,
                "highlight", &mapvals.material.highlight,
                "metallic", &mapvals.material.metallic,

                "light-color-2", &color_2,
                "light-intensity-2", &mapvals.lightsource[1].intensity,
                "light-position-x-2", &mapvals.lightsource[1].position.x,
                "light-position-y-2", &mapvals.lightsource[1].position.y,
                "light-position-z-2", &mapvals.lightsource[1].position.z,
                "light-direction-x-2", &mapvals.lightsource[1].direction.x,
                "light-direction-y-2", &mapvals.lightsource[1].direction.y,
                "light-direction-z-2", &mapvals.lightsource[1].direction.z,
                "light-color-3", &color_3,
                "light-intensity-3", &mapvals.lightsource[2].intensity,
                "light-position-x-3", &mapvals.lightsource[2].position.x,
                "light-position-y-3", &mapvals.lightsource[2].position.y,
                "light-position-z-3", &mapvals.lightsource[2].position.z,
                "light-direction-x-3", &mapvals.lightsource[2].direction.x,
                "light-direction-y-3", &mapvals.lightsource[2].direction.y,
                "light-direction-z-3", &mapvals.lightsource[2].direction.z,
                "light-color-4", &color_4,
                "light-intensity-4", &mapvals.lightsource[3].intensity,
                "light-position-x-4", &mapvals.lightsource[3].position.x,
                "light-position-y-4", &mapvals.lightsource[3].position.y,
                "light-position-z-4", &mapvals.lightsource[3].position.z,
                "light-direction-x-4", &mapvals.lightsource[3].direction.x,
                "light-direction-y-4", &mapvals.lightsource[3].direction.y,
                "light-direction-z-4", &mapvals.lightsource[3].direction.z,
                "light-color-5", &color_5,
                "light-intensity-5", &mapvals.lightsource[4].intensity,
                "light-position-x-5", &mapvals.lightsource[4].position.x,
                "light-position-y-5", &mapvals.lightsource[4].position.y,
                "light-position-z-5", &mapvals.lightsource[4].position.z,
                "light-direction-x-5", &mapvals.lightsource[4].direction.x,
                "light-direction-y-5", &mapvals.lightsource[4].direction.y,
                "light-direction-z-5", &mapvals.lightsource[4].direction.z,
                "light-color-6", &color_6,
                "light-intensity-6", &mapvals.lightsource[5].intensity,
                "light-position-x-6", &mapvals.lightsource[5].position.x,
                "light-position-y-6", &mapvals.lightsource[5].position.y,
                "light-position-z-6", &mapvals.lightsource[5].position.z,
                "light-direction-x-6", &mapvals.lightsource[5].direction.x,
                "light-direction-y-6", &mapvals.lightsource[5].direction.y,
                "light-direction-z-6", &mapvals.lightsource[5].direction.z,
                "bump-drawable", &temp_bump_map_drawable,
                "env-drawable", &temp_env_map_drawable,
                NULL);

  if (color_1)
    {
      gegl_color_get_pixel (color_1, babl_format ("R'G'B'A double"),
                            &mapvals.lightsource[0].color);
      g_object_unref (color_1);
    }
  if (color_2)
    {
      gegl_color_get_pixel (color_2, babl_format ("R'G'B'A double"),
                            &mapvals.lightsource[1].color);
      g_object_unref (color_2);
    }
  if (color_3)
    {
      gegl_color_get_pixel (color_3, babl_format ("R'G'B'A double"),
                            &mapvals.lightsource[2].color);
      g_object_unref (color_3);
    }
  if (color_4)
    {
      gegl_color_get_pixel (color_4, babl_format ("R'G'B'A double"),
                            &mapvals.lightsource[3].color);
      g_object_unref (color_4);
    }
  if (color_5)
    {
      gegl_color_get_pixel (color_5, babl_format ("R'G'B'A double"),
                            &mapvals.lightsource[4].color);
      g_object_unref (color_5);
    }
  if (color_6)
    {
      gegl_color_get_pixel (color_6, babl_format ("R'G'B'A double"),
                            &mapvals.lightsource[5].color);
      g_object_unref (color_6);
    }

  mapvals.bumpmaptype    = gimp_procedure_config_get_choice_id (config, "bumpmap-type");
  mapvals.light_selected = gimp_procedure_config_get_choice_id (config, "which-light");

  mapvals.lightsource[0].type = gimp_procedure_config_get_choice_id (config, "light-type-1");
  mapvals.lightsource[1].type = gimp_procedure_config_get_choice_id (config, "light-type-2");
  mapvals.lightsource[2].type = gimp_procedure_config_get_choice_id (config, "light-type-3");
  mapvals.lightsource[3].type = gimp_procedure_config_get_choice_id (config, "light-type-4");
  mapvals.lightsource[4].type = gimp_procedure_config_get_choice_id (config, "light-type-5");
  mapvals.lightsource[5].type = gimp_procedure_config_get_choice_id (config, "light-type-6");

  if (temp_bump_map_drawable)
    mapvals.bumpmap_id = gimp_item_get_id (GIMP_ITEM (temp_bump_map_drawable));
  else
    mapvals.bumpmap_id = -1;

  if (temp_env_map_drawable)
    mapvals.envmap_id = gimp_item_get_id (GIMP_ITEM (temp_env_map_drawable));
  else
    mapvals.envmap_id = -1;
}
