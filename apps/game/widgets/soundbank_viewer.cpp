#include "soundbank_viewer.h"

#include "sound_chef/sound_chef_bank.h"

void soundbank_viewer_widget::render()
{

}

void soundbank_viewer_widget::end()
{
    sc_bank_uninit(&m_bank);
}

void soundbank_viewer_widget::set_soundbank_to_view(const std::filesystem::path& soundbankFilePath)
{
    const sc_result initResult = sc_bank_init(&m_bank, soundbankFilePath.string().c_str(), MA_OPEN_MODE_READ);

    if (initResult == MA_SUCCESS)
    {
        sc_bank_read(&m_bank);
    }
}