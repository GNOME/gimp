#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_DIRENT_H
#define HAVE_UNISTD_H
#endif

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "gimpressionist.h"
#include "ppmtool.h"


#define NUMPLACERADIO 2

GtkWidget *placeradio[NUMPLACERADIO];
GtkWidget *placecenter = NULL;
GtkObject *brushdensityadjust = NULL;


void placechange(GtkWidget *wg, void *d, int num)
{
  int n;
  if(wg) {
    n = (long)d;
    pcvals.placetype = n;
  } else {
    int i;
    n = num;
    for(i = 0; i < NUMPLACERADIO; i++)
      if(i != n)
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(placeradio[i]), FALSE);
      else
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(placeradio[n]), TRUE);
  }
}

void create_placementpage(GtkNotebook *notebook)
{
  GtkWidget *box0, *box1, *box2, *box3, *thispage;
  GtkWidget *labelbox, *menubox;
  GtkWidget *tmpw;
  char title[100];
  int i;

  sprintf(title, "Placement");

  labelbox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new(title);
  gtk_box_pack_start(GTK_BOX(labelbox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(labelbox);

  menubox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new(title);
  gtk_box_pack_start(GTK_BOX(menubox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(menubox);

  thispage = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (thispage), 5);
  gtk_widget_show(thispage);

  box0 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box0,FALSE,FALSE,0);
  gtk_widget_show (box0);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box0), box1,FALSE,FALSE,0);
  gtk_widget_show (box1);

  box2 = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  tmpw = gtk_label_new("Placement:");
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  box3 = gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box1), box3,FALSE,FALSE, 10);
  gtk_widget_show(box3);

  i = pcvals.placetype;

  placeradio[0] = tmpw = gtk_radio_button_new_with_label(NULL, "Randomly");
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_widget_show(tmpw);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)placechange, (void *)0);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Place strokes randomly around the image", NULL);
  if(i == 0)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  placeradio[1] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), "Evenly distributed");
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)placechange, (void *)1);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "The strokes are evenly distributed across the image", NULL);
  if(i == 1)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);


  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box0), box1,FALSE,FALSE,0);
  gtk_widget_show (box1);

  tmpw = gtk_label_new("Stroke density:");
  gtk_box_pack_start(GTK_BOX(box1), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  brushdensityadjust = gtk_adjustment_new(pcvals.brushdensity, 1.0, 51.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(brushdensityadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 2);
  gtk_box_pack_start (GTK_BOX (box1), tmpw, FALSE, FALSE, 10);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "The relative density of the brush strokes", NULL);

  placecenter = tmpw = gtk_check_button_new_with_label("Centerize");
  gtk_box_pack_start(GTK_BOX(box0), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tmpw), FALSE);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Focus the brush strokes around the center of the image", NULL);
  if(pcvals.placecenter)
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tmpw), TRUE);

    
  gtk_notebook_append_page_menu (notebook, thispage, labelbox, menubox);
}
