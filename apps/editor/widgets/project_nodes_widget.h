#pragma once

#include "IconsFontaudio.h"
#include "gluten/widgets/widget.h"
#include "imgui.h"
#include "sound_bakery/pch.h"

namespace sbk::engine
{
    class node;
}

enum class NodeCreationType
{
    New,
    NewParent,
    NewChild
};

class project_nodes_widget : public gluten::widget
{
public:
    project_nodes_widget(gluten::widget_subsystem* parentSubsystem)
        : widget(parentSubsystem), m_renameID(0)
    {
    }

    project_nodes_widget(widget* parentWidget)
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

    bool NodeHasChildren(sbk::engine::node* node);

    void HandleOpenNode(sbk::core::database_object* object);

    bool ObjectIsRenaming(sbk::core::database_object* object);

    void RenderRenameObject(sbk::core::database_object* const& object);

private:
    bool RenderNodeContextMenu(rttr::type type, rttr::instance instance);

    void RenderCreateParentOrChildMenu(SB_OBJECT_CATEGORY category,
                                       rttr::instance node,
                                       NodeCreationType creationType);

    std::string_view CreateParentOrChildMenuName(NodeCreationType creationType);

private:
    void SetupRenameNode(sbk::core::database_object* object);

private:
    sbk_id m_renameID         = 0;
    sbk_id m_nodeToOpen       = 0;
    char m_renameString[255] = "\0";
};
