/**************************************************
 * file: megawidget/megawidget.h
 *
 * Copyright (c) 1997 Eric L. Hernes (erich@rrnet.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */

#ifndef MEGAWIDGET_H

struct mwRadioGroup {
   gchar *name;
   gint var;
};
struct mwValueRadioGroup {
   gchar *name;
   gint val;
};

struct mwPreview {
   gint width;
   gint height;
   gint bpp;
   gdouble scale;
   guchar *bits;
};

struct mwColorSel {
   gdouble *color;
   gdouble savcolor[4];
   gint opacity;
   guchar *name;
   GtkWidget *window;
   GtkWidget *preview;
};

#ifndef PREVIEW_SIZE
#define PREVIEW_SIZE 100
#endif

GtkWidget *mw_app_new(gchar *resname, gchar *appname, gint *runpp);
void mw_fscale_entry_new(GtkWidget *table, char *name,
                         gfloat lorange, gfloat hirange,
                         gfloat st_inc, gfloat pg_inc, gfloat pgsiz,
                         gint left_a, gint right_a, gint top_a,
                         gint bottom_a, gdouble *var);
void mw_iscale_entry_new(GtkWidget *table, char *name,
                         gint lorange, gint hirange,
                         gint st_inc, gint pg_inc, gint pgsiz,
                         gint left_a, gint right_a, gint top_a,
                         gint bottom_a, gint *var);

GSList *mw_radio_new(GSList *gsl, GtkWidget *parent, gchar *name,
             gint *varp, gint init);
GSList *mw_radio_group_new(GtkWidget *parent, gchar *name,
                           struct mwRadioGroup *rg);
gint mw_radio_result(struct mwRadioGroup *rg);
GSList * mw_value_radio_group_new(GtkWidget *parent, gchar *name,
				  struct mwValueRadioGroup *rg, gint *var);

GtkWidget *mw_toggle_button_new(GtkWidget *parent, gchar *fname,
                                gchar *label, gint *varp);
GtkWidget *mw_ientry_button_new(GtkWidget *parent, gchar *fname,
                                gchar *name, gint *varp);
GtkWidget *mw_fentry_button_new(GtkWidget *parent, gchar *fname,
                                gchar *name, gdouble *varp);
struct mwColorSel *mw_color_select_button_create(GtkWidget *parent,
						 gchar *name,
						 gdouble *color,
						 gint opacity);

void mw_ientry_new(GtkWidget *parent, gchar *fname,
		   gchar *name, gint *varp);
void mw_fentry_new(GtkWidget *parent, gchar *fname,
		   gchar *name, gdouble *varp);

typedef void mw_preview_t (GtkWidget *pvw);

GtkWidget *mw_preview_new(GtkWidget *parent, struct mwPreview *mwp,
                          mw_preview_t *fcn);

struct mwPreview *mw_preview_build(GDrawable *drw);
struct mwPreview *mw_preview_build_virgin(GDrawable *drw);

#endif /* MEGAWIDGET_H */
/* end of megawidget/megawidget.h */
