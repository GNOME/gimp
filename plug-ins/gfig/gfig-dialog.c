/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Generates images containing vector type drawings.
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
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>

#ifdef G_OS_WIN32
#include <libgimpbase/gimpwin32-io.h>
#endif

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "gfig.h"
#include "gfig-style.h"
#include "gfig-dialog.h"
#include "gfig-arc.h"
#include "gfig-bezier.h"
#include "gfig-circle.h"
#include "gfig-dobject.h"
#include "gfig-ellipse.h"
#include "gfig-grid.h"
#include "gfig-icons.h"
#include "gfig-line.h"
#include "gfig-poly.h"
#include "gfig-preview.h"
#include "gfig-rectangle.h"
#include "gfig-spiral.h"
#include "gfig-star.h"

#define SEL_BUTTON_WIDTH  100
#define SEL_BUTTON_HEIGHT  20

#define GRID_TYPE_MENU      1
#define GRID_RENDER_MENU    2

#define PAINT_BGS_MENU      2
#define PAINT_TYPE_MENU     3

#define OBJ_SELECT_GT       1
#define OBJ_SELECT_LT       2
#define OBJ_SELECT_EQ       4

#define UPDATE_DELAY 300 /* From GtkRange in GTK+ 2.22 */

/* Globals */
gint   undo_level;  /* Last slot filled in -1 = no undo */
GList *undo_table[MAX_UNDO];

/* Values when first invoked */
SelectItVals selvals =
{
  {
    MIN_GRID + (MAX_GRID - MIN_GRID)/2, /* Gridspacing     */
    RECT_GRID,            /* Default to rectangle type     */
    FALSE,                /* drawgrid                      */
    FALSE,                /* snap2grid                     */
    FALSE,                /* lockongrid                    */
    TRUE,                 /* show control points           */
    0.0,                  /* grid_radius_min               */
    10.0,                 /* grid_radius_interval          */
    0.0,                  /* grid_rotation                 */
    5.0,                  /* grid_granularity              */
    120                   /* grid_sectors_desired          */
  },
  FALSE,                  /* show image                    */
  MIN_UNDO + (MAX_UNDO - MIN_UNDO)/2,  /* Max level of undos */
  TRUE,                   /* Show pos updates              */
  0.0,                    /* Brush fade                    */
  0.0,                    /* Brush gradient                */
  20.0,                   /* Air brush pressure            */
  ORIGINAL_LAYER,         /* Draw all objects on one layer */
  LAYER_TRANS_BG,         /* New layers background         */
  PAINT_BRUSH_TYPE,       /* Default to use brushes        */
  FALSE,                  /* reverse lines                 */
  TRUE,                   /* Scale to image when painting  */
  1.0,                    /* Scale to image fp             */
  BRUSH_BRUSH_TYPE,       /* Default to use a brush        */
  LINE                    /* Initial object type           */
};

selection_option selopt =
{
  GIMP_CHANNEL_OP_ADD, /* type */
  FALSE,               /* Antia */
  FALSE,               /* Feather */
  10.0,                /* feather radius */
  ARC_SEGMENT,         /* Arc as a segment */
  FILL_PATTERN,        /* Fill as pattern */
  100.0,               /* Max opacity */
};

/* Should be kept in sync with GfigOpts */
typedef struct
{
  GtkWidget *gridspacing;
  GtkWidget *grid_sectors_desired;
  GtkWidget *grid_radius_interval;
  GtkWidget *gridtypemenu;
  GtkWidget *drawgrid;
  GtkWidget *snap2grid;
  GtkWidget *lockongrid;
  GtkWidget *showcontrol;
} GfigOptWidgets;

static GfigOptWidgets  gfig_opt_widget = { NULL, NULL, NULL, NULL, NULL, NULL };
static gchar          *gfig_path       = NULL;
static GtkWidget      *page_menu_bg;
static GtkWidget      *tool_options_notebook;
static GtkWidget      *fill_type_notebook;
static guint           paint_timeout   = 0;

static void       shape_change_state         (GSimpleAction *action,
                                              GVariant      *new_state,
                                              gpointer       user_data);

static void       gfig_response              (GtkWidget *widget,
                                              gint       response_id,
                                              gpointer   data);
static void       gfig_load_action           (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data);
static void       gfig_save_action           (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data);
static void       gfig_list_load_all         (const gchar *path);
static void       gfig_list_free_all         (void);
static void       create_notebook_pages      (GtkWidget *notebook);
static void       select_filltype_callback   (GtkWidget *widget);
static void       gfig_close_action          (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data);
static void       gfig_undo_action           (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data);
static void       gfig_clear_action          (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data);
static void       gfig_grid_action           (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data);
static void       gfig_prefs_action          (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data);
static void       toggle_show_image          (void);
static void       gridtype_combo_callback    (GtkWidget *widget,
                                              gpointer data);

static void      load_file_chooser_response  (GtkFileChooser *chooser,
                                              gint            response_id,
                                              gpointer        data);
static void      save_file_chooser_response  (GtkFileChooser *chooser,
                                              gint            response_id,
                                              GFigObj        *obj);
static void     paint_combo_callback         (GtkWidget *widget,
                                              gpointer   data);

static void     select_button_clicked        (gint       type);
static void     select_button_clicked_lt     (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data);
static void     select_button_clicked_gt     (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data);
static void     select_button_clicked_eq     (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data);
static void     raise_selected_obj_to_top    (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data);
static void     lower_selected_obj_to_bottom (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data);
static void     raise_selected_obj           (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data);
static void     lower_selected_obj           (GSimpleAction *action,
                                              GVariant      *parameter,
                                              gpointer       user_data);

static void     toggle_obj_type              (DobjType   new_type);

static void   gfig_scale_entry_update_double (GimpLabelSpin  *entry,
                                              gdouble        *value);
static void   gfig_scale_entry_update_int    (GimpLabelSpin  *entry,
                                              gint           *value);

/* GAction helper methods */
static GtkWidget * add_tool_button            (GtkWidget     *toolbar,
                                               const char    *action,
                                               const char    *icon,
                                               const char    *label,
                                               const char    *tooltip);
static GtkWidget * add_toggle_button          (GtkWidget     *toolbar,
                                               const char    *action,
                                               const char    *icon,
                                               const char    *label,
                                               const char    *tooltip);
static void        add_tool_separator         (GtkWidget     *toolbar,
                                               gboolean       expand);

static const GActionEntry ACTIONS[] =
{
  /* Sub-menu options */
  { "open", gfig_load_action },
  { "save", gfig_save_action },
  { "close", gfig_close_action },
  { "undo", gfig_undo_action },
  { "clear", gfig_clear_action },
  { "grid", gfig_grid_action },
  { "preferences", gfig_prefs_action },

  /* Toolbar buttons */
  { "raise", raise_selected_obj },
  { "lower", lower_selected_obj },
  { "top", raise_selected_obj_to_top },
  { "bottom", lower_selected_obj_to_bottom },
  { "show-prev", select_button_clicked_lt },
  { "show-next", select_button_clicked_gt },
  { "show-all", select_button_clicked_eq },

  /* RadioButtons - only the default state is shown here. */
  { "shape", shape_change_state, "s", "'line'", NULL },
};

gboolean
gfig_dialog (GimpGfig *gfig)
{
  GAction      *action;
  GtkWidget    *main_hbox;
  GtkWidget    *vbox;
  GFigObj      *gfig_obj;
  GimpParasite *parasite;
  GimpLayer    *newlayer;
  GtkWidget    *menubar;
  GMenuModel   *model;
  GtkWidget    *toolbar;
  GtkWidget    *combo;
  GtkWidget    *frame;
  gint          img_width;
  gint          img_height;
  GimpImageType img_type;
  GtkWidget    *toggle;
  GtkWidget    *right_vbox;
  GtkWidget    *hbox;
  GtkWidget    *empty_label;
  gchar        *path;

  gimp_ui_init (PLUG_IN_BINARY);

  img_width  = gimp_drawable_get_width (gfig_context->drawable);
  img_height = gimp_drawable_get_height (gfig_context->drawable);
  img_type   = gimp_drawable_type_with_alpha (gfig_context->drawable);

  /*
   * See if there is a "gfig" parasite.  If so, this is a gfig layer,
   * and we start by clearing it to transparent.
   * If not, we create a new transparent layer.
   */
  gfig_list = NULL;
  undo_level = -1;
  parasite = gimp_item_get_parasite (GIMP_ITEM (gfig_context->drawable), "gfig");
  gfig_context->enable_repaint = FALSE;

  /* debug */
  gfig_context->debug_styles = FALSE;

  /* initial default style */
  gfig_read_gimp_style (&gfig_context->default_style, "Base");
  gfig_context->default_style.paint_type = selvals.painttype;

  if (parasite)
    {
      gimp_drawable_fill (gfig_context->drawable, GIMP_FILL_TRANSPARENT);
      gfig_context->using_new_layer = FALSE;
      gimp_parasite_free (parasite);
    }
  else
    {
      newlayer = gimp_layer_new (gfig_context->image, "GFig",
                                 img_width, img_height,
                                 img_type,
                                 100.0,
                                 gimp_image_get_default_new_layer_mode (gfig_context->image));
      gimp_drawable_fill (GIMP_DRAWABLE (newlayer), GIMP_FILL_TRANSPARENT);
      gimp_image_insert_layer (gfig_context->image, newlayer, NULL, -1);
      gfig_context->drawable = GIMP_DRAWABLE (newlayer);
      gfig_context->using_new_layer = TRUE;
    }

  gfig_icons_init ();

  path = gimp_gimprc_query ("gfig-path");

  if (path)
    {
      gfig_path = g_filename_from_utf8 (path, -1, NULL, NULL, NULL);
      g_free (path);
    }
  else
    {
      GFile *gimprc    = gimp_directory_file ("gimprc", NULL);
      gchar *full_path = gimp_config_build_data_path ("gfig");
      gchar *esc_path  = g_strescape (full_path, NULL);
      g_free (full_path);

      g_message (_("No %s in gimprc:\n"
                   "You need to add an entry like\n"
                   "(%s \"%s\")\n"
                   "to your %s file."),
                   "gfig-path", "gfig-path", esc_path,
                   gimp_file_get_utf8_name (gimprc));

      g_object_unref (gimprc);
      g_free (esc_path);
    }

  /* Start building the dialog up */
  gfig->top_level_dlg = gimp_dialog_new (_("Gfig"), PLUG_IN_ROLE,
                                         NULL, 0,
                                         gimp_standard_help_func, PLUG_IN_PROC,

                                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                                         _("_Close"),  GTK_RESPONSE_OK,

                                         NULL);

  gtk_window_set_application (GTK_WINDOW (gfig->top_level_dlg), gfig->app);
  gimp_window_set_transient (GTK_WINDOW (gfig->top_level_dlg));

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (gfig->top_level_dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (gfig->top_level_dlg, "response",
                    G_CALLBACK (gfig_response),
                    gfig);

  /* build the menu */
  g_action_map_add_action_entries (G_ACTION_MAP (gfig->app),
                                   ACTIONS, G_N_ELEMENTS (ACTIONS),
                                   gfig);

  model = G_MENU_MODEL (gtk_builder_get_object (gfig->builder, "gfig-menubar"));
  menubar = gtk_menu_bar_new_from_model (model);

  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (gfig->top_level_dlg))),
                      menubar, FALSE, FALSE, 0);
  gtk_widget_show (menubar);

  toolbar = gtk_toolbar_new ();
  add_toggle_button (toolbar, "app.shape::line", GFIG_ICON_LINE,
                     _("Line"), _("Create line"));
  add_toggle_button (toolbar, "app.shape::rectangle", GFIG_ICON_RECTANGLE,
                   _("Rectangle"), _("Create rectangle"));
  add_toggle_button (toolbar, "app.shape::circle", GFIG_ICON_CIRCLE,
                     _("Circle"), _("Create circle"));
  add_toggle_button (toolbar, "app.shape::ellipse", GFIG_ICON_ELLIPSE,
                     _("Ellipse"), _("Create ellipse"));
  add_toggle_button (toolbar, "app.shape::arc", GFIG_ICON_CURVE,
                     _("Arc"), _("Create arc"));
  add_toggle_button (toolbar, "app.shape::polygon", GFIG_ICON_POLYGON,
                     _("Polygon"), _("Create reg polygon"));
  add_toggle_button (toolbar, "app.shape::star", GFIG_ICON_STAR,
                     _("Star"), _("Create star"));
  add_toggle_button (toolbar, "app.shape::spiral", GFIG_ICON_SPIRAL,
                     _("Spiral"), _("Create spiral"));
  add_toggle_button (toolbar, "app.shape::bezier", GFIG_ICON_BEZIER,
                     _("Bezier"), _("Create bezier curve. "
                                    "Shift + Button ends object creation."));
  add_toggle_button (toolbar, "app.shape::move-obj", GFIG_ICON_MOVE_OBJECT,
                     _("Move Object"), _("Move an object"));
  add_toggle_button (toolbar, "app.shape::move-point", GFIG_ICON_MOVE_POINT,
                     _("Move Point"), _("Move a single point"));
  add_toggle_button (toolbar, "app.shape::copy", GFIG_ICON_COPY_OBJECT,
                     _("Copy"), _("Copy an object"));
  add_toggle_button (toolbar, "app.shape::delete", GFIG_ICON_DELETE_OBJECT,
                     _("Delete"), _("Delete an object"));
  add_toggle_button (toolbar, "app.shape::select", GIMP_ICON_CURSOR,
                     _("Select"), _("Select an object"));
  add_tool_separator (toolbar, FALSE);
  add_tool_button (toolbar, "app.raise", GIMP_ICON_GO_UP,
                   _("Raise"), _("Raise selected object"));
  add_tool_button (toolbar, "app.lower", GIMP_ICON_GO_DOWN,
                   _("Lower"), _("Lower selected object"));
  add_tool_button (toolbar, "app.raise", GIMP_ICON_GO_TOP,
                   _("To Top"), _("Raise selected object to top"));
  add_tool_button (toolbar, "app.lower", GIMP_ICON_GO_BOTTOM,
                   _("To Bottom"), _("Lower selected object to bottom"));
  add_tool_separator (toolbar, FALSE);
  add_tool_button (toolbar, "app.show-prev", GIMP_ICON_GO_PREVIOUS,
                   _("Show Previous"), _("Show previous object"));
  add_tool_button (toolbar, "app.show-next", GIMP_ICON_GO_NEXT,
                   _("Show Next"), _("Show next object"));
  add_tool_button (toolbar, "app.show-all", GFIG_ICON_SHOW_ALL,
                   _("Show All"), _("Show all objects"));

  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (gfig->top_level_dlg))),
                      toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  action = g_action_map_lookup_action (G_ACTION_MAP (gfig->app), "undo");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                               undo_level >= 0);

  /* Main box */
  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (gfig->top_level_dlg))),
                    main_hbox, TRUE, TRUE, 0);

  /* Preview itself */
  gtk_box_pack_start (GTK_BOX (main_hbox), make_preview (gfig), FALSE, FALSE, 0);

  gtk_widget_show (gfig_context->preview);

  right_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_hexpand (right_vbox, FALSE);
  gtk_widget_set_halign (right_vbox, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (main_hbox), right_vbox, FALSE, FALSE, 0);
  gtk_widget_show (right_vbox);

  /* Tool options notebook */
  frame = gimp_frame_new ( _("Tool Options"));
  gtk_box_pack_start (GTK_BOX (right_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  tool_options_notebook = gtk_notebook_new ();
  gtk_container_add (GTK_CONTAINER (frame), tool_options_notebook);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (tool_options_notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (tool_options_notebook), FALSE);
  gtk_widget_show (tool_options_notebook);
  create_notebook_pages (tool_options_notebook);

  /* Stroke frame on right side */
  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (right_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gfig_context->paint_type_toggle =
    toggle = gtk_check_button_new_with_mnemonic (_("_Stroke"));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), selvals.painttype);
  gtk_frame_set_label_widget (GTK_FRAME (frame), toggle);
  gtk_widget_show (toggle);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  gtk_widget_set_sensitive (vbox, selvals.painttype);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (set_paint_type_callback),
                    vbox);

  /* foreground color button in Stroke frame*/
  gfig_context->fg_color = gegl_color_duplicate (gfig_context->default_style.foreground);
  gfig_context->fg_color_button = gimp_color_button_new (_("Foreground"),
                                                         SEL_BUTTON_WIDTH,
                                                         SEL_BUTTON_HEIGHT,
                                                         gfig_context->fg_color,
                                                         GIMP_COLOR_AREA_SMALL_CHECKS);
  g_signal_connect (gfig_context->fg_color_button, "color-changed",
                    G_CALLBACK (set_foreground_callback),
                    gfig_context->fg_color);
  gtk_box_pack_start (GTK_BOX (vbox), gfig_context->fg_color_button,
                      FALSE, FALSE, 0);
  gtk_widget_show (gfig_context->fg_color_button);

  /* brush selector in Stroke frame */
  gfig_context->brush_select
    = gimp_brush_chooser_new (_("Brush"), _("Brush"),
                              gfig_context->default_style.brush);
  g_signal_connect (gfig_context->brush_select, "resource-set",
                    G_CALLBACK (gfig_brush_changed_callback), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), gfig_context->brush_select,
                      FALSE, FALSE, 0);
  gtk_widget_show (gfig_context->brush_select);

  /* Fill frame on right side */
  frame = gimp_frame_new (_("Fill"));
  gtk_box_pack_start (GTK_BOX (right_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /* fill style combo box in Style frame  */
  gfig_context->fillstyle_combo = combo
    = gimp_int_combo_box_new (_("No fill"),             FILL_NONE,
                              _("Color fill"),          FILL_COLOR,
                              _("Pattern fill"),        FILL_PATTERN,
                              _("Shape gradient"),      FILL_GRADIENT,
                              _("Vertical gradient"),   FILL_VERTICAL,
                              _("Horizontal gradient"), FILL_HORIZONTAL,
                              NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), 0);
  g_signal_connect (combo, "changed",
                    G_CALLBACK (select_filltype_callback),
                    NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  fill_type_notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (vbox), fill_type_notebook, FALSE, FALSE, 0);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (fill_type_notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (fill_type_notebook), FALSE);
  gtk_widget_show (fill_type_notebook);

  /* An empty page for "No fill" */
  empty_label = gtk_label_new ("");
  gtk_widget_show (empty_label);
  gtk_notebook_append_page (GTK_NOTEBOOK (fill_type_notebook),
                            empty_label, NULL);

  /* A page for the fill color button */
  gfig_context->bg_color = gegl_color_duplicate (gfig_context->default_style.background);
  gfig_context->bg_color_button = gimp_color_button_new (_("Background"),
                                                         SEL_BUTTON_WIDTH, SEL_BUTTON_HEIGHT,
                                                         gfig_context->bg_color,
                                                         GIMP_COLOR_AREA_SMALL_CHECKS);
  g_signal_connect (gfig_context->bg_color_button, "color-changed",
                    G_CALLBACK (set_background_callback),
                    gfig_context->bg_color);
  gtk_widget_show (gfig_context->bg_color_button);
  gtk_notebook_append_page (GTK_NOTEBOOK (fill_type_notebook),
                            gfig_context->bg_color_button, NULL);

  /* A page for the pattern selector */
  gfig_context->pattern_select
    = gimp_pattern_chooser_new (_("Pattern"), _("Pattern"),
                                gfig_context->default_style.pattern);
  g_signal_connect (gfig_context->pattern_select, "resource-set",
                    G_CALLBACK (gfig_pattern_changed_callback), NULL);
  gtk_widget_show (gfig_context->pattern_select);
  gtk_notebook_append_page (GTK_NOTEBOOK (fill_type_notebook),
                            gfig_context->pattern_select, NULL);

  /* A page for the gradient selector */
  gfig_context->gradient_select
    = gimp_gradient_chooser_new (_("Gradient"), _("Gradient"),
                                 gfig_context->default_style.gradient);
  g_signal_connect (gfig_context->gradient_select, "resource-set",
                    G_CALLBACK (gfig_gradient_changed_callback), NULL);
  gtk_widget_show (gfig_context->gradient_select);
  gtk_notebook_append_page (GTK_NOTEBOOK (fill_type_notebook),
                            gfig_context->gradient_select, NULL);


  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (right_vbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /* "show image" checkbutton at bottom of style frame */
  toggle = gtk_check_button_new_with_label (_("Show image"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                gfig_context->show_background);
  gtk_box_pack_end (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &gfig_context->show_background);
  g_signal_connect_swapped (toggle, "toggled",
                    G_CALLBACK (gtk_widget_queue_draw),
                    gfig_context->preview);
  gtk_widget_show (toggle);

  /* "snap to grid" checkbutton at bottom of style frame */
  toggle = gtk_check_button_new_with_label (C_("checkbutton", "Snap to grid"));
  gtk_box_pack_end (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &selvals.opts.snap2grid);
  gtk_widget_show (toggle);
  gfig_opt_widget.snap2grid = toggle;

  /* "show grid" checkbutton at bottom of style frame */
  toggle = gtk_check_button_new_with_label (_("Show grid"));
  gtk_box_pack_end (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &selvals.opts.drawgrid);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (draw_grid_clear),
                    NULL);
  gtk_widget_show (toggle);
  gfig_opt_widget.drawgrid = toggle;

  /* Load saved objects */
  gfig_list_load_all (gfig_path);

  /* Setup initial brush settings */
  if (gimp_context_get_brush ())
    set_context_bdesc (gimp_context_get_brush ());

  gtk_widget_show (main_hbox);

  gtk_widget_show (gfig->top_level_dlg);

  gfig_obj = gfig_load_from_parasite (gfig);
  if (gfig_obj)
    {
      gfig_list_insert (gfig_obj);
      new_obj_2edit (gfig, gfig_obj);
      gfig_style_set_context_from_style (&gfig_context->default_style);
      gfig_style_apply (&gfig_context->default_style);
    }

  gfig_context->enable_repaint = TRUE;
  gfig_paint_callback ();

  /* FIXME */
  return TRUE;
}

static void shape_change_state (GSimpleAction *action,
                                GVariant      *new_state,
                                gpointer       user_data)
{
  gchar    *str;
  DobjType  new_type = LINE;

  str = g_strdup_printf ("%s",  g_variant_get_string (new_state, NULL));

  if (! strcmp (str, "line"))
    new_type = LINE;
  else if (! strcmp (str, "rectangle"))
    new_type = RECTANGLE;
  else if (! strcmp (str, "circle"))
    new_type = CIRCLE;
  else if (! strcmp (str, "ellipse"))
    new_type = ELLIPSE;
  else if (! strcmp (str, "arc"))
    new_type = ARC;
  else if (! strcmp (str, "polygon"))
    new_type = POLY;
  else if (! strcmp (str, "star"))
    new_type = STAR;
  else if (! strcmp (str, "spiral"))
    new_type = SPIRAL;
  else if (! strcmp (str, "bezier"))
    new_type = BEZIER;
  else if (! strcmp (str, "move-obj"))
    new_type = MOVE_OBJ;
  else if (! strcmp (str, "move-point"))
    new_type = MOVE_POINT;
  else if (! strcmp (str, "copy"))
    new_type = COPY_OBJ;
  else if (! strcmp (str, "delete"))
    new_type = DEL_OBJ;
  else if (! strcmp (str, "select"))
    new_type = SELECT_OBJ;

  g_free (str);

  g_simple_action_set_state (action, new_state);
  toggle_obj_type (new_type);
}

static void
gfig_response (GtkWidget *widget,
               gint       response_id,
               gpointer   data)
{
  GFigObj *gfig_obj;

  switch (response_id)
    {
    case GTK_RESPONSE_DELETE_EVENT:
    case GTK_RESPONSE_CANCEL:
      /* if we created a new layer, delete it */
      if (gfig_context->using_new_layer)
        {
          gimp_image_remove_layer (gfig_context->image,
                                   GIMP_LAYER (gfig_context->drawable));
        }
      else /* revert back to the original figure */
        {
          free_all_objs (gfig_context->current_obj->obj_list);
          gfig_context->current_obj->obj_list = NULL;
          gfig_obj = gfig_load_from_parasite (GIMP_GFIG (data));
          if (gfig_obj)
            {
              gfig_list_insert (gfig_obj);
              new_obj_2edit (GIMP_GFIG (data), gfig_obj);
            }
          gfig_context->enable_repaint = TRUE;
          gfig_paint_callback ();
        }
      break;

    case GTK_RESPONSE_OK:  /* Close button */
      gfig_save_as_parasite ();
      break;

    default:
      break;
    }

  gtk_application_remove_window ((GIMP_GFIG (data))->app,
                                 GTK_WINDOW ((GIMP_GFIG (data))->top_level_dlg));
}

static gchar *
gfig_get_user_writable_dir (void)
{
  if (gfig_path)
    {
      GList *list;
      gchar *dir;

      list = gimp_path_parse (gfig_path, 256, FALSE, NULL);
      dir = gimp_path_get_user_writable_dir (list);
      gimp_path_free (list);

      return dir;
    }

  return g_strdup (gimp_directory ());
}

static void
gfig_load_action (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
  static GtkWidget *dialog = NULL;

  if (! dialog)
    {
      gchar *dir;

      dialog =
        gtk_file_chooser_dialog_new (_("Load Gfig Object Collection"),
                                     NULL,
                                     GTK_FILE_CHOOSER_ACTION_OPEN,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Open"),   GTK_RESPONSE_OK,

                                     NULL);

      gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

      g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer) &dialog);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (load_file_chooser_response),
                        user_data);

      dir = gfig_get_user_writable_dir ();
      if (dir)
        {
          gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
                                               dir);
          g_free (dir);
        }

      gtk_widget_show (dialog);
    }
  else
    {
      gtk_window_present (GTK_WINDOW (dialog));
    }
}

static void
gfig_save_action (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
  static GtkWidget *dialog = NULL;

  if (!dialog)
    {
      gchar *dir;

      dialog =
        gtk_file_chooser_dialog_new (_("Save Gfig Drawing"),
                                     NULL,
                                     GTK_FILE_CHOOSER_ACTION_SAVE,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Save"),   GTK_RESPONSE_OK,

                                     NULL);

      gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);
      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                      TRUE);

      g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer) &dialog);

      /* FIXME: GFigObj should be a GObject and g_signal_connect_object()
       *        should be used here.
       */
      g_signal_connect (dialog, "response",
                        G_CALLBACK (save_file_chooser_response),
                        gfig_context->current_obj);

      dir = gfig_get_user_writable_dir ();
      if (dir)
        {
          gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), dir);
          g_free (dir);
        }

      gtk_widget_show (dialog);
    }
  else
    {
      gtk_window_present (GTK_WINDOW (dialog));
    }
}

static void
gfig_close_action (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
  GimpGfig *gfig = GIMP_GFIG (user_data);

  gtk_dialog_response (GTK_DIALOG (gfig->top_level_dlg), GTK_RESPONSE_OK);
}

static void
gfig_undo_action (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
  GAction  *action_undo;
  GimpGfig *gfig = GIMP_GFIG (user_data);

  if (undo_level >= 0)
    {
      /* Free current objects an reinstate previous */
      free_all_objs (gfig_context->current_obj->obj_list);
      gfig_context->current_obj->obj_list = NULL;
      tmp_bezier = tmp_line = obj_creating = NULL;
      gfig_context->current_obj->obj_list = undo_table[undo_level];
      undo_level--;
      /* Update the screen */
      gtk_widget_queue_draw (gfig_context->preview);
      /* And preview */
      gfig_context->current_obj->obj_status |= GFIG_MODIFIED;
      if (gfig_context->current_obj->obj_list)
        gfig_context->selected_obj = gfig_context->current_obj->obj_list->data;
      else
        gfig_context->selected_obj = NULL;
    }

  action_undo = g_action_map_lookup_action (G_ACTION_MAP (gfig->app), "undo");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action_undo),
                               undo_level >= 0);

  gfig_paint_callback ();
}

static void
gfig_clear_action (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
  /* Make sure we can get back - if we have some objects to get back to */
  if (!gfig_context->current_obj->obj_list)
    return;

  setup_undo (GIMP_GFIG (user_data));
  /* Free all objects */
  free_all_objs (gfig_context->current_obj->obj_list);
  gfig_context->current_obj->obj_list = NULL;
  gfig_context->selected_obj = NULL;
  obj_creating = NULL;
  tmp_line = NULL;
  tmp_bezier = NULL;
  gtk_widget_queue_draw (gfig_context->preview);
  gfig_paint_callback ();
}

void
draw_item (cairo_t *cr, gboolean fill)
{
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);

  if (fill)
    {
      cairo_fill_preserve (cr);
    }
  else
    {
      cairo_set_line_width (cr, 3.0);
      cairo_stroke_preserve (cr);
    }

  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
  cairo_set_line_width (cr, 1.0);
  cairo_stroke (cr);
}

/* Given a point x, y draw a circle */
void
draw_circle (GdkPoint *p,
             gboolean  selected,
             cairo_t  *cr)
{
  if (!selvals.opts.showcontrol)
    return;

  cairo_arc (cr,
             gfig_scale_x (p->x) + .5,
             gfig_scale_y (p->y) + .5,
             SQ_SIZE / 2,
             0, 2 * G_PI);

  draw_item (cr, selected);
}

/* Given a point x, y draw a square around it */
void
draw_sqr (GdkPoint *p,
          gboolean  selected,
          cairo_t  *cr)
{
  if (!selvals.opts.showcontrol)
    return;

  cairo_rectangle (cr,
                   gfig_scale_x (p->x) + .5 - SQ_SIZE / 2,
                   gfig_scale_y (p->y) + .5 - SQ_SIZE / 2,
                   SQ_SIZE,
                   SQ_SIZE);

  draw_item (cr, selected);
}

static void
gfig_list_load_all (const gchar *path)
{
  /*  Make sure to clear any existing gfigs  */
  gfig_context->current_obj = NULL;
  gfig_list_free_all ();

  if (! gfig_list)
    {
      GFigObj *gfig;

      /* lets have at least one! */
      gfig = gfig_new ();
      gfig->draw_name = g_strdup (_("First Gfig"));
      gfig_list_insert (gfig);
    }

  gfig_context->current_obj = gfig_list->data;  /* set to first entry */
}

static void
gfig_list_free_all (void)
{
  g_list_free_full (gfig_list, (GDestroyNotify) gfig_free);
  gfig_list = NULL;
}

static void
tool_option_no_option (GtkWidget *notebook)
{
  GtkWidget *label;

  label = gtk_label_new (_("This tool has no options"));
  gtk_widget_show (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), label, NULL);
}

static void
create_notebook_pages (GtkWidget *notebook)
{
  tool_option_no_option (notebook);   /* Line          */
  tool_option_no_option (notebook);   /* Rectangle     */
  tool_option_no_option (notebook);   /* Circle        */
  tool_option_no_option (notebook);   /* Ellipse       */
  tool_option_no_option (notebook);   /* Arc           */
  tool_options_poly (notebook);       /* Polygon       */
  tool_options_star (notebook);       /* Star          */
  tool_options_spiral (notebook);     /* Spiral        */
  tool_options_bezier (notebook);     /* Bezier        */
  tool_option_no_option (notebook);   /* Dummy         */
  tool_option_no_option (notebook);   /* Move Object   */
  tool_option_no_option (notebook);   /* Move Point    */
  tool_option_no_option (notebook);   /* Copy Object   */
  tool_option_no_option (notebook);   /* Delete Object */
}

static void
raise_selected_obj_to_top (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
  if (!gfig_context->selected_obj)
    return;

  if (g_list_find (gfig_context->current_obj->obj_list,
                   gfig_context->selected_obj))
    {
      gfig_context->current_obj->obj_list =
        g_list_remove (gfig_context->current_obj->obj_list,
                       gfig_context->selected_obj);
      gfig_context->current_obj->obj_list =
        g_list_append (gfig_context->current_obj->obj_list,
                       gfig_context->selected_obj);
    }
  else
    {
      g_message ("Trying to raise object that does not exist.");
      return;
    }

  gfig_paint_callback ();
}

static void
lower_selected_obj_to_bottom (GSimpleAction *action,
                              GVariant      *parameter,
                              gpointer       user_data)
{
  if (!gfig_context->selected_obj)
    return;

  if (g_list_find (gfig_context->current_obj->obj_list,
                   gfig_context->selected_obj))
    {
      gfig_context->current_obj->obj_list =
        g_list_remove (gfig_context->current_obj->obj_list,
                       gfig_context->selected_obj);
      gfig_context->current_obj->obj_list =
        g_list_prepend (gfig_context->current_obj->obj_list,
                        gfig_context->selected_obj);
    }
  else
    {
      g_message ("Trying to lower object that does not exist.");
      return;
    }

  gfig_paint_callback ();
}

static void
raise_selected_obj (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  if (!gfig_context->selected_obj)
    return;

  if (g_list_find (gfig_context->current_obj->obj_list,
                   gfig_context->selected_obj))
    {
      int position;

      position = g_list_index (gfig_context->current_obj->obj_list,
                               gfig_context->selected_obj);
      gfig_context->current_obj->obj_list =
        g_list_remove (gfig_context->current_obj->obj_list,
                       gfig_context->selected_obj);
      gfig_context->current_obj->obj_list =
        g_list_insert (gfig_context->current_obj->obj_list,
                       gfig_context->selected_obj,
                       position + 1);
    }
  else
    {
      g_message ("Trying to raise object that does not exist.");
      return;
    }

  gfig_paint_callback ();
}


static void
lower_selected_obj (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  if (!gfig_context->selected_obj)
    return;

  if (g_list_find (gfig_context->current_obj->obj_list,
                   gfig_context->selected_obj))
    {
      int position;

      position = g_list_index (gfig_context->current_obj->obj_list,
                               gfig_context->selected_obj);
      gfig_context->current_obj->obj_list =
        g_list_remove (gfig_context->current_obj->obj_list,
                       gfig_context->selected_obj);
      gfig_context->current_obj->obj_list =
        g_list_insert (gfig_context->current_obj->obj_list,
                       gfig_context->selected_obj,
                       MAX (0, position - 1));
    }
  else
    {
      g_message ("Trying to lower object that does not exist.");
      return;
    }

  gfig_paint_callback ();
}


static void
select_filltype_callback (GtkWidget *widget)
{
  gint value;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (fill_type_notebook),
                                 MIN (value, FILL_GRADIENT));

  gfig_context_get_current_style ()->fill_type = (FillType) value;

  gfig_paint_callback ();
}

static gboolean
gfig_paint_timeout (gpointer data)
{
  gfig_paint_callback ();

  paint_timeout = 0;

  return FALSE;
}

static void
gfig_paint_delayed (void)
{
  if (paint_timeout)
    g_source_remove (paint_timeout);

  paint_timeout =
    g_timeout_add (UPDATE_DELAY, gfig_paint_timeout, NULL);
}

static void
gfig_prefs_action (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
  static GtkWidget *dialog = NULL;

  if (!dialog)
    {
      GtkWidget     *main_vbox;
      GtkWidget     *grid;
      GtkWidget     *toggle;
      GtkWidget     *undo_scale;
      GtkWidget     *scale;
      GtkAdjustment *scale_data;

      dialog = gimp_dialog_new (_("Options"), "gimp-gfig-options",
                                NULL, 0, NULL, NULL,

                                _("_Close"),  GTK_RESPONSE_CLOSE,

                                NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

      g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer) &dialog);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (gtk_widget_destroy),
                        NULL);

      main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
      gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                          main_vbox, TRUE, TRUE, 0);
      gtk_widget_show (main_vbox);

      /* Put buttons in */
      toggle = gtk_check_button_new_with_label (_("Show position"));
      gtk_box_pack_start (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 6);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    selvals.showpos);
      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &selvals.showpos);
      g_signal_connect_after (toggle, "toggled",
                              G_CALLBACK (gfig_pos_enable),
                              NULL);
      gtk_widget_show (toggle);

      toggle = gtk_check_button_new_with_label (_("Show control points"));
      gtk_box_pack_start (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 6);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    selvals.opts.showcontrol);
      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &selvals.opts.showcontrol);
      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (toggle_show_image),
                        NULL);
      gtk_widget_show (toggle);

      gfig_opt_widget.showcontrol = toggle;
      g_object_add_weak_pointer (G_OBJECT (gfig_opt_widget.showcontrol),
                                 (gpointer) &gfig_opt_widget.showcontrol);

      toggle = gtk_check_button_new_with_label (_("Antialiasing"));
      gtk_box_pack_start (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 6);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), selopt.antia);
      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &selopt.antia);
      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gfig_paint_callback),
                        NULL);
      gtk_widget_show (toggle);

      grid = gtk_grid_new ();
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_box_pack_start (GTK_BOX (main_vbox), grid, FALSE, FALSE, 6);
      gtk_widget_show (grid);

      undo_scale = gimp_scale_entry_new (_("Max undo:"), selvals.maxundo, MIN_UNDO, MAX_UNDO, 0);
      g_signal_connect (undo_scale, "value-changed",
                        G_CALLBACK (gfig_scale_entry_update_int),
                        &selvals.maxundo);
      gtk_grid_attach (GTK_GRID (grid), undo_scale, 0, 0, 3, 1);
      gtk_widget_show (undo_scale);

      page_menu_bg = gimp_int_combo_box_new (_("Transparent"), LAYER_TRANS_BG,
                                             _("Background"),  LAYER_BG_BG,
                                             _("Foreground"),  LAYER_FG_BG,
                                             _("White"),       LAYER_WHITE_BG,
                                             _("Copy"),        LAYER_COPY_BG,
                                             NULL);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (page_menu_bg), selvals.onlayerbg);

      g_signal_connect (page_menu_bg, "changed",
                        G_CALLBACK (paint_combo_callback),
                        GINT_TO_POINTER (PAINT_BGS_MENU));

      gimp_help_set_help_data (page_menu_bg,
                               _("Layer background type. Copy causes the "
                                 "previous layer to be copied before the "
                                 "draw is performed."),
                               NULL);

      gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                _("Background:"), 0.0, 0.5,
                                page_menu_bg, 2);

      toggle = gtk_check_button_new_with_label (_("Feather"));
      gtk_grid_attach (GTK_GRID (grid), toggle, 2, 2, 1, 1);
      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &selopt.feather);
      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gfig_paint_callback),
                        NULL);
      gtk_widget_show (toggle);

      scale_data =
        gtk_adjustment_new (selopt.feather_radius, 0.0, 100.0, 1.0, 1.0, 0.0);
      scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, scale_data);
      gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);

      g_signal_connect (scale_data, "value-changed",
                        G_CALLBACK (gimp_double_adjustment_update),
                        &selopt.feather_radius);
      g_signal_connect (scale_data, "value-changed",
                        G_CALLBACK (gfig_paint_delayed),
                        NULL);
      gimp_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                                _("Radius:"), 0.0, 1.0, scale, 1);

      gtk_widget_show (dialog);
    }
  else
    {
      gtk_window_present (GTK_WINDOW (dialog));
    }
}

static void
gfig_grid_action (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
  static GtkWidget *dialog = NULL;

  if (!dialog)
    {
      GtkWidget *main_vbox;
      GtkWidget *hbox;
      GtkWidget *grid;
      GtkWidget *combo;
      GtkWidget *grid_scale;
      GtkWidget *sectors_scale;
      GtkWidget *radius_scale;

      dialog = gimp_dialog_new (_("Grid"), "gimp-gfig-grid",
                                NULL, 0, NULL, NULL,

                                _("_Close"),  GTK_RESPONSE_CLOSE,

                                NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

      g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer) &dialog);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (gtk_widget_destroy),
                        NULL);

      main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
      gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                          main_vbox, TRUE, TRUE, 0);
      gtk_widget_show (main_vbox);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      grid = gtk_grid_new ();
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_box_pack_start (GTK_BOX (main_vbox), grid, FALSE, FALSE, 0);
      gtk_widget_show (grid);

      grid_scale = gimp_scale_entry_new (_("Grid spacing:"),
                                         selvals.opts.gridspacing,
                                         MIN_GRID, MAX_GRID, 0);
      g_signal_connect (grid_scale, "value-changed",
                        G_CALLBACK (gfig_scale_entry_update_int),
                        &selvals.opts.gridspacing);
      g_signal_connect (grid_scale, "value-changed",
                        G_CALLBACK (draw_grid_clear),
                        NULL);
      gtk_grid_attach (GTK_GRID (grid), grid_scale, 0, 0, 3, 1);
      gtk_widget_show (grid_scale);

      gfig_opt_widget.gridspacing = grid_scale;
      g_object_add_weak_pointer (G_OBJECT (gfig_opt_widget.gridspacing),
                                 (gpointer) &gfig_opt_widget.gridspacing);

      sectors_scale = gimp_scale_entry_new (_("Polar grid sectors desired:"),
                                            selvals.opts.grid_sectors_desired,
                                            5, 360, 0);
      g_signal_connect (sectors_scale, "value-changed",
                        G_CALLBACK (gfig_scale_entry_update_int),
                        &selvals.opts.grid_sectors_desired);
      g_signal_connect (sectors_scale, "value-changed",
                        G_CALLBACK (draw_grid_clear),
                        NULL);
      gtk_grid_attach (GTK_GRID (grid), sectors_scale, 0, 3, 3, 1);
      gtk_widget_show (sectors_scale);


      gfig_opt_widget.grid_sectors_desired = sectors_scale;
      g_object_add_weak_pointer (G_OBJECT (gfig_opt_widget.grid_sectors_desired),
                                 (gpointer) &gfig_opt_widget.grid_sectors_desired);

      radius_scale = gimp_scale_entry_new (_("Polar grid radius interval:"),
                                           selvals.opts.grid_radius_interval,
                                           5, 50, 0);
      g_signal_connect (radius_scale, "value-changed",
                        G_CALLBACK (gfig_scale_entry_update_double),
                        &selvals.opts.grid_radius_interval);
      g_signal_connect (radius_scale, "value-changed",
                        G_CALLBACK (draw_grid_clear),
                        NULL);
      gtk_grid_attach (GTK_GRID (grid), radius_scale, 0, 4, 3, 1);
      gtk_widget_show (radius_scale);

      gfig_opt_widget.grid_radius_interval = radius_scale;
      g_object_add_weak_pointer (G_OBJECT (gfig_opt_widget.grid_radius_interval),
                                 (gpointer) &gfig_opt_widget.grid_radius_interval);

      combo = gimp_int_combo_box_new (_("Rectangle"), RECT_GRID,
                                      _("Polar"),     POLAR_GRID,
                                      _("Isometric"), ISO_GRID,
                                      NULL);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), selvals.opts.gridtype);

      g_signal_connect (combo, "changed",
                        G_CALLBACK (gridtype_combo_callback),
                        GINT_TO_POINTER (GRID_TYPE_MENU));

      gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                _("Grid type:"), 0.0, 0.5,
                                combo, 2);

      gfig_opt_widget.gridtypemenu = combo;
      g_object_add_weak_pointer (G_OBJECT (gfig_opt_widget.gridtypemenu),
                                 (gpointer) &gfig_opt_widget.gridtypemenu);

      combo = gimp_int_combo_box_new (_("Normal"),    GFIG_NORMAL_GC,
                                      _("Black"),     GFIG_BLACK_GC,
                                      _("White"),     GFIG_WHITE_GC,
                                      _("Grey"),      GFIG_GREY_GC,
                                      _("Darker"),    GFIG_DARKER_GC,
                                      _("Lighter"),   GFIG_LIGHTER_GC,
                                      _("Very dark"), GFIG_VERY_DARK_GC,
                                      NULL);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), grid_gc_type);

      g_signal_connect (combo, "changed",
                        G_CALLBACK (gridtype_combo_callback),
                        GINT_TO_POINTER (GRID_RENDER_MENU));

      gimp_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                                _("Grid color:"), 0.0, 0.5,
                                combo, 2);

      gtk_widget_show (dialog);
    }
  else
    {
      gtk_window_present (GTK_WINDOW (dialog));
    }
}

void
options_update (GFigObj *old_obj)
{
  /* Save old vals */
  if (selvals.opts.gridspacing != old_obj->opts.gridspacing)
    {
      old_obj->opts.gridspacing = selvals.opts.gridspacing;
    }
  if (selvals.opts.grid_sectors_desired != old_obj->opts.grid_sectors_desired)
    {
      old_obj->opts.grid_sectors_desired = selvals.opts.grid_sectors_desired;
    }
  if (selvals.opts.grid_radius_interval != old_obj->opts.grid_radius_interval)
    {
      old_obj->opts.grid_radius_interval = selvals.opts.grid_radius_interval;
    }
  if (selvals.opts.gridtype != old_obj->opts.gridtype)
    {
      old_obj->opts.gridtype = selvals.opts.gridtype;
    }
  if (selvals.opts.drawgrid != old_obj->opts.drawgrid)
    {
      old_obj->opts.drawgrid = selvals.opts.drawgrid;
    }
  if (selvals.opts.snap2grid != old_obj->opts.snap2grid)
    {
      old_obj->opts.snap2grid = selvals.opts.snap2grid;
    }
  if (selvals.opts.lockongrid != old_obj->opts.lockongrid)
    {
      old_obj->opts.lockongrid = selvals.opts.lockongrid;
    }
  if (selvals.opts.showcontrol != old_obj->opts.showcontrol)
    {
      old_obj->opts.showcontrol = selvals.opts.showcontrol;
    }

  /* New vals */
  if (selvals.opts.gridspacing != gfig_context->current_obj->opts.gridspacing)
    {
      if (gfig_opt_widget.gridspacing)
        gimp_label_spin_set_value (GIMP_LABEL_SPIN (gfig_opt_widget.gridspacing),
                                   gfig_context->current_obj->opts.gridspacing);
    }
  if (selvals.opts.grid_sectors_desired != gfig_context->current_obj->opts.grid_sectors_desired)
    {
      if (gfig_opt_widget.grid_sectors_desired)
        gimp_label_spin_set_value (GIMP_LABEL_SPIN (gfig_opt_widget.grid_sectors_desired),
                                   gfig_context->current_obj->opts.grid_sectors_desired);
    }
  if (selvals.opts.grid_radius_interval != gfig_context->current_obj->opts.grid_radius_interval)
    {
      if (gfig_opt_widget.grid_radius_interval)
        gimp_label_spin_set_value (GIMP_LABEL_SPIN (gfig_opt_widget.grid_radius_interval),
                                   gfig_context->current_obj->opts.grid_radius_interval);
    }
  if (selvals.opts.gridtype != gfig_context->current_obj->opts.gridtype)
    {
      if (gfig_opt_widget.gridtypemenu)
        gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (gfig_opt_widget.gridtypemenu),
                                       gfig_context->current_obj->opts.gridtype);
    }
  if (selvals.opts.drawgrid != gfig_context->current_obj->opts.drawgrid)
    {
      if (gfig_opt_widget.drawgrid)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gfig_opt_widget.drawgrid),
                                      gfig_context->current_obj->opts.drawgrid);
    }
  if (selvals.opts.snap2grid != gfig_context->current_obj->opts.snap2grid)
    {
      if (gfig_opt_widget.snap2grid)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gfig_opt_widget.snap2grid),
                                      gfig_context->current_obj->opts.snap2grid);
    }
  if (selvals.opts.lockongrid != gfig_context->current_obj->opts.lockongrid)
    {
      if (gfig_opt_widget.lockongrid)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gfig_opt_widget.lockongrid),
                                      gfig_context->current_obj->opts.lockongrid);
    }
  if (selvals.opts.showcontrol != gfig_context->current_obj->opts.showcontrol)
    {
      if (gfig_opt_widget.showcontrol)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gfig_opt_widget.showcontrol),
                                      gfig_context->current_obj->opts.showcontrol);
    }
}

static void
save_file_chooser_response (GtkFileChooser *chooser,
                            gint            response_id,
                            GFigObj        *obj)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar   *filename;

      filename = gtk_file_chooser_get_filename (chooser);

      obj->filename = filename;

      gfig_context->current_obj = obj;
      gfig_save_callbk ();
    }

  gtk_widget_destroy (GTK_WIDGET (chooser));
}

static GfigObject *
gfig_select_obj_by_number (gint count)
{
  GList      *objs;
  GfigObject *object = NULL;
  gint        k;

  gfig_context->selected_obj = NULL;

  for (objs = gfig_context->current_obj->obj_list, k = 0;
       objs;
       objs = g_list_next (objs), k++)
    {
      if (k == obj_show_single)
        {
          object = objs->data;
          gfig_context->selected_obj = object;
          gfig_style_set_context_from_style (&object->style);
          break;
        }
    }

  return object;
}

static void
select_button_clicked (gint type)
{
  gint   count = 0;

  if (gfig_context->current_obj)
    {
      count = g_list_length (gfig_context->current_obj->obj_list);
    }

  switch (type)
    {
    case OBJ_SELECT_LT:
      obj_show_single--;
      if (obj_show_single < 0)
        obj_show_single = count - 1;
      break;

    case OBJ_SELECT_GT:
      obj_show_single++;
      if (obj_show_single >= count)
        obj_show_single = 0;
      break;

    case OBJ_SELECT_EQ:
      obj_show_single = -1; /* Reset to show all */
      break;

    default:
      break;
    }

  if (obj_show_single >= 0)
    gfig_select_obj_by_number (obj_show_single);

  draw_grid_clear ();
  gfig_paint_callback ();
}

static void
select_button_clicked_lt (GSimpleAction *action,
                          GVariant      *parameter,
                          gpointer       user_data)
{
  select_button_clicked (OBJ_SELECT_LT);
}

static void
select_button_clicked_gt (GSimpleAction *action,
                          GVariant      *parameter,
                          gpointer       user_data)
{
  select_button_clicked (OBJ_SELECT_GT);
}

static void
select_button_clicked_eq (GSimpleAction *action,
                          GVariant      *parameter,
                          gpointer       user_data)
{
  select_button_clicked (OBJ_SELECT_EQ);
}

/* Special case for now - options on poly/star/spiral button */

GtkWidget *
num_sides_widget (const gchar *d_title,
                  gint        *num_sides,
                  gint        *which_way,
                  gint         adj_min,
                  gint         adj_max)
{
  GtkWidget *grid;
  GtkWidget *scale;

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_widget_show (grid);

  scale = gimp_scale_entry_new (_("Sides:"), *num_sides, adj_min, adj_max, 0);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (gfig_scale_entry_update_int),
                    num_sides);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 0, 3, 1);
  gtk_widget_show (scale);

  if (which_way)
    {
      GtkWidget *combo = gimp_int_combo_box_new (_("Right"),      0,
                                                 _("Left"), 1,
                                                 NULL);

      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), *which_way);

      g_signal_connect (combo, "changed",
                        G_CALLBACK (gimp_int_combo_box_get_active),
                        which_way);

      gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                _("Orientation:"), 0.0, 0.5,
                                combo, 1);
    }
  return grid;
}

void
gfig_paint (BrushType     brush_type,
            GimpDrawable *drawable,
            gint          seg_count,
            gdouble       line_pnts[])
{
  switch (brush_type)
    {
    case BRUSH_BRUSH_TYPE:
      gimp_paintbrush (drawable,
                       selvals.brushfade,
                       seg_count, line_pnts,
                       GIMP_PAINT_CONSTANT,
                       selvals.brushgradient);
      break;

    case BRUSH_PENCIL_TYPE:
      gimp_pencil (drawable,
                   seg_count, line_pnts);
      break;

    case BRUSH_AIRBRUSH_TYPE:
      gimp_airbrush (drawable,
                     selvals.airbrushpressure,
                     seg_count, line_pnts);
      break;

    case BRUSH_PATTERN_TYPE:
      gimp_clone (drawable,
                  drawable,
                  GIMP_CLONE_PATTERN,
                  0.0, 0.0,
                  seg_count, line_pnts);
      break;
    }
}


static void
paint_combo_callback (GtkWidget *widget,
                      gpointer   data)
{
  gint mtype = GPOINTER_TO_INT (data);
  gint value;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value);

  switch (mtype)
    {
    case PAINT_BGS_MENU:
      selvals.onlayerbg = (LayersBGType) value;
      break;

    default:
      g_return_if_reached ();
      break;
    }

  gfig_paint_callback ();
}


static void
gridtype_combo_callback (GtkWidget *widget,
                         gpointer   data)
{
  gint mtype = GPOINTER_TO_INT (data);
  gint value;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value);

  switch (mtype)
    {
    case GRID_TYPE_MENU:
      selvals.opts.gridtype = value;
      break;

    case GRID_RENDER_MENU:
      grid_gc_type = value;
      break;

    default:
      g_return_if_reached ();
      break;
    }

  draw_grid_clear ();
}

/*
 *  The edit gfig name attributes dialog
 *  Modified from Gimp source - layer edit.
 */

typedef struct _GfigListOptions
{
  GtkWidget *query_box;
  GtkWidget *name_entry;
  GtkWidget *list_entry;
  GFigObj   *obj;
  gboolean   created;
} GfigListOptions;

static void
load_file_chooser_response (GtkFileChooser *chooser,
                            gint            response_id,
                            gpointer        data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar   *filename;
      GFigObj *gfig_obj;
      GFigObj *current_saved;

      filename = gtk_file_chooser_get_filename (chooser);

      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        {
          /* Hack - current object MUST be NULL to prevent setup_undo ()
           * from kicking in.
           */
          current_saved = gfig_context->current_obj;
          gfig_context->current_obj = NULL;
          gfig_obj = gfig_load (GIMP_GFIG (data), filename, filename);
          gfig_context->current_obj = current_saved;

          if (gfig_obj)
            {
              /* Read only ?*/
              if (access (filename, W_OK))
                gfig_obj->obj_status |= GFIG_READONLY;

              gfig_list_insert (gfig_obj);
              new_obj_2edit (GIMP_GFIG (data), gfig_obj);
            }
        }

      g_free (filename);
    }

  gtk_widget_destroy (GTK_WIDGET (chooser));
  gfig_paint_callback ();
}

void
paint_layer_fill (gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
  GimpFillType fill_type = GIMP_FILL_FOREGROUND;
  Style *current_style;

  current_style = gfig_context_get_current_style ();

  gimp_context_push ();

  gimp_context_set_paint_mode (GIMP_LAYER_MODE_NORMAL_LEGACY);
  gimp_context_set_opacity (100.0);
  gimp_context_set_gradient_repeat_mode (GIMP_REPEAT_NONE);
  gimp_context_set_gradient_reverse (FALSE);

  switch (current_style->fill_type)
    {
    case FILL_NONE:
      return;

    case FILL_COLOR:
      fill_type = GIMP_FILL_BACKGROUND;
      break;

    case FILL_PATTERN:
      fill_type = GIMP_FILL_PATTERN;
      break;

    case FILL_GRADIENT:
      gimp_drawable_edit_gradient_fill (gfig_context->drawable,
                                        GIMP_GRADIENT_SHAPEBURST_DIMPLED,
                                        0.0,       /* offset             */
                                        FALSE,     /* supersampling      */
                                        0,         /* max_depth          */
                                        0.0,       /* threshold          */
                                        FALSE,     /* dither             */
                                        0.0, 0.0,  /* (x1, y1) - ignored */
                                        0.0, 0.0); /* (x2, y2) - ignored */
      return;
    case FILL_VERTICAL:
      gimp_drawable_edit_gradient_fill (gfig_context->drawable,
                                        GIMP_GRADIENT_LINEAR,
                                        0.0,
                                        FALSE,
                                        0,
                                        0.0,
                                        FALSE,
                                        x1, y1,
                                        x1, y2);
      return;
    case FILL_HORIZONTAL:
      gimp_drawable_edit_gradient_fill (gfig_context->drawable,
                                        GIMP_GRADIENT_LINEAR,
                                        0.0,
                                        FALSE,
                                        0,
                                        0.0,
                                        FALSE,
                                        x1, y1,
                                        x2, y1);
      return;
    }

  gimp_context_set_opacity (current_style->fill_opacity);

  gimp_drawable_edit_fill (gfig_context->drawable,
                           fill_type);

  gimp_context_pop ();
}

void
gfig_paint_callback (void)
{
  GList      *objs;
  gint        ccount = 0;
  GfigObject *object;

  if (!gfig_context->enable_repaint || !gfig_context->current_obj)
    return;

  objs = gfig_context->current_obj->obj_list;

  gimp_drawable_fill (gfig_context->drawable, GIMP_FILL_TRANSPARENT);

  while (objs)
    {
      if (ccount == obj_show_single || obj_show_single == -1)
        {
          FillType saved_filltype;

          object = objs->data;

          gfig_style_apply (&object->style);

          saved_filltype = gfig_context_get_current_style ()->fill_type;
          gfig_context_get_current_style ()->fill_type = object->style.fill_type;
          object->class->paintfunc (object);
          gfig_context_get_current_style ()->fill_type = saved_filltype;
        }

      objs = g_list_next (objs);

      ccount++;
    }

  gimp_displays_flush ();

  if (back_pixbuf)
    {
      g_object_unref (back_pixbuf);
      back_pixbuf = NULL;
    }

  gtk_widget_queue_draw (gfig_context->preview);
}

/* Draw the grid on the screen
 */

void
draw_grid_clear (void)
{
  /* wipe slate and start again */
  gtk_widget_queue_draw (gfig_context->preview);
}

static void
toggle_show_image (void)
{
  /* wipe slate and start again */
  draw_grid_clear ();
}

static void
toggle_obj_type (DobjType new_type)
{
  /* cache of cursors.
   * Must be larger than DobjType values, i.e. NULL_OPER.
   * Test by clicking the "select object" icon.
   * C ensures is initialized to NULL.
   */
  static GdkCursor *p_cursors[NULL_OPER];

  GdkCursorType     ctype = GDK_LAST_CURSOR;

  if (selvals.otype != new_type)
    {
      /* Mem leak */
      obj_creating = NULL;
      tmp_line = NULL;
      tmp_bezier = NULL;

      if (new_type < MOVE_OBJ) /* Eeeeek */
        {
          g_debug ("%s new_type < MOVE_OBJ", G_STRFUNC);
          obj_show_single = -1; /* Cancel select preview */
        }
      /* Update draw areas */
      gtk_widget_queue_draw (gfig_context->preview);
    }

  g_debug ("%s: old and new obj type %d %d", G_STRFUNC, selvals.otype, new_type);

  selvals.otype = new_type;

  gtk_notebook_set_current_page (GTK_NOTEBOOK (tool_options_notebook),
                                 new_type - 1);

  switch (selvals.otype)
    {
    case LINE:
    case RECTANGLE:
    case CIRCLE:
    case ELLIPSE:
    case ARC:
    case POLY:
    case STAR:
    case SPIRAL:
    case BEZIER:
      ctype = GDK_CROSSHAIR;
      break;
    case MOVE_OBJ:
    case MOVE_POINT:
    case COPY_OBJ:
    case MOVE_COPY_OBJ:
      ctype = GDK_DIAMOND_CROSS;
      break;
    case DEL_OBJ:
      ctype = GDK_PIRATE;
      break;
    case OBJ_TYPE_NONE:
      ctype = GDK_CROSSHAIR;
      break;
    default:
      g_debug ("%s: default cursor for object type %d.", G_STRFUNC, selvals.otype);
      ctype = GDK_CROSSHAIR;
      break;
    }

  /* Get cached cursor. */
  if (!p_cursors[selvals.otype])
    {
      GdkCursor *cursor;

      GdkDisplay *display = gtk_widget_get_display (gfig_context->preview);
      cursor = gdk_cursor_new_for_display (display, ctype);
      p_cursors[selvals.otype] = cursor;
    }
  /* Require cursor (possibly from cache) is-a cursor. */
  g_assert (GDK_IS_CURSOR (p_cursors[selvals.otype]));

  gdk_window_set_cursor (gtk_widget_get_window (gfig_context->preview),
                         p_cursors[selvals.otype]);
}

/* This could belong in a separate file ... but makes it easier to lump into
 * one when compiling the plugin.
 */

/* Given a number of float co-ords adjust for scaling back to org size */
/* Size is number of PAIRS of points */
/* FP + int variants */

static void
scale_to_orginal_x (gdouble *list)
{
  *list *= scale_x_factor;
}

gint
gfig_scale_x (gint x)
{
  if (!selvals.scaletoimage)
    return (gint) (x * (1 / scale_x_factor));
  else
    return x;
}

static void
scale_to_orginal_y (gdouble *list)
{
  *list *= scale_y_factor;
}

gint
gfig_scale_y (gint y)
{
  if (!selvals.scaletoimage)
    return (gint) (y * (1 / scale_y_factor));
  else
    return y;
}

/* Pairs x followed by y */
void
scale_to_original_xy (gdouble *list,
                      gint     size)
{
  gint i;

  for (i = 0; i < size * 2; i += 2)
    {
      scale_to_orginal_x (&list[i]);
      scale_to_orginal_y (&list[i + 1]);
    }
}

/* Pairs x followed by y */
void
scale_to_xy (gdouble *list,
             gint     size)
{
  gint i;

  for (i = 0; i < size * 2; i += 2)
    {
      list[i] *= (org_scale_x_factor / scale_x_factor);
      list[i + 1] *= (org_scale_y_factor / scale_y_factor);
    }
}

void
gfig_draw_arc (gint x, gint y, gint width, gint height, gint angle1,
               gint angle2, cairo_t *cr)
{
  cairo_save (cr);

  cairo_translate (cr, gfig_scale_x (x), gfig_scale_y (y));
  cairo_scale (cr, gfig_scale_x (width), gfig_scale_y (height));

  if (angle1 < angle2)
    cairo_arc (cr, 0., 0., 1., angle1 * G_PI / 180, angle2 * G_PI / 180);
  else
    cairo_arc_negative (cr, 0., 0., 1., angle1 * G_PI / 180, angle2 * G_PI / 180);

  cairo_restore (cr);

  draw_item (cr, FALSE);
}

void
gfig_draw_line (gint x0, gint y0, gint x1, gint y1, cairo_t *cr)
{
  cairo_move_to (cr, gfig_scale_x (x0) + .5, gfig_scale_y (y0) + .5);
  cairo_line_to (cr, gfig_scale_x (x1) + .5, gfig_scale_y (y1) + .5);

  draw_item (cr, FALSE);
}

static void
gfig_scale_entry_update_double (GimpLabelSpin *entry,
                                gdouble       *value)
{
  *value = gimp_label_spin_get_value (entry);
}

static void
gfig_scale_entry_update_int (GimpLabelSpin *entry,
                             gint          *value)
{
  *value = (gint) gimp_label_spin_get_value (entry);
}

static GtkWidget *
add_tool_button (GtkWidget  *toolbar,
                 const char *action,
                 const char *icon,
                 const char *label,
                 const char *tooltip)
{
  GtkWidget   *tool_icon;
  GtkToolItem *tool_button;

  tool_icon = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (GTK_WIDGET (tool_icon));
  tool_button = gtk_tool_button_new (tool_icon, label);
  gtk_widget_show (GTK_WIDGET (tool_button));
  gtk_tool_item_set_tooltip_text (tool_button, tooltip);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (tool_button), action);

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), tool_button, -1);

  return GTK_WIDGET (tool_button);
}

GtkWidget *
add_toggle_button (GtkWidget  *toolbar,
                   const char *action,
                   const char *icon,
                   const char *label,
                   const char *tooltip)
{
  GtkWidget   *tool_icon;
  GtkToolItem *toggle_tool_button;

  tool_icon = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (GTK_WIDGET (tool_icon));

  toggle_tool_button = gtk_toggle_tool_button_new ();
  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (toggle_tool_button),
                                   tool_icon);
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (toggle_tool_button), label);
  gtk_widget_show (GTK_WIDGET (toggle_tool_button));
  gtk_tool_item_set_tooltip_text (toggle_tool_button, tooltip);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (toggle_tool_button), action);

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toggle_tool_button, -1);

  return GTK_WIDGET (toggle_tool_button);
}

static void
add_tool_separator (GtkWidget *toolbar,
                    gboolean   expand)
{
  GtkToolItem *item;

  item = gtk_separator_tool_item_new ();
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
  gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item), FALSE);
  gtk_tool_item_set_expand (item, expand);
  gtk_widget_show (GTK_WIDGET (item));
}
