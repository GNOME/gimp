#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gimpressionist.h"
#include "ppmtool.h"

#define MAPFILE "data.out"

GtkWidget *smwindow = NULL;

GtkWidget *smvectorprev = NULL;
GtkWidget *smpreviewprev = NULL;

GtkObject *smvectprevbrightadjust = NULL;

GtkObject *sizadjust = NULL;
GtkObject *smstradjust = NULL;
GtkObject *smstrexpadjust = NULL;
GtkWidget *sizevoronoi = NULL;

#define OMWIDTH 150
#define OMHEIGHT 150

char sbuffer[OMWIDTH*OMHEIGHT];

struct smvector_t smvector[MAXSIZEVECT];
int numsmvect = 0;

double dist(double x, double y, double dx, double dy);


double getsiz(double x, double y, int from)
{
  int i;
  int n;
  int voronoi;
  double sum, ssum, dst;
  struct smvector_t *vec;
  double smstrexp;
  int first = 0, last;

  if((x < 0.0) || (x > 1.0)) printf("HUH? x = %f\n",x);

  if(from == 0) {
    n = numsmvect;
    vec = smvector;
    smstrexp = GTK_ADJUSTMENT(smstrexpadjust)->value;
    voronoi = GTK_TOGGLE_BUTTON(sizevoronoi)->active;
  } else {
    n = pcvals.numsizevector;
    vec = pcvals.sizevector;
    smstrexp = pcvals.sizestrexp;
    voronoi = pcvals.sizevoronoi;
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

  sum = ssum = 0.0;
  for(i = first; i < last; i++) {
    double s = vec[i].str;

    dst = dist(x,y,vec[i].x,vec[i].y);
    dst = pow(dst, smstrexp);
    if(dst < 0.0001) dst = 0.0001;
    s = s / dst;

    sum += vec[i].siz * s;
    ssum += 1.0/dst;
  }
  sum = sum / ssum / 100.0;
  if(sum < 0.0) sum = 0.0;
  else if(sum > 1.0) sum = 1.0;
  return sum;
}

void updatesmpreviewprev(void)
{
  int x, y;
  static struct ppm nsbuffer = {0,0,NULL};
  guchar black[3] = {0,0,0};
  guchar gray[3] = {120,120,120};

  if(!nsbuffer.col) {
    newppm(&nsbuffer,OMWIDTH,OMHEIGHT);
  }
  fill(&nsbuffer, black);

  for(y = 6; y < OMHEIGHT-4; y += 10)
    for(x = 6; x < OMWIDTH-4; x += 10) {
      double siz = 5 * getsiz(x/(double)OMWIDTH,y/(double)OMHEIGHT,0);
      drawline(&nsbuffer, x-siz, y-siz, x+siz, y-siz, gray);
      drawline(&nsbuffer, x+siz, y-siz, x+siz, y+siz, gray);
      drawline(&nsbuffer, x+siz, y+siz, x-siz, y+siz, gray);
      drawline(&nsbuffer, x-siz, y+siz, x-siz, y-siz, gray);
    }

  for(y = 0; y < OMHEIGHT; y++)
    gtk_preview_draw_row(GTK_PREVIEW(smpreviewprev), &nsbuffer.col[y*nsbuffer.width*3], 0, y, OMWIDTH);
  gtk_widget_draw(smpreviewprev,NULL);
}

int selectedsmvector = 0;

void updatesmvectorprev(void)
{
  static struct ppm backup = {0,0,NULL};
  static struct ppm sbuffer = {0,0,NULL};
  static int ok = 0;
  int i, x, y;
  double val;
  static double lastval = 0.0;
  guchar gray[3] = {120,120,120};
  guchar red[3] = {255,0,0};
  guchar white[3] = {255,255,255};

  if(smvectprevbrightadjust) val = 1.0 - GTK_ADJUSTMENT(smvectprevbrightadjust)->value / 100.0;
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
  copyppm(&backup, &sbuffer);

  for(i = 0; i < numsmvect; i++) {
    x = smvector[i].x * OMWIDTH;
    y = smvector[i].y * OMHEIGHT;
    if(i == selectedsmvector) {
      drawline(&sbuffer, x-5, y, x+5, y, red);
      drawline(&sbuffer, x, y-5, x, y+5, red);
    } else {
      drawline(&sbuffer, x-5, y, x+5, y, gray);
      drawline(&sbuffer, x, y-5, x, y+5, gray);
    }
    putrgb(&sbuffer, x, y, white);
  }

  for(y = 0; y < OMHEIGHT; y++)
    gtk_preview_draw_row(GTK_PREVIEW(smvectorprev), &sbuffer.col[y*sbuffer.width*3], 0, y, OMWIDTH);
  gtk_widget_draw(smvectorprev,NULL);

}


int smadjignore = 0;

void updatesmsliders(void)
{
  smadjignore = 1;
  gtk_adjustment_set_value(GTK_ADJUSTMENT(sizadjust),
			   smvector[selectedsmvector].siz);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(smstradjust),
			   smvector[selectedsmvector].str);
  smadjignore = 0;
}

void smprevclick(GtkWidget *w, gpointer data)
{
  selectedsmvector--;
  if(selectedsmvector < 0) selectedsmvector = numsmvect-1;
  updatesmsliders();
  updatesmvectorprev();
}

void smnextclick(GtkWidget *w, gpointer data)
{
  selectedsmvector++;
  if(selectedsmvector == numsmvect) selectedsmvector = 0;
  updatesmsliders();
  updatesmvectorprev();
}

void smaddclick(GtkWidget *w, gpointer data)
{
  if(numsmvect + 1 == MAXSIZEVECT) return;
  smvector[numsmvect].x = 0.5;
  smvector[numsmvect].y = 0.5;
  smvector[numsmvect].siz = 50.0;
  smvector[numsmvect].str = 1.0;
  selectedsmvector = numsmvect;
  numsmvect++;
  updatesmsliders();
  updatesmvectorprev();
  updatesmpreviewprev();
}

void smdeleteclick(GtkWidget *w, gpointer data)
{
  int i;

  if(numsmvect == 1) return;

  for(i = selectedsmvector; i < numsmvect-1; i++) {
    memcpy(&smvector[i], &smvector[i+1], sizeof(struct smvector_t));
  }
  numsmvect--;
  if(selectedsmvector >= numsmvect) selectedsmvector = 0;
  updatesmsliders();
  updatesmvectorprev();
  updatesmpreviewprev();
}


void smmapclick(GtkWidget *w, GdkEventButton *event)
{
  if(event->button == 1) {
    smvector[selectedsmvector].x = event->x / (double)OMWIDTH;
    smvector[selectedsmvector].y = event->y / (double)OMHEIGHT;

  } else if(event->button == 2) {
    if(numsmvect + 1 == MAXSIZEVECT) return;
    smvector[numsmvect].x = event->x / (double)OMWIDTH;
    smvector[numsmvect].y = event->y / (double)OMHEIGHT;
    smvector[numsmvect].siz = 0.0;
    smvector[numsmvect].str = 1.0;
    selectedsmvector = numsmvect;
    numsmvect++;
    updatesmsliders();

    /*
  } else if(event->button == 3) {
    double d;
    d = atan2(OMWIDTH * smvector[selectedsmvector].x - event->x,
	      OMHEIGHT * smvector[selectedsmvector].y - event->y);
    smvector[selectedsmvector].dir = radtodeg(d);
    updatesmsliders();
    */
  }
  updatesmvectorprev();
  updatesmpreviewprev();
}

void angsmadjmove(GtkWidget *w, gpointer data)
{
  if(smadjignore) return;
  smvector[selectedsmvector].siz = GTK_ADJUSTMENT(sizadjust)->value;
  updatesmvectorprev();
  updatesmpreviewprev();
}

void strsmadjmove(GtkWidget *w, gpointer data)
{
  if(smadjignore) return;
  smvector[selectedsmvector].str = GTK_ADJUSTMENT(smstradjust)->value;
  updatesmvectorprev();
  updatesmpreviewprev();
}

void smstrexpsmadjmove(GtkWidget *w, gpointer data)
{
  if(smadjignore) return;
  updatesmvectorprev();
  updatesmpreviewprev();
}


void hidewin(GtkWidget *w, GtkWidget **win);

void smcancelclick(GtkWidget *w, GtkWidget *win)
{
  if(win)
    gtk_widget_hide(win);
}

void smokclick(GtkWidget *w, GtkWidget *win)
{
  int i;
  for(i = 0; i < numsmvect; i++) {
    memcpy(&pcvals.sizevector[i], &smvector[i], sizeof(struct smvector_t));
  }
  pcvals.numsizevector = numsmvect;
  pcvals.sizestrexp = GTK_ADJUSTMENT(smstrexpadjust)->value;
  pcvals.sizevoronoi = GTK_TOGGLE_BUTTON(sizevoronoi)->active;
  if(win)
    gtk_widget_hide(win);
}

void initsmvectors(void)
{
  int i;

  if(pcvals.numsizevector) {
    numsmvect = pcvals.numsizevector;
    for(i = 0; i < numsmvect; i++) {
      memcpy(&smvector[i], &pcvals.sizevector[i], sizeof(struct smvector_t));
    }
  } else {
    /* Shouldn't happen */
    numsmvect = 1;
    smvector[0].x = 0.5;
    smvector[0].y = 0.5;
    smvector[0].siz = 0.0;
    smvector[0].str = 1.0;
  }
  if(selectedsmvector >= numsmvect)
    selectedsmvector = numsmvect-1;
}

void update_sizemap_dialog(void)
{
  if(!smwindow) return;

  initsmvectors();

  gtk_adjustment_set_value(GTK_ADJUSTMENT(smstrexpadjust), pcvals.sizestrexp);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(sizevoronoi), pcvals.sizevoronoi);

  updatesmvectorprev();
  updatesmpreviewprev();
}

void create_sizemap_dialog(void)
{
  GtkWidget *tmpw, *tmpw2;
  GtkWidget *table1;
  GtkWidget *table2;
  GtkWidget *hbox;

  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (sizeradio[7]), TRUE);

  initsmvectors();

  if(smwindow) {
    updatesmvectorprev();
    updatesmpreviewprev();
    gtk_widget_show(smwindow);
    return;
  }

  smwindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_signal_connect (GTK_OBJECT(smwindow), "destroy",
                      GTK_SIGNAL_FUNC(gtk_widget_destroyed),
                      &smwindow);
  gtk_signal_connect (GTK_OBJECT(smwindow), "delete_event",
                      GTK_SIGNAL_FUNC(gtk_widget_hide_on_delete),
                      &smwindow);

  gtk_window_set_title(GTK_WINDOW(smwindow), "Size Map Editor");

  gtk_container_border_width (GTK_CONTAINER(smwindow), 5);

  tmpw = table1 = gtk_table_new(2,5,FALSE);
  gtk_widget_show(tmpw);
  gtk_container_add(GTK_CONTAINER(smwindow), tmpw);

  tmpw2 = tmpw = gtk_frame_new("Smvectors");
  gtk_container_border_width (GTK_CONTAINER (tmpw), 2);
  gtk_table_attach(GTK_TABLE(table1), tmpw, 0,1,0,1,GTK_EXPAND,GTK_EXPAND,0,0);
  gtk_widget_show(tmpw);

  tmpw = hbox = gtk_hbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(tmpw2), tmpw);
  gtk_widget_show(tmpw);

  tmpw = gtk_event_box_new();
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "The smvector-field. Left-click to move selected smvector, Right-click to point it towards mouse, Middle-click to add a new smvector.", NULL);
  gtk_box_pack_start(GTK_BOX(hbox), tmpw, FALSE, FALSE, 0);
  tmpw2 = tmpw;

  tmpw = smvectorprev = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size(GTK_PREVIEW(tmpw), OMWIDTH, OMHEIGHT);
  gtk_container_add(GTK_CONTAINER(tmpw2), tmpw);
  gtk_widget_show(tmpw);
  gtk_widget_set_events(tmpw2, GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect(GTK_OBJECT(tmpw2), "button_press_event",
                     GTK_SIGNAL_FUNC(smmapclick), NULL);
  gtk_widget_realize(tmpw2);
  gtk_widget_show(tmpw2);

  smvectprevbrightadjust = gtk_adjustment_new(50.0, 0.0, 100.0, 1.0, 1.0, 1.0);
  tmpw = gtk_vscale_new(GTK_ADJUSTMENT(smvectprevbrightadjust));
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), tmpw,FALSE,FALSE,0);
  gtk_widget_show(tmpw);
  gtk_signal_connect(GTK_OBJECT(smvectprevbrightadjust), "value_changed",
                     (GtkSignalFunc)updatesmvectorprev, NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Adjust the preview's brightness", NULL);

  tmpw2 = tmpw = gtk_frame_new("Preview");
  gtk_container_border_width (GTK_CONTAINER (tmpw), 2);
  gtk_table_attach(GTK_TABLE(table1), tmpw, 1,2,0,1,GTK_EXPAND,GTK_EXPAND,0,0);
  gtk_widget_show(tmpw);

  tmpw = smpreviewprev = gtk_preview_new(GTK_PREVIEW_COLOR);
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
		      GTK_SIGNAL_FUNC(smprevclick), NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Select previous smvector", NULL);

  tmpw = gtk_button_new_with_label(">>");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show(tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(smnextclick), NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Select next smvector", NULL);

  tmpw = gtk_button_new_with_label("Add");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show(tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(smaddclick), NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Add new smvector", NULL);

  tmpw = gtk_button_new_with_label("Kill");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show(tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(smdeleteclick), NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Delete selected smvector", NULL);


  tmpw = table2 = gtk_table_new(2,2,FALSE);
  gtk_widget_show(tmpw);
  gtk_table_attach_defaults(GTK_TABLE(table1), tmpw, 0,1,2,3);
  gtk_widget_show(tmpw);

  tmpw = gtk_label_new("Size:");
  gtk_widget_show(tmpw);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 0,1,0,1);

  sizadjust = gtk_adjustment_new(50.0, 0.0, 101.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(sizadjust));
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 1);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 1,2,0,1);
  gtk_widget_show(tmpw);
  gtk_signal_connect(GTK_OBJECT(sizadjust), "value_changed",
		     (GtkSignalFunc)angsmadjmove, NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Change the angle of the selected smvector", NULL);

  tmpw = gtk_label_new("Strength:");
  gtk_widget_show(tmpw);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 0,1,1,2);

  smstradjust = gtk_adjustment_new(1.0, 0.1, 6.0, 0.1, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(smstradjust));
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 1);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 1,2,1,2);
  gtk_widget_show(tmpw);
  gtk_signal_connect(GTK_OBJECT(smstradjust), "value_changed",
		     (GtkSignalFunc)strsmadjmove, NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Change the strength of the selected smvector", NULL);


  tmpw = hbox = gtk_hbox_new(TRUE,0);
  gtk_table_attach_defaults(GTK_TABLE(table1), tmpw, 1,2,1,2);
  gtk_container_border_width (GTK_CONTAINER (tmpw), 2);
  gtk_widget_show(tmpw);

  tmpw = gtk_button_new_with_label("OK");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show(tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(smokclick), smwindow);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Apply and exit the editor", NULL);

  tmpw = gtk_button_new_with_label("Apply");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show(tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(smokclick), NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Apply, but stay inside the editor", NULL);

  tmpw = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX(hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show(tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(smcancelclick), smwindow);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Cancel all changes and exit", NULL);


  tmpw = table2 = gtk_table_new(2,2,FALSE);
  gtk_widget_show(tmpw);
  gtk_table_attach_defaults(GTK_TABLE(table1), tmpw, 1,2,2,3);
  gtk_widget_show(tmpw);

  tmpw = gtk_label_new("Strength exp.:");
  gtk_widget_show(tmpw);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 0,1,0,1);

  smstrexpadjust = gtk_adjustment_new(pcvals.sizestrexp, 0.1, 11.0, 0.1, 0.1, 0.1);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(smstrexpadjust));
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 1);
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 1,2,0,1);
  gtk_widget_show(tmpw);
  gtk_signal_connect(GTK_OBJECT(smstrexpadjust), "value_changed",
		     (GtkSignalFunc)smstrexpsmadjmove, NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Change the exponent of the strength", NULL);


  sizevoronoi = tmpw = gtk_check_button_new_with_label("Voronoi");
  gtk_table_attach_defaults(GTK_TABLE(table2), tmpw, 0,1,2,3);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tmpw), FALSE);
  gtk_widget_show (tmpw);
  if(pcvals.sizevoronoi)
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tmpw), TRUE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)smstrexpsmadjmove, NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Voronoi-mode makes only the smvector closest to the given point have any influence", NULL);
  
  gtk_widget_show(smwindow);

  updatesmvectorprev();
  updatesmpreviewprev();
}
