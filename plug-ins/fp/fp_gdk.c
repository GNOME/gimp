#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "fp.h"
#include "fp_hsv.h"

extern AdvancedWindow AW;
extern FP_Params Current;


extern gint nudgeArray[256];


void
slider_erase (GdkWindow *window,
	      int        xpos)
{
  gdk_window_clear_area (window, MARGIN + xpos - (RANGE_HEIGHT - 1) / 2, 0,
			 RANGE_HEIGHT, RANGE_HEIGHT);
}

void
draw_slider(GdkWindow *window,
	    GdkGC     *border_gc,
	    GdkGC     *fill_gc,
	    int        xpos)
{
  int i;
  for (i = 0; i < RANGE_HEIGHT; i++)
    gdk_draw_line(window, fill_gc, MARGIN + xpos-i/2, i, MARGIN + xpos+i/2,i);
  
  gdk_draw_line(window, border_gc, MARGIN + xpos, 0,
		MARGIN + xpos - (RANGE_HEIGHT - 1) / 2, RANGE_HEIGHT - 1);

  gdk_draw_line(window, border_gc, MARGIN + xpos, 0,
		MARGIN + xpos + (RANGE_HEIGHT - 1) / 2, RANGE_HEIGHT - 1);

  gdk_draw_line(window, border_gc, MARGIN + xpos-(RANGE_HEIGHT-1)/2,RANGE_HEIGHT-1,
		MARGIN + xpos + (RANGE_HEIGHT-1)/2, RANGE_HEIGHT-1);
}


void
draw_it(GtkWidget *widget)
{
  draw_slider(AW.aliasingGraph->window,
	      AW.aliasingGraph->style->black_gc,
	      AW.aliasingGraph->style->dark_gc[GTK_STATE_NORMAL],
	      Current.Cutoffs[SHADOWS]);

  draw_slider(AW.aliasingGraph->window,
	      AW.aliasingGraph->style->black_gc,
	      AW.aliasingGraph->style->dark_gc[GTK_STATE_NORMAL],
	      Current.Cutoffs[MIDTONES]);

  draw_slider(AW.aliasingGraph->window,
	      AW.aliasingGraph->style->black_gc,
	      AW.aliasingGraph->style->dark_gc[GTK_STATE_SELECTED],
	      Current.Offset);
} 

gint     
FP_Range_Change_Events  (GtkWidget *widget, 
			 GdkEvent  *event, 
			 FP_Params *current)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint shad, mid, offset, min;
  static guchar  *new;
  gint  x;

  switch(event->type) {

  case GDK_EXPOSE:
    draw_it(NULL);
    break;
    
  case GDK_BUTTON_PRESS:
    bevent=(GdkEventButton *) event;

    shad   =  abs(bevent->x - Current.Cutoffs[SHADOWS]);
    mid    =  abs(bevent->x - Current.Cutoffs[MIDTONES]);
    offset =  abs(bevent->x - Current.Offset);
    
    min= MIN(MIN(shad,mid),offset);

    
    if (bevent->x >0 && bevent->x<256) {
      if (min==shad) 
	new  = &Current.Cutoffs[SHADOWS];
      else if (min==mid) 
	new  = &Current.Cutoffs[MIDTONES];
      else 
	new  = &Current.Offset;

      slider_erase(AW.aliasingGraph->window,*new);
      *new=bevent->x;
    }
    
    draw_it(NULL);
  
    if (Current.RealTime) {
      fp_range_preview_spill(AW.rangePreview,Current.ValueBy);
      update_range_labels();
      fp_create_smoothness_graph(AW.aliasingPreview);
      refreshPreviews(Current.VisibleFrames);
    }
    break;

  case GDK_BUTTON_RELEASE:
    if (!Current.RealTime) {
      fp_range_preview_spill(AW.rangePreview,Current.ValueBy);
      update_range_labels();
      fp_create_smoothness_graph(AW.aliasingPreview);
      refreshPreviews(Current.VisibleFrames);
    }
    break;

  case GDK_MOTION_NOTIFY:
    mevent= (GdkEventMotion *) event;
    gdk_window_get_pointer(widget->window, &x,NULL, NULL);
    
    if (x>=0 && x<256) {
      slider_erase(AW.aliasingGraph->window,*new);
      *new=x;
      draw_it(NULL);
      if (Current.RealTime) {
	fp_range_preview_spill(AW.rangePreview,Current.ValueBy);
	update_range_labels();
	fp_create_smoothness_graph(AW.aliasingPreview);
	refreshPreviews(Current.VisibleFrames);
      }
    }
   
    break;

  default: 
    break;
  }
return 0;
}

void
update_range_labels()
{
  guchar buffer[3];
  
  gtk_label_set(GTK_LABEL(Current.rangeLabels[1]),"0");
  
  sprintf(buffer,"%d",Current.Cutoffs[SHADOWS]);
  gtk_label_set(GTK_LABEL(Current.rangeLabels[3]),buffer);
  gtk_label_set(GTK_LABEL(Current.rangeLabels[5]),buffer);

  sprintf(buffer,"%d",Current.Cutoffs[MIDTONES]);
  gtk_label_set(GTK_LABEL(Current.rangeLabels[7]),buffer);
  gtk_label_set(GTK_LABEL(Current.rangeLabels[9]),buffer);

  gtk_label_set(GTK_LABEL(Current.rangeLabels[11]),"255");
}
