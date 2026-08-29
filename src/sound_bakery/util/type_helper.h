#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/memory/memory.h"

namespace sbk::engine
{
    class node;
}  // namespace sbk::engine

namespace sbk::util
{
    struct SB_CLASS type_comparator
    {
        auto operator()(const rttr::type lhs, const rttr::type rhs) const -> bool;
    };

    class SB_CLASS type_helper final
    {
    public:
        [[nodiscard]] static auto get_category_from_type(rttr::type type) -> sbk::memory::object_category;

        [[nodiscard]] static auto get_types_from_category(sbk::memory::object_category category) -> std::set<rttr::type, type_comparator>;

        [[nodiscard]] static auto get_display_name_from_type(rttr::type type) -> rttr::string_view;

        [[nodiscard]] static auto get_folder_name_for_object_type(rttr::type type) -> std::string;

        [[nodiscard]] static auto get_file_extension_of_object_category(sbk::memory::object_category category) -> std::string_view;

        [[nodiscard]] static auto get_payload_from_type(rttr::type type) -> std::string_view;

        [[nodiscard]] static auto is_type_playable(const rttr::type& type) -> bool;

        [[nodiscard]] static auto get_object_category_enum() -> rttr::enumeration;

        [[nodiscard]] static auto get_object_category_name(const sbk::memory::object_category& objectCategory) -> rttr::string_view;

        [[nodiscard]] static auto get_object_from_instance(const rttr::instance& instance) -> sbk::core::object*;

        [[nodiscard]] static auto get_database_object_from_instance(const rttr::instance& instance) -> sbk::core::database_object*;

        [[nodiscard]] static auto get_node_from_instance(const rttr::instance& instance) -> sbk::engine::node*;

        [[nodiscard]] static auto get_node_base_from_instance(const rttr::instance& instance) -> sbk::engine::node*;
    };
}  // namespace sbk::util