/* ----------------------------------------------------------------------
 * p_plug_in_alienmap_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_alienmap_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_alienmap_Vals 
    {
      gdouble      redstretch;
      gdouble      greenstretch;
      gdouble      bluestretch;
      gint      redmode;
      gint      greenmode;
      gint      bluemode;
    } t_plug_in_alienmap_Vals; 

    t_plug_in_alienmap_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_alienmap_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_alienmap_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_alienmap_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_alienmap_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_alienmap_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_alienmap_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_alienmap_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.redstretch, buf_from->redstretch, buf_to->redstretch, total_steps, current_step);
    p_delta_gdouble(&buf.greenstretch, buf_from->greenstretch, buf_to->greenstretch, total_steps, current_step);
    p_delta_gdouble(&buf.bluestretch, buf_from->bluestretch, buf_to->bluestretch, total_steps, current_step);
    p_delta_gint(&buf.redmode, buf_from->redmode, buf_to->redmode, total_steps, current_step);
    p_delta_gint(&buf.greenmode, buf_from->greenmode, buf_to->greenmode, total_steps, current_step);
    p_delta_gint(&buf.bluemode, buf_from->bluemode, buf_to->bluemode, total_steps, current_step);

    gimp_set_data("plug_in_alienmap", &buf, sizeof(buf)); 

    return 0; /* OK */
}
