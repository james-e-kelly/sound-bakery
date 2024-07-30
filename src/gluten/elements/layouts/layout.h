#pragma once

#include "gluten/elements/element.h"

namespace gluten
{
    class layout : public element
    {
    public:
        /**
         * @brief Which direction/technique to layout the children
         */
        enum class layout_type
        {
            horizontal,
            vertical
        };

        void add_layout_child(element* child);
        void remove_layout_child(element* child);

    private:
        std::unordered_set<element*> m_children;
    };
}