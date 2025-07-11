#include "workspace_widget.h"

#include "app/review_app.h"
#include "managers/workspace_manager.h"
#include "elements/file_drop_element.h"
#include "elements/project_element.h"
#include "elements/review_element.h"
#include "widgets/create_project_popup.h"
#include "widgets/create_review_popup.h"
#include "widgets/update_review_popup.h"

namespace
{
    constexpr float leftToobarWidth             = 100.0f;
    constexpr float leftToolbarButtonHeight     = leftToobarWidth;
    constexpr float topHeaderHeight             = leftToobarWidth;
    constexpr float leftToolbarHalfButtonHeight = leftToolbarButtonHeight / 2.0f;
    constexpr float descriptionBoxHeight        = topHeaderHeight * 2.0f;

    constexpr float itemListWidth = leftToobarWidth * 5.0f;
}

auto workspace_widget::start_implementation() -> void
{
    ImGuiWindowClass windowClass;
    windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;

    set_window_class(windowClass);
}

auto workspace_widget::render_window_implementation() -> void
{
    gluten::imgui::scoped_style noGaps(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    render_left_toolbar();
    
    ImGui::SameLine();

    switch (m_activeView)
    {
        case workspace_widget::reviews_view:
            render_list();
            ImGui::SameLine();
            render_content();
            break;
        case workspace_widget::users_view:
            ImGui::TextUnformatted("Users");
            break;
        default:
            break;
    }
}

auto workspace_widget::render_list() -> void
{
    gluten::imgui::scoped_color backgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::layer02);
    gluten::imgui::scoped_color borderColor(ImGuiCol_Border, gluten::theme::carbon_g100::background);
    gluten::imgui::scoped_color separatorColor(ImGuiCol_Separator, gluten::theme::carbon_g100::background);

    static constexpr float buttonOffset = 20.0f;

    if (ImGui::BeginChild("ItemsPanel", ImVec2(itemListWidth, 0), ImGuiChildFlags_ResizeX))
    {
        gluten::element topToolbar(gluten::element::anchor_preset::stretch_top);
        topToolbar.get_element_anchor().maxOffset.y = leftToobarWidth;
        topToolbar.render_window();

        const bool listingProjects = !get_app()->get_manager_by_class<workspace_manager>()->has_selected_project();
        const bool listingReviews  = !listingProjects;

        gluten::text titleText(listingProjects ? "Projects" : "Reviews", ImVec2(0.5f, 0.5f), gluten::element::anchor_preset::center_middle);
        titleText
            .set_font(gluten::fonts::title)
            .set_element_content_font_size(gluten::g_baseFontSize * 2.0f)
            .render(topToolbar.get_element_rect());

        gluten::icon_button backButton("##BackButton", ICON_LC_ARROW_BIG_LEFT, gluten::fonts::regular_lucide_icons);
        backButton
            .set_icon_alignment(ImVec2(0.0f, 0.5f))
            .set_button_alignment(ImVec2(0.0f, 0.5f))
            .set_button_border(2.0f, 0.0f)
            .set_button_scale(1.2f)
            .set_element_background_color(gluten::theme::carbon_g100::field03)
            .set_element_hover_color(gluten::theme::carbon_g100::fieldHover03)
            .set_element_active_color(gluten::theme::carbon_g100::layerActive01)
            .set_element_anchor_preset(gluten::element::anchor_preset::left_middle)
            .set_element_content_scale(2.0f)
            .set_element_translation(ImVec2(buttonOffset, 0.0f));
        

        gluten::icon_button newButton("##NewButton", ICON_LC_PLUS, gluten::fonts::regular_lucide_icons);
        newButton
            .set_icon_alignment(ImVec2(1.0f, 0.5f))
            .set_button_alignment(ImVec2(1.0f, 0.5f))
            .set_button_border(2.0f, 0.0f)
            .set_button_scale(1.2f)
            .set_element_background_color(gluten::theme::carbon_g100::field03)
            .set_element_hover_color(gluten::theme::carbon_g100::fieldHover03)
            .set_element_active_color(gluten::theme::carbon_g100::layerActive01)
            .set_element_anchor_preset(gluten::element::anchor_preset::right_middle)
            .set_element_content_scale(2.0f)
            .set_element_translation(ImVec2(-buttonOffset, 0.0f));

        if (listingReviews)
        {
            if (backButton.render(topToolbar.get_element_rect()))
            {
                get_app()->get_manager_by_class<workspace_manager>()->select_project({});
            }
        }

        if (newButton.render(topToolbar.get_element_rect()))
        {
            if (listingProjects)
            {
                static std::shared_ptr<create_project_popup> createProjectPopup;
                createProjectPopup = add_child_widget<create_project_popup>(this);

                if (createProjectPopup)
                {
                    createProjectPopup->open_popup();
                }
            }
            else if (listingReviews)
            {
                static std::shared_ptr<create_review_popup> createReviewPopup;
                createReviewPopup = add_child_widget<create_review_popup>(this);

                if (createReviewPopup)
                {
                    createReviewPopup->open_popup();
                }
            }
        }

        ImGui::SetCursorPos(topToolbar.get_element_rect_local().GetBL());

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 2.0f);

        if (ImGui::BeginChild("ItemsList", ImVec2(0, 0), 0, ImGuiWindowFlags_AlwaysVerticalScrollbar))
        {
            gluten::layout itemsLayout(gluten::layout::layout_type::top_to_bottom, gluten::element::anchor_preset::stretch_full);
            itemsLayout.set_layout_spacing(2.0f);
            itemsLayout.render_window();

            if (listingProjects)
            {
                for (const auto& project : get_app()->get_manager_by_class<workspace_manager>()->get_projects())
                {
                    project_element projectElement(project.m_projectName, project.m_projectDescription, 2, 5);
                    if (itemsLayout.render_layout_element_pixels_vertical(&projectElement, 100.0f))
                    {
                        get_app()->get_manager_by_class<workspace_manager>()->select_project(project.m_projectName);
                    }
                }
            }
            else if (listingReviews)
            {
                for (const auto& review : get_app()->get_manager_by_class<workspace_manager>()->get_all_reviews())
                {
                    review_element reviewElement(review);
                    if (itemsLayout.render_layout_element_pixels_vertical(&reviewElement, 100.0f))
                    {
                        get_app()->get_manager_by_class<workspace_manager>()->select_review(review.m_reviewId);
                    }
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
}

auto workspace_widget::render_content() -> void
{
    gluten::imgui::scoped_color backgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::layer03);
    gluten::imgui::scoped_color borderColor(ImGuiCol_Border, gluten::theme::carbon_g100::borderStrong02);
    gluten::imgui::scoped_color tabBg(ImGuiCol_Tab, gluten::theme::carbon_g100::field03);
    gluten::imgui::scoped_color tabSelectedBg(ImGuiCol_TabActive, gluten::theme::carbon_g100::layerAccentActive03);
    gluten::imgui::scoped_color tabHoverdBg(ImGuiCol_TabHovered, gluten::theme::carbon_g100::layerHover03);
    gluten::imgui::scoped_color frameBg(ImGuiCol_FrameBg, gluten::theme::carbon_g100::layer02);
    gluten::imgui::scoped_color frameHoveredBg(ImGuiCol_FrameBgHovered, gluten::theme::carbon_g100::layerHover02);
    gluten::imgui::scoped_font iconFont(gluten::app::get()->get_font(gluten::fonts::regular_lucide_icons));

    if (ImGui::BeginChild("Content"))
    {
        gluten::layout verticalLayout(gluten::layout::layout_type::top_to_bottom,
                                      gluten::element::anchor_preset::stretch_full);
        verticalLayout.set_layout_spacing(10.0f);
        verticalLayout.render_window();

        gluten::background titleBarBackground;
        titleBarBackground.set_element_background_color(gluten::theme::carbon_g100::fieldHover02);
        verticalLayout.render_layout_element_pixels_vertical(&titleBarBackground, topHeaderHeight);

        const project_data& selectedProject = get_app()->get_manager_by_class<workspace_manager>()->get_selected_project();
        const review_data& selectedReview = get_app()->get_manager_by_class<workspace_manager>()->get_selected_review();

        if (selectedReview.m_reviewId)
        {
            gluten::text reviewTitleText(fmt::format("{} / {}", selectedProject.m_projectName, selectedReview.m_reviewId).c_str(), ImVec2(0, 0.5f), gluten::element::anchor_preset::left_top);
            reviewTitleText
                .set_element_content_font_size(gluten::g_baseFontSize * 2.0f)
                .set_element_translation(ImVec2(5, 50.0f));
            reviewTitleText.render(titleBarBackground.get_element_rect());

            gluten::layout descriptionBox;
            descriptionBox
                .set_layout_type(gluten::layout::layout_type::top_to_bottom)
                .set_element_anchor_preset(gluten::element::anchor_preset::stretch_full)
                .set_element_background_color(gluten::theme::carbon_g100::field03)
                .set_element_border(2.0f, 0.0f)
                .set_element_padding(ImVec2(ImGui::GetStyle().FramePadding.x * 2.0f, 0.0f));
            verticalLayout.render_layout_element_pixels_vertical(&descriptionBox, descriptionBoxHeight);

            ImRect paddedRect = descriptionBox.get_element_rect();
            paddedRect.Expand(ImVec2(-ImGui::GetStyle().FramePadding.x, 0.0f));

            gluten::text descriptionTitleText(selectedReview.m_reviewName.c_str(), ImVec2(0.0f, 0.5f), gluten::element::anchor_preset::left_middle);
            descriptionTitleText.
                set_element_content_font_size(gluten::g_baseFontSize * 1.5f);
            descriptionBox.render_layout_element_percent_vertical(&descriptionTitleText, 0.1f);

            gluten::text descriptionDescriptionText(selectedReview.m_reviewDescription.c_str(), ImVec2(0.0f, 0.0f), gluten::element::anchor_preset::stretch_top);
            descriptionBox.render_layout_element_percent_vertical(&descriptionDescriptionText, 0.1f);

            const ImVec4 inactivePhaseColor = gluten::theme::carbon_g100::layer03;

            {
                static std::unordered_map<review_phase, ImVec4> phaseColours{
                    {review_phase::temp, gluten::theme::blue60},
                    {review_phase::first_pass, gluten::theme::green60},
                    {review_phase::iteration_pass, gluten::theme::yellow60},
                    {review_phase::final_pass, gluten::theme::purple60}};

                static std::unordered_map<review_phase, const char*> phaseNames{
                    {review_phase::temp, "Temp"},
                    {review_phase::first_pass, "First Pass"},
                    {review_phase::iteration_pass, "Iteration Pass"},
                    {review_phase::final_pass, "Final Pass"}};

                gluten::layout phaseLayout(gluten::layout::layout_type::left_to_right);
                phaseLayout.get_element_anchor().min = ImVec2(0.0f, 0.4f);
                phaseLayout.get_element_anchor().max = ImVec2(1.0f, 0.6f);
                phaseLayout.render(descriptionBox.get_element_rect());

                for (int i = 0; i < (int)review_phase::num; ++i)
                {
                    gluten::background background;
                    background.set_element_background_color(selectedReview.m_reviewPhase == (review_phase)i
                                                                ? phaseColours[(review_phase)i]
                                                                : inactivePhaseColor);
                    background.set_element_border(2.0f, 0.0f);
                    phaseLayout.render_layout_element_percent_horizontal(&background, 1.0f / (int)review_phase::num);

                    gluten::text text(phaseNames[(review_phase)i], ImVec2(0.5f, 0.5f),
                                      gluten::element::anchor_preset::center_middle);
                    text.render(background.get_element_rect());
                }
            }

            {
                static std::unordered_map<review_quality, ImVec4> qualityColours
                {
                    {review_quality::c, gluten::theme::blue60},
                    {review_quality::b, gluten::theme::green60},
                    {review_quality::a, gluten::theme::yellow60},
                    {review_quality::a_plus, gluten::theme::purple50},
                    {review_quality::a_plus_plus, gluten::theme::purple60}
                };

                static std::unordered_map<review_quality, const char*> qualityNames
                {
                    {review_quality::c, "C"},
                    {review_quality::b, "B"},
                    {review_quality::a, "A"},
                    {review_quality::a_plus, "A+"},
                    {review_quality::a_plus_plus, "A++"}
                };

                gluten::layout qualityLayout(gluten::layout::layout_type::left_to_right);
                qualityLayout.get_element_anchor().min = ImVec2(0.0f, 0.6f);
                qualityLayout.get_element_anchor().max = ImVec2(1.0f, 0.8f);
                qualityLayout.render(descriptionBox.get_element_rect());

                for (int i = 0; i < (int)review_quality::num; ++i)
                {
                    gluten::background background;
                    background.set_element_background_color(selectedReview.m_reviewQuality == (review_quality)i ? qualityColours[(review_quality)i] : inactivePhaseColor);
                    background.set_element_border(2.0f, 0.0f);
                    qualityLayout.render_layout_element_percent_horizontal(&background, 1.0f / (int)review_quality::num);

                    gluten::text text(qualityNames[(review_quality)i], ImVec2(0.5f, 0.5f), gluten::element::anchor_preset::center_middle);
                    text.render(background.get_element_rect());
                }
            }

            {
                gluten::imgui::scoped_color frameProgressBg(ImGuiCol_FrameBg, gluten::theme::carbon_g100::layer03);

                gluten::layout scrutinyLayout(gluten::layout::layout_type::left_to_right);
                scrutinyLayout.set_element_border(2.0f, 0.0f);
                scrutinyLayout.get_element_anchor().min = ImVec2(0.0f, 0.8f);
                scrutinyLayout.get_element_anchor().max = ImVec2(1.0f, 1.0f);
                scrutinyLayout.render(descriptionBox.get_element_rect());

                constexpr float max = ((int)review_phase::num) * ((int)review_quality::num);
                constexpr float third = max * 0.33f;
                constexpr float twoThirds = max * 0.66f;
                const float value   = ((int)selectedReview.m_reviewPhase + 1) * ((int)selectedReview.m_reviewQuality + 1);
                const float fraction = value / max;

                gluten::imgui::scoped_color progressBarColor(ImGuiCol_PlotHistogram, value >= twoThirds ? gluten::theme::red60 : value >= third ? gluten::theme::yellow60 : gluten::theme::green60);

                ImGui::ProgressBar(fraction, scrutinyLayout.get_element_rect().GetSize(), "Scrutiny");
            }

            gluten::button descriptionEditButton("Edit " ICON_LC_PENCIL_LINE, false, gluten::element::anchor_preset::right_top);

            descriptionEditButton
                .set_element_alignment(ImVec2(1.0f, -0.25f))
                .set_element_translation(ImVec2(-5, 0));

            if (descriptionEditButton.render(paddedRect))
            {
                static std::shared_ptr<update_review_popup> updateProjectPopup;
                updateProjectPopup = add_child_widget<update_review_popup>(false);

                if (updateProjectPopup)
                {
                    updateProjectPopup->set_review_data(selectedReview);
                    updateProjectPopup->open_popup();
                }
            }

            gluten::background remaining;
            remaining.set_element_padding(ImVec2(ImGui::GetStyle().FramePadding.x * 2.0f, 0.0));
            remaining.get_element_anchor().maxOffset.y -= 20.0f;
            verticalLayout.render_layout_element_remaining(&remaining);

            if (ImGui::BeginChild("ReviewContent", remaining.get_element_rect().GetSize()))
            {
                if (ImGui::BeginTabBar("Tabs"))
                {
                    if (ImGui::BeginTabItem("Files"))
                    {
                        ImGui::Dummy(ImVec2(0.0f, 8.0f));

                        if (ImGui::BeginCombo("Review Version", "#1"))
                        {
                            ImGui::Selectable("#1");
                            ImGui::Selectable("#2");

                            ImGui::EndCombo();
                        }

                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Comments"))
                    {
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Activity"))
                    {
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }
            }
            ImGui::EndChild();
        }

        /*ImSpinner::SpinnerAngEclipse("Loading", ImGui::GetFontSize() / 2.0f, 2.0f, gluten::theme::white, 8.0f);
        ImGui::SameLine();
        ImGui::Text(" Hello");*/
    }
    ImGui::EndChild();
}

auto workspace_widget::render_left_toolbar() -> void
{
    gluten::imgui::scoped_color toolbarBackgroundColor(ImGuiCol_ChildBg, gluten::theme::carbon_g100::layer01);

    if (ImGui::BeginChild("LeftToolbar", ImVec2(leftToobarWidth, 0)))
    {
        gluten::layout buttonsLayout(gluten::layout::layout_type::top_to_bottom,
                                     gluten::element::anchor_preset::stretch_full);
        buttonsLayout.get_element_anchor().max.x += 0.1f;
        buttonsLayout.render_window();

        gluten::icon_button reviewsButton("##ReviewsButton", ICON_LC_CHART_NO_AXES_GANTT, gluten::fonts::regular_lucide_icons);
        reviewsButton
            .set_element_content_scale(3.0f)
            .set_element_hover_color(gluten::theme::carbon_g100::fieldHover01)
            .set_element_active_color(gluten::theme::carbon_g100::layerActive01)
            .set_element_active(m_activeView == reviews_view);

        gluten::icon_button usersButton("##UsersButton", ICON_LC_USERS, gluten::fonts::regular_lucide_icons);
        usersButton
            .set_element_content_scale(3.0f)
            .set_element_hover_color(gluten::theme::carbon_g100::fieldHover01)
            .set_element_active_color(gluten::theme::carbon_g100::layerActive01)
            .set_element_active(m_activeView == users_view);

        if (buttonsLayout.render_layout_element_pixels_vertical(&reviewsButton, leftToolbarButtonHeight))
        {
            m_activeView = reviews_view;
        }

        if (buttonsLayout.render_layout_element_pixels_vertical(&usersButton, leftToolbarButtonHeight))
        {
            m_activeView = users_view;
        }
    }
    ImGui::EndChild();
}

auto workspace_widget::render_menu_implementation() -> void
{
    if (ImGui::BeginMenu(s_fileMenuName))
    {
        if (ImGui::MenuItem("Close Workspace"))
        {
            get_app()->get_manager_by_class<workspace_manager>()->close_workspace();
        }
        ImGui::EndMenu();
    }
}