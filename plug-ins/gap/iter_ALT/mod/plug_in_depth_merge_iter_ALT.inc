/* ----------------------------------------------------------------------
 * p_plug_in_depth_merge_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_depth_merge_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_depth_merge_Vals 
    {
      gint32    result;
      gint32    source1;
      gint32    source2;
      gint32    depthMap1;
      gint32    depthMap2;
      float   overlap;
      float   offset;
      float   scale1;
      float   scale2;
    } t_plug_in_depth_merge_Vals; 

    t_plug_in_depth_merge_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_depth_merge_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_depth_merge_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_depth_merge_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_depth_merge_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_depth_merge_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_depth_merge_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_depth_merge_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_drawable(&buf.result, buf_from->result, buf_to->result, total_steps, current_step);
    p_delta_drawable(&buf.source1, buf_from->source1, buf_to->source1, total_steps, current_step);
    p_delta_drawable(&buf.source2, buf_from->source2, buf_to->source2, total_steps, current_step);
    p_delta_drawable(&buf.depthMap1, buf_from->depthMap1, buf_to->depthMap1, total_steps, current_step);
    p_delta_drawable(&buf.depthMap2, buf_from->depthMap2, buf_to->depthMap2, total_steps, current_step);
    p_delta_float(&buf.overlap, buf_from->overlap, buf_to->overlap, total_steps, current_step);
    p_delta_float(&buf.offset, buf_from->offset, buf_to->offset, total_steps, current_step);
    p_delta_float(&buf.scale1, buf_from->scale1, buf_to->scale1, total_steps, current_step);
    p_delta_float(&buf.scale2, buf_from->scale2, buf_to->scale2, total_steps, current_step);

    gimp_set_data("plug_in_depth_merge", &buf, sizeof(buf)); 

    return 0; /* OK */
}
