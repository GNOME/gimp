/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* IWarp  a plug-in for the GIMP
   Version 0.1 
   
   IWarp is a gimp plug-in for interactive image warping. To apply the 
   selected deformation to the image, press the left mouse button and 
   move the mouse pointer in the preview image.
   
   Copyright (C) 1997 Norbert Schmitz
   nobert.schmitz@student.uni-tuebingen.de

   Most of the gimp and gtk specific code is taken from other plug-ins 

   v0.11a
    animation of non-alpha layers (background) creates now layers with 
    alpha channel. (thanks to Adrian Likins for reporting this bug)

  v0.12 
    fixes a very bad bug. 
     (thanks to Arthur Hagen for reporting it)
    
*/
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#define MAX_PREVIEW_WIDTH 256
#define MAX_PREVIEW_HEIGHT 256
#define MAX_DEFORM_AREA_RADIUS 100

#define SCALE_WIDTH 150
#define MAX_NUM_FRAMES 100
#define CHECK_SIZE 32
#define CHECK_LIGHT 170
#define CHECK_DARK  85

typedef struct 
{
 gfloat x;
 gfloat y;
}vector_2d;
 

typedef struct
{
 gint run;
}iwarp_interface;
 
typedef struct
{
 gint deform_area_radius;
 gfloat deform_amount;
 gint do_grow;
 gint do_shrink;
 gint do_move;
 gint do_remove;
 gint do_swirl_ccw;
 gint do_swirl_cw;
 gint do_bilinear;
 gint do_supersample;
 gfloat supersample_threshold;
 gint max_supersample_depth;
} iwarp_vals_t; 


/* Declare local functions.
 */
static void      query  (void);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);

static gint      iwarp_dialog();
static void      iwarp();
static void      iwarp_frame();

static void      iwarp_close_callback  (GtkWidget *widget,
					 gpointer   data);
static void      iwarp_ok_callback     (GtkWidget *widget,
					 gpointer   data);
static gint      iwarp_motion_callback (GtkWidget *widget,
                                          GdkEvent *event);
static void      iwarp_iscale_update (GtkAdjustment *adjustment,
                                      gint* scale_val);
static void      iwarp_fscale_update (GtkAdjustment *adjustment,
                                      gfloat* scale_val);
static void      iwarp_toggle_update (GtkWidget *widget,
                                        int  *data);
static void      iwarp_supersample_toggle (GtkWidget *widget,
                                        int  *data);
static void      iwarp_animate_toggle (GtkWidget *widget,
                                        int  *data);

static void      iwarp_reset_callback (GtkWidget *widget,
		                       gpointer   data);


static void      iwarp_update_preview(int x0, int y0,int x1,int y1);

static gint32    iwarp_layer_copy(gint32 layerID);

static void      iwarp_get_pixel(int x, int y, guchar *pixel);

static void      iwarp_get_deform_vector(gfloat x, gfloat y, 
                                         gfloat* xv, gfloat* yv);
                                         
static void      iwarp_get_point(gfloat x, gfloat y, guchar *color);

static gint      iwarp_supersample_test(vector_2d* v0 , vector_2d* v1, 
                                        vector_2d* v2, vector_2d* v3);
                                        
static void      iwarp_getsample(vector_2d v0, vector_2d  v1,
                                 vector_2d v2, vector_2d v3, 
                                 gfloat x, gfloat y,gint* sample,gint* cc,
                                 gint depth,gfloat scale);
                                 
static void      iwarp_supersample(gint sxl,gint syl ,gint sxr, gint syr,
                                   guchar* dest_data,int stride,
                                   int* progress,int max_progress);
                                   
static guchar    iwarp_transparent_color(int x, int y);

static void      iwarp_cpy_images();

static void      iwarp_animate_dialog(GtkWidget* dlg, GtkWidget* notebook);
                                 
static void      iwarp_settings_dialog(GtkWidget* dlg, GtkWidget* notebook);                                 

static void      iwarp_preview_get_pixel(int x, int y , guchar **color);

static void      iwarp_preview_get_point( gfloat x, gfloat y,guchar* color);

static void      iwarp_deform(int x, int y, gfloat vx, gfloat vy);

static void      iwarp_move(int x, int y,int xx, int yy);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static iwarp_interface wint = 
{
  FALSE
};
static iwarp_vals_t iwarp_vals = 
{
 20,
 0.3,
 FALSE,
 FALSE,
 TRUE,
 FALSE,
 FALSE,
 FALSE,
 TRUE,
 FALSE,
 2.0,
 2
};  


static GDrawable *drawable = NULL;
static GDrawable *destdrawable = NULL;
static GtkWidget *preview = NULL;
static GtkWidget* supersample_frame;
static GtkWidget* animate_frame;
static guchar *srcimage = NULL;
static guchar *dstimage = NULL;
static gint preview_width, preview_height,sel_width,sel_height;
static gint image_bpp;
static vector_2d* deform_vectors = NULL;
static vector_2d* deform_area_vectors = NULL;
static int lastx, lasty;
static gfloat filter[MAX_DEFORM_AREA_RADIUS];
static int do_animate = FALSE;
static int do_animate_reverse = FALSE;
static int do_animate_ping_pong = FALSE;
static gfloat supersample_threshold_2;
static int xl,yl,xh,yh;
static int tile_width, tile_height;
static GTile* tile = NULL;
static gfloat pre2img, img2pre;
static int preview_bpp;
static gfloat animate_deform_value = 1.0;
static gint32 imageID;
static int animate_num_frames = 2;
static int frame_number;
static int layer_alpha;


MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_iwarp",
			  "Interactive warping of the specified drawable",
			  "Interactive warping of the specified drawable ",
			  "Norbert Schmitz",
			  "Norbert Schmitz",
			  "1997",
			  "<Image>/Filters/Distorts/IWarp",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  /*  Get the specified drawable  */
  destdrawable = drawable = gimp_drawable_get (param[2].data.d_drawable);
  imageID = param[1].data.d_int32;
  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_color (drawable->id) || gimp_drawable_is_gray (drawable->id)) { 
      switch ( run_mode) {
        case RUN_INTERACTIVE :
          gimp_get_data("plug_in_iwarp",&iwarp_vals);
          gimp_tile_cache_ntiles(2*(drawable->width +gimp_tile_width()-1) / gimp_tile_width());
          if (iwarp_dialog()) iwarp();
          gimp_set_data("plug_in_iwarp",&iwarp_vals,sizeof(iwarp_vals_t));
 	  gimp_displays_flush ();
	break;
        case RUN_NONINTERACTIVE :
         status = STATUS_CALLING_ERROR;
        break;
        case RUN_WITH_LAST_VALS :
         status = STATUS_CALLING_ERROR;
        break;

        default : 
        break;	
      }
  }
  else {
      status = STATUS_EXECUTION_ERROR;
  }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
  if (srcimage != NULL) g_free(srcimage);
  if (dstimage != NULL) g_free(dstimage);  
  if (deform_vectors != NULL) g_free(deform_vectors);
  if (deform_area_vectors != NULL) g_free(deform_area_vectors);
}




static void
iwarp_get_pixel(int x, int y, guchar *pixel)
{
 static gint old_col = -1 , old_row = -1;
 guchar* data;
 gint col, row;
 int i;
 
 if (x>=xl && x < xh && y >=yl && y < yh) {
  col = x / tile_width;
  row = y / tile_height;
  if ( col != old_col || row != old_row) {
    gimp_tile_unref(tile,FALSE);
    tile = gimp_drawable_get_tile(drawable,FALSE,row,col);
    gimp_tile_ref(tile);
    old_col = col;
    old_row = row;
  }
  data = tile->data + (tile->ewidth * (y % tile_height) + x % tile_width) * image_bpp;
  for (i=0; i<image_bpp; i++) *pixel++ = *data++;
 }
 else {
  pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
 }
}
 


static void
iwarp_get_deform_vector(gfloat x, gfloat y, gfloat* xv, gfloat* yv)
{
 int i,xi,yi;
 gfloat dx,dy,my0,my1,mx0,mx1;

 if (x>=0 && x< (preview_width-1) && y>=0  && y<(preview_height-1)) { 
  xi = (int)x;
  yi = (int)y;
  dx = x-xi;
  dy = y-yi;
  i = (yi*preview_width+ xi);
  mx0 = deform_vectors[i].x + (deform_vectors[i+1].x-deform_vectors[i].x)*dx;
  mx1 = deform_vectors[i+preview_width].x +  (deform_vectors[i+preview_width+1].x-deform_vectors[i+preview_width].x)*dx;
  my0 = deform_vectors[i].y +dx * (deform_vectors[i+1].y-deform_vectors[i].y);
  my1 = deform_vectors[i+preview_width].y + dx * (deform_vectors[i+preview_width+1].y-deform_vectors[i+preview_width].y);
  *xv = mx0 + dy *( mx1-mx0);
  *yv = my0 + dy * (my1-my0); 
 }
 else  *xv = *yv = 0.0; 
}



static void
iwarp_get_point(gfloat x, gfloat y, guchar *color)
{
  gfloat dx,dy,m0,m1;
  guchar p0[4],p1[4],p2[4],p3[4]; 
  gint xi,yi,i;
  
  xi = (int)x;
  yi = (int)y;
  dx = x-xi;
  dy = y-yi; 
  iwarp_get_pixel(xi,yi,p0);
  iwarp_get_pixel(xi+1,yi,p1);
  iwarp_get_pixel(xi,yi+1,p2);
  iwarp_get_pixel(xi+1,yi+1,p3); 	         
  for (i = 0; i < image_bpp; i++) {
    m0 = p0[i] + dx*(p1[i]-p0[i]);
    m1 = p2[i] + dx*(p3[i]-p2[i]);
    color[i] = (guchar)(m0 + dy*(m1-m0));
  }
} 


static gint
iwarp_supersample_test(vector_2d* v0 , vector_2d* v1, vector_2d* v2, vector_2d* v3)
{
 gfloat dx,dy;
 
 dx = 1.0+v1->x - v0->x;
 dy = v1->y - v0->y;
 if (dx*dx+dy*dy > supersample_threshold_2 ) 
   return 1;
 dx = 1.0+v2->x - v3->x;
 dy = v2->y - v3->y;
 if (dx*dx+dy*dy > supersample_threshold_2 ) 
   return 1;
 dx = v2->x - v0->x;
 dy = 1.0+v2->y - v0->y;
 if (dx*dx+dy*dy > supersample_threshold_2 ) 
   return 1;
 dx = v3->x - v1->x;
 dy = 1.0+v3->y - v1->y;
 if (dx*dx+dy*dy > supersample_threshold_2 ) 
   return 1; 
 return 0;
}

static void
iwarp_getsample(vector_2d v0, vector_2d  v1,vector_2d v2, vector_2d v3, 
                gfloat x, gfloat y,gint* sample,gint* cc,
                gint depth,gfloat scale)
{
 int i;
 gfloat xv,yv;
 vector_2d v01,v13,v23,v02,vm;
 guchar c[4];

 if ((depth >=  iwarp_vals.max_supersample_depth) 
      || (!iwarp_supersample_test(&v0,&v1,&v2,&v3))) { 
   iwarp_get_deform_vector(img2pre * (x - xl), img2pre * (y-yl),&xv,&yv);
   xv *= animate_deform_value; yv *= animate_deform_value;
   iwarp_get_point(pre2img*xv+x,pre2img*yv+y,c);
   for (i=0;  i< image_bpp; i++) sample[i] += c[i];
   (*cc)++;
 }
 else {
   scale *= 0.5;
   iwarp_get_deform_vector(img2pre * (x - xl), img2pre * (y-yl),&xv,&yv);
   xv *= animate_deform_value; yv *= animate_deform_value;
   iwarp_get_point(pre2img*xv+x,pre2img*yv+y,c);
   for (i=0;  i< image_bpp; i++) sample[i] += c[i];
   (*cc)++;
   vm.x = xv;
   vm.y = yv;

   iwarp_get_deform_vector(img2pre * (x - xl), img2pre * (y-yl-scale),&xv,&yv);
   xv *= animate_deform_value; yv *= animate_deform_value;
   v01.x = xv;
   v01.y = yv;
  
   iwarp_get_deform_vector(img2pre * (x - xl+scale), img2pre * (y-yl),&xv,&yv);
   xv *= animate_deform_value; yv *= animate_deform_value;
   v13.x = xv;
   v13.y = yv;
 
   iwarp_get_deform_vector(img2pre * (x - xl), img2pre * (y-yl+scale),&xv,&yv);
   xv *= animate_deform_value; yv *= animate_deform_value;
   v23.x = xv;
   v23.y = yv;
 
   iwarp_get_deform_vector(img2pre * (x - xl-scale), img2pre * (y-yl),&xv,&yv);
   xv *= animate_deform_value; yv *= animate_deform_value;
   v02.x = xv;
   v02.y = yv;
 
   iwarp_getsample(v0,v01,vm,v02,x-scale, y-scale,sample, cc, depth+1,
                   scale);
   iwarp_getsample(v01,v1,v13,vm,x+scale, y-scale,sample, cc, depth+1,
                   scale);
   iwarp_getsample(v02,vm,v23,v2,x-scale, y+scale,sample, cc, depth+1,
                  scale);
   iwarp_getsample(vm,v13,v3,v23,x+scale, y+scale,sample, cc, depth+1,
                  scale);
  }   
}


static void
iwarp_supersample(gint sxl,gint syl ,gint sxr, gint syr,guchar* dest_data,int stride,
                  int* progress,int max_progress)
{
 int i,wx,wy,col,row,cc;
 vector_2d *srow,*srow_old, *vh;
 gfloat xv,yv;
 gint color[4];
 guchar *dest;
 
 wx = sxr-sxl+1;
 wy = syr-syl+1;
 srow = g_malloc((sxr-sxl+1)*2*sizeof(gfloat));
 srow_old = g_malloc((sxr-sxl+1)*2*sizeof(gfloat));

 for (i=sxl; i< (sxr+1); i++) {
    iwarp_get_deform_vector(img2pre * (-0.5 +i-xl), img2pre * (-0.5+syl-yl),&xv,&yv);
   xv *= animate_deform_value; yv *= animate_deform_value;
   srow_old[i-sxl].x = xv;
    srow_old[i-sxl].y = yv;
 }
 
 for (col = syl; col <syr; col++) {
  iwarp_get_deform_vector(img2pre * (-0.5+sxl-xl), img2pre * (0.5+col-yl),&xv,&yv);
  xv *= animate_deform_value; yv *= animate_deform_value;
  srow[0].x = xv;
  srow[0].y = yv; 
  for (row = sxl; row <sxr; row++) {
    iwarp_get_deform_vector(img2pre * (0.5+row-xl), img2pre * (0.5+col-yl),&xv,&yv);
    xv *= animate_deform_value; yv *= animate_deform_value;
    srow[row-sxl+1].x = xv;
    srow[row-sxl+1].y = yv;
    cc = 0;
    color[0] = color[1] = color[2] = color[3] = 0;
    iwarp_getsample( srow_old[row-sxl] ,srow_old[row-sxl+1],
                     srow[row-sxl] ,srow[row-sxl+1], row,col,color,&cc,0,1.0);
    if (layer_alpha) dest = dest_data+(col-syl)*(stride)+(row-sxl)*(image_bpp+1);
    else  dest = dest_data+(col-syl)*stride+(row-sxl)*image_bpp;
    for (i=0; i<image_bpp; i++) *dest++ = color[i] / cc;
    if (layer_alpha) *dest++ = 255; 
    (*progress)++;
  }      
  gimp_progress_update((double) (*progress) / max_progress);
  vh = srow_old;
  srow_old = srow;
  srow = vh;   
 }    
 g_free(srow);
 g_free(srow_old);
}


static void
iwarp_frame()
{
  GPixelRgn dest_rgn;
  gpointer pr;
  guchar *dest_row, *dest;
  gint row, col;
  gint i;
  gint progress, max_progress;
  guchar color[4];
  gfloat xv,yv;

  progress = 0;
  max_progress = (yh-yl)*(xh-xl);

  gimp_pixel_rgn_init (&dest_rgn, destdrawable, xl, yl, xh-xl, yh-yl, TRUE, TRUE);
  if (!do_animate) gimp_progress_init("Warping ...");
  for (pr = gimp_pixel_rgns_register(1, &dest_rgn);
         pr != NULL; pr = gimp_pixel_rgns_process(pr)) {
         dest_row = dest_rgn.data;
         if (!iwarp_vals.do_supersample) {
	   for (row = dest_rgn.y; row < (dest_rgn.y + dest_rgn.h); row++) {
	     dest = dest_row;
             for (col = dest_rgn.x; col < (dest_rgn.x + dest_rgn.w); col++) {  	  
	        progress++;
	        iwarp_get_deform_vector(img2pre * (col -xl), img2pre * (row -yl), &xv ,&yv);
	        xv *= animate_deform_value; yv *= animate_deform_value;
	        if (fabs(xv) > 0.0 || fabs(yv) > 0.0) { 
	          iwarp_get_point(pre2img * xv +col, pre2img *yv +row,color);
 	          for (i=0; i<image_bpp; i++) *dest++ = color[i];
                  if (layer_alpha) *dest++ = 255;
 	        }
 	        else {
 	         iwarp_get_pixel(col,row,color);
 	         for (i=0; i<image_bpp; i++) *dest++ = color[i];
 	         if (layer_alpha) *dest++ = 255;
 	        }  
	     }
	     dest_row += dest_rgn.rowstride;
             gimp_progress_update((double) (progress) / max_progress);
	   }
	 }
	 else {
            supersample_threshold_2 = iwarp_vals.supersample_threshold * 
                                      iwarp_vals.supersample_threshold; 
	    iwarp_supersample(dest_rgn.x, dest_rgn.y, dest_rgn.x+dest_rgn.w,
	                      dest_rgn.y+dest_rgn.h,dest_rgn.data,
	                      dest_rgn.rowstride,&progress, max_progress);
	     
	 } 
	     
  }
  gimp_drawable_flush (destdrawable);
  gimp_drawable_merge_shadow (destdrawable->id, TRUE);
  gimp_drawable_update (destdrawable->id, xl, yl, (xh - xl), (yh - yl));

}

static gint32 
iwarp_layer_copy(gint32 layerID)
{
 GParam *return_vals;
 int nreturn_vals;
 gint32 nlayer;

 return_vals = gimp_run_procedure ("gimp_layer_copy", 
                                    &nreturn_vals,
                                    PARAM_LAYER, layerID,
                                    PARAM_INT32, TRUE,
                                    PARAM_END);
 
 if (return_vals[0].data.d_status == STATUS_SUCCESS)
   nlayer = return_vals[1].data.d_layer;
 else nlayer = -1;
 gimp_destroy_params(return_vals, nreturn_vals);
 return nlayer;
} 
 
                                  


static void iwarp()
{
 int i;
 gint32 layerID;
 gint32 *animlayers;
 char st[100];
 gfloat delta;

 
 if (animate_num_frames > 1 && do_animate) {
  animlayers = g_malloc(animate_num_frames * sizeof(gint32));
  if (do_animate_reverse) {
   animate_deform_value = 1.0;
   delta = -1.0/(animate_num_frames-1);
  }
  else {
   animate_deform_value = 0.0;
   delta = 1.0/(animate_num_frames-1);
  }
  layerID = gimp_image_get_active_layer(imageID);
  if (image_bpp == 1 || image_bpp == 3) layer_alpha = TRUE; else layer_alpha = FALSE;
  frame_number = 0;
  for (i=0; i< animate_num_frames; i++) {
   sprintf(st,"Frame %d",i);
   animlayers[i] = iwarp_layer_copy(layerID);
   gimp_layer_set_name(animlayers[i],st);
   destdrawable = gimp_drawable_get(animlayers[i]);
   sprintf(st,"Warping Frame Nr %d ...",frame_number);
   gimp_progress_init(st);
   if (animate_deform_value >0.0) iwarp_frame();
   gimp_image_add_layer(imageID,animlayers[i],0);
   animate_deform_value = animate_deform_value + delta;
   frame_number++;
  }
  if (do_animate_ping_pong) {
   sprintf(st,"Warping Frame Nr %d ...",frame_number);
   gimp_progress_init("Ping Pong");
   for (i=0; i < animate_num_frames; i++) {
     gimp_progress_update((double)i / (animate_num_frames-1));
     layerID = iwarp_layer_copy(animlayers[animate_num_frames-i-1]); 
     sprintf(st,"Frame %d",i+animate_num_frames);
     gimp_layer_set_name(layerID,st);
     gimp_image_add_layer(imageID,layerID,0); 
   }
  }
  g_free(animlayers);
 } 
 else {
  animate_deform_value = 1.0;
  iwarp_frame();
 } 
 if (tile != NULL) {
   gimp_tile_unref(tile, FALSE);
   tile = NULL;
 }
} 


static guchar
iwarp_transparent_color(int x, int y)
{

 if ((y % CHECK_SIZE) > (CHECK_SIZE / 2)) {
  if ((x % CHECK_SIZE) > (CHECK_SIZE / 2))  return CHECK_DARK; 
   else return CHECK_LIGHT; 
 }
 else
  if ((x % CHECK_SIZE) < (CHECK_SIZE / 2))  return CHECK_DARK; 
   else return CHECK_LIGHT;
}



static void
iwarp_cpy_images()
{
 int i,j,k,p;
 gfloat alpha;
 guchar *srccolor, *dstcolor;
 
 if (image_bpp == 1 || image_bpp ==3) 
  memcpy(dstimage,srcimage,preview_width *preview_height*preview_bpp);
 else {
  for (i=0; i< preview_width; i++) 
   for (j=0; j< preview_height; j++) {
    p = (j*preview_width+i) ;
    srccolor = srcimage + p*image_bpp;
    alpha = (gfloat)srccolor[image_bpp-1]/255;
    dstcolor = dstimage + p*preview_bpp;
    for (k=0; k <preview_bpp; k++) {
     *dstcolor++ = (guchar)(alpha *srccolor[k]+(1.0-alpha)*iwarp_transparent_color(i,j));
    }
   }  
 }
}


static void 
iwarp_init()
{
 int y,x,xi,i;
 GPixelRgn srcrgn;
 guchar *pts;
 guchar *linebuffer = NULL;
 gfloat dx,dy;
 
 gimp_drawable_mask_bounds (drawable->id, &xl, &yl, &xh, &yh);
 sel_width = xh-xl;
 sel_height = yh-yl;
 image_bpp = gimp_drawable_bpp(drawable->id);
 if (image_bpp <3) preview_bpp = 1; else preview_bpp = 3;
 dx = (gfloat) sel_width / MAX_PREVIEW_WIDTH;
 dy = (gfloat) sel_height / MAX_PREVIEW_HEIGHT;
 if (dx >dy) pre2img = dx; else pre2img = dy;
 if (dx <=1.0 && dy <= 1.0) pre2img = 1.0;  
 img2pre = 1.0 / pre2img;
 preview_width = (int)(sel_width / pre2img);
 preview_height = (int)(sel_height / pre2img);
 tile_width = gimp_tile_width();
 tile_height = gimp_tile_height();

 srcimage = g_malloc(preview_width * preview_height * image_bpp * sizeof(guchar));
 dstimage = g_malloc(preview_width * preview_height * preview_bpp * sizeof(guchar));
 deform_vectors = g_malloc(preview_width * preview_height*sizeof(gfloat)*2);
 deform_area_vectors = g_malloc((MAX_DEFORM_AREA_RADIUS*2+1)*(MAX_DEFORM_AREA_RADIUS*2+1)*sizeof(gfloat)*2);
 linebuffer = g_malloc(sel_width* image_bpp *sizeof(guchar));

 for (i=0; i<preview_width*preview_height; i++) 
        deform_vectors[i].x = deform_vectors[i].y = 0.0;
  
 gimp_pixel_rgn_init(&srcrgn,drawable,xl,yl,sel_width,sel_height,FALSE,FALSE);
  
 for (y=0; y<preview_height; y++) {
  gimp_pixel_rgn_get_row(&srcrgn,linebuffer,xl,(int)(pre2img*y)+yl,sel_width);
  for (x = 0; x<preview_width; x++) {
   pts = srcimage + (y * preview_width +x) * image_bpp;
   xi = (int)(pre2img*x);
   for (i=0; i<image_bpp; i++) {
     *pts++ = linebuffer[xi*image_bpp+i];
   }
  }
 }
 iwarp_cpy_images();
 for (i=0; i <MAX_DEFORM_AREA_RADIUS; i++) { 
   filter[i] = 
    pow((cos(sqrt((gfloat)i / MAX_DEFORM_AREA_RADIUS)*G_PI)+1)*0.5,0.7); /*0.7*/
 }
 g_free(linebuffer);

}     
 
static void
iwarp_animate_dialog(GtkWidget* dlg, GtkWidget* notebook)
{ 
 GtkWidget *table;
 GtkWidget *animate_table;
 GtkWidget *button;
 GtkWidget *label;
 GtkWidget *scale;
 GtkObject *scale_data;
 

 table = gtk_table_new (2, 2, FALSE);

 gtk_container_border_width (GTK_CONTAINER (table), 5);
 gtk_table_set_row_spacings(GTK_TABLE(table), 10);
 gtk_table_set_col_spacings(GTK_TABLE(table), 3);

 button = gtk_check_button_new_with_label ("Animate");
 gtk_table_attach (GTK_TABLE (table), button, 0, 1, 0, 1, GTK_FILL , GTK_FILL , 0, 0); 
 gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) iwarp_animate_toggle,
                      &do_animate);
 gtk_widget_show(button);

 animate_table = gtk_table_new (3, 5, FALSE);
 gtk_container_border_width (GTK_CONTAINER (animate_table), 5);
 gtk_table_set_row_spacings(GTK_TABLE(animate_table), 5);
 gtk_table_set_col_spacings(GTK_TABLE(animate_table), 3);

 
 animate_frame = gtk_frame_new ("");
 gtk_frame_set_shadow_type (GTK_FRAME (animate_frame), GTK_SHADOW_ETCHED_IN);
 gtk_container_add (GTK_CONTAINER (animate_frame), animate_table);
 

 label = gtk_label_new ("Number of Frames");
 gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
 gtk_table_attach (GTK_TABLE (animate_table), label, 0, 1, 0, 1, GTK_FILL, 0, 5, 0);
 scale_data = gtk_adjustment_new (animate_num_frames, 2, MAX_NUM_FRAMES, 1.0, 1.0, 0.0);
 scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
 gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
 gtk_table_attach (GTK_TABLE (animate_table), scale, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
 gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
 gtk_scale_set_digits(GTK_SCALE(scale),0);
 gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
 gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) iwarp_iscale_update,
		      &animate_num_frames);
 gtk_widget_show (label);
 gtk_widget_show (scale);
   

 button = gtk_check_button_new_with_label ("Reverse");
 gtk_table_attach (GTK_TABLE (animate_table), button, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0); 
 gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) iwarp_toggle_update,
                      &do_animate_reverse);
 gtk_widget_show(button);
 
 button = gtk_check_button_new_with_label ("Ping Pong");
 gtk_table_attach (GTK_TABLE (animate_table), button, 0, 1, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0); 
 gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) iwarp_toggle_update,
                      &do_animate_ping_pong);
 gtk_widget_show(button);
 



  gtk_widget_show (animate_table);

  gtk_widget_show (animate_frame);
  gtk_widget_set_sensitive(animate_frame,do_animate);
  gtk_table_attach (GTK_TABLE (table), animate_frame, 0, 2, 1, 2, GTK_FILL, 0, 0, 0);
 
  label = gtk_label_new("Animate");
  gtk_misc_set_alignment(GTK_MISC(label),0.5,0.5);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,label);
  gtk_widget_show(label);
  gtk_widget_show(table);
} 
   


static void
iwarp_settings_dialog(GtkWidget* dlg, GtkWidget* notebook)
{
 GtkWidget *button;
 GtkWidget *table;
 GtkWidget *label;
 GtkWidget *scale;
 GtkWidget *toggle;
 GtkWidget *separator;
 GtkObject *scale_data;
 GtkWidget *supersample_table;  
 GSList *group = NULL;
 
 table = gtk_table_new (12, 2, FALSE);
 gtk_container_border_width (GTK_CONTAINER (table), 5);
 gtk_table_set_row_spacings(GTK_TABLE(table), 5);
 gtk_table_set_col_spacings(GTK_TABLE(table), 3);
  

 label = gtk_label_new ("Deform Radius");
 gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
 gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, 0, 5, 0);
 scale_data = gtk_adjustment_new (iwarp_vals.deform_area_radius, 5.0, MAX_DEFORM_AREA_RADIUS, 1.0, 1.0, 0.0);
 scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
 gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
 gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
 gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
 gtk_scale_set_digits(GTK_SCALE(scale),0);
 gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
 gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
	      (GtkSignalFunc) iwarp_iscale_update,
		      &iwarp_vals.deform_area_radius);
 gtk_widget_show (label);
 gtk_widget_show (scale);
 
 label = gtk_label_new ("Deform Amount");
 gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
 gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, 0, 5, 0);
 scale_data = gtk_adjustment_new (iwarp_vals.deform_amount, 0.0, 1.0, 0.01,0.01, 0.0);
 scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
 gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
 gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
 gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
 gtk_scale_set_digits(GTK_SCALE(scale),2);
 gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
 gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
	      (GtkSignalFunc) iwarp_fscale_update,
		      &iwarp_vals.deform_amount);
 gtk_widget_show (label);
 gtk_widget_show (scale);

 separator = gtk_hseparator_new();
 gtk_table_attach (GTK_TABLE(table), separator, 0, 2 ,2,3,GTK_FILL,0,0,0); 
 gtk_widget_show(separator);
    
 toggle = gtk_radio_button_new_with_label(group,"Move");
 group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
 gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 3, 4, GTK_FILL, 0, 0, 0);
 gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) iwarp_toggle_update,
		      &iwarp_vals.do_move);
 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), iwarp_vals.do_move);
 gtk_widget_show(toggle);  
 
 toggle = gtk_radio_button_new_with_label(group,"Shrink");
 group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
 gtk_table_attach (GTK_TABLE (table), toggle, 1, 2, 4, 5, GTK_FILL, 0, 0, 0);
 gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) iwarp_toggle_update,
		      &iwarp_vals.do_shrink);
 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), iwarp_vals.do_shrink);
 gtk_widget_show(toggle);  
 
 toggle = gtk_radio_button_new_with_label(group,"Grow");
 group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
 gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 4, 5, GTK_FILL, 0, 0, 0);
 gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
	      (GtkSignalFunc) iwarp_toggle_update,
		      &iwarp_vals.do_grow);
 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), iwarp_vals.do_grow);
 gtk_widget_show(toggle);  

 toggle = gtk_radio_button_new_with_label(group,"Remove");
 group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
 gtk_table_attach (GTK_TABLE (table), toggle, 1, 2, 3, 4, GTK_FILL, 0, 0, 0);
 gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) iwarp_toggle_update,
		      &iwarp_vals.do_remove);
 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), iwarp_vals.do_remove);
 gtk_widget_show(toggle);  

 toggle = gtk_radio_button_new_with_label(group,"Swirl CW");
 group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
 gtk_table_attach (GTK_TABLE (table), toggle, 1, 2, 5, 6, GTK_FILL, 0, 0, 0);
 gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) iwarp_toggle_update,
		      &iwarp_vals.do_swirl_cw);
 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), iwarp_vals.do_swirl_cw);
 gtk_widget_show(toggle);  
 
 toggle = gtk_radio_button_new_with_label(group,"Swirl CCW");
 group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
 gtk_table_attach (GTK_TABLE (table), toggle, 0, 1, 5, 6, GTK_FILL, 0, 0, 0);
 gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) iwarp_toggle_update,
		      &iwarp_vals.do_swirl_ccw);
 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), iwarp_vals.do_swirl_ccw);
 gtk_widget_show(toggle);  


 separator = gtk_hseparator_new();
 gtk_table_attach (GTK_TABLE(table), separator, 0, 2 ,6,7,GTK_FILL,0,0,0); 
 gtk_widget_show(separator);
  

 button = gtk_check_button_new_with_label ("Bilinear");
 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), iwarp_vals.do_bilinear);
 gtk_table_attach (GTK_TABLE (table), button, 1, 2, 7, 8, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0); 
 gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) iwarp_toggle_update,
                      &iwarp_vals.do_bilinear);
 gtk_widget_show(button);


 button = gtk_button_new_with_label ("Reset");
 gtk_table_attach (GTK_TABLE (table), button, 0, 1, 7, 8, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0); 
 gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) iwarp_reset_callback,
                      NULL);
 gtk_widget_show(button);

 separator = gtk_hseparator_new();
 gtk_table_attach (GTK_TABLE(table), separator, 0, 2 ,8,9,GTK_FILL,0,0,0); 
 gtk_widget_show(separator);
  

 button = gtk_check_button_new_with_label ("Adaptive Supersample");
 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), iwarp_vals.do_supersample);
 gtk_table_attach (GTK_TABLE (table), button, 0, 1, 9, 10, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0); 
 gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) iwarp_supersample_toggle,
                      &iwarp_vals.do_supersample);
 gtk_widget_show(button);

 supersample_frame = gtk_frame_new ("");
 gtk_frame_set_shadow_type (GTK_FRAME (supersample_frame), GTK_SHADOW_ETCHED_IN);
 supersample_table = gtk_table_new (2, 2, FALSE);
 gtk_container_border_width (GTK_CONTAINER (supersample_table), 5);
 gtk_table_set_row_spacings(GTK_TABLE(supersample_table), 5);
 gtk_table_set_col_spacings(GTK_TABLE(supersample_table), 3);
  
 gtk_container_add (GTK_CONTAINER (supersample_frame), supersample_table);
 
 label = gtk_label_new ("Max Depth");
 gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
 gtk_table_attach (GTK_TABLE (supersample_table), label, 0, 1, 0, 1, GTK_FILL, 0, 5, 0);
 scale_data = gtk_adjustment_new (iwarp_vals.max_supersample_depth, 1.0, 5.0, 1.0, 1.0, 0.0);
 scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
 gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
 gtk_table_attach (GTK_TABLE (supersample_table), scale, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);
 gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
 gtk_scale_set_digits(GTK_SCALE(scale),0);
 gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
 gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) iwarp_iscale_update,
		      &iwarp_vals.max_supersample_depth);
 gtk_widget_show (label);
 gtk_widget_show (scale);
   
 label = gtk_label_new ("Threshold");
 gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
 gtk_table_attach (GTK_TABLE (supersample_table), label, 0, 1, 1, 2, GTK_FILL, 0, 5, 0);
 scale_data = gtk_adjustment_new (iwarp_vals.supersample_threshold, 1.0, 10.0, 0.01,0.01, 0.0);
 scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
 gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
 gtk_table_attach (GTK_TABLE (supersample_table), scale, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);
 gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
 gtk_scale_set_digits(GTK_SCALE(scale),2);
 gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
 gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) iwarp_fscale_update,
		      &iwarp_vals.supersample_threshold);
 gtk_widget_show (label);
 gtk_widget_show (scale);


 gtk_widget_show (supersample_table);
 gtk_widget_show (supersample_frame);

 gtk_widget_set_sensitive(supersample_frame,iwarp_vals.do_supersample);

 gtk_table_attach (GTK_TABLE (table), supersample_frame, 0, 2, 11, 12, GTK_FILL, 0, 0, 0);

    
 gtk_widget_show (table);

 label = gtk_label_new("Settings");
 gtk_misc_set_alignment(GTK_MISC(label),0.5,0.5);
 gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,label);
 gtk_widget_show(label);
 gtk_widget_show(table);
}


static gint
iwarp_dialog()
{
 GtkWidget *dlg;
 GtkWidget *pframe;
 GtkWidget *top_table;
 GtkWidget *notebook;
 GtkWidget *button;
 guchar *color_cube; 
 gint argc;
 gchar** argv;
  
 
 argc = 1;
 argv = g_new(gchar *, 1);
 argv[0] = g_strdup("iwarp");

 gtk_init(&argc,&argv);
 gtk_rc_parse(gimp_gtkrc());
 gtk_preview_set_gamma (gimp_gamma ());
 gtk_preview_set_install_cmap (gimp_install_cmap ());
 color_cube = gimp_color_cube ();
 gtk_preview_set_color_cube (color_cube[0], color_cube[1],
			      color_cube[2], color_cube[3]);

 gtk_widget_set_default_visual (gtk_preview_get_visual ());
 gtk_widget_set_default_colormap (gtk_preview_get_cmap ());
 
 iwarp_init();
 
 dlg = gtk_dialog_new ();
 gtk_window_set_title (GTK_WINDOW (dlg), "IWarp");
 gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
 gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) iwarp_close_callback,
		      NULL);
 
 button = gtk_button_new_with_label ("OK");
 GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
 gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) iwarp_ok_callback,
                      dlg);
 gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
 gtk_widget_grab_default (button);
 gtk_widget_show (button);

 button = gtk_button_new_with_label ("Cancel");
 GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
 gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));

 gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
 gtk_widget_show (button);

  
 pframe = gtk_frame_new (NULL);
 gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
 gtk_widget_show (pframe); 

 if (preview_bpp == 3) preview = gtk_preview_new (GTK_PREVIEW_COLOR);
 else preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
 gtk_preview_size (GTK_PREVIEW (preview), preview_width, preview_height);
 iwarp_update_preview(0,0,preview_width,preview_height);
 gtk_container_add (GTK_CONTAINER (pframe), preview);
 gtk_widget_show (preview);
 
 gtk_widget_set_events(preview,GDK_BUTTON_PRESS_MASK |
   GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK | 
   GDK_POINTER_MOTION_HINT_MASK);
 gtk_signal_connect(GTK_OBJECT(preview), "event",
                     (GtkSignalFunc)iwarp_motion_callback,NULL);

 notebook = gtk_notebook_new();
 gtk_notebook_set_tab_pos(GTK_NOTEBOOK (notebook), GTK_POS_TOP);
 
 iwarp_settings_dialog(dlg,notebook);
 iwarp_animate_dialog(dlg,notebook);
 
 gtk_widget_show(notebook);
  
 top_table = gtk_table_new(1, 2, FALSE);
 gtk_container_border_width(GTK_CONTAINER(top_table), 6);
 gtk_table_set_row_spacings(GTK_TABLE(top_table), 5);
 gtk_table_set_col_spacings(GTK_TABLE(top_table), 3);
 gtk_table_attach(GTK_TABLE(top_table), pframe, 0,1,0,1,0,0,0,0); 
 gtk_table_attach(GTK_TABLE(top_table), notebook, 1,2,0,1,0,0,0,0);
 gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), top_table, FALSE, FALSE, 0);
 gtk_widget_show(top_table);
 gtk_widget_show(dlg);
 gtk_main ();
 gdk_flush ();

 return wint.run;   
}


static void 
iwarp_update_preview(int x0, int y0,int x1,int y1)
{
 int i;
 GdkRectangle rect;
 
 if (x0<0) x0=0;
 if (y0<0) y0=0;
 if (x1>=preview_width) x1=preview_width;
 if (y1>=preview_height) y1=preview_height;
 for (i = y0; i < y1; i++)
    gtk_preview_draw_row (GTK_PREVIEW (preview),
			  dstimage + (i * preview_width + x0) * preview_bpp,
			  x0, i,x1-x0);
 rect.x = x0;
 rect.y = y0;
 rect.width = x1-x0;
 rect.height = y1-y0;  
 gtk_widget_draw(preview,&rect);
 gdk_flush();
}


static void
iwarp_preview_get_pixel(int x, int y , guchar **color)
{
 static guchar black[4] = {0,0,0,0};

 if (x < 0 || x >= preview_width || y<0 || y >= preview_height) { 
  *color = black;
  return;
 }
 *color = srcimage +(y*preview_width+ x)*image_bpp; 
} 

 

static void
iwarp_preview_get_point( gfloat x, gfloat y,guchar* color)
{
 int xi,yi,j;
 gfloat dx,dy,m0,m1;
 guchar *p0,*p1,*p2,*p3;

  xi = (int)x;
  yi = (int)y;
  if (iwarp_vals.do_bilinear) {
   dx = x-xi;
   dy = y-yi; 
   iwarp_preview_get_pixel(xi,yi,&p0);
   iwarp_preview_get_pixel(xi+1,yi,&p1);
   iwarp_preview_get_pixel(xi,yi+1,&p2);
   iwarp_preview_get_pixel(xi+1,yi+1,&p3);
   for (j=0; j< image_bpp; j++) {
      m0 = p0[j] +dx * (p1[j]-p0[j]);
      m1 = p2[j]+ dx * (p3[j]-p2[j]);
     color[j] = (guchar)(m0 + dy * (m1-m0));  
   }
  }
  else {
   iwarp_preview_get_pixel(xi,yi,&p0);
   for (j=0; j < image_bpp; j++) color[j] = p0[j];
  }
  
}


static void 
iwarp_deform(int x, int y, gfloat vx, gfloat vy)
{
 int xi,yi,ptr,fptr,x0,x1,y0,y1,radius2,length2;
 gfloat deform_value,xn,yn,nvx=0,nvy=0,emh,em,edge_width,xv,yv,alpha;
 guchar color[4];
 
 if (x - iwarp_vals.deform_area_radius <0) x0 = -x; else x0 = -iwarp_vals.deform_area_radius;
 if (x + iwarp_vals.deform_area_radius >= preview_width) x1 = preview_width-x-1; else x1 = iwarp_vals.deform_area_radius;
 if (y - iwarp_vals.deform_area_radius <0) y0 = -y; else y0 = -iwarp_vals.deform_area_radius;
 if (y + iwarp_vals.deform_area_radius >= preview_height) y1 = preview_height-y-1; else y1 = iwarp_vals.deform_area_radius;

 radius2 = iwarp_vals.deform_area_radius*iwarp_vals.deform_area_radius;
 for (yi= y0; yi <= y1; yi++)
  for (xi = x0; xi <= x1; xi++) {
   length2 = (xi*xi+yi*yi)*MAX_DEFORM_AREA_RADIUS / radius2;
   if (length2 < MAX_DEFORM_AREA_RADIUS) {
    ptr = (y + yi) * preview_width + x + xi;   
    fptr = (yi+iwarp_vals.deform_area_radius) * (iwarp_vals.deform_area_radius*2+1) + xi+iwarp_vals.deform_area_radius;
   
    if (iwarp_vals.do_grow) { 
      deform_value = filter[length2] *  0.1* iwarp_vals.deform_amount; 
      nvx = -deform_value * xi; 
      nvy = -deform_value * yi; 
    }  
    else if (iwarp_vals.do_shrink) { 
      deform_value = filter[length2] * 0.1* iwarp_vals.deform_amount; 
      nvx = deform_value * xi; 
      nvy = deform_value * yi; 
    }
    else if (iwarp_vals.do_swirl_cw) { 
      deform_value = filter[length2] * iwarp_vals.deform_amount * 0.5; 
      nvx =  deform_value * yi; 
      nvy = -deform_value * xi; 
    }
    else if (iwarp_vals.do_swirl_ccw) { 
      deform_value = filter[length2] *iwarp_vals.deform_amount * 0.5; 
      nvx = -deform_value * yi; 
      nvy =  deform_value  * xi; 
    }
    else if (iwarp_vals.do_move) { 
      deform_value = filter[length2] * iwarp_vals.deform_amount; 
      nvx = deform_value * vx; 
      nvy = deform_value * vy; 
    } 
    if (iwarp_vals.do_remove) {
     deform_value = 1.0-0.5*iwarp_vals.deform_amount*filter[length2];
     deform_area_vectors[fptr].x = deform_value * deform_vectors[ptr].x ;
     deform_area_vectors[fptr].y = deform_value * deform_vectors[ptr].y ;
    } 
    else {
     edge_width = 0.2 *  iwarp_vals.deform_area_radius;
     emh = em = 1.0;
     if (x+xi < edge_width) em = (gfloat)(x+xi) / (edge_width);
     if (y+yi < edge_width) emh = (gfloat)(y+yi) / (edge_width);
     if (emh <em) em = emh;
     if (preview_width-x-xi-1 < edge_width ) emh = (gfloat)(preview_width-x-xi-1)/ (edge_width);
     if (emh <em) em = emh;
     if (preview_height-y-yi-1 < edge_width ) emh = (gfloat)(preview_height-y-yi-1)/ (edge_width);
     if (emh <em) em = emh;        
     nvx = nvx*em;
     nvy = nvy*em;

     iwarp_get_deform_vector(nvx + x+ xi,nvy + y +yi, &xv , &yv);
     xv = nvx +xv;
     if (xv +x+xi <0.0) xv = -x-xi;
     else if (xv + x +xi > (preview_width-1)) xv = preview_width - x -xi-1;
     yv = nvy +yv;   
     if (yv +y+yi <0.0) yv = -y-yi;
     else if (yv + y +yi > (preview_height-1)) yv = preview_height - y -yi-1;
     deform_area_vectors[fptr].x =xv;
     deform_area_vectors[fptr].y = yv; 
    } 

    xn = deform_area_vectors[fptr].x + x + xi;
    yn = deform_area_vectors[fptr].y + y + yi;
    iwarp_preview_get_point(xn,yn,color);

    if (preview_bpp == 3) { 
     if (image_bpp == 4) {
       alpha = (gfloat)color[3] / 255;
       dstimage[ptr*3] = (guchar)(alpha*color[0]+ (1.0-alpha)*iwarp_transparent_color(x+xi,y+yi));
       dstimage[ptr*3+1] = (guchar)(alpha*color[1]+ (1.0-alpha)*iwarp_transparent_color(x+xi,y+yi));
       dstimage[ptr*3+2] = (guchar)(alpha*color[2]+ (1.0-alpha)*iwarp_transparent_color(x+xi,y+yi));
     }
     else {
       dstimage[ptr*3] = color[0];
       dstimage[ptr*3+1] = color[1];
       dstimage[ptr*3+2] = color[2];
     }
    }
    else {
     if (image_bpp == 2) { 
      alpha = (gfloat)color[1] / 255;
      dstimage[ptr] = (guchar)(alpha *color[0]+ (1.0-alpha)*iwarp_transparent_color(x+xi,y+yi)) ;
     }
     else
      dstimage[ptr] = color[0];
    }
   }
  } 
   
 for (yi= y0; yi <= y1; yi++)
  for (xi = x0; xi <= x1; xi++) {
   length2 = (xi*xi+yi*yi)*MAX_DEFORM_AREA_RADIUS / radius2;
   if (length2 <MAX_DEFORM_AREA_RADIUS) {
    ptr = (yi +y) * preview_width + xi +x;
    fptr = (yi+iwarp_vals.deform_area_radius) * (iwarp_vals.deform_area_radius*2+1) + xi + iwarp_vals.deform_area_radius;
    deform_vectors[ptr].x = deform_area_vectors[fptr].x;
    deform_vectors[ptr].y = deform_area_vectors[fptr].y;
   }
  }  
 iwarp_update_preview(x+x0,y+y0,x+x1+1,y+y1+1);   
}  
    

static void
iwarp_move(int x, int y,int xx, int yy)
{
 gfloat l,dx,dy,xf,yf;
 int num,i,x0,y0; 
 
 dx = x-xx;
 dy = y-yy;
 l= sqrt(dx*dx+dy*dy);
 num = (int)(l * 2 / iwarp_vals.deform_area_radius)+1;
 dx /= num;
 dy /= num;
 xf = xx+dx; yf = yy+dy; 
 for (i=0; i< num; i++) {
  x0 = (int)(xf);
  y0 = (int)(yf);
  iwarp_deform(x0,y0,-dx,-dy);
  xf += dx;
  yf += dy;
 }
}
  


static void
iwarp_close_callback (GtkWidget *widget,
		       gpointer   data)
{
  gtk_main_quit ();
}

static void
iwarp_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  wint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}


static gint
iwarp_motion_callback(GtkWidget *widget,
                      GdkEvent *event)
{
 GdkEventButton* mb;
 int x,y;
 
 mb = (GdkEventButton*) event; 
 switch (event->type) {
   case GDK_BUTTON_PRESS:
      lastx = mb->x;
      lasty = mb->y;
   break;
   case GDK_BUTTON_RELEASE:
     if (mb->state & GDK_BUTTON1_MASK) {
       x = mb->x;
       y = mb->y;
       if (iwarp_vals.do_move) iwarp_move(x,y,lastx,lasty);
       else iwarp_deform(x, y,0.0,0.0);
     }
   break; 
   case GDK_MOTION_NOTIFY :
     if (mb->state & GDK_BUTTON1_MASK) { 
       x = mb->x;
       y = mb->y;
       if (iwarp_vals.do_move) iwarp_move(x,y,lastx,lasty);
       else iwarp_deform(x, y,0.0,0.0);
       lastx = x;
       lasty = y;
       gtk_widget_get_pointer(widget,NULL,NULL); 
     }
   break;
   default:
   break;
 }
 
return FALSE;
}


static void    
iwarp_iscale_update (GtkAdjustment *adjustment, gint* scale_val)
{
 *scale_val = (gint)adjustment->value;
}

static void    
iwarp_fscale_update (GtkAdjustment *adjustment, gfloat* scale_val)
{
 *scale_val = adjustment->value;
}
                                      
                                      

static void    
iwarp_toggle_update (GtkWidget *widget,
                     int  *data)
{
 if ( GTK_TOGGLE_BUTTON (widget)->active) *data = TRUE;
 else *data = FALSE;
}

static void    
iwarp_supersample_toggle (GtkWidget *widget,
                     int  *data)
{
 if ( GTK_TOGGLE_BUTTON (widget)->active) *data = TRUE;
 else *data = FALSE;
 gtk_widget_set_sensitive(supersample_frame,iwarp_vals.do_supersample);
}

static void    
iwarp_animate_toggle (GtkWidget *widget,
                     int  *data)
{
 if ( GTK_TOGGLE_BUTTON (widget)->active) *data = TRUE;
 else *data = FALSE;
 gtk_widget_set_sensitive(animate_frame,do_animate);
}


static void
iwarp_reset_callback (GtkWidget *widget,
		    gpointer   data)
{
 int i;

 iwarp_cpy_images();
 for (i=0; i<preview_width* preview_height; i++)  deform_vectors[i].x = deform_vectors[i].y = 0.0;
 iwarp_update_preview(0,0,preview_width, preview_height);   
}
