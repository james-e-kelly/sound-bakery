#include "sound_chef/sound_chef.h"

sbk_status sc_node_group_init(sc_system* system, sc_node_group* nodeGroup)
{
    // Always create a fader/sound_group by default
    const sc_dsp_config faderConfig = sc_dsp_config_init_type(system, sc_dsp_type_fader);
    SC_CHECK_STATUS(sc_system_create_dsp(system, &faderConfig, &nodeGroup->fader));

    nodeGroup->head = nodeGroup->fader;
    nodeGroup->tail = nodeGroup->fader;

    if (system->masterNodeGroup != NULL && nodeGroup != system->masterNodeGroup)
    {
        SC_CHECK_STATUS(sc_node_group_set_parent(nodeGroup, system->masterNodeGroup));
    }

    return SBK_SUCCESS;
}

sbk_status sc_node_group_add_dsp(sc_node_group* nodeGroup, sc_dsp* dsp, sc_dsp_index index)
{
    SC_CHECK(index == sc_dsp_index_head, SC_STATUS_FROM_MA_RESULT(MA_NOT_IMPLEMENTED));
    SC_CHECK_ARG(nodeGroup != NULL);
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK(dsp->prev == NULL, SC_STATUS_FROM_MA_RESULT(MA_NOT_IMPLEMENTED));  // don't have detatch logic
    SC_CHECK(dsp->next == NULL, SC_STATUS_FROM_MA_RESULT(MA_NOT_IMPLEMENTED));  // don't have detatch logic

    sbk_status result = SBK_ERR_CHEF;

    switch ((int)index)
    {
        case sc_dsp_index_head:
        {
            sc_dsp* currentHead = nodeGroup->head;
            SC_ASSERT(currentHead->next == NULL);  // head nodes can't have
                                                   // something after them
            ma_node_base* currentParent = ((ma_node_base*)currentHead->node)->pOutputBuses[0].pInputNode;
            SC_ASSERT(currentParent != NULL);  // must be attached to
                                               // something, even if it's the
                                               // endpoint

            if (currentParent)
            {
                // Attach the dsp to the get_parent output
                result = SC_STATUS_FROM_MA_RESULT(ma_node_attach_output_bus(dsp->node, 0, currentParent, 0));
                SC_CHECK_STATUS(result);

                // Make the current head attach to the DSP (which is now the
                // head)
                result = SC_STATUS_FROM_MA_RESULT(ma_node_attach_output_bus(currentHead->node, 0, dsp->node, 0));
                SC_CHECK_STATUS(result);

                nodeGroup->head->next = dsp;
                dsp->prev             = nodeGroup->head;

                nodeGroup->head = dsp;
            }

            break;
        }
        case 0:
        case sc_dsp_index_tail:
        {
            sc_dsp* currentTail = nodeGroup->tail;

            result = SC_STATUS_FROM_MA_RESULT(ma_node_attach_output_bus(dsp->node, 0, currentTail->node, 0));
            SC_CHECK_STATUS(result);

            break;
        }
        default:
            break;
    }

    SC_ASSERT(result == SBK_SUCCESS);

    return result;
}

sbk_status sc_node_group_set_parent(sc_node_group* nodeGroup, sc_node_group* parent)
{
    SC_CHECK_ARG(nodeGroup != NULL);
    SC_CHECK_ARG(parent != NULL);

    return SC_STATUS_FROM_MA_RESULT(ma_node_attach_output_bus(nodeGroup->head->node, 0, parent->tail->node, 0));
}

sbk_status sc_node_group_set_parent_endpoint(sc_node_group* nodeGroup)
{
    SC_CHECK_ARG(nodeGroup != NULL);

    sc_system* const system = (sc_system*)nodeGroup->fader->system;
    SC_CHECK(system != NULL, SBK_ERR_NULL);

    ma_node* const endPoint = ma_node_graph_get_endpoint((ma_node_graph*)system);
    SC_CHECK(endPoint != NULL, SC_STATUS_FROM_MA_RESULT(MA_BAD_ADDRESS));

    return SC_STATUS_FROM_MA_RESULT(ma_node_attach_output_bus(nodeGroup->head->node, 0, endPoint, 0));
}

sbk_status sc_node_group_get_dsp(sc_node_group* nodeGroup, sc_dsp_type type, sc_dsp** dsp)
{
    SC_CHECK_ARG(nodeGroup != NULL);
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK(nodeGroup->tail != NULL, SBK_ERR_NULL);

    *dsp = NULL;

    sc_dsp* currentDsp = nodeGroup->tail;

    do
    {
        if (currentDsp->handle == (sc_uint32)type)
        {
            *dsp = currentDsp;
            return SBK_SUCCESS;
        }
        currentDsp = currentDsp->next;
    } while (currentDsp != NULL);

    return SBK_ERR_NOT_FOUND;
}

sbk_status sc_node_group_uninit(sc_node_group* nodeGroup)
{
    SC_CHECK_ARG(nodeGroup != NULL);

    const sc_system* system = (sc_system*)nodeGroup->fader->system;

    sc_dsp* iDSP = nodeGroup->tail;

    while (iDSP != NULL)
    {
        sc_dsp* toFreeDSP = iDSP;
        iDSP              = toFreeDSP->next;
        sc_dsp_release(toFreeDSP);
    }

    nodeGroup->fader = NULL;
    nodeGroup->head  = NULL;
    nodeGroup->tail  = NULL;
    
    return SBK_SUCCESS;
}

sbk_status sc_node_group_release(sc_node_group* nodeGroup)
{
    SC_CHECK_ARG(nodeGroup != NULL);

    const sc_system* system = (sc_system*)nodeGroup->fader->system;

    (void)sc_node_group_uninit(nodeGroup);
    SC_FREE(nodeGroup, system);

    return SBK_SUCCESS;
}