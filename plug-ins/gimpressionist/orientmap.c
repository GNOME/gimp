/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpressionist.h"
#include "ppmtool.h"
#include "infile.h"

#include "preview.h"

#include "orientmap.h"

#include "libgimp/stdplugins-intl.h"

#define NUMVECTYPES 4

static GtkWidget     *orient_map_window;

static GtkWidget     *vector_preview;
static GtkWidget     *orient_map_preview_prev;
static GtkWidget     *prev_button;
static GtkWidget     *next_button;
static GtkWidget     *add_button;
static GtkWidget     *kill_button;
static GtkAdjustment *vector_preview_brightness_adjust = NULL;

static GtkAdjustment *angle_adjust              = NULL;
static GtkAdjustment *strength_adjust           = NULL;
static GtkAdjustment *orient_map_str_exp_adjust = NULL;
static GtkAdjustment *angle_offset_adjust       = NULL;
static GtkWidget     *vector_types[NUMVECTYPES];
static GtkWidget     *orient_voronoi            = NULL;

#define OMWIDTH 150
#define OMHEIGHT 150

static vector_t vector[MAXORIENTVECT];
static gint     num_vectors = 0;
static gint     vector_type;

static ppm_t    update_om_preview_nbuffer = {0, 0, NULL};

static gint     selectedvector = 0;
static ppm_t    update_vector_preview_backup = {0, 0, NULL};
static ppm_t    update_vector_preview_buffer = {0, 0, NULL};

static gboolean adjignore = FALSE;

double get_direction (double x, double y, int from)
{
  gint      i;
  gint      n;
  gint      voronoi;
  gdouble   sum, dx, dy, dst;
  vector_t *vec;
  gdouble   angoff, strexp;
  gint      first = 0, last;

  if (from == 0)
    {
      n = num_vectors;
      vec = vector;
      angoff = gtk_adjustment_get_value (angle_offset_adjust);
      strexp = gtk_adjustment_get_value (orient_map_str_exp_adjust);
      voronoi = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (orient_voronoi));
    }
  else
    {
      n = pcvals.num_orient_vectors;
      vec = pcvals.orient_vectors;
      angoff = pcvals.orient_angle_offset;
      strexp = pcvals.orient_strength_exponent;
      voronoi = pcvals.orient_voronoi;
  }

  if (voronoi)
    {
      gdouble bestdist = -1.0;

      for (i = 0; i < n; i++)
        {
          dst = dist (x,y,vec[i].x,vec[i].y);

          if ((bestdist < 0.0) || (dst < bestdist))
            {
              bestdist = dst;
              first = i;
            }
        }
        last = first+1;
    }
  else
    {
      first = 0;
      last = n;
    }

  dx = dy = 0.0;
  sum = 0.0;
  for (i = first; i < last; i++)
    {
      gdouble s = vec[i].str;
      gdouble tx = 0.0, ty = 0.0;

      if (vec[i].type == 0)
        {
          tx = vec[i].dx;
          ty = vec[i].dy;
        }
      else if (vec[i].type == 1)
        {
          gdouble a = atan2 (vec[i].dy, vec[i].dx);

          a -= atan2 (y-vec[i].y, x-vec[i].x);
          tx = sin (a + G_PI_2);
          ty = cos (a + G_PI_2);
        }
      else if (vec[i].type == 2)
        {
          gdouble a = atan2 (vec[i].dy, vec[i].dx);

          a += atan2 (y-vec[i].y, x-vec[i].x);
          tx = sin (a + G_PI_2);
          ty = cos (a + G_PI_2);
        }
      else if (vec[i].type == 3)
        {
          gdouble a = atan2 (vec[i].dy, vec[i].dx);

          a -= atan2 (y-vec[i].y, x-vec[i].x)*2;
          tx = sin (a + G_PI_2);
          ty = cos (a + G_PI_2);
        }

      dst = dist (x,y,vec[i].x,vec[i].y);
      dst = pow (dst, strexp);

      if (dst < 0.0001)
        dst = 0.0001;
      s = s / dst;

      dx += tx * s;
      dy += ty * s;
      sum += s;
    }
  dx = dx / sum;
  dy = dy / sum;

  return 90 - (gimp_rad_to_deg (atan2 (dy, dx)) + angoff);
}

static void
update_orient_map_preview_prev (void)
{
  int    x, y;
  guchar black[3] = {0, 0, 0};
  guchar gray[3] = {120, 120, 120};
  guchar white[3] = {255, 255, 255};

  if (!PPM_IS_INITED (&update_om_preview_nbuffer))
    ppm_new (&update_om_preview_nbuffer,OMWIDTH,OMHEIGHT);

  fill (&update_om_preview_nbuffer, black);

  for (y = 6; y < OMHEIGHT-4; y += 10)
    for (x = 6; x < OMWIDTH-4; x += 10)
      {
        double dir =
          gimp_deg_to_rad (get_direction (x / (double)OMWIDTH,
                                          y / (double)OMHEIGHT,0));
        double xo = sin (dir) * 4.0;
        double yo = cos (dir) * 4.0;
        ppm_drawline (&update_om_preview_nbuffer,
                      x - xo, y - yo, x + xo, y + yo,
                      gray);
        ppm_put_rgb (&update_om_preview_nbuffer,
                     x - xo, y - yo,
                     white);
      }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (orient_map_preview_prev),
                          0, 0, OMWIDTH, OMHEIGHT,
                          GIMP_RGB_IMAGE,
                          (guchar *)update_om_preview_nbuffer.col,
                          OMWIDTH * 3);

  gtk_widget_queue_draw (orient_map_preview_prev);

  gtk_widget_set_sensitive (prev_button, (num_vectors > 1));
  gtk_widget_set_sensitive (next_button, (num_vectors > 1));
  gtk_widget_set_sensitive (add_button, (num_vectors < MAXORIENTVECT));
  gtk_widget_set_sensitive (kill_button, (num_vectors > 1));
}

static void
update_vector_prev (void)
{
  static gint    ok = 0;
  gint           i, x, y;
  gdouble        dir, xo, yo;
  gdouble        val;
  static gdouble last_val = 0.0;
  guchar         gray[3]  = {120, 120, 120};
  guchar         red[3]   = {255, 0, 0};
  guchar         white[3] = {255, 255, 255};

  if (vector_preview_brightness_adjust)
    val = 1.0 - gtk_adjustment_get_value (vector_preview_brightness_adjust) / 100.0;
  else
    val = 0.5;

  if (!ok || (val != last_val))
    {
      infile_copy_to_ppm (&update_vector_preview_backup);
      ppm_apply_brightness (&update_vector_preview_backup, val, 1,1,1);

      if ((update_vector_preview_backup.width != OMWIDTH) ||
          (update_vector_preview_backup.height != OMHEIGHT))
        resize_fast (&update_vector_preview_backup, OMWIDTH, OMHEIGHT);
      ok = 1;
    }
  ppm_copy (&update_vector_preview_backup, &update_vector_preview_buffer);

  for (i = 0; i < num_vectors; i++)
    {
      gdouble s;

      x = vector[i].x * OMWIDTH;
      y = vector[i].y * OMHEIGHT;
      dir = gimp_deg_to_rad (vector[i].dir);
      s = gimp_deg_to_rad (vector[i].str);
      xo = sin (dir) * (6.0+100*s);
      yo = cos (dir) * (6.0+100*s);

      if (i == selectedvector)
        {
          ppm_drawline (&update_vector_preview_buffer,
                        x - xo, y - yo, x + xo, y + yo, red);
        }
      else
        {
          ppm_drawline (&update_vector_preview_buffer,
                        x - xo, y - yo, x + xo, y + yo, gray);
        }
      ppm_put_rgb (&update_vector_preview_buffer, x - xo, y - yo, white);
  }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (vector_preview),
                          0, 0, OMWIDTH, OMHEIGHT,
                          GIMP_RGB_IMAGE,
                          (guchar *)update_vector_preview_buffer.col,
                          OMWIDTH * 3);
}

void
orientation_map_free_resources (void)
{
  ppm_kill (&update_om_preview_nbuffer);
  ppm_kill (&update_vector_preview_backup);
  ppm_kill (&update_vector_preview_buffer);
}

static void
update_slides (void)
{
  gint type;

  adjignore = TRUE;
  gtk_adjustment_set_value (angle_adjust, vector[selectedvector].dir);
  gtk_adjustment_set_value (strength_adjust, vector[selectedvector].str);
  type = vector[selectedvector].type;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (vector_types[type]), TRUE);
  adjignore = FALSE;
}

static void
prev_click_callback (GtkWidget *w, gpointer data)
{
  selectedvector--;
  if (selectedvector < 0)
    selectedvector = num_vectors-1;
  update_slides ();
  update_vector_prev ();
}

static void
next_click_callback (GtkWidget *w, gpointer data)
{
  selectedvector++;
  if (selectedvector == num_vectors) selectedvector = 0;
  update_slides ();
  update_vector_prev ();
}

static void
add_new_vector (gdouble x, gdouble y)
{
  vector[num_vectors].x = x;
  vector[num_vectors].y = y;
  vector[num_vectors].dir = 0.0;
  vector[num_vectors].dx = sin (gimp_deg_to_rad (0.0));
  vector[num_vectors].dy = cos (gimp_deg_to_rad (0.0));
  vector[num_vectors].str = 1.0;
  vector[num_vectors].type = 0;
  selectedvector = num_vectors;
  num_vectors++;
}

static void
add_click_callback (GtkWidget *w, gpointer data)
{
  add_new_vector (0.5, 0.5);
  update_slides ();
  update_vector_prev ();
  update_orient_map_preview_prev ();
}

static void
delete_click_callback (GtkWidget *w, gpointer data)
{
  int i;

  for (i = selectedvector; i < num_vectors-1; i++)
    vector[i] = vector[i + 1];

  num_vectors--;

  if (selectedvector >= num_vectors) selectedvector = 0;
  update_slides ();
  update_vector_prev ();
  update_orient_map_preview_prev ();
}

static void
map_click_callback (GtkWidget *w, GdkEventButton *event)
{
  if (event->button == 1)
    {
      vector[selectedvector].x = event->x / (double)OMWIDTH;
      vector[selectedvector].y = event->y / (double)OMHEIGHT;
    }
  else if (event->button == 2)
    {
      if (num_vectors + 1 == MAXORIENTVECT)
        return;
      add_new_vector (event->x / (double)OMWIDTH,
                      event->y / (double)OMHEIGHT);
      update_slides ();

    }
  else if (event->button == 3)
    {
      gdouble d;

      d = atan2 (OMWIDTH * vector[selectedvector].x - event->x,
                OMHEIGHT * vector[selectedvector].y - event->y);
      vector[selectedvector].dir = gimp_rad_to_deg (d);
      vector[selectedvector].dx = sin (d);
      vector[selectedvector].dy = cos (d);
      update_slides ();
    }
    update_vector_prev ();
    update_orient_map_preview_prev ();
}

static void
angle_adjust_move_callback (GtkWidget *w, gpointer data)
{
  if (adjignore)
    return;
  vector[selectedvector].dir = gtk_adjustment_get_value (angle_adjust);
  vector[selectedvector].dx =
    sin (gimp_deg_to_rad (vector[selectedvector].dir));
  vector[selectedvector].dy =
    cos (gimp_deg_to_rad (vector[selectedvector].dir));
  update_vector_prev ();
  update_orient_map_preview_prev ();
}

static void
strength_adjust_move_callback (GtkWidget *w, gpointer data)
{
  if (adjignore)
    return;
  vector[selectedvector].str = gtk_adjustment_get_value (strength_adjust);
  update_vector_prev ();
  update_orient_map_preview_prev ();
}

static void
strength_exponent_adjust_move_callback (GtkWidget *w, gpointer data)
{
  if (adjignore)
    return;
  update_vector_prev ();
  update_orient_map_preview_prev ();
}

static void
angle_offset_adjust_move_callback (GtkWidget *w, gpointer data)
{
  if (adjignore)
    return;
  update_vector_prev ();
  update_orient_map_preview_prev ();
}

static void
vector_type_click_callback (GtkWidget *w, gpointer data)
{
  if (adjignore)
    return;

  gimp_radio_button_update (w, data);
  vector[selectedvector].type = vector_type;
  update_vector_prev ();
  update_orient_map_preview_prev ();
}

static void
orient_map_response (GtkWidget *widget,
                     gint       response_id,
                     gpointer   data)
{
  switch (response_id)
    {
    case GTK_RESPONSE_APPLY:
    case GTK_RESPONSE_OK:
      {
        gint i;

        for (i = 0; i < num_vectors; i++)
          pcvals.orient_vectors[i] = vector[i];

        pcvals.num_orient_vectors = num_vectors;
        pcvals.orient_strength_exponent  = gtk_adjustment_get_value (orient_map_str_exp_adjust);
        pcvals.orient_angle_offset  = gtk_adjustment_get_value (angle_offset_adjust);
        pcvals.orient_voronoi = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (orient_voronoi));
      }
    };

  if (response_id != GTK_RESPONSE_APPLY)
    gtk_widget_hide (widget);
}

static void
init_vectors (void)
{
  if (pcvals.num_orient_vectors)
    {
      gint i;

      num_vectors = pcvals.num_orient_vectors;
      for (i = 0; i < num_vectors; i++)
        vector[i] = pcvals.orient_vectors[i];
    }
  else
    {/* Shouldn't happen */
      num_vectors = 0;
      add_new_vector (0.5, 0.5);
    }
  if (selectedvector >= num_vectors)
    selectedvector = num_vectors-1;
}

void
update_orientmap_dialog (void)
{
  if (!orient_map_window) return;

  init_vectors ();

  gtk_adjustment_set_value (orient_map_str_exp_adjust,
                            pcvals.orient_strength_exponent);
  gtk_adjustment_set_value (angle_offset_adjust,
                            pcvals.orient_angle_offset);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (orient_voronoi),
                                pcvals.orient_voronoi);

  update_vector_prev ();
  update_orient_map_preview_prev ();
}

void
create_orientmap_dialog (GtkWidget *parent)
{
  GtkWidget *tmpw, *tmpw2;
  GtkWidget *grid1, *grid2;
  GtkWidget *frame;
  GtkWidget *ebox, *hbox, *vbox;

  init_vectors ();

  if (orient_map_window)
    {
      update_vector_prev ();
      update_orient_map_preview_prev ();
      gtk_widget_show (orient_map_window);
      return;
    }

  orient_map_window =
    gimp_dialog_new (_("Orientation Map Editor"), PLUG_IN_ROLE,
                     gtk_widget_get_toplevel (parent), 0,
                     gimp_standard_help_func, PLUG_IN_PROC,

                     _("_Apply"),  GTK_RESPONSE_APPLY,
                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                     _("_OK"),     GTK_RESPONSE_OK,

                     NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (orient_map_window),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_APPLY,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (orient_map_window, "response",
                    G_CALLBACK (orient_map_response),
                    orient_map_window);
  g_signal_connect (orient_map_window, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &orient_map_window);

  grid1 = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid1), 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (orient_map_window))),
                      grid1, TRUE, TRUE, 0);
  gtk_widget_show (grid1);

  frame = gtk_frame_new (_("Vectors"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
  gtk_grid_attach (GTK_GRID (grid1), frame, 0, 0, 1, 1);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  ebox = gtk_event_box_new ();
  gimp_help_set_help_data (ebox,
                           _("The vector-field. "
                             "Left-click to move selected vector, "
                             "Right-click to point it towards mouse, "
                             "Middle-click to add a new vector."), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), ebox, FALSE, FALSE, 0);

  tmpw = vector_preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (tmpw, OMWIDTH, OMHEIGHT);
  gtk_container_add (GTK_CONTAINER (ebox), tmpw);
  gtk_widget_show (tmpw);
  gtk_widget_add_events (ebox, GDK_BUTTON_PRESS_MASK);
  g_signal_connect (ebox, "button-press-event",
                   G_CALLBACK (map_click_callback), NULL);
  gtk_widget_show (ebox);

  vector_preview_brightness_adjust = (GtkAdjustment *)
    gtk_adjustment_new (50.0, 0.0, 100.0, 1.0, 1.0, 1.0);
  tmpw = gtk_scale_new (GTK_ORIENTATION_VERTICAL,
                        vector_preview_brightness_adjust);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), tmpw, FALSE, FALSE,0);
  gtk_widget_show (tmpw);
  g_signal_connect (vector_preview_brightness_adjust, "value-changed",
                    G_CALLBACK (update_vector_prev), NULL);
  gimp_help_set_help_data (tmpw, _("Adjust the preview's brightness"), NULL);

  tmpw2 = tmpw = gtk_frame_new (_("Preview"));
  gtk_container_set_border_width (GTK_CONTAINER (tmpw), 2);
  gtk_grid_attach (GTK_GRID (grid1), tmpw, 1, 0, 1, 1);
  gtk_widget_show (tmpw);

  tmpw = orient_map_preview_prev = gimp_preview_area_new ();
  gtk_widget_set_size_request (tmpw, OMWIDTH, OMHEIGHT);
  gtk_container_add (GTK_CONTAINER (tmpw2), tmpw);
  gtk_widget_show (tmpw);

  hbox = tmpw = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (tmpw), 2);
  gtk_grid_attach (GTK_GRID (grid1), tmpw, 0, 1, 1, 1);
  gtk_widget_show (tmpw);

  prev_button = tmpw = gtk_button_new_with_mnemonic ("_<<");
  gtk_box_pack_start (GTK_BOX (hbox), tmpw, FALSE, TRUE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked", G_CALLBACK (prev_click_callback), NULL);
  gimp_help_set_help_data (tmpw, _("Select previous vector"), NULL);

  next_button = tmpw = gtk_button_new_with_mnemonic ("_>>");
  gtk_box_pack_start (GTK_BOX (hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked", G_CALLBACK (next_click_callback), NULL);
  gimp_help_set_help_data (tmpw, _("Select next vector"), NULL);

  add_button = tmpw = gtk_button_new_with_mnemonic ( _("A_dd"));
  gtk_box_pack_start (GTK_BOX (hbox), tmpw, FALSE, TRUE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked", G_CALLBACK (add_click_callback), NULL);
  gimp_help_set_help_data (tmpw, _("Add new vector"), NULL);

  kill_button = tmpw = gtk_button_new_with_mnemonic ( _("_Kill"));
  gtk_box_pack_start (GTK_BOX (hbox), tmpw, FALSE, TRUE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked", G_CALLBACK (delete_click_callback), NULL);
  gimp_help_set_help_data (tmpw, _("Delete selected vector"), NULL);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_spacing (GTK_BOX (hbox), 12);
  gtk_grid_attach (GTK_GRID (grid1), hbox, 0, 2, 2, 1);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame = gimp_int_radio_group_new (TRUE, _("Type"),
                                    G_CALLBACK (vector_type_click_callback),
                                    &vector_type, 0,

                                    _("_Normal"),  0, &vector_types[0],
                                    _("Vorte_x"),  1, &vector_types[1],
                                    _("Vortex_2"), 2, &vector_types[2],
                                    _("Vortex_3"), 3, &vector_types[3],

                                    NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  orient_voronoi = tmpw = gtk_check_button_new_with_mnemonic ( _("_Voronoi"));
  gtk_box_pack_start (GTK_BOX (vbox), tmpw, TRUE, TRUE, 0);
  gtk_widget_show (tmpw);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tmpw),
                                pcvals.orient_voronoi);
  g_signal_connect (tmpw, "clicked",
                    G_CALLBACK (angle_offset_adjust_move_callback), NULL);
  gimp_help_set_help_data (tmpw,
                          _("Voronoi-mode makes only the vector closest to the given point have any influence"),
                          NULL);

  grid2 = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid2), 4);
  gtk_box_pack_start (GTK_BOX (hbox), grid2, TRUE, TRUE, 0);
  gtk_widget_show (grid2);

  angle_adjust = (GtkAdjustment *)
    gimp_scale_entry_new_grid (GTK_GRID (grid2), 0, 0,
                               _("A_ngle:"),
                               150, 6, 0.0,
                               0.0, 360.0, 1.0, 10.0, 1,
                               TRUE, 0, 0,
                               _("Change the angle of the selected vector"),
                               NULL);
  g_signal_connect (angle_adjust, "value-changed",
                    G_CALLBACK (angle_adjust_move_callback), NULL);

  angle_offset_adjust = (GtkAdjustment *)
    gimp_scale_entry_new_grid (GTK_GRID (grid2), 0, 1,
                               _("Ang_le offset:"),
                               150, 6, 0.0,
                               0.0, 360.0, 1.0, 10.0, 1,
                               TRUE, 0, 0,
                               _("Offset all vectors with a given angle"),
                               NULL);
  g_signal_connect (angle_offset_adjust, "value-changed",
                    G_CALLBACK (angle_offset_adjust_move_callback), NULL);

  strength_adjust = (GtkAdjustment *)
    gimp_scale_entry_new_grid (GTK_GRID (grid2), 0, 2,
                               _("_Strength:"),
                               150, 6, 1.0,
                               0.1, 5.0, 0.1, 1.0, 1,
                               TRUE, 0, 0,
                               _("Change the strength of the selected vector"),
                               NULL);
  g_signal_connect (strength_adjust, "value-changed",
                    G_CALLBACK (strength_adjust_move_callback), NULL);

  orient_map_str_exp_adjust = (GtkAdjustment *)
    gimp_scale_entry_new_grid (GTK_GRID (grid2), 0, 3,
                               _("S_trength exp.:"),
                               150, 6, 1.0,
                               0.1, 10.9, 0.1, 1.0, 1,
                               TRUE, 0, 0,
                               _("Change the exponent of the strength"),
                               NULL);
  g_signal_connect (orient_map_str_exp_adjust, "value-changed",
                    G_CALLBACK (strength_exponent_adjust_move_callback), NULL);

  gtk_widget_show (orient_map_window);

  update_vector_prev ();
  update_orient_map_preview_prev ();
}
