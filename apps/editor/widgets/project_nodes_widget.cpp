#include "project_nodes_widget.h"

#include "app/app.h"
#include "gluten/theme/carbon_theme_g100.h"
#include "gluten/utils/imgui_util_structures.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "managers/project_manager.h"
#include "sound_bakery/core/database/database.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/editor/editor_defines.h"
#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/factory.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/parameter/parameter.h"
#include "sound_bakery/profiling/voice_tracker.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"
#include "widgets/file_browser_widget.h"

static const std::vector<SB_OBJECT_CATEGORY> s_objectPageCategories{SB_CATEGORY_PARAMETER, SB_CATEGORY_BUS,
                                                                    SB_CATEGORY_NODE, SB_CATEGORY_MUSIC};

static const std::vector<SB_OBJECT_CATEGORY> s_eventPageCategories{SB_CATEGORY_EVENT};

static const std::vector<SB_OBJECT_CATEGORY> s_soundbankPageCategories{SB_CATEGORY_BANK};

void project_nodes_widget::render_page(const std::vector<SB_OBJECT_CATEGORY>& categories)
{
    ImGui::BeginChild("##page");
    for (const SB_OBJECT_CATEGORY category : categories)
    {
        rttr::string_view categoryName = sbk::util::type_helper::getObjectCategoryName(category);

        if (categoryName.empty())
        {
            categoryName = "Null. Please add enum value to reflection file";
        }

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);

        if (ImGui::TreeNodeEx(categoryName.data(), ImGuiTreeNodeFlags_NavLeftJumpsBackHere))
        {
            if (ImGui::BeginPopupContextItem("TopNodeContext"))
            {
                render_create_parent_or_child_menu(category, rttr::instance(), node_creation_type::New);
                ImGui::EndPopup();
            }

            render_category(category);
            ImGui::TreePop();
        }
    }
    ImGui::EndChild();
}

void project_nodes_widget::render_objects_page() { render_page(s_objectPageCategories); }

void project_nodes_widget::render_events_page() { render_page(s_eventPageCategories); }

void project_nodes_widget::render_soundbank_page() { render_page(s_soundbankPageCategories); }

void project_nodes_widget::render_category(SB_OBJECT_CATEGORY category)
{
    const std::unordered_set<sbk::core::object*> categoryObjects =
        sbk::engine::system::get()->get_objects_of_category(category);


    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float x1             = ImGui::GetCurrentWindow()->WorkRect.Min.x;
    float x2             = ImGui::GetCurrentWindow()->WorkRect.Max.x;
    float item_spacing_y = ImGui::GetStyle().ItemSpacing.y;
    float item_offset_y  = -item_spacing_y * 0.5f;
    float line_height    = ImGui::GetTextLineHeight() + item_spacing_y;

    float y0 = ImGui::GetCursorScreenPos().y + (float)(int)item_offset_y;
   
    const auto pos = ImGui::GetCursorPos();
    ImGuiListClipper clipper;
    clipper.Begin(categoryObjects.size(), line_height);
    while (clipper.Step())
    {
        for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; ++row_n)
        {
            ImU32 col = (row_n & 1) ? ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::layer02)
                                    : ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::layer03);
            if ((col & IM_COL32_A_MASK) == 0)
                continue;
            float y1 = y0 + (line_height * static_cast<float>(row_n));
            float y2 = y1 + line_height;
            draw_list->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), col);
        }
    }
    ImGui::SetCursorPos(pos);

    for (sbk::core::object* const object : categoryObjects)
    {
        if (object)
        {
            const rttr::type type = object->getType();

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
        if (sbk::core::database_object* const object = sbk::util::type_helper::getDatabaseObjectFromInstance(instance))
        {
            if (object->get_editor_hidden())
            {
                return;
            }

            sbk::engine::node* const node = rttr::rttr_cast<sbk::engine::node*, sbk::core::database_object*>(object);

            const bool hasChildren = node && node_has_children(node);

            handle_open_node(object);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None |
                                       ImGuiTreeNodeFlags_NavLeftJumpsBackHere | ImGuiTreeNodeFlags_SpanFullWidth;

            if (hasChildren || object->getType() == sbk::engine::named_parameter::type())
            {
                flags |= ImGuiTreeNodeFlags_OpenOnArrow;
            }
            else
            {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }

            static ImGuiID previousFocusID = 0;

            const bool opened = ImGui::TreeNodeEx(fmt::format("##{}", object->get_database_name()).c_str(), flags);

            if (std::string_view payloadString = sbk::util::type_helper::getPayloadFromType(object->getType());
                payloadString.size())
            {
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    sbk_id dragID = object->get_database_id();

                    ImGui::SetDragDropPayload(payloadString.data(), &dragID, sizeof(sbk_id), ImGuiCond_Once);

                    ImGui::TextUnformatted(object->get_database_name().data());

                    ImGui::EndDragDropSource();
                }

                if (ImGui::BeginDragDropTarget())
                {
                    if (node != nullptr)
                    {
                        if (const ImGuiPayload* const currentPayload = ImGui::GetDragDropPayload())
                        {
                            sbk_id payloadID = *static_cast<sbk_id*>(currentPayload->Data);
                            sbk::core::database_ptr<sbk::engine::node_base> potentialChild(payloadID);

                            if (node->can_add_child(potentialChild))
                            {
                                if (const ImGuiPayload* const payload =
                                        ImGui::AcceptDragDropPayload(currentPayload->DataType))
                                {
                                    node->addChild(potentialChild);
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

            const bool nodeClicked =
                ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !ImGui::GetDragDropPayload();
            const bool nodeKeyboardFocused =
                ImGui::IsItemFocused() && !nodeClicked && !ImGui::IsAnyMouseDown() && !ImGui::GetDragDropPayload();

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
                    ImGui::Text(ICON_FAD_FILTER_BELL " %s", object->get_database_name().data());

                    if (unsigned int playingCount =
                            sbk::engine::Profiling::VoiceTracker::get()->getPlayingCountOfObject(
                                object->get_database_id()))
                    {
                        ImGui::SameLine();
                        ImGui::Text("|%u|", playingCount);
                    }
                }

                if (opened)
                {
                    if (hasChildren)
                    {
                        for (auto child : node->getChildren())
                        {
                            render_single_node(type, child);
                        }
                    }
                    else if (sbk::engine::named_parameter* const intParameter =
                                 object->try_convert_object<sbk::engine::named_parameter>())
                    {
                        for (const sbk::core::database_ptr<sbk::engine::named_parameter_value>& value :
                             intParameter->get_values())
                        {
                            if (value.lookup())
                            {
                                render_single_node(type, rttr::instance(value.raw()));
                            }
                        }
                    }
                }
            }

            if (opened)
            {
                ImGui::TreePop();
            }
        }
    }
}

bool project_nodes_widget::node_has_children(sbk::engine::node* node) { return node ? node->getChildCount() : false; }

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
        object->set_database_name(m_renameString);
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

        if (sbk::core::database_object* const object = sbk::util::type_helper::getDatabaseObjectFromInstance(instance))
        {
            if (ImGui::BeginPopupContextItem(std::to_string(object->get_database_id()).c_str()))
            {
                const SB_OBJECT_CATEGORY category = sbk::util::type_helper::getCategoryFromType(type);

                if (object->getType().is_derived_from(sbk::engine::node_base::type()))
                {
                    render_create_parent_or_child_menu(category, instance, node_creation_type::NewParent);

                    const sbk::engine::node_base* const nodeBase = object->try_convert_object<sbk::engine::node_base>();

                    if (nodeBase->can_add_children())
                    {
                        render_create_parent_or_child_menu(category, instance, node_creation_type::NewChild);
                    }

                    ImGui::Separator();
                }

                if (object->getType() == sbk::engine::named_parameter::type())
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
                    object->destroy();
                    result = false;
                }

                ImGui::EndPopup();
            }
        }
    }

    return result;
}

void project_nodes_widget::render_create_parent_or_child_menu(SB_OBJECT_CATEGORY category,
                                                              rttr::instance node,
                                                              node_creation_type creationType)
{
    m_renameID = 0;

    const std::unordered_set<rttr::type> categoryTypes = sbk::util::type_helper::getTypesFromCategory(category);

    sbk::engine::node* const castedNode = sbk::util::type_helper::getNodeFromInstance(node);

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
                std::shared_ptr<sbk::core::database_object> const newObject =
                    sbk::engine::system::get()->get_project()->create_database_object(type);

                assert(newObject);

                setup_rename_node(newObject.get());

                sbk::engine::node* newNode =
                    sbk::reflection::cast<sbk::engine::node*, sbk::core::object*>(newObject.get());

                if (newNode)
                {
                    switch (creationType)
                    {
                        case node_creation_type::NewParent:
                        {
                            if (sbk::engine::node_base* baseParent = castedNode->get_parent())
                            {
                                m_nodeToOpen = baseParent->get_database_id();
                                baseParent->addChild(newNode);
                                baseParent->removeChild(castedNode);
                            }

                            newNode->addChild(castedNode);
                            break;
                        }
                        case node_creation_type::NewChild:
                        {
                            m_nodeToOpen = castedNode->get_database_id();
                            castedNode->addChild(newNode);
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
                    m_nodeToOpen = newObject->get_database_id();
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
    std::string_view nodeName = object->get_database_name();
    nodeName.copy(m_renameString, 255);
    m_renameString[nodeName.length()] = '\0';
}