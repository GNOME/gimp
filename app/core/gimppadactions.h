/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_PAD_ACTIONS_H__
#define __GIMP_PAD_ACTIONS_H__


#include "gimpdata.h"


#define GIMP_TYPE_PAD_ACTIONS            (gimp_pad_actions_get_type ())
#define GIMP_PAD_ACTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PAD_ACTIONS, GimpPadActions))
#define GIMP_PAD_ACTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PAD_ACTIONS, GimpPadActionsClass))
#define GIMP_IS_PAD_ACTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PAD_ACTIONS))
#define GIMP_IS_PAD_ACTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PAD_ACTIONS))
#define GIMP_PAD_ACTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PAD_ACTIONS, GimpPadActionsClass))


typedef struct _GimpPadActionsPrivate GimpPadActionsPrivate;
typedef struct _GimpPadActionsClass   GimpPadActionsClass;

struct _GimpPadActions
{
  GObject                parent_instance;
  GimpPadActionsPrivate *priv;
};

struct _GimpPadActionsClass
{
  GObjectClass parent_class;
};

typedef enum _GimpPadActionType GimpPadActionType;
enum _GimpPadActionType
{
  GIMP_PAD_ACTION_BUTTON,
  GIMP_PAD_ACTION_RING,
  GIMP_PAD_ACTION_STRIP
};

typedef void (* GimpPadActionForeach) (GimpPadActions    *pad_actions,
                                       GimpPadActionType  action_type,
                                       guint              number,
                                       guint              mode,
                                       const gchar       *action,
                                       gpointer           data);


GType            gimp_pad_actions_get_type      (void) G_GNUC_CONST;

GimpPadActions * gimp_pad_actions_new           (void);

void             gimp_pad_actions_foreach       (GimpPadActions       *pad_actions,
                                                 GimpPadActionForeach  func,
                                                 gpointer              data);
gint             gimp_pad_actions_set_action    (GimpPadActions       *pad_actions,
                                                 GimpPadActionType     action_type,
                                                 guint                 number,
                                                 guint                 mode,
                                                 const gchar          *action_name);
void             gimp_pad_actions_clear_action  (GimpPadActions       *pad_actions,
                                                 GimpPadActionType     action_type,
                                                 guint                 number,
                                                 guint                 mode);
gint             gimp_pad_actions_get_n_actions (GimpPadActions       *pad_actions);
const gchar    * gimp_pad_actions_get_action    (GimpPadActions       *pad_actions,
                                                 GimpPadActionType     action_type,
                                                 guint                 number,
                                                 guint                 mode);


#endif /* __GIMP_PAD_ACTIONS_H__ */
