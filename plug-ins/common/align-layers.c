/* align_layers.c
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 * Version:  0.26
 *
 * Copyright (C) 1997-1998 Shuji Narazaki <narazaki@InetQ.or.jp>
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define PLUG_IN_PROC   "plug-in-align-layers"
#define PLUG_IN_BINARY "align-layers"
#define PLUG_IN_ROLE   "gimp-align-layers"
#define SCALE_WIDTH    150

enum
{
  H_NONE,
  H_COLLECT,
  LEFT2RIGHT,
  RIGHT2LEFT,
  SNAP2HGRID
};

enum
{
  H_BASE_LEFT,
  H_BASE_CENTER,
  H_BASE_RIGHT
};

enum
{
  V_NONE,
  V_COLLECT,
  TOP2BOTTOM,
  BOTTOM2TOP,
  SNAP2VGRID
};

enum
{
  V_BASE_TOP,
  V_BASE_CENTER,
  V_BASE_BOTTOM
};


typedef struct
{
  gint step_x;
  gint step_y;
  gint base_x;
  gint base_y;
} AlignData;

typedef struct _AlignLayers      AlignLayers;
typedef struct _AlignLayersClass AlignLayersClass;

struct _AlignLayers
{
  GimpPlugIn      parent_instance;
};

struct _AlignLayersClass
{
  GimpPlugInClass parent_class;
};


#define ALIGN_LAYERS_TYPE  (align_layers_get_type ())
#define ALIGN_LAYERS (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ALIGN_LAYERS_TYPE, AlignLayers))

GType                   align_layers_get_type               (void) G_GNUC_CONST;

static GList          * align_layers_query_procedures       (GimpPlugIn           *plug_in);
static GimpProcedure  * align_layers_create_procedure       (GimpPlugIn           *plug_in,
                                                             const gchar          *name);

static GimpValueArray * align_layers_run                    (GimpProcedure        *procedure,
                                                             GimpRunMode           run_mode,
                                                             GimpImage            *image,
                                                             GimpDrawable         *drawable,
                                                             const GimpValueArray *args,
                                                             gpointer              run_data);


/* Main function */
static GimpPDBStatusType align_layers                       (GimpImage            *image);

/* Helpers and internal functions */
static gint             align_layers_count_visibles_layers  (GList                *layers);
static GimpLayer      * align_layers_find_last_layer        (GList                *layers,
                                                             gboolean             *found);
static gint             align_layers_spread_visibles_layers (GList                *layers,
                                                             GimpLayer           **layers_array);
static GimpLayer     ** align_layers_spread_image           (GimpImage            *image,
                                                             gint                 *layer_num);
static GimpLayer      * align_layers_find_background        (GimpImage            *image);
static AlignData        align_layers_gather_data            (GimpLayer           **layers,
                                                             gint                  layer_num,
                                                             GimpLayer            *background);
static void             align_layers_perform_alignment      (GimpLayer           **layers,
                                                             gint                  layer_num,
                                                             AlignData             data);
static void             align_layers_get_align_offsets      (GimpDrawable         *drawable,
                                                             gint                 *x,
                                                             gint                 *y);
static gint             align_layers_dialog                 (void);


G_DEFINE_TYPE (AlignLayers, align_layers, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (ALIGN_LAYERS_TYPE)

/* dialog variables */
typedef struct
{
  gint     h_style;
  gint     h_base;
  gint     v_style;
  gint     v_base;
  gboolean ignore_bottom;
  gboolean base_is_bottom_layer;
  gint     grid_size;
} ValueType;

static ValueType VALS =
{
  H_NONE,
  H_BASE_LEFT,
  V_NONE,
  V_BASE_TOP,
  TRUE,
  FALSE,
  10
};

static void
align_layers_class_init (AlignLayersClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = align_layers_query_procedures;
  plug_in_class->create_procedure = align_layers_create_procedure;
}

static void
align_layers_init (AlignLayers *film)
{
}

static GList *
align_layers_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
align_layers_create_procedure (GimpPlugIn  *plug_in,
                               const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            align_layers_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("Align Visi_ble Layers..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Image/Arrange");

      gimp_procedure_set_documentation (procedure,
                                        N_("Align all visible layers of the image"),
                                        "Align visible layers",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Shuji Narazaki <narazaki@InetQ.or.jp>",
                                      "Shuji Narazaki",
                                      "1997");

      /* TODO: these 2 arguments existed in the original procedure but
       * were actually unused. If we want to keep them, let's at least
       * implement the documented behavior, or else let's just drop
       * them.
       */
      /*
      GIMP_PROC_ARG_BOOLEAN (procedure,
                             "link-after-alignment",
                             "Link the visible layers after alignment",
                             FALSE,
                             G_PARAM_READWRITE);
      GIMP_PROC_ARG_BOOLEAN (procedure,
                             "use-bottom",
                             "use the bottom layer as the base of alignment",
                             TRUE,
                             G_PARAM_READWRITE);
                             */
    }

  return procedure;
}

static GimpValueArray *
align_layers_run (GimpProcedure        *procedure,
                  GimpRunMode           run_mode,
                  GimpImage            *image,
                  GimpDrawable         *drawable,
                  const GimpValueArray *args,
                  gpointer              run_data)
{
  GimpValueArray    *return_vals = NULL;
  GimpPDBStatusType  status = GIMP_PDB_EXECUTION_ERROR;
  GError            *error  = NULL;
  GList             *layers;
  gint               layer_num;

  INIT_I18N ();

  switch ( run_mode )
    {
    case GIMP_RUN_INTERACTIVE:
      layers = gimp_image_list_layers (image);
      layer_num = align_layers_count_visibles_layers (layers);
      g_list_free (layers);
      if (layer_num < 2)
        {
          error = g_error_new_literal (0, 0,
                                       _("There are not enough layers to align."));
          break;
        }
      gimp_get_data (PLUG_IN_PROC, &VALS);
      VALS.grid_size = MAX (VALS.grid_size, 1);
      if (! align_layers_dialog ())
        status = GIMP_PDB_CANCEL;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &VALS);
      break;
    }

  if (status != GIMP_PDB_CANCEL)
    {
      status = align_layers (image);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }

  if (run_mode == GIMP_RUN_INTERACTIVE && status == GIMP_PDB_SUCCESS)
    gimp_set_data (PLUG_IN_PROC, &VALS, sizeof (ValueType));

  return_vals = gimp_procedure_new_return_values (procedure, status,
                                                  error);

  return return_vals;
}

/*
 * Main function
 */
static GimpPDBStatusType
align_layers (GimpImage *image)
{
  gint       layer_num  = 0;
  GimpLayer  **layers     = NULL;
  GimpLayer   *background = 0;
  AlignData  data;

  layers = align_layers_spread_image (image, &layer_num);
  if (layer_num < 2)
    {
      g_free (layers);
      return GIMP_PDB_EXECUTION_ERROR;
    }

  background = align_layers_find_background (image);

  /* If we want to ignore the bottom layer and if it's visible */
  if (VALS.ignore_bottom && background == layers[layer_num - 1])
    {
      layer_num--;
    }

  data = align_layers_gather_data (layers,
                                   layer_num,
                                   background);

  gimp_image_undo_group_start (image);

  align_layers_perform_alignment (layers,
                                  layer_num,
                                  data);

  gimp_image_undo_group_end (image);

  g_free (layers);

  return GIMP_PDB_SUCCESS;
}

/*
 * Find the bottommost layer, visible or not
 * The image must contain at least one layer.
 */
static GimpLayer *
align_layers_find_last_layer (GList    *layers,
                              gboolean *found)
{
  GList *last = g_list_last (layers);

  for (; last; last = last->prev)
    {
      GimpItem *item = last->data;

      if (gimp_item_is_group (item))
        {
          GList     *children;
          GimpLayer *last_layer;

          children = gimp_item_list_children (item);
          last_layer = align_layers_find_last_layer (children,
                                                     found);
          g_list_free (children);
          if (*found)
            return last_layer;
        }
      else if (gimp_item_is_layer (item))
        {
          *found = TRUE;
          return GIMP_LAYER (item);
        }
    }

  /* should never happen */
  return NULL;
}

/*
 * Return the bottom layer.
 */
static GimpLayer *
align_layers_find_background (GimpImage *image)
{
  GList     *layers;
  GimpLayer *background;
  gboolean   found = FALSE;

  layers = gimp_image_list_layers (image);
  background = align_layers_find_last_layer (layers, &found);
  g_list_free (layers);

  return background;
}

/*
 * Fill layers_array with all visible layers.
 * layers_array needs to be allocated before the call
 */
static gint
align_layers_spread_visibles_layers (GList      *layers,
                                     GimpLayer **layers_array)
{
  GList *iter;
  gint   index = 0;

  for (iter = layers; iter; iter = iter->next)
    {
      GimpItem *item = iter->data;

      if (gimp_item_get_visible (item))
        {
          if (gimp_item_is_group (item))
            {
              GList *children;

              children = gimp_item_list_children (item);
              index += align_layers_spread_visibles_layers (children,
                                                            &(layers_array[index]));
              g_list_free (children);
            }
          else if (gimp_item_is_layer (item))
            {
              layers_array[index] = GIMP_LAYER (item);
              index++;
            }
        }
    }

  return index;
}

/*
 * Return a contiguous array of all visible layers
 */
static GimpLayer **
align_layers_spread_image (GimpImage *image,
                           gint      *layer_num)
{
  GList      *layers;
  GimpLayer **layers_array;

  layers = gimp_image_list_layers (image);
  *layer_num = align_layers_count_visibles_layers (layers);

  layers_array = g_malloc (sizeof (GimpLayer *) * *layer_num);

  align_layers_spread_visibles_layers (layers,
                                       layers_array);
  g_list_free (layers);

  return layers_array;
}

static gint
align_layers_count_visibles_layers (GList *layers)
{
  GList *iter;
  gint   count = 0;

  for (iter = layers; iter; iter = iter->next)
    {
      GimpItem *item = iter->data;

      if (gimp_item_get_visible (item))
        {
          if (gimp_item_is_group (item))
            {
              GList *children;

              children = gimp_item_list_children (item);
              count += align_layers_count_visibles_layers (children);
              g_list_free (children);
            }
          else if (gimp_item_is_layer (item))
            {
              count += 1;
            }
        }
    }

  return count;
}

static AlignData
align_layers_gather_data (GimpLayer **layers,
                          gint        layer_num,
                          GimpLayer  *background)
{
  AlignData data;
  gint   min_x = G_MAXINT;
  gint   min_y = G_MAXINT;
  gint   max_x = G_MININT;
  gint   max_y = G_MININT;
  gint   index;
  gint   orig_x   = 0;
  gint   orig_y   = 0;
  gint   offset_x = 0;
  gint   offset_y = 0;

  data.step_x = 0;
  data.step_y = 0;
  data.base_x = 0;
  data.base_y = 0;

  /* 0 is the top layer */
  for (index = 0; index < layer_num; index++)
    {
      gimp_drawable_offsets (GIMP_DRAWABLE (layers[index]), &orig_x, &orig_y);

      align_layers_get_align_offsets (GIMP_DRAWABLE (layers[index]),
                                      &offset_x,
                                      &offset_y);
      orig_x += offset_x;
      orig_y += offset_y;

      min_x = MIN (min_x, orig_x);
      max_x = MAX (max_x, orig_x);
      min_y = MIN (min_y, orig_y);
      max_y = MAX (max_y, orig_y);
    }

  if (VALS.base_is_bottom_layer)
    {
      gimp_drawable_offsets (GIMP_DRAWABLE (background), &orig_x, &orig_y);

      align_layers_get_align_offsets (GIMP_DRAWABLE (background),
                                      &offset_x,
                                      &offset_y);
      orig_x += offset_x;
      orig_y += offset_y;
      data.base_x = min_x = orig_x;
      data.base_y = min_y = orig_y;
    }

  if (layer_num > 1)
    {
      data.step_x = (max_x - min_x) / (layer_num - 1);
      data.step_y = (max_y - min_y) / (layer_num - 1);
    }

  if ( (VALS.h_style == LEFT2RIGHT) || (VALS.h_style == RIGHT2LEFT))
    data.base_x = min_x;

  if ( (VALS.v_style == TOP2BOTTOM) || (VALS.v_style == BOTTOM2TOP))
    data.base_y = min_y;

  return data;
}

/*
 * Modifies position of each visible layers
 * according to data.
 */
static void
align_layers_perform_alignment (GimpLayer **layers,
                                gint        layer_num,
                                AlignData   data)
{
  gint index;

  for (index = 0; index < layer_num; index++)
    {
      gint x = 0;
      gint y = 0;
      gint orig_x;
      gint orig_y;
      gint offset_x;
      gint offset_y;

      gimp_drawable_offsets (GIMP_DRAWABLE (layers[index]), &orig_x, &orig_y);

      align_layers_get_align_offsets (GIMP_DRAWABLE (layers[index]),
                                      &offset_x,
                                      &offset_y);
      switch (VALS.h_style)
        {
        case H_NONE:
          x = orig_x;
          break;
        case H_COLLECT:
          x = data.base_x - offset_x;
          break;
        case LEFT2RIGHT:
          x = (data.base_x + index * data.step_x) - offset_x;
          break;
        case RIGHT2LEFT:
          x = (data.base_x + (layer_num - index - 1) * data.step_x) - offset_x;
          break;
        case SNAP2HGRID:
          x = VALS.grid_size
            * (int) ((orig_x + offset_x + VALS.grid_size /2) / VALS.grid_size)
            - offset_x;
          break;
        }

      switch (VALS.v_style)
        {
        case V_NONE:
          y = orig_y;
          break;
        case V_COLLECT:
          y = data.base_y - offset_y;
          break;
        case TOP2BOTTOM:
          y = (data.base_y + index * data.step_y) - offset_y;
          break;
        case BOTTOM2TOP:
          y = (data.base_y + (layer_num - index - 1) * data.step_y) - offset_y;
          break;
        case SNAP2VGRID:
          y = VALS.grid_size
            * (int) ((orig_y + offset_y + VALS.grid_size / 2) / VALS.grid_size)
            - offset_y;
          break;
        }

      gimp_layer_set_offsets (layers[index], x, y);
    }
}

static void
align_layers_get_align_offsets (GimpDrawable *drawable,
                                gint         *x,
                                gint         *y)
{
  gint width  = gimp_drawable_width  (drawable);
  gint height = gimp_drawable_height (drawable);

  switch (VALS.h_base)
    {
    case H_BASE_LEFT:
      *x = 0;
      break;
    case H_BASE_CENTER:
      *x = width / 2;
      break;
    case H_BASE_RIGHT:
      *x = width;
      break;
    default:
      *x = 0;
      break;
    }

  switch (VALS.v_base)
    {
    case V_BASE_TOP:
      *y = 0;
      break;
    case V_BASE_CENTER:
      *y = height / 2;
      break;
    case V_BASE_BOTTOM:
      *y = height;
      break;
    default:
      *y = 0;
      break;
    }
}

static int
align_layers_dialog (void)
{
  GtkWidget     *dialog;
  GtkWidget     *grid;
  GtkWidget     *combo;
  GtkWidget     *toggle;
  GtkAdjustment *adj;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_dialog_new (_("Align Visible Layers"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  combo = gimp_int_combo_box_new (C_("align-style", "None"), H_NONE,
                                  _("Collect"),              H_COLLECT,
                                  _("Fill (left to right)"), LEFT2RIGHT,
                                  _("Fill (right to left)"), RIGHT2LEFT,
                                  _("Snap to grid"),         SNAP2HGRID,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), VALS.h_style);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_int_combo_box_get_active),
                    &VALS.h_style);

  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Horizontal style:"), 0.0, 0.5,
                            combo, 2);


  combo = gimp_int_combo_box_new (_("Left edge"),  H_BASE_LEFT,
                                  _("Center"),     H_BASE_CENTER,
                                  _("Right edge"), H_BASE_RIGHT,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), VALS.h_base);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_int_combo_box_get_active),
                    &VALS.h_base);

  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Ho_rizontal base:"), 0.0, 0.5,
                            combo, 2);

  combo = gimp_int_combo_box_new (C_("align-style", "None"), V_NONE,
                                  _("Collect"),              V_COLLECT,
                                  _("Fill (top to bottom)"), TOP2BOTTOM,
                                  _("Fill (bottom to top)"), BOTTOM2TOP,
                                  _("Snap to grid"),         SNAP2VGRID,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), VALS.v_style);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_int_combo_box_get_active),
                    &VALS.v_style);

  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("_Vertical style:"), 0.0, 0.5,
                            combo, 2);

  combo = gimp_int_combo_box_new (_("Top edge"),    V_BASE_TOP,
                                  _("Center"),      V_BASE_CENTER,
                                  _("Bottom edge"), V_BASE_BOTTOM,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), VALS.v_base);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_int_combo_box_get_active),
                    &VALS.v_base);

  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                            _("Ver_tical base:"), 0.0, 0.5,
                            combo, 2);

  adj = gimp_scale_entry_new (GTK_GRID (grid), 0, 4,
                              _("_Grid size:"), SCALE_WIDTH, 0,
                              VALS.grid_size, 1, 200, 1, 10, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &VALS.grid_size);

  toggle = gtk_check_button_new_with_mnemonic
    (_("_Ignore the bottom layer even if visible"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), VALS.ignore_bottom);
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, 5, 3, 1);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &VALS.ignore_bottom);

  toggle = gtk_check_button_new_with_mnemonic
    (_("_Use the (invisible) bottom layer as the base"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                VALS.base_is_bottom_layer);
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, 6, 3, 1);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &VALS.base_is_bottom_layer);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
