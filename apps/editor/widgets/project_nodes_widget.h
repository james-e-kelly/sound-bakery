#pragma once

#include "IconsFontaudio.h"
#include "gluten/widgets/widget.h"
#include "imgui.h"
#include "sound_bakery/pch.h"

namespace sbk::engine
{
    class node;
}

enum class node_creation_type
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
    void render_page(const std::vector<SB_OBJECT_CATEGORY>& categories);
    void render_objects_page();
    void render_events_page();
    void render_soundbank_page();

public:
    void render_category(SB_OBJECT_CATEGORY category);
    void render_single_node(rttr::type type, rttr::instance instance);
    void render_rename_object(sbk::core::database_object* const& object);

    void handle_open_node(sbk::core::database_object* object);

    bool node_has_children(sbk::engine::node* node);
    bool object_is_renaming(sbk::core::database_object* object);

private:
    bool render_node_context_menu(rttr::type type, rttr::instance instance);

    void render_create_parent_or_child_menu(SB_OBJECT_CATEGORY category,
                                       rttr::instance node,
                                       node_creation_type creationType);

    std::string_view create_parent_or_child_menu_name(node_creation_type creationType);

private:
    void setup_rename_node(sbk::core::database_object* object);

private:
    sbk_id m_renameID         = 0;
    sbk_id m_nodeToOpen       = 0;
    char m_renameString[255] = "\0";
};
