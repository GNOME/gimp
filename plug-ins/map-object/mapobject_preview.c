/*************************************************/
/* Compute a preview image and preview wireframe */
/*************************************************/

#include "config.h"

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "mapobject_main.h"
#include "mapobject_ui.h"
#include "mapobject_image.h"
#include "mapobject_apply.h"
#include "mapobject_shade.h"
#include "mapobject_preview.h"

line    linetab[WIRESIZE*2+8];
gdouble mat[3][4];

gint       lightx, lighty;
BackBuffer backbuf = { 0, 0, 0, 0, NULL };

/* Protos */
/* ====== */

static void draw_light_marker       (gint        xpos,
				     gint        ypos);
static void clear_light_marker      (void);

static gint draw_line               (gint        n,
				     gint        startx,
				     gint        starty,
				     gint        pw,
				     gint        ph,
				     gdouble     cx1,
				     gdouble     cy1,
				     gdouble     cx2,
				     gdouble     cy2,
				     GimpVector3 a,
				     GimpVector3 b);

static void draw_wireframe_plane    (gint        startx,
				     gint        starty,
				     gint        pw,
				     gint        ph);
static void draw_wireframe_sphere   (gint        startx,
				     gint        starty,
				     gint        pw,
				     gint        ph);
static void draw_wireframe_box      (gint        startx,
				     gint        starty,
				     gint        pw,
				     gint        ph);
static void draw_wireframe_cylinder (gint        startx,
				     gint        starty,
				     gint        pw,
				     gint        ph);

/*************************************************/
/* Quick and dirty (and slow) line clip routine. */
/* The function returns FALSE if the line isn't  */
/* visible according to the limits given.        */
/*************************************************/

static gboolean
clip_line (gdouble *x1,
	   gdouble *y1,
	   gdouble *x2,
	   gdouble *y2,
	   gdouble  minx,
	   gdouble  miny,
	   gdouble  maxx,
	   gdouble  maxy)
{
  gdouble tmp;

  g_assert (x1 != NULL);
  g_assert (y1 != NULL);
  g_assert (x2 != NULL);
  g_assert (y2 != NULL);

  /* First, check if line is visible at all */
  /* ====================================== */

  if (*x1 < minx && *x2 < minx) return FALSE;
  if (*x1 > maxx && *x2 > maxx) return FALSE;
  if (*y1 < miny && *y2 < miny) return FALSE;
  if (*y1 > maxy && *y2 > maxy) return FALSE;

  /* Check for intersection with the four edges. Sort on x first. */
  /* ============================================================ */

  if (*x2 < *x1)
    {
      tmp = *x1;
      *x1 = *x2;
      *x2 = tmp;
      tmp = *y1;
      *y1 = *y2;
      *y2 = tmp;
    }

  if (*x1 < minx)
    {
      if (*y1 < *y2) *y1 = *y1 + (minx - *x1) * ((*y2 - *y1) / (*x2 - *x1));
      else           *y1 = *y1 - (minx - *x1) * ((*y1 - *y2) / (*x2 - *x1));

      *x1 = minx;
    }

  if (*x2 > maxx)
    {
      if (*y1 < *y2) *y2 = *y2 - (*x2 - maxx) * ((*y2 - *y1) / (*x2 - *x1));
      else           *y2 = *y2 + (*x2 - maxx) * ((*y1 - *y2) / (*x2 - *x1));

      *x2 = maxx;
    }

  if (*y1 < miny)
    {
      *x1 = *x1 + (miny - *y1) * ((*x2 - *x1) / (*y2 - *y1));
      *y1 = miny;
    }

  if (*y2 < miny)
    {
      *x2 = *x2 - (miny - *y2) * ((*x2 - *x1) / (*y1 - *y2));
      *y2 = miny;
    }

  if (*y1 > maxy)
    {
      *x1 = *x1 + (*y1 - maxy) * ((*x2 - *x1) / (*y1 - *y2));
      *y1 = maxy;
    }

  if (*y2 > maxy)
    {
      *x2 = *x2 - (*y2 - maxy) * ((*x2 - *x1) / (*y2 - *y1));
      *y2 = maxy;
    }

  return TRUE;
}

/**************************************************************/
/* Computes a preview of the rectangle starting at (x,y) with */
/* dimensions (w,h), placing the result in preview_RGB_data.  */
/**************************************************************/

void
compute_preview (gint x,
		 gint y,
		 gint w,
		 gint h,
		 gint pw,
		 gint ph)
{
  gdouble      xpostab[PREVIEW_WIDTH];
  gdouble      ypostab[PREVIEW_HEIGHT];
  gdouble      realw;
  gdouble      realh;
  GimpVector3  p1, p2;
  GimpRGB      color;
  GimpRGB      lightcheck, darkcheck;
  gint         xcnt, ycnt, f1, f2;
  glong        index = 0;

  init_compute ();

  p1 = int_to_pos (x, y);
  p2 = int_to_pos (x + w, y + h);

  /* First, compute the linear mapping (x,y,x+w,y+h) to (0,0,pw,ph) */
  /* ============================================================== */

  realw = (p2.x - p1.x);
  realh = (p2.y - p1.y);

  for (xcnt = 0; xcnt < pw; xcnt++)
    xpostab[xcnt] = p1.x + realw * ((gdouble) xcnt / (gdouble) pw);

  for (ycnt = 0; ycnt < ph; ycnt++)
    ypostab[ycnt] = p1.y + realh * ((gdouble) ycnt / (gdouble) ph);

  /* Compute preview using the offset tables */
  /* ======================================= */

  if (mapvals.transparent_background == TRUE)
    {
      gimp_rgba_set (&background, 0.0, 0.0, 0.0, 0.0);
    }
  else
    {
      gimp_context_get_background (&background);
      gimp_rgb_set_alpha (&background, 1.0);
    }

  gimp_rgba_set (&lightcheck,
		 GIMP_CHECK_LIGHT, GIMP_CHECK_LIGHT, GIMP_CHECK_LIGHT, 1.0);
  gimp_rgba_set (&darkcheck,
		 GIMP_CHECK_DARK, GIMP_CHECK_DARK, GIMP_CHECK_DARK, 1.0);
  gimp_vector3_set (&p2, -1.0, -1.0, 0.0);

  for (ycnt = 0; ycnt < ph; ycnt++)
    {
      for (xcnt = 0; xcnt < pw; xcnt++)
        {
          p1.x = xpostab[xcnt];
          p1.y = ypostab[ycnt];

          p2 = p1;
          color = (* get_ray_color) (&p1);

          if (color.a < 1.0)
            {
              f1 = ((xcnt % 32) < 16);
              f2 = ((ycnt % 32) < 16);
              f1 = f1 ^ f2;

              if (f1)
                {
                  if (color.a == 0.0)
		    color = lightcheck;
                  else
		    gimp_rgb_composite (&color, &lightcheck,
					GIMP_RGB_COMPOSITE_BEHIND);
                 }
              else
                {
                  if (color.a == 0.0)
		    color = darkcheck;
                  else
		    gimp_rgb_composite (&color, &darkcheck,
					GIMP_RGB_COMPOSITE_BEHIND);
                }
            }

	  gimp_rgb_get_uchar (&color,
			      preview_rgb_data + index,
			      preview_rgb_data + index + 1,
			      preview_rgb_data + index + 2);
	  index += 3;
        }
    }
}

/*************************************************/
/* Check if the given position is within the     */
/* light marker. Return TRUE if so, FALSE if not */
/*************************************************/

gint
check_light_hit (gint xpos,
		 gint ypos)
{
  gdouble dx, dy, r;

  if (mapvals.lightsource.type == POINT_LIGHT)
    {
      dx = (gdouble) lightx - xpos;
      dy = (gdouble) lighty - ypos;
      r  = sqrt (dx * dx + dy * dy) + 0.5;

      if ((gint) r > 7)
        return FALSE;
      else
        return TRUE;
    }

  return FALSE;
}

/****************************************/
/* Draw a marker to show light position */
/****************************************/

static void
draw_light_marker (gint xpos,
		   gint ypos)
{
  GdkColor  color;

  color.red   = 0x0;
  color.green = 0x0;
  color.blue  = 0x0;
  gdk_gc_set_rgb_bg_color (gc, &color);

  color.red   = 0x0;
  color.green = 0x4000;
  color.blue  = 0xFFFF;
  gdk_gc_set_rgb_fg_color (gc, &color);

  gdk_gc_set_function (gc, GDK_COPY);

  if (mapvals.lightsource.type == POINT_LIGHT)
    {
      lightx = xpos;
      lighty = ypos;

      /* Save background */
      /* =============== */

      backbuf.x = lightx - 7;
      backbuf.y = lighty - 7;
      backbuf.w = 14;
      backbuf.h = 14;

      /* X doesn't like images that's outside a window, make sure */
      /* we get the backbuffer image from within the boundaries   */
      /* ======================================================== */

      if (backbuf.x < 0)
        backbuf.x = 0;
      else if ((backbuf.x + backbuf.w) > PREVIEW_WIDTH)
        backbuf.w = (PREVIEW_WIDTH - backbuf.x);
      if (backbuf.y < 0)
        backbuf.y = 0;
      else if ((backbuf.y + backbuf.h) > PREVIEW_HEIGHT)
        backbuf.h = (PREVIEW_WIDTH - backbuf.y);

      backbuf.image = gdk_drawable_get_image (previewarea->window,
                                              backbuf.x, backbuf.y,
                                              backbuf.w, backbuf.h);

      gdk_draw_arc (previewarea->window, gc,
		    TRUE,
		    lightx - 7, lighty - 7, 14, 14,
		    0, 360 * 64);
    }
}

static void
clear_light_marker (void)
{
  /* Restore background if it has been saved */
  /* ======================================= */

  if (backbuf.image != NULL)
    {
      GdkColor  color;

      color.red   = 0x0;
      color.green = 0x0;
      color.blue  = 0x0;
      gdk_gc_set_rgb_bg_color (gc, &color);

      color.red   = 0xFFFF;
      color.green = 0xFFFF;
      color.blue  = 0xFFFF;
      gdk_gc_set_rgb_fg_color (gc, &color);

      gdk_gc_set_function (gc, GDK_COPY);

      gdk_draw_image (previewarea->window, gc,
		      backbuf.image,
		      0, 0,
		      backbuf.x, backbuf.y,
		      backbuf.w, backbuf.h);

      g_object_unref (backbuf.image);
      backbuf.image = NULL;
    }
}

static void
draw_lights (gint startx,
	     gint starty,
	     gint pw,
	     gint ph)
{
  gdouble dxpos, dypos;
  gint    xpos, ypos;

  clear_light_marker ();

  gimp_vector_3d_to_2d (startx, starty, pw, ph,
			&dxpos, &dypos, &mapvals.viewpoint,
			&mapvals.lightsource.position);
  xpos = RINT (dxpos);
  ypos = RINT (dypos);

  if (xpos >= 0 && xpos <= PREVIEW_WIDTH &&
      ypos >= 0 && ypos <= PREVIEW_HEIGHT)
    draw_light_marker (xpos, ypos);
}

/*************************************************/
/* Update light position given new screen coords */
/*************************************************/

void
update_light (gint xpos,
	      gint ypos)
{
  gint    startx, starty, pw, ph;

  pw     = PREVIEW_WIDTH * mapvals.zoom;
  ph     = PREVIEW_HEIGHT * mapvals.zoom;
  startx = (PREVIEW_WIDTH  - pw) / 2;
  starty = (PREVIEW_HEIGHT - ph) / 2;

  gimp_vector_2d_to_3d (startx, starty, pw, ph, xpos, ypos,
			&mapvals.viewpoint, &mapvals.lightsource.position);
  draw_lights (startx, starty,pw, ph);
}

/******************************************************************/
/* Draw preview image. if DoCompute is TRUE then recompute image. */
/******************************************************************/

void
draw_preview_image (gint docompute)
{
  gint startx, starty, pw, ph;
  GdkColor  color;

  color.red   = 0x0;
  color.green = 0x0;
  color.blue  = 0x0;
  gdk_gc_set_rgb_bg_color (gc, &color);

  color.red   = 0xFFFF;
  color.green = 0xFFFF;
  color.blue  = 0xFFFF;
  gdk_gc_set_rgb_fg_color (gc, &color);

  gdk_gc_set_function (gc, GDK_COPY);
  linetab[0].x1 = -1;

  pw = PREVIEW_WIDTH * mapvals.zoom;
  ph = PREVIEW_HEIGHT * mapvals.zoom;
  startx = (PREVIEW_WIDTH - pw) / 2;
  starty = (PREVIEW_HEIGHT - ph) / 2;

  if (docompute == TRUE)
    {
      GdkDisplay *display = gtk_widget_get_display (previewarea);
      GdkCursor  *cursor;

      cursor = gdk_cursor_new_for_display (display, GDK_WATCH);
      gdk_window_set_cursor (previewarea->window, cursor);
      gdk_cursor_unref (cursor);

      compute_preview (0, 0, width - 1, height - 1, pw, ph);

      cursor = gdk_cursor_new_for_display (display, GDK_HAND2);
      gdk_window_set_cursor(previewarea->window, cursor);
      gdk_cursor_unref (cursor);

      clear_light_marker ();
    }

  if (pw != PREVIEW_WIDTH || ph != PREVIEW_HEIGHT)
    gdk_window_clear (previewarea->window);

  gdk_draw_rgb_image (previewarea->window, gc,
                      startx, starty, pw, ph,
                      GDK_RGB_DITHER_MAX,
                      preview_rgb_data, 3 * pw);

  draw_lights (startx, starty, pw, ph);
}

/**************************/
/* Draw preview wireframe */
/**************************/

void
draw_preview_wireframe (void)
{
  gint      startx, starty, pw, ph;
  GdkColor  color;

  color.red   = 0x0;
  color.green = 0x0;
  color.blue  = 0x0;
  gdk_gc_set_rgb_bg_color (gc, &color);

  color.red   = 0xFFFF;
  color.green = 0xFFFF;
  color.blue  = 0xFFFF;
  gdk_gc_set_rgb_fg_color (gc, &color);

  gdk_gc_set_function (gc, GDK_INVERT);

  pw     = PREVIEW_WIDTH  * mapvals.zoom;
  ph     = PREVIEW_HEIGHT * mapvals.zoom;
  startx = (PREVIEW_WIDTH  - pw) / 2;
  starty = (PREVIEW_HEIGHT - ph) / 2;

  clear_wireframe ();
  draw_wireframe (startx, starty, pw, ph);
}

/****************************/
/* Draw a wireframe preview */
/****************************/

void
draw_wireframe (gint startx,
		gint starty,
		gint pw,
		gint ph)
{
  switch (mapvals.maptype)
    {
    case MAP_PLANE:
      draw_wireframe_plane (startx, starty, pw, ph);
      break;
    case MAP_SPHERE:
      draw_wireframe_sphere (startx, starty, pw, ph);
      break;
    case MAP_BOX:
      draw_wireframe_box (startx, starty, pw, ph);
      break;
    case MAP_CYLINDER:
      draw_wireframe_cylinder (startx, starty, pw, ph);
      break;
    }
}

static void
draw_wireframe_plane (gint startx,
		      gint starty,
		      gint pw,
		      gint ph)
{
  GimpVector3 v1, v2, a, b, c, d, dir1, dir2;
  gint        cnt, n = 0;
  gdouble     x1, y1, x2, y2, cx1, cy1, cx2, cy2, fac;

  /* Find rotated box corners */
  /* ======================== */

  gimp_vector3_set (&v1, 0.5, 0.0, 0.0);
  gimp_vector3_set (&v2, 0.0, 0.5, 0.0);

  gimp_vector3_rotate (&v1,
		       gimp_deg_to_rad (mapvals.alpha),
		       gimp_deg_to_rad (mapvals.beta),
		       gimp_deg_to_rad (mapvals.gamma));

  gimp_vector3_rotate (&v2,
		       gimp_deg_to_rad (mapvals.alpha),
		       gimp_deg_to_rad (mapvals.beta),
		       gimp_deg_to_rad (mapvals.gamma));

  dir1 = v1; gimp_vector3_normalize (&dir1);
  dir2 = v2; gimp_vector3_normalize (&dir2);

  fac = 1.0 / (gdouble) WIRESIZE;

  gimp_vector3_mul (&dir1, fac);
  gimp_vector3_mul (&dir2, fac);

  gimp_vector3_add (&a, &mapvals.position, &v1);
  gimp_vector3_sub (&b, &a, &v2);
  gimp_vector3_add (&a, &a, &v2);
  gimp_vector3_sub (&d, &mapvals.position, &v1);
  gimp_vector3_sub (&d, &d, &v2);

  c = b;

  cx1 = (gdouble) startx;
  cy1 = (gdouble) starty;
  cx2 = cx1 + (gdouble) pw;
  cy2 = cy1 + (gdouble) ph;

  for (cnt = 0; cnt <= WIRESIZE; cnt++)
    {
      gimp_vector_3d_to_2d (startx, starty, pw, ph,
			    &x1, &y1, &mapvals.viewpoint, &a);
      gimp_vector_3d_to_2d (startx, starty, pw, ph,
			    &x2, &y2, &mapvals.viewpoint, &b);

      if (clip_line (&x1, &y1, &x2, &y2, cx1, cy1, cx2, cy2) == TRUE)
        {
          linetab[n].x1 = RINT (x1);
          linetab[n].y1 = RINT (y1);
          linetab[n].x2 = RINT (x2);
          linetab[n].y2 = RINT (y2);
          linetab[n].linewidth = 1;
          linetab[n].linestyle = GDK_LINE_SOLID;
          gdk_gc_set_line_attributes (gc,
				      linetab[n].linewidth,
				      linetab[n].linestyle,
				      GDK_CAP_NOT_LAST,
				      GDK_JOIN_MITER);
          gdk_draw_line (previewarea->window, gc,
			 linetab[n].x1, linetab[n].y1,
			 linetab[n].x2, linetab[n].y2);
          n++;
        }

      gimp_vector_3d_to_2d (startx, starty, pw, ph,
			    &x1, &y1, &mapvals.viewpoint, &c);
      gimp_vector_3d_to_2d (startx, starty, pw, ph,
			    &x2, &y2, &mapvals.viewpoint, &d);

      if (clip_line (&x1, &y1, &x2, &y2, cx1, cy1, cx2, cy2) == TRUE)
        {
          linetab[n].x1 = RINT (x1);
          linetab[n].y1 = RINT (y1);
          linetab[n].x2 = RINT (x2);
          linetab[n].y2 = RINT (y2);
          linetab[n].linewidth = 1;
          linetab[n].linestyle = GDK_LINE_SOLID;
          gdk_gc_set_line_attributes (gc,
				      linetab[n].linewidth,
				      linetab[n].linestyle,
				      GDK_CAP_NOT_LAST,
				      GDK_JOIN_MITER);
          gdk_draw_line (previewarea->window, gc,
			 linetab[n].x1, linetab[n].y1,
			 linetab[n].x2, linetab[n].y2);
          n++;
        }

      gimp_vector3_sub (&a, &a, &dir1);
      gimp_vector3_sub (&b, &b, &dir1);
      gimp_vector3_add (&c, &c, &dir2);
      gimp_vector3_add (&d, &d, &dir2);
    }

  /* Mark end of lines */
  /* ================= */

  linetab[n].x1 = -1;
}

static void
draw_wireframe_sphere (gint startx,
		       gint starty,
		       gint pw,
		       gint ph)
{
  GimpVector3 p[2 * (WIRESIZE + 5)];
  gint        cnt, cnt2, n = 0;
  gdouble     x1, y1, x2, y2, twopifac, cx1, cy1, cx2, cy2;

  /* Compute wireframe points */
  /* ======================== */

  twopifac = (2.0 * G_PI) / WIRESIZE;

  for (cnt = 0; cnt < WIRESIZE; cnt++)
    {
      p[cnt].x = mapvals.radius * cos ((gdouble) cnt * twopifac);
      p[cnt].y = 0.0;
      p[cnt].z = mapvals.radius * sin ((gdouble) cnt * twopifac);
      gimp_vector3_rotate (&p[cnt],
			   gimp_deg_to_rad (mapvals.alpha),
			   gimp_deg_to_rad (mapvals.beta),
			   gimp_deg_to_rad (mapvals.gamma));
      gimp_vector3_add (&p[cnt], &p[cnt], &mapvals.position);
    }

  p[cnt] = p[0];

  for (cnt = WIRESIZE + 1; cnt < 2 * WIRESIZE + 1; cnt++)
    {
      p[cnt].x = mapvals.radius * cos ((gdouble) (cnt-(WIRESIZE+1))*twopifac);
      p[cnt].y = mapvals.radius * sin ((gdouble) (cnt-(WIRESIZE+1))*twopifac);
      p[cnt].z = 0.0;
      gimp_vector3_rotate (&p[cnt],
			   gimp_deg_to_rad (mapvals.alpha),
			   gimp_deg_to_rad (mapvals.beta),
			   gimp_deg_to_rad (mapvals.gamma));
      gimp_vector3_add (&p[cnt], &p[cnt], &mapvals.position);
    }

  p[cnt] = p[WIRESIZE+1];
  cnt++;
  cnt2 = cnt;

  /* Find rotated axis */
  /* ================= */

  gimp_vector3_set (&p[cnt], 0.0, -0.35, 0.0);
  gimp_vector3_rotate (&p[cnt],
		       gimp_deg_to_rad (mapvals.alpha),
		       gimp_deg_to_rad (mapvals.beta),
		       gimp_deg_to_rad (mapvals.gamma));
  p[cnt+1] = mapvals.position;

  gimp_vector3_set (&p[cnt+2], 0.0, 0.0, -0.35);
  gimp_vector3_rotate (&p[cnt+2],
		       gimp_deg_to_rad (mapvals.alpha),
		       gimp_deg_to_rad (mapvals.beta),
		       gimp_deg_to_rad (mapvals.gamma));
  p[cnt+3] = mapvals.position;

  p[cnt + 4] = p[cnt];
  gimp_vector3_mul (&p[cnt + 4], -1.0);
  p[cnt + 5] = p[cnt + 1];

  gimp_vector3_add (&p[cnt],     &p[cnt],     &mapvals.position);
  gimp_vector3_add (&p[cnt + 2], &p[cnt + 2], &mapvals.position);
  gimp_vector3_add (&p[cnt + 4], &p[cnt + 4], &mapvals.position);

  /* Draw the circles (equator and zero meridian) */
  /* ============================================ */

  cx1 = (gdouble) startx;
  cy1 = (gdouble) starty;
  cx2 = cx1 + (gdouble) pw;
  cy2 = cy1 + (gdouble) ph;

  for (cnt = 0; cnt < cnt2 - 1; cnt++)
    {
      if (p[cnt].z > mapvals.position.z && p[cnt + 1].z > mapvals.position.z)
        {
          gimp_vector_3d_to_2d (startx, starty, pw, ph,
				&x1, &y1, &mapvals.viewpoint, &p[cnt]);
          gimp_vector_3d_to_2d (startx, starty, pw, ph,
				&x2, &y2, &mapvals.viewpoint, &p[cnt + 1]);

          if (clip_line (&x1, &y1, &x2, &y2, cx1, cy1, cx2, cy2) == TRUE)
            {
              linetab[n].x1 = (gint) (x1 + 0.5);
              linetab[n].y1 = (gint) (y1 + 0.5);
              linetab[n].x2 = (gint) (x2 + 0.5);
              linetab[n].y2 = (gint) (y2 + 0.5);
              linetab[n].linewidth = 3;
              linetab[n].linestyle = GDK_LINE_SOLID;
              gdk_gc_set_line_attributes (gc,
					  linetab[n].linewidth,
					  linetab[n].linestyle,
					  GDK_CAP_NOT_LAST,
					  GDK_JOIN_MITER);
              gdk_draw_line (previewarea->window, gc,
			     linetab[n].x1, linetab[n].y1,
			     linetab[n].x2, linetab[n].y2);
              n++;
            }
        }
    }

  /* Draw the axis (pole to pole and center to zero meridian) */
  /* ======================================================== */

  for (cnt = 0; cnt < 3; cnt++)
    {
      gimp_vector_3d_to_2d (startx, starty, pw, ph,
			    &x1, &y1, &mapvals.viewpoint, &p[cnt2]);
      gimp_vector_3d_to_2d (startx, starty, pw, ph,
			    &x2, &y2, &mapvals.viewpoint, &p[cnt2 + 1]);

      if (clip_line (&x1, &y1, &x2, &y2, cx1, cy1, cx2, cy2) == TRUE)
        {
          linetab[n].x1 = RINT (x1);
          linetab[n].y1 = RINT (y1);
          linetab[n].x2 = RINT (x2);
          linetab[n].y2 = RINT (y2);

          if (p[cnt2].z < mapvals.position.z || p[cnt2+1].z < mapvals.position.z)
            {
              linetab[n].linewidth = 1;
              linetab[n].linestyle = GDK_LINE_DOUBLE_DASH;
            }
          else
            {
              linetab[n].linewidth = 3;
              linetab[n].linestyle = GDK_LINE_SOLID;
            }

          gdk_gc_set_line_attributes (gc,
				      linetab[n].linewidth,
				      linetab[n].linestyle,
				      GDK_CAP_NOT_LAST,
				      GDK_JOIN_MITER);
          gdk_draw_line (previewarea->window, gc,
			 linetab[n].x1, linetab[n].y1,
			 linetab[n].x2, linetab[n].y2);
          n++;
        }

      cnt2 += 2;
    }

  /* Mark end of lines */
  /* ================= */

  linetab[n].x1 = -1;
}

static gint
draw_line (gint        n,
	   gint        startx,
	   gint        starty,
	   gint        pw,
	   gint        ph,
	   gdouble     cx1,
	   gdouble     cy1,
	   gdouble     cx2,
	   gdouble     cy2,
	   GimpVector3 a,
	   GimpVector3 b)
{
  gdouble x1, y1, x2, y2;
  gint    i = n;

  gimp_vector_3d_to_2d (startx, starty, pw, ph,
			&x1, &y1, &mapvals.viewpoint, &a);
  gimp_vector_3d_to_2d (startx, starty, pw, ph,
			&x2, &y2, &mapvals.viewpoint, &b);

  if (clip_line (&x1, &y1, &x2, &y2, cx1, cy1, cx2, cy2) == TRUE)
    {
      linetab[i].x1 = RINT (x1);
      linetab[i].y1 = RINT (y1);
      linetab[i].x2 = RINT (x2);
      linetab[i].y2 = RINT (y2);
      linetab[i].linewidth = 3;
      linetab[i].linestyle = GDK_LINE_SOLID;
      gdk_gc_set_line_attributes (gc,
				  linetab[i].linewidth,
				  linetab[i].linestyle,
				  GDK_CAP_NOT_LAST,
				  GDK_JOIN_MITER);
      gdk_draw_line (previewarea->window, gc,
		     linetab[i].x1, linetab[i].y1,
		     linetab[i].x2, linetab[i].y2);
      i++;
    }

  return i;
}

static void
draw_wireframe_box (gint startx,
		    gint starty,
		    gint pw,
		    gint ph)
{
  GimpVector3 p[8], tmp, scale;
  gint        n = 0, i;
  gdouble     cx1, cy1, cx2, cy2;

  /* Compute wireframe points */
  /* ======================== */

  init_compute ();

  scale = mapvals.scale;
  gimp_vector3_mul (&scale, 0.5);

  gimp_vector3_set (&p[0], -scale.x, -scale.y, scale.z);
  gimp_vector3_set (&p[1],  scale.x, -scale.y, scale.z);
  gimp_vector3_set (&p[2],  scale.x,  scale.y, scale.z);
  gimp_vector3_set (&p[3], -scale.x,  scale.y, scale.z);

  gimp_vector3_set (&p[4], -scale.x, -scale.y, -scale.z);
  gimp_vector3_set (&p[5],  scale.x, -scale.y, -scale.z);
  gimp_vector3_set (&p[6],  scale.x,  scale.y, -scale.z);
  gimp_vector3_set (&p[7], -scale.x,  scale.y, -scale.z);

  /* Rotate and translate points */
  /* =========================== */

  for (i = 0; i < 8; i++)
    {
      vecmulmat (&tmp, &p[i], rotmat);
      gimp_vector3_add (&p[i], &tmp, &mapvals.position);
    }

  /* Draw the box */
  /* ============ */

  cx1 = (gdouble) startx;
  cy1 = (gdouble) starty;
  cx2 = cx1 + (gdouble) pw;
  cy2 = cy1 + (gdouble) ph;

  n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[0],p[1]);
  n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[1],p[2]);
  n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[2],p[3]);
  n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[3],p[0]);

  n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[4],p[5]);
  n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[5],p[6]);
  n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[6],p[7]);
  n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[7],p[4]);

  n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[0],p[4]);
  n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[1],p[5]);
  n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[2],p[6]);
  n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[3],p[7]);

  /* Mark end of lines */
  /* ================= */

  linetab[n].x1 = -1;
}

static void
draw_wireframe_cylinder (gint startx,
			 gint starty,
			 gint pw,
			 gint ph)
{
  GimpVector3 p[2*8], a, axis, scale;
  gint        n = 0, i;
  gdouble     cx1, cy1, cx2, cy2;
  gfloat      m[16], l, angle;

  /* Compute wireframe points */
  /* ======================== */

  init_compute ();

  scale = mapvals.scale;
  gimp_vector3_mul (&scale, 0.5);

  l = mapvals.cylinder_length / 2.0;
  angle = 0;

  gimp_vector3_set (&axis, 0.0, 1.0, 0.0);

  for (i = 0; i < 8; i++)
    {
      rotatemat (angle, &axis, m);

      gimp_vector3_set (&a, mapvals.cylinder_radius, 0.0, 0.0);

      vecmulmat (&p[i], &a, m);

      p[i+8] = p[i];

      p[i].y   += l;
      p[i+8].y -= l;

      angle += 360.0 / 8.0;
    }

  /* Rotate and translate points */
  /* =========================== */

  for (i = 0; i < 16; i++)
    {
      vecmulmat (&a, &p[i], rotmat);
      gimp_vector3_add (&p[i], &a, &mapvals.position);
    }

  /* Draw the box */
  /* ============ */

  cx1 = (gdouble) startx;
  cy1 = (gdouble) starty;
  cx2 = cx1 + (gdouble) pw;
  cy2 = cy1 + (gdouble) ph;

  for (i = 0; i < 7; i++)
    {
      n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[i],p[i+1]);
      n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[i+8],p[i+9]);
      n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[i],p[i+8]);
    }

  n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[7],p[0]);
  n = draw_line (n, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[15],p[8]);

  /* Mark end of lines */
  /* ================= */

  linetab[n].x1 = -1;
}

void
clear_wireframe (void)
{
  gint n = 0;

  while (linetab[n].x1 != -1)
    {
      gdk_gc_set_line_attributes (gc,
				  linetab[n].linewidth,
				  linetab[n].linestyle,
				  GDK_CAP_NOT_LAST,
				  GDK_JOIN_MITER);
      gdk_draw_line (previewarea->window, gc,
		     linetab[n].x1, linetab[n].y1,
		     linetab[n].x2, linetab[n].y2);
      n++;
    }
}
