#include "rcm.h"

extern RcmParams Current;

float arctg(float y, float x)
{
  float temp=atan2(y,x);
  return (temp<0)?(temp+TP):temp;
}

float sign(float x)
{
  return (x<0)?(-1):(1);
}

float rcm_linear(float A, 
		 float B, 
		 float C, 
		 float D, 
		 float x)
{
  if (B > A)
    if (A<=x && x<=B)
      return C+(D-C)/(B-A)*(x-A);
    else if (A<=x+TP && x+TP<=B)
      return C+(D-C)/(B-A)*(x+TP-A);    
    else
      return x;
  else 
    if (B<=x && x<=A)
      return C+(D-C)/(B-A)*(x-A);
    else if (B<=x+TP && x+TP<=A)
      return C+(D-C)/(B-A)*(x+TP-A);    
    else
      return x;
}

float rcm_left_end(RcmAngle *angle)
{
  gfloat alpha  = angle->alpha;
  gfloat beta   = angle->beta;
  gint   cw_ccw = angle->cw_ccw;
  
  switch (cw_ccw) {

  case (-1): if (alpha < beta) return alpha + TP;

  case 1:  default: return alpha;
  }
}

float rcm_right_end(RcmAngle *angle)
{
  gfloat alpha  = angle->alpha;
  gfloat beta   = angle->beta;
  gint   cw_ccw = angle->cw_ccw;
  
  switch (cw_ccw) {

  case 1: if (beta < alpha) return beta + TP;

  case (-1): default: return beta;
  }
}

float min_prox(float alpha, 
	       float beta, 
	       float angle)
{
  gfloat temp1 = MIN(angle_mod_2PI(alpha - angle),
		   TP-angle_mod_2PI(alpha - angle));
  gfloat temp2 = MIN(angle_mod_2PI(beta - angle),
		   TP-angle_mod_2PI(beta - angle));
  return MIN(temp1,temp2);
}

float *closest(float *alpha, float *beta, float angle)
{
  float temp_alpha = MIN(angle_mod_2PI(*alpha-angle),
			 TP-angle_mod_2PI(*alpha-angle));

  float temp_beta  = MIN(angle_mod_2PI(*beta -angle),
			 TP-angle_mod_2PI(*beta -angle));

  if (temp_alpha-temp_beta<0) 
    return alpha;
  else 
    return beta;
}

float angle_mod_2PI(float angle)
{
  if (angle < 0) 
    return angle + TP;
  else if (angle > TP)
    return angle - TP;
  else
    return angle;
}

float beta_from_0_to_2PI(float alpha, float delta)
{
  float beta = alpha + delta;
  if (delta > 0)
    return (beta < 2*PI) ? beta :  beta - 2*PI;
  else
    return (beta > 0)    ? beta :  beta + 2*PI;
}

float 
angle_inside_slice(float angle, 
		   RcmAngle *slice)
{
  return angle_mod_2PI(slice->cw_ccw*(slice->beta-angle))/
         angle_mod_2PI(slice->cw_ccw*(slice->beta-slice->alpha));
}

float 
rcm_units_factor(gint units)
{
  switch (units) {
  case DEGREES:         return 180.0/PI;
  case RADIANS:         return 1.0;
  case RADIANS_OVER_PI: return 1.0/PI;
  default:              return -1;
  }
}
gchar *
rcm_units_string(gint units)
{
  switch (units) {
  case DEGREES:         return " °";
  case RADIANS:         return " Rad";
  case RADIANS_OVER_PI: return " \227";
  default:              return "Error";
  }
}

gint 
rcm_is_gray(float s)
{
  if (s <= Current.Gray->gray_sat) 
    return 1;
  else 
    return 0;
}

void 
rcm_update_entries(gint units)
{
  Current.From->action_flag = DO_NOTHING;
  Current.To->action_flag   = DO_NOTHING;
  Current.Gray->action_flag = DO_NOTHING;
  
  rcm_float_in_an_entry(Current.From->alpha_entry,
			Current.From->angle->alpha*
			rcm_units_factor(Current.Units));
	
  rcm_float_in_an_entry(Current.From->beta_entry,
			Current.From->angle->beta*
			rcm_units_factor(Current.Units));
			  
  
  rcm_float_in_an_entry(Current.To->alpha_entry,
			Current.To->angle->alpha*
			rcm_units_factor(Current.Units));

  rcm_float_in_an_entry(Current.To->beta_entry,
			Current.To->angle->beta*
			rcm_units_factor(Current.Units));

  rcm_float_in_an_entry(Current.Gray->hue_entry,
			      Current.Gray->hue*
			      rcm_units_factor(Current.Units));

  gtk_label_set(GTK_LABEL(Current.From->alpha_units_label),
		rcm_units_string(Current.Units));
  gtk_label_set(GTK_LABEL(Current.From->beta_units_label),
		rcm_units_string(Current.Units));
  gtk_label_set(GTK_LABEL(Current.To->alpha_units_label),
		rcm_units_string(Current.Units));
  gtk_label_set(GTK_LABEL(Current.To->beta_units_label),
		rcm_units_string(Current.Units));
  gtk_label_set(GTK_LABEL(Current.Gray->hue_units_label),
		rcm_units_string(Current.Units));

  Current.From->action_flag = VIRGIN;
  Current.To->action_flag   = VIRGIN;
  Current.Gray->action_flag = VIRGIN;
}

ReducedImage  *
Reduce_The_Image(GDrawable *drawable,
		 GDrawable *mask,
		 gint LongerSize, 
		 gint Slctn)
{
  gint RH, RW, width, height, bytes=drawable->bpp;  
  ReducedImage *temp = g_new(ReducedImage, 1);
  guchar *tempRGB, *src_row, *tempmask, *src_mask_row,R,G,B;
  gint i, j, whichcol, whichrow, x1, x2, y1, y2;
  GPixelRgn srcPR, srcMask;
  gint NoSelectionMade=TRUE;
  hsv *tempHSV, H, S, V;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  width  = x2-x1;
  height = y2-y1;

  if (width != drawable->width && height != drawable->height) 
    NoSelectionMade=FALSE;

  if (Slctn==ENTIRE_IMAGE) {
    x1=0;
    x2=drawable->width;
    y1=0;
    y2=drawable->height;
  }  

  if (Slctn==SELECTION_IN_CONTEXT) {
    x1=MAX(0,                x1-width/2.0);
    x2=MIN(drawable->width,  x2+width/2.0);
    y1=MAX(0,                y1-height/2.0);
    y2=MIN(drawable->height, y2+height/2.0);
  }  
  
  width  = x2-x1;
  height = y2-y1;
  
  if (width>height) {
    RW=LongerSize;
    RH=(float) height * (float) LongerSize/ (float) width;
  }
  else {
    RH=LongerSize;
    RW=(float)width * (float) LongerSize/ (float) height;
  }
  
  tempRGB   = (guchar *) malloc(RW*RH*bytes);
  tempHSV   = (hsv *)    malloc(RW*RH*bytes*sizeof(hsv));
  tempmask  = (guchar *) malloc(RW*RH);

  gimp_pixel_rgn_init (&srcPR, drawable, x1, y1, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&srcMask, mask, x1, y1, width, height, FALSE, FALSE);
  
  src_row = (guchar *) malloc (width*bytes);
  src_mask_row = (guchar *) malloc (width*bytes);

  for (i=0; i<RH; i++) {
    whichrow=(float)i*(float)height/(float)RH;
    gimp_pixel_rgn_get_row (&srcPR, src_row, x1, y1+whichrow, width);   
    gimp_pixel_rgn_get_row (&srcMask, src_mask_row, x1, y1+whichrow, width);
   
    for (j=0; j<RW; j++) {
      whichcol=(float)j*(float)width/(float)RW;

      if (NoSelectionMade)
	tempmask[i*RW+j]=255;
      else
	tempmask[i*RW+j]=src_mask_row[whichcol];

      R=src_row[whichcol*bytes+0];
      G=src_row[whichcol*bytes+1];
      B=src_row[whichcol*bytes+2];
      
      rgb_to_hsv(R/255.0,G/255.0,B/255.0,&H,&S,&V);

      tempRGB[i*RW*bytes+j*bytes+0]=R;
      tempRGB[i*RW*bytes+j*bytes+1]=G;
      tempRGB[i*RW*bytes+j*bytes+2]=B;

      tempHSV[i*RW*bytes+j*bytes+0]=H;
      tempHSV[i*RW*bytes+j*bytes+1]=S;
      tempHSV[i*RW*bytes+j*bytes+2]=V;

      if (bytes==4)
	tempRGB[i*RW*bytes+j*bytes+3]=src_row[whichcol*bytes+3];
	     
    }
  }
  temp->width=RW;
  temp->height=RH;
  temp->rgb=tempRGB;
  temp->hsv=tempHSV;
  temp->mask=tempmask;  
  return temp;
}

void 
rcm_render_circle_preview(GtkWidget  *preview,
			  int        sum,
			  int        margin)
{ 
  gint i, j;
  hsv h, s, v, r, g, b;
  guchar *a =(guchar *) malloc(3*sum);
  
  if (preview==NULL) return;
  
  for (j=0; j<sum; j++) {
    for (i=0; i<sum; i++) {
      if (( s=sqrt((sqr(i-sum/2.0) + 
		   sqr(j-sum/2.0))/
		  (float)sqr(sum/2.0-margin)))>1) {
	a[i*3+0] = 255;
	a[i*3+1] = 255;
	a[i*3+2] = 255;
      }
      else {
	h=arctg(sum/2.0-j,i-sum/2.0)/(2*PI);
	v=1-sqrt(s)/2;
	hsv_to_rgb(h,s,v, &r, &g, &b);
	a[i*3+0] = r*255;
	a[i*3+1] = g*255;
	a[i*3+2] = b*255;
      }	
    }   
    gtk_preview_draw_row( GTK_PREVIEW(preview),a,0,j,sum);
  }
  
  free(a); 
  gtk_widget_draw(preview,NULL);
  gdk_flush();
}

void 
rcm_render_preview(GtkWidget     *preview, 
		   gint          version)
{ 
  guchar *a;
  ReducedImage *reduced = Current.reduced;
  gint RW = reduced->width;
  gint RH = reduced->height;
  gint bytes = Current.drawable->bpp;
  gint i, j, k, unchanged, skip;
  hsv H, S, V, R, G, B;
  hsv *hsv_array = reduced->hsv;
  guchar *rgb_array = reduced->rgb;
  float degree;

  a =(guchar *) malloc(bytes*RW);
  
  if (preview==NULL) {
    printf("Asked to preview a NULL! Shouldn't happen!\n");
    return;
  }

  if (version==CURRENT) 
    for (i=0; i<RH; i++) {
      for (j=0; j<RW; j++) {
	
	unchanged = 1; skip = 0;
	
	H  =  hsv_array[i*RW*bytes + j*bytes + 0];
	S  =  hsv_array[i*RW*bytes + j*bytes + 1];
	V  =  hsv_array[i*RW*bytes + j*bytes + 2];
	
	
	if (rcm_is_gray(S)&&reduced->mask[i*RW+j]!=0) 
	  switch (Current.Gray_to_from) {
	  case GRAY_FROM:
	    if (angle_inside_slice(Current.Gray->hue, 
				   Current.From->angle)<=1) 
	      {
		H = Current.Gray->hue/TP;
		S = Current.Gray->satur;
	      }
	    else 
	      skip = 1;
	    break;
	    
	  case GRAY_TO:  
	    unchanged = 0; skip = 1;
	    hsv_to_rgb(Current.Gray->hue/TP,
		       Current.Gray->satur,
		       V,
		       &R,&G,&B);
	    break;
	    
	  default: break;
	  }
	
	
	if (!skip) {
	  unchanged = 0;
	  H = rcm_linear(rcm_left_end(Current.From->angle),
			 rcm_right_end(Current.From->angle),
			 rcm_left_end(Current.To->angle),
			 rcm_right_end(Current.To->angle),
			 H*TP);    
	  H = angle_mod_2PI(H)/TP;
	  hsv_to_rgb(H,S,V,&R,&G,&B);
	}
	
	if (unchanged)
	  degree=0;
	else
	  degree=reduced->mask[i*RW+j]/255.0;
	
	  a[j*3+0]=(1-degree)*rgb_array[i*RW*bytes+j*bytes+0]+degree*R*255;
	  a[j*3+1]=(1-degree)*rgb_array[i*RW*bytes+j*bytes+1]+degree*G*255;
	  a[j*3+2]=(1-degree)*rgb_array[i*RW*bytes+j*bytes+2]+degree*B*255;
	
	if (bytes==4) 
	  for (k=0; k<3; k++) {
	    float transp=reduced->mask[i*RW*bytes+j*bytes+3]/255.0;
	    a[3*j+k]=transp*a[3*j+k]+(1-transp)*rcm_fake_transparency(i,j);
	  }
      }
      gtk_preview_draw_row( GTK_PREVIEW(preview),a,0,i,RW);
    }
  else
    for (i = 0;  i < RH;  i++) {
      for (j = 0;  j < RW;  j++) {
	  a[j*3+0] = rgb_array[i*RW*bytes + j*bytes + 0];
	  a[j*3+1] = rgb_array[i*RW*bytes + j*bytes + 1];
	  a[j*3+2] = rgb_array[i*RW*bytes + j*bytes + 2];
      
      if (bytes==4) 
	for (k=0; k<3; k++) {
	  float transp=rgb_array[i*RW*bytes+j*bytes+3]/255.0;
	  a[3*j+k]=transp*a[3*j+k]+(1-transp)*rcm_fake_transparency(i,j);
	}
      }
      gtk_preview_draw_row( GTK_PREVIEW(preview),a,0,i,RW);
    } 
  free(a); 
  gtk_widget_draw(preview,NULL);
  gdk_flush();
}

gint rcm_fake_transparency(gint i, gint j)
{
  if ( ((i%20)- 10) * ((j%20)- 10)>0   )
    return 64;
  else 
    return 196;
}

