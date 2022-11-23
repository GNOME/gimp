/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmafgbgview.h
 * Copyright (C) 2005  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_FG_BG_VIEW_H__
#define __LIGMA_FG_BG_VIEW_H__


#define LIGMA_TYPE_FG_BG_VIEW            (ligma_fg_bg_view_get_type ())
#define LIGMA_FG_BG_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FG_BG_VIEW, LigmaFgBgView))
#define LIGMA_FG_BG_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FG_BG_VIEW, LigmaFgBgViewClass))
#define LIGMA_IS_FG_BG_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FG_BG_VIEW))
#define LIGMA_IS_FG_BG_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FG_BG_VIEW))
#define LIGMA_FG_BG_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FG_BG_VIEW, LigmaFgBgViewClass))


typedef struct _LigmaFgBgViewClass LigmaFgBgViewClass;

struct _LigmaFgBgView
{
  GtkWidget           parent_instance;

  LigmaContext        *context;
  LigmaColorConfig    *color_config;
  LigmaColorTransform *transform;
};

struct _LigmaFgBgViewClass
{
  GtkWidgetClass  parent_class;
};


GType       ligma_fg_bg_view_get_type    (void) G_GNUC_CONST;

GtkWidget * ligma_fg_bg_view_new         (LigmaContext  *context);

void        ligma_fg_bg_view_set_context (LigmaFgBgView *view,
                                         LigmaContext  *context);


#endif  /*  __LIGMA_FG_BG_VIEW_H__  */
