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
#include <glib.h>
#include <sys/types.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "gimpressionist.h"
#include "ppmtool.h"
#include <libgimp/gimp.h>
/* #include <libgimp/gimpenv.h> */

#ifndef GIMP_CHECK_VERSION
#define GIMP_CHECK_VERSION(a,b,c) 0
#endif

GtkWidget *window = NULL;
GtkTooltips *tooltips = NULL;

struct ppm infile = {0,0,NULL};
struct ppm inalpha = {0,0,NULL};

GList *parsepath(void)
{
  GParam *return_vals;
  gint nreturn_vals;
  static GList *lastpath = NULL;
  char *gimpdatasubdir, *defaultpath, *tmps, *lastchr;
  struct stat st;

  if(lastpath) return lastpath;

#if GIMP_CHECK_VERSION(1, 1, 0)
  gimpdatasubdir = g_strconcat (gimp_data_directory (),
				G_DIR_SEPARATOR_S,
				"gimpressionist",
				NULL);

  defaultpath = g_strconcat (gimp_directory (),
			     G_DIR_SEPARATOR_S,
			     "gimpressionist",
			     G_SEARCHPATH_SEPARATOR_S,
			     gimpdatasubdir,
			     NULL);
 
#else
  defaultpath = DEFAULTPATH;
  gimpdatasubdir = strchr (defaultpath, ':') + 1;
#endif

  if(standalone)
    tmps = g_strdup(defaultpath);
  else {
    return_vals = gimp_run_procedure ("gimp_gimprc_query", &nreturn_vals,
				    PARAM_STRING, "gimpressionist-path",
                                    PARAM_END);
    if(return_vals[0].data.d_status != STATUS_SUCCESS ||
       return_vals[1].data.d_string == NULL) {
      if (stat (gimpdatasubdir, &st) != 0
	  || !S_ISDIR(st.st_mode)) {
	/* No gimpressionist-path parameter, and the default doesn't exist */
#if GIMP_CHECK_VERSION(1, 1, 0)
	g_message("*** Warning ***\n"
		  "It is highly recommended to add\n"
		  " (gimpressionist-path \"${gimp_dir}" G_DIR_SEPARATOR_S "gimpressionist"
		    G_SEARCHPATH_SEPARATOR_S
		    "${gimp_data_dir}" G_DIR_SEPARATOR_S "gimpressionist\")\n"
		  "(or similar) to your gimprc file.\n");
#else
	g_message("*** Warning ***\nIt is highly recommended to add\n  (gimpressionist-path \"%s\")\n(or similar) to your gimprc file.\n", defaultpath);
#endif
      }
      tmps = g_strdup(defaultpath);
    } else {
      tmps = g_strdup(return_vals[1].data.d_string);
    }
    gimp_destroy_params (return_vals, nreturn_vals);
  }

  for(;;) {
    lastchr = strchr(tmps, G_SEARCHPATH_SEPARATOR);
    if(lastchr) *lastchr = '\0';
    if(*tmps == '~') {
      char *tmps2, *home;
      home = g_get_home_dir ();
      if(!home) {
	g_message("*** Warning ***\nNo home directory!\n");
	home = "";
      }
      tmps2 = g_strconcat(home, tmps + 1, NULL);
      tmps = tmps2;
    }
    lastpath = g_list_append(lastpath, tmps);
    if(lastchr) 
      tmps = lastchr + 1;
    else
      break;
  }
  return lastpath;
}

char *findfile(char *fn)
{
  static GList *rcpath = NULL;
  GList *thispath;
  static char file[200];
  struct stat st;

  if(!rcpath) rcpath = parsepath();

  thispath = rcpath;
  while(rcpath && thispath) {
    sprintf(file, "%s" G_DIR_SEPARATOR_S "%s", (char *)thispath->data, fn);
    if(!stat(file, &st)) return file;
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
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(paperinvert), pcvals.paperinvert);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(generalpaintedges), pcvals.generalpaintedges);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(generaltileable), pcvals.generaltileable);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(generaldropshadow), pcvals.generaldropshadow);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(generalshadowdepth), pcvals.generalshadowdepth);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(generalshadowblur), pcvals.generalshadowblur);

  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(placecenter), pcvals.placecenter);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(paperoverlay), pcvals.paperoverlay);

  drawcolor(NULL);

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

  tmpfile = strrchr(fname, G_DIR_SEPARATOR);
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
  char *fpath;
  struct dirent *de;
  struct stat st;
  GtkWidget *selectedw = NULL, *tmpw;
  DIR *dir;
  GList *flist = NULL;
 
  if(selected) {
    if(!selected[0])
      selected = NULL;
    else {
      char *nsel;
      nsel = strrchr(selected, G_DIR_SEPARATOR);
      if(nsel) selected = ++nsel;
    }
  }

  dir = opendir(subdir);

  if(!dir)
    return;

  for(;;) {
    if(!(de = readdir(dir))) break;
    fpath = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", subdir, de->d_name);
    stat(fpath, &st);
    g_free(fpath);
    if(!S_ISREG(st.st_mode)) continue;
    flist = g_list_insert_sorted(flist, g_strdup(de->d_name), (GCompareFunc)g_strcasecmp);
  }
  closedir(dir);

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

  while(thispath) {
    tmpdir = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", (char *)thispath->data, subdir);
    readdirintolist_real(tmpdir, list, selected);
    g_free(tmpdir);
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
  window = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(window), "The GIMPressionist!");
  gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
		     GTK_SIGNAL_FUNC (gtk_widget_destroyed),
		     &window);
  gtk_quit_add_destroy (1, GTK_OBJECT(window));
  gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		     GTK_SIGNAL_FUNC (gtk_widget_hide_on_delete),
		     &window);
      
  tmpw = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(tmpw, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT(tmpw), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_hide),
			     GTK_OBJECT(window));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area),
		     tmpw, TRUE, TRUE, 0);
  gtk_widget_grab_default(tmpw);
  gtk_widget_show(tmpw);

  tmpvbox = gtk_vbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(tmpvbox), 5);
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
  guchar     *color_cube;

  argc = 1;
  argv = g_new(char *, 1);
  argv[0] = "gimpressionist";

  gtk_init(&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
  if(!standalone) {
    gdk_set_use_xshm(gimp_use_xshm());
    gtk_preview_set_gamma(gimp_gamma());
    gtk_preview_set_install_cmap(gimp_install_cmap());
    color_cube = gimp_color_cube();
    gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2], color_cube[3]);
  }

  gtk_widget_set_default_visual(gtk_preview_get_visual());
  gtk_widget_set_default_colormap(gtk_preview_get_cmap());

  tooltips = gtk_tooltips_new();
  gtk_tooltips_enable(tooltips);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      (GtkSignalFunc)dialog_close_callback,
		      NULL);

  gtk_window_set_title (GTK_WINDOW (window), "The GIMPressionist!");
  gtk_container_border_width (GTK_CONTAINER (window), 5);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), box1);
  gtk_widget_show (box1);

  box2 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
  gtk_widget_show (box2);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (box1), notebook, TRUE, TRUE, 5);
  gtk_container_border_width (GTK_CONTAINER (notebook), 0);
  gtk_widget_realize (notebook);
  gtk_widget_show(notebook);

  create_paperpage(GTK_NOTEBOOK (notebook));
  create_brushpage(GTK_NOTEBOOK (notebook));
  create_orientationpage(GTK_NOTEBOOK (notebook));
  create_sizepage(GTK_NOTEBOOK (notebook));
  create_placementpage(GTK_NOTEBOOK (notebook));
  create_generalpage(GTK_NOTEBOOK (notebook));
  create_presetpage(GTK_NOTEBOOK (notebook));

  preview_box = create_preview();
  gtk_box_pack_start (GTK_BOX (box2), preview_box, FALSE, FALSE, 0);
  gtk_widget_show (preview_box);

  box3 = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_end (GTK_BOX (box2), box3, FALSE, FALSE, 0);
  gtk_widget_show (box3);
  
  tmpw = gtk_button_new_with_label(" OK ");
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)dialog_ok_callback, window);
  gtk_box_pack_start (GTK_BOX (box3), tmpw, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (tmpw, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (tmpw);
  gtk_widget_show(tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Run with the selected settings", NULL);

  tmpw = gtk_button_new_with_label(" Cancel ");
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)dialog_cancel_callback, window);
  gtk_box_pack_start (GTK_BOX (box3), tmpw, TRUE, TRUE, 0);
  gtk_widget_show(tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Quit the program", NULL);

  tmpw = gtk_button_new_with_label(" About... ");
  gtk_signal_connect(GTK_OBJECT(tmpw), "clicked",
		     (GtkSignalFunc)showabout, window);
  gtk_box_pack_start (GTK_BOX (box3), tmpw, TRUE, TRUE, 0);
  gtk_widget_show(tmpw);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Show some information about program", NULL);

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

extern GPlugInInfo PLUG_IN_INFO;

#ifdef NATIVE_WIN32
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

