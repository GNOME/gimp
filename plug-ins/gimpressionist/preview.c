#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_DIRENT_H
#define HAVE_UNISTD_H
#endif
#include <string.h>
#include <gtk/gtk.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "gimpressionist.h"
#include "ppmtool.h"

GtkWidget *previewprev = NULL;
GtkWidget *previewbutton = NULL;
GtkWidget *resetpreviewbutton = NULL;

void drawalpha(struct ppm *p, struct ppm *a)
{
  int x, y, g;
  double v;
  int gridsize = 16;
  int rowstride = p->width * 3;

  for(y = 0; y < p->height; y++) {
    for(x = 0; x < p->width; x++) {
      int k = y*rowstride + x*3;
      if(!a->col[k]) continue;
      v = 1.0 - a->col[k] / 255.0;
      g = ((x/gridsize+y/gridsize)%2)*60+100;
      p->col[k+0] *= v;
      p->col[k+1] *= v;
      p->col[k+2] *= v;
      v = 1.0-v;
      p->col[k+0] += g*v;
      p->col[k+1] += g*v;
      p->col[k+2] += g*v;
    }
  }
  
  return;
}

void updatepreviewprev(GtkWidget *wg, void *d)
{
  int i;
  char buf[PREVIEWSIZE*3];
  static struct ppm p = {0,0,NULL};
  static struct ppm a = {0,0,NULL};
  static struct ppm backup = {0,0,NULL};
  static struct ppm abackup = {0,0,NULL};

  if(!infile.col && d) grabarea();

  if(!infile.col && !d) {
    memset(buf, 0, PREVIEWSIZE*3);
    for(i = 0; i < PREVIEWSIZE; i++) {
      gtk_preview_draw_row (GTK_PREVIEW(previewprev), buf, 0, i, PREVIEWSIZE);
    }
  } else {
    if(!backup.col) {
      copyppm(&infile, &backup);
      if((backup.width != PREVIEWSIZE) || (backup.height != PREVIEWSIZE))
	resize_fast(&backup, PREVIEWSIZE, PREVIEWSIZE);
      if(img_has_alpha) {
	copyppm(&inalpha, &abackup);
	if((abackup.width != PREVIEWSIZE) || (abackup.height != PREVIEWSIZE))
	  resize_fast(&abackup, PREVIEWSIZE, PREVIEWSIZE);
      }
    }
    if(!p.col) {
      copyppm(&backup, &p);
      if(img_has_alpha)
	copyppm(&abackup, &a);
    }
    if(d) {
      storevals();

      if(d != (void *)2)
	repaint(&p, &a);
    }
    if(img_has_alpha)
      drawalpha(&p, &a);

    for(i = 0; i < PREVIEWSIZE; i++) {
      memset(buf,0,PREVIEWSIZE*3);
      /*for(j = 0; j < p.width; j++)*/
      gtk_preview_draw_row(GTK_PREVIEW(previewprev), (guchar *)&p.col[i*PREVIEWSIZE*3], 0, i, PREVIEWSIZE);
    }
    killppm(&p);
    if(img_has_alpha)
      killppm(&a);
  }
  gtk_widget_draw (previewprev, NULL);
}

GtkWidget* create_preview()
{
  GtkWidget *box1, *box2, *tmpw;

  box1 = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (box1), 5);

  tmpw = gtk_label_new("Preview");
  gtk_box_pack_start(GTK_BOX(box1), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);

  previewprev = tmpw = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size(GTK_PREVIEW (tmpw), PREVIEWSIZE, PREVIEWSIZE);
  gtk_box_pack_start(GTK_BOX (box1), tmpw, FALSE, FALSE, 5);
  gtk_widget_show(tmpw);

  box2 = gtk_hbox_new(TRUE, 0);

  previewbutton = tmpw = gtk_button_new_with_label(" Update ");
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
                     (GtkSignalFunc)updatepreviewprev, (void *)1);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, TRUE, TRUE, 0);
  gtk_widget_show(tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Refresh the Preview window", NULL);

  resetpreviewbutton = tmpw = gtk_button_new_with_label(" Reset ");
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
                     (GtkSignalFunc)updatepreviewprev, (void *)2);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, TRUE, TRUE, 0);
  gtk_widget_show(tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Revert to the original image", NULL);

  gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
  gtk_widget_show(box2);

  updatepreviewprev(NULL, 0);

  return box1;
}
