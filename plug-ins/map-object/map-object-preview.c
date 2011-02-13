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


typedef struct
{
  gint x, y, w, h;
  cairo_surface_t *image;
} BackBuffer;

gdouble mat[3][4];

gint       lightx, lighty;

static BackBuffer backbuf = { 0, 0, 0, 0, NULL };

/* Protos */
/* ====== */

static void draw_light_marker       (gint        xpos,
				     gint        ypos);
static void clear_light_marker      (void);

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
  guchar       r, g, b;
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
draw_light_marker (gint xpos,
		   gint ypos)
{
  GdkColor  color;
  cairo_t *cr, *cr_p;
  gint pw, ph, startx, starty;

  if (mapvals.lightsource.type != POINT_LIGHT)
    return;

  pw = PREVIEW_WIDTH * mapvals.zoom;
  ph = PREVIEW_HEIGHT * mapvals.zoom;
  startx = (PREVIEW_WIDTH - pw) / 2;
  starty = (PREVIEW_HEIGHT - ph) / 2;

  cr = gdk_cairo_create (gtk_widget_get_window (previewarea));

  cairo_set_line_width (cr, 1.0);

  color.red   = 0x0;
  color.green = 0x4000;
  color.blue  = 0xFFFF;
  gdk_cairo_set_source_color (cr, &color);

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

  if (backbuf.x < startx)
    backbuf.x = startx;
  else if ((backbuf.x + backbuf.w) > startx + pw)
    backbuf.w = MAX(startx + pw - backbuf.x, 0);
  if (backbuf.y < starty)
    backbuf.y = starty;
  else if ((backbuf.y + backbuf.h) > starty + ph)
    backbuf.h = MAX(starty + ph - backbuf.y, 0);

  if (backbuf.image)
    cairo_surface_destroy (backbuf.image);

  backbuf.image = cairo_surface_create_similar (preview_surface,
                                                CAIRO_CONTENT_COLOR,
                                                PREVIEW_WIDTH, PREVIEW_HEIGHT);
  cr_p = cairo_create (backbuf.image);
  cairo_rectangle (cr_p, backbuf.x, backbuf.y, backbuf.w, backbuf.h);
  cairo_clip (cr_p);
  cairo_set_source_surface (cr_p, preview_surface, startx, starty);
  cairo_paint (cr_p);
  cairo_destroy (cr_p);

  cairo_arc (cr, lightx, lighty, 7, 0, 2 * M_PI);
  cairo_fill (cr);

  cairo_destroy (cr);
}

static void
clear_light_marker (void)
{
  /* Restore background if it has been saved */
  /* ======================================= */

  gint pw, ph, startx, starty;

  if (backbuf.image != NULL)
    {
      cairo_t *cr;

      cr = gdk_cairo_create (gtk_widget_get_window (previewarea));

      cairo_rectangle (cr, backbuf.x, backbuf.y, backbuf.w, backbuf.h);
      cairo_clip (cr);
      cairo_set_source_surface (cr, backbuf.image, 0.0, 0.0);
      cairo_paint (cr);

      cairo_destroy (cr);

      cairo_surface_destroy (backbuf.image);
      backbuf.image = NULL;
    }

  pw = PREVIEW_WIDTH * mapvals.zoom;
  ph = PREVIEW_HEIGHT * mapvals.zoom;

  if (pw != PREVIEW_WIDTH || ph != PREVIEW_HEIGHT)
    {
      startx = (PREVIEW_WIDTH - pw) / 2;
      starty = (PREVIEW_HEIGHT - ph) / 2;

      gdk_window_clear_area (gtk_widget_get_window (previewarea),
                             0, 0, startx, PREVIEW_HEIGHT);
      gdk_window_clear_area (gtk_widget_get_window (previewarea),
                             startx, 0, pw, starty);
      gdk_window_clear_area (gtk_widget_get_window (previewarea),
                             pw + startx, 0, startx, PREVIEW_HEIGHT);
      gdk_window_clear_area (gtk_widget_get_window (previewarea),
                             startx, ph + starty, pw, starty);
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
  cairo_t *cr;

  cr = gdk_cairo_create (gtk_widget_get_window (previewarea));

  color.red   = 0xFFFF;
  color.green = 0xFFFF;
  color.blue  = 0xFFFF;
  gdk_cairo_set_source_color (cr, &color);

  pw = PREVIEW_WIDTH * mapvals.zoom;
  ph = PREVIEW_HEIGHT * mapvals.zoom;
  startx = (PREVIEW_WIDTH - pw) / 2;
  starty = (PREVIEW_HEIGHT - ph) / 2;

  if (docompute == TRUE)
    {
      GdkDisplay *display = gtk_widget_get_display (previewarea);
      GdkCursor  *cursor;

      cursor = gdk_cursor_new_for_display (display, GDK_WATCH);
      gdk_window_set_cursor (gtk_widget_get_window (previewarea), cursor);
      gdk_cursor_unref (cursor);

      compute_preview (0, 0, width - 1, height - 1, pw, ph);

      cursor = gdk_cursor_new_for_display (display, GDK_HAND2);
      gdk_window_set_cursor(gtk_widget_get_window (previewarea), cursor);
      gdk_cursor_unref (cursor);

      clear_light_marker ();
    }

  if (pw != PREVIEW_WIDTH || ph != PREVIEW_HEIGHT)
    gdk_window_clear (gtk_widget_get_window (previewarea));

  cairo_set_source_surface (cr, preview_surface, startx, starty);
  cairo_rectangle (cr, startx, starty, pw, ph);
  cairo_clip (cr);

  cairo_paint (cr);

  draw_lights (startx, starty, pw, ph);

  cairo_destroy (cr);
}
