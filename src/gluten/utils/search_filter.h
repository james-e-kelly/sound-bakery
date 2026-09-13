#pragma once

#include "gluten/pch.h"

namespace gluten
{
    class search_filter
    {
    public:
        search_filter() = default;
        search_filter(const search_filter&) = default;
        search_filter(search_filter&&)      = default;
        ~search_filter()                    = default;

        [[nodiscard]] auto get_query() const -> const std::string& { return m_query; }
        [[nodiscard]] auto is_active() const -> bool { return !m_groups.empty(); }

        [[nodiscard]] auto matches(std::string_view text) const -> bool
        {
            if (!is_active())
            {
                return true;
            }

            std::string lowered_text(text);
            convert_string_to_lower(lowered_text);

            for (const auto& group : m_groups)
            {
                if (group_matches(group, lowered_text))
                {
                    return true;
                }
            }

            return false;
        }

        void set_query(std::string_view query)
        {
            if (query == m_query)
            {
                return;
            }

            m_query = query;
            m_groups.clear();
            parse(m_query);
        }

        void clear()
        {
            m_query.clear();
            m_groups.clear();
        }

    private:
        struct token
        {
            std::string term;
            bool negated = false;
        };

        using token_group = std::vector<token>;

        std::string m_query;
        std::vector<token_group> m_groups;

        void parse(std::string_view query)
        {
            size_t start = 0;

            while (!index_reached_end_of_string(query, start))
            {
                const size_t comma_position = find_unquoted_comma(query, start);
                std::string_view segment    = query.substr(start, comma_position - start);

                token_group group;
                parse_segment(segment, group);

                if (!group.empty())
                {
                    m_groups.push_back(std::move(group));
                }

                start = index_reached_end_of_string(query, comma_position) ? query.size() : comma_position + 1;
            }
        }

        static void convert_string_to_lower(std::string& s)
        {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        }

        static auto string_contains(const std::string& text, const std::string& term) -> bool
        {
            return text.find(term) != std::string::npos;
        }

        static auto index_reached_end_of_string(std::string_view text, size_t index) -> bool
        {
            return index >= text.size();
        }

        static auto group_matches(const token_group& group, const std::string& lowered_text) -> bool
        {
            for (const auto& token : group)
            {
                const bool found = string_contains(lowered_text, token.term);

                if (token.negated && found)
                {
                    return false;
                }

                if (!token.negated && !found)
                {
                    return false;
                }
            }

            return true;
        }

        static auto find_unquoted_comma(std::string_view query, size_t from) -> size_t
        {
            bool inside_quotes = false;

            for (size_t index = from; !index_reached_end_of_string(query, index); ++index)
            {
                if (query[index] == '"')
                {
                    inside_quotes = !inside_quotes;
                }
                else if (query[index] == ',' && !inside_quotes)
                {
                    return index;
                }
            }

            return query.size();
        }

        static void iterate_past_whitespace(std::string_view text, size_t& index)
        {
            while (!index_reached_end_of_string(text, index) && text[index] == ' ')
            {
                ++index;
            }
        }

        static void iterate_till_whitespace(std::string_view text, size_t& index)
        {
            while (!index_reached_end_of_string(text, index) && text[index] != ' ')
            {
                ++index;
            }
        }

        static auto parse_quoted_term(std::string_view segment, size_t& index) -> std::string
        {
            ++index;
            const size_t closing_quote = segment.find('"', index);
            const bool has_closing_quote = closing_quote != std::string_view::npos;

            if (has_closing_quote)
            {
                std::string term(segment.substr(index, closing_quote - index));
                index = closing_quote + 1;
                return term;
            }

            std::string term(segment.substr(index));
            index = segment.size();
            return term;
        }

        static auto parse_bare_term(std::string_view segment, size_t& index) -> std::string
        {
            const size_t start = index;
            iterate_till_whitespace(segment, index);
            return std::string(segment.substr(start, index - start));
        }

        static void parse_segment(std::string_view segment, token_group& group)
        {
            size_t index = 0;

            while (!index_reached_end_of_string(segment, index))
            {
                iterate_past_whitespace(segment, index);

                if (index_reached_end_of_string(segment, index))
                {
                    return;
                }

                const bool is_negated = segment[index] == '-' && !index_reached_end_of_string(segment, index + 1) && segment[index + 1] != ' ';

                if (is_negated)
                {
                    ++index;
                }

                const bool is_lone_dash = !is_negated && !index_reached_end_of_string(segment, index) && segment[index] == '-';

                if (is_lone_dash)
                {
                    ++index;
                    continue;
                }

                const bool is_quoted = !index_reached_end_of_string(segment, index) && segment[index] == '"';
                std::string term     = is_quoted ? parse_quoted_term(segment, index) : parse_bare_term(segment, index);

                if (!term.empty())
                {
                    convert_string_to_lower(term);
                    group.push_back({std::move(term), is_negated});
                }
            }
        }
    };
}  // namespace gluten