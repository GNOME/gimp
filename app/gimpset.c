#include "gimpsignal.h"
#include "gimpsetP.h"

/* Yep, this can be optimized quite a lot */

enum
{
  ADD,
  REMOVE,
  LAST_SIGNAL
};

static guint gimp_set_signals [LAST_SIGNAL];

static GimpObjectClass* parent_class;

static void
gimp_set_destroy (GtkObject* ob)
{
	GimpSet* set=GIMP_SET(ob);
	GSList* node;
	for(node=set->list;node;node=node->next){
		if(!set->weak)
			gtk_object_unref(GTK_OBJECT(node->data));
		gtk_signal_emit (GTK_OBJECT(set),
				 gimp_set_signals[REMOVE],
				 node->data);
	}
	g_slist_free(set->list);
	GTK_OBJECT_CLASS(parent_class)->destroy (ob);
}

static void
gimp_set_init (GimpSet* set)
{
	set->list=NULL;
	set->type=GTK_TYPE_OBJECT;
}

static void
gimp_set_class_init (GimpSetClass* klass)
{
	GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);
	GtkType type = object_class->type;

	parent_class=gtk_type_parent_class(type);
	
	object_class->destroy = gimp_set_destroy;
	
	gimp_set_signals[ADD]=
		gimp_signal_new ("add", 0, type, 0, gimp_sigtype_pointer);
	gimp_set_signals[REMOVE]=
		gimp_signal_new ("remove", 0, type, 0, gimp_sigtype_pointer);
	gtk_object_class_add_signals (object_class,
				      gimp_set_signals,
				      LAST_SIGNAL);
}

GtkType gimp_set_get_type (void)
{
	static GtkType type;
	GIMP_TYPE_INIT(type,
		       GimpSet,
		       GimpSetClass,
		       gimp_set_init,
		       gimp_set_class_init,
		       GIMP_TYPE_OBJECT);
	return type;
}


GimpSet*
gimp_set_new (GtkType type, gboolean weak){
	GimpSet* set=gtk_type_new (gimp_set_get_type ());
	set->type=type;
	set->weak=weak;
	return set;
}

static void
gimp_set_destroy_cb (GtkObject* ob, gpointer data){
	GimpSet* set=GIMP_SET(data);
	gimp_set_remove(set, ob);
}

gboolean
gimp_set_add (GimpSet* set, gpointer val)
{
	g_return_val_if_fail(set, FALSE);
	g_return_val_if_fail(GTK_CHECK_TYPE(val, set->type), FALSE);
	
	if(g_slist_find(set->list, val))
		return FALSE;
	
	set->list=g_slist_prepend(set->list, val);
	if(set->weak)
		gtk_signal_connect(GTK_OBJECT(val), "destroy",
				   GTK_SIGNAL_FUNC(gimp_set_destroy_cb), set);
	else
		gtk_object_ref(GTK_OBJECT(val));
	
	gtk_signal_emit (GTK_OBJECT(set), gimp_set_signals[ADD], val);
	return TRUE;
}

gboolean
gimp_set_remove (GimpSet* set, gpointer val) {
	g_return_val_if_fail(set, FALSE);

	if(!g_slist_find(set->list, val))
		return FALSE;

	set->list=g_slist_remove(set->list, val);

	gtk_signal_emit (GTK_OBJECT(set), gimp_set_signals[REMOVE], val);

	if(set->weak)
		gtk_signal_disconnect_by_func
			(GTK_OBJECT(val),
			 GTK_SIGNAL_FUNC(gimp_set_destroy_cb),
			 set);
	else
		gtk_object_unref(GTK_OBJECT(val));

	return TRUE;
}

gboolean
gimp_set_have (GimpSet* set, gpointer val) {
	return g_slist_find(set->list, val)?TRUE:FALSE;
}

void
gimp_set_foreach(GimpSet* set, GFunc func, gpointer user_data)
{
	g_slist_foreach(set->list, func, user_data);
}

GtkType
gimp_set_type (GimpSet* set){
	return set->type;
}
