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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include "gfig-line.h"
#include "gfig-poly.h"
#include "gfig-preview.h"
#include "gfig-rectangle.h"
#include "gfig-spiral.h"
#include "gfig-star.h"
#include "gfig-stock.h"

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
  ADD,          /* type */
  FALSE,        /* Antia */
  FALSE,        /* Feather */
  10.0,         /* feather radius */
  ARC_SEGMENT,  /* Arc as a segment */
  FILL_PATTERN, /* Fill as pattern */
  100.0,        /* Max opacity */
};

/* Should be kept in sync with GfigOpts */
typedef struct
{
  GtkAdjustment *gridspacing;
  GtkAdjustment *grid_sectors_desired;
  GtkAdjustment *grid_radius_interval;
  GtkWidget     *gridtypemenu;
  GtkWidget     *drawgrid;
  GtkWidget     *snap2grid;
  GtkWidget     *lockongrid;
  GtkWidget     *showcontrol;
} GfigOptWidgets;

static GfigOptWidgets  gfig_opt_widget = { NULL, NULL, NULL, NULL, NULL, NULL };
static gchar          *gfig_path       = NULL;
static GtkWidget      *page_menu_bg;
static GtkWidget      *tool_options_notebook;
static GtkWidget      *fill_type_notebook;
static guint           paint_timeout   = 0;

static GtkActionGroup *gfig_actions    = NULL;


static void       gfig_response              (GtkWidget *widget,
                                              gint       response_id,
                                              gpointer   data);
static void       gfig_load_action_callback  (GtkAction *action,
                                              gpointer   data);
static void       gfig_save_action_callback  (GtkAction *action,
                                              gpointer   data);
static void       gfig_list_load_all         (const gchar *path);
static void       gfig_list_free_all         (void);
static void       create_notebook_pages      (GtkWidget *notebook);
static void       select_filltype_callback   (GtkWidget *widget);
static void       gfig_grid_action_callback  (GtkAction *action,
                                              gpointer   data);
static void       gfig_prefs_action_callback (GtkAction *action,
                                              gpointer   data);
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
static void     select_button_clicked_lt     (void);
static void     select_button_clicked_gt     (void);
static void     select_button_clicked_eq     (void);
static void     raise_selected_obj_to_top    (GtkWidget *widget,
                                              gpointer   data);
static void     lower_selected_obj_to_bottom (GtkWidget *widget,
                                              gpointer   data);
static void     raise_selected_obj           (GtkWidget *widget,
                                              gpointer   data);
static void     lower_selected_obj           (GtkWidget *widget,
                                              gpointer   data);

static void     toggle_obj_type              (GtkRadioAction *action,
                                              GtkRadioAction *current,
                                              gpointer        data);

static GtkUIManager *create_ui_manager       (GtkWidget *window);


gboolean
gfig_dialog (void)
{
  GtkWidget    *main_hbox;
  GtkWidget    *vbox;
  GFigObj      *gfig;
  GimpParasite *parasite;
  gint          newlayer;
  GtkWidget    *menubar;
  GtkWidget    *toolbar;
  GtkWidget    *combo;
  GtkWidget    *frame;
  gint          img_width;
  gint          img_height;
  GimpImageType img_type;
  GtkWidget    *toggle;
  GtkWidget    *right_vbox;
  GtkWidget    *hbox;
  GtkUIManager *ui_manager;
  GtkWidget    *empty_label;
  gchar        *path;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  img_width  = gimp_drawable_width (gfig_context->drawable_id);
  img_height = gimp_drawable_height (gfig_context->drawable_id);
  img_type   = gimp_drawable_type_with_alpha (gfig_context->drawable_id);

  /*
   * See if there is a "gfig" parasite.  If so, this is a gfig layer,
   * and we start by clearing it to transparent.
   * If not, we create a new transparent layer.
   */
  gfig_list = NULL;
  undo_level = -1;
  parasite = gimp_item_get_parasite (gfig_context->drawable_id, "gfig");
  gfig_context->enable_repaint = FALSE;

  /* debug */
  gfig_context->debug_styles = FALSE;

  /* initial default style */
  gfig_read_gimp_style (&gfig_context->default_style, "Base");
  gfig_context->default_style.paint_type = selvals.painttype;

  if (parasite)
    {
      gimp_drawable_fill (gfig_context->drawable_id, GIMP_FILL_TRANSPARENT);
      gfig_context->using_new_layer = FALSE;
      gimp_parasite_free (parasite);
    }
  else
    {
      newlayer = gimp_layer_new (gfig_context->image_id, "GFig",
                                 img_width, img_height,
                                 img_type,
                                 100.0,
                                 gimp_image_get_default_new_layer_mode (gfig_context->image_id));
      gimp_drawable_fill (newlayer, GIMP_FILL_TRANSPARENT);
      gimp_image_insert_layer (gfig_context->image_id, newlayer, -1, -1);
      gfig_context->drawable_id = newlayer;
      gfig_context->using_new_layer = TRUE;
    }

  gfig_stock_init ();

  path = gimp_gimprc_query ("gfig-path");

  if (path)
    {
      gfig_path = g_filename_from_utf8 (path, -1, NULL, NULL, NULL);
      g_free (path);
    }
  else
    {
      gchar *gimprc    = gimp_personal_rc_file ("gimprc");
      gchar *full_path = gimp_config_build_data_path ("gfig");
      gchar *esc_path  = g_strescape (full_path, NULL);
      g_free (full_path);

      g_message (_("No %s in gimprc:\n"
                   "You need to add an entry like\n"
                   "(%s \"%s\")\n"
                   "to your %s file."),
                   "gfig-path", "gfig-path", esc_path,
                   gimp_filename_to_utf8 (gimprc));

      g_free (gimprc);
      g_free (esc_path);
    }

  /* Start building the dialog up */
  top_level_dlg = gimp_dialog_new (_("Gfig"), PLUG_IN_ROLE,
                                   NULL, 0,
                                   gimp_standard_help_func, PLUG_IN_PROC,

                                   _("_Cancel"), GTK_RESPONSE_CANCEL,
                                   _("_Close"),  GTK_RESPONSE_OK,

                                   NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (top_level_dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (top_level_dlg, "response",
                    G_CALLBACK (gfig_response),
                    top_level_dlg);

  /* build the menu */
  ui_manager = create_ui_manager (top_level_dlg);
  menubar = gtk_ui_manager_get_widget (ui_manager, "/ui/gfig-menubar");
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (top_level_dlg))),
                      menubar, FALSE, FALSE, 0);
  gtk_widget_show (menubar);
  toolbar = gtk_ui_manager_get_widget (ui_manager, "/ui/gfig-toolbar");
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (top_level_dlg))),
                      toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  gfig_dialog_action_set_sensitive ("undo", undo_level >= 0);

  /* Main box */
  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (top_level_dlg))),
                    main_hbox, TRUE, TRUE, 0);

  /* Preview itself */
  gtk_box_pack_start (GTK_BOX (main_hbox), make_preview (), FALSE, FALSE, 0);

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
  gfig_context->fg_color = g_new0 (GimpRGB, 1);
  gfig_context->fg_color_button = gimp_color_button_new ("Foreground",
                                                    SEL_BUTTON_WIDTH,
                                                    SEL_BUTTON_HEIGHT,
                                                    gfig_context->fg_color,
                                                    GIMP_COLOR_AREA_SMALL_CHECKS);
  g_signal_connect (gfig_context->fg_color_button, "color-changed",
                    G_CALLBACK (set_foreground_callback),
                    gfig_context->fg_color);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (gfig_context->fg_color_button),
                               &gfig_context->default_style.foreground);
  gtk_box_pack_start (GTK_BOX (vbox), gfig_context->fg_color_button,
                      FALSE, FALSE, 0);
  gtk_widget_show (gfig_context->fg_color_button);

  /* brush selector in Stroke frame */
  gfig_context->brush_select
    = gimp_brush_select_button_new ("Brush",
                                    gfig_context->default_style.brush_name,
                                    -1.0, -1, -1);
  g_signal_connect (gfig_context->brush_select, "brush-set",
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
  gfig_context->bg_color = g_new0 (GimpRGB, 1);
  gfig_context->bg_color_button = gimp_color_button_new ("Background",
                                           SEL_BUTTON_WIDTH, SEL_BUTTON_HEIGHT,
                                           gfig_context->bg_color,
                                           GIMP_COLOR_AREA_SMALL_CHECKS);
  g_signal_connect (gfig_context->bg_color_button, "color-changed",
                    G_CALLBACK (set_background_callback),
                    gfig_context->bg_color);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (gfig_context->bg_color_button),
                               &gfig_context->default_style.background);
  gtk_widget_show (gfig_context->bg_color_button);
  gtk_notebook_append_page (GTK_NOTEBOOK (fill_type_notebook),
                            gfig_context->bg_color_button, NULL);

  /* A page for the pattern selector */
  gfig_context->pattern_select
    = gimp_pattern_select_button_new ("Pattern", gfig_context->default_style.pattern);
  g_signal_connect (gfig_context->pattern_select, "pattern-set",
                    G_CALLBACK (gfig_pattern_changed_callback), NULL);
  gtk_widget_show (gfig_context->pattern_select);
  gtk_notebook_append_page (GTK_NOTEBOOK (fill_type_notebook),
                            gfig_context->pattern_select, NULL);

  /* A page for the gradient selector */
  gfig_context->gradient_select
    = gimp_gradient_select_button_new ("Gradient", gfig_context->default_style.gradient);
  g_signal_connect (gfig_context->gradient_select, "gradient-set",
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
  gfig_context->bdesc.name = gimp_context_get_brush ();
  mygimp_brush_info (&gfig_context->bdesc.width, &gfig_context->bdesc.height);

  gtk_widget_show (main_hbox);

  gtk_widget_show (top_level_dlg);

  gfig = gfig_load_from_parasite ();
  if (gfig)
    {
      gfig_list_insert (gfig);
      new_obj_2edit (gfig);
      gfig_style_set_context_from_style (&gfig_context->default_style);
      gfig_style_apply (&gfig_context->default_style);
    }

  gfig_context->enable_repaint = TRUE;
  gfig_paint_callback ();

  gtk_main ();

  /* FIXME */
  return TRUE;
}

static void
gfig_response (GtkWidget *widget,
               gint       response_id,
               gpointer   data)
{
  GFigObj *gfig;

  switch (response_id)
    {
    case GTK_RESPONSE_DELETE_EVENT:
    case GTK_RESPONSE_CANCEL:
      /* if we created a new layer, delete it */
      if (gfig_context->using_new_layer)
        {
          gimp_image_remove_layer (gfig_context->image_id,
                                   gfig_context->drawable_id);
        }
      else /* revert back to the original figure */
        {
          free_all_objs (gfig_context->current_obj->obj_list);
          gfig_context->current_obj->obj_list = NULL;
          gfig = gfig_load_from_parasite ();
          if (gfig)
            {
              gfig_list_insert (gfig);
              new_obj_2edit (gfig);
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

  gtk_widget_destroy (widget);
  gtk_main_quit ();
}

void
gfig_dialog_action_set_sensitive (const gchar *name,
                                  gboolean     sensitive)
{
  g_return_if_fail (name != NULL);

  if (gfig_actions)
    {
      GtkAction *action = gtk_action_group_get_action (gfig_actions, name);

      if (! action)
        {
          g_warning ("%s: Unable to set sensitivity of action "
                     "which doesn't exist: %s",
                     G_STRFUNC, name);
          return;
        }

      g_object_set (action, "sensitive", sensitive ? TRUE : FALSE, NULL);
    }
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
gfig_load_action_callback (GtkAction *action,
                           gpointer   data)
{
  static GtkWidget *dialog = NULL;

  if (! dialog)
    {
      gchar *dir;

      dialog =
        gtk_file_chooser_dialog_new (_("Load Gfig Object Collection"),
                                     GTK_WINDOW (data),
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
                        NULL);

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
gfig_save_action_callback (GtkAction *action,
                           gpointer   data)
{
  static GtkWidget *dialog = NULL;

  if (!dialog)
    {
      gchar *dir;

      dialog =
        gtk_file_chooser_dialog_new (_("Save Gfig Drawing"),
                                     GTK_WINDOW (data),
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
gfig_close_action_callback (GtkAction *action,
                            gpointer   data)
{
  gtk_dialog_response (GTK_DIALOG (data), GTK_RESPONSE_OK);
}

static void
gfig_undo_action_callback (GtkAction *action,
                           gpointer   data)
{
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

  gfig_dialog_action_set_sensitive ("undo", undo_level >= 0);
  gfig_paint_callback ();
}

static void
gfig_clear_action_callback (GtkWidget *widget,
                          gpointer   data)
{
  /* Make sure we can get back - if we have some objects to get back to */
  if (!gfig_context->current_obj->obj_list)
    return;

  setup_undo ();
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

static GtkUIManager *
create_ui_manager (GtkWidget *window)
{
  static GtkActionEntry actions[] =
  {
    { "gfig-menubar", NULL, "GFig Menu" },

    { "gfig-file-menu", NULL, "_File" },

    { "open", GIMP_ICON_DOCUMENT_OPEN,
      N_("_Open..."), "<control>O", NULL,
      G_CALLBACK (gfig_load_action_callback) },

    { "save", GIMP_ICON_DOCUMENT_SAVE,
      N_("_Save..."), "<control>S", NULL,
      G_CALLBACK (gfig_save_action_callback) },

    { "close", GIMP_ICON_CLOSE,
      N_("_Close"), "<control>C", NULL,
      G_CALLBACK (gfig_close_action_callback) },

    { "gfig-edit-menu", NULL, "_Edit" },

    { "undo", GIMP_ICON_EDIT_UNDO,
      N_("_Undo"), "<control>Z", NULL,
      G_CALLBACK (gfig_undo_action_callback) },

    { "clear", GIMP_ICON_EDIT_CLEAR,
      N_("_Clear"), NULL, NULL,
      G_CALLBACK (gfig_clear_action_callback) },

    { "grid", GIMP_ICON_GRID,
      N_("_Grid"), "<control>G", NULL,
      G_CALLBACK (gfig_grid_action_callback) },

    { "prefs", GIMP_ICON_PREFERENCES_SYSTEM,
      N_("_Preferences..."), "<control>P", NULL,
      G_CALLBACK (gfig_prefs_action_callback) },

    { "raise", GIMP_ICON_GO_UP,
      N_("_Raise"), "<control>U", N_("Raise selected object"),
      G_CALLBACK (raise_selected_obj) },

    { "lower", GIMP_ICON_GO_DOWN,
      N_("_Lower"), "<control>D", N_("Lower selected object"),
      G_CALLBACK (lower_selected_obj) },

    { "top", GIMP_ICON_GO_TOP,
      N_("Raise to _top"), "<control>T", N_("Raise selected object to top"),
      G_CALLBACK (raise_selected_obj_to_top) },

    { "bottom", GIMP_ICON_GO_BOTTOM,
      N_("Lower to _bottom"), "<control>B", N_("Lower selected object to bottom"),
      G_CALLBACK (lower_selected_obj_to_bottom) },

    { "show_previous", GIMP_ICON_GO_PREVIOUS,
      N_("_Previous"), "<control>H", N_("Show previous object"),
      G_CALLBACK (select_button_clicked_lt) },

    { "show_next", GIMP_ICON_GO_NEXT,
      N_("_Next"), "<control>L", N_("Show next object"),
      G_CALLBACK (select_button_clicked_gt) },

    { "show_all", GFIG_STOCK_SHOW_ALL,
      N_("Show _all"), "<control>A", N_("Show all objects"),
      G_CALLBACK (select_button_clicked_eq) }
  };
  static GtkRadioActionEntry radio_actions[] =
  {
    { "line", GFIG_STOCK_LINE,
      NULL, "L", N_("Create line"), LINE },

    { "rectangle", GFIG_STOCK_RECTANGLE,
      NULL, "R", N_("Create rectangle"), RECTANGLE },

    { "circle", GFIG_STOCK_CIRCLE,
      NULL, "C", N_("Create circle"), CIRCLE },

    { "ellipse", GFIG_STOCK_ELLIPSE,
      NULL, "E", N_("Create ellipse"), ELLIPSE },

    { "arc", GFIG_STOCK_CURVE,
      NULL, "A", N_("Create arc"), ARC },

    { "polygon", GFIG_STOCK_POLYGON,
      NULL, "P", N_("Create reg polygon"), POLY },

    { "star", GFIG_STOCK_STAR,
      NULL, "S", N_("Create star"), STAR },

    { "spiral", GFIG_STOCK_SPIRAL,
       NULL, "I", N_("Create spiral"), SPIRAL },

    { "bezier", GFIG_STOCK_BEZIER,
      NULL, "B", N_("Create bezier curve. "
                    "Shift + Button ends object creation."), BEZIER },

    { "move_obj", GFIG_STOCK_MOVE_OBJECT,
      NULL, "M", N_("Move an object"), MOVE_OBJ },

    { "move_point", GFIG_STOCK_MOVE_POINT,
      NULL, "V", N_("Move a single point"), MOVE_POINT },

    { "copy", GFIG_STOCK_COPY_OBJECT,
      NULL, "Y", N_("Copy an object"), COPY_OBJ },

    { "delete", GFIG_STOCK_DELETE_OBJECT,
      NULL, "D", N_("Delete an object"), DEL_OBJ },

    { "select", GFIG_STOCK_SELECT_OBJECT,
      NULL, "A", N_("Select an object"), SELECT_OBJ }
  };

  GtkUIManager   *ui_manager = gtk_ui_manager_new ();

  gfig_actions = gtk_action_group_new ("Actions");

  gtk_action_group_set_translation_domain (gfig_actions, NULL);

  gtk_action_group_add_actions (gfig_actions,
                                actions,
                                G_N_ELEMENTS (actions),
                                window);
  gtk_action_group_add_radio_actions (gfig_actions,
                                      radio_actions,
                                      G_N_ELEMENTS (radio_actions),
                                      LINE,
                                      G_CALLBACK (toggle_obj_type),
                                      window);

  gtk_window_add_accel_group (GTK_WINDOW (window),
                              gtk_ui_manager_get_accel_group (ui_manager));
  gtk_accel_group_lock (gtk_ui_manager_get_accel_group (ui_manager));

  gtk_ui_manager_insert_action_group (ui_manager, gfig_actions, -1);
  g_object_unref (gfig_actions);

  gtk_ui_manager_add_ui_from_string (ui_manager,
                                     "<ui>"
                                     "  <menubar name=\"gfig-menubar\">"
                                     "    <menu name=\"File\" action=\"gfig-file-menu\">"
                                     "      <menuitem action=\"open\" />"
                                     "      <menuitem action=\"save\" />"
                                     "      <menuitem action=\"close\" />"
                                     "    </menu>"
                                     "    <menu name=\"Edit\" action=\"gfig-edit-menu\">"
                                     "      <menuitem action=\"undo\" />"
                                     "      <menuitem action=\"clear\" />"
                                     "      <menuitem action=\"grid\" />"
                                     "      <menuitem action=\"prefs\" />"
                                     "    </menu>"
                                     "  </menubar>"
                                     "</ui>",
                                     -1, NULL);
  gtk_ui_manager_add_ui_from_string (ui_manager,
                                     "<ui>"
                                     "  <toolbar name=\"gfig-toolbar\">"
                                     "    <toolitem action=\"line\" />"
                                     "    <toolitem action=\"rectangle\" />"
                                     "    <toolitem action=\"circle\" />"
                                     "    <toolitem action=\"ellipse\" />"
                                     "    <toolitem action=\"arc\" />"
                                     "    <toolitem action=\"polygon\" />"
                                     "    <toolitem action=\"star\" />"
                                     "    <toolitem action=\"spiral\" />"
                                     "    <toolitem action=\"bezier\" />"
                                     "    <toolitem action=\"move_obj\" />"
                                     "    <toolitem action=\"move_point\" />"
                                     "    <toolitem action=\"copy\" />"
                                     "    <toolitem action=\"delete\" />"
                                     "    <toolitem action=\"select\" />"
                                     "    <separator />"
                                     "    <toolitem action=\"raise\" />"
                                     "    <toolitem action=\"lower\" />"
                                     "    <toolitem action=\"top\" />"
                                     "    <toolitem action=\"bottom\" />"
                                     "    <separator />"
                                     "    <toolitem action=\"show_previous\" />"
                                     "    <toolitem action=\"show_next\" />"
                                     "    <toolitem action=\"show_all\" />"
                                     "  </toolbar>"
                                     "</ui>",
                                     -1, NULL);

  return ui_manager;
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
raise_selected_obj_to_top (GtkWidget *widget,
                           gpointer   data)
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
lower_selected_obj_to_bottom (GtkWidget *widget,
                              gpointer   data)
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
raise_selected_obj (GtkWidget *widget,
                    gpointer   data)
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
lower_selected_obj (GtkWidget *widget,
                    gpointer   data)
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
gfig_prefs_action_callback (GtkAction *widget,
                            gpointer   data)
{
  static GtkWidget *dialog = NULL;

  if (!dialog)
    {
      GtkWidget     *main_vbox;
      GtkWidget     *grid;
      GtkWidget     *toggle;
      GtkAdjustment *size_data;
      GtkWidget     *scale;
      GtkAdjustment *scale_data;

      dialog = gimp_dialog_new (_("Options"), "gimp-gfig-options",
                                GTK_WIDGET (data), 0, NULL, NULL,

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

      size_data = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 0,
                                             _("Max undo:"), 100, 50,
                                             selvals.maxundo,
                                             MIN_UNDO, MAX_UNDO, 1, 2, 0,
                                             TRUE, 0, 0,
                                             NULL, NULL);
      g_signal_connect (size_data, "value-changed",
                        G_CALLBACK (gimp_int_adjustment_update),
                        &selvals.maxundo);

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
gfig_grid_action_callback (GtkAction *action,
                           gpointer   data)
{
  static GtkWidget *dialog = NULL;

  if (!dialog)
    {
      GtkWidget     *main_vbox;
      GtkWidget     *hbox;
      GtkWidget     *grid;
      GtkWidget     *combo;
      GtkAdjustment *size_data;
      GtkAdjustment *sectors_data;
      GtkAdjustment *radius_data;

      dialog = gimp_dialog_new (_("Grid"), "gimp-gfig-grid",
                                GTK_WIDGET (data), 0, NULL, NULL,

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

      size_data = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 0,
                                             _("Grid spacing:"), 100, 50,
                                             selvals.opts.gridspacing,
                                             MIN_GRID, MAX_GRID, 1, 10, 0,
                                             TRUE, 0, 0,
                                             NULL, NULL);
      g_signal_connect (size_data, "value-changed",
                        G_CALLBACK (gimp_int_adjustment_update),
                        &selvals.opts.gridspacing);
      g_signal_connect (size_data, "value-changed",
                        G_CALLBACK (draw_grid_clear),
                        NULL);

      gfig_opt_widget.gridspacing = size_data;
      g_object_add_weak_pointer (G_OBJECT (gfig_opt_widget.gridspacing),
                                 (gpointer) &gfig_opt_widget.gridspacing);

      sectors_data = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 3,
                                                _("Polar grid sectors desired:"), 1, 5,
                                                selvals.opts.grid_sectors_desired,
                                                5, 360, 5, 1, 0,
                                                TRUE, 0, 0,
                                                NULL, NULL);
      g_signal_connect (sectors_data, "value-changed",
                        G_CALLBACK (gimp_int_adjustment_update),
                        &selvals.opts.grid_sectors_desired);
      g_signal_connect (sectors_data, "value-changed",
                        G_CALLBACK (draw_grid_clear),
                        NULL);

      gfig_opt_widget.grid_sectors_desired = sectors_data;
      g_object_add_weak_pointer (G_OBJECT (gfig_opt_widget.grid_sectors_desired),
                                 (gpointer) &gfig_opt_widget.grid_sectors_desired);


      gfig_opt_widget.gridspacing = size_data;
      g_object_add_weak_pointer (G_OBJECT (gfig_opt_widget.gridspacing),
                                 (gpointer) &gfig_opt_widget.gridspacing);

      radius_data = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 4,
                                               _("Polar grid radius interval:"), 1, 5,
                                               selvals.opts.grid_radius_interval,
                                               5, 50, 5, 1, 0,
                                               TRUE, 0, 0,
                                               NULL, NULL);
      g_signal_connect (radius_data, "value-changed",
                        G_CALLBACK (gimp_double_adjustment_update),
                        &selvals.opts.grid_radius_interval);
      g_signal_connect (radius_data, "value-changed",
                        G_CALLBACK (draw_grid_clear),
                        NULL);

      gfig_opt_widget.grid_radius_interval = radius_data;
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
        gtk_adjustment_set_value (gfig_opt_widget.gridspacing,
                                  gfig_context->current_obj->opts.gridspacing);
    }
  if (selvals.opts.grid_sectors_desired != gfig_context->current_obj->opts.grid_sectors_desired)
    {
      if (gfig_opt_widget.grid_sectors_desired)
        gtk_adjustment_set_value (gfig_opt_widget.grid_sectors_desired,
                                  gfig_context->current_obj->opts.grid_sectors_desired);
    }
  if (selvals.opts.grid_radius_interval != gfig_context->current_obj->opts.grid_radius_interval)
    {
      if (gfig_opt_widget.grid_radius_interval)
        gtk_adjustment_set_value (gfig_opt_widget.grid_radius_interval,
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
select_button_clicked_lt (void)
{
  select_button_clicked (OBJ_SELECT_LT);
}

static void
select_button_clicked_gt (void)
{
  select_button_clicked (OBJ_SELECT_GT);
}

static void
select_button_clicked_eq (void)
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
  GtkWidget     *grid;
  GtkAdjustment *size_data;

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_widget_show (grid);

  size_data = gimp_scale_entry_new_grid (GTK_GRID (grid), 0, 0,
                                         _("Sides:"), 0, 0,
                                         *num_sides, adj_min, adj_max, 1, 10, 0,
                                         TRUE, 0, 0,
                                         NULL, NULL);
  g_signal_connect (size_data, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    num_sides);

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
gfig_paint (BrushType brush_type,
            gint32    drawable_ID,
            gint      seg_count,
            gdouble   line_pnts[])
{
  switch (brush_type)
    {
    case BRUSH_BRUSH_TYPE:
      gimp_paintbrush (drawable_ID,
                       selvals.brushfade,
                       seg_count, line_pnts,
                       GIMP_PAINT_CONSTANT,
                       selvals.brushgradient);
      break;

    case BRUSH_PENCIL_TYPE:
      gimp_pencil (drawable_ID,
                   seg_count, line_pnts);
      break;

    case BRUSH_AIRBRUSH_TYPE:
      gimp_airbrush (drawable_ID,
                     selvals.airbrushpressure,
                     seg_count, line_pnts);
      break;

    case BRUSH_PATTERN_TYPE:
      gimp_clone (drawable_ID,
                  drawable_ID,
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
      GFigObj *gfig;
      GFigObj *current_saved;

      filename = gtk_file_chooser_get_filename (chooser);

      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        {
          /* Hack - current object MUST be NULL to prevent setup_undo ()
           * from kicking in.
           */
          current_saved = gfig_context->current_obj;
          gfig_context->current_obj = NULL;
          gfig = gfig_load (filename, filename);
          gfig_context->current_obj = current_saved;

          if (gfig)
            {
              /* Read only ?*/
              if (access (filename, W_OK))
                gfig->obj_status |= GFIG_READONLY;

              gfig_list_insert (gfig);
              new_obj_2edit (gfig);
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
      fill_type = GIMP_FILL_FOREGROUND;
      break;

    case FILL_PATTERN:
      fill_type = GIMP_FILL_PATTERN;
      break;

    case FILL_GRADIENT:
      gimp_drawable_edit_gradient_fill (gfig_context->drawable_id,
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
      gimp_drawable_edit_gradient_fill (gfig_context->drawable_id,
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
      gimp_drawable_edit_gradient_fill (gfig_context->drawable_id,
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

  gimp_drawable_edit_fill (gfig_context->drawable_id,
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

  gimp_drawable_fill (gfig_context->drawable_id, GIMP_FILL_TRANSPARENT);

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
toggle_obj_type (GtkRadioAction *action,
                 GtkRadioAction *current,
                 gpointer        data)
{
  static GdkCursor *p_cursors[DEL_OBJ + 1];
  GdkCursorType     ctype = GDK_LAST_CURSOR;
  DobjType          new_type;

  new_type = gtk_radio_action_get_current_value (action);
  if (selvals.otype != new_type)
    {
      /* Mem leak */
      obj_creating = NULL;
      tmp_line = NULL;
      tmp_bezier = NULL;

      if (new_type < MOVE_OBJ) /* Eeeeek */
        {
          obj_show_single = -1; /* Cancel select preview */
        }
      /* Update draw areas */
      gtk_widget_queue_draw (gfig_context->preview);
    }

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
    default:
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
    }

  if (!p_cursors[selvals.otype])
    {
      GdkDisplay *display = gtk_widget_get_display (gfig_context->preview);

      p_cursors[selvals.otype] = gdk_cursor_new_for_display (display, ctype);
    }

  gdk_window_set_cursor (gtk_widget_get_window (gfig_context->preview),
                         p_cursors[selvals.otype]);
}

/* This could belong in a separate file ... but makes it easier to lump into
 * one when compiling the plugin.
 */

/* Given a number of float co-ords adjust for scaling back to org size */
/* Size is number of PAIRS of points */
/* FP + int varients */

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
