
#include <libgimp/gimp.h>
#include <gtk/gtk.h>

#define STDWIDTH 80.0
#define STDDEPTH 15.0
#define STDUPDOWN 0
#define STDFROMLEFT 0

extern gint magic_dialog(void);
extern gint magiceye_constrain(gint32 image, gint32 drawable_ID, gpointer data);
extern void toggle_update(GtkWidget *widget, gpointer data);
extern void drawable_callback(gint32 id, gpointer data);
extern void ok_callback(GtkWidget *widget, gpointer data);
extern void close_callback(GtkWidget *widget, gpointer data);
extern void magiceye_update(GtkWidget *widget, gpointer data);
extern void scale_update (GtkAdjustment *adjustment, double *scale_val);

extern gdouble strip_width;
extern gdouble depth;
extern gint from_left;
extern gint up_down;
extern gint magiceye_ID;
extern GDrawable *map_drawable;
extern gint magiceye (GDrawable *mapimage, gint32 *layer);

extern void query(void);
extern void run(char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals);
