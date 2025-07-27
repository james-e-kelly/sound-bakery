#pragma once

#include <atomic>
#include <boost/assert.hpp>

namespace sbk::util
{
    template <class owner_class>
    class leaked_object_detector
    {
    public:
        leaked_object_detector() noexcept { ++(get_counter().numObjects); }
        leaked_object_detector(const leaked_object_detector&) noexcept { ++(get_counter().numObjects); }

        leaked_object_detector& operator=(const leaked_object_detector&) noexcept = default;

        ~leaked_object_detector()
        {
            if (--(get_counter().numObjects) < 0)
            {
                /**
                 * Deleted a dangling pointer!
                 */
                BOOST_ASSERT_MSG(false, "Dangling Pointer!");
            }
        }

    private:
        class leak_counter
        {
        public:
            leak_counter() = default;

            ~leak_counter()
            {
                if (numObjects.load() > 0)
                {
                    /**
                     * Leak Detected!!!
                     */
                    BOOST_ASSERT_MSG(false, "Leak Detected!");
                }
            }

            std::atomic<int> numObjects;
        };

        static leak_counter& get_counter() noexcept
        {
            static leak_counter counter;
            return counter;
        }
    };

}  // namespace sbk::util
#define LEAK_DETECTOR(owner_class)                               \
private:                                                         \
    friend class sbk::util::leaked_object_detector<owner_class>; \
    sbk::util::leaked_object_detector<owner_class> leakDetector;