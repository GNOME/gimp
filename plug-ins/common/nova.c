/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * SuperNova plug-in
 * Copyright (C) 1997 Eiichi Takamori <taka@ma1.seikyou.ne.jp>,
 *                    Spencer Kimball, Federico Mena Quintero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * version 1.200
 *
 * This plug-in produces an effect like a supernova burst.
 *
 *      Eiichi Takamori <taka@ma1.seikyou.ne.jp>
 *      http://ha1.seikyou.ne.jp/home/taka/gimp/
 *
 *      Preview render mode by timecop@japan.co.jp
 *      http://www.ne.jp/asahi/linux/timecop
 *
 * Changes from version 1.122 to version 1.200 by tim copperfield:
 * - preview mode now previews the nova with scale;
 * - toggle for cursor show/hide during preview
 *
 * Changes from version 1.1115 to version 1.122 by Martin Weber:
 * - Little bug fixes
 * - Added random hue
 * - Freeing memory
 *
 * Changes from version 1.1114 to version 1.1115:
 * - Added gtk_rc_parse
 * - Fixed bug that drawing preview of small height image
 *   (maybe image height < PREVIEW_SIZE) caused core dump.
 * - Changed default value.
 * - Moved to <Image>/Filters/Effects.  right?
 * - etc...
 *
 * Changes from version 1.1112 to version 1.1114:
 * - Modified proc args to GIMP_PDB_COLOR, also included nspoke.
 * - nova_int_entryscale_new(): Fixed caption was guchar -> gchar, etc.
 * - Now nova renders properly with alpha channel.
 *   (Very thanks to Spencer Kimball and Federico Mena !)
 *
 * TODO:
 * - clean up the code more
 * - add notebook interface and so on
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "gimpoldpreview.h"

#ifdef RCSID
static char rcsid[] = "$Id$";
#endif

#define ENTRY_WIDTH      50
#define SCALE_WIDTH     125
#define TILE_CACHE_SIZE  16

#define PREVIEW   0x1
#define CURSOR    0x2
#define ALL       0xf

#define PREVIEW_MASK (GDK_EXPOSURE_MASK | \
                      GDK_BUTTON_PRESS_MASK | \
                      GDK_BUTTON1_MOTION_MASK)

static GimpOldPreview *preview;
static gboolean        show_cursor = FALSE;

typedef struct
{
  gint    xcenter;
  gint    ycenter;
  GimpRGB color;
  gint    radius;
  gint    nspoke;
  gint    randomhue;
} NovaValues;

typedef struct
{
  GimpDrawable *drawable;
  gint          dwidth;
  gint          dheight;
  gint          bpp;
  GtkObject    *xadj;
  GtkObject    *yadj;
  gint          cursor;
  gint          curx, cury;              /* x,y of cursor in preview */
  gint          oldx, oldy;
  gboolean      in_call;
} NovaCenter;


/* Declare a local function.
 */
static void        query (void);
static void        run   (const gchar      *name,
                          gint              nparams,
                          const GimpParam  *param,
                          gint             *nreturn_vals,
                          GimpParam       **return_vals);

static void        nova                    (GimpDrawable *drawable,
                                            gboolean      preview_mode);

static gint        nova_dialog             (GimpDrawable *drawable);

static GtkWidget * nova_center_create            (GimpDrawable  *drawable);
static void        nova_center_destroy           (GtkWidget     *widget,
                                                  gpointer       data);
static void        nova_center_draw              (NovaCenter    *center,
                                                  gint           update);
static void        nova_center_adjustment_update (GtkAdjustment *widget,
                                                  gpointer       data);
static void        nova_center_cursor_update     (NovaCenter    *center);
static gint        nova_center_preview_expose    (GtkWidget     *widget,
                                                  GdkEvent      *event,
                                                  gpointer       data);
static gint        nova_center_preview_events    (GtkWidget     *widget,
                                                  GdkEvent      *event,
                                                  gpointer       data);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static NovaValues pvals =
{
  128, 128,                 /* xcenter, ycenter */
  { 0.35, 0.39, 1.0, 1.0 }, /* color */
  20,                       /* radius */
  100,                      /* nspoke */
  0                         /* random hue */
};


MAIN ()

static void
query (void)
{
  static GimpParamDef args[]=
  {
    { GIMP_PDB_INT32,    "run_mode",  "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",     "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",  "Input drawable" },
    { GIMP_PDB_INT32,    "xcenter",   "X coordinates of the center of supernova" },
    { GIMP_PDB_INT32,    "ycenter",   "Y coordinates of the center of supernova" },
    { GIMP_PDB_COLOR,    "color",     "Color of supernova" },
    { GIMP_PDB_INT32,    "radius",    "Radius of supernova" },
    { GIMP_PDB_INT32,    "nspoke",    "Number of spokes" },
    { GIMP_PDB_INT32,    "randomhue", "Random hue" }
  };

  gimp_install_procedure ("plug_in_nova",
                          "Produce Supernova effect to the specified drawable",
                          "This plug-in produces an effect like a supernova "
                          "burst. The amount of the light effect is "
                          "approximately in proportion to 1/r, where r is the "
                          "distance from the center of the star. It works with "
                          "RGB*, GRAY* image.",
                          "Eiichi Takamori",
                          "Eiichi Takamori",
                          "May 2000",
                          /* don't translate '<Image>' */
                          N_("<Image>/Filters/Light Effects/Su_perNova..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (const gchar       *name,
     gint               nparams,
     const GimpParam   *param,
     gint              *nreturn_vals,
     GimpParam        **return_vals)
{
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_nova", &pvals);

      /*  First acquire information with a dialog  */
      if (! nova_dialog (drawable))
        {
          gimp_drawable_detach (drawable);
          return;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 9)
        status = GIMP_PDB_CALLING_ERROR;

      if (status == GIMP_PDB_SUCCESS)
        {
          pvals.xcenter   = param[3].data.d_int32;
          pvals.ycenter   = param[4].data.d_int32;
          pvals.color     = param[5].data.d_color;
          pvals.radius    = param[6].data.d_int32;
          pvals.nspoke    = param[7].data.d_int32;
          pvals.randomhue = param[8].data.d_int32;
        }

      if ((status == GIMP_PDB_SUCCESS) &&
          pvals.radius <= 0)
        status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_nova", &pvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id) ||
          gimp_drawable_is_gray (drawable->drawable_id))
        {
          gimp_progress_init (_("Rendering SuperNova..."));
          gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

          nova (drawable, 0);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data ("plug_in_nova", &pvals, sizeof (NovaValues));
        }
      else
        {
          /* gimp_message ("nova: cannot operate on indexed color images"); */
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

/*******************/
/*   Main Dialog   */
/*******************/

static gint
nova_dialog (GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *center_frame;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init ("nova", TRUE);

  dlg = gimp_dialog_new (_("SuperNova"), "nova",
                         NULL, 0,
                         gimp_standard_help_func, "filters/nova.html",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  /*  parameter settings  */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (5, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  center_frame = nova_center_create (drawable);
  gtk_table_attach (GTK_TABLE (table), center_frame, 0, 3, 0, 1,
                    0, 0, 0, 0);
  button = gimp_color_button_new (_("SuperNova Color Picker"),
                                  SCALE_WIDTH - 8, 16,
                                  &pvals.color, GIMP_COLOR_AREA_FLAT);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Co_lor:"), 1.0, 0.5,
                             button, 1, TRUE);

  g_signal_connect (button, "color_changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &pvals.color);
  g_signal_connect_swapped (button, "color_changed",
                            G_CALLBACK (nova),
                            drawable);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
                              _("_Radius:"), SCALE_WIDTH, 8,
                              pvals.radius, 1, 100, 1, 10, 0,
                              FALSE, 1, GIMP_MAX_IMAGE_SIZE,
                              NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &pvals.radius);
  g_signal_connect_swapped (adj, "value_changed",
                            G_CALLBACK (nova),
                            drawable);
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 4,
                              _("_Spokes:"), SCALE_WIDTH, 8,
                              pvals.nspoke, 1, 1024, 1, 16, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &pvals.nspoke);
  g_signal_connect_swapped (adj, "value_changed",
                            G_CALLBACK (nova),
                            drawable);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 5,
                              _("R_andom Hue:"), SCALE_WIDTH, 8,
                              pvals.randomhue, 0, 360, 1, 15, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &pvals.randomhue);
  g_signal_connect_swapped (adj, "value_changed",
                            G_CALLBACK (nova),
                            drawable);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}


/*=================================================================
    CenterFrame

    A frame that contains one preview and 2 entrys, used for positioning
    of the center of Nova.
==================================================================*/

/*
 * Create new CenterFrame, and return it (GtkFrame).
 */
static GtkWidget *
nova_center_create (GimpDrawable *drawable)
{
  NovaCenter *center;
  GtkWidget  *frame;
  GtkWidget  *table;
  GtkWidget  *label;
  GtkWidget  *pframe;
  GtkWidget  *spinbutton;
  GtkWidget  *check;

  center = g_new (NovaCenter, 1);
  center->drawable = drawable;
  center->dwidth   = gimp_drawable_width (drawable->drawable_id);
  center->dheight  = gimp_drawable_height (drawable->drawable_id);
  center->bpp      = gimp_drawable_bpp (drawable->drawable_id);

  if (gimp_drawable_has_alpha (drawable->drawable_id))
    center->bpp--;

  center->cursor   = FALSE;
  center->curx     = 0;
  center->cury     = 0;
  center->oldx     = 0;
  center->oldy     = 0;
  center->in_call  = TRUE;  /* to avoid side effects while initialization */

  frame = gtk_frame_new (_("Center of SuperNova"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 4);

  g_signal_connect (frame, "destroy",
                    G_CALLBACK (nova_center_destroy),
                    center);

  table = gtk_table_new (3, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  label = gtk_label_new_with_mnemonic (_("_X:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  spinbutton =
    gimp_spin_button_new (&center->xadj,
                          pvals.xcenter, G_MININT, G_MAXINT,
                          1, 10, 10, 0, 0);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (spinbutton);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

  g_object_set_data (G_OBJECT (center->xadj), "center", center);

  g_signal_connect (center->xadj, "value_changed",
                    G_CALLBACK (nova_center_adjustment_update),
                    &pvals.xcenter);

  label = gtk_label_new_with_mnemonic (_("_Y:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  spinbutton =
    gimp_spin_button_new (&center->yadj,
                          pvals.ycenter, G_MININT, G_MAXINT,
                          1, 10, 10, 0, 0);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 3, 4, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (spinbutton);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

  g_object_set_data (G_OBJECT (center->yadj), "center", center);

  g_signal_connect (center->yadj, "value_changed",
                    G_CALLBACK (nova_center_adjustment_update),
                    &pvals.ycenter);

  /* frame that contains preview */
  pframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (table), pframe, 0, 4, 1, 2,
                    0, 0, 0, 0);

  /* PREVIEW */
  preview = gimp_old_preview_new (drawable, FALSE);
  gtk_widget_set_events (preview->widget, PREVIEW_MASK);
  gtk_container_add (GTK_CONTAINER (pframe), preview->widget);
  gtk_widget_show (preview->widget);

  g_signal_connect_after (preview->widget, "expose_event",
                          G_CALLBACK (nova_center_preview_expose),
                          center);
  g_signal_connect (preview->widget, "event",
                    G_CALLBACK (nova_center_preview_events),
                    center);

  gtk_widget_show (pframe);
  gtk_widget_show (table);

  g_object_set_data (G_OBJECT (frame), "center", center);

  gtk_widget_show (frame);

  check = gtk_check_button_new_with_mnemonic (_("S_how Cursor"));
  gtk_table_attach (GTK_TABLE (table), check, 0, 4, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), show_cursor);
  gtk_widget_show (check);

  g_signal_connect (check, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &show_cursor);
  g_signal_connect_swapped (check, "toggled",
                            G_CALLBACK (nova),
                            drawable);

  nova_center_cursor_update (center);

  center->cursor  = FALSE;   /* Make sure that the cursor has not been drawn */
  center->in_call = FALSE;   /* End of initialization */
  nova (drawable, TRUE);

  return frame;
}

static void
nova_center_destroy (GtkWidget *widget,
                     gpointer   data)
{
  NovaCenter *center = data;
  g_free (center);
}

/*
 *   Drawing CenterFrame
 *   if update & PREVIEW, draw preview
 *   if update & CURSOR,  draw cross cursor
 */

static void
nova_center_draw (NovaCenter *center,
                  gint        update)
{
  GtkWidget *prvw = preview->widget;

  if (update & PREVIEW)
    {
      center->cursor = FALSE;
    }

  if (update & CURSOR)
    {
      gdk_gc_set_function (prvw->style->black_gc, GDK_INVERT);
      if (show_cursor)
        {
          if (center->cursor)
            {
              gdk_draw_line (prvw->window,
                             prvw->style->black_gc,
                             center->oldx, 1, center->oldx,
                             preview->height - 1);
              gdk_draw_line (prvw->window,
                             prvw->style->black_gc,
                             1, center->oldy,
                             preview->width - 1, center->oldy);
            }

          gdk_draw_line (prvw->window,
                         prvw->style->black_gc,
                         center->curx, 1, center->curx,
                         preview->height - 1);
          gdk_draw_line (prvw->window,
                         prvw->style->black_gc,
                         1, center->cury,
                         preview->width - 1, center->cury);
        }

      /* current position of cursor is updated */
      center->oldx = center->curx;
      center->oldy = center->cury;
      center->cursor = TRUE;
      gdk_gc_set_function (prvw->style->black_gc, GDK_COPY);
    }
}

/*
 *  CenterFrame entry callback
 */

static void
nova_center_adjustment_update (GtkAdjustment *adjustment,
                               gpointer       data)
{
  NovaCenter *center;

  gimp_int_adjustment_update (adjustment, data);

  center = g_object_get_data (G_OBJECT (adjustment), "center");

  if (!center->in_call)
    {
      nova_center_cursor_update (center);
      nova_center_draw (center, CURSOR);
      nova (center->drawable, TRUE);
    }
}

/*
 *  Update the cross cursor's  coordinates accoding to pvals.[xy]center
 *  but not redraw it
 */

static void
nova_center_cursor_update (NovaCenter *center)
{
  center->curx = pvals.xcenter * preview->width / center->dwidth;
  center->cury = pvals.ycenter * preview->height / center->dheight;

  center->curx = CLAMP (center->curx, 0, preview->width - 1);
  center->cury = CLAMP (center->cury, 0, preview->height - 1);
}

/*
 *    Handle the expose event on the preview
 */
static gint
nova_center_preview_expose (GtkWidget *widget,
                            GdkEvent  *event,
                            gpointer   data)
{
  printf("Before\n");
  nova_center_draw ((NovaCenter*) data, ALL);
  printf("After\n");
  return FALSE;
}

/*
 *    Handle other events on the preview
 */

static gint
nova_center_preview_events (GtkWidget *widget,
                            GdkEvent  *event,
                            gpointer   data)
{
  NovaCenter *center;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;

  center = (NovaCenter *) data;

  switch (event->type)
    {
    case GDK_EXPOSE:
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      center->curx = bevent->x;
      center->cury = bevent->y;
      goto mouse;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      if (!mevent->state)
        break;
      center->curx = mevent->x;
      center->cury = mevent->y;
    mouse:
      nova_center_draw (center, CURSOR);
      center->in_call = TRUE;
      gtk_adjustment_set_value (GTK_ADJUSTMENT (center->xadj),
                                center->curx * center->dwidth /
                                preview->width);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (center->yadj),
                                center->cury * center->dheight /
                                preview->height);
      center->in_call = FALSE;
      nova (center->drawable, 1);
      break;

    default:
      break;
    }
  return FALSE;
}

/*
  ################################################################
  ##                                                            ##
  ##                   Main Calculation                         ##
  ##                                                            ##
  ################################################################
*/

static gdouble
gauss (GRand *gr)
{
  gdouble sum = 0;
  gint i;

  for (i = 0; i < 6; i++)
    sum += (gdouble) g_rand_double (gr);

  return sum / 6;
}

static void
nova (GimpDrawable *drawable,
      gboolean      preview_mode)
{
   GimpPixelRgn  src_rgn;
   GimpPixelRgn  dest_rgn;
   guchar   *src_row, *dest_row;
   guchar   *src, *dest;
   gint      x1, y1, x2, y2;
   gint      row, col;
   gint      x, y;
   gint      alpha, bpp;
   gint      progress, max_progress;
   gboolean  has_alpha;
   gint      xc, yc; /* center of nova */
   gdouble   u, v;
   gdouble   l;
   gdouble   w, w1, c;
   gdouble  *spoke;
   gdouble   nova_alpha;
   GimpRGB   color;
   GimpRGB   src_color;
   GimpRGB  *spokecolor;
   GimpHSV   hsv;
   gint      i;
   GRand    *gr;

   gr = g_rand_new ();

   /* initialize */
   has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

   spoke = g_new (gdouble, pvals.nspoke);
   spokecolor = g_new (GimpRGB, pvals.nspoke);

   gimp_rgb_set_alpha (&pvals.color, 1.0);
   gimp_rgb_to_hsv (&pvals.color, &hsv);

   for (i = 0; i < pvals.nspoke; i++)
     {
       spoke[i] = gauss (gr);

       hsv.h += ((gdouble) pvals.randomhue / 360.0) *
         g_rand_double_range (gr, -0.5, 0.5);
       if (hsv.h < 0)
         hsv.h += 1.0;
       else if (hsv.h >= 1.0)
         hsv.h -= 1.0;

       gimp_hsv_to_rgb (&hsv, spokecolor + i);
     }

   if (preview_mode)
     {
       xc = (gdouble) pvals.xcenter * preview->scale_x;
       yc = (gdouble) pvals.ycenter * preview->scale_y;

       x1 = y1 = 0;
       x2 = preview->width;
       y2 = preview->height;
       bpp = preview->bpp;
       has_alpha = FALSE;
       alpha = bpp;
     }
   else
     {
       gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
       bpp = gimp_drawable_bpp (drawable->drawable_id);
       alpha = (has_alpha) ? bpp - 1 : bpp;
       xc = pvals.xcenter;
       yc = pvals.ycenter;

       gimp_pixel_rgn_init (&src_rgn, drawable,
                            x1, y1, x2-x1, y2-y1, FALSE, FALSE);
       gimp_pixel_rgn_init (&dest_rgn, drawable,
                            x1, y1, x2-x1, y2-y1, TRUE, TRUE);
     }

   /* Initialize progress */
   progress     = 0;
   max_progress = (x2 - x1) * (y2 - y1);

   if (preview_mode)
     {
       src_row  = g_malloc (y2 * preview->rowstride);
       memcpy (src_row, preview->cache, y2 * preview->rowstride);

       dest_row = g_malloc (preview->rowstride);

       for (row = 0, y = 0; row < y2; row++, y++)
         {
           src  = src_row;
           dest = dest_row;

           for (col = 0, x = 0; col < x2; col++, x++)
             {
               u = (gdouble) (x - xc) / (pvals.radius * preview->scale_x);
               v = (gdouble) (y - yc) / (pvals.radius * preview->scale_y);
               l = sqrt (u * u + v * v);

               /* This algorithm is still under construction. */
               c = (atan2 (u, v) / (2 * G_PI) + .51) * pvals.nspoke;
               i = (gint) floor (c);
               c -= i;
               i %= pvals.nspoke;
               w1 = spoke[i] * (1 - c) + spoke[(i + 1) % pvals.nspoke] * c;
               w1 = w1 * w1;

               w = 1.0 / (l + 0.001) * 0.9;

               nova_alpha = CLAMP (w, 0.0, 1.0);

               /*  blend two neighbored spokecolors  */
               color = spokecolor[i];
               gimp_rgb_set_alpha (&color, 1.0);
               gimp_rgb_set_alpha (spokecolor + ((i + 1) % pvals.nspoke), c);
               gimp_rgb_composite (&color,
                                   spokecolor + (i + 1) % pvals.nspoke,
                                   GIMP_RGB_COMPOSITE_NORMAL);

               if (w > 1.0)
                 {
                   gimp_rgb_multiply (&color, w);
                   gimp_rgb_clamp (&color);
                 }
               else
                 {
                   gimp_rgba_set_uchar (&src_color,
                                        src[0], src[1], src[2], 1.0);
                   gimp_rgb_set_alpha (&color, nova_alpha);
                   gimp_rgb_composite (&color, &src_color,
                                       GIMP_RGB_COMPOSITE_BEHIND);
                 }

/*             c = CLAMP (w1 * w, 0.0, 1.0); */
/*             gimp_rgb_add (&color, c); */

               gimp_rgb_get_uchar (&color,
                                   dest, dest + 1, dest + 2);

               src  += bpp;
               dest += bpp;
             }

           src_row  += preview->rowstride;

           gimp_old_preview_do_row (preview, row, y2, dest_row);
         }

       gtk_widget_queue_draw (preview->widget);
     }
   else
     { /* normal mode */

#ifdef EEEEK
       for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
            pr != NULL; pr = gimp_pixel_rgns_process (pr))
         {
           src_row = src_rgn.data;
           dest_row = dest_rgn.data;

           for (row = 0, y = src_rgn.y; row < src_rgn.h; row++, y++)
             {
               src = src_row;
               dest = dest_row;

               for (col = 0, x = src_rgn.x; col < src_rgn.w; col++, x++)
                 {
                   u = (gdouble) (x-xc) / pvals.radius;
                   v = (gdouble) (y-yc) / pvals.radius;
                   l = sqrt(u*u + v*v);

                   /* This algorithm is still under construction. */
                   c = (atan2 (u, v) / (2 * G_PI) + .51) * pvals.nspoke;
                   i = (gint) floor (c);
                   c -= i;
                   i %= pvals.nspoke;
                   w1 = spoke[i] * (1 - c) + spoke[(i + 1) % pvals.nspoke] * c;
                   w1 = w1 * w1;

                   w = 1/(l+0.001)*0.9;

                   nova_alpha = CLAMP (w, 0.0, 1.0);

                   switch (bpp)
                     {
                     case 1:
                       gimp_rgba_set_uchar (&src_color,
                                            src[0], src[0], src[0], 1.0);
                       break;
                     case 2:
                       gimp_rgba_set_uchar (&src_color,
                                            src[0], src[0], src[0], src[1]);
                       break;
                     case 3:
                       gimp_rgba_set_uchar (&src_color,
                                            src[0], src[1], src[2], 1.0);
                       break;
                     case 4:
                       gimp_rgba_set_uchar (&src_color,
                                            src[0], src[1], src[2], src[3]);
                       break;
                     }

                   if (has_alpha)
                     {
                       src_alpha = (gdouble) src[alpha] / 255.0;
                       new_alpha = src_alpha + (1.0 - src_alpha) * nova_alpha;

                       if (new_alpha != 0.0)
                         ratio = nova_alpha / new_alpha;
                       else
                         ratio = 0.0;
                     }
                   else
                     ratio = nova_alpha;

                   compl_ratio = 1.0 - ratio;

                   for (j = 0; j < alpha; j++)
                     {
                       spokecol = (gdouble)spokecolor[3*i+j]*(1.0-c) +
                                  (gdouble)spokecolor[3*((i + 1) % pvals.nspoke)+j]*c;
                       if (w > 1.0)
                         color[j] = CLAMP (spokecol * w, 0, 255);
                       else
                         color[j] = src[j] * compl_ratio + spokecol * ratio;

                       c = CLAMP (w1 * w, 0, 1);
                       color[j] = color[j] + 255 * c;

                       dest[j]= CLAMP (color[j], 0, 255);
                     }

                   if (has_alpha)
                     dest[alpha] = new_alpha * 255.0;

                   src += src_rgn.bpp;
                   dest += dest_rgn.bpp;
                 }
               src_row += src_rgn.rowstride;
               dest_row += dest_rgn.rowstride;
             }

           /* Update progress */
           progress += src_rgn.w * src_rgn.h;
           gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
         }

       gimp_drawable_flush (drawable);
       gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
       gimp_drawable_update (drawable->drawable_id,
                             x1, y1, (x2 - x1), (y2 - y1));
#else
       gimp_message ("Sorry, the SuperNova effect\n"
                     "is broken at the moment and\n"
                     "has been temporarily disabled.");
#endif
     }

   g_free (spoke);
   g_free (spokecolor);
}
