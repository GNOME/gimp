/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpgradient.h"

#include "widgets/gimpdataeditor.h"
#include "widgets/gimpgradienteditor.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"

#include "actions/gradient-editor-commands.h"

#include "gradient-editor-menu.h"
#include "menus.h"

#include "gimp-intl.h"


#define LOAD_LEFT_FROM(num,magic) \
  { { "/Load Left Color From/" num, NULL, \
      gradient_editor_load_left_cmd_callback, (magic) }, \
    NULL, GIMP_HELP_GRADIENT_EDITOR_LEFT_LOAD, NULL }
#define SAVE_LEFT_TO(num,magic) \
  { { "/Save Left Color To/" num, NULL, \
      gradient_editor_save_left_cmd_callback, (magic) }, \
    NULL, GIMP_HELP_GRADIENT_EDITOR_LEFT_SAVE, NULL }
#define LOAD_RIGHT_FROM(num,magic) \
  { { "/Load Right Color From/" num, NULL, \
      gradient_editor_load_right_cmd_callback, (magic) }, \
    NULL, GIMP_HELP_GRADIENT_EDITOR_RIGHT_LOAD, NULL }
#define SAVE_RIGHT_TO(num,magic) \
  { { "/Save Right Color To/" num, NULL, \
      gradient_editor_save_right_cmd_callback, (magic) }, \
    NULL, GIMP_HELP_GRADIENT_EDITOR_RIGHT_SAVE, NULL }


GimpItemFactoryEntry gradient_editor_menu_entries[] =
{
  { { N_("/L_eft Endpoint's Color..."), NULL,
      gradient_editor_left_color_cmd_callback, 0 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_LEFT_COLOR, NULL },

  MENU_BRANCH (N_("/_Load Left Color From")),

  { { N_("/Load Left Color From/_Left Neighbor's Right Endpoint"), NULL,
      gradient_editor_load_left_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { N_("/Load Left Color From/_Right Endpoint"), NULL,
      gradient_editor_load_left_cmd_callback, 1 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_LEFT_LOAD, NULL },
  { { N_("/Load Left Color From/_FG Color"), NULL,
      gradient_editor_load_left_cmd_callback, 2 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_LEFT_LOAD, NULL },
  { { N_("/Load Left Color From/_BG Color"), NULL,
      gradient_editor_load_left_cmd_callback, 3 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_LEFT_LOAD, NULL },

  MENU_SEPARATOR ("/Load Left Color From/---"),

  LOAD_LEFT_FROM ("01", 4),
  LOAD_LEFT_FROM ("02", 5),
  LOAD_LEFT_FROM ("03", 6),
  LOAD_LEFT_FROM ("04", 7),
  LOAD_LEFT_FROM ("05", 8),
  LOAD_LEFT_FROM ("06", 9),
  LOAD_LEFT_FROM ("07", 10),
  LOAD_LEFT_FROM ("08", 11),
  LOAD_LEFT_FROM ("09", 12),
  LOAD_LEFT_FROM ("10", 13),

  MENU_BRANCH (N_("/_Save Left Color To")),

  SAVE_LEFT_TO ("01", 0),
  SAVE_LEFT_TO ("02", 1),
  SAVE_LEFT_TO ("03", 2),
  SAVE_LEFT_TO ("04", 3),
  SAVE_LEFT_TO ("05", 4),
  SAVE_LEFT_TO ("06", 5),
  SAVE_LEFT_TO ("07", 6),
  SAVE_LEFT_TO ("08", 7),
  SAVE_LEFT_TO ("09", 8),
  SAVE_LEFT_TO ("10", 9),

  MENU_SEPARATOR ("/---"),

  { { N_("/R_ight Endpoint's Color..."), NULL,
      gradient_editor_right_color_cmd_callback, 0 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_RIGHT_COLOR, NULL },

  MENU_BRANCH (N_("/Load Right Color Fr_om")),

  { { N_("/Load Right Color From/_Right Neighbor's Left Endpoint"), NULL,
      gradient_editor_load_right_cmd_callback, 0 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_RIGHT_LOAD, NULL },
  { { N_("/Load Right Color From/_Left Endpoint"), NULL,
      gradient_editor_load_right_cmd_callback, 1 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_RIGHT_LOAD, NULL },
  { { N_("/Load Right Color From/_FG Color"), NULL,
      gradient_editor_load_right_cmd_callback, 2 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_RIGHT_LOAD, NULL },
  { { N_("/Load Right Color From/_BG Color"), NULL,
      gradient_editor_load_right_cmd_callback, 3 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_RIGHT_LOAD, NULL },

  MENU_SEPARATOR ("/Load Right Color From/---"),

  LOAD_RIGHT_FROM ("01", 4),
  LOAD_RIGHT_FROM ("02", 5),
  LOAD_RIGHT_FROM ("03", 6),
  LOAD_RIGHT_FROM ("04", 7),
  LOAD_RIGHT_FROM ("05", 8),
  LOAD_RIGHT_FROM ("06", 9),
  LOAD_RIGHT_FROM ("07", 10),
  LOAD_RIGHT_FROM ("08", 11),
  LOAD_RIGHT_FROM ("09", 12),
  LOAD_RIGHT_FROM ("10", 13),

  MENU_BRANCH (N_("/Sa_ve Right Color To")),

  SAVE_RIGHT_TO ("01", 0),
  SAVE_RIGHT_TO ("02", 1),
  SAVE_RIGHT_TO ("03", 2),
  SAVE_RIGHT_TO ("04", 3),
  SAVE_RIGHT_TO ("05", 4),
  SAVE_RIGHT_TO ("06", 5),
  SAVE_RIGHT_TO ("07", 6),
  SAVE_RIGHT_TO ("08", 7),
  SAVE_RIGHT_TO ("09", 8),
  SAVE_RIGHT_TO ("10", 9),

  MENU_SEPARATOR ("/---"),

  { { N_("/blendingfunction/_Linear"), NULL,
      gradient_editor_blending_func_cmd_callback,
      GIMP_GRAD_LINEAR, "<RadioItem>" },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_BLENDING, NULL },
  { { N_("/blendingfunction/_Curved"), NULL,
      gradient_editor_blending_func_cmd_callback,
      GIMP_GRAD_CURVED, "/blendingfunction/Linear" },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_BLENDING, NULL },
  { { N_("/blendingfunction/_Sinusodial"), NULL,
      gradient_editor_blending_func_cmd_callback,
      GIMP_GRAD_SINE, "/blendingfunction/Linear" },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_BLENDING, NULL },
  { { N_("/blendingfunction/Spherical (i_ncreasing)"), NULL,
      gradient_editor_blending_func_cmd_callback,
      GIMP_GRAD_SPHERE_INCREASING, "/blendingfunction/Linear" },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_BLENDING, NULL },
  { { N_("/blendingfunction/Spherical (_decreasing)"), NULL,
      gradient_editor_blending_func_cmd_callback,
      GIMP_GRAD_SPHERE_DECREASING, "/blendingfunction/Linear" },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_BLENDING, NULL },
  { { N_("/blendingfunction/(Varies)"), NULL, NULL,
      0, "/blendingfunction/Linear" },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_BLENDING, NULL },

  { { N_("/coloringtype/_RGB"), NULL,
      gradient_editor_coloring_type_cmd_callback,
      GIMP_GRAD_RGB, "<RadioItem>" },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_COLORING, NULL },
  { { N_("/coloringtype/HSV (_counter-clockwise hue)"), NULL,
      gradient_editor_coloring_type_cmd_callback,
      GIMP_GRAD_HSV_CCW, "/coloringtype/RGB" },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_COLORING, NULL },
  { { N_("/coloringtype/HSV (clockwise _hue)"), NULL,
      gradient_editor_coloring_type_cmd_callback,
      GIMP_GRAD_HSV_CW, "/coloringtype/RGB" },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_COLORING, NULL },
  { { N_("/coloringtype/(Varies)"), NULL, NULL,
      0, "/coloringtype/RGB" },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_COLORING, NULL },

  MENU_SEPARATOR ("/---"),

  { { "/flip", "F",
      gradient_editor_flip_cmd_callback, 0 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_FLIP, NULL },
  { { "/replicate", "R",
      gradient_editor_replicate_cmd_callback, 0 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_FLIP, NULL },
  { { "/splitmidpoint", "S",
      gradient_editor_split_midpoint_cmd_callback, 0 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_SPLIT_MIDPOINT, NULL },
  { { "/splituniformly", "U",
      gradient_editor_split_uniformly_cmd_callback, 0 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_SPLIT_MIDPOINT, NULL },
  { { "/delete", "D",
      gradient_editor_delete_cmd_callback, 0 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_DELETE, NULL },
  { { "/recenter", "C",
      gradient_editor_recenter_cmd_callback, 0 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_RECENTER, NULL },
  { { "/redistribute", "<control>C",
      gradient_editor_redistribute_cmd_callback, 0 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_REDISTRIBUTE, NULL },

  MENU_SEPARATOR ("/---"),

  { { N_("/Ble_nd Endpoints' Colors"), "B",
      gradient_editor_blend_color_cmd_callback, 0 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_BLEND_COLOR, NULL },
  { { N_("/Blend Endpoints' Opacit_y"), "<control>B",
      gradient_editor_blend_opacity_cmd_callback, 0 },
    NULL,
    GIMP_HELP_GRADIENT_EDITOR_BLEND_OPACITY, NULL },
};

#undef LOAD_LEFT_FROM
#undef SAVE_LEFT_TO
#undef LOAD_RIGHT_FROM
#undef SAVE_RIGHT_TO

gint n_gradient_editor_menu_entries = G_N_ELEMENTS (gradient_editor_menu_entries);


void
gradient_editor_menu_update (GtkItemFactory *factory,
                             gpointer        data)
{
  GimpGradientEditor  *editor;
  GimpContext         *user_context;
  GimpGradientSegment *left_seg;
  GimpGradientSegment *right_seg;
  GimpRGB              fg;
  GimpRGB              bg;
  gboolean             blending_equal = TRUE;
  gboolean             coloring_equal = TRUE;
  gboolean             selection;
  gboolean             delete;

  editor = GIMP_GRADIENT_EDITOR (data);

  user_context = gimp_get_user_context (GIMP_DATA_EDITOR (editor)->data_factory->gimp);

  if (editor->control_sel_l->prev)
    left_seg = editor->control_sel_l->prev;
  else
    left_seg = gimp_gradient_segment_get_last (editor->control_sel_l);

  if (editor->control_sel_r->next)
    right_seg = editor->control_sel_r->next;
  else
    right_seg = gimp_gradient_segment_get_first (editor->control_sel_r);

  gimp_context_get_foreground (user_context, &fg);
  gimp_context_get_background (user_context, &bg);

  {
    GimpGradientSegmentType  type;
    GimpGradientSegmentColor color;
    GimpGradientSegment     *seg, *aseg;

    type  = editor->control_sel_l->type;
    color = editor->control_sel_l->color;

    seg = editor->control_sel_l;

    do
      {
        blending_equal = blending_equal && (seg->type == type);
        coloring_equal = coloring_equal && (seg->color == color);

        aseg = seg;
        seg  = seg->next;
      }
    while (aseg != editor->control_sel_r);
  }

  selection = (editor->control_sel_l != editor->control_sel_r);
  delete    = (editor->control_sel_l->prev || editor->control_sel_r->next);

#define SET_ACTIVE(menu,active) \
        gimp_item_factory_set_active (factory, menu, (active))
#define SET_COLOR(menu,color,set_label) \
        gimp_item_factory_set_color (factory, menu, (color), (set_label))
#define SET_LABEL(menu,label) \
        gimp_item_factory_set_label (factory, menu, (label))
#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)
#define SET_VISIBLE(menu,condition) \
        gimp_item_factory_set_visible (factory, menu, (condition) != 0)

  SET_COLOR ("/Left Endpoint's Color...",
             &editor->control_sel_l->left_color, FALSE);
  SET_COLOR ("/Load Left Color From/Left Neighbor's Right Endpoint",
             &left_seg->right_color, FALSE);
  SET_COLOR ("/Load Left Color From/Right Endpoint",
             &editor->control_sel_r->right_color, FALSE);
  SET_COLOR ("/Load Left Color From/FG Color", &fg, FALSE);
  SET_COLOR ("/Load Left Color From/BG Color", &bg, FALSE);

  SET_COLOR ("/Load Left Color From/01", &editor->saved_colors[0], TRUE);
  SET_COLOR ("/Load Left Color From/02", &editor->saved_colors[1], TRUE);
  SET_COLOR ("/Load Left Color From/03", &editor->saved_colors[2], TRUE);
  SET_COLOR ("/Load Left Color From/04", &editor->saved_colors[3], TRUE);
  SET_COLOR ("/Load Left Color From/05", &editor->saved_colors[4], TRUE);
  SET_COLOR ("/Load Left Color From/06", &editor->saved_colors[5], TRUE);
  SET_COLOR ("/Load Left Color From/07", &editor->saved_colors[6], TRUE);
  SET_COLOR ("/Load Left Color From/08", &editor->saved_colors[7], TRUE);
  SET_COLOR ("/Load Left Color From/09", &editor->saved_colors[8], TRUE);
  SET_COLOR ("/Load Left Color From/10", &editor->saved_colors[9], TRUE);

  SET_COLOR ("/Save Left Color To/01", &editor->saved_colors[0], TRUE);
  SET_COLOR ("/Save Left Color To/02", &editor->saved_colors[1], TRUE);
  SET_COLOR ("/Save Left Color To/03", &editor->saved_colors[2], TRUE);
  SET_COLOR ("/Save Left Color To/04", &editor->saved_colors[3], TRUE);
  SET_COLOR ("/Save Left Color To/05", &editor->saved_colors[4], TRUE);
  SET_COLOR ("/Save Left Color To/06", &editor->saved_colors[5], TRUE);
  SET_COLOR ("/Save Left Color To/07", &editor->saved_colors[6], TRUE);
  SET_COLOR ("/Save Left Color To/08", &editor->saved_colors[7], TRUE);
  SET_COLOR ("/Save Left Color To/09", &editor->saved_colors[8], TRUE);
  SET_COLOR ("/Save Left Color To/10", &editor->saved_colors[9], TRUE);

  SET_COLOR ("/Right Endpoint's Color...",
             &editor->control_sel_r->right_color, FALSE);
  SET_COLOR ("/Load Right Color From/Right Neighbor's Left Endpoint",
             &right_seg->left_color, FALSE);
  SET_COLOR ("/Load Right Color From/Left Endpoint",
             &editor->control_sel_l->left_color, FALSE);
  SET_COLOR ("/Load Right Color From/FG Color", &fg, FALSE);
  SET_COLOR ("/Load Right Color From/BG Color", &bg, FALSE);

  SET_COLOR ("/Load Right Color From/01", &editor->saved_colors[0], TRUE);
  SET_COLOR ("/Load Right Color From/02", &editor->saved_colors[1], TRUE);
  SET_COLOR ("/Load Right Color From/03", &editor->saved_colors[2], TRUE);
  SET_COLOR ("/Load Right Color From/04", &editor->saved_colors[3], TRUE);
  SET_COLOR ("/Load Right Color From/05", &editor->saved_colors[4], TRUE);
  SET_COLOR ("/Load Right Color From/06", &editor->saved_colors[5], TRUE);
  SET_COLOR ("/Load Right Color From/07", &editor->saved_colors[6], TRUE);
  SET_COLOR ("/Load Right Color From/08", &editor->saved_colors[7], TRUE);
  SET_COLOR ("/Load Right Color From/09", &editor->saved_colors[8], TRUE);
  SET_COLOR ("/Load Right Color From/10", &editor->saved_colors[9], TRUE);

  SET_COLOR ("/Save Right Color To/01", &editor->saved_colors[0], TRUE);
  SET_COLOR ("/Save Right Color To/02", &editor->saved_colors[1], TRUE);
  SET_COLOR ("/Save Right Color To/03", &editor->saved_colors[2], TRUE);
  SET_COLOR ("/Save Right Color To/04", &editor->saved_colors[3], TRUE);
  SET_COLOR ("/Save Right Color To/05", &editor->saved_colors[4], TRUE);
  SET_COLOR ("/Save Right Color To/06", &editor->saved_colors[5], TRUE);
  SET_COLOR ("/Save Right Color To/07", &editor->saved_colors[6], TRUE);
  SET_COLOR ("/Save Right Color To/08", &editor->saved_colors[7], TRUE);
  SET_COLOR ("/Save Right Color To/09", &editor->saved_colors[8], TRUE);
  SET_COLOR ("/Save Right Color To/10", &editor->saved_colors[9], TRUE);

  if (! selection)
    {
      SET_LABEL ("/blendingfunction", _("_Blending Function for Segment"));
      SET_LABEL ("/coloringtype",     _("Coloring _Type for Segment"));

      SET_LABEL ("/flip",             _("_Flip Segment"));
      SET_LABEL ("/replicate",        _("_Replicate Segment..."));
      SET_LABEL ("/splitmidpoint",    _("Split Segment at _Midpoint"));
      SET_LABEL ("/splituniformly",   _("Split Segment _Uniformly..."));
      SET_LABEL ("/delete",           _("_Delete Segment"));
      SET_LABEL ("/recenter",         _("Re-_center Segment's Midpoint"));
      SET_LABEL ("/redistribute",     _("Re-distribute _Handles in Segment"));
    }
  else
    {
      SET_LABEL ("/blendingfunction", _("_Blending Function for Selection"));
      SET_LABEL ("/coloringtype",     _("Coloring _Type for Selection"));

      SET_LABEL ("/flip",             _("_Flip Selection"));
      SET_LABEL ("/replicate",        _("_Replicate Selection..."));
      SET_LABEL ("/splitmidpoint",    _("Split Segments at _Midpoints"));
      SET_LABEL ("/splituniformly",   _("Split Segments _Uniformly..."));
      SET_LABEL ("/delete",           _("_Delete Selection"));
      SET_LABEL ("/recenter",         _("Re-_center Midpoints in Selection"));
      SET_LABEL ("/redistribute",     _("Re-distribute _Handles in Selection"));
    }

  SET_SENSITIVE ("/blendingfunction/(Varies)", FALSE);
  SET_SENSITIVE ("/coloringtype/(Varies)",     FALSE);

  if (blending_equal)
    {
      SET_VISIBLE ("/blendingfunction/(Varies)", FALSE);

      switch (editor->control_sel_l->type)
        {
        case GIMP_GRAD_LINEAR:
          SET_ACTIVE ("/blendingfunction/Linear", TRUE);
          break;
        case GIMP_GRAD_CURVED:
          SET_ACTIVE ("/blendingfunction/Curved", TRUE);
          break;
        case GIMP_GRAD_SINE:
          SET_ACTIVE ("/blendingfunction/Sinusodial", TRUE);
          break;
        case GIMP_GRAD_SPHERE_INCREASING:
          SET_ACTIVE ("/blendingfunction/Spherical (increasing)", TRUE);
          break;
        case GIMP_GRAD_SPHERE_DECREASING:
          SET_ACTIVE ("/blendingfunction/Spherical (decreasing)", TRUE);
          break;
        }
    }
  else
    {
      SET_VISIBLE ("/blendingfunction/(Varies)", TRUE);
      SET_ACTIVE ("/blendingfunction/(Varies)", TRUE);
    }

  if (coloring_equal)
    {
      SET_VISIBLE ("/coloringtype/(Varies)", FALSE);

      switch (editor->control_sel_l->color)
        {
        case GIMP_GRAD_RGB:
          SET_ACTIVE ("/coloringtype/RGB", TRUE);
          break;
        case GIMP_GRAD_HSV_CCW:
          SET_ACTIVE ("/coloringtype/HSV (counter-clockwise hue)", TRUE);
          break;
        case GIMP_GRAD_HSV_CW:
          SET_ACTIVE ("/coloringtype/HSV (clockwise hue)", TRUE);
          break;
        }
    }
  else
    {
      SET_VISIBLE ("/coloringtype/(Varies)", TRUE);
      SET_ACTIVE ("/coloringtype/(Varies)", TRUE);
    }

  SET_SENSITIVE ("/Blend Endpoints' Colors",  selection);
  SET_SENSITIVE ("/Blend Endpoints' Opacity", selection);
  SET_SENSITIVE ("/delete", delete);

#undef SET_ACTIVE
#undef SET_COLOR
#undef SET_LABEL
#undef SET_SENSITIVE
#undef SET_VISIBLE
}
