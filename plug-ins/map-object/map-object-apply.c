/******************************************************/
/* Apply mapping and shading on the whole input image */
/******************************************************/

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "map-object-main.h"
#include "map-object-image.h"
#include "map-object-shade.h"
#include "map-object-apply.h"

#include "libgimp/stdplugins-intl.h"


/*************/
/* Main loop */
/*************/

gdouble       imat[4][4];
gfloat        rotmat[16];
static gfloat a[16], b[16];

void
init_compute (void)
{
  gint i;

  switch (mapvals.maptype)
    {
      case MAP_SPHERE:

        /* Rotate the equator/northpole axis */
        /* ================================= */

        gimp_vector3_set (&mapvals.firstaxis,  0.0, 0.0, -1.0);
        gimp_vector3_set (&mapvals.secondaxis, 0.0, 1.0,  0.0);

        gimp_vector3_rotate (&mapvals.firstaxis,
                             gimp_deg_to_rad (mapvals.alpha),
                             gimp_deg_to_rad (mapvals.beta),
                             gimp_deg_to_rad (mapvals.gamma));
        gimp_vector3_rotate (&mapvals.secondaxis,
                             gimp_deg_to_rad (mapvals.alpha),
                             gimp_deg_to_rad (mapvals.beta),
                             gimp_deg_to_rad (mapvals.gamma));

        /* Compute the 2D bounding box of the sphere spanned by the axis */
        /* ============================================================= */

        compute_bounding_box ();

        get_ray_color = get_ray_color_sphere;

        break;

      case MAP_PLANE:

        /* Rotate the plane axis */
        /* ===================== */

        gimp_vector3_set (&mapvals.firstaxis,  1.0, 0.0, 0.0);
        gimp_vector3_set (&mapvals.secondaxis, 0.0, 1.0, 0.0);
        gimp_vector3_set (&mapvals.normal,     0.0, 0.0, 1.0);

        gimp_vector3_rotate (&mapvals.firstaxis,
                             gimp_deg_to_rad (mapvals.alpha),
                             gimp_deg_to_rad (mapvals.beta),
                             gimp_deg_to_rad (mapvals.gamma));
        gimp_vector3_rotate (&mapvals.secondaxis,
                             gimp_deg_to_rad (mapvals.alpha),
                             gimp_deg_to_rad (mapvals.beta),
                             gimp_deg_to_rad (mapvals.gamma));

        mapvals.normal = gimp_vector3_cross_product (&mapvals.firstaxis,
                                                     &mapvals.secondaxis);

        if (mapvals.normal.z < 0.0)
          gimp_vector3_mul (&mapvals.normal, -1.0);

        /* Initialize intersection matrix */
        /* ============================== */

        imat[0][1] = -mapvals.firstaxis.x;
        imat[1][1] = -mapvals.firstaxis.y;
        imat[2][1] = -mapvals.firstaxis.z;

        imat[0][2] = -mapvals.secondaxis.x;
        imat[1][2] = -mapvals.secondaxis.y;
        imat[2][2] = -mapvals.secondaxis.z;

        imat[0][3] = mapvals.position.x - mapvals.viewpoint.x;
        imat[1][3] = mapvals.position.y - mapvals.viewpoint.y;
        imat[2][3] = mapvals.position.z - mapvals.viewpoint.z;

        get_ray_color = get_ray_color_plane;

        break;

      case MAP_BOX:
        get_ray_color = get_ray_color_box;

        gimp_vector3_set (&mapvals.firstaxis,  1.0, 0.0, 0.0);
        gimp_vector3_set (&mapvals.secondaxis, 0.0, 1.0, 0.0);
        gimp_vector3_set (&mapvals.normal,     0.0, 0.0, 1.0);

        ident_mat (rotmat);

        rotatemat (mapvals.alpha, &mapvals.firstaxis, a);

        matmul (a, rotmat, b);

        memcpy (rotmat, b, sizeof (gfloat) * 16);

        rotatemat (mapvals.beta, &mapvals.secondaxis, a);
        matmul (a, rotmat, b);

        memcpy (rotmat, b, sizeof (gfloat) * 16);

        rotatemat (mapvals.gamma, &mapvals.normal, a);
        matmul (a, rotmat, b);

        memcpy (rotmat, b, sizeof (gfloat) * 16);

        /* Set up pixel regions for the box face images */
        /* ============================================ */

        for (i = 0; i < 6; i++)
          {
            box_drawable_ids[i] = mapvals.boxmap_id[i];

            box_buffers[i] = gimp_drawable_get_buffer (box_drawable_ids[i]);
          }

        break;

      case MAP_CYLINDER:
        get_ray_color = get_ray_color_cylinder;

        gimp_vector3_set (&mapvals.firstaxis,  1.0, 0.0, 0.0);
        gimp_vector3_set (&mapvals.secondaxis, 0.0, 1.0, 0.0);
        gimp_vector3_set (&mapvals.normal,     0.0, 0.0, 1.0);

        ident_mat (rotmat);

        rotatemat (mapvals.alpha, &mapvals.firstaxis, a);

        matmul (a, rotmat, b);

        memcpy (rotmat, b, sizeof (gfloat) * 16);

        rotatemat (mapvals.beta, &mapvals.secondaxis, a);
        matmul (a, rotmat, b);

        memcpy (rotmat, b, sizeof (gfloat) * 16);

        rotatemat (mapvals.gamma, &mapvals.normal, a);
        matmul (a, rotmat, b);

        memcpy (rotmat, b, sizeof (gfloat) * 16);

        /* Set up pixel regions for the cylinder cap images */
        /* ================================================ */

        for (i = 0; i < 2; i++)
          {
            cylinder_drawable_ids[i] = mapvals.cylindermap_id[i];

            cylinder_buffers[i] = gimp_drawable_get_buffer (cylinder_drawable_ids[i]);
          }
        break;
    }

  max_depth = (gint) mapvals.maxdepth;
}

static void
render (gdouble   x,
        gdouble   y,
        GimpRGB  *col,
        gpointer  data)
{
  GimpVector3 pos;

  pos.x = x / (gdouble) width;
  pos.y = y / (gdouble) height;
  pos.z = 0.0;

  *col = get_ray_color (&pos);
}

static void
show_progress (gint     min,
               gint     max,
               gint     curr,
               gpointer data)
{
  gimp_progress_update ((gdouble) curr / (gdouble) max);
}

/**************************************************/
/* Performs map-to-sphere on the whole input image */
/* and updates or creates a new GIMP image.       */
/**************************************************/

void
compute_image (void)
{
  gint         xcount, ycount;
  GimpRGB      color;
  glong        progress_counter = 0;
  GimpVector3  p;
  gint32       new_image_id = -1;
  gint32       new_layer_id = -1;
  gboolean     insert_layer = FALSE;

  init_compute ();

  if (mapvals.create_new_image)
    {
      new_image_id = gimp_image_new (width, height, GIMP_RGB);
    }
  else
    {
      new_image_id = image_id;
    }

  gimp_image_undo_group_start (new_image_id);

  if (mapvals.create_new_image ||
      mapvals.create_new_layer ||
      (mapvals.transparent_background &&
       ! gimp_drawable_has_alpha (output_drawable_id)))
    {
      gchar *layername[] = {_("Map to plane"),
                            _("Map to sphere"),
                            _("Map to box"),
                            _("Map to cylinder"),
                            _("Background")};

      new_layer_id = gimp_layer_new (new_image_id,
                                     layername[mapvals.create_new_image ? 4 :
                                               mapvals.maptype],
                                     width, height,
                                     mapvals.transparent_background ?
                                     GIMP_RGBA_IMAGE :
                                     GIMP_RGB_IMAGE,
                                     100.0,
                                     gimp_image_get_default_new_layer_mode (new_image_id));

      insert_layer = TRUE;
      output_drawable_id = new_layer_id;
    }

  dest_buffer = gimp_drawable_get_shadow_buffer (output_drawable_id);

  switch (mapvals.maptype)
    {
      case MAP_PLANE:
        gimp_progress_init (_("Map to plane"));
        break;
      case MAP_SPHERE:
        gimp_progress_init (_("Map to sphere"));
        break;
      case MAP_BOX:
        gimp_progress_init (_("Map to box"));
        break;
      case MAP_CYLINDER:
        gimp_progress_init (_("Map to cylinder"));
        break;
    }

  if (! mapvals.antialiasing)
    {
      for (ycount = 0; ycount < height; ycount++)
        {
          for (xcount = 0; xcount < width; xcount++)
            {
              p = int_to_pos (xcount, ycount);
              color = (* get_ray_color) (&p);
              poke (xcount, ycount, &color, NULL);

              progress_counter++;
            }

          gimp_progress_update ((gdouble) progress_counter /
                                (gdouble) maxcounter);
        }
    }
  else
    {
      gimp_adaptive_supersample_area (0, 0,
                                      width - 1, height - 1,
                                      max_depth,
                                      mapvals.pixelthreshold,
                                      render,
                                      NULL,
                                      poke,
                                      NULL,
                                      show_progress,
                                      NULL);
    }

  gimp_progress_update (1.0);

  g_object_unref (source_buffer);
  g_object_unref (dest_buffer);

  if (insert_layer)
    gimp_image_insert_layer (new_image_id, new_layer_id, -1, 0);

  gimp_drawable_merge_shadow (output_drawable_id, TRUE);
  gimp_drawable_update (output_drawable_id, 0, 0, width, height);

  if (new_image_id != image_id)
    {
      gimp_display_new (new_image_id);
      gimp_displays_flush ();
    }

  gimp_image_undo_group_end (new_image_id);
}
