#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpcolorspace.h"
#include "fp.h"

extern FP_Params Current;

extern GDrawable *drawable, *mask;
extern ReducedImage *reduced;

extern gint nudgeArray[256];

gint colorSign[3][ALL_PRIMARY]=
{{1,-1,-1,-1,1,1},{-1,1,-1,1,1,-1},{-1,-1,1,1,-1,1}};


void initializeFilterPacks()
{
  gint i, j;
  for (i=0; i<256; i++) 
    for (j=BY_HUE; j<JUDGE_BY; j++) {
      Current.redAdj   [j][i]=0;
      Current.greenAdj [j][i]=0;
      Current.blueAdj  [j][i]=0;
      Current.satAdj   [j][i]=0;
    }
}

void resetFilterPacks()
{
  initializeFilterPacks();
  refreshPreviews(Current.VisibleFrames);  
}

ReducedImage *Reduce_The_Image(GDrawable *drawable,
			       GDrawable *mask,
			       gint LongerSize, 
			       gint Slctn)
{
  gint RH, RW, width, height, bytes=drawable->bpp;  
  ReducedImage *temp=(ReducedImage *)malloc(sizeof(ReducedImage));
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

  if (Slctn==0) {
    x1=0;
    x2=drawable->width;
    y1=0;
    y2=drawable->height;
  }  

  if (Slctn==2) {
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
      
      H = R/255.0;
      S = G/255.0;
      V = B/255.0;
      gimp_rgb_to_hsv_double(&H,&S,&V);

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

/*************************************************************/
/************** The Preview Function *************************/
void 
fp_render_preview(GtkWidget     *preview, 
		  gint          changewhat,
		  gint          changewhich)
{ 
  guchar *a;
  gint Inten, bytes=drawable->bpp;
  gint i, j, k, nudge, M, m, middle,JudgeBy;
  float partial;
  gint RW=reduced->width;
  gint RH=reduced->height;
  gint backupP[3], P[3], tempSat[JUDGE_BY][256];

  a =(guchar *) malloc(bytes*RW);
   
  if (changewhat==SATURATION)
    for (k=0; k<256; k++) {
      for (JudgeBy=BY_HUE; JudgeBy<JUDGE_BY; JudgeBy++)
	tempSat[JudgeBy][k]=0; 
	tempSat[Current.ValueBy][k] += changewhich*nudgeArray[(k+Current.Offset)%256];
    }
	  
  for (i=0; i<RH; i++) {
    for (j=0; j<RW; j++) {
      
      backupP[0] = P[0]  = (int) reduced->rgb[i*RW*bytes + j*bytes + 0];
      backupP[1] = P[1]  = (int) reduced->rgb[i*RW*bytes + j*bytes + 1];
      backupP[2] = P[2]  = (int) reduced->rgb[i*RW*bytes + j*bytes + 2];
      
      m = MIN(MIN(P[0],P[1]),P[2]);
      M = MAX(MAX(P[0],P[1]),P[2]);
      middle=(M+m)/2;
      for (k=0; k<3; k++)
	if (P[k]!=m && P[k]!=M) middle=P[k];
      
      partial = reduced->mask[i*RW+j]/255.0;
      
      for (JudgeBy=BY_HUE; JudgeBy<JUDGE_BY; JudgeBy++) {
	if (!Current.Touched[JudgeBy]) continue;
	
	Inten   = reduced->hsv[i*RW*bytes + j*bytes + JudgeBy]*255.0;
	
	/*DO SATURATION FIRST*/
	if (changewhat != NONEATALL) {
	  if (M!=m) {
	    for (k=0; k<3; k++)
	      if (backupP[k] == M)
		P[k] = MAX(P[k]+partial*Current.satAdj[JudgeBy][Inten],middle);
	      else if (backupP[k] == m)
		P[k] = MIN(P[k]-partial*Current.satAdj[JudgeBy][Inten],middle); 
	  }
	  P[0]  += partial*Current.redAdj[JudgeBy][Inten];
	  P[1]  += partial*Current.greenAdj[JudgeBy][Inten];
	  P[2]  += partial*Current.blueAdj[JudgeBy][Inten];
	}
      }
      
      Inten   = reduced->hsv[i*RW*bytes + j*bytes + Current.ValueBy]*255.0;
      nudge   = partial*nudgeArray[(Inten+Current.Offset)%256];  

      switch (changewhat) {
      case HUE:
	P[0]  += colorSign[RED][changewhich]   * nudge;
	P[1]  += colorSign[GREEN][changewhich] * nudge; 
	P[2]  += colorSign[BLUE][changewhich]  * nudge;
	break;
	
      case SATURATION:
	for (JudgeBy=BY_HUE; JudgeBy<JUDGE_BY; JudgeBy++)
	  for (k=0; k<3; k++)
	    if (M!=m) {
	      if (backupP[k] == M)
		P[k] = MAX(P[k]+
			   partial*tempSat[JudgeBy][Inten],middle);
	      else if (backupP[k] == m)
		P[k] = MIN(P[k]-
			   partial*tempSat[JudgeBy][Inten],middle);
	    }
	break;

      case VALUE:
	P[0]  += changewhich  * nudge;
	P[1]  += changewhich  * nudge;
	P[2]  += changewhich  * nudge;
	break;

      default: 
	break;
      }
      
      a[j*3+0] = MAX(0,MIN(P[0], 255));
      a[j*3+1] = MAX(0,MIN(P[1], 255));
      a[j*3+2] = MAX(0,MIN(P[2], 255));

      if (bytes==4) 
	for (k=0; k<3; k++) {
	  float transp=reduced->rgb[i*RW*bytes+j*bytes+3]/255.0;
	  a[3*j+k]=transp*a[3*j+k]+(1-transp)*fp_fake_transparency(i,j);
	}
    }
    gtk_preview_draw_row( GTK_PREVIEW(preview),a,0,i,RW);
  }
  
  free(a); 
  gtk_widget_draw(preview,NULL);
  gdk_flush();
}

void      
Update_Current_FP     (gint   changewhat,
		       gint   changewhich)
{
  int i, nudge;
  
  for (i=0; i<256; i++) {
    
    fp_Create_Nudge(nudgeArray);
    nudge=nudgeArray[(i+Current.Offset)%256];
    
    switch (changewhat) {
    case HUE:
      Current.redAdj[Current.ValueBy][i]   += 
	colorSign[RED][changewhich] * nudge;
      
      Current.greenAdj[Current.ValueBy][i] += 
	colorSign[GREEN][changewhich] * nudge;
      
      Current.blueAdj[Current.ValueBy][i]  += 
	colorSign[BLUE][changewhich]  * nudge;
      break;
      
    case SATURATION:
      Current.satAdj[Current.ValueBy][i] += changewhich*nudge;
      break;
      
    case VALUE:
      Current.redAdj[Current.ValueBy][i]   += changewhich * nudge;
      Current.greenAdj[Current.ValueBy][i] += changewhich * nudge;
      Current.blueAdj[Current.ValueBy][i]  += changewhich * nudge;
      break;
      
    default:
      break;
    } /* switch */
    
  } /* for */
}


void
fp_create_smoothness_graph        (GtkWidget *preview)
{
  guchar data[256*3];
  gint nArray[256];
  int i, j, toBeBlack;

  fp_Create_Nudge(nArray);

  for (i=0; i<MAX_ROUGHNESS; i++)
    {
      int coor=MAX_ROUGHNESS-i;
      for (j=0; j<256; j++) {
	data[3*j+0]=255;
	data[3*j+1]=255;
	data[3*j+2]=255;
	if  (!(i%(MAX_ROUGHNESS/4))) {
	  data[3*j+0]=255;
	  data[3*j+1]=128;
	  data[3*j+2]=128;
	}
	if  (!((j+1)%32)) {
	  data[3*j+0]=255;
	  data[3*j+1]=128;
	  data[3*j+2]=128;
	}
	toBeBlack=0;
	if (nArray[j]==coor) toBeBlack=1;

	if (j<255) {
	  int jump=abs(nArray[j]-nArray[j+1]);
	  if (   abs(coor-nArray[j])   < jump 
	      && abs(coor-nArray[j+1]) < jump)
	    toBeBlack=1;
	}
	if (toBeBlack) {
	  data[3*j+0]=0;
	  data[3*j+1]=0;
	  data[3*j+2]=0;
	}  
      }
      gtk_preview_draw_row( GTK_PREVIEW(preview),data,0,i,256);
    }
  gtk_widget_draw(preview,NULL);
  gdk_flush();
}

void
fp_range_preview_spill(GtkWidget *preview, int type)
{
  guchar data[256*3];
  int i, j;
  hsv R,G,B;
  
  for (i=0; i<RANGE_HEIGHT; i++) {
    for (j=0; j<256; j++)
      if  (!((j+1)%32)) {
	data[3*j+0]=255;
	data[3*j+1]=128;
	data[3*j+2]=128;
      }
      else
	switch (type) {
	case BY_VAL:
	  data[3*j+0]=j-Current.Offset;
	  data[3*j+1]=j-Current.Offset;
	  data[3*j+2]=j-Current.Offset;
	  break;
	case BY_HUE:
	  R = (hsv)((j-Current.Offset+256)%256)/255.0;
	  G = 1.0;
	  B = .5;
	  gimp_hsv_to_rgb_double(&R, &G, &B);
	  data[3*j+0]=R*255;
	  data[3*j+1]=G*255;
	  data[3*j+2]=B*255;
	  break;
	case BY_SAT:
	  R = .5;
	  G = (hsv)((j-(gint)Current.Offset+256)%256)/255.0;
	  B = .5;
	  gimp_hsv_to_rgb_double(&R,&G,&B);
	  data[3*j+0]=R*255;
	  data[3*j+1]=G*255;
	  data[3*j+2]=B*255;
	  break;
	}
    gtk_preview_draw_row( GTK_PREVIEW(preview),data,0,i,256);
  }
  gtk_widget_draw(preview,NULL);
  gdk_flush();
}


void  fp_Create_Nudge(gint  *adjArray)
{
  int left, right, middle,i;
  /* The following function was determined by trial and error */
  double Steepness=pow(1-Current.Alias,4)*.8; 

  left = (Current.Range == SHADOWS) ? 0 : Current.Cutoffs[Current.Range-1];
  right = Current.Cutoffs[Current.Range];
  middle = (left + right)/2; 

  if (Current.Alias!=0)
    for (i=0; i<256; i++)
      if (i<=middle)
	adjArray[i] = MAX_ROUGHNESS * 
	  Current.Rough*(1+tanh(Steepness*(i-left)))/2;
      else
	adjArray[i] = MAX_ROUGHNESS * 
	  Current.Rough*(1+tanh(Steepness*(right-i)))/2;
  else
     for (i=0; i<256; i++)
       if (left<=i && i<=right)
	 adjArray[i] = MAX_ROUGHNESS * Current.Rough;
       else
	 adjArray[i] = 0;
}

gint fp_fake_transparency(gint i, gint j)
{
  if ( ((i%20)- 10) * ((j%20)- 10)>0   )
    return 64;
  else 
    return 196;
}
