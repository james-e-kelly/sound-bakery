#include "project_nodes_widget.h"

#include "sound_bakery/core/database/database.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/editor/editor_defines.h"
#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/parameter/parameter.h"
#include "sound_bakery/profiling/voice_tracker.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"

#include "app/app.h"
#include "gluten/theme/theme.h"
#include "gluten/utils/imgui_util_functions.h"
#include "gluten/utils/imgui_util_structures.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "managers/project_manager.h"
#include "widgets/file_browser_widget.h"

static const std::vector<sbk::memory::object_category> s_objectPageCategories{sbk::memory::object_category::parameter, sbk::memory::object_category::bus,
                                                                              sbk::memory::object_category::node, sbk::memory::object_category::music};

static const std::vector<sbk::memory::object_category> s_eventPageCategories{sbk::memory::object_category::event};

static const std::vector<sbk::memory::object_category> s_soundbankPageCategories{sbk::memory::object_category::bank};

void project_nodes_widget::render_page(const std::vector<sbk::memory::object_category>& categories)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    const gluten::imgui::scoped_color frameIsBackgroundColor(ImGuiCol_FrameBg, gluten::theme::background);
    const gluten::imgui::scoped_style borderAroundTable(ImGuiStyleVar_FrameBorderSize, 2.0f);

    if (ImGui::BeginChild("##Page", ImVec2(0, 0), ImGuiChildFlags_FrameStyle))
    {
        ImGui::PopStyleVar();

        for (const sbk::memory::object_category category : categories)
        {
            rttr::string_view categoryName = sbk::util::type_helper::get_object_category_name(category);

            if (categoryName.empty())
            {
                categoryName = "Null. Please add enum value to reflection file";
            }

            ImGui::SetNextItemOpen(true, ImGuiCond_Once);

            if (ImGui::TreeNodeEx(categoryName.data(),
                                  ImGuiTreeNodeFlags_NavLeftJumpsBackHere | ImGuiTreeNodeFlags_SpanFullWidth))
            {
                if (gluten::imgui::scoped_context_menu contextMenu{"TopNodeContext"})
                {
                    render_create_parent_or_child_menu(category, rttr::instance(), node_creation_type::New);
                }

                render_category(category);
                ImGui::TreePop();
            }
        }
    }
    ImGui::EndChild();
}

void project_nodes_widget::render_objects_page() { render_page(s_objectPageCategories); }

void project_nodes_widget::render_events_page() { render_page(s_eventPageCategories); }

void project_nodes_widget::render_soundbank_page() { render_page(s_soundbankPageCategories); }

static int numNodesRendered = 0;

void project_nodes_widget::render_category(sbk::memory::object_category category)
{
    const std::set<sbk::core::object*, sbk::core::object_ptr_comparator> categoryObjects = sbk::engine::system::get()->convert_to_ordered(sbk::engine::system::get()->get_objects_of_category(category));

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float x1             = ImGui::GetCurrentWindow()->WorkRect.Min.x;
    float x2             = ImGui::GetCurrentWindow()->WorkRect.Max.x;
    float item_spacing_y = ImGui::GetStyle().ItemSpacing.y;
    float item_offset_y  = -item_spacing_y * 0.5f;
    float line_height    = ImGui::GetTextLineHeight() + item_spacing_y;

    float y0 = ImGui::GetCursorScreenPos().y + (float)(int)item_offset_y;

    const auto pos = ImGui::GetCursorPos();

    numNodesRendered = categoryObjects.size();

    ImGuiListClipper clipper;
    clipper.Begin(numNodesRendered, line_height);
    while (clipper.Step())
    {
        for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; ++row_n)
        {
            ImU32 col = (row_n & 1) ? ImGui::ColorConvertFloat4ToU32(gluten::theme::background) : ImGui::ColorConvertFloat4ToU32(gluten::theme::color_with_multiplied_value(gluten::theme::backgroundSelected, 0.22f));
            if ((col & IM_COL32_A_MASK) == 0)
                continue;
            float y1 = y0 + (line_height * static_cast<float>(row_n));
            float y2 = y1 + line_height;
            draw_list->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), col);
        }
    }

    if (numNodesRendered > 0)
    {
        ImGui::SetCursorPos(pos);
    }

    for (sbk::core::object* const object : categoryObjects)
    {
        if (object)
        {
            const rttr::type type = object->get_object_type();

            sbk::engine::node_base* const nodeBase = object->try_convert_object<sbk::engine::node_base>();

            const bool notNodeType = nodeBase == nullptr;

            if (notNodeType || (nodeBase && nodeBase->get_parent() == nullptr))
            {
                render_single_node(type, rttr::instance(object));
            }
        }
    }
}

void project_nodes_widget::render_single_node(rttr::type type, rttr::instance instance)
{
    if (instance)
    {
        if (sbk::core::database_object* const object = sbk::util::type_helper::get_database_object_from_instance(instance))
        {
            if (object->get_editor_hidden())
            {
                return;
            }

            gluten::imgui::scoped_id uniqueObjectID(object->get_database_id());

            sbk::engine::node* const node = rttr::rttr_cast<sbk::engine::node*, sbk::core::database_object*>(object);

            const bool hasChildren = node && node_has_children(node);

            handle_open_node(object);

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_None | ImGuiTreeNodeFlags_NavLeftJumpsBackHere | ImGuiTreeNodeFlags_SpanFullWidth;

            if (hasChildren || object->get_object_type() == sbk::engine::named_parameter::type())
            {
                flags |= ImGuiTreeNodeFlags_OpenOnArrow;
            }
            else
            {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }

            static ImGuiID previousFocusID = 0;

            const bool opened = ImGui::TreeNodeEx(fmt::format("##{}", object->get_object_name()).c_str(), flags);

            if (std::string_view payloadString = sbk::util::type_helper::get_payload_from_type(object->get_object_type());
                payloadString.size())
            {
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    sbk_id dragID = object->get_database_id();

                    ImGui::SetDragDropPayload(payloadString.data(), &dragID, sizeof(sbk_id), ImGuiCond_Once);

                    ImGui::TextUnformatted(object->get_object_name().data());

                    ImGui::EndDragDropSource();
                }

                if (ImGui::BeginDragDropTarget())
                {
                    if (node != nullptr && node->get_database_id() != SBK_INVALID_ID)
                    {
                        if (const ImGuiPayload* const currentPayload = ImGui::GetDragDropPayload())
                        {
                            sbk_id payloadID = *static_cast<sbk_id*>(currentPayload->Data);
                            sbk::core::database_ptr<sbk::engine::node_base> potentialChild(payloadID);

                            if (node->can_add_child(potentialChild))
                            {
                                if (const ImGuiPayload* const payload = ImGui::AcceptDragDropPayload(currentPayload->DataType))
                                {
                                    node->add_child(potentialChild);
                                }
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // Using drag drop changes the focus to the dragged element
                // This focus therefore shifts the selected object to the focus once complete
                // This code sets the focus to the previous focus when a drag drop ends
                static bool previousDragDropActive = false;

                const bool dragDropActive = ImGui::IsDragDropActive();

                if (!dragDropActive && previousDragDropActive && previousFocusID != 0)
                {
                    ImGui::SetFocusID(previousFocusID, ImGui::GetCurrentWindow());
                }

                previousDragDropActive = dragDropActive;
            }

            const bool nodeClicked         = ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !ImGui::GetDragDropPayload();
            const bool nodeKeyboardFocused = ImGui::IsItemFocused() && !nodeClicked && !ImGui::IsAnyMouseDown() && !ImGui::GetDragDropPayload();

            if (nodeClicked || nodeKeyboardFocused)
            {
                get_app()->get_manager_by_class<project_manager>()->get_selection().selected_object(object);

                previousFocusID = ImGui::GetFocusID();
            }

            if (render_node_context_menu(type, instance))
            {
                ImGui::SameLine();

                if (object_is_renaming(object))
                {
                    render_rename_object(object);
                }
                else
                {
                    ImGui::Text(ICON_FAD_FILTER_BELL " %s", object->get_object_name().data());

                    if (sbk::engine::profiling::voice_tracker* const voiceTracker = sbk::engine::system::get()->get_voice_tracker())
                    {
                        if (unsigned int playingCount = voiceTracker->get_playing_count_of_object(object->get_database_id()))
                        {
                            ImGui::SameLine();
                            ImGui::Text("|%u|", playingCount);
                        }
                    }
                }

                if (opened)
                {
                    if (hasChildren)
                    {
                        for (auto child : node->get_children())
                        {
                            render_single_node(type, child);
                        }
                    }
                    else if (sbk::engine::named_parameter* const intParameter = object->try_convert_object<sbk::engine::named_parameter>())
                    {
                        for (const sbk::core::database_ptr<sbk::engine::named_parameter_value>& value : intParameter->get_values())
                        {
                            if (const auto valueShared = value.shared())
                            {
                                render_single_node(type, rttr::instance(valueShared.get()));
                            }
                        }
                    }
                }
            }

            if (opened)
            {
                ++numNodesRendered;
                ImGui::TreePop();
            }
        }
    }
}

bool project_nodes_widget::node_has_children(sbk::engine::node* node) { return node ? node->get_child_count() : false; }

void project_nodes_widget::handle_open_node(sbk::core::database_object* object)
{
    if (object)
    {
        if (m_nodeToOpen == object->get_database_id())
        {
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            m_nodeToOpen = 0;
        }
    }
}

bool project_nodes_widget::object_is_renaming(sbk::core::database_object* object)
{
    return object && object->get_database_id() && object->get_database_id() == m_renameID;
}

void project_nodes_widget::render_rename_object(sbk::core::database_object* const& object)
{
    if (ImGui::InputText("###rename", m_renameString, 255,
                         ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
    {
        object->set_object_name(m_renameString);
        m_renameID        = 0;
        m_renameString[0] = '\0';
    }

    ImGui::SetKeyboardFocusHere(-1);
}

bool project_nodes_widget::render_node_context_menu(rttr::type type, rttr::instance instance)
{
    bool result = false;

    if (instance)
    {
        result = true;

        if (sbk::core::database_object* const object = sbk::util::type_helper::get_database_object_from_instance(instance))
        {
            if (gluten::imgui::scoped_context_menu contextMenu{std::to_string(object->get_database_id()).c_str()})
            {
                const sbk::memory::object_category category = sbk::util::type_helper::get_category_from_type(type);

                if (object->get_object_type().is_derived_from(sbk::engine::node_base::type()))
                {
                    render_create_parent_or_child_menu(category, instance, node_creation_type::NewParent);

                    const sbk::engine::node_base* const nodeBase = object->try_convert_object<sbk::engine::node_base>();

                    if (nodeBase->can_add_children())
                    {
                        render_create_parent_or_child_menu(category, instance, node_creation_type::NewChild);
                    }

                    ImGui::Separator();
                }

                if (object->get_object_type().is_derived_from(sbk::engine::sound::type()))
                {
                    if (ImGui::MenuItem("Create Sound Node"))
                    {
                        auto createdSoundNodeResult = object->get_owner()->create_database_object<sbk::engine::sound_container>();
                        if (createdSoundNodeResult.has_value())
                        {
                            createdSoundNodeResult.value()->set_object_name(object->get_database_name());
                            createdSoundNodeResult.value()->set_sound(object->try_convert_object<sbk::engine::sound>());
                            get_app()->get_manager_by_class<project_manager>()->get_selection().selected_object(createdSoundNodeResult.value().get());
                        }
                    }
                    ImGui::Separator();
                }

                if (object->get_object_type() == sbk::engine::named_parameter::type())
                {
                    if (ImGui::MenuItem("Create New Value"))
                    {
                        if (sbk::engine::named_parameter* const intParameter =
                                object->try_convert_object<sbk::engine::named_parameter>())
                        {
                            intParameter->add_new_value("New Switch Value");
                        }
                    }

                    ImGui::Separator();
                }

                if (ImGui::MenuItem("Rename"))
                {
                    setup_rename_node(object);
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Delete"))
                {
                    get_app()->get_manager_by_class<project_manager>()->get_selection().selected_object(nullptr);
                    object->get_owner()->remove_object(object->shared_from_this());
                    result = false;
                }
            }
        }
    }

    return result;
}

void project_nodes_widget::render_create_parent_or_child_menu(sbk::memory::object_category category,
                                                              rttr::instance node,
                                                              node_creation_type creationType)
{
    m_renameID = 0;

    const std::set<rttr::type, sbk::util::type_comparator> categoryTypes = sbk::util::type_helper::get_types_from_category(category);

    sbk::engine::node* const castedNode = sbk::util::type_helper::get_node_from_instance(node);

    if (creationType == node_creation_type::NewChild && !castedNode->can_add_children())
    {
        return;
    }

    if (creationType == node_creation_type::NewParent && !castedNode->can_add_parent())
    {
        return;
    }

    if (ImGui::BeginMenu(create_parent_or_child_menu_name(creationType).data()))
    {
        for (const rttr::type type : categoryTypes)
        {
            const rttr::string_view typeIndexName = sbk::util::type_helper::get_display_name_from_type(type).data();

            if (creationType == node_creation_type::NewChild && !castedNode->can_add_child_type(type))
            {
                continue;
            }

            if (creationType == node_creation_type::NewParent && !castedNode->can_add_parent_type(type))
            {
                continue;
            }

            if (ImGui::MenuItem(typeIndexName.data()))
            {
                auto newObjectResult = sbk::engine::system::get()->get_project()->create_database_object(type);
                assert(newObjectResult.has_value());

                setup_rename_node(newObjectResult.value().get());

                sbk::engine::node* newNode = sbk::cast<sbk::engine::node*, sbk::core::object*>(newObjectResult.value().get());

                if (newNode)
                {
                    switch (creationType)
                    {
                        case node_creation_type::NewParent:
                        {
                            if (auto baseParent = castedNode->get_parent())
                            {
                                m_nodeToOpen = baseParent->get_database_id();
                                baseParent->add_child(newNode);
                                baseParent->remove_child(castedNode);
                            }

                            newNode->add_child(castedNode);
                            break;
                        }
                        case node_creation_type::NewChild:
                        {
                            m_nodeToOpen = castedNode->get_database_id();
                            castedNode->add_child(newNode);
                            break;
                        }
                        case node_creation_type::New:
                            m_nodeToOpen = newNode->get_database_id();
                            break;
                        default:
                            break;
                    }
                }
                else
                {
                    m_nodeToOpen = newObjectResult.value()->get_database_id();
                }
            }
        }
        ImGui::EndMenu();
    }
}

std::string_view project_nodes_widget::create_parent_or_child_menu_name(node_creation_type creationType)
{
    std::string_view menuName;

    switch (creationType)
    {
        case node_creation_type::New:
            menuName = "Create New";
            break;
        case node_creation_type::NewParent:
            menuName = "Create Parent";
            break;
        case node_creation_type::NewChild:
            menuName = "Create Child";
            break;
        default:
            break;
    }

    return menuName;
}

void project_nodes_widget::setup_rename_node(sbk::core::database_object* object)
{
    m_renameID                = object->get_database_id();
    std::string_view nodeName = object->get_object_name();
    nodeName.copy(m_renameString, 255);
    m_renameString[nodeName.length()] = '\0';
}