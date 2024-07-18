#include "soundbank_viewer.h"

#include "sound_chef/sound_chef_bank.h"

#include "imgui.h"

void soundbank_viewer_widget::render()
{
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
                    ImGui::TextUnformatted(std::to_string(m_bank.riff->subChunks[subChunkIndex]->id).c_str());
                    ImGui::TextUnformatted(std::to_string(m_bank.riff->subChunks[subChunkIndex]->size).c_str());
                    ImGui::TextUnformatted(m_bank.riff->subChunks[subChunkIndex]->name);

                    ImGui::TextUnformatted("-");
                }
            }
        }
    }

    ImGui::End();
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
        const sc_result readResult = sc_bank_read(&m_bank);

        (void)readResult;
    }
}