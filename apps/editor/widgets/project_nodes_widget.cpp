#include "project_nodes_widget.h"

#include "app/app.h"
#include "managers/project_manager.h"
#include "widgets/file_browser_widget.h"
#include "imgui.h"
#include "sound_bakery/core/database/database.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/editor/editor_defines.h"
#include "sound_bakery/factory.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/parameter/parameter.h"
#include "sound_bakery/profiling/voice_tracker.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"
#include "sound_bakery/editor/project/project.h"

static const std::vector<SB_OBJECT_CATEGORY> s_objectPageCategories{
    SB_CATEGORY_PARAMETER, SB_CATEGORY_BUS, SB_CATEGORY_NODE,
    SB_CATEGORY_MUSIC};

static const std::vector<SB_OBJECT_CATEGORY> s_eventPageCategories{
    SB_CATEGORY_EVENT};

static const std::vector<SB_OBJECT_CATEGORY> s_soundbankPageCategories{SB_CATEGORY_BANK};

void project_nodes_widget::RenderPage(
    const std::vector<SB_OBJECT_CATEGORY>& categories)
{
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
                RenderCreateParentOrChildMenu(category, rttr::instance(),
                                              NodeCreationType::New);
                ImGui::EndPopup();
            }

            RenderCategory(category);
            ImGui::TreePop();
        }
    }
}

void project_nodes_widget::RenderObjectsPage()
{
    RenderPage(s_objectPageCategories);
}

void project_nodes_widget::RenderEventsPage()
{
    RenderPage(s_eventPageCategories);
}

void project_nodes_widget::RenderSoundbankPage() 
{ 
    RenderPage(s_soundbankPageCategories); 
}

void project_nodes_widget::RenderCategory(SB_OBJECT_CATEGORY category)
{
    const std::unordered_set<sbk::core::object*> categoryObjects =
        sbk::engine::system::get()->get_objects_of_category(category);

    for (sbk::core::object* const object : categoryObjects)
    {
        if (object)
        {
            const rttr::type type = object->getType();

            sbk::engine::node_base* const nodeBase =
                object->try_convert_object<sbk::engine::node_base>();

            const bool notNodeType = nodeBase == nullptr;

            if (notNodeType || (nodeBase && nodeBase->parent() == nullptr))
            {
                RenderSingleNode(type, rttr::instance(object));
            }
        }
    }
}

void project_nodes_widget::RenderSingleNode(rttr::type type,
                                          rttr::instance instance)
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

            const bool hasChildren = node && NodeHasChildren(node);

            HandleOpenNode(object);

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_None | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NavLeftJumpsBackHere;

            if (hasChildren  || object->getType() ==
                                   sbk::engine::NamedParameter::type())
            {
                flags |= ImGuiTreeNodeFlags_OpenOnArrow;
            }
            else
            {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }

            const bool opened = ImGui::TreeNodeEx(
                fmt::format("##{}", object->get_database_name()).c_str(), flags);

            if (std::string_view payloadString =
                    sbk::util::type_helper::getPayloadFromType(object->getType());
                payloadString.size())
            {
                if (ImGui::BeginDragDropSource(
                        ImGuiDragDropFlags_SourceAllowNullID))
                {
                    sbk_id dragID = object->get_database_id();

                    ImGui::SetDragDropPayload(payloadString.data(), &dragID,
                                              sizeof(sbk_id), ImGuiCond_Once);

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
            }

            const bool nodeClicked = ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !ImGui::GetDragDropPayload();
            const bool nodeKeyboardFocused = ImGui::IsItemFocused() && !nodeClicked;

            if (nodeClicked || nodeKeyboardFocused)
            {
                get_app()->get_manager_by_class<project_manager>()->get_selection().selected_object(
                    object);
            }

            if (RenderNodeContextMenu(type, instance))
            {
                ImGui::SameLine();

                if (ObjectIsRenaming(object))
                {
                    RenderRenameObject(object);
                }
                else
                {
                    ImGui::Text(ICON_FAD_FILTER_BELL " %s",
                                object->get_database_name().data());

                    if (unsigned int playingCount =
                            sbk::engine::Profiling::VoiceTracker::get()
                                ->getPlayingCountOfObject(
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
                            RenderSingleNode(type, child);
                        }
                    }
                    else if (sbk::engine::NamedParameter* const intParameter =
                                 object->try_convert_object<
                                     sbk::engine::NamedParameter>())
                    {
                        for (const sbk::core::database_ptr<
                                 sbk::engine::NamedParameterValue>& value :
                             intParameter->getValues())
                        {
                            if (value.lookup())
                            {
                                RenderSingleNode(type,
                                                 rttr::instance(value.raw()));
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

bool project_nodes_widget::NodeHasChildren(sbk::engine::node* node)
{
    return node ? node->getChildCount() : false;
}

void project_nodes_widget::HandleOpenNode(sbk::core::database_object* object)
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

bool project_nodes_widget::ObjectIsRenaming(sbk::core::database_object* object)
{
    return object && object->get_database_id() &&
           object->get_database_id() == m_renameID;
}

void project_nodes_widget::RenderRenameObject(
    sbk::core::database_object* const& object)
{
    if (ImGui::InputText("###rename", m_renameString, 255,
                         ImGuiInputTextFlags_AutoSelectAll |
                             ImGuiInputTextFlags_EnterReturnsTrue))
    {
        object->set_database_name(m_renameString);
        m_renameID        = 0;
        m_renameString[0] = '\0';
    }

    ImGui::SetKeyboardFocusHere(-1);
}

bool project_nodes_widget::RenderNodeContextMenu(rttr::type type,
                                               rttr::instance instance)
{
    bool result = false;

    if (instance)
    {
        result = true;

        if (sbk::core::database_object* const object = sbk::util::type_helper::getDatabaseObjectFromInstance(instance))
        {
            if (ImGui::BeginPopupContextItem(
                    std::to_string(object->get_database_id()).c_str()))
            {
                const SB_OBJECT_CATEGORY category =
                    sbk::util::type_helper::getCategoryFromType(type);

                if (object->getType().is_derived_from(
                        sbk::engine::node_base::type()))
                {
                    RenderCreateParentOrChildMenu(category, instance,
                                                  NodeCreationType::NewParent);

                    const sbk::engine::node_base* const nodeBase = object->try_convert_object<sbk::engine::node_base>();

                    if (nodeBase->can_add_children())
                    {
                        RenderCreateParentOrChildMenu(category, instance, NodeCreationType::NewChild);
                    }

                    ImGui::Separator();
                }

                if (object->getType() ==
                    sbk::engine::NamedParameter::type())
                {
                    if (ImGui::MenuItem("Create New Value"))
                    {
                        if (sbk::engine::NamedParameter* const intParameter =
                                object->try_convert_object<
                                    sbk::engine::NamedParameter>())
                        {
                            intParameter->addNewValue("New Switch Value");
                        }
                    }

                    ImGui::Separator();
                }

                if (ImGui::MenuItem("Rename"))
                {
                    SetupRenameNode(object);
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Delete"))
                {
                    get_app()->get_manager_by_class<project_manager>()->get_selection().selected_object(
                        nullptr);
                    sbk::engine::system::get()->remove_object_from_database(object->get_database_id());
                    result = false;
                }

                ImGui::EndPopup();
            }
        }
    }

    return result;
}

void project_nodes_widget::RenderCreateParentOrChildMenu(
    SB_OBJECT_CATEGORY category,
    rttr::instance node,
    NodeCreationType creationType)
{
    m_renameID = 0;

    const std::unordered_set<rttr::type> categoryTypes =
        sbk::util::type_helper::getTypesFromCategory(category);

    sbk::engine::node* const castedNode = sbk::util::type_helper::getNodeFromInstance(node);

    if (creationType == NodeCreationType::NewChild && !castedNode->can_add_children())
    {
        return;
    }

    if (ImGui::BeginMenu(CreateParentOrChildMenuName(creationType).data()))
    {
        for (const rttr::type type : categoryTypes)
        {
            const rttr::string_view typeIndexName =
                sbk::util::type_helper::get_display_name_from_type(type).data();

            if (creationType == NodeCreationType::NewChild && !castedNode->can_add_child_type(type))
            {
                continue;
            }

            if (ImGui::MenuItem(typeIndexName.data()))
            {
                std::shared_ptr<sbk::core::database_object> const newObject =
                    sbk::engine::system::get()->get_project()->create_database_object(type);

                assert(newObject);

                SetupRenameNode(newObject.get());

                sbk::engine::node* newNode =
                    sbk::reflection::cast<sbk::engine::node*, sbk::core::object*>(
                        newObject.get());

                if (newNode)
                {
                    switch (creationType)
                    {
                        case NodeCreationType::NewParent:
                        {
                            if (sbk::engine::node_base* baseParent =
                                    castedNode->parent())
                            {
                                m_nodeToOpen = baseParent->get_database_id();
                                baseParent->addChild(newNode);
                                baseParent->removeChild(castedNode);
                            }

                            newNode->addChild(castedNode);
                            break;
                        }
                        case NodeCreationType::NewChild:
                        {
                            m_nodeToOpen = castedNode->get_database_id();
                            castedNode->addChild(newNode);
                            break;
                        }
                        case NodeCreationType::New:
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

std::string_view project_nodes_widget::CreateParentOrChildMenuName(
    NodeCreationType creationType)
{
    std::string_view menuName;

    switch (creationType)
    {
        case NodeCreationType::New:
            menuName = "Create New";
            break;
        case NodeCreationType::NewParent:
            menuName = "Create Parent";
            break;
        case NodeCreationType::NewChild:
            menuName = "Create Child";
            break;
        default:
            break;
    }

    return menuName;
}

void project_nodes_widget::SetupRenameNode(sbk::core::database_object* object)
{
    m_renameID                = object->get_database_id();
    std::string_view nodeName = object->get_database_name();
    nodeName.copy(m_renameString, 255);
    m_renameString[nodeName.length()] = '\0';
}