/*********************************************************************************/
/* LIC 0.14 -- image filter plug-in for The Gimp program                         */
/* Copyright (C) 1996 Tom Bech                                                   */
/*===============================================================================*/
/* E-mail: tomb@gimp.org                                                         */
/* You can contact the original The Gimp authors at gimp@xcf.berkeley.edu        */
/*===============================================================================*/
/* This program is free software; you can redistribute it and/or modify it under */
/* the terms of the GNU General Public License as published by the Free Software */
/* Foundation; either version 2 of the License, or (at your option) any later    */
/* version.                                                                      */
/*===============================================================================*/
/* This program is distributed in the hope that it will be useful, but WITHOUT   */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS */
/* FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.*/
/*===============================================================================*/
/* You should have received a copy of the GNU General Public License along with  */
/* this program; if not, write to the Free Software Foundation, Inc., 675 Mass   */
/* Ave, Cambridge, MA 02139, USA.                                                */
/*===============================================================================*/
/* In other words, you can't sue me for whatever happens while using this ;)     */
/*********************************************************************************/
/* Changes (post 0.10):                                                          */
/* -> 0.11: Fixed a bug in the convolution kernels (Tom).                        */
/* -> 0.12: Added Quartic's bilinear interpolation stuff (Tom).                  */
/* -> 0.13 Changed some UI stuff causing trouble with the 0.60 release, added    */
/*         the (GIMP) tags and changed random() calls to rand() (Tom)            */
/* -> 0.14 Ported to 0.99.11 (Tom)                                               */
/*********************************************************************************/
/* This plug-in implements the Line Integral Convolution (LIC) as described in   */
/* Cabral et al. "Imaging vector fields using line integral convolution" in the  */
/* Proceedings of ACM SIGGRAPH 93. Publ. by ACM, New York, NY, USA. p. 263-270.  */
/* Some of the code is based on code by Steinar Haugen (thanks!), the Perlin     */
/* noise function is practically ripped as is :)                                 */
/*********************************************************************************/
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "config.h"
#include "libgimp/stdplugins-intl.h"
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>


/************/
/* Typedefs */
/************/

#define CHECKBOUNDS(x,y) (x>=0 && y>=0 && x<width && y<height)

#define EPSILON 1.0e-5

#define numx    40              /* Pseudo-random vector grid size */
#define numy    40

#define stepx   0.5
#define stepy   0.5

#define TILE_CACHE_SIZE 16

/***********************/
/* Some useful structs */
/***********************/

typedef struct
{
  gdouble r,g,b,a;
} rgbpixel;

/*****************************/
/* Global variables and such */
/*****************************/

rgbpixel white = {1.0,1.0,1.0};
rgbpixel black = {0.0,0.0,0.0};
rgbpixel background;

gdouble G[numx][numy][2];

typedef struct {
  glong filtlen,noisemag,intsteps,minv,maxv;
  gboolean create_new_image;
  guchar effect_channel;
  guchar effect_operator;
  guchar effect_convolve;
  gint32 effect_image_id;
} LicValues;

LicValues licvals;

gdouble l=10.0,dx=2.0,dy=2.0,minv=-2.5,maxv=2.5,isteps=20.0;

GDrawable *input_drawable,*output_drawable;
GPixelRgn source_region,dest_region;

gint imgtype,width,height,in_channels,out_channels;
gint border_x1,border_y1,border_x2,border_y2;
glong maxcounter;
guchar *scalarfield;

GtkWidget  *dialog;

/************************/
/* Convenience routines */
/************************/

void rgb_add(rgbpixel *a,rgbpixel *b)
{
  a->r=a->r+b->r;
  a->g=a->g+b->g;
  a->b=a->b+b->b;
}

void rgb_mul(rgbpixel *a,gdouble b)
{
  a->r=a->r*b;
  a->g=a->g*b;
  a->b=a->b*b;
}

void rgb_clamp(rgbpixel *a)
{
  if (a->r>1.0)
    a->r=1.0;
  
  if (a->g>1.0)
    a->g=1.0;
  
  if (a->b>1.0)
    a->b=1.0;
  
  if (a->r<0.0)
    a->r=0.0;
  
  if (a->g<0.0)
    a->g=0.0;
  
  if (a->b<0.0)
    a->b=0.0;
}

void set_color(rgbpixel *a,gdouble r,gdouble g,gdouble b)
{
  a->r=r; a->g=g; a->b=b;
}

glong xy_to_index(gint x,gint y)
{
  return((glong)in_channels*((glong)x+(glong)y*(glong)width));
}

gboolean checkbounds(gint x,gint y)
{
  if (x<0 || y<0 || x>width-1 || y>height-1)
    return(FALSE);
  
  return(TRUE);
}

rgbpixel peek(gint x,gint y)
{
  static guchar data[4];
  rgbpixel color;

  gimp_pixel_rgn_get_pixel(&source_region,data,x,y);

  color.r=(gdouble)(data[0])/255.0;
  color.g=(gdouble)(data[1])/255.0;
  color.b=(gdouble)(data[2])/255.0;

  if (input_drawable->bpp==4)
    {
      if (in_channels==4)
        color.a=(gdouble)(data[3])/255.0;
      else
        color.a=1.0;
    }
  else
    color.a=1.0;

  return(color);
}

void poke(gint x,gint y,rgbpixel *color)
{
  static guchar data[4];
  
  data[0]=(guchar)(color->r*255.0);
  data[1]=(guchar)(color->g*255.0);
  data[2]=(guchar)(color->b*255.0);
  data[3]=(guchar)(color->a*255.0);
  
  gimp_pixel_rgn_set_pixel(&dest_region,data,x,y);
}

void pos_to_int(gdouble x,gdouble y,gint *scr_x,gint *scr_y)
{
  *scr_x=(gint)((x*(gdouble)width));
  *scr_y=(gint)((y*(gdouble)height));
}

/****************************************/
/* Allocate memory for temporary images */
/****************************************/

gint image_setup(GDrawable *drawable,gint interactive)
{
  /* Get some useful info on the input drawable */
  /* ========================================== */

  input_drawable=drawable;
  output_drawable=drawable;

  gimp_drawable_mask_bounds (drawable->id, &border_x1, &border_y1, &border_x2, &border_y2);

  width=input_drawable->width;
  height=input_drawable->height;

  gimp_pixel_rgn_init (&source_region, input_drawable,  0, 0, width, height, FALSE, FALSE);

  maxcounter=(glong)width*(glong)height;

  /* Assume at least RGB */
  /* =================== */

  in_channels=3;
  if (gimp_drawable_has_alpha(input_drawable->id)==TRUE)
    in_channels++;

  if (interactive==TRUE)
    {
      /* Allocate memory for temp. images */
      /* ================================ */
    }

  return(TRUE);
}

guchar peekmap(guchar *image,gint x,gint y)
{
  glong index;

  index=(glong)x+(glong)width*(glong)y;

  return(image[index]);
}

/*************/
/* Main part */
/*************/

/***************************************************/
/* Compute the derivative in the x and y direction */
/* We use these convolution kernels:               */
/*     |1 0 -1|     |  1   2   1|                  */
/* DX: |2 0 -2| DY: |  0   0   0|                  */
/*     |1 0 -1|     | -1  -2  -1|                  */
/* (It's a varation of the Sobel kernels, really)  */
/***************************************************/

gint gradx(guchar *image,gint x,gint y)
{
  gint val=0;

  if (CHECKBOUNDS(x-1,y-1)==TRUE)
    val=val+(gint)peekmap(image,x-1,y-1);
  if (CHECKBOUNDS(x+1,y-1)==TRUE)
    val=val-(gint)peekmap(image,x+1,y-1);

  if (CHECKBOUNDS(x-1,y)==TRUE)
    val=val+2*(gint)peekmap(image,x-1,y);
  if (CHECKBOUNDS(x+1,y)==TRUE)
    val=val-2*(gint)peekmap(image,x+1,y);

  if (CHECKBOUNDS(x-1,y+1)==TRUE)
    val=val+(gint)peekmap(image,x-1,y+1);
  if (CHECKBOUNDS(x+1,y+1)==TRUE)
    val=val-(gint)peekmap(image,x+1,y+1);

  return(val);
}

gint grady(guchar *image,gint x,gint y)
{
  gint val=0;

  if (CHECKBOUNDS(x-1,y-1)==TRUE)
    val=val+(gint)peekmap(image,x-1,y-1);
  if (CHECKBOUNDS(x,y-1)==TRUE)
    val=val+2*(gint)peekmap(image,x,y-1);
  if (CHECKBOUNDS(x+1,y-1)==TRUE)
    val=val+(gint)peekmap(image,x+1,y-1);

  if (CHECKBOUNDS(x-1,y+1)==TRUE)
    val=val-(gint)peekmap(image,x-1,y+1);
  if (CHECKBOUNDS(x,y+1)==TRUE)
    val=val-2*(gint)peekmap(image,x,y+1);
  if (CHECKBOUNDS(x+1,y+1)==TRUE)
    val=val-(gint)peekmap(image,x+1,y+1);

  return(val);
}

/************************************/
/* A nice 2nd order cubic spline :) */
/************************************/

gdouble cubic(gdouble t)
{
  gdouble at=fabs(t);
  
  if (at<1.0)
    return(2.0*at*at*at-3.0*at*at+1.0);
  
  return(0.0);
}

gdouble omega(gdouble u,gdouble v,gint i,gint j)
{
  while (i<0)
    i+=numx;
  
  while (j<0)
    j+=numy;
  
  i%=numx;
  j%=numy;
  
  return(cubic(u)*cubic(v)*(G[i][j][0]*u+G[i][j][1]*v));
}

/*************************************************************/
/* The noise function (2D variant of Perlins noise function) */
/*************************************************************/

gdouble noise(gdouble x,gdouble y)
{
  gint i,sti=(gint)floor(x/dx);
  gint j,stj=(gint)floor(y/dy);

  gdouble sum=0.0;

  /* Calculate the gdouble sum */
  /* ======================== */

  for (i=sti; i<=sti+1; i++)
    {
      for (j=stj; j<=stj+1; j++)
        sum+=omega((x-(gdouble)i*dx)/dx,(y-(gdouble)j*dy)/dy,i,j);
    }

  return(sum);
}

/*************************************************/
/* Generates pseudo-random vectors with length 1 */
/*************************************************/

void generatevectors(void)
{
  gdouble alpha;
  gint i,j;

  for (i=0; i<numx; i++)
    {
      for (j=0; j<numy; j++)
        {
          alpha = (gdouble)(rand()%1000)*(G_PI/500.0);
          G[i][j][0] = cos(alpha);
          G[i][j][1] = sin(alpha);
        }
    }
}

/* A simple triangle filter */
/* ======================== */

gdouble filter(gdouble u)
{
  gdouble f=1.0-fabs(u)/l;
  
  if (f<0.0)
    f=0.0;
  
  return(f);
}

/******************************************************/
/* Compute the Line Integral Convolution (LIC) at x,y */
/******************************************************/

gdouble lic_noise(gint x,gint y,gdouble vx,gdouble vy)
{
  gdouble i=0.0;
  gdouble f1=0.0,f2=0.0;
  gdouble u,step=2.0*l/isteps;
  gdouble xx=(gdouble)x,yy=(gdouble)y;
  gdouble c,s;
  
  /* Get vector at x,y */
  /* ================= */

  c = vx;
  s = vy;
  
  /* Calculate integral numerically */
  /* ============================== */

  f1 = filter(-l)*noise(xx+l*c,yy+l*s);

  for (u=-l+step; u<=l; u+=step)
    {
      f2 = filter(u)*noise(xx-u*c,yy-u*s);
      i+=(f1+f2)*0.5*step;
      f1=f2;
    }

  i=(i-minv)/(maxv-minv);

  if (i<0.0)
    i=0.0;
  
  if (i>1.0)
    i=1.0;

  i=(i/2.0)+0.5;

  return(i);
}

static rgbpixel bilinear(gdouble x, gdouble y, rgbpixel *p)
{
  gdouble   m0, m1;
  gdouble   ix, iy;
  rgbpixel v;

  x = fmod(x, 1.0);
  y = fmod(y, 1.0);

  if (x < 0)
     x += 1.0;

  if (y < 0)
    y += 1.0;

  ix = 1.0 - x;
  iy = 1.0 - y;

  /* Red */
  /* === */

  m0 = ix * p[0].r + x * p[1].r;
  m1 = ix * p[2].r + x * p[3].r;

  v.r = iy * m0 + y * m1;

  /* Green */
  /* ===== */

  m0 = ix * p[0].g + x * p[1].g;
  m1 = ix * p[2].g + x * p[3].g;

  v.g = iy * m0 + y * m1;

  /* Blue */
  /* ==== */

  m0 = ix * p[0].b + x * p[1].b;
  m1 = ix * p[2].b + x * p[3].b;

  v.b = iy * m0 + y * m1;

  return(v);
} /* bilinear */

void getpixel(rgbpixel *p,gdouble u,gdouble v)
{
  register gint x1, y1, x2, y2;
  static rgbpixel pp[4];
 
  x1 = (gint)u;
  y1 = (gint)v;

  if (x1 < 0)
    x1 = width - (-x1 % width);
  else
    x1 = x1 % width;
  
  if (y1 < 0)
    y1 = height - (-y1 % height);
  else
    y1 = y1 % height;

  x2 = (x1 + 1) % width;
  y2 = (y1 + 1) % height;
 
  pp[0] = peek(x1, y1);
  pp[1] = peek(x2, y1);
  pp[2] = peek(x1, y2);
  pp[3] = peek(x2, y2);

  *p=bilinear(u,v,pp);
}

void lic_image(gint x,gint y,gdouble vx,gdouble vy,rgbpixel *color)
{
  gdouble u,step=2.0*l/isteps;
  gdouble xx=(gdouble)x,yy=(gdouble)y;
  gdouble c,s;
  rgbpixel col,col1,col2,col3;

  /* Get vector at x,y */
  /* ================= */

  c = vx;
  s = vy;
  
  /* Calculate integral numerically */
  /* ============================== */

  col=black;
  getpixel(&col1,xx+l*c,yy+l*s);
  rgb_mul(&col1,filter(-l));

  for (u=-l+step; u<=l; u+=step)
    {
      getpixel(&col2,xx-u*c,yy-u*s);
      rgb_mul(&col2,filter(u));

      col3=col1;
      rgb_add(&col3,&col2);
      rgb_mul(&col3,0.5*step);
      rgb_add(&col,&col3);

      col1=col2;
    }

  rgb_mul(&col,1.0/l);
  rgb_clamp(&col);

  *color=col;
}

gdouble maximum(gdouble a,gdouble b,gdouble c)
{
  gdouble max=a;
  
  if (b>max)
    max=b;
  if (c>max)
    max=c;
  
  return(max);
}

gdouble minimum(gdouble a,gdouble b,gdouble c)
{ 
  gdouble min=a;
  
  if (b<min)
    min=b;
  if (c<min)
    min=c;
  
  return(min);
}

void get_hue(rgbpixel *col,gdouble *hue)
{
  gdouble max,min,delta;

  max=maximum(col->r,col->g,col->b);
  min=minimum(col->r,col->g,col->b);

  if (max==min) *hue=-1.0;
  else
    {
      delta=max-min;
      if (col->r==max)
        *hue=(col->g-col->b)/delta;
      else if (col->g==max)
        *hue=2.0+(col->b-col->r)/delta;
      else if (col->b==max)
        *hue=4.0+(col->r-col->g)/delta;
      
      *hue=*hue*60.0;
      if (*hue<0.0)
        *hue=*hue+360.0;
    }
}

void get_saturation(rgbpixel *col,gdouble *sat)
{
  gdouble max,min,l;

  max=maximum(col->r,col->g,col->b);
  min=minimum(col->r,col->g,col->b);

  if (max==min)
    *sat=0.0;
  else
    {
      l=(max+min)/2.0;
      if (l<=0.5)
        *sat=(max-min)/(max+min);
      else
        *sat=(max-min)/(2.0-max-min);
    }
}

void get_brightness(rgbpixel *col,gdouble *bri)
{
  gdouble max,min;

  max=maximum(col->r,col->g,col->b);
  min=minimum(col->r,col->g,col->b);

  *bri=(max+min)/2.0;
}

void rgb_to_hue(GDrawable *image,guchar **map)
{
  guchar *themap,data[4];
  gint w,h,x,y;
  rgbpixel color;
  gdouble val;
  glong maxc,index=0;
  GPixelRgn region;

  w=image->width;
  h=image->height;
  maxc=(glong)w*(glong)h;

/*  gimp_drawable_mask_bounds (drawable->id, &border_x1, &border_y1, &border_x2, &border_y2); */

  gimp_pixel_rgn_init (&region, image,  0, 0, w, h, FALSE, FALSE);

  themap=(guchar *)malloc((size_t)maxc*sizeof(guchar));

  for (y=0;y<h;y++)
    {
      for (x=0;x<w;x++)
        {
          gimp_pixel_rgn_get_pixel(&region,data,x,y);
          
          color.r=data[0];
          color.g=data[1];
          color.b=data[2];
          
          get_hue(&color,&val);
         
          themap[index++]=(guchar)(val*255.0);
        }
    }
  
  *map=themap;
}

void rgb_to_saturation(GDrawable *image,guchar **map)
{
  guchar *themap,data[4];
  gint w,h,x,y;
  rgbpixel color;
  gdouble val;
  glong maxc,index=0;
  GPixelRgn region;

  w=image->width;
  h=image->height;
  maxc=(glong)w*(glong)h;

/*  gimp_drawable_mask_bounds (drawable->id, &border_x1, &border_y1, &border_x2, &border_y2); */

  gimp_pixel_rgn_init (&region, image,  0, 0, w, h, FALSE, FALSE);

  themap=(guchar *)malloc((size_t)maxc*sizeof(guchar));

  for (y=0;y<h;y++)
    {
      for (x=0;x<w;x++)
        {
          gimp_pixel_rgn_get_pixel(&region,data,x,y);
          
          color.r=data[0];
          color.g=data[1];
          color.b=data[2];
          
          get_saturation(&color,&val);
         
          themap[index++]=(guchar)(val*255.0);
        }
    }
  
  *map=themap;
}

void rgb_to_brightness(GDrawable *image,guchar **map)
{
  guchar *themap,data[4];
  gint w,h,x,y;
  rgbpixel color;
  gdouble val;
  glong maxc,index=0;
  GPixelRgn region;

  w=image->width;
  h=image->height;
  maxc=(glong)w*(glong)h;

/*  gimp_drawable_mask_bounds (drawable->id, &border_x1, &border_y1, &border_x2, &border_y2); */

  gimp_pixel_rgn_init (&region, image,  0, 0, w, h, FALSE, FALSE);

  themap=(guchar *)malloc((size_t)maxc*sizeof(guchar));

  for (y=0;y<h;y++)
    {
      for (x=0;x<w;x++)
        {
          gimp_pixel_rgn_get_pixel(&region,data,x,y);
          
          color.r=data[0];
          color.g=data[1];
          color.b=data[2];
          
          get_brightness(&color,&val);
         
          themap[index++]=(guchar)(val*255.0);
        }
    }
  
  *map=themap;
}

void compute_lic_derivative(void)
{
  gint xcount,ycount;
  glong counter=0;
  rgbpixel color;
  gdouble vx,vy,tmp;
    
  for (ycount=0;ycount<height;ycount++)
    {
      for (xcount=0;xcount<width;xcount++)
        {
          /* Get direction vector at (x,y) and normalize it */
          /* ============================================== */
			 
          vx=gradx(scalarfield,xcount,ycount);
          vy=grady(scalarfield,xcount,ycount);

          tmp=sqrt(vx*vx+vy*vy);
          if (tmp!=0.0)
            {
              tmp=1.0/tmp;
              vx*=tmp; vy*=tmp;
            }

          /* Convolve with the LIC at (x,y) */
          /* ============================== */

          if (licvals.effect_convolve==0)
            {
              color=peek(xcount,ycount);
              tmp=lic_noise(xcount,ycount,vx,vy);
              rgb_mul(&color,tmp);
            }
          else
            lic_image(xcount,ycount,vx,vy,&color);
          
          poke(xcount,ycount,&color);

          counter++;

          if ((counter % width)==0)
            gimp_progress_update((gfloat)counter/(gfloat)maxcounter);
        }
    }
}

void compute_lic_gradient(void)
{
  gint xcount,ycount;
  glong counter=0;
  rgbpixel color;
  gdouble vx,vy,tmp;
    
  for (ycount=0;ycount<height;ycount++)
    {
      for (xcount=0;xcount<width;xcount++)
        {
          /* Get derivative at (x,y), rotate it 90 degrees and normalize it */
          /* ============================================================== */

          vx=gradx(scalarfield,xcount,ycount);
          vy=grady(scalarfield,xcount,ycount);

          vx=-1.0*vx; tmp=vy; vy=vx; vx=tmp;

          tmp=sqrt(vx*vx+vy*vy);
          if (tmp!=0.0)
            {
              tmp=1.0/tmp;
              vx*=tmp; vy*=tmp;
            }

          /* Convolve with the LIC at (x,y) */
          /* ============================== */
 
          if (licvals.effect_convolve==0)
            {
              color=peek(xcount,ycount);
              tmp=lic_noise(xcount,ycount,vx,vy);
              rgb_mul(&color,tmp);
            }
          else
            lic_image(xcount,ycount,vx,vy,&color);
          
          poke(xcount,ycount,&color);

          counter++;

          if ((counter % width)==0)
            gimp_progress_update((gfloat)counter/(gfloat)maxcounter);
        }
    }
}

void compute_image(void)
{
  gint32 new_image_id=-1,new_layer_id=-1;
  GDrawable *effect;
  
  if (licvals.create_new_image==TRUE)
    {
      /* Create a new image */
      /* ================== */

      new_image_id=gimp_image_new(width,height,RGB);

      /* Create a "normal" layer */
      /* ======================= */

       new_layer_id=gimp_layer_new(new_image_id, _("Background"),
         width,height,RGB_IMAGE,100.0,NORMAL_MODE);

      gimp_image_add_layer(new_image_id,new_layer_id,0);
      output_drawable=gimp_drawable_get(new_layer_id);
    }

  gimp_pixel_rgn_init (&dest_region, output_drawable, 0,0, width,height, TRUE,TRUE);

  gimp_progress_init( _("Van Gogh (LIC)"));

  if (licvals.effect_convolve==0)
    generatevectors();

  l=(gdouble)licvals.filtlen;
  dx=dy=(gdouble)licvals.noisemag;
  minv=((gdouble)licvals.minv)/10.0;
  maxv=((gdouble)licvals.maxv)/10.0;
  isteps=(gdouble)licvals.intsteps;

  effect=gimp_drawable_get(licvals.effect_image_id);

  switch (licvals.effect_channel)
    {
      case 0:
        rgb_to_hue(effect,&scalarfield);
        break;
      case 1:
        rgb_to_saturation(effect,&scalarfield);
        break;
      case 2:
        rgb_to_brightness(effect,&scalarfield);
        break;
    }

  if (scalarfield==NULL)
    {
      printf( _("LIC: Couldn't allocate temporary buffer - out of memory!\n"));
      return;
    }

  if (licvals.effect_operator==0)
    compute_lic_derivative();
  else
    compute_lic_gradient();

  free(scalarfield);

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

/**************************/
/* Below is only UI stuff */
/**************************/

void ok_button_clicked(GtkWidget *widget, gpointer client_data)
{
  gtk_widget_hide((GtkWidget *)client_data);
  gdk_flush();
  compute_image();
  gtk_main_quit();
}

void cancel_button_clicked(GtkWidget *widget, gpointer client_data)
{
  gtk_main_quit();
}

void dialog_destroy(GtkWidget *widget, GdkEvent *ev, gpointer client_data)
{
  gtk_main_quit();
}

void effect_channel_callback(GtkWidget *widget,gpointer client_data)
{
  if (GTK_TOGGLE_BUTTON(widget)->active)
    {
      licvals.effect_channel=(guchar)((glong)client_data);
    }
}

void effect_operator_callback(GtkWidget *widget,gpointer client_data)
{
  if (GTK_TOGGLE_BUTTON(widget)->active)
    {
      licvals.effect_operator=(guchar)((glong)client_data);
    }
}

void effect_convolve_callback(GtkWidget *widget,gpointer client_data)
{
  if (GTK_TOGGLE_BUTTON(widget)->active)
    licvals.effect_convolve=(guchar)((glong)client_data);
}

gint effect_image_constrain(gint32 image_id, gint32 drawable_id, gpointer data)
{
  if (drawable_id == -1)
    return(TRUE);

  return(gimp_drawable_is_rgb(drawable_id));
}

void effect_image_callback(gint32 id, gpointer data)
{
  licvals.effect_image_id=id;
}

void effect_parameter_update(GtkAdjustment *adjustment, gpointer client_data)
{
  *((glong *)client_data)=(gdouble)adjustment->value;
}
 
void create_main_dialog(void)
{
  GtkWidget *frame;
  GtkWidget *vbox,*hbox,*vbox2,*hbox2;
  GtkWidget *label;
  GtkWidget *option_menu;
  GtkWidget *menu;
  GtkWidget *hbbox;
  GtkWidget *button;
  GtkWidget *scale;
  GtkObject *scale_data;
  GSList *group=NULL;
  
  /* Dialog */
    
  dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), _("Van Gogh (LIC)"));
  gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width(GTK_CONTAINER(dialog), 0);
  gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
    (GtkSignalFunc)dialog_destroy, (gpointer)dialog);

  hbox = gtk_hbox_new(FALSE,5);
  gtk_container_border_width(GTK_CONTAINER(hbox),5);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),hbox);

  vbox=gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

  frame = gtk_frame_new( _("Options"));
  gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER(vbox),frame);

  hbox2=gtk_hbox_new(FALSE,5);
  gtk_container_add(GTK_CONTAINER(frame),hbox2);
  
  button = gtk_check_button_new_with_label( _("Create new image"));
  if (licvals.create_new_image==TRUE)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),TRUE);
  gtk_container_add(GTK_CONTAINER(hbox2),button);
  gtk_widget_show(button);

  gtk_widget_show(hbox2);
  gtk_widget_show(frame);
  
  frame = gtk_frame_new( _("Effect channel"));
  gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER(vbox),frame);

  vbox2=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(frame),vbox2);

  button = gtk_radio_button_new_with_label(NULL, _("Hue"));
  group  = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
  if (licvals.effect_channel==0)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

  gtk_signal_connect(GTK_OBJECT(button), "toggled",
    (GtkSignalFunc)effect_channel_callback,
    (gpointer)0);

  gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  button = gtk_radio_button_new_with_label(group, _("Saturation"));
  if (licvals.effect_channel==1)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

  gtk_signal_connect(GTK_OBJECT(button), "toggled",
    (GtkSignalFunc)effect_channel_callback,
    (gpointer)1);

  gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  button = gtk_radio_button_new_with_label(group, _("Brightness"));
  if (licvals.effect_channel==2)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

  gtk_signal_connect(GTK_OBJECT(button), "toggled",
    (GtkSignalFunc)effect_channel_callback,
    (gpointer)2);

  gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  gtk_widget_show(vbox2);
  gtk_widget_show(frame);
  
  frame = gtk_frame_new( _("Effect operator"));
  gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER(vbox),frame);
  
  vbox2=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(frame),vbox2);

  button = gtk_radio_button_new_with_label(NULL, _("Derivative"));
  group  = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
  if (licvals.effect_operator==0)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

  gtk_signal_connect(GTK_OBJECT(button), "toggled",
    (GtkSignalFunc)effect_operator_callback,
    (gpointer)0);

  gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  button = gtk_radio_button_new_with_label(group, _("Gradient"));
  if (licvals.effect_operator==1)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

  gtk_signal_connect(GTK_OBJECT(button), "toggled",
    (GtkSignalFunc)effect_operator_callback,
    (gpointer)1);

  gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show(button);
  
  gtk_widget_show(vbox2);
  gtk_widget_show(frame);

  frame = gtk_frame_new( _("Convolve"));
  gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER(vbox),frame);

  vbox2=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(frame),vbox2);

  button = gtk_radio_button_new_with_label(NULL, _("With white noise"));
  group  = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
  if (licvals.effect_convolve==0)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

  gtk_signal_connect(GTK_OBJECT(button), "toggled",
    (GtkSignalFunc)effect_convolve_callback,
    (gpointer)0);

  gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  button = gtk_radio_button_new_with_label(group, _("With source image"));
  if (licvals.effect_convolve==1)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

  gtk_signal_connect(GTK_OBJECT(button), "toggled",
    (GtkSignalFunc)effect_convolve_callback,
    (gpointer)1);

  gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show(button);
  
  gtk_widget_show(vbox2);
  gtk_widget_show(frame);

  gtk_widget_show(vbox);

  vbox=gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

  frame = gtk_frame_new( _("Parameters"));
  gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER(vbox),frame);

  /* Effect image menu */

  vbox2=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(frame),vbox2);

  hbox2=gtk_hbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(vbox2),hbox2);

  label = gtk_label_new( _("Effect image:"));
  gtk_container_add(GTK_CONTAINER(hbox2),label);
  gtk_widget_show(label);

  option_menu = gtk_option_menu_new();
    
  menu = gimp_drawable_menu_new(effect_image_constrain,
                                effect_image_callback,
				NULL,
                                licvals.effect_image_id);

  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);

  gtk_container_add(GTK_CONTAINER(hbox2),option_menu);
  gtk_widget_show(option_menu);
    
  gtk_widget_show(hbox2);

  label = gtk_label_new( _("Filter length:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.5, 1.0);
  gtk_container_add(GTK_CONTAINER(vbox2),label);
  gtk_widget_show(label);

  scale_data = gtk_adjustment_new(licvals.filtlen, 0, 64, 1.0, 1.0, 0.0);
  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
    (GtkSignalFunc)effect_parameter_update,
    (gpointer)&licvals.filtlen);				    
  scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_container_add(GTK_CONTAINER(vbox2),scale);
  gtk_widget_show(scale);

  label = gtk_label_new( _("Noise magnitude:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.5, 1.0);
  gtk_container_add(GTK_CONTAINER(vbox2),label);
  gtk_widget_show(label);

  scale_data = gtk_adjustment_new(licvals.noisemag, 1, 5, 1.0, 1.0, 0.0);
  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
    (GtkSignalFunc)effect_parameter_update,
    (gpointer)&licvals.noisemag);
 
  scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_container_add(GTK_CONTAINER(vbox2),scale);
  gtk_widget_show(scale);

  label = gtk_label_new( _("Integration steps:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.5, 1.0);
  gtk_container_add(GTK_CONTAINER(vbox2),label);
  gtk_widget_show(label);

  scale_data = gtk_adjustment_new(licvals.intsteps, 1, 40, 1.0, 1.0, 0.0);
  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
    (GtkSignalFunc)effect_parameter_update,
    (gpointer)&licvals.intsteps);
 
  scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_container_add(GTK_CONTAINER(vbox2),scale);
  gtk_widget_show(scale);

  label = gtk_label_new( _("Minimum value:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.5, 1.0);
  gtk_container_add(GTK_CONTAINER(vbox2),label);
  gtk_widget_show(label);

  scale_data = gtk_adjustment_new(licvals.minv, -100, 0, 1.0, 1.0, 0.0);
  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
    (GtkSignalFunc)effect_parameter_update,
    (gpointer)&licvals.minv);
 
  scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_container_add(GTK_CONTAINER(vbox2),scale);
  gtk_widget_show(scale);

  label = gtk_label_new( _("Maximum value:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.5, 1.0);
  gtk_container_add(GTK_CONTAINER(vbox2),label);
  gtk_widget_show(label);

  scale_data = gtk_adjustment_new(licvals.maxv, 0, 100, 1.0, 1.0, 0.0);
  gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
    (GtkSignalFunc)effect_parameter_update,
    (gpointer)&licvals.maxv);
 
  scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
  gtk_container_add(GTK_CONTAINER(vbox2),scale);
  gtk_widget_show(scale);

  gtk_widget_show(vbox2);
  gtk_widget_show(frame);

  gtk_widget_show(vbox);
  gtk_widget_show(hbox);

  /*  Action area  */
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dialog)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);
 
  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) ok_button_clicked,
		      dialog);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) cancel_button_clicked,
		      (gpointer)dialog);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* Done */
    
  gtk_widget_show(dialog);
}

/******************/
/* Implementation */
/******************/

void lic_interactive    (GDrawable *drawable);
void lic_noninteractive (GDrawable *drawable);

/*************************************/
/* Set parameters to standard values */
/*************************************/

void set_default_settings(void)
{
  licvals.filtlen=5;
  licvals.noisemag=2;
  licvals.intsteps=25;
  licvals.minv=-25;
  licvals.maxv=25;
  licvals.create_new_image=TRUE;  
  licvals.effect_channel=2;
  licvals.effect_operator=1;
  licvals.effect_convolve=1;
  licvals.effect_image_id=0;
}

static void query(void)
{
  static GParamDef args[] =
    {
      { PARAM_INT32, "run_mode", "Interactive" },
      { PARAM_IMAGE, "image", "Input image" },
      { PARAM_DRAWABLE, "drawable", "Input drawable" },
    };

  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure ("plug_in_lic",
			  _("Creates a Van Gogh effect (Line Integral Convolution)"),
			  "No help yet",
			  "Tom Bech & Federico Mena Quintero",
			  "Tom Bech & Federico Mena Quintero",
			  "Version 0.14, September 24 1997",
			  N_("<Image>/Filters/Map/Van Gogh (LIC)..."),
			  "RGB",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void run(gchar   *name,
                gint     nparams,
                GParam  *param,
                gint    *nreturn_vals,
                GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  if (run_mode == RUN_INTERACTIVE) {
    INIT_I18N_UI();
  } else {
    INIT_I18N();
  }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  /* Set default values */
  /* ================== */

  set_default_settings();

  /* Possibly retrieve data */
  /* ====================== */

  gimp_get_data ("plug_in_lic", &licvals);

  /* Get the specified drawable */
  /* ========================== */
  
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  if (status == STATUS_SUCCESS)
    {
      /* Make sure that the drawable is RGBA or RGB color */
      /* ================================================ */

      if (gimp_drawable_is_rgb(drawable->id))
	{
	  /* Set the tile cache size */
          /* ======================= */

	  gimp_tile_cache_ntiles(TILE_CACHE_SIZE);
          
          switch (run_mode)
            {
              case RUN_INTERACTIVE:
                lic_interactive(drawable);
                gimp_set_data("plug_in_lic", &licvals, sizeof(LicValues));
              break;
              case RUN_WITH_LAST_VALS:
                image_setup(drawable,FALSE);
                compute_image();
                break;
              default:
                break;
            }
        }
      else
        status = STATUS_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;
  gimp_drawable_detach (drawable);
}

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

void lic_interactive(GDrawable *drawable)
{
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("lic");

  gtk_init (&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());

  gdk_set_use_xshm(gimp_use_xshm());

  /* Create application window */
  /* ========================= */

  create_main_dialog();

  /* Prepare images */
  /* ============== */

  image_setup(drawable,TRUE);
  
  /* Gtk main event loop */
  /* =================== */
  
  gtk_main();
  gdk_flush();
}

void lic_noninteractive(GDrawable *drawable)
{
  printf( _("Noninteractive not yet implemented! Sorry.\n"));
}

MAIN()

