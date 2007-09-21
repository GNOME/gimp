/*
 * This is a plug-in for GIMP.
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
 *
 *
 */

/*
 * Exchange one color with the other (settable threshold to convert from
 * one color-shade to another...might do wonders on certain images, or be
 * totally useless on others).
 *
 * Author: robert@experimental.net
 *
 * Added ability to select "from" color by clicking on the preview image.
 * As a side effect, clicking twice on the same spot reverses the action,
 * i.e. you can click once, see the result, and click again to revert.
 *
 * Also changed update policies for all sliders to delayed.  On a slow machine
 * the algorithm really chewes up CPU time.
 *
 * - timecop@japan.co.jp
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-exchange"
#define PLUG_IN_BINARY "exchange"

#define SCALE_WIDTH    128


/* datastructure to store parameters in */
typedef struct
{
  GimpRGB  from;
  GimpRGB  to;
  GimpRGB  threshold;
} myParams;

/* lets prototype */
static void     query (void);
static void     run   (const gchar      *name,
                       gint              nparams,
                       const GimpParam  *param,
                       gint             *nreturn_vals,
                       GimpParam       **return_vals);

static void     exchange              (GimpDrawable  *drawable,
                                       GimpPreview   *preview);

static gboolean exchange_dialog       (GimpDrawable  *preview);
static void     color_button_callback (GtkWidget     *widget,
                                       gpointer       data);
static void     scale_callback        (GtkAdjustment *adj,
                                       gpointer       data);


/* some global variables */
static myParams xargs =
{
  { 0.0, 0.0, 0.0, 1.0 }, /* from      */
  { 0.0, 0.0, 0.0, 1.0 }, /* to        */
  { 0.0, 0.0, 0.0, 1.0 }  /* threshold */
};

static GtkWidget    *from_colorbutton;
static gboolean      lock_threshold = FALSE;

/* lets declare what we want to do */
const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

/* run program */
MAIN ()

/* tell GIMP who we are */
static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",        "Interactive"        },
    { GIMP_PDB_IMAGE,    "image",           "Input image"        },
    { GIMP_PDB_DRAWABLE, "drawable",        "Input drawable"     },
    { GIMP_PDB_INT8,     "from-red",        "Red value (from)"   },
    { GIMP_PDB_INT8,     "from-green",      "Green value (from)" },
    { GIMP_PDB_INT8,     "from-blue",       "Blue value (from)"  },
    { GIMP_PDB_INT8,     "to-red",          "Red value (to)"     },
    { GIMP_PDB_INT8,     "to-green",        "Green value (to)"   },
    { GIMP_PDB_INT8,     "to-blue",         "Blue value (to)"    },
    { GIMP_PDB_INT8,     "red-threshold",   "Red threshold"      },
    { GIMP_PDB_INT8,     "green-threshold", "Green threshold"    },
    { GIMP_PDB_INT8,     "blue-threshold",  "Blue threshold"     }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Swap one color with another"),
                          "Exchange one color with another, optionally setting a threshold "
                          "to convert from one shade to another",
                          "robert@experimental.net",
                          "robert@experimental.net",
                          "June 17th, 1997",
                          N_("_Color Exchange..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Colors/Map");
}

/* main function */
static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpRunMode        runmode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpDrawable      *drawable;

  *nreturn_vals = 1;
  *return_vals = values;

  INIT_I18N ();

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  runmode        = param[0].data.d_int32;
  drawable       = gimp_drawable_get (param[2].data.d_drawable);

  switch (runmode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* retrieve stored arguments (if any) */
      gimp_get_data (PLUG_IN_PROC, &xargs);
      /* initialize using foreground color */
      gimp_context_get_foreground (&xargs.from);

      if (! exchange_dialog (drawable))
        return;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &xargs);
      /*
       * instead of recalling the last-set values,
       * run with the current foreground as 'from'
       * color, making ALT-F somewhat more useful.
       */
      gimp_context_get_foreground (&xargs.from);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 12)
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
      else
        {
          gimp_rgb_set_uchar (&xargs.from,
                              param[3].data.d_int8,
                              param[4].data.d_int8,
                              param[5].data.d_int8);
          gimp_rgb_set_uchar (&xargs.to,
                              param[6].data.d_int8,
                              param[7].data.d_int8,
                              param[8].data.d_int8);
          gimp_rgb_set_uchar (&xargs.threshold,
                              param[9].data.d_int8,
                              param[10].data.d_int8,
                              param[11].data.d_int8);
        }
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (gimp_drawable_is_rgb (drawable->drawable_id))
        {
          gimp_progress_init (_("Color Exchange"));
          gimp_tile_cache_ntiles (2 * (drawable->width /
                                       gimp_tile_width () + 1));
          exchange (drawable, NULL);
          gimp_drawable_detach (drawable);

          /* store our settings */
          if (runmode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC, &xargs, sizeof (myParams));

          /* and flush */
          if (runmode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();
        }
      else
        status = GIMP_PDB_EXECUTION_ERROR;
    }
  values[0].data.d_status = status;
}

static gboolean
preview_event_handler (GtkWidget *area,
                       GdkEvent  *event,
                       GtkWidget *preview)
{
  gint            pos;
  guchar         *buf;
  guint32         drawable_id;
  GimpRGB         color;
  GdkEventButton *button_event = (GdkEventButton *)event;

  buf = GIMP_PREVIEW_AREA (area)->buf;
  drawable_id = GIMP_DRAWABLE_PREVIEW (preview)->drawable->drawable_id;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if (button_event->button == 2)
        {
          pos = event->button.x * gimp_drawable_bpp (drawable_id) +
                event->button.y * GIMP_PREVIEW_AREA (area)->rowstride;

          gimp_rgb_set_uchar (&color, buf[pos], buf[pos + 1], buf[pos + 2]);
          gimp_color_button_set_color (GIMP_COLOR_BUTTON (from_colorbutton),
                                       &color);
        }
      break;

   default:
      break;
    }

  return FALSE;
}

/* show our dialog */
static gboolean
exchange_dialog (GimpDrawable *drawable)
{
  GtkWidget    *dialog;
  GtkWidget    *main_vbox;
  GtkWidget    *frame;
  GtkWidget    *preview;
  GtkWidget    *table;
  GtkWidget    *threshold;
  GtkWidget    *colorbutton;
  GtkWidget    *scale;
  GtkSizeGroup *group;
  GtkObject    *adj;
  gint          framenumber;
  gboolean      run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Color Exchange"), PLUG_IN_BINARY,
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

  /* do some boxes here */
  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  frame = gimp_frame_new (_("Middle-Click Inside Preview to Pick \"From Color\""));
  gtk_box_pack_start_defaults (GTK_BOX (main_vbox), frame);
  gtk_widget_show (frame);

  preview = gimp_drawable_preview_new (drawable, NULL);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (exchange),
                            drawable);
  g_signal_connect (GIMP_PREVIEW (preview)->area, "event",
                    G_CALLBACK (preview_event_handler),
                    preview);

  /*  a hidden color_button to handle the threshold more easily  */
  threshold = gimp_color_button_new (NULL, 1, 1,
                                     &xargs.threshold,
                                     GIMP_COLOR_AREA_FLAT);

  g_signal_connect (threshold, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &xargs.threshold);
  g_signal_connect (threshold, "color-changed",
                    G_CALLBACK (color_button_callback),
                    &xargs.threshold);
  g_signal_connect_swapped (threshold, "color-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* and our scales */
  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  for (framenumber = 0; framenumber < 2; framenumber++)
    {
      GtkWidget *image;
      gint       row = 0;

      frame = gimp_frame_new (framenumber ? _("To Color") : _("From Color"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      table = gtk_table_new (framenumber ? 4 : 8, 4, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 6);
      gtk_table_set_row_spacings (GTK_TABLE (table), 2);
      gtk_table_set_row_spacing (GTK_TABLE (table), 0, 12);
      if (!framenumber)
        {
          gtk_table_set_row_spacing (GTK_TABLE (table), 2, 6);
          gtk_table_set_row_spacing (GTK_TABLE (table), 4, 6);
          gtk_table_set_row_spacing (GTK_TABLE (table), 6, 6);
        }
      gtk_container_add (GTK_CONTAINER (frame), table);
      gtk_widget_show (table);

      colorbutton = gimp_color_button_new (framenumber ?
                                           _("Color Exchange: To Color") :
                                           _("Color Exchange: From Color"),
                                           SCALE_WIDTH / 2, 16,
                                           (framenumber ?
                                            &xargs.to : &xargs.from),
                                           GIMP_COLOR_AREA_FLAT);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                                 NULL, 0.0, 0.0,
                                 colorbutton, 1, FALSE);

      g_signal_connect (colorbutton, "color-changed",
                        G_CALLBACK (gimp_color_button_get_color),
                        framenumber ? &xargs.to : &xargs.from);
      g_signal_connect (colorbutton, "color-changed",
                        G_CALLBACK (color_button_callback),
                        framenumber ? &xargs.to : &xargs.from);
      g_signal_connect_swapped (colorbutton, "color-changed",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);

      if (!framenumber)
        from_colorbutton = colorbutton;

      /*  Red  */
      image = gtk_image_new_from_stock (GIMP_STOCK_CHANNEL_RED,
                                        GTK_ICON_SIZE_BUTTON);
      gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.5);
      gtk_table_attach (GTK_TABLE (table), image,
                        0, 1, row, row + 1 + (framenumber ? 0 : 1),
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (image);

      adj = gimp_scale_entry_new (GTK_TABLE (table), 1, row++,
                                  _("_Red:"), SCALE_WIDTH, 0,
                                  framenumber ? xargs.to.r : xargs.from.r,
                                  0.0, 1.0, 0.01, 0.1, 3,
                                  TRUE, 0, 0,
                                  NULL, NULL);

      g_object_set_data (G_OBJECT (adj), "colorbutton", colorbutton);
      g_object_set_data (G_OBJECT (colorbutton), "red", adj);

      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (gimp_double_adjustment_update),
                        framenumber ? &xargs.to.r : &xargs.from.r);
      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (scale_callback),
                        framenumber ? &xargs.to : &xargs.from);
      g_signal_connect_swapped (adj, "value-changed",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);

      scale = GTK_WIDGET (GIMP_SCALE_ENTRY_SCALE (adj));
      gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
      gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (adj));

      if (! framenumber)
        {
          adj = gimp_scale_entry_new (GTK_TABLE (table), 1, row++,
                                      _("R_ed threshold:"), SCALE_WIDTH, 0,
                                      xargs.threshold.r,
                                      0.0, 1.0, 0.01, 0.1, 3,
                                      TRUE, 0, 0,
                                      NULL, NULL);

          g_object_set_data (G_OBJECT (adj), "colorbutton", threshold);
          g_object_set_data (G_OBJECT (threshold), "red", adj);

          g_signal_connect (adj, "value-changed",
                            G_CALLBACK (gimp_double_adjustment_update),
                            &xargs.threshold.r);
          g_signal_connect (adj, "value-changed",
                            G_CALLBACK (scale_callback),
                            &xargs.threshold);
          g_signal_connect_swapped (adj, "value-changed",
                                    G_CALLBACK (gimp_preview_invalidate),
                                    preview);

          scale = GTK_WIDGET (GIMP_SCALE_ENTRY_SCALE (adj));
          gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
          gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (adj));
        }

      /*  Green  */
      image = gtk_image_new_from_stock (GIMP_STOCK_CHANNEL_GREEN,
                                        GTK_ICON_SIZE_BUTTON);
      gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.5);
      gtk_table_attach (GTK_TABLE (table), image,
                        0, 1, row, row + 1 + (framenumber ? 0 : 1),
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (image);

      adj = gimp_scale_entry_new (GTK_TABLE (table), 1, row++,
                                  _("_Green:"), SCALE_WIDTH, 0,
                                  framenumber ? xargs.to.g : xargs.from.g,
                                  0.0, 1.0, 0.01, 0.1, 3,
                                  TRUE, 0, 0,
                                  NULL, NULL);

      g_object_set_data (G_OBJECT (adj), "colorbutton", colorbutton);
      g_object_set_data (G_OBJECT (colorbutton), "green", adj);

      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (gimp_double_adjustment_update),
                        framenumber ? &xargs.to.g : &xargs.from.g);
      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (scale_callback),
                        framenumber ? &xargs.to : &xargs.from);
      g_signal_connect_swapped (adj, "value-changed",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);

      scale = GTK_WIDGET (GIMP_SCALE_ENTRY_SCALE (adj));
      gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
      gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (adj));

      if (!framenumber)
        {
          adj = gimp_scale_entry_new (GTK_TABLE (table), 1, row++,
                                      _("G_reen threshold:"), SCALE_WIDTH, 0,
                                      xargs.threshold.g,
                                      0.0, 1.0, 0.01, 0.1, 3,
                                      TRUE, 0, 0,
                                      NULL, NULL);

          g_object_set_data (G_OBJECT (adj), "colorbutton", threshold);
          g_object_set_data (G_OBJECT (threshold), "green", adj);

          g_signal_connect (adj, "value-changed",
                            G_CALLBACK (gimp_double_adjustment_update),
                            &xargs.threshold.g);
          g_signal_connect (adj, "value-changed",
                            G_CALLBACK (scale_callback),
                            &xargs.threshold);
          g_signal_connect_swapped (adj, "value-changed",
                                    G_CALLBACK (gimp_preview_invalidate),
                                    preview);


          scale = GTK_WIDGET (GIMP_SCALE_ENTRY_SCALE (adj));
          gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
          gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (adj));
        }

      /*  Blue  */
      image = gtk_image_new_from_stock (GIMP_STOCK_CHANNEL_BLUE,
                                        GTK_ICON_SIZE_BUTTON);
      gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.5);
      gtk_table_attach (GTK_TABLE (table), image,
                        0, 1, row, row + 1 + (framenumber ? 0 : 1),
                        GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (image);

      adj = gimp_scale_entry_new (GTK_TABLE (table), 1, row++,
                                  _("_Blue:"), SCALE_WIDTH, 0,
                                  framenumber ? xargs.to.b : xargs.from.b,
                                  0.0, 1.0, 0.01, 0.1, 3,
                                  TRUE, 0, 0,
                                  NULL, NULL);

      g_object_set_data (G_OBJECT (adj), "colorbutton", colorbutton);
      g_object_set_data (G_OBJECT (colorbutton), "blue", adj);

      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (gimp_double_adjustment_update),
                        framenumber ? &xargs.to.b : &xargs.from.b);
      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (scale_callback),
                        framenumber ? &xargs.to : &xargs.from);
      g_signal_connect_swapped (adj, "value-changed",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);

      scale = GTK_WIDGET (GIMP_SCALE_ENTRY_SCALE (adj));
      gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
      gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (adj));

      if (! framenumber)
        {
          adj = gimp_scale_entry_new (GTK_TABLE (table), 1, row++,
                                      _("B_lue threshold:"), SCALE_WIDTH, 0,
                                      xargs.threshold.b,
                                      0.0, 1.0, 0.01, 0.1, 3,
                                      TRUE, 0, 0,
                                      NULL, NULL);

          g_object_set_data (G_OBJECT (adj), "colorbutton", threshold);
          g_object_set_data (G_OBJECT (threshold), "blue", adj);

          g_signal_connect (adj, "value-changed",
                            G_CALLBACK (gimp_double_adjustment_update),
                            &xargs.threshold.b);
          g_signal_connect (adj, "value-changed",
                            G_CALLBACK (scale_callback),
                            &xargs.threshold);
          g_signal_connect_swapped (adj, "value-changed",
                                    G_CALLBACK (gimp_preview_invalidate),
                                    preview);

          scale = GTK_WIDGET (GIMP_SCALE_ENTRY_SCALE (adj));
          gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
          gtk_size_group_add_widget (group, GIMP_SCALE_ENTRY_LABEL (adj));
        }

      if (!framenumber)
        {
          GtkWidget *button;

          button = gtk_check_button_new_with_mnemonic (_("Lock _thresholds"));
          gtk_table_attach (GTK_TABLE (table), button, 2, 4, row, row + 1,
                            GTK_FILL, 0, 0, 0);
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                        lock_threshold);
          gtk_widget_show (button);

          g_signal_connect (button, "clicked",
                            G_CALLBACK (gimp_toggle_button_update),
                            &lock_threshold);
          g_signal_connect_swapped (button, "clicked",
                                    G_CALLBACK (gimp_preview_invalidate),
                                    preview);
        }
    }

  g_object_unref (group);

  /* show everything */
  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
color_button_callback (GtkWidget *widget,
                       gpointer   data)
{
  GtkObject *red_adj;
  GtkObject *green_adj;
  GtkObject *blue_adj;
  GimpRGB   *color;

  color = (GimpRGB *) data;

  red_adj   = (GtkObject *) g_object_get_data (G_OBJECT (widget), "red");
  green_adj = (GtkObject *) g_object_get_data (G_OBJECT (widget), "green");
  blue_adj  = (GtkObject *) g_object_get_data (G_OBJECT (widget), "blue");

  if (red_adj)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (red_adj),   color->r);
  if (green_adj)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (green_adj), color->g);
  if (blue_adj)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (blue_adj),  color->b);
}

static void
scale_callback (GtkAdjustment *adj,
                gpointer       data)
{
  GtkObject *object;
  GimpRGB   *color;

  color = (GimpRGB *) data;

  object = g_object_get_data (G_OBJECT (adj), "colorbutton");

  if (GIMP_IS_COLOR_BUTTON (object))
    {
      if (color == &xargs.threshold && lock_threshold == TRUE)
        gimp_rgb_set (color, adj->value, adj->value, adj->value);

      gimp_color_button_set_color (GIMP_COLOR_BUTTON (object), color);
    }
}

/* do the exchanging */
static void
exchange (GimpDrawable *drawable,
          GimpPreview  *preview)
{
  GimpPixelRgn  srcPR, destPR;
  guchar        min_red,  min_green,  min_blue;
  guchar        max_red,  max_green,  max_blue;
  guchar        from_red, from_green, from_blue;
  guchar        to_red,   to_green,   to_blue;
  guchar       *src_row, *dest_row;
  gint          x, y, bpp = drawable->bpp;
  gboolean      has_alpha;
  gint          x1, y1, x2, y2;
  gint          width, height;
  GimpRGB       min;
  GimpRGB       max;

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);

      x2 = x1 + width;
      y2 = y1 + height;
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

      width  = x2 - x1;
      height = y2 - y1;
    }

  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  /* allocate memory */
  src_row = g_new (guchar, drawable->width * bpp);

  gimp_rgb_get_uchar (&xargs.from, &from_red, &from_green, &from_blue);
  gimp_rgb_get_uchar (&xargs.to,   &to_red,   &to_green,   &to_blue);

  /* get boundary values */
  min = xargs.from;
  gimp_rgb_subtract (&min, &xargs.threshold);
  gimp_rgb_clamp (&min);
  gimp_rgb_get_uchar (&min, &min_red, &min_green, &min_blue);

  max = xargs.from;
  gimp_rgb_add (&max, &xargs.threshold);
  gimp_rgb_clamp (&max);
  gimp_rgb_get_uchar (&max, &max_red, &max_green, &max_blue);

  dest_row = g_new (guchar, drawable->width * bpp);

  gimp_pixel_rgn_init (&srcPR, drawable,
                       x1, y1, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable,
                       x1, y1, width, height, (preview == NULL), TRUE);

  for (y = y1; y < y2; y++)
    {
      gimp_pixel_rgn_get_row (&srcPR, src_row, x1, y, width);

      for (x = 0; x < width; x++)
        {
          guchar pixel_red, pixel_green, pixel_blue;
          guchar new_red, new_green, new_blue;
          guint  idx;

          /* get current pixel-values */
          pixel_red   = src_row[x * bpp];
          pixel_green = src_row[x * bpp + 1];
          pixel_blue  = src_row[x * bpp + 2];

          idx = x * bpp;

          /* want this pixel? */
          if (pixel_red >= min_red &&
              pixel_red <= max_red &&
              pixel_green >= min_green &&
              pixel_green <= max_green &&
              pixel_blue >= min_blue &&
              pixel_blue <= max_blue)
            {
              guchar red_delta, green_delta, blue_delta;

              red_delta   = pixel_red > from_red ?
                pixel_red - from_red : from_red - pixel_red;
              green_delta = pixel_green > from_green ?
                pixel_green - from_green : from_green - pixel_green;
              blue_delta  = pixel_blue > from_blue ?
                pixel_blue - from_blue : from_blue - pixel_blue;

              new_red   = CLAMP (to_red   + red_delta,   0, 255);
              new_green = CLAMP (to_green + green_delta, 0, 255);
              new_blue  = CLAMP (to_blue  + blue_delta,  0, 255);
            }
          else
            {
              new_red   = pixel_red;
              new_green = pixel_green;
              new_blue  = pixel_blue;
            }

          /* fill buffer */
          dest_row[idx + 0] = new_red;
          dest_row[idx + 1] = new_green;
          dest_row[idx + 2] = new_blue;

          /* copy alpha-channel */
          if (has_alpha)
            dest_row[idx + 3] = src_row[x * bpp + 3];
        }
      /* store the dest */
      gimp_pixel_rgn_set_row (&destPR, dest_row, x1, y, width);

      /* and tell the user what we're doing */
      if (!preview && (y % 10) == 0)
        gimp_progress_update ((gdouble) y / (gdouble) height);
    }

  g_free (src_row);
  g_free (dest_row);

  if (preview)
    {
      gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),
                                         &destPR);
    }
  else
    {
      /* update the processed region */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);
    }
}
