/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Tileit - This plugin take an image and makes repeated copies of it.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * A fair proportion of this code was taken from the Whirl plug-in
 * which was copyrighted by Federico Mena Quintero (as below).
 *
 * Whirl plug-in --- distort an image into a whirlpool
 * Copyright (C) 1997 Federico Mena Quintero
 *
 */

/* Change log:-
 * 0.2  Added new functions to allow "editing" of the tile pattern.
 *
 * 0.1 First version released.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define PLUG_IN_PROC   "plug-in-small-tiles"
#define PLUG_IN_BINARY "tile-small"
#define PLUG_IN_ROLE   "gimp-tile-small"

/***** Magic numbers *****/

#define PREVIEW_SIZE 128

#define MAX_SEGS       6

#define PREVIEW_MASK   GDK_EXPOSURE_MASK | \
                       GDK_BUTTON_PRESS_MASK | \
                       GDK_BUTTON_MOTION_MASK

/* Variables set in dialog box */
typedef struct data
{
  gint numtiles;
} TileItVals;

typedef struct
{
  GtkWidget *preview;
  guchar     preview_row[PREVIEW_SIZE * 4];
  gint       img_bpp;
  guchar    *pv_cache;
} TileItInterface;

static TileItInterface tint =
{
  NULL,  /* Preview */
  {
    '4',
    'u'
  },     /* Preview_row */
  4,     /* bpp of drawable */
  NULL
};


typedef struct _Tile      Tile;
typedef struct _TileClass TileClass;

struct _Tile
{
  GimpPlugIn parent_instance;
};

struct _TileClass
{
  GimpPlugInClass parent_class;
};


#define TILE_TYPE  (tile_get_type ())
#define TILE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TILE_TYPE, Tile))

GType                   tile_get_type         (void) G_GNUC_CONST;

static GList          * tile_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * tile_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * tile_run              (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               GimpDrawable        **drawables,
                                               GimpProcedureConfig  *config,
                                               gpointer              run_data);

static gboolean  tileit_dialog          (GimpProcedure       *procedure,
                                         GimpProcedureConfig *config,
                                         GimpDrawable        *drawable);

static void      tileit_scale_update    (GimpLabelSpin       *entry,
                                         gint                *value);
static void      tileit_config_update   (GimpProcedureConfig *config);

static void      tileit_exp_update      (GtkWidget           *widget,
                                         gpointer             value);
static void      tileit_exp_update_f    (GtkWidget           *widget,
                                         gpointer             value);

static void      tileit_reset           (GtkWidget           *widget,
                                         gpointer             value);
static void      tileit_radio_update    (GtkWidget           *widget,
                                         gpointer             data);
static void      tileit_hvtoggle_update (GtkWidget           *widget,
                                         gpointer             data);

static void      do_tiles               (GimpProcedureConfig *config,
                                         GimpDrawable        *drawable);
static gint      tiles_xy               (GimpProcedureConfig *config,
                                         gint                 width,
                                         gint                 height,
                                         gint                 x,
                                         gint                 y,
                                         gint                *nx,
                                         gint                *ny);
static void      all_update             (void);
static void      alt_update             (void);
static void      explicit_update        (gboolean);

static void      dialog_update_preview  (void);
static void      cache_preview          (GimpDrawable        *drawable);
static gboolean  tileit_preview_draw    (GtkWidget           *widget,
                                         cairo_t             *cr);
static gboolean  tileit_preview_events  (GtkWidget           *widget,
                                         GdkEvent            *event);


G_DEFINE_TYPE (Tile, tile, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (TILE_TYPE)
DEFINE_STD_SET_I18N


/* Values when first invoked */
static TileItVals itvals =
{
  2
};

/* Structures for call backs... */
/* The "explicit tile" & family */
typedef enum
{
  ALL,
  ALT,
  EXPLICIT
} AppliedTo;

typedef struct
{
  AppliedTo  type;

  gint           x;        /* X - pos of tile   */
  gint           y;        /* Y - pos of tile   */
  GtkAdjustment *r_adj;    /* row adjustment    */
  GtkAdjustment *c_adj;    /* column adjustment */
  GtkWidget     *applybut; /* The apply button  */
} Exp_Call;

static Exp_Call exp_call =
{
  ALL,
  -1,
  -1,
  NULL,
  NULL,
  NULL,
};

/* The reset button needs to know some toggle widgets.. */

typedef struct
{
  GtkWidget *htoggle;
  GtkWidget *vtoggle;
} Reset_Call;

static Reset_Call res_call =
{
  NULL,
  NULL,
};

/* 2D - Array that holds the actions for each tile */
/* Action type on cell */
#define HORIZONTAL 0x1
#define VERTICAL   0x2

static gint tileactions[MAX_SEGS][MAX_SEGS];

/* What actions buttons toggled */
static gint   do_horz = FALSE;
static gint   do_vert = FALSE;
static gint   opacity = 100;

/* Stuff for the preview bit */
static gint     sel_x1, sel_y1, sel_x2, sel_y2;
static gint     sel_width, sel_height;
static gint     preview_width, preview_height;
static gboolean has_alpha;


static void
tile_class_init (TileClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = tile_query_procedures;
  plug_in_class->create_procedure = tile_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
tile_init (Tile *tile)
{
}

static GList *
tile_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
tile_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            tile_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Small Tiles..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Map");

      gimp_procedure_set_documentation (procedure,
                                        _("Tile image into smaller "
                                          "versions of the original"),
                                        "More here later",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Andy Thomas",
                                      "Andy Thomas",
                                      "1997");

      gimp_procedure_add_int_argument (procedure, "num-tiles",
                                       _("_n²"),
                                       _("Number of tiles to make"),
                                       2, MAX_SEGS, 2,
                                       G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
tile_run (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          GimpDrawable        **drawables,
          GimpProcedureConfig  *config,
          gpointer              run_data)
{
  GimpDrawable *drawable;
  gint          pwidth;
  gint          pheight;

  gegl_init (NULL, NULL);

  if (gimp_core_object_array_get_length ((GObject **) drawables) != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  has_alpha = gimp_drawable_has_alpha (drawable);

  if (! gimp_drawable_mask_intersect (drawable,
                                      &sel_x1,    &sel_y1,
                                      &sel_width, &sel_height))
    {
      g_message (_("Region selected for filter is empty."));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_SUCCESS,
                                               NULL);
    }

  sel_x2 = sel_x1 + sel_width;
  sel_y2 = sel_y1 + sel_height;

  /* Calculate preview size */

  if (sel_width > sel_height)
    {
      pwidth  = MIN (sel_width, PREVIEW_SIZE);
      pheight = sel_height * pwidth / sel_width;
    }
  else
    {
      pheight = MIN (sel_height, PREVIEW_SIZE);
      pwidth  = sel_width * pheight / sel_height;
    }

  preview_width  = MAX (pwidth,  2);  /* Min size is 2 */
  preview_height = MAX (pheight, 2);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      if (! tileit_dialog (procedure, config, drawable))
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CANCEL,
                                                   NULL);
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      break;
    }

  if (gimp_drawable_is_rgb  (drawable) ||
      gimp_drawable_is_gray (drawable))
    {
      gimp_progress_init (_("Tiling"));

      do_tiles (config, drawable);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }
  else
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static GtkWidget *
spin_button_new (GtkAdjustment **adjustment,  /* return value */
                 gdouble         value,
                 gdouble         lower,
                 gdouble         upper,
                 gdouble         step_increment,
                 gdouble         page_increment,
                 gdouble         page_size,
                 gdouble         climb_rate,
                 guint           digits)
{
  GtkWidget *spinbutton;

  *adjustment = gtk_adjustment_new (value, lower, upper,
                                    step_increment, page_increment, 0);

  spinbutton = gimp_spin_button_new (*adjustment, climb_rate, digits);

  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);

  return spinbutton;
}

static gboolean
tileit_dialog (GimpProcedure       *procedure,
               GimpProcedureConfig *config,
               GimpDrawable        *drawable)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *hbox;
  GtkWidget     *vbox;
  GtkWidget     *frame;
  GtkWidget     *grid;
  GtkWidget     *button;
  GtkWidget     *label;
  GtkWidget     *spinbutton;
  GtkAdjustment *adj;
  GtkWidget     *scale;
  GtkWidget     *toggle;
  GSList        *orientation_group = NULL;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY);

  cache_preview (drawable); /* Get the preview image */

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Small Tiles"));

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  tint.preview = gimp_preview_area_new ();
  g_object_set (tint.preview,
                "check-size",          gimp_check_size (),
                "check-type",          gimp_check_type (),
                "check-custom-color1", gimp_check_custom_color1 (),
                "check-custom-color2", gimp_check_custom_color2 (),
                NULL);
  gtk_widget_set_size_request (tint.preview, preview_width, preview_height);
  gtk_widget_set_events (GTK_WIDGET (tint.preview), PREVIEW_MASK);
  gtk_container_add (GTK_CONTAINER (frame), tint.preview);
  gtk_widget_show (tint.preview);

  g_signal_connect_after (tint.preview, "draw",
                          G_CALLBACK (tileit_preview_draw),
                          NULL);
  g_signal_connect (tint.preview, "event",
                    G_CALLBACK (tileit_preview_events),
                    NULL);

  /* Area for buttons etc */

  frame = gimp_frame_new (_("Flip"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  toggle = gtk_check_button_new_with_mnemonic (_("_Horizontal"));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (tileit_hvtoggle_update),
                    &do_horz);

  res_call.htoggle = toggle;

  toggle = gtk_check_button_new_with_mnemonic (_("_Vertical"));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (tileit_hvtoggle_update),
                    &do_vert);

  res_call.vtoggle = toggle;

  button = gtk_button_new_with_mnemonic (_("_Reset"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (tileit_reset),
                    &res_call);

  /* Grid for the inner widgets..*/
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  toggle = gtk_radio_button_new_with_mnemonic (orientation_group,
                                               _("A_ll tiles"));
  orientation_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, 0, 4, 1);
  gtk_widget_show (toggle);

  g_object_set_data (G_OBJECT (toggle), "gimp-item-data",
                     GINT_TO_POINTER (ALL));

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (tileit_radio_update),
                    &exp_call.type);

  toggle = gtk_radio_button_new_with_mnemonic (orientation_group,
                                               _("Al_ternate tiles"));
  orientation_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, 1, 4, 1);
  gtk_widget_show (toggle);

  g_object_set_data (G_OBJECT (toggle), "gimp-item-data",
                     GINT_TO_POINTER (ALT));

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (tileit_radio_update),
                    &exp_call.type);

  toggle = gtk_radio_button_new_with_mnemonic (orientation_group,
                                               _("_Explicit tile"));
  orientation_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (toggle));
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, 2, 1, 2);
  gtk_widget_show (toggle);

  label = gtk_label_new_with_mnemonic (_("Ro_w:"));
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 2, 1, 1);
  gtk_widget_show (label);

  g_object_bind_property (toggle, "active",
                          label,  "sensitive",
                          G_BINDING_SYNC_CREATE);

  spinbutton = spin_button_new (&adj, 2, 1, 6, 1, 1, 0, 1, 0);
  gtk_widget_set_hexpand (spinbutton, TRUE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);
  gtk_grid_attach (GTK_GRID (grid), spinbutton, 2, 2, 1, 1);
  gtk_widget_show (spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (tileit_exp_update_f),
                    &exp_call);

  exp_call.r_adj = adj;

  g_object_bind_property (toggle,     "active",
                          spinbutton, "sensitive",
                          G_BINDING_SYNC_CREATE);

  label = gtk_label_new_with_mnemonic (_("Col_umn:"));
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_widget_show (label);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 3, 1, 1);

  g_object_bind_property (toggle, "active",
                          label,  "sensitive",
                          G_BINDING_SYNC_CREATE);

  spinbutton = spin_button_new (&adj, 2, 1, 6, 1, 1, 0, 1, 0);
  gtk_widget_set_hexpand (spinbutton, TRUE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);
  gtk_grid_attach (GTK_GRID (grid), spinbutton, 2, 3, 1, 1);
  gtk_widget_show (spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (tileit_exp_update_f),
                    &exp_call);

  exp_call.c_adj = adj;

  g_object_bind_property (toggle,     "active",
                          spinbutton, "sensitive",
                          G_BINDING_SYNC_CREATE);

  g_object_set_data (G_OBJECT (toggle), "gimp-item-data",
                     GINT_TO_POINTER (EXPLICIT));

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (tileit_radio_update),
                    &exp_call.type);

  button = gtk_button_new_with_mnemonic (_("_Apply"));
  gtk_grid_attach (GTK_GRID (grid), button, 3, 2, 1, 2);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (tileit_exp_update),
                    &exp_call);

  exp_call.applybut = button;

  g_object_bind_property (toggle,     "active",
                          spinbutton, "sensitive",
                          G_BINDING_SYNC_CREATE);

  /* Widget for selecting the Opacity */

  scale = gimp_scale_entry_new (_("O_pacity:"), opacity, 0, 100, 0);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (tileit_scale_update),
                    &opacity);

  gtk_widget_show (scale);
  gtk_widget_set_sensitive (scale, gimp_drawable_has_alpha (drawable));
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 6);

  /* Lower frame saying how many segments */
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog), "segments-label",
                                   _("Number of Segments"), FALSE, FALSE);
  scale = gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                                 "num-tiles", 1);
  g_signal_connect_object (config, "notify::num-tiles",
                           G_CALLBACK (tileit_config_update),
                           NULL, 0);

  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "num-tiles-frame",
                                    "segments-label", FALSE,
                                    "num-tiles");
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), "num-tiles-frame",
                              NULL);

  gtk_widget_show (dialog);

  g_object_get (config,
                "num-tiles", &itvals.numtiles,
                NULL);
  dialog_update_preview ();

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static void
tileit_hvtoggle_update (GtkWidget *widget,
                        gpointer   data)
{
  gimp_toggle_button_update (widget, data);

  switch (exp_call.type)
    {
    case ALL:
      /* Clear current settings */
      memset (tileactions, 0, sizeof (tileactions));
      all_update ();
      break;

    case ALT:
      /* Clear current settings */
      memset (tileactions, 0, sizeof (tileactions));
      alt_update ();
      break;

    case EXPLICIT:
      break;
    }

  dialog_update_preview ();
}

static gboolean
tileit_preview_draw (GtkWidget *widget,
                     cairo_t   *cr)
{
  if (exp_call.type == EXPLICIT)
    {
      gdouble width  = (gdouble) preview_width / (gdouble) itvals.numtiles;
      gdouble height = (gdouble) preview_height / (gdouble) itvals.numtiles;
      gdouble x , y;

      x = width  * (exp_call.x - 1);
      y = height * (exp_call.y - 1);

      cairo_rectangle (cr, x + 1.5, y + 1.5, width - 2, height - 2);

      cairo_set_line_width (cr, 3.0);
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
      cairo_stroke_preserve (cr);

      cairo_set_line_width (cr, 1.0);
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
      cairo_stroke_preserve (cr);
    }

  return FALSE;
}

static void
exp_need_update (gint nx,
                 gint ny)
{
  if (nx <= 0 || nx > itvals.numtiles || ny <= 0 || ny > itvals.numtiles)
    return;

  if (nx != exp_call.x || ny != exp_call.y)
    {
      exp_call.x = nx;
      exp_call.y = ny;
      gtk_widget_queue_draw (tint.preview);

      g_signal_handlers_block_by_func (exp_call.c_adj,
                                       tileit_exp_update_f,
                                       &exp_call);
      g_signal_handlers_block_by_func (exp_call.r_adj,
                                       tileit_exp_update_f,
                                       &exp_call);

      gtk_adjustment_set_value (exp_call.c_adj, nx);
      gtk_adjustment_set_value (exp_call.r_adj, ny);

      g_signal_handlers_unblock_by_func (exp_call.c_adj,
                                         tileit_exp_update_f,
                                         &exp_call);
      g_signal_handlers_unblock_by_func (exp_call.r_adj,
                                         tileit_exp_update_f,
                                         &exp_call);
    }
}

static gboolean
tileit_preview_events (GtkWidget *widget,
                       GdkEvent  *event)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint nx, ny;
  gint twidth  = preview_width / itvals.numtiles;
  gint theight = preview_height / itvals.numtiles;

  switch (event->type)
    {
    case GDK_EXPOSE:
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      nx = bevent->x/twidth + 1;
      ny = bevent->y/theight + 1;
      exp_need_update (nx, ny);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      if ( !mevent->state )
        break;
      if (mevent->x < 0 || mevent->y < 0)
        break;
      nx = mevent->x/twidth + 1;
      ny = mevent->y/theight + 1;
      exp_need_update (nx, ny);
      break;

    default:
      break;
    }

  return FALSE;
}

static void
explicit_update (gboolean settile)
{
  gint x,y;

  /* Make sure bounds are OK */
  y = ROUND (gtk_adjustment_get_value (exp_call.r_adj));
  if (y > itvals.numtiles || y <= 0)
    {
      y = itvals.numtiles;
    }
  x = ROUND (gtk_adjustment_get_value (exp_call.c_adj));
  if (x > itvals.numtiles || x <= 0)
    {
      x = itvals.numtiles;
    }

  /* Set it */
  if (settile)
    tileactions[x-1][y-1] = (((do_horz) ? HORIZONTAL : 0) |
                             ((do_vert) ? VERTICAL : 0));

  exp_call.x = x;
  exp_call.y = y;
}

static void
all_update (void)
{
  gint x,y;

  for (x = 0 ; x < MAX_SEGS; x++)
    for (y = 0 ; y < MAX_SEGS; y++)
      tileactions[x][y] |= (((do_horz) ? HORIZONTAL : 0) |
                            ((do_vert) ? VERTICAL : 0));
}

static void
alt_update (void)
{
  gint x,y;

  for (x = 0 ; x < MAX_SEGS; x++)
    for (y = 0 ; y < MAX_SEGS; y++)
      if (!((x + y) % 2))
        tileactions[x][y] |= (((do_horz) ? HORIZONTAL : 0) |
                              ((do_vert) ? VERTICAL : 0));
}

static void
tileit_radio_update (GtkWidget *widget,
                     gpointer   data)
{
  gimp_radio_button_update (widget, data);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      switch (exp_call.type)
        {
        case ALL:
          /* Clear current settings */
          memset (tileactions, 0, sizeof (tileactions));
          all_update ();
          break;

        case ALT:
          /* Clear current settings */
          memset (tileactions, 0, sizeof (tileactions));
          alt_update ();
          break;

        case EXPLICIT:
          explicit_update (FALSE);
          break;
        }

      dialog_update_preview ();
    }
}


static void
tileit_scale_update (GimpLabelSpin *scale,
                     gint          *value)
{
  *value = RINT (gimp_label_spin_get_value (scale));

  dialog_update_preview ();
}

static void
tileit_config_update (GimpProcedureConfig *config)
{
  g_object_get (config,
                "num-tiles", &itvals.numtiles,
                NULL);

  dialog_update_preview ();
}

static void
tileit_reset (GtkWidget *widget,
              gpointer   data)
{
  Reset_Call *r = (Reset_Call *) data;

  memset (tileactions, 0, sizeof (tileactions));

  g_signal_handlers_block_by_func (r->htoggle,
                                   tileit_hvtoggle_update,
                                   &do_horz);
  g_signal_handlers_block_by_func (r->vtoggle,
                                   tileit_hvtoggle_update,
                                   &do_vert);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (r->htoggle), FALSE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (r->vtoggle), FALSE);

  g_signal_handlers_unblock_by_func (r->htoggle,
                                     tileit_hvtoggle_update,
                                     &do_horz);
  g_signal_handlers_unblock_by_func (r->vtoggle,
                                     tileit_hvtoggle_update,
                                     &do_vert);

  do_horz = do_vert = FALSE;

  dialog_update_preview ();
}


/* Could avoid almost dup. functions by using a field in the data
 * passed.  Must still pass the data since used in sig blocking func.
 */

static void
tileit_exp_update (GtkWidget *widget,
                   gpointer   applied)
{
  explicit_update (TRUE);
  dialog_update_preview ();
}

static void
tileit_exp_update_f (GtkWidget *widget,
                     gpointer   applied)
{
  explicit_update (FALSE);
  dialog_update_preview ();
}

/* Cache the preview image - updates are a lot faster. */
/* The preview_cache will contain the small image */

static void
cache_preview (GimpDrawable *drawable)
{
  GeglBuffer *buffer = gimp_drawable_get_buffer (drawable);
  const Babl *format;
  gdouble     scale;

  if (gimp_drawable_has_alpha (drawable))
    format = babl_format ("R'G'B'A u8");
  else
    format = babl_format ("R'G'B' u8");

  tint.img_bpp = babl_format_get_bytes_per_pixel (format);

  tint.pv_cache = g_new (guchar, preview_width * preview_height * 4);

  scale = MIN ((gdouble) preview_width  / (gdouble) sel_width,
               (gdouble) preview_height / (gdouble) sel_height);

  gegl_buffer_get (buffer,
                   GEGL_RECTANGLE (scale * sel_x1, scale * sel_y1,
                                   preview_width, preview_height),
                   scale,
                   format, tint.pv_cache,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  g_object_unref (buffer);
}

static void
do_tiles (GimpProcedureConfig *config,
          GimpDrawable        *drawable)
{
  GeglBuffer         *src_buffer;
  GeglBuffer         *dest_buffer;
  GeglBufferIterator *iter;
  const Babl         *format;
  gboolean            has_alpha;
  gint                progress, max_progress;
  gint                bpp;
  guchar              pixel[4];
  gint                nc, nr;
  gint                i;

  src_buffer  = gimp_drawable_get_buffer (drawable);
  dest_buffer = gimp_drawable_get_shadow_buffer (drawable);

  has_alpha = gimp_drawable_has_alpha (drawable);

  if (has_alpha)
    format = babl_format ("R'G'B'A u8");
  else
    format = babl_format ("R'G'B' u8");

  bpp = babl_format_get_bytes_per_pixel (format);

  progress     = 0;
  max_progress = sel_width * sel_height;

  iter = gegl_buffer_iterator_new (dest_buffer,
                                   GEGL_RECTANGLE (sel_x1, sel_y1,
                                                   sel_width, sel_height), 0,
                                   format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);


  while (gegl_buffer_iterator_next (iter))
    {
      GeglRectangle  dest_roi = iter->items[0].roi;
      guchar        *dest_row = iter->items[0].data;
      gint           row;

      for (row = dest_roi.y; row < (dest_roi.y + dest_roi.height); row++)
        {
          guchar *dest = dest_row;
          gint    col;

          for (col = dest_roi.x; col < (dest_roi.x + dest_roi.width); col++)
            {
              tiles_xy (config,
                        sel_width,
                        sel_height,
                        col - sel_x1,
                        row - sel_y1,
                        &nc, &nr);

              gegl_buffer_sample (src_buffer, nc + sel_x1, nr + sel_y1, NULL,
                                  pixel, format,
                                  GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

              for (i = 0; i < bpp; i++)
                dest[i] = pixel[i];

              if (has_alpha)
                dest[bpp - 1] = (pixel[bpp - 1] * opacity) / 100;

              dest += bpp;
            }

          dest_row += dest_roi.width * bpp;
        }

      progress += dest_roi.width * dest_roi.height;
      gimp_progress_update ((double) progress / max_progress);
    }

  gimp_progress_update (1.0);

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  gimp_drawable_merge_shadow (drawable, TRUE);
  gimp_drawable_update (drawable,
                        sel_x1, sel_y1, sel_width, sel_height);
}


/* Get the xy pos and any action */
static gint
tiles_xy (GimpProcedureConfig *config,
          gint                 width,
          gint                 height,
          gint                 x,
          gint                 y,
          gint                *nx,
          gint                *ny)
{
  gint    px,py;
  gint    rnum,cnum;
  gint    actiontype;
  gint    num_tiles;
  gdouble rnd;

  g_object_get (config,
                "num-tiles", &num_tiles,
                NULL);

  rnd = 1 - (1.0 / (gdouble) num_tiles) + 0.01;

  rnum = y * num_tiles / height;

  py   = (y * num_tiles) % height;
  px   = (x * num_tiles) % width;
  cnum = x * num_tiles / width;

  if ((actiontype = tileactions[cnum][rnum]))
    {
      if (actiontype & VERTICAL)
        {
          gdouble pyr;

          pyr =  height - y - 1 + rnd;
          py = ((gint) (pyr * (gdouble) num_tiles)) % height;
        }

      if (actiontype & HORIZONTAL)
        {
          gdouble pxr;

          pxr = width - x - 1 + rnd;
          px = ((gint) (pxr * (gdouble) num_tiles)) % width;
        }
    }

  *nx = px;
  *ny = py;

  return (actiontype);
}


/* Given a row then shrink it down a bit */
static void
do_tiles_preview (guchar *dest_row,
                  guchar *src_rows,
                  gint    width,
                  gint    dh,
                  gint    height,
                  gint    bpp)
{
  gint    x;
  gint    i;
  gint    px, py;
  gint    rnum,cnum;
  gint    actiontype;
  gdouble rnd = 1 - (1.0 / (gdouble) itvals.numtiles) + 0.01;

  rnum = dh * itvals.numtiles / height;

  for (x = 0; x < width; x ++)
    {

      py = (dh*itvals.numtiles)%height;

      px = (x*itvals.numtiles)%width;
      cnum = x*itvals.numtiles/width;

      if ((actiontype = tileactions[cnum][rnum]))
        {
          if (actiontype & VERTICAL)
            {
              gdouble pyr;
              pyr =  height - dh - 1 + rnd;
              py = ((int)(pyr*(gdouble)itvals.numtiles))%height;
            }

          if (actiontype & HORIZONTAL)
            {
              gdouble pxr;
              pxr = width - x - 1 + rnd;
              px = ((int)(pxr*(gdouble)itvals.numtiles))%width;
            }
        }

      for (i = 0 ; i < bpp; i++ )
        dest_row[x*tint.img_bpp+i] =
          src_rows[(px + (py*width))*bpp+i];

      if (has_alpha)
        dest_row[x*tint.img_bpp + (bpp - 1)] =
          (dest_row[x*tint.img_bpp + (bpp - 1)]*opacity)/100;

    }
}

static void
dialog_update_preview (void)
{
  gint    y;
  guchar *buffer;

  buffer = g_new (guchar, preview_width * preview_height * tint.img_bpp);

  for (y = 0; y < preview_height; y++)
    {
      do_tiles_preview (tint.preview_row,
                        tint.pv_cache,
                        preview_width,
                        y,
                        preview_height,
                        tint.img_bpp);

      memcpy (buffer + y* (preview_width * tint.img_bpp),
              tint.preview_row,
              preview_width * tint.img_bpp);
    }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (tint.preview),
                          0, 0, preview_width, preview_height,
                          (tint.img_bpp>3)?GIMP_RGBA_IMAGE:GIMP_RGB_IMAGE,
                          buffer,
                          preview_width * tint.img_bpp);

  g_free (buffer);

  gtk_widget_queue_draw (tint.preview);
}
