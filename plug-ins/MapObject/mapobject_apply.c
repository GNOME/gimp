/******************************************************/
/* Apply mapping and shading on the whole input image */
/******************************************************/

#include "mapobject_shade.h"

/*************/
/* Main loop */
/*************/

gdouble imat[4][4];
gfloat rotmat[16], a[16], b[16];

void init_compute(void)
{
  gint i;

  switch (mapvals.maptype)
    {
      case MAP_SPHERE:

        /* Rotate the equator/northpole axis */
        /* ================================= */
    
        gck_vector3_set(&mapvals.firstaxis,0.0,0.0,-1.0);
        gck_vector3_set(&mapvals.secondaxis,0.0,1.0,0.0);
    
        gck_vector3_rotate(&mapvals.firstaxis,gck_deg_to_rad(mapvals.alpha),
                           gck_deg_to_rad(mapvals.beta),gck_deg_to_rad(mapvals.gamma));
        gck_vector3_rotate(&mapvals.secondaxis,gck_deg_to_rad(mapvals.alpha),
                           gck_deg_to_rad(mapvals.beta),gck_deg_to_rad(mapvals.gamma));
    
        /* Compute the 2D bounding box of the sphere spanned by the axis */
        /* ============================================================= */
    
        compute_bounding_box();

        get_ray_color=get_ray_color_sphere;

        break;

      case MAP_PLANE:

        /* Rotate the plane axis */
        /* ===================== */
    
        gck_vector3_set(&mapvals.firstaxis, 1.0,0.0,0.0);
        gck_vector3_set(&mapvals.secondaxis,0.0,1.0,0.0);
        gck_vector3_set(&mapvals.normal,0.0,0.0,1.0);
    
        gck_vector3_rotate(&mapvals.firstaxis,gck_deg_to_rad(mapvals.alpha),
                           gck_deg_to_rad(mapvals.beta),gck_deg_to_rad(mapvals.gamma));
        gck_vector3_rotate(&mapvals.secondaxis,gck_deg_to_rad(mapvals.alpha),
                           gck_deg_to_rad(mapvals.beta),gck_deg_to_rad(mapvals.gamma));
    
        mapvals.normal=gck_vector3_cross_product(&mapvals.firstaxis,&mapvals.secondaxis);
    
        if (mapvals.normal.z<0.0)
          gck_vector3_mul(&mapvals.normal,-1.0);
    
        /* Initialize intersection matrix */
        /* ============================== */
    
        imat[0][1]=-mapvals.firstaxis.x;
        imat[1][1]=-mapvals.firstaxis.y;
        imat[2][1]=-mapvals.firstaxis.z; 
  
        imat[0][2]=-mapvals.secondaxis.x;
        imat[1][2]=-mapvals.secondaxis.y;
        imat[2][2]=-mapvals.secondaxis.z;
    
        imat[0][3]=mapvals.position.x-mapvals.viewpoint.x;
        imat[1][3]=mapvals.position.y-mapvals.viewpoint.y;
        imat[2][3]=mapvals.position.z-mapvals.viewpoint.z;

        get_ray_color=get_ray_color_plane;

        break;
      case MAP_BOX:
        gck_vector3_set(&mapvals.firstaxis, 1.0,0.0,0.0);
        gck_vector3_set(&mapvals.secondaxis,0.0,1.0,0.0);
        gck_vector3_set(&mapvals.normal,0.0,0.0,1.0);

        get_ray_color=get_ray_color_box;
        
        ident_mat(rotmat);
        
        rotatemat(mapvals.alpha, &mapvals.firstaxis, a);

        matmul(a,rotmat,b);

        memcpy(rotmat, b, sizeof(gfloat)*16);

        rotatemat(mapvals.beta, &mapvals.secondaxis, a);
        matmul(a,rotmat,b);

        memcpy(rotmat, b, sizeof(gfloat)*16);

        rotatemat(mapvals.gamma, &mapvals.normal, a);
        matmul(a,rotmat,b);

        memcpy(rotmat, b, sizeof(gfloat)*16);

        /* Set up pixel regions for the box face images */
        /* ============================================ */

        for (i=0;i<6;i++)
          {
             box_drawables[i] = gimp_drawable_get (mapvals.boxmap_id[i]);
             
             gimp_pixel_rgn_init (&box_regions[i], box_drawables[i],
                                 0, 0,
                                 box_drawables[i]->width, box_drawables[i]->height,
                                 FALSE, FALSE);
          }
        break;
      case MAP_CYLINDER:
        get_ray_color=get_ray_color_cylinder;

        gck_vector3_set(&mapvals.firstaxis, 1.0,0.0,0.0);
        gck_vector3_set(&mapvals.secondaxis,0.0,1.0,0.0);
        gck_vector3_set(&mapvals.normal,0.0,0.0,1.0);

        ident_mat(rotmat);
        
        rotatemat(mapvals.alpha, &mapvals.firstaxis, a);

        matmul(a,rotmat,b);

        memcpy(rotmat, b, sizeof(gfloat)*16);

        rotatemat(mapvals.beta, &mapvals.secondaxis, a);
        matmul(a,rotmat,b);

        memcpy(rotmat, b, sizeof(gfloat)*16);

        rotatemat(mapvals.gamma, &mapvals.normal, a);
        matmul(a,rotmat,b);

        memcpy(rotmat, b, sizeof(gfloat)*16);

        /* Set up pixel regions for the cylinder cap images */
        /* ================================================ */

        for (i=0;i<2;i++)
          {
             cylinder_drawables[i] = gimp_drawable_get (mapvals.cylindermap_id[i]);
             
             gimp_pixel_rgn_init (&cylinder_regions[i], cylinder_drawables[i],
                                 0, 0,
                                 cylinder_drawables[i]->width, cylinder_drawables[i]->height,
                                 FALSE, FALSE);
          }
        break;
    }

  max_depth=(gint)mapvals.maxdepth;
}

void render(gdouble x,gdouble y,GckRGB *col)
{
  GckVector3 pos;
  
  pos.x=x/(gdouble)width;
  pos.y=y/(gdouble)height;
  pos.z=0.0;
  
  *col=get_ray_color(&pos);
}

void show_progress(gint min,gint max,gint curr)
{
  gimp_progress_update((gdouble)curr/(gdouble)max);
}

/**************************************************/
/* Performs map-to-sphere on the whole input image */
/* and updates or creates a new GIMP image.       */
/**************************************************/

void compute_image(void)
{
  gint xcount,ycount;
  GckRGB color;
  glong progress_counter=0;
  GckVector3 p;
  gint32 new_image_id=-1,new_layer_id=-1;

  init_compute();
  
  if (mapvals.create_new_image==TRUE || (mapvals.transparent_background==TRUE 
      && input_drawable->bpp!=4))
    {
      /* Create a new image */
      /* ================== */

      new_image_id=gimp_image_new(width,height,RGB);

      if (mapvals.transparent_background==TRUE)
        {
          /* Add a layer with an alpha channel */
          /* ================================= */

          new_layer_id=gimp_layer_new(new_image_id,"Background",width,height,RGBA_IMAGE,100.0,NORMAL_MODE);
        }
      else
        {
          /* Create a "normal" layer */
          /* ======================= */

          new_layer_id=gimp_layer_new(new_image_id,"Background",width,height,RGB_IMAGE,100.0,NORMAL_MODE);
        }

      gimp_image_add_layer(new_image_id,new_layer_id,0);
      output_drawable=gimp_drawable_get(new_layer_id);
    }

  gimp_pixel_rgn_init (&dest_region, output_drawable, 0, 0, width, height, TRUE, TRUE);

  switch (mapvals.maptype)
    {
      case MAP_PLANE:
        gimp_progress_init("Map to object (plane)");
        break;
      case MAP_SPHERE:
        gimp_progress_init("Map to object (sphere)");
        break;
      case MAP_BOX:
        gimp_progress_init("Map to object (box)");
        break;
      case MAP_CYLINDER:
        gimp_progress_init("Map to object (cylinder)");
        break;
    }

  if (mapvals.antialiasing==FALSE)
    {
      for (ycount=0;ycount<height;ycount++)
        {
          for (xcount=0;xcount<width;xcount++)
            {
              p=int_to_pos(xcount,ycount);
              color=(*get_ray_color)(&p);
              poke(xcount,ycount,&color);
              if ((progress_counter++ % width)==0)
                gimp_progress_update((gdouble)progress_counter/(gdouble)maxcounter);
            }
        }
    }
  else
    gck_adaptive_supersample_area(0,0,width-1,height-1,max_depth,mapvals.pixeltreshold,
      render,poke,show_progress);

  /* Update the region */
  /* ================= */

  gimp_drawable_flush (output_drawable);
  gimp_drawable_merge_shadow (output_drawable->id, TRUE);
  gimp_drawable_update (output_drawable->id, 0, 0, width,height);

  if (new_image_id!=-1)
    {
      gimp_display_new(new_image_id);
      gimp_displays_flush();
      gimp_drawable_detach (output_drawable);
    }
}

