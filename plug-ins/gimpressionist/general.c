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
#include <libgimp/stdplugins-intl.h>
#include "gimpressionist.h"
#include "ppmtool.h"

#define COLORBUTTONWIDTH 30
#define COLORBUTTONHEIGHT 20

GtkObject *generaldarkedgeadjust = NULL;
GtkWidget *generalpaintedges = NULL;
GtkWidget *generaltileable = NULL;
GtkWidget *generaldropshadow = NULL;
GtkObject *generalshadowadjust = NULL;
GtkObject *generalshadowdepth = NULL;
GtkObject *generalshadowblur = NULL;
GtkObject *devthreshadjust = NULL;

#define NUMGENERALBGRADIO 4

GtkWidget *generalbgradio[NUMGENERALBGRADIO];

void generalbgchange(GtkWidget *wg, void *d, int num)
{
  int n;

  if(wg) {
    n = (long)d;
    if(!img_has_alpha && (n == 3))
      n = 1;
    pcvals.generalbgtype = n;
  } else {
    int i;
    n = num;
    if(!img_has_alpha && (n == 3))
      n = 1;
    for(i = 0; i < NUMGENERALBGRADIO; i++)
      if(i != n)
        gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(generalbgradio[i]), FALSE);
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(generalbgradio[n]), TRUE);
  }
}

void drawcolor(GtkWidget *w)
{
  static GtkWidget *lastw = NULL;
  int x, y;
  guchar buf[COLORBUTTONWIDTH*3];

  if(w) lastw = w;
  else w = lastw;
  if(!w) return;

  for(x = 0; x < COLORBUTTONWIDTH; x++)
    memcpy(&buf[x*3], &pcvals.color, 3);
    
  for(y = 0; y < COLORBUTTONHEIGHT; y++)
    gtk_preview_draw_row (GTK_PREVIEW(w), buf, 0, y, COLORBUTTONWIDTH);
  gtk_widget_draw(w, NULL);
}

void selectcolor_ok(GtkWidget *w, gpointer d)
{
  GtkWidget *win = (GtkWidget *)d;
  gdouble tmpcol[3];
  gtk_color_selection_get_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (win)->colorsel), tmpcol);
  pcvals.color[0] = tmpcol[0] * 255.0;
  pcvals.color[1] = tmpcol[1] * 255.0;
  pcvals.color[2] = tmpcol[2] * 255.0;
  drawcolor(NULL);
  gtk_widget_destroy(win);
}

void selectcolor(void)
{
  static GtkWidget *window = NULL;
  gdouble tmpcol[3];

  if(window) {
    gtk_widget_show(window);
    gdk_window_raise(window->window);
    return;
  }

  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(generalbgradio[0]), TRUE);

  window = gtk_color_selection_dialog_new( _("Color Selection Dialog"));
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC(gtk_widget_destroyed),
		      &window);

  tmpcol[0] = pcvals.color[0] / 255.0;
  tmpcol[1] = pcvals.color[1] / 255.0;
  tmpcol[2] = pcvals.color[2] / 255.0;

  gtk_color_selection_set_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(window)->colorsel), tmpcol);

  /*
  gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG (window)->colorsel),
		     "color_changed",
		     GTK_SIGNAL_FUNC(color_selection_changed),
		     window);
		     */
  gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(window)->ok_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(selectcolor_ok),
		     window);
  gtk_signal_connect_object(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(window)->cancel_button),
			    "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT (window));
  gtk_widget_show (window);
}

void create_generalpage(GtkNotebook *notebook)
{
  GtkWidget *box1, *box2, *box3, *box4, *thispage;
  GtkWidget *labelbox, *menubox;
  GtkWidget *tmpw, *colbutton;

  labelbox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new (_("General"));
  gtk_box_pack_start(GTK_BOX(labelbox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(labelbox);

  menubox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new (_("General"));
  gtk_box_pack_start(GTK_BOX(menubox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(menubox);


  thispage = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (thispage), 5);
  gtk_widget_show(thispage);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box1,FALSE,FALSE,0);
  gtk_widget_show (box1);

  box2 = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  tmpw = gtk_label_new( _("Edge darken:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);



  box2 = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE, 10);
  gtk_widget_show (box2);

  generaldarkedgeadjust = gtk_adjustment_new(pcvals.generaldarkedge, 0.0, 2.0, 0.1, 0.1, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(generaldarkedgeadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 2);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("How much to \"darken\" the edges of each brush stroke"), NULL);


  box2 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  tmpw = gtk_label_new( _("Background:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  box3 = gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box2), box3,FALSE,FALSE, 10);
  gtk_widget_show(box3);

  generalbgradio[1] = tmpw = gtk_radio_button_new_with_label(NULL, _("Keep original"));
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)generalbgchange, (void *)1);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Preserve the original image as a background"), NULL);
  if(pcvals.generalbgtype == 1)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  generalbgradio[2] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("From paper"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)generalbgchange, (void *)2);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Copy the texture of the selected paper as a background"), NULL);
  if(pcvals.generalbgtype == 2)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  box3 = gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box2), box3,FALSE,FALSE, 10);
  gtk_widget_show(box3);


  box4 = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box3), box4, FALSE, FALSE, 0);
  gtk_widget_show(box4);

  generalbgradio[0] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(tmpw)), _("Solid"));
  gtk_box_pack_start(GTK_BOX(box4), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)generalbgchange, (void *)0);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Solid colored background"), NULL);
  if(pcvals.generalbgtype == 0)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);

  colbutton = tmpw = gtk_button_new();
  gtk_box_pack_start(GTK_BOX(box4), tmpw, FALSE, FALSE, 10);
  gtk_widget_show(tmpw);
  gtk_signal_connect (GTK_OBJECT (tmpw), "clicked",
		      (GtkSignalFunc)selectcolor,
		      NULL);

  tmpw = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (tmpw), COLORBUTTONWIDTH, COLORBUTTONHEIGHT);
  gtk_container_add (GTK_CONTAINER (colbutton), tmpw);
  gtk_widget_show(tmpw);
  drawcolor(tmpw);

  generalbgradio[3] = tmpw = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(generalbgradio[0])), _("Transparent"));
  gtk_box_pack_start(GTK_BOX(box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)generalbgchange, (void *)3);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Use a transparent background; Only the strokes painted will be visible"), NULL);
  if(!img_has_alpha)
    gtk_widget_set_sensitive (tmpw, FALSE);
  if(pcvals.generalbgtype == 3)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (tmpw), TRUE);


  box1 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box1,FALSE,FALSE,0);
  gtk_widget_show (box1);

  box2 = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  generalpaintedges = tmpw = gtk_check_button_new_with_label( _("Paint edges"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tmpw), FALSE);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Selects if to place strokes all the way out to the edges of the image", NULL);
  if(pcvals.generalpaintedges)
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tmpw), TRUE);

  generaltileable = tmpw = gtk_check_button_new_with_label( _("Tileable"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tmpw), FALSE);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Selects if the resulting image should be seamlessly tileable"), NULL);
  if(pcvals.generaltileable)
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tmpw), TRUE);

  box2 = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  generaldropshadow = tmpw = gtk_check_button_new_with_label( _("Drop Shadow"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tmpw), FALSE);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Adds a shadow effect to each brush stroke"), NULL);
  if(pcvals.generaldropshadow)
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(tmpw), TRUE);

  generalshadowadjust = gtk_adjustment_new(pcvals.generalshadowdarkness, 0.0, 100.0, 0.1, 0.1, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(generalshadowadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 2);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("How much to \"darken\" the drop shadow"), NULL);

  box2 = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  tmpw = gtk_label_new( _("Shadow depth:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  generalshadowdepth = gtk_adjustment_new(pcvals.generalshadowdarkness, 0.0, 100.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(generalshadowdepth));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 0);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("The depth of the drop shadow, i.e. how far apart from the object it should be"), NULL);

  box2 = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  tmpw = gtk_label_new( _("Shadow blur:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  generalshadowblur = gtk_adjustment_new(pcvals.generalshadowdarkness, 0.0, 100.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(generalshadowblur));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 0);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("How much to blur the drop shadow"), NULL);

  box2 = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  tmpw = gtk_label_new( _("Deviation threshold:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  devthreshadjust = gtk_adjustment_new(pcvals.devthresh, 0.0, 2.0, 0.1, 0.1, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(devthreshadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 2);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("A bailout-value for adaptive selections"), NULL);

  gtk_notebook_append_page_menu (notebook, thispage, labelbox, menubox);
}
