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
#include <ctype.h>
#include "gimpressionist.h"
#include "ppmtool.h"

GtkWidget *presetnameentry = NULL;
GtkWidget *presetsavebutton = NULL;
GtkWidget *presetlist = NULL;
GtkWidget *presetdesctext = NULL;
GtkWidget *presetdesclabel = NULL;

char presetdesc[4096] = "";

char *factory_defaults = "<Factory defaults>";

void presetsrefresh(void)
{
  GtkWidget *list = presetlist;
  GtkWidget *tmpw;
  int n = g_list_length(GTK_LIST(list)->children);

  gtk_list_clear_items(GTK_LIST(list), 0, n);

  tmpw = gtk_list_item_new_with_label(factory_defaults);
  gtk_container_add(GTK_CONTAINER(list), tmpw);
  gtk_widget_show(tmpw);
  
  readdirintolist("Presets", list, NULL);
}

#define PRESETMAGIC "Preset"

int loadoldpreset(char *fname)
{
  FILE *f;
  int len;

  f = fopen(fname, "rb");
  if(!f) {
    fprintf(stderr, "Error opening file \"%s\" for reading!%c\n", fname, 7);
    return -1;
  }
  len = fread(&pcvals, 1, sizeof(pcvals), f);
  fclose(f);

  return 0;
}

void chop(char *buffer)
{
  while(strlen(buffer) && buffer[strlen(buffer)-1] <= ' ')
    buffer[strlen(buffer)-1] = '\0';
}

unsigned int hexval(char c)
{
  c = tolower(c);
  if((c >= 'a') && (c <= 'f')) return c - 'a' + 10;
  if((c >= '0') && (c <= '9')) return c - '0';
  return 0;
}

char *parsergbstring(char *s)
{
  static char col[3];
  col[0] = (hexval(s[0]) << 4) | hexval(s[1]);
  col[1] = (hexval(s[2]) << 4) | hexval(s[3]);
  col[2] = (hexval(s[4]) << 4) | hexval(s[5]);
  return col;
}

void setorientvector(char *str)
{
  /* num,x,y,dir,dx,dy,str,type */
  char *tmps = str;
  int n;

  n = atoi(tmps);

  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.orientvector[n].x = atof(++tmps);

  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.orientvector[n].y = atof(++tmps);
  
  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.orientvector[n].dir = atof(++tmps);

  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.orientvector[n].dx = atof(++tmps);

  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.orientvector[n].dy = atof(++tmps);
  
  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.orientvector[n].str = atof(++tmps);
  
  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.orientvector[n].type = atoi(++tmps);
  
}

void setsizevector(char *str)
{
  /* num,x,y,siz,str,type */
  char *tmps = str;
  int n;

  n = atoi(tmps);

  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.sizevector[n].x = atof(++tmps);

  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.sizevector[n].y = atof(++tmps);
  
  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.sizevector[n].siz = atof(++tmps);

  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.sizevector[n].str = atof(++tmps);
  
}

void parsedesc(char *str, char *d)
{
  while(*str) {
    if(*str == '\\') {
      *d = (str[1] - '0') * 100;
      *d += (str[2] - '0') * 10;
      *d += (str[3] - '0');
      str += 3;
    } else *d = *str;
    str++;
    d++;
  }
  *d = '\0';
}

void setval(char *key, char *val)
{
  if(!strcmp(key, "desc"))
    parsedesc(val, presetdesc);
  else if(!strcmp(key, "orientnum"))
    pcvals.orientnum = atoi(val);
  else if(!strcmp(key, "orientfirst"))
    pcvals.orientfirst = atof(val);
  else if(!strcmp(key, "orientlast"))
    pcvals.orientlast = atof(val);
  else if(!strcmp(key, "orienttype"))
   pcvals.orienttype = atoi(val);

  else if(!strcmp(key, "sizenum"))
    pcvals.sizenum = atoi(val);
  else if(!strcmp(key, "sizefirst"))
    pcvals.sizefirst = atof(val);
  else if(!strcmp(key, "sizelast"))
    pcvals.sizelast = atof(val);
  else if(!strcmp(key, "sizetype"))
   pcvals.sizetype = atoi(val);

  else if(!strcmp(key, "brushrelief"))
    pcvals.brushrelief = atof(val);
  else if(!strcmp(key, "brushscale")) {
    /* For compatibility */
    pcvals.sizenum = 1;
    pcvals.sizefirst = pcvals.sizelast = atof(val);
  }
  else if(!strcmp(key, "brushdensity"))
    pcvals.brushdensity = atof(val);
  else if(!strcmp(key, "brushgamma"))
    pcvals.brushgamma = atof(val);
  else if(!strcmp(key, "brushaspect"))
    pcvals.brushaspect = atof(val);

  else if(!strcmp(key, "generalbgtype"))
    pcvals.generalbgtype = atoi(val);
  else if(!strcmp(key, "generaldarkedge"))
    pcvals.generaldarkedge = atof(val);
  else if(!strcmp(key, "generalpaintedges"))
    pcvals.generalpaintedges = atoi(val);
  else if(!strcmp(key, "generaltileable"))
    pcvals.generaltileable = atoi(val);
  else if(!strcmp(key, "generaldropshadow"))
    pcvals.generaldropshadow = atoi(val);
  else if(!strcmp(key, "generalshadowdarkness"))
    pcvals.generalshadowdarkness = atof(val);
  else if(!strcmp(key, "generalshadowdepth"))
    pcvals.generalshadowdepth = atoi(val);
  else if(!strcmp(key, "generalshadowblur"))
    pcvals.generalshadowblur = atoi(val);
  else if(!strcmp(key, "devthresh"))
    pcvals.devthresh = atof(val);

  else if(!strcmp(key, "paperrelief"))
    pcvals.paperrelief = atof(val);
  else if(!strcmp(key, "paperscale"))
    pcvals.paperscale = atof(val);
  else if(!strcmp(key, "paperinvert"))
    pcvals.paperinvert = atoi(val);
  else if(!strcmp(key, "paperoverlay"))
    pcvals.paperoverlay = atoi(val);

  else if(!strcmp(key, "placetype"))
    pcvals.placetype = atoi(val);
  else if(!strcmp(key, "placecenter"))
    pcvals.placecenter = atoi(val);

  else if(!strcmp(key, "selectedbrush"))
    strncpy(pcvals.selectedbrush, val, 99);
  else if(!strcmp(key, "selectedpaper"))
    strncpy(pcvals.selectedpaper, val, 99);

  else if(!strcmp(key, "color"))
    memcpy(pcvals.color, parsergbstring(val), 3);
  
  else if(!strcmp(key, "numorientvector"))
    pcvals.numorientvector = atoi(val);
  else if(!strcmp(key, "orientvector"))
    setorientvector(val);
  else if(!strcmp(key, "orientangoff"))
   pcvals.orientangoff = atof(val);
  else if(!strcmp(key, "orientstrexp"))
   pcvals.orientstrexp = atof(val);
  else if(!strcmp(key, "orientvoronoi"))
   pcvals.orientvoronoi = atoi(val);

  else if(!strcmp(key, "numsizevector"))
    pcvals.numsizevector = atoi(val);
  else if(!strcmp(key, "sizevector"))
    setsizevector(val);
  else if(!strcmp(key, "sizestrexp"))
   pcvals.sizestrexp = atof(val);
  else if(!strcmp(key, "sizevoronoi"))
   pcvals.sizevoronoi = atoi(val);
}

int loadpreset(char *fn)
{
  char line[1024] = "";
  FILE *f;

  f = fopen(fn, "rt");
  if(!f) {
    fprintf(stderr, "Error opening file \"%s\" for reading!\n", fn);
    return -1;
  }
  fgets(line,10,f);
  if(strncmp(line,PRESETMAGIC,4)) {
    fclose(f);
    return loadoldpreset(fn);
  }
  memcpy(&pcvals, &defaultpcvals, sizeof(pcvals));
  while(!feof(f)) {
    char *tmps;
    if(!fgets(line,1024,f)) break;
    chop(line);
    tmps = strchr(line, '=');
    if(!tmps) continue;
    *tmps = '\0';
    tmps++;
    setval(line, tmps);
  }
  fclose(f);
  return 0;
}

void applypreset(void)
{
  GList *h = GTK_LIST(presetlist)->selection;
  GtkWidget *tmpw = h->data;
  char *l;
  static char fname[200];

  gtk_label_get(GTK_LABEL(GTK_BIN(tmpw)->child), &l);

  /* Restore defaults, in case of old/short Preset file */
  memcpy(&pcvals, &defaultpcvals, sizeof(pcvals));
  presetdesc[0] = '\0';

  if(!strcmp(l, factory_defaults)) {
    restorevals();
    return;
  }

  sprintf(fname, "Presets/%s", l);
  strcpy(fname, findfile(fname));

  loadpreset(fname);

  restorevals();
}

void deletepreset(void)
{
  GList *h = GTK_LIST(presetlist)->selection;
  GtkWidget *tmpw = h->data;
  char *l;
  static char fname[200];

  gtk_label_get(GTK_LABEL(GTK_BIN(tmpw)->child), &l);

  sprintf(fname, "Presets/%s", l);
  strcpy(fname, findfile(fname));

  unlink(fname);
  presetsrefresh();
}

void savepreset(GtkWidget *wg, GtkWidget *p);

void presetdesccallback(GtkWidget *widget, gpointer data)
{
  guchar *s;
  char *d, *str;
  str = gtk_editable_get_chars(GTK_EDITABLE (widget),0,-1);
  s = str;
  d = presetdesc;
  while(*s) {
    if((*s < ' ') || (*s == '\\')) { sprintf(d, "\\%03d", *s); d += 4; }
    else { *d = *s; d++; }
    s++;
  }
  *d = '\0';
  g_free(str);
}

void oksavepreset(GtkWidget *wg, GtkWidget *p)
{
  if(wg) gtk_widget_destroy(wg);
  savepreset(NULL,NULL);
}

void create_savepreset(void)
{
  static GtkWidget *window = NULL;
  GtkWidget *button;
  GtkWidget *box, *label;
  GtkWidget *text;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    
  gtk_signal_connect_object (GTK_OBJECT (window), "delete_event",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT(window));
    
  gtk_signal_connect_object(GTK_OBJECT(window), "destroy",
			    GTK_SIGNAL_FUNC (gtk_widget_destroy),
			    GTK_OBJECT(window));

  gtk_container_border_width (GTK_CONTAINER (window), 5);

  box = gtk_vbox_new(FALSE,5);

  label = gtk_label_new("Description:");
  gtk_box_pack_start(GTK_BOX(box),label,FALSE,FALSE,0);
  gtk_widget_show (label);

  presetdesctext = text = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (text), TRUE);
  gtk_box_pack_start(GTK_BOX(box),text,FALSE,FALSE,0);
  gtk_widget_show (text);

  gtk_text_set_word_wrap(GTK_TEXT(text), 0);
  gtk_text_set_line_wrap(GTK_TEXT(text), 0);

  gtk_text_insert (GTK_TEXT (text), NULL, NULL,
		   NULL, presetdesc, strlen(presetdesc));

  gtk_signal_connect (GTK_OBJECT (text), "changed",
		      (GtkSignalFunc) presetdesccallback,
		      NULL);

  button = gtk_button_new_with_label ("OK");
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC (oksavepreset),
			     GTK_OBJECT(window));
  gtk_box_pack_start(GTK_BOX(box),button,FALSE,FALSE,0);
  gtk_widget_show (button);


  button = gtk_button_new_with_label ("Cancel");
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT(window));
  gtk_box_pack_start(GTK_BOX(box),button,FALSE,FALSE,0);
  gtk_widget_show (button);

    
  gtk_container_add (GTK_CONTAINER (window), box);
    
  gtk_widget_show (box);
  gtk_widget_show (window);

}

void savepreset(GtkWidget *wg, GtkWidget *p)
{
  char *l;
  static char fname[200];
  FILE *f;
  GList *thispath;
  int i;

  l = gtk_entry_get_text(GTK_ENTRY(presetnameentry));
  thispath = parsepath();
  storevals();

  if(!thispath) {
    fprintf(stderr, "Internal error: (savepreset) thispath == NULL");
    return;
  }

  sprintf(fname, "%s/Presets/%s", (char *)thispath->data, l);

  f = fopen(fname, "wt");
  if(!f) {
    fprintf(stderr, "Error opening file \"%s\" for writing!%c\n", fname, 7);
    return;
  }
  fprintf(f, "%s\n", PRESETMAGIC);
  fprintf(f, "desc=%s\n", presetdesc);
  fprintf(f, "orientnum=%d\n", pcvals.orientnum);
  fprintf(f, "orientfirst=%f\n", pcvals.orientfirst);
  fprintf(f, "orientlast=%f\n", pcvals.orientlast);
  fprintf(f, "orienttype=%d\n", pcvals.orienttype);

  fprintf(f, "sizenum=%d\n", pcvals.sizenum);
  fprintf(f, "sizefirst=%f\n", pcvals.sizefirst);
  fprintf(f, "sizelast=%f\n", pcvals.sizelast);
  fprintf(f, "sizetype=%d\n", pcvals.sizetype);

  fprintf(f, "brushrelief=%f\n", pcvals.brushrelief);
  fprintf(f, "brushdensity=%f\n", pcvals.brushdensity);
  fprintf(f, "brushgamma=%f\n", pcvals.brushgamma);
  fprintf(f, "brushaspect=%f\n", pcvals.brushaspect);

  fprintf(f, "generalbgtype=%d\n", pcvals.generalbgtype);
  fprintf(f, "generaldarkedge=%f\n", pcvals.generaldarkedge);
  fprintf(f, "generalpaintedges=%d\n", pcvals.generalpaintedges);
  fprintf(f, "generaltileable=%d\n", pcvals.generaltileable);
  fprintf(f, "generaldropshadow=%d\n", pcvals.generaldropshadow);
  fprintf(f, "generalshadowdarkness=%f\n", pcvals.generalshadowdarkness);
  fprintf(f, "generalshadowdepth=%d\n", pcvals.generalshadowdepth);
  fprintf(f, "generalshadowblur=%d\n", pcvals.generalshadowblur);
  fprintf(f, "devthresh=%f\n", pcvals.devthresh);

  fprintf(f, "paperrelief=%f\n", pcvals.paperrelief);
  fprintf(f, "paperscale=%f\n", pcvals.paperscale);
  fprintf(f, "paperinvert=%d\n", pcvals.paperinvert);
  fprintf(f, "paperoverlay=%d\n", pcvals.paperoverlay);

  fprintf(f, "selectedbrush=%s\n", pcvals.selectedbrush);
  fprintf(f, "selectedpaper=%s\n", pcvals.selectedpaper);

  fprintf(f, "color=%02x%02x%02x\n", pcvals.color[0],
	  pcvals.color[1], pcvals.color[2]);
  
  fprintf(f, "placetype=%d\n", pcvals.placetype);
  fprintf(f, "placecenter=%d\n", pcvals.placecenter);

  fprintf(f, "numorientvector=%d\n", pcvals.numorientvector);
  for(i = 0; i < pcvals.numorientvector; i++) {
    fprintf(f, "orientvector=%d,%f,%f,%f,%f,%f,%f,%d\n", i,
	    pcvals.orientvector[i].x,
	    pcvals.orientvector[i].y,
	    pcvals.orientvector[i].dir,
	    pcvals.orientvector[i].dx,
	    pcvals.orientvector[i].dy,
	    pcvals.orientvector[i].str,
	    pcvals.orientvector[i].type);
  }
  fprintf(f, "orientangoff=%f\n", pcvals.orientangoff);
  fprintf(f, "orientstrexp=%f\n", pcvals.orientstrexp);
  fprintf(f, "orientvoronoi=%d\n", pcvals.orientvoronoi);

  fprintf(f, "numsizevector=%d\n", pcvals.numsizevector);
  for(i = 0; i < pcvals.numsizevector; i++) {
    fprintf(f, "sizevector=%d,%f,%f,%f,%f\n", i,
	    pcvals.sizevector[i].x,
	    pcvals.sizevector[i].y,
	    pcvals.sizevector[i].siz,
	    pcvals.sizevector[i].str);
  }
  fprintf(f, "sizestrexp=%f\n", pcvals.sizestrexp);
  fprintf(f, "sizevoronoi=%d\n", pcvals.sizevoronoi);

  fclose(f);
  presetsrefresh();
  reselect(presetlist, fname);
}

void readdesc(char *fn)
{
  char *tmp, fname[200];
  FILE *f;
  
  sprintf(fname, "Presets/%s", fn);
  tmp = findfile(fname);
  if(!tmp) {
    if(presetdesclabel)
      gtk_label_set_text(GTK_LABEL(presetdesclabel), "");
    return; 
  }
  strcpy(fname, tmp);

  f = fopen(fname, "rt");
  if(f) { 
    char line[4096];
    char tmplabel[4096];
    while(!feof(f)) {
      fgets(line, 4095, f);
      if(!strncmp(line, "desc=", 5)) {
	parsedesc(line+5, tmplabel);
	gtk_label_set_text(GTK_LABEL(presetdesclabel), tmplabel);
	fclose(f);
	return;
      }
    }
    fclose(f);
  }
  gtk_label_set_text(GTK_LABEL(presetdesclabel), "");
  return; 
}

void selectpreset(GtkWidget *wg, GtkWidget *p)
{
  GList *h = GTK_LIST(p)->selection;
  GtkWidget *tmpw;
  char *l;

  if(!h) return;
  tmpw = h->data;
  if(!tmpw) return;

  gtk_label_get(GTK_LABEL(GTK_BIN(tmpw)->child), &l);
  if(strcmp(l, factory_defaults))
    gtk_entry_set_text(GTK_ENTRY(presetnameentry), l);

  readdesc(l);
}

void create_presetpage(GtkNotebook *notebook)
{
  GtkWidget *box1, *thispage, *box2;
  GtkWidget *labelbox, *menubox;
  GtkWidget *scrolled_win, *list;
  GtkWidget *tmpw;
  char title[100];

  sprintf(title, "Presets");

  labelbox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new(title);
  gtk_box_pack_start(GTK_BOX(labelbox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(labelbox);

  menubox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new(title);
  gtk_box_pack_start(GTK_BOX(menubox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(menubox);


  presetlist = list = gtk_list_new ();

  thispage = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (thispage), 5);
  gtk_widget_show(thispage);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box1, FALSE, FALSE, 0);
  gtk_widget_show (box1);

  presetnameentry = tmpw = gtk_entry_new();
  gtk_box_pack_start (GTK_BOX (box1), tmpw, FALSE, FALSE, 0);
  gtk_widget_set_usize(tmpw, 150, -1);
  gtk_widget_show(tmpw);

  presetsavebutton = tmpw = gtk_button_new_with_label(" Save current ");
  gtk_box_pack_start(GTK_BOX(box1), tmpw,FALSE,FALSE,5);
  gtk_widget_show (tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(create_savepreset),
		      NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Save the current settings to the specified file", NULL);

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

  /* list = gtk_list_new (); */ /* Moved up */
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_BROWSE);
#if GTK_MINOR_VERSION > 0
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
#else
  gtk_container_add (GTK_CONTAINER (scrolled_win), list);
#endif
  gtk_widget_show (list);

  gtk_signal_connect (GTK_OBJECT(list), "selection_changed",
                      GTK_SIGNAL_FUNC(selectpreset),
                      list);

  tmpw = gtk_list_item_new_with_label(factory_defaults);
  gtk_container_add(GTK_CONTAINER(list), tmpw);
  gtk_widget_show(tmpw);

  readdirintolist("Presets", list, NULL);

  box2 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box1), box2,FALSE,FALSE,5);
  gtk_widget_show (box2);
  /* gtk_container_border_width (GTK_CONTAINER (box2), 5); */

  tmpw = gtk_button_new_with_label(" Apply ");
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(applypreset),
		      NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Reads the selected Preset into memory", NULL);

  tmpw = gtk_button_new_with_label(" Delete ");
  gtk_box_pack_start(GTK_BOX(box2), tmpw, FALSE, FALSE,0);
  gtk_widget_show (tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(deletepreset),
		      NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Deletes the selected Preset", NULL);

  tmpw = gtk_button_new_with_label(" Refresh ");
  gtk_box_pack_start(GTK_BOX(box2), tmpw, FALSE, FALSE,0);
  gtk_widget_show (tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(presetsrefresh),
		      NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, "Reread the directory of Presets", NULL);

  presetdesclabel = tmpw = gtk_label_new("(Desc)");
  gtk_box_pack_start(GTK_BOX(box2), tmpw, FALSE, FALSE,0);
  gtk_widget_show(tmpw);


  tmpw = gtk_label_new("\nIf you come up with some nice Presets,\n\
(or Brushes and Papers for that matter)\n\
feel free to send them to me <vidar@prosalg.no>\n\
for inclusion into the next release!\n");
  gtk_box_pack_start(GTK_BOX(thispage), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);

  gtk_notebook_append_page_menu (notebook, thispage, labelbox, menubox);
}
