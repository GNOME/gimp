#ifndef __MAPOBJECT_PREVIEW_H__
#define __MAPOBJECT_PREVIEW_H__

#define PREVIEW_WIDTH 200
#define PREVIEW_HEIGHT 200

#define WIRESIZE 16

/* Externally visible variables */
/* ============================ */

extern gdouble    mat[3][4];
extern gint       lightx,lighty;

/* Externally visible functions */
/* ============================ */

void     compute_preview_image  (void);
gboolean preview_draw           (GtkWidget *widget,
                                 cairo_t   *cr);
gint     check_light_hit        (gint            xpos,
                                 gint            ypos);
void     update_light           (gint            xpos,
                                 gint            ypos);

#endif  /* __MAPOBJECT_PREVIEW_H__ */
