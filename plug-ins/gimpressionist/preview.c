#include "config.h"

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


static GtkWidget *preview       = NULL;
GtkWidget        *previewbutton = NULL;


static void drawalpha(ppm_t *p, ppm_t *a)
{
  int x, y, g;
  double v;
  int gridsize = 16;
  int rowstride = p->width * 3;

  for (y = 0; y < p->height; y++) {
    for (x = 0; x < p->width; x++) {
      int k = y*rowstride + x*3;
      if (!a->col[k]) continue;
      v = 1.0 - a->col[k] / 255.0;
      g = ((x/gridsize+y/gridsize)%2)*60+100;
      p->col[k+0] *= v;
      p->col[k+1] *= v;
      p->col[k+2] *= v;
      v = 1.0 - v;
      p->col[k+0] += g*v;
      p->col[k+1] += g*v;
      p->col[k+2] += g*v;
    }
  }
}

void
updatepreview (GtkWidget *wg, gpointer d)
{
  gint   i;
  guchar buf[PREVIEWSIZE*3];
  static ppm_t p = {0,0,NULL};
  static ppm_t a = {0,0,NULL};
  static ppm_t backup = {0,0,NULL};
  static ppm_t abackup = {0,0,NULL};

  if(!infile.col && d) grabarea();

  if(!infile.col && !d) {
    memset(buf, 0, PREVIEWSIZE*3);
    for(i = 0; i < PREVIEWSIZE; i++) {
      gtk_preview_draw_row (GTK_PREVIEW(preview), buf, 0, i, PREVIEWSIZE);
    }
  } else {
    if(!backup.col) {
      copyppm(&infile, &backup);
      if((backup.width != PREVIEWSIZE) || (backup.height != PREVIEWSIZE))
	resize_fast(&backup, PREVIEWSIZE, PREVIEWSIZE);
      if(img_has_alpha) {
	copyppm(&inalpha, &abackup);
	if((abackup.width != PREVIEWSIZE) || (abackup.height != PREVIEWSIZE))
	  resize_fast(&abackup, PREVIEWSIZE, PREVIEWSIZE);
      }
    }
    if(!p.col) {
      copyppm(&backup, &p);
      if(img_has_alpha)
	copyppm(&abackup, &a);
    }
    if(d) {
      storevals();

      if(GPOINTER_TO_INT(d) != 2)
	repaint(&p, &a);
    }
    if(img_has_alpha)
      drawalpha(&p, &a);

    for(i = 0; i < PREVIEWSIZE; i++) {
      gtk_preview_draw_row(GTK_PREVIEW(preview),
			   (guchar*) &p.col[i * PREVIEWSIZE * 3], 0, i,
			   PREVIEWSIZE);
    }
    killppm(&p);
    if(img_has_alpha)
      killppm(&a);
  }

  gtk_widget_queue_draw (preview);
}

GtkWidget *
create_preview (void)
{
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *button;

  vbox = gtk_vbox_new (FALSE, 6);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX (vbox), frame, FALSE, FALSE, 5);
  gtk_widget_show (frame);

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), PREVIEWSIZE, PREVIEWSIZE);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  hbox = gtk_hbox_new (TRUE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  previewbutton = button = gtk_button_new_with_mnemonic( _("_Update"));
  g_signal_connect (button, "clicked",
		    G_CALLBACK (updatepreview), (gpointer) 1);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  gimp_help_set_help_data (button,
			   _("Refresh the Preview window"), NULL);

  button = gtk_button_new_from_stock (GIMP_STOCK_RESET);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (updatepreview), (gpointer) 2);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  gimp_help_set_help_data (button,
			   _("Revert to the original image"), NULL);

  return vbox;
}
