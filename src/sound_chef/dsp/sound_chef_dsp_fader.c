#include "sound_chef/sound_chef_internal.h"
#include "sound_chef/sound_chef_dsp.h"

static sbk_status sc_dsp_fader_create(sc_system* system, sc_dsp* dsp, void* userData)
{
    SC_CREATE(dsp->node, ma_sound_group, system);

    ma_sound_group_config config = ma_sound_group_config_init_2(system);
    return SBK_FROM_MA(ma_sound_group_init_ex((ma_engine*)system, &config, (ma_sound_group*)dsp->node));
}

static sbk_status sc_dsp_fader_release(sc_system* system, sc_dsp* dsp)
{
    ma_sound_group_uninit((ma_sound_group*)dsp->node);
    SC_FREE(dsp->node, system);
    return SBK_SUCCESS;
}

sc_dsp_description g_dspFaderVTable =
{
    sc_dsp_fader_create,
    sc_dsp_fader_release
};
