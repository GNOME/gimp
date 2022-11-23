/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaactionimpl.h
 * Copyright (C) 2004-2019 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_ACTION_IMPL_H__
#define __LIGMA_ACTION_IMPL_H__


#define LIGMA_TYPE_ACTION_IMPL            (ligma_action_impl_get_type ())
#define LIGMA_ACTION_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ACTION_IMPL, LigmaActionImpl))
#define LIGMA_ACTION_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ACTION_IMPL, LigmaActionImplClass))
#define LIGMA_IS_ACTION_IMPL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ACTION_IMPL))
#define LIGMA_IS_ACTION_IMPL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), LIGMA_TYPE_ACTION_IMPL))
#define LIGMA_ACTION_IMPL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), LIGMA_TYPE_ACTION_IMPL, LigmaActionImplClass))

typedef struct _LigmaActionImpl      LigmaActionImpl;
typedef struct _LigmaActionImplClass LigmaActionImplClass;

struct _LigmaActionImpl
{
  GtkAction           parent_instance;

  LigmaContext        *context;

  gchar              *disable_reason;
  LigmaRGB            *color;
  LigmaViewable       *viewable;
  PangoEllipsizeMode  ellipsize;
  gint                max_width_chars;
};

struct _LigmaActionImplClass
{
  GtkActionClass parent_class;
};

GType         ligma_action_impl_get_type       (void) G_GNUC_CONST;

LigmaAction  * ligma_action_impl_new            (const gchar   *name,
                                               const gchar   *label,
                                               const gchar   *tooltip,
                                               const gchar   *icon_name,
                                               const gchar   *help_id);


#endif  /* __LIGMA_ACTION_IMPL_H__ */
