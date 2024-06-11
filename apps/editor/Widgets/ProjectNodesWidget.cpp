#include "ProjectNodesWidget.h"

#include "App/App.h"
#include "Managers/ProjectManager.h"
#include "Widgets/FileBrowserWidget.h"
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

static const std::vector<SB_OBJECT_CATEGORY> s_objectPageCategories{
    SB_CATEGORY_PARAMETER, SB_CATEGORY_BUS, SB_CATEGORY_NODE,
    SB_CATEGORY_MUSIC};

static const std::vector<SB_OBJECT_CATEGORY> s_eventPageCategories{
    SB_CATEGORY_EVENT};

static const std::vector<SB_OBJECT_CATEGORY> s_soundbankPageCategories{SB_CATEGORY_BANK};

void ProjectNodesWidget::RenderPage(
    const std::vector<SB_OBJECT_CATEGORY>& categories)
{
    for (const SB_OBJECT_CATEGORY category : categories)
    {
        rttr::string_view categoryName = SB::Util::TypeHelper::getObjectCategoryName(category);

        if (categoryName.empty())
        {
            categoryName = "Null. Please add enum value to reflection file";
        }

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);

        if (ImGui::TreeNode(categoryName.data()))
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

void ProjectNodesWidget::RenderObjectsPage()
{
    RenderPage(s_objectPageCategories);
}

void ProjectNodesWidget::RenderEventsPage()
{
    RenderPage(s_eventPageCategories);
}

void ProjectNodesWidget::RenderSoundbankPage() 
{ 
    RenderPage(s_soundbankPageCategories); 
}

void ProjectNodesWidget::RenderCategory(SB_OBJECT_CATEGORY category)
{
    const std::unordered_set<SB::Core::object*> categoryObjects =
        SB::Engine::System::getObjectTracker()->getObjectsOfCategory(category);

    for (SB::Core::object* const object : categoryObjects)
    {
        if (object)
        {
            const rttr::type type = object->getType();

            SB::Engine::NodeBase* const nodeBase =
                object->try_convert_object<SB::Engine::NodeBase>();

            const bool notNodeType = nodeBase == nullptr;

            if (notNodeType || (nodeBase && nodeBase->parent() == nullptr))
            {
                RenderSingleNode(type, rttr::instance(object));
            }
        }
    }
}

void ProjectNodesWidget::RenderSingleNode(rttr::type type,
                                          rttr::instance instance)
{
    if (instance)
    {
        if (SB::Core::database_object* const object = SB::Util::TypeHelper::getDatabaseObjectFromInstance(instance))
        {
            if (object->get_editor_hidden())
            {
                return;
            }

            SB::Engine::Node* const node = rttr::rttr_cast<SB::Engine::Node*, SB::Core::database_object*>(object);

            const bool hasChildren = node && NodeHasChildren(node);

            HandleOpenNode(object);

            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_None | ImGuiTreeNodeFlags_SpanAvailWidth;

            if (hasChildren  || object->getType() ==
                                   SB::Engine::NamedParameter::type())
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
                    SB::Util::TypeHelper::getPayloadFromType(object->getType());
                payloadString.size())
            {
                if (ImGui::BeginDragDropSource(
                        ImGuiDragDropFlags_SourceAllowNullID))
                {
                    sb_id dragID = object->get_database_id();

                    ImGui::SetDragDropPayload(payloadString.data(), &dragID,
                                              sizeof(sb_id), ImGuiCond_Once);

                    ImGui::TextUnformatted(object->get_database_name().data());

                    ImGui::EndDragDropSource();
                }

                if (ImGui::BeginDragDropTarget())
                {
                    if (node != nullptr)
                    {
                        if (const ImGuiPayload* const currentPayload = ImGui::GetDragDropPayload())
                        {
                            sb_id payloadID = *static_cast<sb_id*>(currentPayload->Data);
                            SB::Core::DatabasePtr<SB::Engine::NodeBase> potentialChild(payloadID);

                            if (node->canAddChild(potentialChild))
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

            if (ImGui::IsItemHovered() &&
                ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                !ImGui::GetDragDropPayload())
            {
                get_app()->GetProjectManager()->GetSelection().SelectObject(
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
                            SB::Engine::Profiling::VoiceTracker::get()
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
                    else if (SB::Engine::NamedParameter* const intParameter =
                                 object->try_convert_object<
                                     SB::Engine::NamedParameter>())
                    {
                        for (const SB::Core::DatabasePtr<
                                 SB::Engine::NamedParameterValue>& value :
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

bool ProjectNodesWidget::NodeHasChildren(SB::Engine::Node* node)
{
    return node ? node->getChildCount() : false;
}

void ProjectNodesWidget::HandleOpenNode(SB::Core::database_object* object)
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

bool ProjectNodesWidget::ObjectIsRenaming(SB::Core::database_object* object)
{
    return object && object->get_database_id() &&
           object->get_database_id() == m_renameID;
}

void ProjectNodesWidget::RenderRenameObject(
    SB::Core::database_object* const& object)
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

bool ProjectNodesWidget::RenderNodeContextMenu(rttr::type type,
                                               rttr::instance instance)
{
    bool result = false;

    if (instance)
    {
        result = true;

        if (SB::Core::database_object* const object = SB::Util::TypeHelper::getDatabaseObjectFromInstance(instance))
        {
            if (ImGui::BeginPopupContextItem(
                    std::to_string(object->get_database_id()).c_str()))
            {
                const SB_OBJECT_CATEGORY category =
                    SB::Util::TypeHelper::getCategoryFromType(type);

                if (object->getType().is_derived_from(
                        SB::Engine::NodeBase::type()))
                {
                    RenderCreateParentOrChildMenu(category, instance,
                                                  NodeCreationType::NewParent);
                    RenderCreateParentOrChildMenu(category, instance,
                                                  NodeCreationType::NewChild);
                    ImGui::Separator();
                }

                if (object->getType() ==
                    SB::Engine::NamedParameter::type())
                {
                    if (ImGui::MenuItem("Create New Value"))
                    {
                        if (SB::Engine::NamedParameter* const intParameter =
                                object->try_convert_object<
                                    SB::Engine::NamedParameter>())
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
                    get_app()->GetProjectManager()->GetSelection().SelectObject(
                        nullptr);
                    SB::Engine::System::getDatabase()->remove(object);
                    result = false;
                }

                ImGui::EndPopup();
            }
        }
    }

    return result;
}

void ProjectNodesWidget::RenderCreateParentOrChildMenu(
    SB_OBJECT_CATEGORY category,
    rttr::instance node,
    NodeCreationType creationType)
{
    m_renameID = 0;

    const std::unordered_set<rttr::type> categoryTypes =
        SB::Util::TypeHelper::getTypesFromCategory(category);

    SB::Engine::Node* const castedNode = SB::Util::TypeHelper::getNodeFromInstance(node);

    if (ImGui::BeginMenu(CreateParentOrChildMenuName(creationType).data()))
    {
        for (const rttr::type type : categoryTypes)
        {
            const rttr::string_view typeIndexName =
                SB::Util::TypeHelper::getDisplayNameFromType(type).data();

            if (ImGui::MenuItem(typeIndexName.data()))
            {
                SB::Core::database_object* const newObject =
                    SB::Engine::Factory::createDatabaseObjectFromType(type);
                newObject->set_database_name(
                    fmt::format("New {} Node", typeIndexName.data()));
                newObject->onLoaded();

                assert(newObject);

                SetupRenameNode(newObject);

                SB::Engine::Node* newNode =
                    SB::Reflection::cast<SB::Engine::Node*, SB::Core::object*>(
                        newObject);

                if (newNode)
                {
                    switch (creationType)
                    {
                        case NodeCreationType::NewParent:
                        {
                            if (SB::Engine::NodeBase* baseParent =
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

std::string_view ProjectNodesWidget::CreateParentOrChildMenuName(
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

void ProjectNodesWidget::SetupRenameNode(SB::Core::database_object* object)
{
    m_renameID                = object->get_database_id();
    std::string_view nodeName = object->get_database_name();
    nodeName.copy(m_renameString, 255);
    m_renameString[nodeName.length()] = '\0';
}