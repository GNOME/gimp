#include "rcm.h"

extern RcmParams Current;
GdkGC *xor_gc;


void
rcm_float_in_an_entry(GtkWidget *entry,
		      float     value)
{
  gchar buffer[10];
  if (value<10)
    sprintf(buffer,"%.4f",value);
  else if (value<100)
    sprintf(buffer,"%.3f",value);
  else if (value<1000)
    sprintf(buffer,"%.2f",value);    
  gtk_entry_set_text(GTK_ENTRY(entry),buffer);

}

gint
rcm_expose_event  (GtkWidget *widget, 
		   GdkEvent  *event, 
		   RcmCircle *circle)
{
  switch (circle->action_flag) {
  case DO_NOTHING: return 0;
  case VIRGIN: rcm_draw_arrows(widget->window,
			       widget->style->black_gc,
			       circle->angle);  break;
  default: if (Current.RealTime)
    rcm_render_preview(Current.Bna->after,CURRENT);  break;
  }
  return 1;
}

gint
rcm_button_press_event  (GtkWidget *widget, 
			 GdkEvent  *event, 
			 RcmCircle *circle)
{
  float clicked_angle;
  float *alpha = &(circle->angle->alpha);
  float *beta  = &(circle->angle->beta);
  GdkEventButton *bevent=(GdkEventButton *) event;

  circle->action_flag=DRAG_START;
  circle->prev_clicked=
  clicked_angle=angle_mod_2PI(arctg(CENTER-bevent->y,bevent->x-CENTER));
  
  if (sqrt(sqr(bevent->y-CENTER)+sqr(bevent->x-CENTER))>RADIUS*EACH_OR_BOTH
      && min_prox( *alpha, *beta, clicked_angle)       <PI/12) {
    circle->mode=EACH;
    circle->target=closest(alpha,beta,clicked_angle);
    
    if (*(circle->target) != clicked_angle) {
      *(circle->target) = clicked_angle; 
      gtk_widget_draw(circle->preview,NULL);
      rcm_draw_arrows(widget->window,
		      widget->style->black_gc,
		      circle->angle);
      
      rcm_float_in_an_entry(circle->alpha_entry,
			    circle->angle->alpha*
			    rcm_units_factor(Current.Units));
      rcm_float_in_an_entry(circle->beta_entry,
			    circle->angle->beta*
			    rcm_units_factor(Current.Units));
      if (Current.RealTime)
	rcm_render_preview(Current.Bna->after, CURRENT);
    }
  }
  else 
    circle->mode = BOTH;

  return 1;
}

gint
rcm_release_event  (GtkWidget *widget, 
		    GdkEvent  *event, 
		    RcmCircle *circle)
{
  if (circle->action_flag==DRAGING)
    rcm_draw_arrows(widget->window,
		    widget->style->black_gc,
		    circle->angle); 
  circle->action_flag = VIRGIN;
  
  if (!(Current.RealTime))
      rcm_render_preview(Current.Bna->after,CURRENT);

  return 1;
}

gint
rcm_motion_notify_event  (GtkWidget *widget,
			  GdkEvent  *event,
			  RcmCircle *circle)
{ 
  gint x, y;
  float clicked_angle;
  float *alpha = &(circle->angle->alpha);
  float *beta  = &(circle->angle->beta);
  int   cw_ccw = circle->angle->cw_ccw;
  float delta=angle_mod_2PI(cw_ccw*(*beta-*alpha));
  GdkGCValues values;

  values.foreground = Current.From->preview->style->white;
  values.function = GDK_XOR;
  xor_gc = gdk_gc_new_with_values (Current.From->preview->window,
				   &values,
				   GDK_GC_FOREGROUND |
				   GDK_GC_FUNCTION);
  

  gdk_window_get_pointer(widget->window, &x,&y, NULL);
  clicked_angle=angle_mod_2PI(arctg(CENTER-y,x-CENTER));
  
  delta=clicked_angle - circle->prev_clicked;
  circle->prev_clicked=clicked_angle;
  
  if (delta) {
    if (circle->action_flag==DRAG_START) {
      gtk_widget_draw(circle->preview,NULL);
      circle->action_flag=DRAGING;
    }
    else 
      rcm_draw_arrows(widget->window,
		      xor_gc,
		      circle->angle);  /* erase! */
    
    if (circle->mode==EACH)
      *(circle->target)=clicked_angle;
    else {
      circle->angle->alpha=angle_mod_2PI(circle->angle->alpha + delta);
      circle->angle->beta =angle_mod_2PI(circle->angle->beta  + delta);
    }
    
    rcm_draw_arrows(widget->window,
		    xor_gc,
		    circle->angle); 
    
    rcm_float_in_an_entry(circle->alpha_entry,
			  circle->angle->alpha*
			  rcm_units_factor(Current.Units));
    rcm_float_in_an_entry(circle->beta_entry,
			  circle->angle->beta*
			  rcm_units_factor(Current.Units));
    
    if (Current.RealTime)
      rcm_render_preview(Current.Bna->after,CURRENT);
  }

  return 1;
}

/******************* GRAY *******************/

gint
rcm_gray_expose_event  (GtkWidget *widget, 
			GdkEvent  *event, 
			RcmGray   *circle)
{
  if (circle->action_flag==VIRGIN) {
    rcm_draw_little_circle(widget->window,
			   widget->style->black_gc,
			   circle->hue,
			   circle->satur);
    rcm_draw_large_circle(widget->window,
			  widget->style->black_gc,
			  circle->gray_sat);
  }
  else if (Current.RealTime)
    rcm_render_preview(Current.Bna->after,CURRENT);

  return 1;
}

gint
rcm_gray_button_press_event  (GtkWidget *widget, 
			      GdkEvent  *event, 
			      RcmGray   *circle)
{
  GdkEventButton *bevent=(GdkEventButton *) event;
  int x = bevent->x-GRAY_CENTER-LITTLE_RADIUS;
  int y = GRAY_CENTER-bevent->y+LITTLE_RADIUS;

  circle->action_flag=DRAG_START;
  circle->hue=angle_mod_2PI(arctg(y,x));
  circle->satur=sqrt(sqr(x)+sqr(y))/GRAY_RADIUS;
  if (circle->satur > 1.0) circle->satur=1;

  gtk_widget_draw(circle->preview,NULL);
  rcm_draw_little_circle(widget->window,
			 widget->style->black_gc,
			 circle->hue,
			 circle->satur);
  
  rcm_draw_large_circle(circle->preview->window,
			circle->preview->style->black_gc,
			circle->gray_sat);  
  rcm_float_in_an_entry(circle->hue_entry,
			      circle->hue*
			      rcm_units_factor(Current.Units));
  rcm_float_in_an_entry(circle->satur_entry,circle->satur);
  
  if (Current.RealTime)
      rcm_render_preview(Current.Bna->after,CURRENT);

  return 1;
}

gint
rcm_gray_release_event  (GtkWidget *widget, 
			 GdkEvent  *event, 
			 RcmGray *circle)
{
  if (circle->action_flag==DRAGING)
    rcm_draw_little_circle(widget->window,
			   widget->style->black_gc,
			   circle->hue,
			   circle->satur);
  circle->action_flag=VIRGIN;
  
  if (!(Current.RealTime))
    rcm_render_preview(Current.Bna->after,CURRENT);

  return 1;
}

gint
rcm_gray_motion_notify_event  (GtkWidget *widget,
			       GdkEvent  *event,
			       RcmGray   *circle)
{ 
  gint x, y;
  GdkGCValues values;
  
  values.foreground = Current.From->preview->style->white;
  values.function = GDK_XOR;
  xor_gc = gdk_gc_new_with_values (Current.From->preview->window,
				   &values,
				   GDK_GC_FOREGROUND |
				   GDK_GC_FUNCTION);
  
 
  if (circle->action_flag==DRAG_START) {
    gtk_widget_draw(circle->preview,NULL);
    rcm_draw_large_circle(circle->preview->window,
			  circle->preview->style->black_gc,
			  circle->gray_sat);  
    circle->action_flag=DRAGING;
  }
  else 
    rcm_draw_little_circle(widget->window,
			   xor_gc,
			   circle->hue,
			   circle->satur); /* erase */

  gdk_window_get_pointer(widget->window, &x,&y, NULL);
  x = x-GRAY_CENTER-LITTLE_RADIUS;
  y = GRAY_CENTER-y+LITTLE_RADIUS;

  circle->hue=angle_mod_2PI(arctg(y,x));
  circle->satur=sqrt(sqr(x)+sqr(y))/GRAY_RADIUS;
  if (circle->satur > 1.0) circle->satur=1;

  rcm_draw_little_circle(widget->window,
			 xor_gc,
			 circle->hue,
			 circle->satur);
  
  rcm_float_in_an_entry(circle->hue_entry,
			circle->hue*
			rcm_units_factor(Current.Units));
  rcm_float_in_an_entry(circle->satur_entry,circle->satur);
  if (Current.RealTime)
    rcm_render_preview(Current.Bna->after,CURRENT);

  return 1;
}

/* Entry Handlers */
void 
rcm_set_alpha(GtkWidget *entry,
	      gpointer  data)
{
  RcmCircle *circle=(RcmCircle *) data;
  if (circle->action_flag!=VIRGIN) return;
 
  circle->angle->alpha=atof( gtk_entry_get_text(GTK_ENTRY(entry)))/
    rcm_units_factor(Current.Units);

  gtk_widget_draw(circle->preview,NULL);
  rcm_draw_arrows(circle->preview->window,
		  circle->preview->style->black_gc,
		  circle->angle); 
  rcm_render_preview(Current.Bna->after,CURRENT);
}

void 
rcm_set_beta(GtkWidget *entry,
	     gpointer  data)
{
  RcmCircle *circle=(RcmCircle *) data;
  if (circle->action_flag!=VIRGIN) return;
  
  circle->angle->beta= atof( gtk_entry_get_text( GTK_ENTRY(entry)))/
    rcm_units_factor(Current.Units);
  gtk_widget_draw(circle->preview,NULL);
  rcm_draw_arrows(circle->preview->window,
		  circle->preview->style->black_gc,
		  circle->angle);  
  rcm_render_preview(Current.Bna->after,CURRENT);
}

void 
rcm_set_hue(GtkWidget *entry,
	    gpointer  data)
{
  RcmGray *circle=(RcmGray *) data;
  if (circle->action_flag!=VIRGIN) return;
  
  circle->hue = atof( gtk_entry_get_text( GTK_ENTRY(entry)))/
    rcm_units_factor(Current.Units);
  gtk_widget_draw(circle->preview,NULL);
  rcm_draw_little_circle(circle->preview->window,
			 circle->preview->style->black_gc,
			 circle->hue,
			 circle->satur);  
  rcm_draw_large_circle(circle->preview->window,
			circle->preview->style->black_gc,
			circle->gray_sat);  
  rcm_render_preview(Current.Bna->after,CURRENT);
}

void 
rcm_set_satur(GtkWidget *entry,
	      gpointer  data)
{
  RcmGray *circle=(RcmGray *) data;
  if (circle->action_flag!=VIRGIN) return;
  
  circle->satur = atof( gtk_entry_get_text( GTK_ENTRY(entry)));
  gtk_widget_draw(circle->preview,NULL);
  rcm_draw_little_circle(circle->preview->window,
			 circle->preview->style->black_gc,
			 circle->hue,
			 circle->satur);  
  rcm_draw_large_circle(circle->preview->window,
			circle->preview->style->black_gc,
			circle->gray_sat);  
  rcm_render_preview(Current.Bna->after,CURRENT);
}

void 
rcm_set_gray_sat(GtkWidget *entry,
		 gpointer  data)
{
  RcmGray *circle=(RcmGray *) data;

  circle->gray_sat = atof( gtk_entry_get_text( GTK_ENTRY(entry)));
  if (circle->gray_sat>1) 
    circle->gray_sat = 1.0;
  gtk_widget_draw(circle->preview,NULL);
  rcm_draw_large_circle(circle->preview->window,
			circle->preview->style->black_gc,
			circle->gray_sat);  
  rcm_render_preview(Current.Bna->after,CURRENT);
}

void 
rcm_draw_little_circle(GdkWindow  *window,
		       GdkGC      *color,
		       float      hue,
		       float      satur)
{
  int x=GRAY_CENTER+GRAY_RADIUS*satur*cos(hue);
  int y=GRAY_CENTER-GRAY_RADIUS*satur*sin(hue);
  gdk_draw_arc        (window,
		       color,
		       0,
                       x-LITTLE_RADIUS,
                       y-LITTLE_RADIUS,
		       2*LITTLE_RADIUS,
		       2*LITTLE_RADIUS,
		       0,
		       360*64);
  
}

int 
R(float X) {
  return X+.5;
}

void 
rcm_draw_large_circle(GdkWindow  *window,
		      GdkGC      *color,
		      float      gray_sat)
{
  int x=GRAY_CENTER;
  int y=GRAY_CENTER;
  gdk_draw_arc        (window,
		       color,
		       0,
                       R(x-GRAY_RADIUS*gray_sat),
                       R(y-GRAY_RADIUS*gray_sat),
		       R(2*GRAY_RADIUS*gray_sat),
		       R(2*GRAY_RADIUS*gray_sat),
		       0,
		       360*64);
  
}


void 
rcm_draw_arrows(GdkWindow  *window,
		GdkGC      *color,
		RcmAngle   *angle)
{
#define REL .8
#define DEL .1
#define TICK 10
  
  int dist;
  float alpha=angle->alpha;
  float beta=angle->beta;
  float cw_ccw=angle->cw_ccw;
  float delta=angle_mod_2PI(beta-alpha);
  if (cw_ccw==-1) delta = delta-TP;

  gdk_draw_line(window,color,
		CENTER,
		CENTER,
		R(CENTER+RADIUS*cos(alpha)),
		R(CENTER-RADIUS*sin(alpha)));

  gdk_draw_line(window,color,
		CENTER+RADIUS*cos(alpha),
		CENTER-RADIUS*sin(alpha),
		R(CENTER+RADIUS*REL*cos(alpha-DEL)),
		R(CENTER-RADIUS*REL*sin(alpha-DEL)));

  gdk_draw_line(window,color,
		CENTER+RADIUS*cos(alpha),
		CENTER-RADIUS*sin(alpha),
		R(CENTER+RADIUS*REL*cos(alpha+DEL)),
		R(CENTER-RADIUS*REL*sin(alpha+DEL)));

  gdk_draw_line(window,color,
		CENTER,
		CENTER,
		R(CENTER+RADIUS*cos(beta)),
		R(CENTER-RADIUS*sin(beta)));


  gdk_draw_line(window,color,
		CENTER+RADIUS*cos(beta),
		CENTER-RADIUS*sin(beta),
		R(CENTER+RADIUS*REL*cos(beta-DEL)),
		R(CENTER-RADIUS*REL*sin(beta-DEL)));

  gdk_draw_line(window,color,
		CENTER+RADIUS*cos(beta),
		CENTER-RADIUS*sin(beta),
		R(CENTER+RADIUS*REL*cos(beta+DEL)),
		R(CENTER-RADIUS*REL*sin(beta+DEL)));

  dist   = RADIUS*EACH_OR_BOTH;

  gdk_draw_line(window,color,
		CENTER   + dist*cos(beta),
		CENTER   - dist*sin(beta),
		R(CENTER + dist*cos(beta)+cw_ccw*TICK*sin(beta)),
		R(CENTER - dist*sin(beta)+cw_ccw*TICK*cos(beta)));

  /*  gdk_draw_line(window,color,
		CENTER   + dist*cos(beta),
		CENTER   - dist*sin(beta),
		R(CENTER + dist*cos(beta)+cw_ccw*TICK*sin(beta))+TICK,
		R(CENTER - dist*sin(beta)+cw_ccw*TICK*cos(beta))+TICK);
		*/
  alpha *= 180*64/PI;
  delta *= 180*64/PI;

  gdk_draw_arc (window,
		color,
		0,
		CENTER-dist,
		CENTER-dist,
		2*dist,
		2*dist,
		alpha,
		delta);
}
