/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcontroller.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef GIMP_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#error GimpController is unstable API under construction
#endif

#ifndef __GIMP_CONTROLLER_H__
#define __GIMP_CONTROLLER_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


/**
 * GimpControllerEventType:
 * @GIMP_CONTROLLER_EVENT_TRIGGER: the event is a simple trigger
 * @GIMP_CONTROLLER_EVENT_VALUE:   the event carries a double value
 *
 * Event types for #GimpController.
 **/
typedef enum
{
  GIMP_CONTROLLER_EVENT_TRIGGER,
  GIMP_CONTROLLER_EVENT_VALUE
} GimpControllerEventType;


typedef struct _GimpControllerEventAny     GimpControllerEventAny;
typedef struct _GimpControllerEventTrigger GimpControllerEventTrigger;
typedef struct _GimpControllerEventValue   GimpControllerEventValue;
typedef union  _GimpControllerEvent        GimpControllerEvent;

struct _GimpControllerEventAny
{
  GimpControllerEventType  type;
  GimpController          *source;
  gint                     event_id;
};

struct _GimpControllerEventTrigger
{
  GimpControllerEventType  type;
  GimpController          *source;
  gint                     event_id;
};

struct _GimpControllerEventValue
{
  GimpControllerEventType  type;
  GimpController          *source;
  gint                     event_id;
  GValue                   value;
};

union _GimpControllerEvent
{
  GimpControllerEventType    type;
  GimpControllerEventAny     any;
  GimpControllerEventTrigger trigger;
  GimpControllerEventValue   value;
};


#define GIMP_TYPE_CONTROLLER            (gimp_controller_get_type ())
#define GIMP_CONTROLLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTROLLER, GimpController))
#define GIMP_CONTROLLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTROLLER, GimpControllerClass))
#define GIMP_IS_CONTROLLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTROLLER))
#define GIMP_IS_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTROLLER))
#define GIMP_CONTROLLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTROLLER, GimpControllerClass))


typedef struct _GimpControllerPrivate GimpControllerPrivate;
typedef struct _GimpControllerClass   GimpControllerClass;

struct _GimpController
{
  GObject                parent_instance;

  GimpControllerPrivate *priv;

  /* FIXME MOVE TO PRIVATE */
  gchar    *name;
  gchar    *state;
};

struct _GimpControllerClass
{
  GObjectClass  parent_class;

  const gchar  *name;
  const gchar  *help_domain;
  const gchar  *help_id;
  const gchar  *icon_name;

  /*  virtual functions  */
  gint          (* get_n_events)    (GimpController            *controller);
  const gchar * (* get_event_name)  (GimpController            *controller,
                                     gint                       event_id);
  const gchar * (* get_event_blurb) (GimpController            *controller,
                                     gint                       event_id);

  /*  signals  */
  gboolean      (* event)           (GimpController            *controller,
                                     const GimpControllerEvent *event);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


GType            gimp_controller_get_type        (void) G_GNUC_CONST;
GimpController * gimp_controller_new             (GType           controller_type);

gint             gimp_controller_get_n_events    (GimpController *controller);
const gchar    * gimp_controller_get_event_name  (GimpController *controller,
                                                  gint            event_id);
const gchar    * gimp_controller_get_event_blurb (GimpController *controller,
                                                  gint            event_id);


/*  protected  */

gboolean         gimp_controller_event (GimpController            *controller,
                                        const GimpControllerEvent *event);


G_END_DECLS

#endif /* __GIMP_CONTROLLER_H__ */
