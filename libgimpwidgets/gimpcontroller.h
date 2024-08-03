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
 * <https://www.gnu.org/licenses/>.
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

/**
 * GimpControllerEventAny:
 * @type:     The event's #GimpControllerEventType
 * @source:   The event's source #GimpController
 * @event_id: The event's ID
 *
 * Generic controller event. Every event has these three members at the
 * beginning of its struct
 **/
struct _GimpControllerEventAny
{
  GimpControllerEventType  type;
  GimpController          *source;
  gint                     event_id;
};

/**
 * GimpControllerEventTrigger:
 * @type:     The event's #GimpControllerEventType
 * @source:   The event's source #GimpController
 * @event_id: The event's ID
 *
 * Trigger controller event.
 **/
struct _GimpControllerEventTrigger
{
  GimpControllerEventType  type;
  GimpController          *source;
  gint                     event_id;
};

/**
 * GimpControllerEventValue:
 * @type:     The event's #GimpControllerEventType
 * @source:   The event's source #GimpController
 * @event_id: The event's ID
 * @value:    The event's value
 *
 * Value controller event.
 **/
struct _GimpControllerEventValue
{
  GimpControllerEventType  type;
  GimpController          *source;
  gint                     event_id;
  GValue                   value;
};

/**
 * GimpControllerEvent:
 * @type:    The event type
 * @any:     GimpControllerEventAny
 * @trigger: GimpControllerEventTrigger
 * @value:   GimpControllerEventValue
 *
 * A union to hjold all event event types
 **/
union _GimpControllerEvent
{
  GimpControllerEventType    type;
  GimpControllerEventAny     any;
  GimpControllerEventTrigger trigger;
  GimpControllerEventValue   value;
};


#define GIMP_TYPE_CONTROLLER (gimp_controller_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpController, gimp_controller, GIMP, CONTROLLER, GObject)

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
