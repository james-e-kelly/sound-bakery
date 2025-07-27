#include "sound_bakery/pch.h"

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
                BOOST_ASSERT(false);
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
                    BOOST_ASSERT(false);
                }
            }

            std::atomic<int> numObjects;
        };

        static const char* get_leaked_object_class_name() { return owner_class::get_type().get_name().c_str(); }

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