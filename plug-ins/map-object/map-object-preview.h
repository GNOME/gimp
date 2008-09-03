#ifndef __MAPOBJECT_PREVIEW_H__
#define __MAPOBJECT_PREVIEW_H__

#define PREVIEW_WIDTH 200
#define PREVIEW_HEIGHT 200

#define WIRESIZE 16

typedef struct
{
  gint         x1, y1, x2, y2;
  gint         linewidth;
  GdkLineStyle linestyle;
} line;

typedef struct
{
  gint      x, y, w, h;
  GdkImage *image;
} BackBuffer;

/* Externally visible variables */
/* ============================ */

extern line       linetab[];
extern gdouble    mat[3][4];
extern gint       lightx,lighty;
extern BackBuffer backbuf;

/* Externally visible functions */
/* ============================ */

void compute_preview        (gint x,
			     gint y,
			     gint w,
			     gint h,
			     gint pw,
			     gint ph);
void draw_wireframe         (gint startx,
			     gint starty,
			     gint pw,
			     gint ph);
void clear_wireframe        (void);
void draw_preview_image     (gint docompute);
void draw_preview_wireframe (void);
gint check_light_hit        (gint xpos,
			     gint ypos);
void update_light           (gint xpos,
			     gint ypos);

#endif  /* __MAPOBJECT_PREVIEW_H__ */
