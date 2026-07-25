#include "create_review_popup.h"

#include "app/review_app.h"
#include "elements/file_drop_element.h"
#include "managers/workspace_manager.h"

auto create_review_popup::render_popup() -> void
{
    ImGui::BeginDisabled(static_cast<bool>(m_asyncCreateReviewResult));
    ImGui::BeginDisabled(review_app::get()->get_is_drag_dropping());

    if (!m_existingReviewId.has_value())
    {
        ImGui::InputTextWithHint("Review Name", "My New Review", reviewNameBuffer, textBufferSize);
        ImGui::SetItemTooltip("Write the title of the review. It can be anything from \"Adding some sounds\" to "
                              "\"[TASK-1234][Level1] Add Looping Fire Sounds\"");

        ImGui::InputTextMultiline("Review Description", reviewDescriptionBuffer, textBufferSize);
        ImGui::SetItemTooltip("Give the review a description to help describe what sounds you are adding or changing");

        ImGui::InputTextWithHint("Review URL", "https://domain.com/task", reviewUrlBuffer, textBufferSize);
        ImGui::SetItemTooltip("Add the Jira/ADO/HacknPlan task URL");

        render_reviewers();
    }
    else
    {
        ImGui::Dummy(ImVec2(400.0f, 5.0f));
    }

    ImGui::EndDisabled();

    {
        ImGui::TextUnformatted("Drag Files To Add");

        gluten::imgui::scoped_color headerBg(ImGuiCol_Header, gluten::theme::field03);

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

    m_reviewData.m_reviewName        = reviewNameBuffer;
    m_reviewData.m_reviewDescription = reviewDescriptionBuffer;
    m_reviewData.m_reviewTaskUrl     = reviewUrlBuffer;

    const bool hasFiles = !m_reviewData.m_absoluteContextFiles.empty() || !m_reviewData.m_absoluteReviewFiles.empty();

    const bool setupValid = !m_reviewData.m_reviewName.empty() || (m_existingReviewId.has_value() && hasFiles);

    ImGui::BeginDisabled(!setupValid);

    if (ImGui::Button("Create"))
    {
        if (std::shared_ptr<workspace_manager> workspaceManager = get_app()->get_manager_by_class<workspace_manager>())
        {
            if (m_newUsers.has_value())
            {
                m_reviewData.m_reviewerIds.resize(m_newUsers.value().size());

                std::transform(m_newUsers.value().begin(),
                               m_newUsers.value().end(),
                               m_reviewData.m_reviewerIds.begin(),
                               [](const user_data& user)
                               { return user.m_userId; });
            }

            if (m_existingReviewId.has_value())
            {
                m_asyncCreateReviewResult = workspaceManager->create_review_version(m_existingReviewId.value(), m_reviewData);
            }
            else
            {
                m_asyncCreateReviewResult = workspaceManager->create_review(m_reviewData);
            }
            set_closable(false);
        }
    }

    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        close_popup();
    }

    if (m_asyncCreateReviewResult)
    {
        ImGui::SameLine();

        ImSpinner::SpinnerAngEclipse("##Loading", ImGui::GetFontSize() / 2.0f, 2.0f, gluten::theme::white, 8.0f);

        switch (m_asyncCreateReviewResult.status())
        {
            case concurrencpp::result_status::value:
            {
                m_asyncCreateReviewResult.get();
                onCompleteDelegate.Broadcast();
                close_popup();
                break;
            }
            case concurrencpp::result_status::exception:
                try
                {
                    m_asyncCreateReviewResult.get();
                }
                catch (std::exception exception)
                {
                    BOOST_ASSERT_MSG(false, exception.what());
                }
                break;
            default:
                break;
        }
    }

    ImGui::EndDisabled();
}