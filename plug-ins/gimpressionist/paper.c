#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_DIRENT_H
#define HAVE_UNISTD_H
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "gimpressionist.h"
#include "ppmtool.h"
#include <libgimp/stdplugins-intl.h>


GtkWidget *paperlist = NULL;
GtkWidget *paperprev = NULL;
GtkObject *paperreliefadjust = NULL;
GtkObject *paperscaleadjust = NULL;
GtkWidget *paperinvert = NULL;
GtkWidget *paperoverlay = NULL;

void updatepaperprev(char *fn)
{
  int i, j;
  char buf[100];

  if(!fn) {
    memset(buf, 0, 100);
    for(i = 0; i < 100; i++) {
      gtk_preview_draw_row (GTK_PREVIEW (paperprev), buf, 0, i, 100);
    }

  } else {
    double sc;
    struct ppm p = {0,0,NULL};
    loadppm(fn, &p);
    sc = p.width > p.height ? p.width : p.height;
    sc = 100.0 / sc;
    resize(&p, p.width*sc,p.height*sc);
    for(i = 0; i < 100; i++) {
      int k = i * p.width * 3;
      memset(buf,0,100);
      if(i < p.height) {
	for(j = 0; j < p.width; j++)
	  buf[j] = p.col[k + j * 3];
	if(GTK_TOGGLE_BUTTON(paperinvert)->active)
	  for(j = 0; j < p.width; j++)
	    buf[j] = 255 - buf[j];
      }
      gtk_preview_draw_row (GTK_PREVIEW (paperprev), buf, 0, i, 100);
    }
    killppm(&p);
  }
  gtk_widget_draw (paperprev, NULL);
}

void selectpaper(GtkWidget *wg, GtkWidget *p)
{
  GList *h = GTK_LIST(p)->selection;
  GtkWidget *tmpw;
  char *l;
  static char fname[200];

  if(!h) return;
  tmpw = h->data;
  if(!tmpw) return;

  gtk_label_get(GTK_LABEL(GTK_BIN(tmpw)->child), &l);

  sprintf(fname, "Paper/%s", l);
  strcpy(pcvals.selectedpaper, fname);
  updatepaperprev(fname);
}



void create_paperpage(GtkNotebook *notebook)
{
  GtkWidget *box1, *thispage, *box2;
  GtkWidget *labelbox, *menubox;
  GtkWidget *scrolled_win, *list;
  GtkWidget *tmpw;

  labelbox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new (_("Paper"));
  gtk_box_pack_start(GTK_BOX(labelbox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(labelbox);

  menubox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new (_("Paper"));
  gtk_box_pack_start(GTK_BOX(menubox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(menubox);

  thispage = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 5);
  gtk_widget_show(thispage);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box1, TRUE, TRUE, 0);
  gtk_widget_show (box1);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (box1), scrolled_win, FALSE, FALSE, 0);
  gtk_widget_show (scrolled_win);
  gtk_widget_set_usize(scrolled_win, 150,-1);

  paperlist = list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_BROWSE);
#if GTK_MINOR_VERSION > 0
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(scrolled_win), list);
#else
  gtk_container_add (GTK_CONTAINER (scrolled_win), list);
#endif
  gtk_widget_show (list);

  readdirintolist("Paper", list, pcvals.selectedpaper);

  box2 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);
  gtk_container_set_border_width (GTK_CONTAINER (box2), 5);

  tmpw = gtk_label_new( _("Paper Preview:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  paperprev = tmpw = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size(GTK_PREVIEW (tmpw), 100, 100);
  gtk_box_pack_start(GTK_BOX (box2), tmpw, FALSE, FALSE, 5);
  gtk_widget_show(tmpw);

  /* updatepaperprev(NULL); */

  paperinvert = tmpw = gtk_check_button_new_with_label( _("Invert"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw), FALSE);
  gtk_widget_show (tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
                      GTK_SIGNAL_FUNC(selectpaper),
                      list);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Inverts the Papers texture"), NULL);
  if(pcvals.paperinvert)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw), TRUE);

  paperoverlay = tmpw = gtk_check_button_new_with_label( _("Overlay"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw), FALSE);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Applies the paper as it is (without embossing it)"), NULL);
  if(pcvals.paperoverlay)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw), TRUE);


  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box1,FALSE,FALSE,5);
  gtk_widget_show (box1);

  box2 = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  tmpw = gtk_label_new( _("Scale:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  tmpw = gtk_label_new( _("Relief:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  box2 = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,10);
  gtk_widget_show (box2);

  paperscaleadjust = gtk_adjustment_new(pcvals.paperscale, 3.0, 151.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(paperscaleadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 2);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Specifies the scale of the texture (in percent of original file)"), NULL);

  paperreliefadjust = gtk_adjustment_new(pcvals.paperrelief, 0.0, 101.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(paperreliefadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 2);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Specifies the amount of embossing to apply to the image (in percent)"), NULL);


  gtk_signal_connect (GTK_OBJECT(list), "selection_changed",
		      GTK_SIGNAL_FUNC(selectpaper),
		      list);
  if(!GTK_LIST(list)->selection)
    gtk_list_select_item(GTK_LIST(list), 0);
  selectpaper(NULL,list);

  gtk_notebook_append_page_menu (notebook, thispage, labelbox, menubox);

}
