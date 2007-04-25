/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Supernova plug-in
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-nova"
#define PLUG_IN_BINARY "nova"

#define SCALE_WIDTH    125


typedef struct
{
  gint     xcenter;
  gint     ycenter;
  GimpRGB  color;
  gint     radius;
  gint     nspoke;
  gint     randomhue;
} NovaValues;

typedef struct
{
  GimpDrawable *drawable;
  GimpPreview  *preview;
  GtkWidget    *coords;
  gint          cursor_drawn;
  gint          curx, cury;              /* x,y of cursor in preview */
  gint          oldx, oldy;
} NovaCenter;


/* Declare a local function.
 */
static void        query                         (void);
static void        run                           (const gchar      *name,
                                                  gint              nparams,
                                                  const GimpParam  *param,
                                                  gint             *nreturn_vals,
                                                  GimpParam       **return_vals);

static void        nova                          (GimpDrawable     *drawable,
                                                  GimpPreview      *preview);

static gboolean    nova_dialog                   (GimpDrawable     *drawable);

static GtkWidget * nova_center_create            (GimpDrawable     *drawable,
                                                  GimpPreview      *preview);
static void        nova_center_cursor_draw       (NovaCenter       *center);
static void        nova_center_coords_update     (GimpSizeEntry    *coords,
                                                  NovaCenter       *center);
static void        nova_center_cursor_update     (NovaCenter       *center);
static void        nova_center_preview_realize   (GtkWidget        *widget,
                                                  NovaCenter       *center);
static gboolean    nova_center_preview_expose    (GtkWidget        *widget,
                                                  GdkEvent         *event,
                                                  NovaCenter       *center);
static gboolean    nova_center_preview_events    (GtkWidget        *widget,
                                                  GdkEvent         *event,
                                                  NovaCenter       *center);


const GimpPlugInInfo PLUG_IN_INFO =
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

static gboolean show_cursor = TRUE;


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[]=
  {
    { GIMP_PDB_INT32,    "run-mode",  "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",     "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",  "Input drawable" },
    { GIMP_PDB_INT32,    "xcenter",   "X coordinates of the center of supernova" },
    { GIMP_PDB_INT32,    "ycenter",   "Y coordinates of the center of supernova" },
    { GIMP_PDB_COLOR,    "color",     "Color of supernova" },
    { GIMP_PDB_INT32,    "radius",    "Radius of supernova" },
    { GIMP_PDB_INT32,    "nspoke",    "Number of spokes" },
    { GIMP_PDB_INT32,    "randomhue", "Random hue" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Add a starburst to the image"),
                          "This plug-in produces an effect like a supernova "
                          "burst. The amount of the light effect is "
                          "approximately in proportion to 1/r, where r is the "
                          "distance from the center of the star. It works with "
                          "RGB*, GRAY* image.",
                          "Eiichi Takamori",
                          "Eiichi Takamori",
                          "May 2000",
                          N_("Super_nova..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC,
                             "<Image>/Filters/Light and Shadow/Light");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
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
      gimp_get_data (PLUG_IN_PROC, &pvals);

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
      gimp_get_data (PLUG_IN_PROC, &pvals);
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
          gimp_progress_init (_("Rendering supernova"));
          gimp_tile_cache_ntiles (2 *
                                  (drawable->width / gimp_tile_width () + 1));

          nova (drawable, NULL);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC, &pvals, sizeof (NovaValues));
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

static gboolean
nova_dialog (GimpDrawable *drawable)
{
  GtkWidget  *dialog;
  GtkWidget  *main_vbox;
  GtkWidget  *preview;
  GtkWidget  *frame;
  GtkWidget  *table;
  GtkWidget  *button;
  GtkObject  *adj;
  gboolean    run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Supernova"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_zoom_preview_new (drawable);
  gtk_widget_add_events (GIMP_PREVIEW (preview)->area,
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON1_MOTION_MASK);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (nova),
                            drawable);

  frame = nova_center_create (drawable, GIMP_PREVIEW (preview));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Color */
  if (gimp_drawable_is_rgb (drawable->drawable_id))
    {
      button = gimp_color_button_new (_("Supernova Color Picker"),
                                      SCALE_WIDTH - 8, 16,
                                      &pvals.color, GIMP_COLOR_AREA_FLAT);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                 _("Co_lor:"), 0.0, 0.5,
                                 button, 1, TRUE);

      g_signal_connect (button, "color-changed",
                        G_CALLBACK (gimp_color_button_get_color),
                        &pvals.color);
      g_signal_connect_swapped (button, "color-changed",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);
    }

  /* Radius */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("_Radius:"), SCALE_WIDTH, 8,
                              pvals.radius, 1, 100, 1, 10, 0,
                              FALSE, 1, GIMP_MAX_IMAGE_SIZE,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &pvals.radius);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  /* Number of spokes */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                              _("_Spokes:"), SCALE_WIDTH, 8,
                              pvals.nspoke, 1, 1024, 1, 16, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &pvals.nspoke);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* Randomness of hue */
  if (gimp_drawable_is_rgb (drawable->drawable_id))
    {
      adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
                                  _("R_andom hue:"), SCALE_WIDTH, 8,
                                  pvals.randomhue, 0, 360, 1, 15, 0,
                                  TRUE, 0, 0,
                                  NULL, NULL);
      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (gimp_int_adjustment_update),
                        &pvals.randomhue);
      g_signal_connect_swapped (adj, "value-changed",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);
    }
  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

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
nova_center_create (GimpDrawable *drawable,
                    GimpPreview  *preview)
{
  NovaCenter *center;
  GtkWidget  *frame;
  GtkWidget  *hbox;
  GtkWidget  *check;
  gint32      image_ID;
  gdouble     res_x;
  gdouble     res_y;

  center = g_new0 (NovaCenter, 1);

  center->drawable     = drawable;
  center->preview      = preview;
  center->cursor_drawn = FALSE;
  center->curx         = 0;
  center->cury         = 0;
  center->oldx         = 0;
  center->oldy         = 0;

  frame = gimp_frame_new (_("Center of Nova"));

  g_signal_connect_swapped (frame, "destroy",
                            G_CALLBACK (g_free),
                            center);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  image_ID = gimp_drawable_get_image (drawable->drawable_id);
  gimp_image_get_resolution (image_ID, &res_x, &res_y);

  center->coords = gimp_coordinates_new (GIMP_UNIT_PIXEL, "%p", TRUE, TRUE, -1,
                                         GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                         FALSE, FALSE,

                                         _("_X:"), pvals.xcenter, res_x,
                                         - (gdouble) drawable->width,
                                         2 * drawable->width,
                                         0, drawable->width,

                                         _("_Y:"), pvals.ycenter, res_y,
                                         - (gdouble) drawable->height,
                                         2 * drawable->height,
                                         0, drawable->height);

  gtk_table_set_row_spacing (GTK_TABLE (center->coords), 1, 6);
  gtk_box_pack_start (GTK_BOX (hbox), center->coords, FALSE, FALSE, 0);
  gtk_widget_show (center->coords);

  g_signal_connect (center->coords, "value-changed",
                    G_CALLBACK (nova_center_coords_update),
                    center);
  g_signal_connect (center->coords, "refval-changed",
                    G_CALLBACK (nova_center_coords_update),
                    center);

  check = gtk_check_button_new_with_mnemonic (_("Show _position"));
  gtk_table_attach (GTK_TABLE (center->coords), check, 0, 5, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), show_cursor);
  gtk_widget_show (check);

  g_signal_connect (check, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &show_cursor);
  g_signal_connect_swapped (check, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  g_signal_connect (preview->area, "realize",
                    G_CALLBACK (nova_center_preview_realize),
                    center);
  g_signal_connect_after (preview->area, "expose-event",
                          G_CALLBACK (nova_center_preview_expose),
                          center);
  g_signal_connect (preview->area, "event",
                    G_CALLBACK (nova_center_preview_events),
                    center);

  nova_center_cursor_update (center);

  return frame;
}

/*
 *   Drawing CenterFrame
 *   if update & PREVIEW, draw preview
 *   if update & CURSOR,  draw cross cursor
 */

static void
nova_center_cursor_draw (NovaCenter *center)
{
  GtkWidget *prvw = center->preview->area;
  gint       width, height;

  gimp_preview_get_size (center->preview, &width, &height);

  gdk_gc_set_function (prvw->style->black_gc, GDK_INVERT);

  if (show_cursor)
    {
      if (center->cursor_drawn)
        {
          gdk_draw_line (prvw->window,
                         prvw->style->black_gc,
                         center->oldx, 1, center->oldx,
                         height - 1);
          gdk_draw_line (prvw->window,
                         prvw->style->black_gc,
                         1, center->oldy,
                         width - 1, center->oldy);
        }

      gdk_draw_line (prvw->window,
                     prvw->style->black_gc,
                     center->curx, 1, center->curx,
                     height - 1);
      gdk_draw_line (prvw->window,
                     prvw->style->black_gc,
                     1, center->cury,
                     width - 1, center->cury);
    }

  /* current position of cursor is updated */
  center->oldx         = center->curx;
  center->oldy         = center->cury;
  center->cursor_drawn = TRUE;

  gdk_gc_set_function (prvw->style->black_gc, GDK_COPY);
}

/*
 *  CenterFrame entry callback
 */
static void
nova_center_coords_update (GimpSizeEntry *coords,
                           NovaCenter    *center)
{
  pvals.xcenter = gimp_size_entry_get_refval (coords, 0);
  pvals.ycenter = gimp_size_entry_get_refval (coords, 1);

  nova_center_cursor_update (center);
  nova_center_cursor_draw (center);

  gimp_preview_invalidate (center->preview);
}

/*
 *  Update the cross cursor's  coordinates accoding to pvals.[xy]center
 *  but do not redraw it.
 */
static void
nova_center_cursor_update (NovaCenter *center)
{
  gimp_preview_transform (center->preview,
                          pvals.xcenter, pvals.ycenter,
                          &center->curx, &center->cury);
}

/*
 *  Set the preview area'a cursor on realize
 */
static void
nova_center_preview_realize (GtkWidget  *widget,
                             NovaCenter *center)
{
  GdkDisplay *display = gtk_widget_get_display (widget);
  GdkCursor  *cursor  = gdk_cursor_new_for_display (display, GDK_CROSSHAIR);

  gimp_preview_set_default_cursor (center->preview, cursor);
  gdk_cursor_unref (cursor);
}

/*
 *    Handle the expose event on the preview
 */
static gboolean
nova_center_preview_expose (GtkWidget  *widget,
                            GdkEvent   *event,
                            NovaCenter *center)
{
  center->cursor_drawn = FALSE;

  nova_center_cursor_update (center);
  nova_center_cursor_draw (center);

  return FALSE;
}

/*
 *    Handle other events on the preview
 */

static gboolean
nova_center_update (GtkWidget  *widget,
                    NovaCenter *center,
                    gint        x,
                    gint        y)
{
  gint tx, ty;

  gimp_preview_untransform (center->preview, x, y, &tx, &ty);

  nova_center_cursor_draw (center);

  g_signal_handlers_block_by_func (center->coords,
                                   nova_center_coords_update,
                                   center);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (center->coords), 0, tx);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (center->coords), 1, ty);

  g_signal_handlers_unblock_by_func (center->coords,
                                     nova_center_coords_update,
                                     center);

  nova_center_coords_update (GIMP_SIZE_ENTRY (center->coords), center);

  return TRUE;
}

static gboolean
nova_center_preview_events (GtkWidget  *widget,
                            GdkEvent   *event,
                            NovaCenter *center)
{
  switch (event->type)
    {
    case GDK_MOTION_NOTIFY:
      {
        GdkEventMotion *mevent = (GdkEventMotion *) event;

        if (mevent->state & GDK_BUTTON1_MASK)
          {
            GdkModifierType mask;
            gint            x, y;

            gdk_window_get_pointer (widget->window, &x, &y, &mask);

            return nova_center_update (widget, center, x, y);
          }
      }
      break;

    case GDK_BUTTON_PRESS:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;

        if (bevent->button == 1)
          return nova_center_update (widget, center, bevent->x, bevent->y);
      }
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
  gdouble sum = 0.0;
  gint    i;

  for (i = 0; i < 6; i++)
    sum += g_rand_double (gr);

  return sum / 6.0;
}

static void
nova (GimpDrawable *drawable,
      GimpPreview  *preview)
{
   GimpPixelRgn  src_rgn;
   GimpPixelRgn  dest_rgn;
   gpointer      pr;
   guchar       *src_row, *dest_row, *save_src;
   guchar       *src, *dest;
   gint          x1, y1, x2, y2, x, y;
   gint          row, col;
   gint          alpha, bpp;
   gint          progress, max_progress;
   gboolean      has_alpha;
   gint          xc, yc; /* center of nova */
   gdouble       u, v, l, w, w1, c, t;
   gdouble      *spoke;
   gdouble       nova_alpha, src_alpha, new_alpha = 0.0;
   gdouble       compl_ratio, ratio;
   GimpRGB       color;
   GimpRGB      *spokecolor;
   GimpHSV       hsv;
   gdouble       spokecol;
   gint          i;
   GRand        *gr;
   guchar       *cache = NULL;
   gint          width, height;
   gdouble       zoom = 0.0;

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

   if (preview)
     {
       cache = gimp_zoom_preview_get_source (GIMP_ZOOM_PREVIEW (preview),
                                             &width, &height, &bpp);

       zoom = gimp_zoom_preview_get_factor (GIMP_ZOOM_PREVIEW (preview));

       gimp_preview_transform (preview,
                               pvals.xcenter, pvals.ycenter, &xc, &yc);

       x1 = 0;
       y1 = 0;
       x2 = width;
       y2 = height;
     }
   else
     {
       gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
       bpp = gimp_drawable_bpp (drawable->drawable_id);
       xc = pvals.xcenter;
       yc = pvals.ycenter;

       gimp_pixel_rgn_init (&src_rgn, drawable,
                            x1, y1, x2-x1, y2-y1, FALSE, FALSE);
       gimp_pixel_rgn_init (&dest_rgn, drawable,
                            x1, y1, x2-x1, y2-y1, TRUE, TRUE);
     }

   alpha = (has_alpha) ? bpp - 1 : bpp;

   /* Initialize progress */
   progress     = 0;
   max_progress = (x2 - x1) * (y2 - y1);

   if (preview)
     {
       save_src = src_row  = g_new (guchar, y2 * width * bpp);
       memcpy (src_row, cache, y2 * width * bpp);

       dest_row = g_new (guchar, y2 * width * bpp);
       dest = dest_row;
       src  = src_row;

       for (row = 0, y = 0; row < y2; row++, y++)
         {

           for (col = 0, x = 0; col < x2; col++, x++)
             {
               u = ((gdouble) (x - xc) /
                    ((gdouble) pvals.radius * width /
                     drawable->width * zoom));
               v = ((gdouble) (y - yc) /
                    ((gdouble) pvals.radius * height /
                     drawable->height * zoom));

               l = sqrt (SQR (u) + SQR (v));

               /* This algorithm is still under construction. */
               t = (atan2 (u, v) / (2 * G_PI) + .51) * pvals.nspoke;
               i = (gint) floor (t);
               t -= i;
               i %= pvals.nspoke;
               w1 = spoke[i] * (1 - t) + spoke[(i + 1) % pvals.nspoke] * t;
               w1 = w1 * w1;

               w = 1.0 / (l + 0.001) * 0.9;

               nova_alpha = CLAMP (w, 0.0, 1.0);

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

               /* red or gray */
               spokecol = (gdouble)spokecolor[i                   ].r * (1.0-t) +
                          (gdouble)spokecolor[(i+1) % pvals.nspoke].r * t;

               if (w>1.0)
                 color.r = CLAMP (spokecol * w, 0.0, 1.0);
               else
                 color.r = src[0]/255.0 * compl_ratio + spokecol * ratio;
               c = CLAMP (w1 * w, 0.0, 1.0);
               color.r += c;
               dest[0] = CLAMP (color.r*255.0, 0, 255);

               if (bpp>2)
               {
                 /* green */
                 spokecol = (gdouble)spokecolor[i                   ].g * (1.0-t) +
                            (gdouble)spokecolor[(i+1) % pvals.nspoke].g * t;

                 if (w>1.0)
                   color.g = CLAMP (spokecol * w, 0.0, 1.0);
                 else
                   color.g = src[1]/255.0 * compl_ratio + spokecol * ratio;
                 c = CLAMP (w1 * w, 0.0, 1.0);
                 color.g += c;
                 dest[1] = CLAMP (color.g*255.0, 0, 255);

                 /* blue */
                 spokecol = (gdouble)spokecolor[i                   ].b * (1.0-t) +
                            (gdouble)spokecolor[(i+1) % pvals.nspoke].b * t;

                 if (w>1.0)
                   color.b = CLAMP (spokecol * w, 0.0, 1.0);
                 else
                   color.b = src[2]/255.0 * compl_ratio + spokecol * ratio;
                 c = CLAMP (w1 * w, 0.0, 1.0);
                 color.b += c;
                 dest[2] = CLAMP (color.b*255.0, 0, 255);
               }

               /* alpha */
               if (has_alpha)
                 dest[alpha] = new_alpha * 255.0;

               src  += bpp;
               dest += bpp;
             }
         }

       gimp_preview_draw_buffer (preview, dest_row, bpp * width);

       g_free (cache);
       g_free (save_src);
       g_free (dest_row);
     }
   else
     { /* normal mode */
       for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
            pr != NULL ;
            pr = gimp_pixel_rgns_process (pr))
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
                   t = (atan2 (u, v) / (2 * G_PI) + .51) * pvals.nspoke;
                   i = (gint) floor (t);
                   t -= i;
                   i %= pvals.nspoke;
                   w1 = spoke[i] * (1 - t) + spoke[(i + 1) % pvals.nspoke] * t;
                   w1 = w1 * w1;

                   w = 1/(l+0.001)*0.9;

                   nova_alpha = CLAMP (w, 0.0, 1.0);

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

                   switch (bpp)
                     {
                     case 1:
                     case 2:
                       /* gray */
                       spokecol = (gdouble)spokecolor[i                   ].r * (1.0-t) +
                                  (gdouble)spokecolor[(i+1) % pvals.nspoke].r * t;
                       if (w>1.0)
                         color.r = CLAMP (spokecol * w, 0.0, 1.0);
                       else
                         color.r = src[0]/255.0 * compl_ratio + spokecol * ratio;
                       c = CLAMP (w1 * w, 0.0, 1.0);
                       color.r += c;
                       dest[0] = CLAMP (color.r*255.0, 0, 255);
                       break;

                     case 3:
                     case 4:
                       /* red */
                       spokecol = (gdouble)spokecolor[i                   ].r * (1.0-t) +
                                  (gdouble)spokecolor[(i+1) % pvals.nspoke].r * t;
                       if (w>1.0)
                         color.r = CLAMP (spokecol * w, 0.0, 1.0);
                       else
                         color.r = src[0]/255.0 * compl_ratio + spokecol * ratio;
                       c = CLAMP (w1 * w, 0.0, 1.0);
                       color.r += c;
                       dest[0] = CLAMP (color.r*255.0, 0, 255);
                       /* green */
                       spokecol = (gdouble)spokecolor[i                   ].g * (1.0-t) +
                                  (gdouble)spokecolor[(i+1) % pvals.nspoke].g * t;
                       if (w>1.0)
                         color.g = CLAMP (spokecol * w, 0.0, 1.0);
                       else
                         color.g = src[1]/255.0 * compl_ratio + spokecol * ratio;
                       c = CLAMP (w1 * w, 0.0, 1.0);
                       color.g += c;
                       dest[1] = CLAMP (color.g*255.0, 0, 255);
                       /* blue */
                       spokecol = (gdouble)spokecolor[i                   ].b * (1.0-t) +
                                  (gdouble)spokecolor[(i+1) % pvals.nspoke].b * t;
                       if (w>1.0)
                         color.b = CLAMP (spokecol * w, 0.0, 1.0);
                       else
                         color.b = src[2]/255.0 * compl_ratio + spokecol * ratio;
                       c = CLAMP (w1 * w, 0.0, 1.0);
                       color.b += c;
                       dest[2] = CLAMP (color.b*255.0, 0, 255);
                       break;
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
     }

   g_free (spoke);
   g_free (spokecolor);
   g_rand_free (gr);
}
