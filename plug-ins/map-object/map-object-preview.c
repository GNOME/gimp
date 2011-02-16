/*************************************************/
/* Compute a preview image and preview wireframe */
/*************************************************/

#include "config.h"

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "map-object-main.h"
#include "map-object-ui.h"
#include "map-object-image.h"
#include "map-object-apply.h"
#include "map-object-shade.h"
#include "map-object-preview.h"


gdouble mat[3][4];
gint    lightx, lighty;

/* Protos */
/* ====== */

static void compute_preview         (gint x,
                                     gint y,
                                     gint w,
                                     gint h,
                                     gint pw,
                                     gint ph);
static void draw_light_marker       (cairo_t *cr,
                                     gint xpos,
                                     gint ypos);
static void draw_line               (cairo_t    *cr,
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
static void draw_wireframe          (cairo_t *cr,
                                     gint startx,
                                     gint starty,
                                     gint pw,
                                     gint ph);
static void draw_preview_wireframe  (cairo_t *cr);
static void draw_wireframe_plane    (cairo_t *cr,
                                     gint        startx,
                                     gint        starty,
                                     gint        pw,
                                     gint        ph);
static void draw_wireframe_sphere   (cairo_t *cr,
                                     gint        startx,
                                     gint        starty,
                                     gint        pw,
                                     gint        ph);
static void draw_wireframe_box      (cairo_t *cr,
                                     gint        startx,
                                     gint        starty,
                                     gint        pw,
                                     gint        ph);
static void draw_wireframe_cylinder (cairo_t *cr,
                                     gint        startx,
                                     gint        starty,
                                     gint        pw,
                                     gint        ph);

/**************************************************************/
/* Computes a preview of the rectangle starting at (x,y) with */
/* dimensions (w,h), placing the result in preview_RGB_data.  */
/**************************************************************/

static void
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
  guchar       r, g, b;
  glong        index = 0;

  init_compute ();

  if (! preview_surface)
    return;

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

  cairo_surface_flush (preview_surface);

  for (ycnt = 0; ycnt < ph; ycnt++)
    {
      index = ycnt * preview_rgb_stride;
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

          gimp_rgb_get_uchar (&color, &r, &g, &b);
          GIMP_CAIRO_RGB24_SET_PIXEL((preview_rgb_data + index), r, g, b);
          index += 4;
        }
    }
  cairo_surface_mark_dirty (preview_surface);
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
draw_light_marker (cairo_t *cr,
                   gint xpos,
                   gint ypos)
{
  GdkColor color;

  if (mapvals.lightsource.type != POINT_LIGHT)
    return;

  cairo_set_line_width (cr, 1.0);

  color.red   = 0x0;
  color.green = 0x4000;
  color.blue  = 0xFFFF;
  gdk_cairo_set_source_color (cr, &color);

  lightx = xpos;
  lighty = ypos;

  cairo_arc (cr, lightx, lighty, 7, 0, 2 * G_PI);
  cairo_fill (cr);
}

static void
draw_lights (cairo_t *cr,
             gint startx,
             gint starty,
             gint pw,
             gint ph)
{
  gdouble dxpos, dypos;
  gint    xpos, ypos;

  gimp_vector_3d_to_2d (startx, starty, pw, ph,
                        &dxpos, &dypos, &mapvals.viewpoint,
                        &mapvals.lightsource.position);
  xpos = RINT (dxpos);
  ypos = RINT (dypos);

  if (xpos >= 0 && xpos <= PREVIEW_WIDTH &&
      ypos >= 0 && ypos <= PREVIEW_HEIGHT)
    {
      draw_light_marker (cr, xpos, ypos);
    }
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

  gtk_widget_queue_draw (previewarea);
}

/**************************/
/* Compute preview image. */
/**************************/

void
compute_preview_image (void)
{
  GdkDisplay *display = gtk_widget_get_display (previewarea);
  GdkCursor  *cursor;
  gint        pw, ph;

  pw = PREVIEW_WIDTH * mapvals.zoom;
  ph = PREVIEW_HEIGHT * mapvals.zoom;

  cursor = gdk_cursor_new_for_display (display, GDK_WATCH);
  gdk_window_set_cursor (gtk_widget_get_window (previewarea), cursor);
  g_object_unref (cursor);

  compute_preview (0, 0, width - 1, height - 1, pw, ph);

  cursor = gdk_cursor_new_for_display (display, GDK_HAND2);
  gdk_window_set_cursor(gtk_widget_get_window (previewarea), cursor);
  g_object_unref (cursor);
}

gboolean
preview_draw (GtkWidget *widget,
              cairo_t   *cr)
{
  gint startx, starty, pw, ph;

  pw = PREVIEW_WIDTH * mapvals.zoom;
  ph = PREVIEW_HEIGHT * mapvals.zoom;
  startx = (PREVIEW_WIDTH - pw) / 2;
  starty = (PREVIEW_HEIGHT - ph) / 2;

  cairo_set_source_surface (cr, preview_surface, startx, starty);
  cairo_rectangle (cr, startx, starty, pw, ph);
  cairo_clip (cr);

  cairo_paint (cr);

  cairo_reset_clip (cr);

  if (mapvals.showgrid)
    draw_preview_wireframe (cr);

  cairo_reset_clip (cr);
  draw_lights (cr, startx, starty, pw, ph);

  return FALSE;
}

/**************************/
/* Draw preview wireframe */
/**************************/

void
draw_preview_wireframe (cairo_t *cr)
{
  gint      startx, starty, pw, ph;

  pw     = PREVIEW_WIDTH  * mapvals.zoom;
  ph     = PREVIEW_HEIGHT * mapvals.zoom;
  startx = (PREVIEW_WIDTH  - pw) / 2;
  starty = (PREVIEW_HEIGHT - ph) / 2;

  draw_wireframe (cr, startx, starty, pw, ph);
}

/****************************/
/* Draw a wireframe preview */
/****************************/

void
draw_wireframe (cairo_t *cr,
                gint startx,
                gint starty,
                gint pw,
                gint ph)
{
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  switch (mapvals.maptype)
    {
    case MAP_PLANE:
      draw_wireframe_plane (cr, startx, starty, pw, ph);
      break;
    case MAP_SPHERE:
      draw_wireframe_sphere (cr, startx, starty, pw, ph);
      break;
    case MAP_BOX:
      draw_wireframe_box (cr, startx, starty, pw, ph);
      break;
    case MAP_CYLINDER:
      draw_wireframe_cylinder (cr, startx, starty, pw, ph);
      break;
    }
}

static void
draw_wireframe_plane (cairo_t *cr,
                      gint startx,
                      gint starty,
                      gint pw,
                      gint ph)
{
  GimpVector3 v1, v2, a, b, c, d, dir1, dir2;
  gint        cnt;
  gdouble     x1, y1, x2, y2, fac;

  cairo_rectangle (cr, startx, starty, pw, ph);
  cairo_clip (cr);

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

  for (cnt = 0; cnt <= WIRESIZE; cnt++)
    {
      gimp_vector_3d_to_2d (startx, starty, pw, ph,
                            &x1, &y1, &mapvals.viewpoint, &a);
      gimp_vector_3d_to_2d (startx, starty, pw, ph,
                            &x2, &y2, &mapvals.viewpoint, &b);

      cairo_move_to (cr, RINT (x1) + 0.5, RINT (y1) + 0.5);
      cairo_line_to (cr, RINT (x2) + 0.5, RINT (y2) + 0.5);

      gimp_vector_3d_to_2d (startx, starty, pw, ph,
                            &x1, &y1, &mapvals.viewpoint, &c);
      gimp_vector_3d_to_2d (startx, starty, pw, ph,
                            &x2, &y2, &mapvals.viewpoint, &d);

      cairo_move_to (cr, RINT (x1) + 0.5, RINT (y1) + 0.5);
      cairo_line_to (cr, RINT (x2) + 0.5, RINT (y2) + 0.5);

      gimp_vector3_sub (&a, &a, &dir1);
      gimp_vector3_sub (&b, &b, &dir1);
      gimp_vector3_add (&c, &c, &dir2);
      gimp_vector3_add (&d, &d, &dir2);
    }

  cairo_set_line_width (cr, 3.0);
  cairo_stroke_preserve (cr);
  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_stroke (cr);
}

static void
draw_wireframe_sphere (cairo_t *cr,
                       gint startx,
                       gint starty,
                       gint pw,
                       gint ph)
{
  GimpVector3 p[2 * (WIRESIZE + 5)];
  gint        cnt, cnt2;
  gdouble     x1, y1, x2, y2, twopifac;

  cairo_rectangle (cr, startx, starty, pw, ph);
  cairo_clip (cr);

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

  for (cnt = 0; cnt < cnt2 - 1; cnt++)
    {
      if (p[cnt].z > mapvals.position.z && p[cnt + 1].z > mapvals.position.z)
        {
          gimp_vector_3d_to_2d (startx, starty, pw, ph,
                                &x1, &y1, &mapvals.viewpoint, &p[cnt]);
          gimp_vector_3d_to_2d (startx, starty, pw, ph,
                                &x2, &y2, &mapvals.viewpoint, &p[cnt + 1]);

          cairo_move_to (cr, (gint) (x1 + 0.5) + 0.5, (gint) (y1 + 0.5) + 0.5);
          cairo_line_to (cr, (gint) (x2 + 0.5) + 0.5, (gint) (y2 + 0.5) + 0.5);
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

      cairo_move_to (cr, RINT (x1) + 0.5, RINT (y1) + 0.5);
      cairo_line_to (cr, RINT (x2) + 0.5, RINT (y2) + 0.5);

      cnt2 += 2;
    }

  cairo_set_line_width (cr, 3.0);
  cairo_stroke_preserve (cr);
  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_stroke (cr);
}

static void
draw_line (cairo_t    *cr,
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

  gimp_vector_3d_to_2d (startx, starty, pw, ph,
                        &x1, &y1, &mapvals.viewpoint, &a);
  gimp_vector_3d_to_2d (startx, starty, pw, ph,
                        &x2, &y2, &mapvals.viewpoint, &b);

  cairo_move_to (cr, RINT (x1) + 0.5, RINT (y1) + 0.5);
  cairo_line_to (cr, RINT (x2) + 0.5, RINT (y2) + 0.5);
}

static void
draw_wireframe_box (cairo_t *cr,
                    gint startx,
                    gint starty,
                    gint pw,
                    gint ph)
{
  GimpVector3 p[8], tmp, scale;
  gint        i;
  gdouble     cx1, cy1, cx2, cy2;

  cairo_rectangle (cr, startx, starty, pw, ph);
  cairo_clip (cr);

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

  draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[0],p[1]);
  draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[1],p[2]);
  draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[2],p[3]);
  draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[3],p[0]);

  draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[4],p[5]);
  draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[5],p[6]);
  draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[6],p[7]);
  draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[7],p[4]);

  draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[0],p[4]);
  draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[1],p[5]);
  draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[2],p[6]);
  draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[3],p[7]);

  cairo_set_line_width (cr, 3.0);
  cairo_stroke_preserve (cr);
  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_stroke (cr);
}

static void
draw_wireframe_cylinder (cairo_t *cr,
                         gint startx,
                         gint starty,
                         gint pw,
                         gint ph)
{
  GimpVector3 p[2*8], a, axis, scale;
  gint        i;
  gdouble     cx1, cy1, cx2, cy2;
  gfloat      m[16], l, angle;

  cairo_rectangle (cr, startx, starty, pw, ph);
  cairo_clip (cr);

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
      draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[i],p[i+1]);
      draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[i+8],p[i+9]);
      draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[i],p[i+8]);
    }

  draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[7],p[0]);
  draw_line (cr, startx,starty,pw,ph, cx1,cy1,cx2,cy2, p[15],p[8]);

  cairo_set_line_width (cr, 3.0);
  cairo_stroke_preserve (cr);
  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_stroke (cr);
}
