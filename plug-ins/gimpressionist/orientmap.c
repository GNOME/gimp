#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gimpressionist.h"
#include "ppmtool.h"

#define MAPFILE "data.out"

#define NUMVECTYPES 4

GtkWidget *omwindow = NULL;

GtkWidget *vectorprev = NULL;
GtkWidget *ompreviewprev = NULL;

GtkObject *vectprevbrightadjust = NULL;

GtkObject *angadjust = NULL;
GtkObject *stradjust = NULL;
GtkObject *strexpadjust = NULL;
GtkObject *angoffadjust = NULL;
GtkWidget *vectypes[NUMVECTYPES];
GtkWidget *orientvoronoi = NULL;

#define OMWIDTH 150
#define OMHEIGHT 150

char buffer[OMWIDTH*OMHEIGHT];

struct vector_t vector[MAXORIENTVECT];
int numvect = 0;

double degtorad(double d)
{
  return d/180.0*G_PI;
}

double radtodeg(double d)
{
  double v = d/G_PI*180.0;
  if(v < 0.0) v += 360;
  return v;
}

char *skipnum(char *s)
{
  while((*s == ' ') || (*s == '\t')) s++;
  while(((*s >= '0') && (*s <= '9')) || (*s == '.')) s++;
  while((*s == ' ') || (*s == '\t')) s++;
  return s;
}

double dist(double x, double y, double dx, double dy)
{
  double ax = fabs(dx-x);
  double ay = fabs(dy-y);
  return sqrt((ax*ax)+(ay*ay));
}

int pixval(double dir)
{
  while(dir < 0.0) dir += 360.0;
  while(dir >= 360.0) dir -= 360.0;
  return dir * 255.0 / 360.0;
}

double getdir(double x, double y, int from)
{
  int i;
  int n;
  int voronoi;
  double sum, dx, dy, dst;
  struct vector_t *vec;
  double angoff, strexp;
  int first = 0, last;

  if(from == 0) {
    n = numvect;
    vec = vector;
    angoff = GTK_ADJUSTMENT(angoffadjust)->value;
    strexp = GTK_ADJUSTMENT(strexpadjust)->value;
    voronoi = GTK_TOGGLE_BUTTON(orientvoronoi)->active;
  } else {
    n = pcvals.numorientvector;
    vec = pcvals.orientvector;
    angoff = pcvals.orientangoff;
    strexp = pcvals.orientstrexp;
    voronoi = pcvals.orientvoronoi;
  }

  if(voronoi) {
    double bestdist = -1.0;
    for(i = 0; i < n; i++) {
      dst = dist(x,y,vec[i].x,vec[i].y);
      if((bestdist < 0.0) || (dst < bestdist)) {
	bestdist = dst;
	first = i;
      }
    }
    last = first+1;
  } else {
    first = 0;
    last = n;
  }

  dx = dy = 0.0;
  sum = 0.0;
  for(i = first; i < last; i++) {
    double s = vec[i].str;
    double tx = 0.0, ty = 0.0;
    
    if(vec[i].type == 0) {
      tx = vec[i].dx;
      ty = vec[i].dy;
    } else if(vec[i].type == 1) {
      double a = atan2(vec[i].dy, vec[i].dx);
      a -= atan2(y-vec[i].y, x-vec[i].x);
      tx = sin(a+G_PI_2);
      ty = cos(a+G_PI_2);
    } else if(vec[i].type == 2) {
      double a = atan2(vec[i].dy, vec[i].dx);
      a += atan2(y-vec[i].y, x-vec[i].x);
      tx = sin(a+G_PI_2);
      ty = cos(a+G_PI_2);
    } else if(vec[i].type == 3) {
      double a = atan2(vec[i].dy, vec[i].dx);
      a -= atan2(y-vec[i].y, x-vec[i].x)*2;
      tx = sin(a+G_PI_2);
      ty = cos(a+G_PI_2);
    }

    dst = dist(x,y,vec[i].x,vec[i].y);
    dst = pow(dst, strexp);
    if(dst < 0.0001) dst = 0.0001;
    s = s / dst;

    dx += tx * s;
    dy += ty * s;
    sum += s;
  }
  dx = dx / sum;
  dy = dy / sum;
  return 90-(radtodeg(atan2(dy,dx))+angoff);
}

void updateompreviewprev(void)
{
  int x, y;
  static struct ppm nbuffer = {0,0,NULL};
  guchar black[3] = {0,0,0};
  guchar gray[3] = {120,120,120};
  guchar white[3] = {255,255,255};

  if(!nbuffer.col) {
    newppm(&nbuffer,OMWIDTH,OMHEIGHT);
  }
  fill(&nbuffer, black);

  for(y = 6; y < OMHEIGHT-4; y += 10)
    for(x = 6; x < OMWIDTH-4; x += 10) {
      double dir = degtorad(getdir(x/(double)OMWIDTH,y/(double)OMHEIGHT,0));
      double xo = sin(dir)*4.0;
      double yo = cos(dir)*4.0;
      drawline(&nbuffer, x-xo, y-yo, x+xo, y+yo, gray);
      putrgb(&nbuffer, x-xo, y-yo, white);
    }

  for(y = 0; y < OMHEIGHT; y++)
    gtk_preview_draw_row(GTK_PREVIEW(ompreviewprev), (guchar *)nbuffer.col + y * OMWIDTH * 3, 0, y, OMWIDTH);
  gtk_widget_draw(ompreviewprev,NULL);
}

int selectedvector = 0;

void updatevectorprev(void)
{
  static struct ppm backup = {0,0,NULL};
  static struct ppm buffer = {0,0,NULL};
  static int ok = 0;
  int i, x, y;
  double dir, xo, yo;
  double val;
  static double lastval = 0.0;
  guchar gray[3] = {120,120,120};
  guchar red[3] = {255,0,0};
  guchar white[3] = {255,255,255};

  if(vectprevbrightadjust) val = 1.0 - GTK_ADJUSTMENT(vectprevbrightadjust)->value / 100.0;
  else val = 0.5;

  if(!ok || (val != lastval)) {
    if(!infile.col)
      updatepreviewprev(NULL, (void *)2); /* Force grabarea() */
    copyppm(&infile, &backup);
    ppmbrightness(&backup, val, 1,1,1);
    if((backup.width != OMWIDTH) || (backup.height != OMHEIGHT))
      resize_fast(&backup, OMWIDTH, OMHEIGHT);
    ok = 1;
  }
  copyppm(&backup, &buffer);

  for(i = 0; i < numvect; i++) {
    double s;
    x = vector[i].x * OMWIDTH;
    y = vector[i].y * OMHEIGHT;
    dir = degtorad(vector[i].dir);
    s = degtorad(vector[i].str);
    xo = sin(dir)*(6.0+100*s);
    yo = cos(dir)*(6.0+100*s);
    if(i == selectedvector)
      drawline(&buffer, x-xo, y-yo, x+xo, y+yo, red);
    else
      drawline(&buffer, x-xo, y-yo, x+xo, y+yo, gray);
    putrgb(&buffer, x-xo, y-yo, white);
  }

  for(y = 0; y < OMHEIGHT; y++)
    gtk_preview_draw_row(GTK_PREVIEW(vectorprev), (guchar *)buffer.col + y * OMWIDTH * 3, 0, y, OMWIDTH);
  gtk_widget_draw(vectorprev,NULL);

}


int adjignore = 0;

void updatesliders(void)
{
  int i;
  adjignore = 1;
  gtk_adjustment_set_value(GTK_ADJUSTMENT(angadjust),
			   vector[selectedvector].dir);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(stradjust),
			   vector[selectedvector].str);
  for(i = 0; i < NUMVECTYPES; i++) {
    if(i == vector[selectedvector].type)
      gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(vectypes[i]), TRUE);
    else
      gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(vectypes[i]), FALSE);
  }
  adjignore = 0;
}

void prevclick(GtkWidget *w, gpointer data)
{
  selectedvector--;
  if(selectedvector < 0) selectedvector = numvect-1;
  updatesliders();
  updatevectorprev();
}

void nextclick(GtkWidget *w, gpointer data)
{
  selectedvector++;
  if(selectedvector == numvect) selectedvector = 0;
  updatesliders();
  updatevectorprev();
}

void addclick(GtkWidget *w, gpointer data)
{
  if(numvect + 1 == MAXORIENTVECT) return;
  vector[numvect].x = 0.5;
  vector[numvect].y = 0.5;
  vector[numvect].dir = 0.0;
  vector[numvect].dx = sin(degtorad(0.0));
  vector[numvect].dy = cos(degtorad(0.0));
  vector[numvect].str = 1.0;
  vector[numvect].type = 0;
  selectedvector = numvect;
  numvect++;
  updatesliders();
  updatevectorprev();
  updateompreviewprev();
}

void deleteclick(GtkWidget *w, gpointer data)
{
  int i;

  if(numvect == 1) return;

  for(i = selectedvector; i < numvect-1; i++) {
    memcpy(&vector[i], &vector[i+1], sizeof(struct vector_t));
  }
  numvect--;
  if(selectedvector >= numvect) selectedvector = 0;
  updatesliders();
  updatevectorprev();
  updateompreviewprev();
}


void mapclick(GtkWidget *w, GdkEventButton *event)
{
  if(event->button == 1) {
    vector[selectedvector].x = event->x / (double)OMWIDTH;
    vector[selectedvector].y = event->y / (double)OMHEIGHT;

  } else if(event->button == 2) {
    if(numvect + 1 == MAXORIENTVECT) return;
    vector[numvect].x = event->x / (double)OMWIDTH;
    vector[numvect].y = event->y / (double)OMHEIGHT;
    vector[numvect].dir = 0.0;
    vector[numvect].dx = sin(degtorad(0.0));
    vector[numvect].dy = cos(degtorad(0.0));
    vector[numvect].str = 1.0;
    selectedvector = numvect;
    numvect++;
    updatesliders();

  } else if(event->button == 3) {
    double d;
    d = atan2(OMWIDTH * vector[selectedvector].x - event->x,
	      OMHEIGHT * vector[selectedvector].y - event->y);
    vector[selectedvector].dir = radtodeg(d);
    vector[selectedvector].dx = sin(d);
    vector[selectedvector].dy = cos(d);
    updatesliders();
  }
  updatevectorprev();
  updateompreviewprev();
}

void angadjmove(GtkWidget *w, gpointer data)
{
  if(adjignore) return;
  vector[selectedvector].dir = GTK_ADJUSTMENT(angadjust)->value;
  vector[selectedvector].dx = sin(degtorad(vector[selectedvector].dir));
  vector[selectedvector].dy = cos(degtorad(vector[selectedvector].dir));
  updatevectorprev();
  updateompreviewprev();
}

void stradjmove(GtkWidget *w, gpointer data)
{
  if(adjignore) return;
  vector[selectedvector].str = GTK_ADJUSTMENT(stradjust)->value;
  updatevectorprev();
  updateompreviewprev();
}

void strexpadjmove(GtkWidget *w, gpointer data)
{
  if(adjignore) return;
  updatevectorprev();
  updateompreviewprev();
}

void angoffadjmove(GtkWidget *w, gpointer data)
{
  if(adjignore) return;
  updatevectorprev();
  updateompreviewprev();
}

void vectypeclick(GtkWidget *w, gpointer data)
{
  int i;
  if(adjignore) return;
  for(i = 0; i < NUMVECTYPES; i++) {
    if(GTK_TOGGLE_BUTTON(vectypes[i])->active)
      vector[selectedvector].type = i;
  }
  updatevectorprev();
  updateompreviewprev();
}

void hidewin(GtkWidget *w, GtkWidget **win)
{
  gtk_widget_hide(*win);
}

void omcancelclick(GtkWidget *w, GtkWidget *win)
{
  if(win)
    gtk_widget_hide(win);
}

void omokclick(GtkWidget *w, GtkWidget *win)
{
  int i;
  for(i = 0; i < numvect; i++) {
    memcpy(&pcvals.orientvector[i], &vector[i], sizeof(struct vector_t));
  }
  pcvals.numorientvector = numvect;
  pcvals.orientstrexp = GTK_ADJUSTMENT(strexpadjust)->value;
  pcvals.orientangoff = GTK_ADJUSTMENT(angoffadjust)->value;
  pcvals.orientvoronoi = GTK_TOGGLE_BUTTON(orientvoronoi)->active;
  if(win)
    gtk_widget_hide(win);
}

void initvectors(void)
{
  int i;

  if(pcvals.numorientvector) {
    numvect = pcvals.numorientvector;
    for(i = 0; i < numvect; i++) {
      memcpy(&vector[i], &pcvals.orientvector[i], sizeof(struct vector_t));
    }
  } else {
    /* Shouldn't happen */
    numvect = 1;
    vector[0].x = 0.5;
    vector[0].y = 0.5;
    vector[0].dir = 0.0;
    vector[0].str = 1.0;
    vector[0].type = 0;
  }
  if(selectedvector >= numvect)
    selectedvector = numvect-1;
}

void update_orientmap_dialog(void)
{
  if(!omwindow) return;

  initvectors();

  gtk_adjustment_set_value(GTK_ADJUSTMENT(strexpadjust), pcvals.orientstrexp);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(angoffadjust), pcvals.orientangoff);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(orientvoronoi), pcvals.orientvoronoi);

  updatevectorprev();
  updateompreviewprev();
}

void create_orientmap_dialog(void)
{
  GtkWidget *tmpw, *tmpw2;
  GtkWidget *table1;
  GtkWidget *table2;
  GtkWidget *hbox;

  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (orientradio[7]), TRUE);

  initvectors();

  if(omwindow) {
    updatevectorprev();
    updateompreviewprev();
    gtk_widget_show(omwindow);
    return;
  }

  omwindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_signal_connect (GTK_OBJECT(omwindow), "destroy",
                      GTK_SIGNAL_FUNC(gtk_widget_destroyed),
                      &omwindow);
  gtk_signal_connect (GTK_OBJECT(omwindow), "delete_event",
                      GTK_SIGNAL_FUNC(gtk_widget_hide_on_delete),
                      &omwindow);

  gtk_window_set_title(GTK_WINDOW(omwindow), "Orientation Map Editor");

  gtk_container_border_width (GTK_CONTAINER(omwindow), 5);

  tmpw = table1 = gtk_table_new(2,5,FALSE);
  gtk_widget_show(tmpw);
  gtk_container_add(GTK_CONTAINER(omwindow), tmpw);

  tmpw2 = tmpw = gtk_frame_new("Vectors");
  gtk_container_border_width (GTK_CONTAINER (tmpw), 2);
  gtk_table_attach(GTK_TABLE(table1), tmpw, 0,1,0,1,GTK_EXPAND,GTK_EXPAND,0,0);
  gtk_widget_show(tmpw);

  tmpw = hbox = gtk_hbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(tmpw2), tmpw);
  gtk_widget_show(tmpw);

  tmpw = gtk_event_box_new();
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "The vector-field. Left-click to move selected vector, Right-click to point it towards mouse, Middle-click to add a new vector.", NULL);
  gtk_box_pack_start(GTK_BOX(hbox), tmpw, FALSE, FALSE, 0);
  tmpw2 = tmpw;

  tmpw = vectorprev = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size(GTK_PREVIEW(tmpw), OMWIDTH, OMHEIGHT);
  gtk_container_add(GTK_CONTAINER(tmpw2), tmpw);
  gtk_widget_show(tmpw);
  gtk_widget_set_events(tmpw2, GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect(GTK_OBJECT(tmpw2), "button_press_event",
                     GTK_SIGNAL_FUNC(mapclick), NULL);
  gtk_widget_realize(tmpw2);
  gtk_widget_show(tmpw2);

  vectprevbrightadjust = gtk_adjustment_new(50.0, 0.0, 100.0, 1.0, 1.0, 1.0);
  tmpw = gtk_vscale_new(GTK_ADJUSTMENT(vectprevbrightadjust));
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), tmpw,FALSE,FALSE,0);
  gtk_widget_show(tmpw);
  gtk_signal_connect(GTK_OBJECT(vectprevbrightadjust), "value_changed",
                     (GtkSignalFunc)updatevectorprev, NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Adjust the preview's brightness", NULL);

  tmpw2 = tmpw = gtk_frame_new("Preview");
  gtk_container_border_width (GTK_CONTAINER (tmpw), 2);
  gtk_table_attach(GTK_TABLE(table1), tmpw, 1,2,0,1,GTK_EXPAND,GTK_EXPAND,0,0);
  gtk_widget_show(tmpw);

  tmpw = ompreviewprev = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size(GTK_PREVIEW(tmpw), OMWIDTH, OMHEIGHT);
  gtk_container_add(GTK_CONTAINER(tmpw2), tmpw);
  gtk_widget_show(tmpw);

  hbox = tmpw = gtk_hbox_new(TRUE,0);
  gtk_container_border_width (GTK_CONTAINER (tmpw), 2);
  gtk_table_attach_defaults(GTK_TABLE(table1), tmpw, 0,1,1,2);
  gtk_widget_show(tmpw);

  tmpw = gtk_button_new_with_label("<<");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show(tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(prevclick), NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Select previous vector", NULL);

  tmpw = gtk_button_new_with_label(">>");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show(tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(nextclick), NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Select next vector", NULL);

  tmpw = gtk_button_new_with_label("Add");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show(tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(addclick), NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Add new vector", NULL);

  tmpw = gtk_button_new_with_label("Kill");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show(tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(deleteclick), NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Delete selected vector", NULL);


  tmpw = table2 = gtk_table_new(2,2,FALSE);
  gtk_widget_show(tmpw);
  gtk_table_attach_defaults(GTK_TABLE(table1), tmpw, 0,1,2,3);
  gtk_widget_show(tmpw);

  tmpw = gtk_label_new("Angle:");
  gtk_widget_show(tmpw);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 0,1,0,1);

  angadjust = gtk_adjustment_new(0.0, 0.0, 361.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(angadjust));
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 1);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 1,2,0,1);
  gtk_widget_show(tmpw);
  gtk_signal_connect(GTK_OBJECT(angadjust), "value_changed",
		     (GtkSignalFunc)angadjmove, NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Change the angle of the selected vector", NULL);

  tmpw = gtk_label_new("Strength:");
  gtk_widget_show(tmpw);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 0,1,1,2);

  stradjust = gtk_adjustment_new(1.0, 0.1, 6.0, 0.1, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(stradjust));
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 1);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 1,2,1,2);
  gtk_widget_show(tmpw);
  gtk_signal_connect(GTK_OBJECT(stradjust), "value_changed",
		     (GtkSignalFunc)stradjmove, NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Change the strength of the selected vector", NULL);


  tmpw = gtk_label_new("Type:");
  gtk_widget_show(tmpw);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 0,1,2,3);


  tmpw = hbox = gtk_hbox_new(TRUE,0);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 1,2,2,3);
  gtk_container_border_width (GTK_CONTAINER (tmpw), 2);
  gtk_widget_show(tmpw);

  vectypes[0] = tmpw = gtk_radio_button_new_with_label(NULL, "Normal");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(vectypeclick), NULL);
  gtk_widget_show(tmpw);
 
  vectypes[1] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(vectypes[0])), "Vortex");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(vectypeclick), NULL);
  gtk_widget_show(tmpw);


  tmpw = hbox = gtk_hbox_new(TRUE,0);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 1,2,3,4);
  gtk_container_border_width (GTK_CONTAINER (tmpw), 2);
  gtk_widget_show(tmpw);
 
  vectypes[2] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(vectypes[0])), "Vortex2");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(vectypeclick), NULL);
  gtk_widget_show(tmpw);
 
  vectypes[3] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(vectypes[0])), "Vortex3");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(vectypeclick), NULL);
  gtk_widget_show(tmpw);
 

  tmpw = hbox = gtk_hbox_new(TRUE,0);
  gtk_table_attach_defaults(GTK_TABLE(table1), tmpw, 1,2,1,2);
  gtk_container_border_width (GTK_CONTAINER (tmpw), 2);
  gtk_widget_show(tmpw);

  tmpw = gtk_button_new_with_label("OK");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show(tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(omokclick), omwindow);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Apply and exit the editor", NULL);

  tmpw = gtk_button_new_with_label("Apply");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show(tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(omokclick), NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Apply, but stay inside the editor", NULL);

  tmpw = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show(tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(omcancelclick), omwindow);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Cancel all changes and exit", NULL);


  tmpw = table2 = gtk_table_new(2,2,FALSE);
  gtk_widget_show(tmpw);
  gtk_table_attach_defaults(GTK_TABLE(table1), tmpw, 1,2,2,3);
  gtk_widget_show(tmpw);

  tmpw = gtk_label_new("Strength exp.:");
  gtk_widget_show(tmpw);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 0,1,0,1);

  strexpadjust = gtk_adjustment_new(pcvals.orientstrexp, 0.1, 11.0, 0.1, 0.1, 0.1);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(strexpadjust));
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 1);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 1,2,0,1);
  gtk_widget_show(tmpw);
  gtk_signal_connect(GTK_OBJECT(strexpadjust), "value_changed",
		     (GtkSignalFunc)strexpadjmove, NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Change the exponent of the strength", NULL);

  tmpw = gtk_label_new("Angle offset:");
  gtk_widget_show(tmpw);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 0,1,1,2);

  angoffadjust = gtk_adjustment_new(pcvals.orientangoff, 0.0, 361.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(angoffadjust));
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 1);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 1,2,1,2);
  gtk_widget_show(tmpw);
  gtk_signal_connect(GTK_OBJECT(angoffadjust), "value_changed",
		     (GtkSignalFunc)angoffadjmove, NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Offset all vectors with a given angle", NULL);

  orientvoronoi = tmpw = gtk_check_button_new_with_label("Voronoi");
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 0,1,2,3);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tmpw), FALSE);
  gtk_widget_show (tmpw);
  if(pcvals.orientvoronoi)
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tmpw), TRUE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)angoffadjmove, NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Voronoi-mode makes only the vector closest to the given point have any influence", NULL);
  
  gtk_widget_show(omwindow);

  updatevectorprev();
  updateompreviewprev();
}

/*
void main(int argc, char **argv)
{
  gtk_set_locale ();
  gtk_init (&argc, &argv);
  gtk_rc_parse ("./rc");

  if(argc > 1)
    readdata(argv[1]);
  else {
    numvect++;
    vector[0].x = 0.5;
    vector[0].y = 0.5;
    vector[0].dir = 0.0;
    vector[0].str = 1.0;
  }
    
  create_orientmap_dialog();
  gtk_main();
}
*/
