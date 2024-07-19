#include "soundbank_viewer.h"

#include "sound_chef/sound_chef_bank.h"

#include "imgui.h"

void soundbank_viewer_widget::start() 
{ 
    sbk::engine::system::create(); 
    sbk::engine::system::init();
}

void soundbank_viewer_widget::render()
{
    static sc_sound* sound = nullptr;
    static sc_sound_instance* soundInstance = nullptr;

    if (ImGui::Begin("Soundbank Viewer"))
    {
        if (m_bank.riff != nullptr)
        {
            ImGui::TextUnformatted(std::to_string(m_bank.riff->id).c_str());
            ImGui::TextUnformatted(std::to_string(m_bank.riff->size).c_str());
            ImGui::TextUnformatted(std::to_string(m_bank.riff->numOfSubchunks).c_str());

            ImGui::TextUnformatted("---");

            if (m_bank.riff->subChunks != nullptr)
            {
                for (std::size_t subChunkIndex = 0; subChunkIndex < m_bank.riff->numOfSubchunks; ++subChunkIndex)
                {
                    ImGui::PushID(subChunkIndex);

                    ImGui::TextUnformatted(std::to_string(m_bank.riff->subChunks[subChunkIndex]->id).c_str());
                    ImGui::TextUnformatted(std::to_string(m_bank.riff->subChunks[subChunkIndex]->size).c_str());
                    ImGui::TextUnformatted(m_bank.riff->subChunks[subChunkIndex]->name);

                    if (ImGui::Button("Play"))
                    {
                        if (soundInstance != nullptr)
                        {
                            sc_sound_instance_release(soundInstance);
                            soundInstance = nullptr;
                        }

                        if (sound != nullptr)
                        {
                            sc_sound_release(sound);
                            sound = nullptr;
                        }

                        sc_system_create_sound_memory(
                            sbk::engine::system::get(), m_bank.riff->subChunks[subChunkIndex]->data,
                            m_bank.riff->subChunks[subChunkIndex]->size - SC_BANK_FILE_NAME_BUFFER_SIZE,
                            SC_SOUND_MODE_DEFAULT, &sound);

                        sc_system_play_sound(sbk::engine::system::get(), sound, &soundInstance, NULL, MA_FALSE);
                    }

                    ImGui::TextUnformatted("--");

                    ImGui::PopID();
                }
            }
        }
    }

    ImGui::End();
}

void soundbank_viewer_widget::end()
{
    sc_bank_uninit(&m_bank);

    sbk::engine::system::destroy();
}

void soundbank_viewer_widget::set_soundbank_to_view(const std::filesystem::path& soundbankFilePath)
{
    const sc_result initResult = sc_bank_init(&m_bank, soundbankFilePath.string().c_str(), MA_OPEN_MODE_READ);

    if (initResult == MA_SUCCESS)
    {
        const sc_result readResult = sc_bank_read(&m_bank);
        assert(readResult == MA_SUCCESS);
        (void)readResult;
    }
}