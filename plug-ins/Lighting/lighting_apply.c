/******************************************************/
/* Apply mapping and shading on the whole input image */
/******************************************************/

#include "lighting_shade.h"

/*************/
/* Main loop */
/*************/

get_ray_func ray_func;

void init_compute(void)
{
}

void render(gdouble x,gdouble y,GckRGB *col)
{
  GimpVector3 pos;

  pos=int_to_pos(x,y);

  *col=(*ray_func)(&pos);
}

void show_progress(gint min,gint max,gint curr)
{
  gimp_progress_update((gdouble)curr/(gdouble)max);
}

void compute_image(void)
{
  gint xcount,ycount;
  GckRGB color;
  glong progress_counter=0;
  GimpVector3 p;
  gint32 new_image_id=-1,new_layer_id=-1,index;
  guchar *row = NULL, obpp;

  gint has_alpha;

  init_compute();
  
  if (mapvals.create_new_image==TRUE || (mapvals.transparent_background==TRUE 
      && !gimp_drawable_has_alpha(input_drawable->id)))
    {
      /* Create a new image */
      /* ================== */

      new_image_id=gimp_image_new(width,height,RGB);

      if (mapvals.transparent_background==TRUE)
        {
          /* Add a layer with an alpha channel */
          /* ================================= */

          new_layer_id=gimp_layer_new(new_image_id,"Background",
            width,height,RGBA_IMAGE,100.0,NORMAL_MODE);
        }
      else
        {
          /* Create a "normal" layer */
          /* ======================= */

          new_layer_id=gimp_layer_new(new_image_id,"Background",
            width,height,RGB_IMAGE,100.0,NORMAL_MODE);
        }

      gimp_image_add_layer(new_image_id,new_layer_id,0);
      output_drawable=gimp_drawable_get(new_layer_id);
    }

  if (mapvals.bump_mapped==TRUE && mapvals.bumpmap_id!=-1)
    {
      gimp_pixel_rgn_init (&bump_region, gimp_drawable_get(mapvals.bumpmap_id),
        0, 0, width, height, FALSE, FALSE);
      precompute_init(width,height);
    }

  if (!mapvals.env_mapped || mapvals.envmap_id==-1)
     ray_func = get_ray_color;
  else
    {
      gimp_pixel_rgn_init (&env_region, gimp_drawable_get(mapvals.envmap_id),
        0, 0, env_width, env_height, FALSE, FALSE);
      ray_func = get_ray_color_ref;
    }

  gimp_pixel_rgn_init (&dest_region, output_drawable, 0,0, width,height, TRUE,TRUE);

  obpp=gimp_drawable_bpp(output_drawable->id);
  has_alpha=gimp_drawable_has_alpha(output_drawable->id);

  row = (guchar *)malloc(sizeof(guchar)*(size_t)(obpp)*(size_t)(width));

  gimp_progress_init("Lighting Effects");

/*  if (mapvals.antialiasing==FALSE)
    { */
      for (ycount=0;ycount<height;ycount++)
        {
          if (mapvals.bump_mapped==TRUE && mapvals.bumpmap_id!=-1)
            precompute_normals(0,width,ycount);

          index=0;
          for (xcount=0;xcount<width;xcount++)
            {
              p=int_to_pos(xcount,ycount);
              color=(*ray_func)(&p);

              row[index++]=(guchar)(color.r*255.0);
              row[index++]=(guchar)(color.g*255.0);
              row[index++]=(guchar)(color.b*255.0);

              if (has_alpha)
                row[index++]=(guchar)(color.a*255.0);

              if ((progress_counter++ % width)==0)
                gimp_progress_update((gdouble)progress_counter/(gdouble)maxcounter);
            }

          gimp_pixel_rgn_set_row(&dest_region,row,0,ycount,width);
        }
/*    }
  else
    gck_adaptive_supersample_area(0,0,width-1,height-1,(gint)mapvals.max_depth,
      mapvals.pixel_treshold,render,poke,show_progress); */

  if (row!=NULL)
    free(row);

  /* Update image */
  /* ============ */

  gimp_drawable_flush(output_drawable);
  gimp_drawable_merge_shadow(output_drawable->id, TRUE);
  gimp_drawable_update(output_drawable->id, 0,0, width,height);

  if (new_image_id!=-1)
    {
      gimp_display_new(new_image_id);
      gimp_displays_flush();
      gimp_drawable_detach(output_drawable);
    }

}

