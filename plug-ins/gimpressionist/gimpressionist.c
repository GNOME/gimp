#ifdef HAVE_CONFIG_H
#include "config.h"
#else

#define HAVE_DIRENT_H
#define HAVE_UNISTD_H
#endif
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpressionist.h"
#include "ppmtool.h"

#include "libgimp/stdplugins-intl.h"


#ifdef G_OS_WIN32
# ifndef S_ISDIR
#  define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#  define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
# endif
#endif

GtkWidget *window = NULL;
GtkTooltips *tooltips = NULL;

struct ppm infile = {0,0,NULL};
struct ppm inalpha = {0,0,NULL};

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

  if (standalone)
    {
      tmps = g_strdup (defaultpath);
    }
  else
    {
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
  pcvals.orientnum = GTK_ADJUSTMENT(orientnumadjust)->value;
  pcvals.orientfirst = GTK_ADJUSTMENT(orientfirstadjust)->value;
  pcvals.orientlast = GTK_ADJUSTMENT(orientlastadjust)->value;
  pcvals.sizenum = GTK_ADJUSTMENT(sizenumadjust)->value;
  pcvals.sizefirst = GTK_ADJUSTMENT(sizefirstadjust)->value;
  pcvals.sizelast = GTK_ADJUSTMENT(sizelastadjust)->value;
  pcvals.brushrelief = GTK_ADJUSTMENT(brushreliefadjust)->value;
  pcvals.brushaspect = GTK_ADJUSTMENT(brushaspectadjust)->value;
  pcvals.brushdensity = GTK_ADJUSTMENT(brushdensityadjust)->value;
  pcvals.brushgamma = GTK_ADJUSTMENT(brushgammaadjust)->value;
  pcvals.generaldarkedge = GTK_ADJUSTMENT(generaldarkedgeadjust)->value;
  pcvals.paperrelief = GTK_ADJUSTMENT(paperreliefadjust)->value;
  pcvals.paperscale = GTK_ADJUSTMENT(paperscaleadjust)->value;
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
  pcvals.colornoise = GTK_ADJUSTMENT(colornoiseadjust)->value;
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

  placechange(NULL, NULL, pcvals.placetype);
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

  drawcolor(NULL);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(colornoiseadjust), pcvals.colornoise);
  colorchange(NULL, NULL, pcvals.colortype);

  update_orientmap_dialog();
}

static void
dialog_close_callback(GtkWidget *widget, gpointer data)
{
  gtk_main_quit();
}

static void dialog_ok_callback(GtkWidget *widget, gpointer data)
{
  storevals();
  pcvals.run = 1;
  gtk_widget_destroy(GTK_WIDGET(data));
  if(omwindow)
    gtk_widget_destroy(GTK_WIDGET(omwindow));
}

static void dialog_cancel_callback(GtkWidget *widget, gpointer data)
{
  pcvals.run = 0;
  gtk_widget_destroy(GTK_WIDGET(data));
}

void unselectall(GtkWidget *list)
{
  GList *h;
  GtkWidget *tmpw;
  for(;;) {
    h = GTK_LIST(list)->selection;
    if(!h) break;
    tmpw = h->data;
    if(!tmpw) break;
    gtk_list_unselect_child(GTK_LIST(list), tmpw);
  }
}

void reselect(GtkWidget *list, char *fname)
{
  GList *h;
  GtkWidget *tmpw;
  char *tmps, *tmpfile;

  tmpfile = strrchr(fname, '/');
  if(tmpfile)
    fname = ++tmpfile;  

  unselectall(list);
  /*
  for(;;) {
    h = GTK_LIST(list)->selection;
    if(!h) break;
    tmpw = h->data;
    if(!tmpw) break;
    gtk_list_unselect_child(GTK_LIST(list), tmpw);
  }
  */  
  h = GTK_LIST(list)->children;
  while(h) {
    tmpw = h->data;
    gtk_label_get(GTK_LABEL(GTK_BIN(tmpw)->child), &tmps);
    if(!strcmp(tmps, fname)) {
      gtk_list_select_child(GTK_LIST(list), tmpw);
      break;
    }
    h = g_list_next(h);
  }
}

void readdirintolist_real(char *subdir, GtkWidget *list, char *selected)
{
  gchar *fpath;
  const gchar *de;
  GtkWidget *selectedw = NULL, *tmpw;
  GDir *dir;
  GList *flist = NULL;
 
  if(selected) {
    if(!selected[0])
      selected = NULL;
    else {
      char *nsel;
      nsel = strrchr(selected, '/');
      if(nsel) selected = ++nsel;
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
      
      flist = g_list_insert_sorted(flist, g_strdup(de), (GCompareFunc)g_ascii_strcasecmp);
    }
  g_dir_close(dir);

  while(flist) {
    tmpw = gtk_list_item_new_with_label(flist->data);
    if(selected)
      if(!strcmp(flist->data, selected))
	selectedw = tmpw;
    gtk_container_add(GTK_CONTAINER(list), tmpw);
    gtk_widget_show(tmpw);
    g_free(flist->data);
    flist = g_list_remove(flist,flist->data);
  }
  if(selectedw)
    gtk_list_select_child(GTK_LIST(list), selectedw);
  else
    gtk_list_select_item(GTK_LIST(list), 0);
}

void readdirintolist(char *subdir, GtkWidget *list, char *selected)
{
  char *tmpdir;
  GList *thispath = parsepath();

  while(thispath) 
    {
      tmpdir = g_build_filename ((gchar *) thispath->data, subdir, NULL);
      readdirintolist_real (tmpdir, list, selected);
      g_free (tmpdir);
      thispath = thispath->next;
    }
}


void showabout(void)
{
  static GtkWidget *window = NULL;
  GtkWidget *tmpw, *tmpvbox, *tmphbox;
  GtkWidget *logobox, *tmpframe;
  int y;

  if(window) {
    gtk_widget_show(window);
    gdk_window_raise(window->window);
    return;
  }
  window = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (window), _("The GIMPressionist!"));
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &window);
  gtk_quit_add_destroy (1, GTK_OBJECT (window));
  g_signal_connect (G_OBJECT (window), "delete_event",
                    G_CALLBACK (gtk_widget_hide_on_delete),
                    &window);

  tmpw = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  GTK_WIDGET_SET_FLAGS(tmpw, GTK_CAN_DEFAULT);
  g_signal_connect_swapped (G_OBJECT(tmpw), "clicked",
                            G_CALLBACK (gtk_widget_hide),
                            window);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG(window)->action_area),
                      tmpw, TRUE, TRUE, 0);
  gtk_widget_grab_default(tmpw);
  gtk_widget_show(tmpw);

  tmpvbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(tmpvbox), 5);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window)->vbox), tmpvbox);
      
  tmphbox = gtk_hbox_new(TRUE, 5);
  gtk_box_pack_start(GTK_BOX(tmpvbox), tmphbox, TRUE, TRUE, 0);
  gtk_widget_show(tmphbox);
  
  logobox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (tmphbox), logobox, FALSE, FALSE, 0);
  gtk_widget_show(logobox);
  
  tmpframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (tmpframe), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (logobox), tmpframe, TRUE, TRUE, 0);
  gtk_widget_show(tmpframe);
  
  tmpw = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW(tmpw), 177, 70);

  for(y = 0; y < 70; y++) {
    gtk_preview_draw_row (GTK_PREVIEW(tmpw), &logobuffer[y*177*3], 0, y, 177); 
  }
  gtk_widget_draw(tmpw, NULL);
  gtk_container_add(GTK_CONTAINER(tmpframe), tmpw);
  gtk_widget_show(tmpw);
      
  tmphbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(tmpvbox), tmphbox, TRUE, TRUE, 0);

  tmpw = gtk_label_new("\xa9 1999 Vidar Madsen\n"
		       "vidar@prosalg.no\n"
		       "http://www.prosalg.no/~vidar/gimpressionist/\n"
		       PLUG_IN_VERSION
		       );
  gtk_box_pack_start(GTK_BOX(tmphbox), tmpw, TRUE, FALSE, 0);
  gtk_widget_show(tmpw);
  
  gtk_widget_show(tmphbox);
  gtk_widget_show(tmpvbox);
  gtk_widget_show(window);
}

int create_dialog(void)
{
  GtkWidget *notebook;
  GtkWidget *tmpw, *box1, *box2, *box3, *preview_box;
  gint        argc;
  gchar     **argv;

  if (standalone)
    {
      argc = 1;
      argv = g_new(char *, 1);
      argv[0] = "gimpressionist";

      gtk_init(&argc, &argv);
      gtk_rc_parse (gimp_gtkrc ());

      gtk_widget_set_default_colormap (gdk_rgb_get_colormap());
    }
  else
    {
      gimp_ui_init ("gimpressionist", TRUE);
    }

  tooltips = gtk_tooltips_new ();
  gtk_tooltips_enable (tooltips);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (dialog_close_callback),
                    NULL);

  gtk_window_set_title (GTK_WINDOW (window), _("The GIMPressionist!"));
  gtk_container_set_border_width (GTK_CONTAINER (window), 5);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), box1);
  gtk_widget_show (box1);

  box2 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
  gtk_widget_show (box2);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (box1), notebook, TRUE, TRUE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (notebook), 0);
  gtk_widget_realize (notebook);
  gtk_widget_show(notebook);

  create_presetpage(GTK_NOTEBOOK (notebook));
  create_paperpage(GTK_NOTEBOOK (notebook));
  create_brushpage(GTK_NOTEBOOK (notebook));
  create_orientationpage(GTK_NOTEBOOK (notebook));
  create_sizepage(GTK_NOTEBOOK (notebook));
  create_placementpage(GTK_NOTEBOOK (notebook));
  create_colorpage(GTK_NOTEBOOK (notebook));
  create_generalpage(GTK_NOTEBOOK (notebook));

  preview_box = create_preview();
  gtk_box_pack_start (GTK_BOX (box2), preview_box, FALSE, FALSE, 0);
  gtk_widget_show (preview_box);

  box3 = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_end (GTK_BOX (box2), box3, FALSE, FALSE, 0);
  gtk_widget_show (box3);
  
  tmpw = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  g_signal_connect (G_OBJECT(tmpw), "clicked",
		    G_CALLBACK (dialog_cancel_callback), window);
  gtk_box_pack_start (GTK_BOX (box3), tmpw, TRUE, TRUE, 0);
  gtk_widget_show(tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Quit the program"), NULL);

  tmpw = gtk_button_new_with_label (_("About..."));
  g_signal_connect (G_OBJECT(tmpw), "clicked",
                    G_CALLBACK (showabout), window);
  gtk_box_pack_start (GTK_BOX (box3), tmpw, TRUE, TRUE, 0);
  gtk_widget_show (tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Show some information about program"), NULL);

  tmpw = gtk_button_new_from_stock (GTK_STOCK_OK);
  g_signal_connect (G_OBJECT(tmpw), "clicked",
                    G_CALLBACK (dialog_ok_callback), window);
  gtk_box_pack_start (GTK_BOX (box3), tmpw, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (tmpw, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (tmpw);
  gtk_widget_show(tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Run with the selected settings"), NULL);

  gtk_widget_show(window);

  return 1;
}

int create_gimpressionist(void)
{
  pcvals.run = 0;

  if(standalone) {
    memcpy(&pcvals, &defaultpcvals, sizeof(pcvals));
    create_dialog();
    restorevals();
  } else {
    create_dialog();
  }

  gtk_main();
  gdk_flush();

  return pcvals.run;
}

char *standalone = NULL;

extern GimpPlugInInfo PLUG_IN_INFO;

#ifdef G_OS_WIN32
/* No standalone on win32. */
MAIN()

#else
int main(int argc, char **argv)
{
  if(argc != 2) {

    /* Is this needed anymore? */
#ifdef __EMX__
    set_gimp_PLUG_IN_INFO(&PLUG_IN_INFO);
#endif

    return gimp_main(argc, argv);
  }

  standalone = argv[1];

  grabarea();

  /* Testing! */
  /*
  copyppm(&infile, &inalpha);
  img_has_alpha = 1;
  */

  if(create_gimpressionist()) {
    fprintf(stderr, "Painting"); fflush(stderr);
    repaint(&infile, &inalpha);
    saveppm(&infile, argv[1]);
  }

  return 0;
}
#endif
