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
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimp/stdplugins-intl.h>
#include <math.h>
#include "gimpressionist.h"
#include "ppmtool.h"


GtkWidget *brushlist = NULL;
GtkWidget *brushprev = NULL;
GtkObject *brushreliefadjust = NULL;
GtkObject *brushaspectadjust = NULL;
GtkObject *brushgammaadjust = NULL;

GtkWidget *brushdrawablemenu = NULL;
GtkWidget *brushemptyitem;

gint32 brushdrawableid;

int brushfile = 2;

struct ppm brushppm = {0,0,NULL};

void updatebrushprev(char *fn);

int colorfile(char *fn)
{
  if(!fn) return 0;
  if(strstr(fn, ".ppm")) return 1;
  return 0;
}

void brushdmenuselect(gint32 id, gpointer data)
{
  GimpPixelRgn src_rgn;
  guchar *src_row;
  guchar *src;
  gint alpha, has_alpha, bpp;
  gint x, y;
  struct ppm *p;
  gint x1, y1, x2, y2;
  gint row, col;
  GimpDrawable *drawable;
  int rowstride;

  if(brushfile == 2) return; /* Not finished GUI-building yet */

  if(brushfile) {
    unselectall(brushlist);
    gtk_widget_set_sensitive (presetsavebutton, FALSE);
  }

  gtk_adjustment_set_value(GTK_ADJUSTMENT(brushgammaadjust), 1.0);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(brushaspectadjust), 0.0);

  drawable = gimp_drawable_get(id);

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  
  bpp = gimp_drawable_bpp (drawable->id);
  has_alpha = gimp_drawable_has_alpha (drawable->id);
  alpha = (has_alpha) ? bpp - 1 : bpp;

  if(brushppm.col)
    killppm(&brushppm);
  newppm(&brushppm, x2-x1, y2-y1);
  p = &brushppm;

  rowstride = p->width * 3;

  src_row = g_new (guchar, (x2 - x1) * bpp);

  gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0, x2-x1, y2-y1, FALSE, FALSE);
 
  if(bpp == 3) { /* RGB */
    int bpr = (x2-x1) * 3;
    for(row = 0, y = y1; y < y2; row++, y++) {
      gimp_pixel_rgn_get_row (&src_rgn, src_row, x1, y, (x2 - x1));
      memcpy(p->col + row*rowstride, src_row, bpr);
    }
  } else if(bpp > 3) { /* RGBA */
    for(row = 0, y = y1; y < y2; row++, y++) {
      guchar *tmprow = p->col + row * rowstride;
      gimp_pixel_rgn_get_row (&src_rgn, src_row, x1, y, (x2 - x1));
      src = src_row;
      for (col = 0, x = x1; x < x2; col++, x++) {
	int k = col * 3;
	tmprow[k+0] = src[0];
	tmprow[k+1] = src[1];
	tmprow[k+2] = src[2];
	src += src_rgn.bpp;
      }
    }  
  } else if(bpp == 2) { /* GrayA */
    for(row = 0, y = y1; y < y2; row++, y++) {
	guchar *tmprow = p->col + row * rowstride;
      gimp_pixel_rgn_get_row (&src_rgn, src_row, x1, y, (x2 - x1));
      src = src_row;
      for (col = 0, x = x1; x < x2; col++, x++) {
	int k = col * 3;
	tmprow[k+0] = src[0];
	tmprow[k+1] = src[0];
	tmprow[k+2] = src[0];
	src += src_rgn.bpp;
      }
    }  
  } else { /* Gray */
    for(row = 0, y = y1; y < y2; row++, y++) {
      guchar *tmprow = p->col + row * rowstride;
      gimp_pixel_rgn_get_row (&src_rgn, src_row, x1, y, (x2 - x1));
      src = src_row;
      for (col = 0, x = x1; x < x2; col++, x++) {
	int k = col * 3;
	tmprow[k+0] = src[0];
	tmprow[k+1] = src[0];
	tmprow[k+2] = src[0];
	src += src_rgn.bpp;
      }
    }  
  }  
  g_free (src_row);

  if(bpp >= 3) pcvals.colorbrushes = 1;
  else pcvals.colorbrushes = 0;

  brushfile = 0;
  updatebrushprev(NULL);
}

void dummybrushdmenuselect(GtkWidget *w, gpointer data)
{
  if(brushppm.col)
    killppm(&brushppm);
  newppm(&brushppm, 10,10);
  brushfile = 0;
  updatebrushprev(NULL);
}

void destroy_window (GtkWidget *widget, GtkWidget **window)
{
  *window = NULL;
}

void brushlistrefresh(void)
{
  GtkWidget *list = brushlist;
  int n = g_list_length(GTK_LIST(list)->children);
  gtk_list_clear_items(GTK_LIST(list), 0, n);
  readdirintolist("Brushes", list, NULL);
}

void savebrush_ok(GtkWidget *w, GtkFileSelection *fs, gpointer data)
{
  char *fn;
  fn = gtk_file_selection_get_filename (GTK_FILE_SELECTION(fs));

  saveppm(&brushppm, fn);

  gtk_widget_destroy(GTK_WIDGET(fs));
  brushlistrefresh();
}

void savebrush(GtkWidget *wg, gpointer data)
{
  static GtkWidget *window = NULL;
  GList *thispath = parsepath();
  char path[200];

  if(!brushppm.col) {
    g_message( _("GIMPressionist: Can only save drawables!\n"));
    return;
  }

  sprintf(path, "%s/Brushes/", (char *)thispath->data);

  window = gtk_file_selection_new( _("Save brush"));
  gtk_window_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);

  gtk_file_selection_set_filename(GTK_FILE_SELECTION(window), path);

  gtk_signal_connect (GTK_OBJECT(window), "destroy",
                      (GtkSignalFunc) destroy_window, &window);
  
  gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(window)->ok_button),
                      "clicked", (GtkSignalFunc)savebrush_ok,(gpointer)window);

  gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(window)->cancel_button),
                            "clicked", (GtkSignalFunc) gtk_widget_destroy,
			    GTK_OBJECT(window));
  gtk_widget_show(window);

  return;
}

gint validdrawable(gint32 imageid, gint32 drawableid, gpointer data)
{
  if(drawableid == -1) return TRUE;
  return (gimp_drawable_is_rgb(drawableid) || gimp_drawable_is_gray(drawableid));
}

void reloadbrush(char *fn, struct ppm *p)
{
  static char lastfn[200] = "";
  static struct ppm cache = {0,0,NULL};

  if(strcmp(fn, lastfn)) {
    strncpy(lastfn, fn, 199);
    loadppm(fn, &cache);
  }
  copyppm(&cache, p);
  pcvals.colorbrushes = colorfile(fn);
}

void padbrush(struct ppm *p, int width, int height)
{
  guchar black[3] = {0,0,0};
  int left = (width - p->width) / 2;
  int right = (width - p->width) - left;
  int top = (height - p->height) / 2;
  int bottom = (height - p->height) - top;
  pad(p, left, right, top, bottom, black);
}

void updatebrushprev(char *fn)
{
  int i, j;
  char buf[100];

  if(fn)
    brushfile = 1;

  if(!fn && brushfile) {
    memset(buf, 0, 100);
    for(i = 0; i < 100; i++) {
      gtk_preview_draw_row (GTK_PREVIEW (brushprev), buf, 0, i, 100);
    }
  } else {
    double sc;
    struct ppm p = {0,0,NULL};
    guchar gammatable[256];
    int newheight;

    if(brushfile)
      reloadbrush(fn, &p);
    else if(brushppm.col)
      copyppm(&brushppm, &p);

    pcvals.colorbrushes = colorfile(fn);

    sc = GTK_ADJUSTMENT(brushgammaadjust)->value;
    if(sc != 1.0)
      for(i = 0; i < 256; i++)
	gammatable[i] = pow(i / 255.0, sc) * 255; 
    else
      for(i = 0; i < 256; i++)
	gammatable[i] = i;

    newheight = p.height*pow(10,GTK_ADJUSTMENT(brushaspectadjust)->value);
    sc = p.width > newheight ? p.width : newheight;
    sc = 100.0 / sc;
    resize_fast(&p, p.width*sc,newheight*sc);
    padbrush(&p,100,100);
    for(i = 0; i < 100; i++) {
      int k = i * p.width * 3;
      memset(buf,0,100);
      if(i < p.height)
	for(j = 0; j < p.width; j++)
	  buf[j] = gammatable[p.col[k + j * 3]];
      gtk_preview_draw_row (GTK_PREVIEW (brushprev), buf, 0, i, 100);
    }
    killppm(&p);
  }
  gtk_widget_draw (brushprev, NULL);
}

int brushdontupdate = 0;

void selectbrush(GtkWidget *wg, GtkWidget *p)
{
  GList *h;
  GtkWidget *tmpw;
  char *l;
  static char *oldl = NULL;
  static char fname[200];

  if(brushdontupdate) return;

  if(brushfile == 0) {
    updatebrushprev(NULL);
    return;
  }

  /* Argh! Doesn't work! WHY!? :-( */
  /*
  gtk_menu_set_active(GTK_MENU(brushdrawablemenu),0);
  gtk_menu_item_select(GTK_MENU_ITEM(brushemptyitem));
  gtk_widget_draw_default(brushdrawablemenu);
  gtk_widget_bite_me(brushdrawablemenu);
  */

  if(!p) return;
  h = GTK_LIST(p)->selection;
  if(!h) return;
  tmpw = h->data;
  if(!tmpw) return;
  gtk_label_get(GTK_LABEL(GTK_BIN(tmpw)->child), &l);

  if(oldl) if(strcmp(oldl, l)) {
    brushdontupdate = 1;
    gtk_adjustment_set_value(GTK_ADJUSTMENT(brushgammaadjust), 1.0);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(brushaspectadjust), 0.0);
    brushdontupdate = 0;
  }
  oldl = l;

  sprintf(fname, "Brushes/%s", l);
  strcpy(pcvals.selectedbrush, fname);

  updatebrushprev(fname);
}

void selectbrushfile(GtkWidget *wg, GtkWidget *p)
{
  brushfile = 1;
  gtk_widget_set_sensitive (presetsavebutton, TRUE);
  selectbrush(wg,p);
}

void create_brushpage(GtkNotebook *notebook)
{
  GtkWidget *box1, *box2, *box3, *thispage;
  GtkWidget *labelbox, *menubox;
  GtkWidget *scrolled_win, *list;
  GtkWidget *tmpw;
  GtkWidget *dmenu;

  labelbox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new (_("Brush"));
  gtk_box_pack_start(GTK_BOX(labelbox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(labelbox);

  menubox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new (_("Brush"));
  gtk_box_pack_start(GTK_BOX(menubox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(menubox);

  thispage = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (thispage), 5);
  gtk_widget_show(thispage);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box1, TRUE,TRUE,0);
  gtk_widget_show (box1);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (box1), scrolled_win, FALSE, FALSE, 0);
  gtk_widget_show (scrolled_win);
  gtk_widget_set_usize(scrolled_win, 150,-1);

  brushlist = list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_BROWSE);
#if GTK_MINOR_VERSION > 0
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
#else
  gtk_container_add (GTK_CONTAINER (scrolled_win), list);
#endif
  gtk_widget_show (list);

  readdirintolist("Brushes", list, pcvals.selectedbrush);

  box2 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);
  gtk_container_border_width (GTK_CONTAINER (box2), 5);

  tmpw = gtk_label_new( _("Brush Preview:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  brushprev = tmpw = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size(GTK_PREVIEW (tmpw), 100, 100);
  gtk_box_pack_start(GTK_BOX (box2), tmpw, FALSE, FALSE, 5);
  gtk_widget_show(tmpw);

  tmpw = gtk_label_new( _("Gamma:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  brushgammaadjust = gtk_adjustment_new(pcvals.brushgamma, 0.5, 3.0, 0.1, 0.1, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(brushgammaadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 100, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), FALSE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 2);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gtk_signal_connect(GTK_OBJECT(brushgammaadjust), "value_changed",
		     (GtkSignalFunc)selectbrush, list);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Changes the gamma (brightness) of the selected brush"), NULL);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box1,FALSE,FALSE,5);
  gtk_widget_show (box1);


  box2 = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  if(!standalone) {
    tmpw = gtk_label_new( _("Select:"));
    gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
    gtk_widget_show (tmpw);
  }

  tmpw = gtk_label_new( _("Aspect ratio:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  tmpw = gtk_label_new( _("Relief:"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  box2 = gtk_vbox_new (TRUE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,10);
  gtk_widget_show (box2);

  if(!standalone) {
    GtkWidget *emptyitem;

    box3 = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(box2),box3, FALSE, FALSE, 0);
    gtk_widget_show(box3);

    brushemptyitem = emptyitem = gtk_menu_item_new_with_label( _("(None)"));
    gtk_signal_connect (GTK_OBJECT(emptyitem), "activate",
			(GtkSignalFunc)dummybrushdmenuselect, NULL);
    gtk_widget_show(emptyitem);

    tmpw = gtk_option_menu_new();
    gtk_box_pack_start(GTK_BOX(box3),tmpw, FALSE, FALSE, 0);
    gtk_widget_show(tmpw);

    brushdrawablemenu = dmenu = gimp_drawable_menu_new(validdrawable, brushdmenuselect, NULL, -1);
    gtk_menu_prepend(GTK_MENU(dmenu), emptyitem);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(tmpw), dmenu);

    tmpw = gtk_button_new_with_label( _("Save..."));
    gtk_box_pack_start(GTK_BOX(box3),tmpw, FALSE, FALSE, 0);
    gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
			GTK_SIGNAL_FUNC(savebrush), NULL);
    gtk_widget_show(tmpw);
  }

  brushaspectadjust = gtk_adjustment_new(pcvals.brushaspect, -1.0, 2.0, 0.1, 0.1, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(brushaspectadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 2);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Specifies the aspect ratio of the brush"), NULL);
  gtk_signal_connect(GTK_OBJECT(brushaspectadjust), "value_changed",
		     (GtkSignalFunc)selectbrush, list);

  brushreliefadjust = gtk_adjustment_new(pcvals.brushrelief, 0.0, 101.0, 1.0, 1.0, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(brushreliefadjust));
  gtk_widget_set_usize (GTK_WIDGET(tmpw), 150, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), TRUE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 2);
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Specifies the amount of embossing to apply to each brush stroke"), NULL);

  gtk_signal_connect (GTK_OBJECT(list), "selection_changed",
		      GTK_SIGNAL_FUNC(selectbrushfile),
		      list);
  if(!GTK_LIST(list)->selection)
    gtk_list_select_item(GTK_LIST(list), 0);
  selectbrush(NULL, list);

  gtk_notebook_append_page_menu (notebook, thispage, labelbox, menubox);
}
