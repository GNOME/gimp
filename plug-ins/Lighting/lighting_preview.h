#ifndef __LIGHTING_PREVIEW_H__
#define __LIGHTING_PREVIEW_H__

#define PREVIEW_WIDTH 300
#define PREVIEW_HEIGHT 300

typedef struct
{
  gint x,y,w,h;
  GdkImage *image;
} BackBuffer;

/* Externally visible variables */
/* ============================ */

extern gint       lightx,lighty;
extern BackBuffer backbuf;
extern gdouble    *xpostab,*ypostab;

/* Externally visible functions */
/* ============================ */

void draw_preview_image (gint recompute);
gint check_light_hit    (gint xpos,
			 gint ypos);
void update_light       (gint xpos,
			 gint ypos);

#endif  /* __LIGHTING_PREVIEW_H__ */
