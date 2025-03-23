#include "sound_bakery/pch.h"

namespace sbk::core
{
    /**
     * @brief Specialised string that can check for illegal characters.
     */
    class SB_CLASS string
    {
    public:
        string() = delete;
        string(std::string_view data, std::string_view illegalCharacters)
            : m_data(data),
            m_illegalCharacters(illegalCharacters)
        {
            BOOST_ASSERT_MSG(!m_data.empty(), "Strings cannot be set to empty");
        }

        auto test_set(std::string_view data) -> bool
        {
            return !data.empty() && data.find_first_of(m_illegalCharacters) == std::string::npos;
        }

        auto set(std::string_view data, bool skipTest = false) -> bool
        {
            if (skipTest || test_set(data))
            {
                m_data = data;
                return true;
            }
            return false;
        }

        auto get() const -> std::string_view
        {
            return m_data;
        }

        operator std::string_view() const
        {
            return get();
        }

        auto operator=(std::string_view data) -> bool 
        { 
            return set(data);
        }

    private:
        std::string m_data;
        std::string m_illegalCharacters;
    };
}