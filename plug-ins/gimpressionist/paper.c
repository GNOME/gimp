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

#include <libgimp/gimp.h>
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
static GtkListStore *paperstore;

static void updatepaperprev(char *fn)
{
  gint    i, j;
  guchar  buf[100];
  gdouble sc;
  ppm_t   p = {0,0,NULL};

  loadppm(fn, &p);
  sc = p.width > p.height ? p.width : p.height;
  sc = 100.0 / sc;
  resize(&p, p.width*sc,p.height*sc);
  for (i = 0; i < 100; i++) {
    int k = i * p.width * 3;
    memset(buf, 0, 100);
    if(i < p.height) {
      for(j = 0; j < p.width; j++)
	buf[j] = p.col[k + j * 3];
      if (GTK_TOGGLE_BUTTON(paperinvert)->active)
	for (j = 0; j < p.width; j++)
	  buf[j] = 255 - buf[j];
    }
    gtk_preview_draw_row (GTK_PREVIEW (paperprev), buf, 0, i, 100);
  }
  killppm(&p);

  gtk_widget_draw (paperprev, NULL);
}

static void selectpaper(GtkTreeSelection *selection, gpointer data)
{
  GtkTreeIter   iter;
  GtkTreeModel *model;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gchar *paper;

      gtk_tree_model_get (model, &iter, 0, &paper, -1);

      if (paper)
        {
          gchar *fname = g_build_filename ("Paper", paper, NULL);

          g_strlcpy (pcvals.selectedpaper,
                     fname, sizeof (pcvals.selectedpaper));

          updatepaperprev (fname);

          g_free (fname);
          g_free (paper);
        }
    }
}

void create_paperpage(GtkNotebook *notebook)
{
  GtkWidget *box1, *thispage, *box2;
  GtkWidget *label, *tmpw, *table;
  GtkWidget *view;
  GtkTreeSelection *selection;
  GtkTreeIter iter;

  label = gtk_label_new_with_mnemonic (_("P_aper"));

  thispage = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 5);
  gtk_widget_show(thispage);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box1, TRUE, TRUE, 0);
  gtk_widget_show (box1);

  paperlist = view = createonecolumnlist (box1, selectpaper);
  paperstore = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW (view)));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

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
  g_signal_connect_swapped (tmpw, "clicked", 
			    G_CALLBACK(selectpaper), selection);
  gimp_help_set_help_data (tmpw, _("Inverts the Papers texture"), NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw), pcvals.paperinvert);

  paperoverlay = tmpw = gtk_check_button_new_with_mnemonic( _("O_verlay"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tmpw), FALSE);
  gtk_widget_show (tmpw);
  gimp_help_set_help_data 
    (tmpw, _("Applies the paper as it is (without embossing it)"), NULL);
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


  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL(paperstore), &iter))
    gtk_tree_selection_select_iter (selection, &iter);

  selectpaper(selection, NULL);
  readdirintolist("Paper", view, pcvals.selectedpaper);
  gtk_notebook_append_page_menu (notebook, thispage, label, NULL);
}
