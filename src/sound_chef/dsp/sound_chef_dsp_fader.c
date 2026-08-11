#include "sound_chef/sound_chef_internal.h"
#include "sound_chef/sound_chef_dsp.h"

static sbk_status sc_dsp_fader_create(sc_dsp_state* state)
{
    state->userData = ma_malloc(sizeof(ma_sound_group), &((sc_system*)state->system)->engine.allocationCallbacks);
    if (state->userData == NULL)
    {
        return SBK_ERR_OUT_OF_MEMORY;
    }

    ma_sound_group_config config = ma_sound_group_config_init_2((ma_engine*)state->system);
    return SBK_FROM_MA(ma_sound_group_init_ex((ma_engine*)state->system, &config, (ma_sound_group*)state->userData));
}

static sbk_status sc_dsp_fader_release(sc_dsp_state* state)
{
    ma_sound_group_uninit((ma_sound_group*)state->userData);
    SC_FREE(state->userData, (sc_system*)state->system);
    return SBK_SUCCESS;
}

sc_dsp_vtable g_dspFaderVTable =
{
    sc_dsp_fader_create,
    sc_dsp_fader_release
};
