/* ----------------------------------------------------------------------
 * p_plug_in_whirl_pinch_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_whirl_pinch_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_whirl_pinch_Vals 
    {
      gdouble   whirl;
      gdouble   pinch;
      gdouble   radius;
    } t_plug_in_whirl_pinch_Vals; 

    t_plug_in_whirl_pinch_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_whirl_pinch_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_whirl_pinch_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_whirl_pinch_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_whirl_pinch_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_whirl_pinch_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_whirl_pinch_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_whirl_pinch_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.whirl, buf_from->whirl, buf_to->whirl, total_steps, current_step);
    p_delta_gdouble(&buf.pinch, buf_from->pinch, buf_to->pinch, total_steps, current_step);
    p_delta_gdouble(&buf.radius, buf_from->radius, buf_to->radius, total_steps, current_step);

    gimp_set_data("plug_in_whirl_pinch", &buf, sizeof(buf)); 

    return 0; /* OK */
}
