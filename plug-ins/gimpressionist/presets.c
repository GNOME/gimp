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
#include <libgimp/stdplugins-intl.h>

/* FIXME: remove usage of the 'broken' GtkText */
#define GTK_ENABLE_BROKEN
#include <gtk/gtktext.h>

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
  pcvals.orientvector[n].x = g_ascii_strtod (++tmps, NULL);

  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.orientvector[n].y = g_ascii_strtod (++tmps, NULL);
  
  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.orientvector[n].dir = g_ascii_strtod (++tmps, NULL);

  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.orientvector[n].dx = g_ascii_strtod (++tmps, NULL);

  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.orientvector[n].dy = g_ascii_strtod (++tmps, NULL);
  
  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.orientvector[n].str = g_ascii_strtod (++tmps, NULL);
  
  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.orientvector[n].type = atoi (++tmps);
  
}

void setsizevector(char *str)
{
  /* num,x,y,siz,str,type */
  char *tmps = str;
  int n;

  n = atoi(tmps);

  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.sizevector[n].x = g_ascii_strtod (++tmps, NULL);

  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.sizevector[n].y = g_ascii_strtod (++tmps, NULL);
  
  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.sizevector[n].siz = g_ascii_strtod (++tmps, NULL);

  if(!(tmps = strchr(tmps, ','))) return;
  pcvals.sizevector[n].str = g_ascii_strtod (++tmps, NULL);
  
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
    pcvals.orientfirst = g_ascii_strtod (val, NULL);
  else if(!strcmp(key, "orientlast"))
    pcvals.orientlast = g_ascii_strtod (val, NULL);
  else if(!strcmp(key, "orienttype"))
   pcvals.orienttype = atoi(val);

  else if(!strcmp(key, "sizenum"))
    pcvals.sizenum = atoi(val);
  else if(!strcmp(key, "sizefirst"))
    pcvals.sizefirst = g_ascii_strtod (val, NULL);
  else if(!strcmp(key, "sizelast"))
    pcvals.sizelast = g_ascii_strtod (val, NULL);
  else if(!strcmp(key, "sizetype"))
   pcvals.sizetype = atoi(val);

  else if(!strcmp(key, "brushrelief"))
    pcvals.brushrelief = g_ascii_strtod (val, NULL);
  else if(!strcmp(key, "brushscale")) {
    /* For compatibility */
    pcvals.sizenum = 1;
    pcvals.sizefirst = pcvals.sizelast = g_ascii_strtod (val, NULL);
  }
  else if(!strcmp(key, "brushdensity"))
    pcvals.brushdensity = g_ascii_strtod (val, NULL);
  else if(!strcmp(key, "brushgamma"))
    pcvals.brushgamma = g_ascii_strtod (val, NULL);
  else if(!strcmp(key, "brushaspect"))
    pcvals.brushaspect = g_ascii_strtod (val, NULL);

  else if(!strcmp(key, "generalbgtype"))
    pcvals.generalbgtype = atoi(val);
  else if(!strcmp(key, "generaldarkedge"))
    pcvals.generaldarkedge = g_ascii_strtod (val, NULL);
  else if(!strcmp(key, "generalpaintedges"))
    pcvals.generalpaintedges = atoi(val);
  else if(!strcmp(key, "generaltileable"))
    pcvals.generaltileable = atoi(val);
  else if(!strcmp(key, "generaldropshadow"))
    pcvals.generaldropshadow = atoi(val);
  else if(!strcmp(key, "generalshadowdarkness"))
    pcvals.generalshadowdarkness = g_ascii_strtod (val, NULL);
  else if(!strcmp(key, "generalshadowdepth"))
    pcvals.generalshadowdepth = atoi(val);
  else if(!strcmp(key, "generalshadowblur"))
    pcvals.generalshadowblur = atoi(val);
  else if(!strcmp(key, "devthresh"))
    pcvals.devthresh = g_ascii_strtod (val, NULL);

  else if(!strcmp(key, "paperrelief"))
    pcvals.paperrelief = g_ascii_strtod (val, NULL);
  else if(!strcmp(key, "paperscale"))
    pcvals.paperscale = g_ascii_strtod (val, NULL);
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
   pcvals.orientangoff = g_ascii_strtod (val, NULL);
  else if(!strcmp(key, "orientstrexp"))
   pcvals.orientstrexp = g_ascii_strtod (val, NULL);
  else if(!strcmp(key, "orientvoronoi"))
   pcvals.orientvoronoi = atoi(val);

  else if(!strcmp(key, "numsizevector"))
    pcvals.numsizevector = atoi(val);
  else if(!strcmp(key, "sizevector"))
    setsizevector(val);
  else if(!strcmp(key, "sizestrexp"))
   pcvals.sizestrexp = g_ascii_strtod (val, NULL);
  else if(!strcmp(key, "sizevoronoi"))
   pcvals.sizevoronoi = atoi(val);

  else if(!strcmp(key, "colortype"))
    pcvals.colortype = atoi(val);
  else if(!strcmp(key, "colornoise"))
    pcvals.colornoise = g_ascii_strtod (val, NULL);
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

  gtk_container_set_border_width (GTK_CONTAINER (window), 5);

  box = gtk_vbox_new(FALSE,5);

  label = gtk_label_new( _("Description:"));
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

  button = gtk_button_new_from_stock ( GTK_STOCK_OK);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC (oksavepreset),
			     GTK_OBJECT(window));
  gtk_box_pack_start(GTK_BOX(box),button,FALSE,FALSE,0);
  gtk_widget_show (button);


  button = gtk_button_new_from_stock ( GTK_STOCK_CANCEL);
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
  const gchar *l;
  static char fname[200];
  FILE *f;
  GList *thispath;
  gchar  buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar  vbuf[6][G_ASCII_DTOSTR_BUF_SIZE];
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
  fprintf(f, "orientfirst=%s\n", 
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orientfirst));
  fprintf(f, "orientlast=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orientlast));
  fprintf(f, "orienttype=%d\n", pcvals.orienttype);

  fprintf(f, "sizenum=%d\n", pcvals.sizenum);
  fprintf(f, "sizefirst=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.sizefirst));
  fprintf(f, "sizelast=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.sizelast));
  fprintf(f, "sizetype=%d\n", pcvals.sizetype);

  fprintf(f, "brushrelief=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.brushrelief));
  fprintf(f, "brushdensity=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.brushdensity));
  fprintf(f, "brushgamma=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.brushgamma));
  fprintf(f, "brushaspect=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.brushaspect));

  fprintf(f, "generalbgtype=%d\n", pcvals.generalbgtype);
  fprintf(f, "generaldarkedge=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.generaldarkedge));
  fprintf(f, "generalpaintedges=%d\n", pcvals.generalpaintedges);
  fprintf(f, "generaltileable=%d\n", pcvals.generaltileable);
  fprintf(f, "generaldropshadow=%d\n", pcvals.generaldropshadow);
  fprintf(f, "generalshadowdarkness=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.generalshadowdarkness));
  fprintf(f, "generalshadowdepth=%d\n", pcvals.generalshadowdepth);
  fprintf(f, "generalshadowblur=%d\n", pcvals.generalshadowblur);
  fprintf(f, "devthresh=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.devthresh));

  fprintf(f, "paperrelief=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.paperrelief));
  fprintf(f, "paperscale=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.paperscale));
  fprintf(f, "paperinvert=%d\n", pcvals.paperinvert);
  fprintf(f, "paperoverlay=%d\n", pcvals.paperoverlay);

  fprintf(f, "selectedbrush=%s\n", pcvals.selectedbrush);
  fprintf(f, "selectedpaper=%s\n", pcvals.selectedpaper);

  fprintf(f, "color=%02x%02x%02x\n", pcvals.color[0],
	  pcvals.color[1], pcvals.color[2]);
  
  fprintf(f, "placetype=%d\n", pcvals.placetype);
  fprintf(f, "placecenter=%d\n", pcvals.placecenter);

  fprintf(f, "numorientvector=%d\n", pcvals.numorientvector);
  for(i = 0; i < pcvals.numorientvector; i++) 
    {
      g_ascii_formatd (vbuf[0], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orientvector[i].x);
      g_ascii_formatd (vbuf[1], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orientvector[i].y);
      g_ascii_formatd (vbuf[2], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orientvector[i].dir);
      g_ascii_formatd (vbuf[3], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orientvector[i].dx);
      g_ascii_formatd (vbuf[4], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orientvector[i].dy);
      g_ascii_formatd (vbuf[5], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orientvector[i].str);
      
      fprintf (f, "orientvector=%d,%s,%s,%s,%s,%s,%s,%d\n", i,
               vbuf[0], vbuf[1], vbuf[2], vbuf[3], vbuf[4], vbuf[5],
               pcvals.orientvector[i].type);
    }

  fprintf(f, "orientangoff=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orientangoff));
  fprintf(f, "orientstrexp=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.orientstrexp));
  fprintf(f, "orientvoronoi=%d\n", pcvals.orientvoronoi);

  fprintf(f, "numsizevector=%d\n", pcvals.numsizevector);
  for (i = 0; i < pcvals.numsizevector; i++) 
    {
      g_ascii_formatd (vbuf[0], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.sizevector[i].x);
      g_ascii_formatd (vbuf[1], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.sizevector[i].y);
      g_ascii_formatd (vbuf[2], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.sizevector[i].siz);
      g_ascii_formatd (vbuf[3], G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.sizevector[i].str);
      fprintf (f, "sizevector=%d,%s,%s,%s,%s\n", i,
               vbuf[0], vbuf[1], vbuf[2], vbuf[3]);
    }
  fprintf(f, "sizestrexp=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.sizestrexp));
  fprintf(f, "sizevoronoi=%d\n", pcvals.sizevoronoi);

  fprintf(f, "colortype=%d\n", pcvals.colortype);
  fprintf(f, "colornoise=%s\n",
          g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f", pcvals.colornoise));

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

  labelbox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new (_("Presets"));
  gtk_box_pack_start(GTK_BOX(labelbox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(labelbox);

  menubox = gtk_hbox_new (FALSE, 0);
  tmpw = gtk_label_new ( _("Presets"));
  gtk_box_pack_start(GTK_BOX(menubox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show_all(menubox);

  presetlist = list = gtk_list_new ();

  thispage = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 5);
  gtk_widget_show(thispage);

  box1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start(GTK_BOX(thispage), box1, FALSE, FALSE, 0);
  gtk_widget_show (box1);

  presetnameentry = tmpw = gtk_entry_new();
  gtk_box_pack_start (GTK_BOX (box1), tmpw, FALSE, FALSE, 0);
  gtk_widget_set_usize(tmpw, 150, -1);
  gtk_widget_show(tmpw);

  presetsavebutton = tmpw = gtk_button_new_with_label( _("Save current"));
  gtk_box_pack_start(GTK_BOX(box1), tmpw,FALSE,FALSE,5);
  gtk_widget_show (tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(create_savepreset),
		      NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Save the current settings to the specified file"), NULL);

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
  /* gtk_container_set_border_width (GTK_CONTAINER (box2), 5); */

  tmpw = gtk_button_new_with_label( _("Apply"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(applypreset),
		      NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Reads the selected Preset into memory"), NULL);

  tmpw = gtk_button_new_with_label( _("Delete"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw, FALSE, FALSE,0);
  gtk_widget_show (tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(deletepreset),
		      NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Deletes the selected Preset"), NULL);

  tmpw = gtk_button_new_with_label( _("Refresh"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw, FALSE, FALSE,0);
  gtk_widget_show (tmpw);
  gtk_signal_connect (GTK_OBJECT(tmpw), "clicked",
		      GTK_SIGNAL_FUNC(presetsrefresh),
		      NULL);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), tmpw, _("Reread the directory of Presets"), NULL);

  presetdesclabel = tmpw = gtk_label_new( _("(Desc)"));
  gtk_box_pack_start(GTK_BOX(box2), tmpw, FALSE, FALSE,0);
  gtk_widget_show(tmpw);


  tmpw = gtk_label_new( _("\nIf you come up with some nice Presets,\n\
(or Brushes and Papers for that matter)\n\
feel free to send them to me <vidar@prosalg.no>\n\
for inclusion into the next release!\n"));
  gtk_box_pack_start(GTK_BOX(thispage), tmpw, FALSE, FALSE, 0);
  gtk_widget_show(tmpw);

  gtk_notebook_append_page_menu (notebook, thispage, labelbox, menubox);
}
