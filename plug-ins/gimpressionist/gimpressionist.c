#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpressionist.h"
#include "ppmtool.h"

#include "logo-pixbuf.h"

#include "libgimp/stdplugins-intl.h"


#define RESPONSE_ABOUT 1


static GtkWidget *dlg = NULL;

ppm_t infile = {0,0,NULL};
ppm_t inalpha = {0,0,NULL};

GRand *gr;

GList * parsepath (void)
{
  static GList *lastpath = NULL;
  gchar *gimpdatasubdir, *defaultpath, *tmps;

  if (lastpath)
    return lastpath;

  gimpdatasubdir = g_build_filename (gimp_data_directory (),
                                     "gimpressionist", NULL);

  defaultpath = g_build_filename (gimp_directory (),
                                  "gimpressionist", gimpdatasubdir, NULL);

  tmps = gimp_gimprc_query ("gimpressionist-path");

  if (!tmps)
    {
      if (!g_file_test (gimpdatasubdir, G_FILE_TEST_IS_DIR))
        {
          /* No gimpressionist-path parameter,
             and the default doesn't exist */
          gchar *path = g_strconcat ("${gimp_dir}",
                                     G_DIR_SEPARATOR_S,
                                     "gimpressionist",
                                     G_SEARCHPATH_SEPARATOR_S,
                                     "${gimp_data_dir}",
                                     G_DIR_SEPARATOR_S,
                                     "gimpressionist",
                                     NULL);

          /* don't translate the gimprc entry */
          g_message (_("It is highly recommended to add\n"
                       " (gimpressionist-path \"%s\")\n"
                       "(or similar) to your gimprc file."), path);
          g_free (path);
        }
      tmps = g_strdup (defaultpath);
    }

  lastpath = gimp_path_parse (tmps, 16, FALSE, NULL);

  g_free (tmps);

  return lastpath;
}

gchar *
findfile (const gchar *fn)
{
  static GList *rcpath = NULL;
  GList        *thispath;
  gchar        *filename;

  if (!rcpath)
    rcpath = parsepath ();

  thispath = rcpath;

  while (rcpath && thispath)
    {
      filename = g_build_filename (thispath->data, fn, NULL);
      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        return filename;
      g_free (filename);
      thispath = thispath->next;
    }
  return NULL;
}

void storevals(void)
{
  pcvals.brushgamma = GTK_ADJUSTMENT(brushgammaadjust)->value;
  pcvals.generaldarkedge = GTK_ADJUSTMENT(generaldarkedgeadjust)->value;
  pcvals.paperinvert = GTK_TOGGLE_BUTTON(paperinvert)->active;
  pcvals.generalpaintedges = GTK_TOGGLE_BUTTON(generalpaintedges)->active;
  pcvals.generaltileable = GTK_TOGGLE_BUTTON(generaltileable)->active;
  pcvals.generaldropshadow = GTK_TOGGLE_BUTTON(generaldropshadow)->active;
  pcvals.generalshadowdarkness = GTK_ADJUSTMENT(generalshadowadjust)->value;
  pcvals.generalshadowdepth = GTK_ADJUSTMENT(generalshadowdepth)->value;
  pcvals.generalshadowblur = GTK_ADJUSTMENT(generalshadowblur)->value;
  pcvals.devthresh = GTK_ADJUSTMENT(devthreshadjust)->value;
  pcvals.placecenter = GTK_TOGGLE_BUTTON(placecenter)->active;
  pcvals.paperoverlay = GTK_TOGGLE_BUTTON(paperoverlay)->active;
}

void restorevals(void)
{
  reselect(brushlist, pcvals.selectedbrush);
  reselect(paperlist, pcvals.selectedpaper);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(orientnumadjust), pcvals.orientnum);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(orientfirstadjust), pcvals.orientfirst);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(orientlastadjust), pcvals.orientlast);
  orientchange(NULL, NULL, pcvals.orienttype);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(sizenumadjust), pcvals.sizenum);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(sizefirstadjust), pcvals.sizefirst);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(sizelastadjust), pcvals.sizelast);
  sizechange(NULL, NULL, pcvals.sizetype);

  placechange(pcvals.placetype);
  generalbgchange(NULL, NULL, pcvals.generalbgtype);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(brushgammaadjust), pcvals.brushgamma);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(brushreliefadjust), pcvals.brushrelief);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(brushaspectadjust), pcvals.brushaspect);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(brushdensityadjust), pcvals.brushdensity);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(generaldarkedgeadjust), pcvals.generaldarkedge);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(generalshadowadjust), pcvals.generalshadowdarkness);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(devthreshadjust), pcvals.devthresh);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(paperreliefadjust), pcvals.paperrelief);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(paperscaleadjust), pcvals.paperscale);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paperinvert), pcvals.paperinvert);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(generalpaintedges), pcvals.generalpaintedges);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(generaltileable), pcvals.generaltileable);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(generaldropshadow), pcvals.generaldropshadow);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(generalshadowdepth), pcvals.generalshadowdepth);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(generalshadowblur), pcvals.generalshadowblur);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(placecenter), pcvals.placecenter);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(paperoverlay), pcvals.paperoverlay);

  gimp_color_button_set_color (GIMP_COLOR_BUTTON(generalcolbutton),
			       &pcvals.color);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(colornoiseadjust), pcvals.colornoise);
  colorchange(pcvals.colortype);

  update_orientmap_dialog();
}

void
reselect (GtkWidget *view,
          gchar     *fname)
{
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  char *tmpfile;

  tmpfile = strrchr(fname, '/');
  if (tmpfile)
    fname = ++tmpfile;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  if (gtk_tree_model_get_iter_first (model, &iter)) {
    do {
      gchar *name;

      gtk_tree_model_get (model, &iter, 0, &name, -1);
      if (!strcmp(name, fname)) {
	gtk_tree_selection_select_iter (selection, &iter);
	g_free (name);
	break;
      }
      g_free (name);

    } while (gtk_tree_model_iter_next (model, &iter));
  }
}

static void readdirintolist_real(char *subdir, GtkWidget *view,
				 char *selected)
{
  gchar *fpath;
  const gchar *de;
  GDir *dir;
  GList *flist = NULL;
  GtkTreeIter iter;
  GtkListStore *store;
  GtkTreeSelection *selection;

  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));

  if (selected) {
    if (!selected[0])
      selected = NULL;
    else {
      char *nsel;
      nsel = strrchr(selected, '/');
      if (nsel) selected = ++nsel;
    }
  }

  dir = g_dir_open (subdir, 0, NULL);

  if (!dir)
    return;

  for(;;)
    {
      gboolean file_exists;

      de = g_dir_read_name (dir);
      if (!de)
        break;

      fpath = g_build_filename (subdir, de, NULL);
      file_exists = g_file_test (fpath, G_FILE_TEST_IS_REGULAR);
      g_free (fpath);

      if (!file_exists)
        continue;

      flist = g_list_insert_sorted(flist, g_strdup(de),
				   (GCompareFunc)g_ascii_strcasecmp);
    }
  g_dir_close(dir);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  while (flist)
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter, 0, flist->data, -1);

      if (selected)
	{
	  if (!strcmp(flist->data, selected))
	    {
	      gtk_tree_selection_select_iter (selection, &iter);
	    }
	}
      g_free (flist->data);
      flist = g_list_remove (flist, flist->data);
    }

  if (!selected)
    {
      if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
	gtk_tree_selection_select_iter (selection, &iter);
    }
}

void readdirintolist(char *subdir, GtkWidget *view, char *selected)
{
  char *tmpdir;
  GList *thispath = parsepath();

  while (thispath)
    {
      tmpdir = g_build_filename ((gchar *) thispath->data, subdir, NULL);
      readdirintolist_real (tmpdir, view, selected);
      g_free (tmpdir);
      thispath = thispath->next;
    }
}

GtkWidget *createonecolumnlist(GtkWidget *parent,
			       void (*changed_cb)
			       (GtkTreeSelection *selection, gpointer data))
{
  GtkListStore *store;
  GtkTreeSelection *selection;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkWidget *swin, *view;

  swin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(swin),
				       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (parent), swin, FALSE, FALSE, 0);
  gtk_widget_show (swin);
  gtk_widget_set_size_request(swin, 150,-1);

  store = gtk_list_store_new (1, G_TYPE_STRING);
  view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
  g_object_unref (store);
  gtk_widget_show (view);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Preset", renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

  gtk_container_add (GTK_CONTAINER (swin), view);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
  g_signal_connect (selection, "changed", G_CALLBACK (changed_cb),
		    NULL);

  return view;
}

static void
showabout (void)
{
  static GtkWidget *window = NULL;
  GtkWidget *tmpw, *tmphbox;
  GtkWidget *logobox, *tmpframe;
  GtkWidget *logo;
  GdkPixbuf *pixbuf;

  if (window)
    {
      gtk_window_present (GTK_WINDOW (window));
      return;
    }

  window =
    gimp_dialog_new (_("The GIMPressionist"), "gimpressionist",
                     NULL, 0,
		     gimp_standard_help_func, HELP_ID,

		     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,

		     NULL);

  gtk_container_set_border_width (GTK_CONTAINER (window), 6);

  g_signal_connect (window, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);
  g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &window);

  tmphbox = gtk_hbox_new(TRUE, 5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), tmphbox,
		     TRUE, TRUE, 0);
  gtk_widget_show(tmphbox);

  logobox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (tmphbox), logobox, FALSE, FALSE, 0);
  gtk_widget_show(logobox);

  tmpframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (tmpframe), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (logobox), tmpframe, TRUE, TRUE, 0);
  gtk_widget_show(tmpframe);

  pixbuf = gdk_pixbuf_new_from_inline (-1, logo_data, FALSE, NULL);
  logo = gtk_image_new_from_pixbuf (pixbuf);
  g_object_unref (pixbuf);

  gtk_container_add (GTK_CONTAINER (tmpframe), logo);
  gtk_widget_show (logo);

  tmphbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), tmphbox,
		     TRUE, TRUE, 0);

  tmpw = gtk_label_new("(C) 1999 Vidar Madsen\n"
		       "vidar@prosalg.no\n"
		       "http://www.prosalg.no/~vidar/gimpressionist/\n"
		       PLUG_IN_VERSION);
  gtk_box_pack_start(GTK_BOX(tmphbox), tmpw, TRUE, FALSE, 0);
  gtk_widget_show(tmpw);

  gtk_widget_show(tmphbox);
  gtk_widget_show(window);
}

static void
dialog_response (GtkWidget *widget,
                 gint       response_id,
                 gpointer   data)
{
  switch (response_id)
    {
    case RESPONSE_ABOUT:
      showabout ();
      break;

    case GTK_RESPONSE_OK:
      storevals ();
      pcvals.run = TRUE;
      gtk_widget_destroy (widget);
      break;

    default:
      gtk_widget_destroy (widget);
      break;
    }
}

static GtkWidget *
create_dialog (void)
{
  GtkWidget *notebook;
  GtkWidget *box1, *box2, *preview_box;

  gimp_ui_init ("gimpressionist", TRUE);

  dlg = gimp_dialog_new (_("Gimpressionist"), "gimpressionist",
                         NULL, 0,
			 gimp_standard_help_func, HELP_ID,

			 _("A_bout"),      RESPONSE_ABOUT,
			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK,     GTK_RESPONSE_OK,

			 NULL);

  g_signal_connect (dlg, "response",
                    G_CALLBACK (dialog_response),
                    NULL);
  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), box1);
  gtk_widget_show (box1);

  box2 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
  gtk_widget_show (box2);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (box1), notebook, TRUE, TRUE, 5);
  gtk_widget_show(notebook);

  create_presetpage (GTK_NOTEBOOK (notebook));
  create_paperpage (GTK_NOTEBOOK (notebook));
  create_brushpage (GTK_NOTEBOOK (notebook));
  create_orientationpage (GTK_NOTEBOOK (notebook));
  create_sizepage (GTK_NOTEBOOK (notebook));
  create_placementpage (GTK_NOTEBOOK (notebook));
  create_colorpage (GTK_NOTEBOOK (notebook));
  create_generalpage (GTK_NOTEBOOK (notebook));

  preview_box = create_preview ();
  gtk_box_pack_start (GTK_BOX (box2), preview_box, FALSE, FALSE, 0);
  gtk_widget_show (preview_box);

  gtk_widget_show (dlg);

  return dlg;
}

gint
create_gimpressionist (void)
{
  pcvals.run = FALSE;

  create_dialog ();

  gtk_main ();

  return pcvals.run;
}
