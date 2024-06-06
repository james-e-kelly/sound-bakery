#ifndef SOUND_CHEF_ENCODER_VORBIS
#define SOUND_CHEF_ENCODER_VORBIS

#include "sound_chef/sound_chef_common.h"

sc_result SC_API sc_encoder_vorbis_on_init(ma_encoder* encoder);
void SC_API sc_encoder_vorbis_on_uninit(ma_encoder* encoder);
sc_result SC_API sc_encoder_vorbis_write_pcm_frames(ma_encoder* encoder,
                                                    const void* framesIn,
                                                    ma_uint64 frameCount,
                                                    ma_uint64* framesWritten);

#endif