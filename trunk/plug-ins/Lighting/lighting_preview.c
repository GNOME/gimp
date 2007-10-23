/*************************************************/
/* Compute a preview image and preview wireframe */
/*************************************************/

#include "config.h"

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimpmath/gimpmath.h>

#include "lighting_main.h"
#include "lighting_ui.h"
#include "lighting_image.h"
#include "lighting_apply.h"
#include "lighting_shade.h"

#include "lighting_preview.h"

#define LIGHT_SYMBOL_SIZE 8

gint handle_xpos = 0, handle_ypos = 0;

BackBuffer backbuf = { 0, 0, 0, 0, NULL };

/* g_free()'ed on exit */
gdouble *xpostab = NULL;
gdouble *ypostab = NULL;

static gint xpostab_size = -1;  /* if preview size change, do realloc */
static gint ypostab_size = -1;

gboolean    light_hit           = FALSE;
gboolean    left_button_pressed = FALSE;
static guint preview_update_timer = 0;


/* Protos */
/* ====== */
static gboolean
interactive_preview_timer_callback ( gpointer data );

static void
compute_preview (gint startx, gint starty, gint w, gint h)
{
  gint xcnt, ycnt, f1, f2;
  gdouble imagex, imagey;
  gint32 index = 0;
  GimpRGB color;
  GimpRGB lightcheck, darkcheck;
  GimpVector3 pos;
  get_ray_func ray_func;

  if (xpostab_size != w)
    {
      if (xpostab)
        {
          g_free (xpostab);
          xpostab = NULL;
        }
    }

  if (!xpostab)
    {
      xpostab = g_new (gdouble, w);
      xpostab_size = w;
    }

  if (ypostab_size != h)
    {
      if (ypostab)
        {
          g_free (ypostab);
          ypostab = NULL;
        }
    }

  if (!ypostab)
    {
      ypostab = g_new (gdouble, h);
      ypostab_size = h;
    }

  for (xcnt = 0; xcnt < w; xcnt++)
    xpostab[xcnt] = (gdouble) width *((gdouble) xcnt / (gdouble) w);
  for (ycnt = 0; ycnt < h; ycnt++)
    ypostab[ycnt] = (gdouble) height *((gdouble) ycnt / (gdouble) h);

  precompute_init (width, height);

  gimp_rgba_set (&lightcheck,
                 GIMP_CHECK_LIGHT, GIMP_CHECK_LIGHT, GIMP_CHECK_LIGHT,
                 1.0);
  gimp_rgba_set (&darkcheck, GIMP_CHECK_DARK, GIMP_CHECK_DARK,
                 GIMP_CHECK_DARK, 1.0);

  if (mapvals.bump_mapped == TRUE && mapvals.bumpmap_id != -1)
    {
      gimp_pixel_rgn_init (&bump_region,
                           gimp_drawable_get (mapvals.bumpmap_id),
                           0, 0, width, height, FALSE, FALSE);
    }

  imagey = 0;

  if (mapvals.previewquality)
    ray_func = get_ray_color;
  else
    ray_func = get_ray_color_no_bilinear;

  if (mapvals.env_mapped == TRUE && mapvals.envmap_id != -1)
    {
      env_width = gimp_drawable_width (mapvals.envmap_id);
      env_height = gimp_drawable_height (mapvals.envmap_id);

      gimp_pixel_rgn_init (&env_region,
                           gimp_drawable_get (mapvals.envmap_id), 0,
                           0, env_width, env_height, FALSE, FALSE);

      if (mapvals.previewquality)
        ray_func = get_ray_color_ref;
      else
        ray_func = get_ray_color_no_bilinear_ref;
    }

  for (ycnt = 0; ycnt < PREVIEW_HEIGHT; ycnt++)
    {
      for (xcnt = 0; xcnt < PREVIEW_WIDTH; xcnt++)
        {
          if ((ycnt >= starty && ycnt < (starty + h)) &&
              (xcnt >= startx && xcnt < (startx + w)))
            {
              imagex = xpostab[xcnt - startx];
              imagey = ypostab[ycnt - starty];
              pos = int_to_posf (imagex, imagey);

              if (mapvals.bump_mapped == TRUE &&
                  mapvals.bumpmap_id != -1 &&
                  xcnt == startx)
                {
                  pos_to_float (pos.x, pos.y, &imagex, &imagey);
                  precompute_normals (0, width, RINT (imagey));
                }

              color = (*ray_func) (&pos);

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
                        gimp_rgb_composite (&color,
                                            &lightcheck,
                                            GIMP_RGB_COMPOSITE_BEHIND);
                    }
                  else
                    {
                      if (color.a == 0.0)
                        color = darkcheck;
                      else
                        gimp_rgb_composite (&color,
                                            &darkcheck,
                                            GIMP_RGB_COMPOSITE_BEHIND);
                    }
                }

              gimp_rgb_get_uchar (&color,
                                  preview_rgb_data + index,
                                  preview_rgb_data + index +
                                  1,
                                  preview_rgb_data + index +
                                  2);
              index += 3;
              imagex++;
            }
          else
            {
              preview_rgb_data[index++] = 200;
              preview_rgb_data[index++] = 200;
              preview_rgb_data[index++] = 200;
            }
        }
    }
}

static void
compute_preview_rectangle (gint * xp, gint * yp, gint * wid, gint * heig)
{
  gdouble x, y, w, h;

  if (width >= height)
    {
      w = (PREVIEW_WIDTH - 50.0);
      h = (gdouble) height *(w / (gdouble) width);

      x = (PREVIEW_WIDTH - w) / 2.0;
      y = (PREVIEW_HEIGHT - h) / 2.0;
    }
  else
    {
      h = (PREVIEW_HEIGHT - 50.0);
      w = (gdouble) width *(h / (gdouble) height);
      x = (PREVIEW_WIDTH - w) / 2.0;
      y = (PREVIEW_HEIGHT - h) / 2.0;
    }
  *xp = RINT (x);
  *yp = RINT (y);
  *wid = RINT (w);
  *heig = RINT (h);
}

/*************************************************/
/* Check if the given position is within the     */
/* light marker. Return TRUE if so, FALSE if not */
/*************************************************/

static gboolean
check_handle_hit (gint xpos, gint ypos)
{
  gint dx,dy,r;
  gint k = mapvals.light_selected;

  dx = handle_xpos - xpos;
  dy = handle_ypos - ypos;


  if (mapvals.lightsource[k].type == POINT_LIGHT ||
      mapvals.lightsource[k].type == DIRECTIONAL_LIGHT)
    {
      r = sqrt (dx * dx + dy * dy) + 0.5;
      if ((gint) r > 7)
        {
          return (FALSE);
        }
      else
        {
          return (TRUE);
        }
    }
  return FALSE;
}

/****************************************/
/* Draw a light symbol                  */
/****************************************/


static void
draw_handles (void)
{
  gdouble     dxpos, dypos;
  gint        startx, starty, pw, ph;
  GimpVector3 viewpoint;
  GimpVector3 light_position;
  gint        k      = mapvals.light_selected;

  gfloat length;
  gfloat delta_x = 0.0;
  gfloat delta_y = 0.0;

  /* calculate handle position */
  compute_preview_rectangle (&startx, &starty, &pw, &ph);
  switch (mapvals.lightsource[k].type)
    {
    case NO_LIGHT:
      return;
    case POINT_LIGHT:
    case SPOT_LIGHT:
      /* swap z to reverse light position */
      viewpoint = mapvals.viewpoint;
      viewpoint.z = -viewpoint.z;
      light_position = mapvals.lightsource[k].position;
      gimp_vector_3d_to_2d (startx, starty, pw, ph, &dxpos, &dypos,
                            &viewpoint, &light_position);
      handle_xpos = (gint) (dxpos + 0.5);
      handle_ypos = (gint) (dypos + 0.5);
      break;
    case DIRECTIONAL_LIGHT:
      light_position.x = light_position.y = 0.5;
      light_position.z = 0;
      viewpoint.z = -viewpoint.z;
      gimp_vector_3d_to_2d (startx, starty, pw, ph, &dxpos, &dypos,
                            &viewpoint, &light_position);
      length = PREVIEW_HEIGHT / 4;
      delta_x = mapvals.lightsource[k].direction.x * length;
      delta_y = mapvals.lightsource[k].direction.y * length;
      handle_xpos = dxpos + delta_x;
      handle_ypos = dypos + delta_y;
      break;
    }

  gdk_gc_set_function (gc, GDK_COPY);

  if (mapvals.lightsource[k].type != NO_LIGHT)
    {
      GdkColor  color;

      /* Restore background if it has been saved */
      /* ======================================= */

      if (backbuf.image != NULL)
        {
          gdk_gc_set_function (gc, GDK_COPY);
          gdk_draw_image (previewarea->window, gc,
                          backbuf.image, 0, 0, backbuf.x,
                          backbuf.y, backbuf.w, backbuf.h);
          g_object_unref (backbuf.image);
          backbuf.image = NULL;
        }

      /* calculate backbuffer */
      switch (mapvals.lightsource[k].type)
        {
        case POINT_LIGHT:
          backbuf.x = handle_xpos - LIGHT_SYMBOL_SIZE / 2;
          backbuf.y = handle_ypos - LIGHT_SYMBOL_SIZE / 2;
          backbuf.w = LIGHT_SYMBOL_SIZE;
          backbuf.h = LIGHT_SYMBOL_SIZE;
          break;
        case DIRECTIONAL_LIGHT:
          if (delta_x <= 0)
            backbuf.x = handle_xpos;
          else
            backbuf.x = startx + pw/2;
          if (delta_y <= 0)
            backbuf.y = handle_ypos;
          else
            backbuf.y = starty + ph/2;
          backbuf.x -= LIGHT_SYMBOL_SIZE/2;
          backbuf.y -= LIGHT_SYMBOL_SIZE/2;
          backbuf.w = fabs(delta_x) + LIGHT_SYMBOL_SIZE;
          backbuf.h = fabs(delta_y) + LIGHT_SYMBOL_SIZE;
          break;
        case SPOT_LIGHT:
          backbuf.x = handle_xpos - LIGHT_SYMBOL_SIZE / 2;
          backbuf.y = handle_ypos - LIGHT_SYMBOL_SIZE / 2;
          backbuf.w = LIGHT_SYMBOL_SIZE;
          backbuf.h = LIGHT_SYMBOL_SIZE;
          break;
        case NO_LIGHT:
          break;
        }

      /* Save background */
      /* =============== */
      if ((backbuf.x >= 0) &&
          (backbuf.x <= PREVIEW_WIDTH) &&
          (backbuf.y >= 0) && (backbuf.y <= PREVIEW_HEIGHT))
        {
          /* clip coordinates to preview widget sizes */
          if ((backbuf.x + backbuf.w) > PREVIEW_WIDTH)
            backbuf.w = (PREVIEW_WIDTH - backbuf.x);

          if ((backbuf.y + backbuf.h) > PREVIEW_HEIGHT)
            backbuf.h = (PREVIEW_HEIGHT - backbuf.y);

          backbuf.image = gdk_drawable_get_image (previewarea->window,
                                                  backbuf.x, backbuf.y,
                                                  backbuf.w, backbuf.h);
        }

      color.red   = 0x0;
      color.green = 0x0;
      color.blue  = 0x0;
      gdk_gc_set_rgb_bg_color (gc, &color);

      color.red   = 0x0;
      color.green = 0x4000;
      color.blue  = 0xFFFF;
      gdk_gc_set_rgb_fg_color (gc, &color);

      /* draw circle at light position */
      switch (mapvals.lightsource[k].type)
        {
        case POINT_LIGHT:
        case SPOT_LIGHT:
          gdk_draw_arc (previewarea->window, gc, TRUE,
                        handle_xpos - LIGHT_SYMBOL_SIZE / 2,
                        handle_ypos - LIGHT_SYMBOL_SIZE / 2,
                        LIGHT_SYMBOL_SIZE,
                        LIGHT_SYMBOL_SIZE, 0, 360 * 64);
          break;
        case DIRECTIONAL_LIGHT:
          gdk_draw_arc (previewarea->window, gc, TRUE,
                        handle_xpos - LIGHT_SYMBOL_SIZE / 2,
                        handle_ypos - LIGHT_SYMBOL_SIZE / 2,
                        LIGHT_SYMBOL_SIZE,
                        LIGHT_SYMBOL_SIZE, 0, 360 * 64);
          gdk_draw_line (previewarea->window, gc,
                         handle_xpos, handle_ypos, startx+pw/2 , starty + ph/2);
          break;
        case NO_LIGHT:
          break;
        }
    }
}


/*************************************************/
/* Update light position given new screen coords */
/*************************************************/

void
update_light (gint xpos, gint ypos)
{
  gint startx, starty, pw, ph;
  GimpVector3  vp;
  gint         k = mapvals.light_selected;

  compute_preview_rectangle (&startx, &starty, &pw, &ph);

  vp = mapvals.viewpoint;
  vp.z = -vp.z;

  switch (mapvals.lightsource[k].type)
    {
    case        NO_LIGHT:
      break;
    case        POINT_LIGHT:
    case        SPOT_LIGHT:
      gimp_vector_2d_to_3d (startx,
                            starty,
                            pw,
                            ph,
                            xpos, ypos, &vp, &mapvals.lightsource[k].position);
      break;
    case DIRECTIONAL_LIGHT:
      gimp_vector_2d_to_3d (startx,
                            starty,
                            pw,
                            ph,
                            xpos, ypos, &vp, &mapvals.lightsource[k].direction);
      break;
    }
}


/******************************************************************/
/* Draw preview image. if DoCompute is TRUE then recompute image. */
/******************************************************************/

void
draw_preview_image (gboolean recompute)
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

  gdk_gc_set_function (gc, GDK_COPY);

  compute_preview_rectangle (&startx, &starty, &pw, &ph);

  if (recompute)
    {
      GdkDisplay *display = gtk_widget_get_display (previewarea);
      GdkCursor  *cursor;

      cursor = gdk_cursor_new_for_display (display, GDK_WATCH);

      gdk_window_set_cursor (previewarea->window, cursor);
      gdk_cursor_unref (cursor);

      compute_preview (startx, starty, pw, ph);
      cursor = gdk_cursor_new_for_display (display, GDK_HAND2);
      gdk_window_set_cursor (previewarea->window, cursor);
      gdk_cursor_unref (cursor);
      gdk_flush ();

      /* if we recompute, clear backbuf, so we don't
       * restore the wrong bitmap */
      if (backbuf.image != NULL)
        {
          g_object_unref (backbuf.image);
          backbuf.image = NULL;
        }
    }

  gdk_draw_rgb_image (previewarea->window, gc,
                      0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT,
                      GDK_RGB_DITHER_MAX, preview_rgb_data,
                      3 * PREVIEW_WIDTH);

  /* draw symbols if enabled in UI */
  if (mapvals.interactive_preview)
    {
      draw_handles ();
    }
}

/******************************/
/* Preview area event handler */
/******************************/

gboolean
preview_events (GtkWidget *area,
                GdkEvent  *event)
{
  switch (event->type)
    {
    case GDK_EXPOSE:

      /* Is this the first exposure? */
      /* =========================== */
      if (!gc)
        {
          gc = gdk_gc_new (area->window);
          draw_preview_image (TRUE);
        }
      else
        draw_preview_image (FALSE);
      break;
    case GDK_ENTER_NOTIFY:
      break;
    case GDK_LEAVE_NOTIFY:
      break;
    case GDK_BUTTON_PRESS:
      light_hit = check_handle_hit (event->button.x, event->button.y);
      left_button_pressed = TRUE;
      break;
    case GDK_BUTTON_RELEASE:
      left_button_pressed = FALSE;
      break;
    case GDK_MOTION_NOTIFY:
      if (left_button_pressed == TRUE &&
          light_hit == TRUE &&
          mapvals.interactive_preview == TRUE )
        {
          draw_handles();
          interactive_preview_callback(NULL);
          update_light (event->motion.x, event->motion.y);
        }
      break;
    default:
      break;
    }

  return FALSE;
}

void
interactive_preview_callback (GtkWidget *widget)
{
  if ( preview_update_timer != 0)
    {
      g_source_remove ( preview_update_timer );
    }
  /* start new timer */
  preview_update_timer = g_timeout_add (100,
                                        interactive_preview_timer_callback, NULL);
}

static gboolean
interactive_preview_timer_callback ( gpointer data )
{
  gint k = mapvals.light_selected;

  mapvals.update_enabled = FALSE;  /* disable apply_settings() */

  gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin_pos_x),
                             mapvals.lightsource[k].position.x);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin_pos_y),
                             mapvals.lightsource[k].position.y);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin_pos_z),
                             mapvals.lightsource[k].position.z);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin_dir_x),
                             mapvals.lightsource[k].direction.x);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin_dir_y),
                             mapvals.lightsource[k].direction.y);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(spin_dir_z),
                             mapvals.lightsource[k].direction.z);

  mapvals.update_enabled = TRUE;

  draw_preview_image (TRUE);

  preview_update_timer = 0;

  return FALSE;
}
