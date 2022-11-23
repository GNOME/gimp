/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmatemplate.h
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TEMPLATE_H__
#define __LIGMA_TEMPLATE_H__


#include "ligmaviewable.h"


#define LIGMA_TEMPLATE_PARAM_COPY_FIRST (1 << (8 + G_PARAM_USER_SHIFT))

#ifdef LIGMA_UNSTABLE
/* Uncommon ratio, with at least one odd value, to encourage testing
 * LIGMA with unusual numbers.
 * It also has to be a higher resolution, to push LIGMA a little further
 * in tests. */
#define LIGMA_DEFAULT_IMAGE_WIDTH   2001
#define LIGMA_DEFAULT_IMAGE_HEIGHT  1984
#else
/* 1366x768 is the most common screen resolution in 2016.
 * 1920x1080 is the second most common.
 * Since LIGMA targets advanced graphics artists, let's go for the
 * highest common dimension.
 */
#define LIGMA_DEFAULT_IMAGE_WIDTH   1920
#define LIGMA_DEFAULT_IMAGE_HEIGHT  1080
#endif


#define LIGMA_TYPE_TEMPLATE            (ligma_template_get_type ())
#define LIGMA_TEMPLATE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TEMPLATE, LigmaTemplate))
#define LIGMA_TEMPLATE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TEMPLATE, LigmaTemplateClass))
#define LIGMA_IS_TEMPLATE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TEMPLATE))
#define LIGMA_IS_TEMPLATE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TEMPLATE))
#define LIGMA_TEMPLATE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TEMPLATE, LigmaTemplateClass))


typedef struct _LigmaTemplateClass LigmaTemplateClass;

struct _LigmaTemplate
{
  LigmaViewable  parent_instance;
};

struct _LigmaTemplateClass
{
  LigmaViewableClass  parent_instance;
};


GType               ligma_template_get_type            (void) G_GNUC_CONST;

LigmaTemplate      * ligma_template_new                 (const gchar  *name);

void                ligma_template_set_from_image      (LigmaTemplate *template,
                                                       LigmaImage    *image);

gint                ligma_template_get_width           (LigmaTemplate *template);
gint                ligma_template_get_height          (LigmaTemplate *template);
LigmaUnit            ligma_template_get_unit            (LigmaTemplate *template);

gdouble             ligma_template_get_resolution_x    (LigmaTemplate *template);
gdouble             ligma_template_get_resolution_y    (LigmaTemplate *template);
LigmaUnit            ligma_template_get_resolution_unit (LigmaTemplate *template);

LigmaImageBaseType   ligma_template_get_base_type       (LigmaTemplate *template);
LigmaPrecision       ligma_template_get_precision       (LigmaTemplate *template);

LigmaColorProfile  * ligma_template_get_color_profile   (LigmaTemplate *template);
LigmaColorProfile  * ligma_template_get_simulation_profile
                                                      (LigmaTemplate *template);
LigmaColorRenderingIntent ligma_template_get_simulation_intent
                                                      (LigmaTemplate *template);
gboolean            ligma_template_get_simulation_bpc  (LigmaTemplate *template);

LigmaFillType        ligma_template_get_fill_type       (LigmaTemplate *template);

const gchar       * ligma_template_get_comment         (LigmaTemplate *template);

guint64             ligma_template_get_initial_size    (LigmaTemplate *template);


#endif /* __LIGMA_TEMPLATE__ */
