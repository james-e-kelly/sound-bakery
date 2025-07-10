#include "create_review_popup.h"

#include "app/review_app.h"
#include "elements/file_drop_element.h"
#include "managers/workspace_manager.h"

auto create_review_popup::render_popup() -> void
{
    ImGui::SetWindowFontScale(1.5f);

    ImGui::BeginDisabled(review_app::get()->get_is_drag_dropping());

    ImGui::InputTextWithHint("Review Name", "My New Review", reviewNameBuffer, textBufferSize);
    ImGui::SetItemTooltip("Write the title of the review. It can be anything from \"Adding some sounds\" to "
                          "\"[TASK-1234][Level1] Add Looping Fire Sounds\"");

    ImGui::InputTextWithHint("Review Description", "Adding a new sound", reviewDescriptionBuffer,
                             textBufferSize);
    ImGui::SetItemTooltip("Give the review a description to help describe what sounds you are adding or changing");

    ImGui::InputTextWithHint("Review URL", "https://domain.com/task", reviewUrlBuffer, textBufferSize);
    ImGui::SetItemTooltip("Add the Jira/ADO/HacknPlan task URL");

    if (ImGui::BeginCombo("Review Phase", magic_enum::enum_name(m_reviewData.m_reviewPhase).data()))
    {
        for (int i = 0; i < static_cast<int>(review_phase::num); ++i)
        {
            review_phase phase = static_cast<review_phase>(i);

            if (ImGui::Selectable(magic_enum::enum_name(phase).data()))
            {
                m_reviewData.m_reviewPhase = phase;
            }
        }

        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("Set the review phase. Useful for giving context as a temp sound does not need as much "
                          "vetting as a final pass sound");

    if (ImGui::BeginCombo("Review Quality", magic_enum::enum_name(m_reviewData.m_reviewQuality).data()))
    {
        for (int i = 0; i < static_cast<int>(review_quality::num); ++i)
        {
            review_quality quality = static_cast<review_quality>(i);

            if (ImGui::Selectable(magic_enum::enum_name(quality).data()))
            {
                m_reviewData.m_reviewQuality = quality;
            }
        }

        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("Set the review quality. The higher the quality, the more the asset may need reviewing. Your "
                          "project may vary but roughly A == \"Industry Competitive\"");

    ImGui::EndDisabled();

    {
        gluten::imgui::scoped_color headerBg(ImGuiCol_Header, gluten::theme::carbon_g100::field03);

        if (ImGui::CollapsingHeader("Context Files"))
        {
            for (auto iter = m_reviewData.m_absoluteContextFiles.begin();
                 iter != m_reviewData.m_absoluteContextFiles.end();)
            {
                ImGui::Text(iter->string().c_str());
                ImGui::SameLine();
                if (ImGui::Button(fmt::format("X##{}", iter->string().c_str()).c_str()))
                {
                    iter = m_reviewData.m_absoluteContextFiles.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }

        if (ImGui::CollapsingHeader("Review Files"))
        {
            for (auto iter = m_reviewData.m_absoluteReviewFiles.begin();
                 iter != m_reviewData.m_absoluteReviewFiles.end();)
            {
                ImGui::Text(iter->string().c_str());
                ImGui::SameLine();
                if (ImGui::Button(fmt::format("X##{}", iter->string().c_str()).c_str()))
                {
                    iter = m_reviewData.m_absoluteReviewFiles.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }
    }
    if (review_app::get()->get_is_drag_dropping())
    {
       file_drop_element contextFilesDrop("Context Files");
       file_drop_element reviewFilesDrop("Review Files");

       if (contextFilesDrop.render_cursor())
       {
           for (const auto& file : contextFilesDrop.m_droppedFiles)
           {
               if (!m_reviewData.m_absoluteContextFiles.contains(file))
               {
                   m_reviewData.m_absoluteContextFiles.insert(file);
               }
           }
       }
       
       if (reviewFilesDrop.render_cursor())
       {
           for (const auto& file : reviewFilesDrop.m_droppedFiles)
           {
               if (!m_reviewData.m_absoluteReviewFiles.contains(file))
               {
                   m_reviewData.m_absoluteReviewFiles.insert(file);
               }
           }
       }
    }

    ImGui::SetWindowFontScale(1.5f);    // Needs to be reset because the drag drop targets can reset the scale

    m_reviewData.m_reviewName = reviewNameBuffer;
    m_reviewData.m_reviewDescription = reviewDescriptionBuffer;
    m_reviewData.m_reviewTaskUrl     = reviewDescriptionBuffer;

    const bool setupValid = !m_reviewData.m_reviewName.empty();

    ImGui::BeginDisabled(!setupValid);

    if (ImGui::Button("Create"))
    {
        if (std::shared_ptr<workspace_manager> workspaceManager = get_app()->get_manager_by_class<workspace_manager>())
        {
            workspaceManager->create_review(m_reviewData);
        }

        close_popup();
    }

    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        close_popup();
    }

    ImGui::SetWindowFontScale(1.0f);
}