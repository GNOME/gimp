/* ----------------------------------------------------------------------
 * p_plug_in_despeckle_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_despeckle_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_despeckle_Vals 
    {
      gint      radius;
      gint      type;    /* not iterated */
      gint      black;
      gint      white;
    } t_plug_in_despeckle_Vals; 

    t_plug_in_despeckle_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_despeckle_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_despeckle_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_despeckle_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_despeckle_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_despeckle_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_despeckle_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_despeckle_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gint(&buf.radius, buf_from->radius, buf_to->radius, total_steps, current_step);
    /* p_delta_gint(&buf.type, buf_from->type, buf_to->type, total_steps, current_step); */
    p_delta_gint(&buf.black, buf_from->black, buf_to->black, total_steps, current_step);
    p_delta_gint(&buf.white, buf_from->white, buf_to->white, total_steps, current_step);

    gimp_set_data("plug_in_despeckle", &buf, sizeof(buf)); 

    return 0; /* OK */
}
