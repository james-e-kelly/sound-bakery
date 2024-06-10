#pragma once

#include "IconsFontaudio.h"
#include "Widget.h"
#include "imgui.h"
#include "sound_bakery/pch.h"

namespace SB::Engine
{
    class Node;
}

enum class NodeCreationType
{
    New,
    NewParent,
    NewChild
};

class ProjectNodesWidget : public widget
{
public:
    ProjectNodesWidget(widget_subsystem* parentSubsystem)
        : widget(parentSubsystem), m_renameID(0)
    {
    }

    ProjectNodesWidget(widget* parentWidget)
        : widget(parentWidget), m_renameID(0)
    {
    }

public:
    void RenderPage(const std::vector<SB_OBJECT_CATEGORY>& categories);
    void RenderObjectsPage();
    void RenderEventsPage();
    void RenderSoundbankPage();

public:
    void RenderCategory(SB_OBJECT_CATEGORY category);

    void RenderSingleNode(rttr::type type, rttr::instance instance);

    bool NodeHasChildren(SB::Engine::Node* node);

    void HandleOpenNode(SB::Core::DatabaseObject* object);

    bool ObjectIsRenaming(SB::Core::DatabaseObject* object);

    void RenderRenameObject(SB::Core::DatabaseObject* const& object);

private:
    bool RenderNodeContextMenu(rttr::type type, rttr::instance instance);

    void RenderCreateParentOrChildMenu(SB_OBJECT_CATEGORY category,
                                       rttr::instance node,
                                       NodeCreationType creationType);

    std::string_view CreateParentOrChildMenuName(NodeCreationType creationType);

private:
    void SetupRenameNode(SB::Core::DatabaseObject* object);

private:
    SB_ID m_renameID         = 0;
    SB_ID m_nodeToOpen       = 0;
    char m_renameString[255] = "\0";
};
