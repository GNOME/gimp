/*****************/
/* Shading stuff */
/*****************/

#include "lighting_shade.h"

GckVector3 *triangle_normals[2] = { NULL, NULL };
GckVector3 *vertex_normals[3] = { NULL, NULL, NULL };
gdouble *heights[3] = { NULL, NULL, NULL };
gdouble xstep,ystep;
guchar *bumprow=NULL;

gint pre_w=-1;
gint pre_h=-1;

/*****************/
/* Phong shading */
/*****************/

GckRGB phong_shade(GckVector3 *position,GckVector3 *viewpoint,
                   GckVector3 *normal,GckVector3 *lightposition,
                   GckRGB *diff_col,GckRGB *spec_col,
                   LightType light_type)
{
  GckRGB ambient_color,diffuse_color,specular_color;
  gdouble nl,rv,dist;
  GckVector3 l,nn,v,n;

  /* Compute ambient intensity */
  /* ========================= */

  n=*normal;
  ambient_color=*diff_col;
  gck_rgb_mul(&ambient_color,mapvals.material.ambient_int);

  /* Compute (N*L) term of Phong's equation */
  /* ====================================== */

  if (light_type==POINT_LIGHT)
    gck_vector3_sub(&l,lightposition,position);
  else
    l=*lightposition;
  
  dist=gck_vector3_length(&l);
  
  if (dist!=0.0)
    gck_vector3_mul(&l,1.0/dist);

  nl=2.0*gck_vector3_inner_product(&n,&l);

  if (nl>=0.0)
    {
      /* Compute (R*V)^alpha term of Phong's equation */
      /* ============================================ */

      gck_vector3_sub(&v,viewpoint,position);
      gck_vector3_normalize(&v);

      gck_vector3_mul(&n,nl);
      gck_vector3_sub(&nn,&n,&l);
      rv=gck_vector3_inner_product(&nn,&v);
      rv=pow(rv,mapvals.material.highlight);

      /* Compute diffuse and specular intensity contribution */
      /* =================================================== */

      diffuse_color=*diff_col;
      gck_rgb_mul(&diffuse_color,mapvals.material.diffuse_ref);
      gck_rgb_mul(&diffuse_color,nl);

      specular_color=*spec_col;
      gck_rgb_mul(&specular_color,mapvals.material.specular_ref);
      gck_rgb_mul(&specular_color,rv);

      gck_rgb_add(&diffuse_color,&specular_color);
      gck_rgb_mul(&diffuse_color,mapvals.material.diffuse_int);
      gck_rgb_clamp(&diffuse_color);

      gck_rgb_add(&ambient_color,&diffuse_color);
    }
 
  gck_rgb_clamp(&ambient_color);
  return(ambient_color);
}

void get_normal(gdouble xf,gdouble yf,GckVector3 *normal)
{
  GckVector3 v1,v2,n;
  gint numvecs=0,x,y,f;
  gdouble val,val1=-1.0,val2=-1.0,val3=-1.0,val4=-1.0, xstep,ystep;

  x=(gint)(xf+0.5);
  y=(gint)(yf+0.5);

  xstep=1.0/(gdouble)width;
  ystep=1.0/(gdouble)height;

  val=mapvals.bumpmax*get_map_value(&bump_region, xf,yf, &f)/255.0;
  if (check_bounds(x-1,y)) val1=mapvals.bumpmax*get_map_value(&bump_region, xf-1.0,yf, &f)/255.0 - val;
  if (check_bounds(x,y-1)) val2=mapvals.bumpmax*get_map_value(&bump_region, xf,yf-1.0, &f)/255.0 - val;
  if (check_bounds(x+1,y)) val3=mapvals.bumpmax*get_map_value(&bump_region, xf+1.0,yf, &f)/255.0 - val;
  if (check_bounds(x,y+1)) val4=mapvals.bumpmax*get_map_value(&bump_region, xf,yf+1.0, &f)/255.0 - val;

  gck_vector3_set(normal, 0.0,0.0,0.0);

  if (val1!=-1.0 && val4!=-1.0)
    {
      v1.x=-xstep; v1.y=0.0; v1.z=val1;
      v2.x=0.0; v2.y=ystep; v2.z=val4;
      n=gck_vector3_cross_product(&v1,&v2);
      gck_vector3_normalize(&n);
      
      if (n.z<0.0)
        n.z=-n.z;
      
      gck_vector3_add(normal,normal,&n);
      numvecs++;
    }

  if (val1!=-1.0 && val2!=-1.0)
    {
      v1.x=-xstep; v1.y=0.0;    v1.z=val1;
      v2.x=0.0;    v2.y=-ystep; v2.z=val2;
      n=gck_vector3_cross_product(&v1,&v2);
      gck_vector3_normalize(&n);
      
      if (n.z<0.0)
        n.z=-n.z;
      
      gck_vector3_add(normal,normal,&n);
      numvecs++;
    }

  if (val2!=-1.0 && val3!=-1.0)
    {
      v1.x=0.0;   v1.y=-ystep; v1.z=val2;
      v2.x=xstep; v2.y=0.0;    v2.z=val3;
      n=gck_vector3_cross_product(&v1,&v2);
      gck_vector3_normalize(&n);
      
      if (n.z<0.0)
        n.z=-n.z;
      
      gck_vector3_add(normal,normal,&n);
      numvecs++;
    }

  if (val3!=-1.0 && val4!=-1.0)
    {
      v1.x=xstep; v1.y=0.0;   v1.z=val3;
      v2.x=0.0;   v2.y=ystep; v2.z=val4;
      n=gck_vector3_cross_product(&v1,&v2);
      gck_vector3_normalize(&n);
      
      if (n.z<0.0)
        n.z=-n.z;
      
      gck_vector3_add(normal,normal,&n);
      numvecs++;
    }

  gck_vector3_mul(normal,1.0/(gdouble)numvecs);
  gck_vector3_normalize(normal);
}

void precompute_init(gint w,gint h)
{
  gint n;

  xstep=1.0/(gdouble)width;
  ystep=1.0/(gdouble)height;

  pre_w=w;
  pre_h=h;
    
  for (n=0;n<3;n++)
    {
      if (vertex_normals[n]!=NULL)
        free(vertex_normals[n]);
      if (heights[n]!=NULL)
        free(heights[n]);
      
      heights[n]=(gdouble *)malloc(sizeof(gdouble)*(size_t)w);
      vertex_normals[n]=(GckVector3 *)malloc(sizeof(GckVector3)*(size_t)w);
    }

  for (n=0;n<2;n++)
    if (triangle_normals[n]!=NULL)
      free(triangle_normals[n]);

  if (bumprow!=NULL)
    {
      free(bumprow);
      bumprow=NULL;
    }

  bumprow=(guchar *)malloc(sizeof(guchar)*(size_t)w);

  triangle_normals[0]=(GckVector3 *)malloc(sizeof(GckVector3)*(size_t)((w<<1)+2));
  triangle_normals[1]=(GckVector3 *)malloc(sizeof(GckVector3)*(size_t)((w<<1)+2));
  
  for (n=0;n<(w<<1)+1;n++)
    {
      gck_vector3_set(&triangle_normals[0][n],0.0,0.0,1.0);
      gck_vector3_set(&triangle_normals[1][n],0.0,0.0,1.0);
    }

  for (n=0;n<w;n++)
    {
      gck_vector3_set(&vertex_normals[0][n],0.0,0.0,1.0);
      gck_vector3_set(&vertex_normals[1][n],0.0,0.0,1.0);
      gck_vector3_set(&vertex_normals[2][n],0.0,0.0,1.0);
      heights[0][n]=0.0;
      heights[1][n]=0.0;
      heights[2][n]=0.0;
    }
}

/********************************************/
/* Compute triangle and then vertex normals */
/********************************************/

void precompute_normals(gint x1,gint x2,gint y)
{
  GckVector3 *tmpv,p1,p2,p3,normal;
  gdouble *tmpd;
  gint n,i,nv;
  guchar *map=NULL;

  /* First, compute the heights */
  /* ========================== */

  tmpv=triangle_normals[0];
  triangle_normals[0]=triangle_normals[1];
  triangle_normals[1]=tmpv;

  tmpv=vertex_normals[0];
  vertex_normals[0]=vertex_normals[1];
  vertex_normals[1]=vertex_normals[2];
  vertex_normals[2]=tmpv;
  
  tmpd=heights[0];
  heights[0]=heights[1];
  heights[1]=heights[2];
  heights[2]=tmpd;

/*  printf("Get row (%d,%d,%d) to %p\n",x1,y,x2-x1,bumprow); */

  gimp_pixel_rgn_get_row(&bump_region,bumprow,x1,y,x2-x1);

  if (mapvals.bumpmaptype>0)
    {
      switch (mapvals.bumpmaptype)
        {
          case 1:
            map=logmap;
            break;
          case 2:
            map=sinemap;
            break;
          default:
            map=spheremap;
            break;
        }
      for (n=0;n<(x2-x1);n++)
       heights[2][n]=(gdouble)mapvals.bumpmax*(gdouble)map[bumprow[n]]/255.0;
    }
  else for (n=0;n<(x2-x1);n++)
    heights[2][n]=(gdouble)mapvals.bumpmax*(gdouble)bumprow[n]/255.0;

  /* Compute triangle normals */
  /* ======================== */
  
  i=0;
  for (n=0;n<(x2-x1-1);n++)
    {
      p1.x=0.0;
      p1.y=ystep; 
      p1.z=heights[2][n]-heights[1][n];
      
      p2.x=xstep;
      p2.y=ystep;
      p2.z=heights[2][n+1]-heights[1][n];

      p3.x=xstep;
      p3.y=0.0;
      p3.z=heights[1][n+1]-heights[1][n];
      
      triangle_normals[1][i]=gck_vector3_cross_product(&p2,&p1);
      triangle_normals[1][i+1]=gck_vector3_cross_product(&p3,&p2);

      gck_vector3_normalize(&triangle_normals[1][i]);
      gck_vector3_normalize(&triangle_normals[1][i+1]);
      
      i+=2;
    }
  
  /* Compute vertex normals */
  /* ====================== */

  i=0;
  gck_vector3_set(&normal, 0.0,0.0,0.0);
  for (n=0;n<(x2-x1-1);n++)
    {
      nv=0;
      if (n>0)
        {
          if (y>0)
            {
              gck_vector3_add(&normal, &normal, &triangle_normals[0][i-1]);
              gck_vector3_add(&normal, &normal, &triangle_normals[0][i-2]);
              nv+=2;
            }
          if (y<pre_h)
            {
              gck_vector3_add(&normal, &normal, &triangle_normals[1][i-1]);
              nv++;
            }
        }
      if (n<pre_w)
        {
          if (y>0)
            {
              gck_vector3_add(&normal, &normal, &triangle_normals[0][i]);
              gck_vector3_add(&normal, &normal, &triangle_normals[0][i+1]);
              nv+=2;
            }
          if (y<pre_h)
            {
              gck_vector3_add(&normal, &normal, &triangle_normals[1][i]);
              gck_vector3_add(&normal, &normal, &triangle_normals[1][i+1]);
              nv+=2;                  
            }
        }
      
      gck_vector3_mul(&normal, 1.0/(gdouble)nv);
      gck_vector3_normalize(&normal);
      vertex_normals[1][n]=normal;

      i+=2;
    }
}

/***********************************************************************/
/* Compute the reflected ray given the normalized normal and ins. vec. */
/***********************************************************************/

GckVector3 compute_reflected_ray(GckVector3 *normal,GckVector3 *view)
{
  GckVector3 ref;
  gdouble nl;

  nl = 2.0*gck_vector3_inner_product(normal,view);
  
  ref = *normal;
  
  gck_vector3_mul(&ref,nl);
  gck_vector3_sub(&ref,&ref,view);
  
  return(ref);
}

/************************************************************************/
/* Given the NorthPole, Equator and a third vector (normal) compute     */
/* the conversion from spherical coordinates to image space coordinates */
/************************************************************************/

void sphere_to_image(GckVector3 *normal,gdouble *u,gdouble *v)
{
  static gdouble alpha,fac;
  static GckVector3 cross_prod;
  static GckVector3 firstaxis  = { 1.0, 0.0, 0.0 };
  static GckVector3 secondaxis  = { 0.0, 1.0, 0.0 };

  alpha=acos(-gck_vector3_inner_product(&secondaxis,normal));

  *v=alpha/M_PI;

  if (*v==0.0 || *v==1.0) *u=0.0;
  else
    {
      fac=gck_vector3_inner_product(&firstaxis,normal)/sin(alpha);

      /* Make sure that we map to -1.0..1.0 (take care of rounding errors) */
      /* ================================================================= */

      if (fac>1.0)
        fac=1.0;
      else if (fac<-1.0) 
        fac=-1.0;

      *u=acos(fac)/(2.0*M_PI);
	  
      cross_prod=gck_vector3_cross_product(&secondaxis,&firstaxis);
      
      if (gck_vector3_inner_product(&cross_prod,normal)<0.0)
        *u=1.0-*u;
    }
}

/*********************************************************************/
/* These routines computes the color of the surface at a given point */
/*********************************************************************/

GckRGB get_ray_color(GckVector3 *position)
{
  GckRGB color;
  gint x,f;
  gdouble xf,yf;
  GckVector3 normal,*p;

  pos_to_float(position->x,position->y,&xf,&yf);

  x = (gint)(xf+0.5);

  if (mapvals.transparent_background && heights[1][x]==0)
    color.a=0.0;
  else
    {
      color=get_image_color(xf,yf,&f);

      if (mapvals.lightsource.type==POINT_LIGHT)
        p=&mapvals.lightsource.position;
      else
        p=&mapvals.lightsource.direction;

      if (mapvals.bump_mapped==FALSE || mapvals.bumpmap_id==-1)
        color=phong_shade(position,
                         &mapvals.viewpoint,
                         &mapvals.planenormal,
                         p,
                         &color,
                         &mapvals.lightsource.color,
                          mapvals.lightsource.type);
      else
        {
          normal=vertex_normals[1][(gint)(xf+0.5)];
    
          color=phong_shade(position,
                           &mapvals.viewpoint,
                           &normal,
                           p,
                           &color,
                           &mapvals.lightsource.color,
                            mapvals.lightsource.type);
        }
    }

  return(color);
}

GckRGB get_ray_color_ref(GckVector3 *position)
{
  GckRGB color,env_color;
  gint x,f;
  gdouble xf,yf;
  GckVector3 normal,*p,v,r;

  pos_to_float(position->x,position->y,&xf,&yf);

  x = (gint)(xf+0.5);

  if (mapvals.transparent_background && heights[1][x]==0)
    color.a=0.0;
  else
    {
      color=get_image_color(xf,yf,&f);

      if (mapvals.lightsource.type==POINT_LIGHT)
        p=&mapvals.lightsource.position;
      else
        p=&mapvals.lightsource.direction;
    
      if (mapvals.bump_mapped==FALSE || mapvals.bumpmap_id==-1)
        color=phong_shade(position,
                         &mapvals.viewpoint,
                         &mapvals.planenormal,
                         p,
                         &color,
                         &mapvals.lightsource.color,
                          mapvals.lightsource.type);
      else
        {
          normal=vertex_normals[1][(gint)(xf+0.5)];
    
          gck_vector3_sub(&v,&mapvals.viewpoint,position);
          gck_vector3_normalize(&v);
    
          r = compute_reflected_ray(&normal,&v);
    
          /* Get color in the direction of r */
          /* =============================== */
    
          sphere_to_image(&r,&xf,&yf);
          env_color = peek_env_map((gint)(env_width*xf+0.5),(gint)(env_height*yf+0.5));
    
          color=phong_shade(position,
                           &mapvals.viewpoint,
                           &normal,
                           p,
                           &env_color,
                           &mapvals.lightsource.color,
                            mapvals.lightsource.type);
        }
    }

  return(color);
}

GckRGB get_ray_color_no_bilinear(GckVector3 *position)
{
  GckRGB color;
  gint x;
  gdouble xf,yf;
  GckVector3 normal,*p;

  pos_to_float(position->x,position->y,&xf,&yf);

  x = (gint)(xf+0.5);
  
  if (mapvals.transparent_background && heights[1][x]==0)
    color.a=0.0;
  else
    {
      color=peek(x,(gint)(yf+0.5));
    
      if (mapvals.lightsource.type==POINT_LIGHT)
        p=&mapvals.lightsource.position;
      else
        p=&mapvals.lightsource.direction;
    
      if (mapvals.bump_mapped==FALSE || mapvals.bumpmap_id==-1)
        color=phong_shade(position,
                         &mapvals.viewpoint,
                         &mapvals.planenormal,
                         p,
                         &color,
                         &mapvals.lightsource.color,
                          mapvals.lightsource.type);
      else
        {
          normal=vertex_normals[1][x];
    
          color=phong_shade(position,
                           &mapvals.viewpoint,
                           &normal,
                           p,
                           &color,
                           &mapvals.lightsource.color,
                            mapvals.lightsource.type);
        }
    }

  return(color);
}

GckRGB get_ray_color_no_bilinear_ref(GckVector3 *position)
{
  GckRGB color,env_color;
  gint x;
  gdouble xf,yf;
  GckVector3 normal,*p,v,r;

  pos_to_float(position->x,position->y,&xf,&yf);

  x = (gint)(xf+0.5);
  
  if (mapvals.transparent_background && heights[1][x]==0)
    color.a=0.0;
  else
    {
      color=peek((gint)(xf+0.5),(gint)(yf+0.5));

      if (mapvals.lightsource.type==POINT_LIGHT)
        p=&mapvals.lightsource.position;
      else
        p=&mapvals.lightsource.direction;
    
      if (mapvals.bump_mapped==FALSE || mapvals.bumpmap_id==-1)
        {
          pos_to_float(position->x,position->y,&xf,&yf);
    
          color=peek((gint)(xf+0.5),(gint)(yf+0.5));
    
          gck_vector3_sub(&v,&mapvals.viewpoint,position);
          gck_vector3_normalize(&v);
    
          r = compute_reflected_ray(&mapvals.planenormal,&v);
    
          /* Get color in the direction of r */
          /* =============================== */
    
          sphere_to_image(&r,&xf,&yf);
          env_color = peek_env_map((gint)(env_width*xf+0.5),(gint)(env_height*yf+0.5));
    
          color=phong_shade(position,
                           &mapvals.viewpoint,
                           &mapvals.planenormal,
                           p,
                           &env_color,
                           &mapvals.lightsource.color,
                           mapvals.lightsource.type);
        }
      else
        {
          normal=vertex_normals[1][(gint)(xf+0.5)];
    
          pos_to_float(position->x,position->y,&xf,&yf);
          color=peek((gint)(xf+0.5),(gint)(yf+0.5));
    
          gck_vector3_sub(&v,&mapvals.viewpoint,position);
          gck_vector3_normalize(&v);
    
          r = compute_reflected_ray(&normal,&v);
    
          /* Get color in the direction of r */
          /* =============================== */
    
          sphere_to_image(&r,&xf,&yf);
          env_color = peek_env_map((gint)(env_width*xf+0.5),(gint)(env_height*yf+0.5));
    
          color=phong_shade(position,
                           &mapvals.viewpoint,
                           &normal,
                           p,
                           &env_color,
                           &mapvals.lightsource.color,
                            mapvals.lightsource.type);
        }
    }

  return(color);
}

