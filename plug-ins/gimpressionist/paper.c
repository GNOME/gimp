#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include <libgimp/gimpui.h>

#include "gimpressionist.h"
#include "ppmtool.h"

#include "libgimp/stdplugins-intl.h"


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
    ppm_t p = {0,0,NULL};
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

static void selectpaper(GtkWidget *wg, GtkWidget *p)
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
  GtkWidget *label, *tmpw, *table;
  GtkWidget *scrolled_win, *list;

  label = gtk_label_new_with_mnemonic (_("P_aper"));

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
  gtk_widget_set_size_request(scrolled_win, 150,-1);

  paperlist = list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_BROWSE);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(scrolled_win),
                                         list);

  gtk_widget_show (list);

  readdirintolist("Paper", list, pcvals.selectedpaper);

  box2 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, FALSE, 0);
  gtk_widget_show (box2);
  gtk_container_set_border_width (GTK_CONTAINER (box2), 5);

  tmpw = gtk_label_new( _("Paper Preview:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  paperprev = tmpw = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size(GTK_PREVIEW (tmpw), 100, 100);
  gtk_box_pack_start(GTK_BOX (box2), tmpw, FALSE, FALSE, 5);
  gtk_widget_show(tmpw);

  paperinvert = tmpw = gtk_check_button_new_with_mnemonic( _("_Invert"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw), FALSE);
  gtk_widget_show (tmpw);
  g_signal_connect (G_OBJECT(tmpw), "clicked", G_CALLBACK(selectpaper), list);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, 
		       _("Inverts the Papers texture"), NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw), pcvals.paperinvert);

  paperoverlay = tmpw = gtk_check_button_new_with_mnemonic( _("O_verlay"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw), FALSE);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, 
		       _("Applies the paper as it is (without embossing it)"), 
		       NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw), pcvals.paperoverlay);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box1,FALSE,FALSE,5);
  gtk_widget_show (box1);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE(table), 4);
  gtk_box_pack_start(GTK_BOX(box1), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  paperscaleadjust = 
    gimp_scale_entry_new (GTK_TABLE(table), 0, 0, 
			  _("Scale:"),
			  150, -1, pcvals.paperscale, 
			  3.0, 150.0, 1.0, 10.0, 1, 
			  TRUE, 0, 0,
			  _("Specifies the scale of the texture (in percent of original file)"),
			  NULL);
  g_signal_connect (paperscaleadjust, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pcvals.paperscale);

  paperreliefadjust = 
    gimp_scale_entry_new (GTK_TABLE(table), 0, 1, 
			  _("Relief:"),
			  150, -1, pcvals.paperrelief, 
			  0.0, 100.0, 1.0, 10.0, 1, 
			  TRUE, 0, 0,
			  _("Specifies the amount of embossing to apply to the image (in percent)"),
			  NULL);
  g_signal_connect (paperreliefadjust, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pcvals.paperrelief);

  g_signal_connect (G_OBJECT(list), "selection_changed", 
		    G_CALLBACK(selectpaper), list);
  if (!GTK_LIST(list)->selection)
    gtk_list_select_item(GTK_LIST(list), 0);
  selectpaper(NULL,list);

  gtk_notebook_append_page_menu (notebook, thispage, label, NULL);
}
