/*****************/
/* Shading stuff */
/*****************/

#include "mapobject_shade.h"

gdouble bx1,by1,bx2,by2;
get_ray_color_func get_ray_color;

/*****************/
/* Phong shading */
/*****************/

GckRGB phong_shade(GckVector3 *pos,GckVector3 *viewpoint,GckVector3 *normal,GckVector3 *light,
                   GckRGB *diff_col,GckRGB *spec_col,gint type)
{
  GckRGB ambientcolor,diffusecolor,specularcolor;
  gdouble NL,RV,dist;
  GckVector3 L,NN,V,N;

  /* Compute ambient intensity */
  /* ========================= */

  N=*normal;
  ambientcolor=*diff_col;
  gck_rgb_mul(&ambientcolor,mapvals.material.ambient_int);

  /* Compute (N*L) term of Phong's equation */
  /* ====================================== */

  if (type==POINT_LIGHT)
    gck_vector3_sub(&L,light,pos);
  else
    L=*light;

  dist=gck_vector3_length(&L);

  if (dist!=0.0)
    gck_vector3_mul(&L,1.0/dist);

  NL=2.0*gck_vector3_inner_product(&N,&L);

  if (NL>=0.0)
    {
      /* Compute (R*V)^alpha term of Phong's equation */
      /* ============================================ */

      gck_vector3_sub(&V,viewpoint,pos);
      gck_vector3_normalize(&V);

      gck_vector3_mul(&N,NL);
      gck_vector3_sub(&NN,&N,&L);
      RV=gck_vector3_inner_product(&NN,&V);
      RV=pow(RV,mapvals.material.highlight);

      /* Compute diffuse and specular intensity contribution */
      /* =================================================== */

      diffusecolor=*diff_col;
      gck_rgb_mul(&diffusecolor,mapvals.material.diffuse_ref);
      gck_rgb_mul(&diffusecolor,NL);

      specularcolor=*spec_col;
      gck_rgb_mul(&specularcolor,mapvals.material.specular_ref);
      gck_rgb_mul(&specularcolor,RV);

      gck_rgb_add(&diffusecolor,&specularcolor);
      gck_rgb_mul(&diffusecolor,mapvals.material.diffuse_int);
      gck_rgb_clamp(&diffusecolor);

      gck_rgb_add(&ambientcolor,&diffusecolor);
    }

  return(ambientcolor);
}

gint plane_intersect(GckVector3 *dir,GckVector3 *viewp,GckVector3 *ipos,gdouble *u,gdouble *v)
{
  static gdouble det,det1,det2,det3,t;
    
  imat[0][0]=dir->x; imat[1][0]=dir->y; imat[2][0]=dir->z;

  /* Compute determinant of the first 3x3 sub matrix (denominator) */
  /* ============================================================= */

  det=imat[0][0]*imat[1][1]*imat[2][2]+imat[0][1]*imat[1][2]*imat[2][0]+
      imat[0][2]*imat[1][0]*imat[2][1]-imat[0][2]*imat[1][1]*imat[2][0]-
      imat[0][0]*imat[1][2]*imat[2][1]-imat[2][2]*imat[0][1]*imat[1][0];

  /* If the determinant is non-zero, a intersection point exists */
  /* =========================================================== */

  if (det!=0.0)
    {
      /* Now, lets compute the numerator determinants (wow ;) */
      /* ==================================================== */
    
      det1=imat[0][3]*imat[1][1]*imat[2][2]+imat[0][1]*imat[1][2]*imat[2][3]+
           imat[0][2]*imat[1][3]*imat[2][1]-imat[0][2]*imat[1][1]*imat[2][3]-
           imat[1][2]*imat[2][1]*imat[0][3]-imat[2][2]*imat[0][1]*imat[1][3];
 
      det2=imat[0][0]*imat[1][3]*imat[2][2]+imat[0][3]*imat[1][2]*imat[2][0]+
           imat[0][2]*imat[1][0]*imat[2][3]-imat[0][2]*imat[1][3]*imat[2][0]-
           imat[1][2]*imat[2][3]*imat[0][0]-imat[2][2]*imat[0][3]*imat[1][0];
 
      det3=imat[0][0]*imat[1][1]*imat[2][3]+imat[0][1]*imat[1][3]*imat[2][0]+
           imat[0][3]*imat[1][0]*imat[2][1]-imat[0][3]*imat[1][1]*imat[2][0]-
           imat[1][3]*imat[2][1]*imat[0][0]-imat[2][3]*imat[0][1]*imat[1][0];

      /* Now we have the simultanous solutions. Lets compute the unknowns */
      /* (skip u&v if t is <0, this means the intersection is behind us)  */
      /* ================================================================ */

      t=det1/det;

      if (t>0.0)
        {
          *u=1.0+((det2/det)-0.5);
          *v=1.0+((det3/det)-0.5);

      	  ipos->x=viewp->x+t*dir->x;
          ipos->y=viewp->y+t*dir->y;
          ipos->z=viewp->z+t*dir->z;
          
          return(TRUE);
        }
    }

  return(FALSE);
}

/**********************************************************************************/
/* These routines computes the color of the surface of the plane at a given point */
/**********************************************************************************/

GckRGB get_ray_color_plane(GckVector3 *pos)
{
  GckRGB color=background;
  static gint inside=FALSE;
  static GckVector3 ray,spos;
  static gdouble vx,vy;

  /* Construct a line from our VP to the point */
  /* ========================================= */

  gck_vector3_sub(&ray,pos,&mapvals.viewpoint);
  gck_vector3_normalize(&ray);

  /* Check for intersection. This is a quasi ray-tracer. */
  /* =================================================== */

  if (plane_intersect(&ray,&mapvals.viewpoint,&spos,&vx,&vy)==TRUE)
    {
      color=get_image_color(vx,vy,&inside);

      if (color.a!=0.0 && inside==TRUE && mapvals.lightsource.type!=NO_LIGHT)
        {
          /* Compute shading at this point */
          /* ============================= */

          color=phong_shade(&spos,&mapvals.viewpoint,&mapvals.normal,
            &mapvals.lightsource.position,&color,
            &mapvals.lightsource.color,mapvals.lightsource.type);

          gck_rgb_clamp(&color);
        }
    }

  
  if (color.a==0.0)
    color=background;
  
  return(color);
}

/***********************************************************************/
/* Given the NorthPole, Equator and a third vector (normal) compute    */
/* the conversion from spherical oordinates to image space coordinates */
/***********************************************************************/

void sphere_to_image(GckVector3 *normal,gdouble *u,gdouble *v)
{
  static gdouble alpha,fac;
  static GckVector3 cross_prod;

  alpha=acos(-gck_vector3_inner_product(&mapvals.secondaxis,normal));

  *v=alpha/M_PI;

  if (*v==0.0 || *v==1.0) *u=0.0;
  else
    {
      fac=gck_vector3_inner_product(&mapvals.firstaxis,normal)/sin(alpha);

      /* Make sure that we map to -1.0..1.0 (take care of rounding errors) */
      /* ================================================================= */

      if (fac>1.0)
        fac=1.0;
      else if (fac<-1.0) 
        fac=-1.0;

      *u=acos(fac)/(2.0*M_PI);
	  
      cross_prod=gck_vector3_cross_product(&mapvals.secondaxis,&mapvals.firstaxis);
      
      if (gck_vector3_inner_product(&cross_prod,normal)<0.0)
        *u=1.0-*u;
    }
}

/***************************************************/
/* Compute intersection point with sphere (if any) */
/***************************************************/

gint sphere_intersect(GckVector3 *dir,GckVector3 *viewp,GckVector3 *spos1,GckVector3 *spos2)
{
  static gdouble alpha,beta,tau,s1,s2,tmp;
  static GckVector3 t;

  gck_vector3_sub(&t,&mapvals.position,viewp);

  alpha=gck_vector3_inner_product(dir,&t);
  beta=gck_vector3_inner_product(&t,&t);

  tau=alpha*alpha-beta+mapvals.radius*mapvals.radius;

  if (tau>=0.0)
    {
      tau=sqrt(tau);
      s1=alpha+tau;
      s2=alpha-tau;

      if (s2<s1)
        {
          tmp=s1;
          s1=s2;
          s2=tmp;
        }

      spos1->x=viewp->x+s1*dir->x;
      spos1->y=viewp->y+s1*dir->y;
      spos1->z=viewp->z+s1*dir->z;
      spos2->x=viewp->x+s2*dir->x;
      spos2->y=viewp->y+s2*dir->y;
      spos2->z=viewp->z+s2*dir->z;
      
      return(TRUE);
    }

  return(FALSE);
}

/***********************************************************************************/
/* These routines computes the color of the surface of the sphere at a given point */
/***********************************************************************************/

GckRGB get_ray_color_sphere(GckVector3 *pos)
{
  GckRGB color=background;
  static GckRGB color2;
  static gint inside=FALSE;
  static GckVector3 normal,ray,spos1,spos2;
  static gdouble vx,vy;

  /* Check if ray is within the bounding box */
  /* ======================================= */
  
  if (pos->x<bx1 || pos->x>bx2 || pos->y<by1 || pos->y>by2)
    return(color);

  /* Construct a line from our VP to the point */
  /* ========================================= */

  gck_vector3_sub(&ray,pos,&mapvals.viewpoint);
  gck_vector3_normalize(&ray);

  /* Check for intersection. This is a quasi ray-tracer. */
  /* =================================================== */

  if (sphere_intersect(&ray,&mapvals.viewpoint,&spos1,&spos2)==TRUE)
    {
      /* Compute spherical to rectangular mapping */
      /* ======================================== */
    
      gck_vector3_sub(&normal,&spos1,&mapvals.position);
      gck_vector3_normalize(&normal);
      sphere_to_image(&normal,&vx,&vy);
      color=get_image_color(vx,vy,&inside);

      /* Check for total transparency... */
      /* =============================== */

      if (color.a<1.0)
        {
          /* Hey, we can see  through here!      */
          /* Lets see what's on the other side.. */
          /* =================================== */
          
          color=phong_shade(&spos1,
            &mapvals.viewpoint,
            &normal,
            &mapvals.lightsource.position,
            &color,
            &mapvals.lightsource.color,
            mapvals.lightsource.type);

          gck_rgba_clamp(&color);

          gck_vector3_sub(&normal,&spos2,&mapvals.position);
          gck_vector3_normalize(&normal);
          sphere_to_image(&normal,&vx,&vy); 
          color2=get_image_color(vx,vy,&inside);

          /* Make the normal point inwards */
          /* ============================= */

          gck_vector3_mul(&normal,-1.0);

          color2=phong_shade(&spos2,
            &mapvals.viewpoint,
            &normal,
            &mapvals.lightsource.position,
            &color2,
            &mapvals.lightsource.color,
            mapvals.lightsource.type);

          gck_rgba_clamp(&color2);
          
          if (mapvals.transparent_background==FALSE && color2.a<1.0)
            {
              color2.r = (color2.r*color2.a)+(background.r*(1.0-color2.a));
              color2.g = (color2.g*color2.a)+(background.g*(1.0-color2.a));
              color2.b = (color2.b*color2.a)+(background.b*(1.0-color2.a));
              color2.a = 1.0;
            }

          /* Compute a mix of the first and second colors */
          /* ============================================ */
          
          color.r = color.r*color2.r;
          color.g = color.g*color2.g;
          color.b = color.b*color2.b;
          color.a = color.a+color2.a;

          gck_rgba_clamp(&color);
        }
      else if (color.a!=0.0 && inside==TRUE && mapvals.lightsource.type!=NO_LIGHT)
        {
          /* Compute shading at this point */
          /* ============================= */

          color=phong_shade(&spos1,
            &mapvals.viewpoint,
            &normal,
            &mapvals.lightsource.position,
            &color,
            &mapvals.lightsource.color,
            mapvals.lightsource.type);

          gck_rgba_clamp(&color);
        }
    }
  
  if (color.a==0.0)
    color=background;
  
  return(color);
}

/***************************************************/
/* Transform the corners of the bounding box to 2D */
/***************************************************/

void compute_bounding_box(void)
{
  GckVector3 p1,p2;
  gdouble t;
  GckVector3 dir;

  p1=mapvals.position;
  p1.x-=(mapvals.radius+0.01);
  p1.y-=(mapvals.radius+0.01);

  p2=mapvals.position;
  p2.x+=(mapvals.radius+0.01);
  p2.y+=(mapvals.radius+0.01);

  gck_vector3_sub(&dir,&p1,&mapvals.viewpoint);
  gck_vector3_normalize(&dir);

  if (dir.z!=0.0)
    {
      t=(-1.0*mapvals.viewpoint.z)/dir.z;
      p1.x=(mapvals.viewpoint.x+t*dir.x);
      p1.y=(mapvals.viewpoint.y+t*dir.y);
    }

  gck_vector3_sub(&dir,&p2,&mapvals.viewpoint);
  gck_vector3_normalize(&dir);

  if (dir.z!=0.0)
    {
      t=(-1.0*mapvals.viewpoint.z)/dir.z;
      p2.x=(mapvals.viewpoint.x+t*dir.x);
      p2.y=(mapvals.viewpoint.y+t*dir.y);
    }

  bx1=p1.x;
  by1=p1.y;
  bx2=p2.x;
  by2=p2.y;
}
