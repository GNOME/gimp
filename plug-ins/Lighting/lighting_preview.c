/*************************************************/
/* Compute a preview image and preview wireframe */
/*************************************************/

#include "lighting_preview.h"

gint lightx,lighty;
BackBuffer backbuf={0,0,0,0,NULL};
gdouble *xpostab=NULL,*ypostab=NULL;

/* Protos */
/* ====== */

void draw_preview_image (gint recompute);
void update_light       (gint xpos,gint ypos);
void draw_light_marker  (gint xpos,gint ypos);
void clear_light_marker (void);

void compute_preview (gint startx,gint starty,gint w,gint h)
{
  gint xcnt,ycnt,f1,f2;
  gdouble imagex,imagey;
  gint32 index=0;
  GckRGB color,darkcheck,lightcheck,temp;
  GckVector3 pos;
  get_ray_func ray_func;

  xpostab = (gdouble *)malloc(sizeof(gdouble)*w);
  ypostab = (gdouble *)malloc(sizeof(gdouble)*h);

  for (xcnt=0;xcnt<w;xcnt++)
    xpostab[xcnt]=(gdouble)width*((gdouble)xcnt/(gdouble)w);
  for (ycnt=0;ycnt<h;ycnt++)
    ypostab[ycnt]=(gdouble)height*((gdouble)ycnt/(gdouble)h);

  init_compute();
  precompute_init(width,height);
  
  gck_rgb_set(&lightcheck,0.75,0.75,0.75);
  gck_rgb_set(&darkcheck, 0.50,0.50,0.50);
  gck_rgb_set(&color,0.3,0.7,1.0);
  
  if (mapvals.bump_mapped==TRUE && mapvals.bumpmap_id!=-1)
    {
      gimp_pixel_rgn_init (&bump_region, gimp_drawable_get(mapvals.bumpmap_id), 
        0, 0, width, height, FALSE, FALSE);
    }

  imagey=0;

  if (mapvals.previewquality)
    ray_func = get_ray_color;
  else
   ray_func = get_ray_color_no_bilinear;
  
  if (mapvals.env_mapped==TRUE && mapvals.envmap_id!=-1) 
    {
      env_width = gimp_drawable_width(mapvals.envmap_id);
      env_height = gimp_drawable_height(mapvals.envmap_id);
      gimp_pixel_rgn_init (&env_region, gimp_drawable_get(mapvals.envmap_id), 
        0, 0, env_width, env_height, FALSE, FALSE);

      if (mapvals.previewquality)
        ray_func = get_ray_color_ref;
      else
        ray_func = get_ray_color_no_bilinear_ref;      
    }

  for (ycnt=0;ycnt<PREVIEW_HEIGHT;ycnt++)
    {
      for (xcnt=0;xcnt<PREVIEW_WIDTH;xcnt++)
        {
          if ((ycnt>=starty && ycnt<(starty+h)) &&
              (xcnt>=startx && xcnt<(startx+w)))
            {
               imagex=xpostab[xcnt-startx];
               imagey=ypostab[ycnt-starty];
               pos=int_to_posf(imagex,imagey);
               if (mapvals.bump_mapped==TRUE && mapvals.bumpmap_id!=-1 && xcnt==startx)
                 {
                   pos_to_float(pos.x,pos.y,&imagex,&imagey);
                   precompute_normals(0,width,(gint)(imagey+0.5));
                 }

               color=(*ray_func)(&pos);
               
               if (color.a<1.0)
                 {
                   f1=((xcnt % 32)<16);
                   f2=((ycnt % 32)<16);
                   f1=f1^f2;
    
                   if (f1)
                     {
                       if (color.a==0.0)
                         color=lightcheck;
                       else
                         {
                           gck_rgb_mul(&color,color.a);
                           temp=lightcheck;
                           gck_rgb_mul(&temp,1.0-color.a);
                           gck_rgb_add(&color,&temp);
                         }
                     }
                   else
                     {
                       if (color.a==0.0)
                         color=darkcheck;
                       else
                         {
                           gck_rgb_mul(&color,color.a);
                           temp=darkcheck;
                           gck_rgb_mul(&temp,1.0-color.a);
                           gck_rgb_add(&color,&temp);
                         }
                     }
                 }

               preview_rgb_data[index++]=(guchar)(255.0*color.r);
               preview_rgb_data[index++]=(guchar)(255.0*color.g);
               preview_rgb_data[index++]=(guchar)(255.0*color.b);
               imagex++;
            }
          else
            {
              preview_rgb_data[index++]=200;
              preview_rgb_data[index++]=200;
              preview_rgb_data[index++]=200;
            }
        }
    }

  gck_rgb_to_gdkimage(appwin->visinfo, preview_rgb_data, image, PREVIEW_WIDTH, PREVIEW_HEIGHT);
}

void blah()
{
  /* First, compute the linear mapping (x,y,x+w,y+h) to (0,0,pw,ph) */
  /* ============================================================== */

/*  realw=(p2.x-p1.x);
  realh=(p2.y-p1.y);

  for (xcnt=0;xcnt<pw;xcnt++)
    xpostab[xcnt]=p1.x+realw*((double)xcnt/(double)pw);
 
  for (ycnt=0;ycnt<ph;ycnt++)
    ypostab[ycnt]=p1.y+realh*((double)ycnt/(double)ph); */
  
  /* Compute preview using the offset tables */
  /* ======================================= */

/*  if (mapvals.transparent_background==TRUE)
    gck_rgba_set(&background,0.0,0.0,0.0,0.0);
  else
    {
      gimp_palette_get_background(&r,&g,&b);
      background.r=(gdouble)r/255.0;
      background.g=(gdouble)g/255.0;
      background.b=(gdouble)b/255.0;
      background.a=1.0;
    }

  gck_rgb_set(&lightcheck,0.75,0.75,0.75);
  gck_rgb_set(&darkcheck, 0.50,0.50,0.50);
  gck_vector3_set(&p2,-1.0,-1.0,0.0);

  for (ycnt=0;ycnt<ph;ycnt++)
    {
      for (xcnt=0;xcnt<pw;xcnt++)
        {
          p1.x=xpostab[xcnt];
          p1.y=ypostab[ycnt]; */
          
          /* If oldpos = newpos => same color, so skip shading */
          /* ================================================= */
          
/*          p2=p1;
          color=get_ray_color(&p1);

          if (color.a<1.0)
            {
              f1=((xcnt % 32)<16);
              f2=((ycnt % 32)<16);
              f1=f1^f2;

              if (f1)
                {
                  if (color.a==0.0)
                    color=lightcheck;
                  else
                    {
                      gck_rgb_mul(&color,color.a);
                      temp=lightcheck;
                      gck_rgb_mul(&temp,1.0-color.a);
                      gck_rgb_add(&color,&temp);
                    }
                }
              else
                {
                  if (color.a==0.0)
                    color=darkcheck;
                  else
                    {
                      gck_rgb_mul(&color,color.a);
                      temp=darkcheck;
                      gck_rgb_mul(&temp,1.0-color.a);
                      gck_rgb_add(&color,&temp);
                    }
                }
            }

          preview_rgb_data[index++]=(guchar)(color.r*255.0);
          preview_rgb_data[index++]=(guchar)(color.g*255.0);
          preview_rgb_data[index++]=(guchar)(color.b*255.0);
        }
    } */

  /* Convert to visual type */
  /* ====================== */

/*  gck_rgb_to_gdkimage(appwin->visinfo,preview_rgb_data,image,pw,ph); */

}

/*************************************************/
/* Check if the given position is within the     */
/* light marker. Return TRUE if so, FALSE if not */
/*************************************************/

gint check_light_hit(gint xpos,gint ypos)
{
/*  gdouble dx,dy,r;
  
  if (mapvals.lightsource.type==POINT_LIGHT)
    {
      dx=(gdouble)lightx-xpos;
      dy=(gdouble)lighty-ypos;
      r=sqrt(dx*dx+dy*dy)+0.5;

      if ((gint)r>7)
        return(FALSE);
      else
        return(TRUE);
    }
   */
  return(FALSE);
}

/****************************************/
/* Draw a marker to show light position */
/****************************************/

void draw_light_marker(gint xpos,gint ypos)
{
/*  gck_gc_set_foreground(appwin->visinfo,gc,0,50,255);
  gck_gc_set_background(appwin->visinfo,gc,0,0,0);

  gdk_gc_set_function(gc,GDK_COPY);

  if (mapvals.lightsource.type==POINT_LIGHT)
    {
      lightx=xpos;
      lighty=ypos; */
    
      /* Save background */
      /* =============== */
 
/*      backbuf.x=lightx-7;
      backbuf.y=lighty-7;
      backbuf.w=14;
      backbuf.h=14; */
    
      /* X doesn't like images that's outside a window, make sure */
      /* we get the backbuffer image from within the boundaries   */
      /* ======================================================== */
 
/*      if (backbuf.x<0)
        backbuf.x=0;
      else if ((backbuf.x+backbuf.w)>PREVIEW_WIDTH)
        backbuf.w=(PREVIEW_WIDTH-backbuf.x);

      if (backbuf.y<0)
        backbuf.y=0;
      else if ((backbuf.y+backbuf.h)>PREVIEW_HEIGHT)
        backbuf.h=(PREVIEW_WIDTH-backbuf.y);
 
      backbuf.image=gdk_image_get(previewarea->window,backbuf.x,backbuf.y,backbuf.w,backbuf.h);
      gdk_draw_arc(previewarea->window,gc,TRUE,lightx-7,lighty-7,14,14,0,360*64);
    } */
}

void clear_light_marker(void)
{
  /* Restore background if it has been saved */
  /* ======================================= */
  
/*  if (backbuf.image!=NULL)
    {
      gck_gc_set_foreground(appwin->visinfo,gc,255,255,255);
      gck_gc_set_background(appwin->visinfo,gc,0,0,0);

      gdk_gc_set_function(gc,GDK_COPY);
      gdk_draw_image(previewarea->window,gc,backbuf.image,0,0,backbuf.x,backbuf.y,
        backbuf.w,backbuf.h);
      gdk_image_destroy(backbuf.image);
      backbuf.image=NULL;
    } */
}

void draw_lights(void)
{
/*  gdouble dxpos,dypos;
  gint xpos,ypos;

  clear_light_marker();
 
  gck_3d_to_2d(startx,starty,pw,ph,&dxpos,&dypos,&mapvals.viewpoint,
    &mapvals.lightsource.position);

  xpos=(gint)(dxpos+0.5);
  ypos=(gint)(dypos+0.5);

  if (xpos>=0 && xpos<=PREVIEW_WIDTH && ypos>=0 && ypos<=PREVIEW_HEIGHT)
    draw_light_marker(xpos,ypos); */
}

/*************************************************/
/* Update light position given new screen coords */
/*************************************************/

void update_light(gint xpos,gint ypos)
{
/*  gint startx,starty,pw,ph;

  pw=PREVIEW_WIDTH >> mapvals.preview_zoom_factor;
  ph=PREVIEW_HEIGHT >> mapvals.preview_zoom_factor;
  startx=(PREVIEW_WIDTH-pw)>>1;
  starty=(PREVIEW_HEIGHT-ph)>>1;
  
  gck_2d_to_3d(startx,starty,pw,ph,xpos,ypos,&mapvals.viewpoint,
    &mapvals.lightsource.position);

  draw_lights(startx,starty,pw,ph); */
}

void compute_preview_rectangle(gint *xp,gint *yp,gint *wid,gint *heig)
{
  gdouble x,y,w,h;

  if (width>=height)
    {
      w=(PREVIEW_WIDTH-50.0);
      h=(gdouble)height*(w/(gdouble)width);
      
      x=(PREVIEW_WIDTH-w)/2.0;
      y=(PREVIEW_HEIGHT-h)/2.0;
    }
  else
    {
      h=(PREVIEW_HEIGHT-50.0);
      w=(gdouble)width*(h/(gdouble)height);
      
      x=(PREVIEW_WIDTH-w)/2.0;
      y=(PREVIEW_HEIGHT-h)/2.0;
    }

  *xp=(gint)(x+0.5);
  *yp=(gint)(y+0.5);
  *wid=(gint)(w+0.5);
  *heig=(gint)(h+0.5);
}

/******************************************************************/
/* Draw preview image. if DoCompute is TRUE then recompute image. */
/******************************************************************/

void draw_preview_image(gint recompute)
{
  gint startx,starty,pw,ph;
  
  gck_gc_set_foreground(appwin->visinfo,gc,255,255,255);
  gck_gc_set_background(appwin->visinfo,gc,0,0,0);

  gdk_gc_set_function(gc,GDK_COPY);

  compute_preview_rectangle(&startx,&starty,&pw,&ph);

  if (recompute==TRUE)
    {
      gck_cursor_set(previewarea->window,GDK_WATCH);
      compute_preview(startx,starty,pw,ph);
      gck_cursor_set(previewarea->window,GDK_HAND2);
      clear_light_marker();
    }

/*  if (pw!=PREVIEW_WIDTH)
    gdk_window_clear(previewarea->window); */

  gdk_draw_image(previewarea->window,gc,image,0,0,0,0,PREVIEW_WIDTH,PREVIEW_HEIGHT);

/*  draw_lights(); */
}

