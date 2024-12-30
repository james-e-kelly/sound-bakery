#pragma once

#include "sound_bakery/core/core_include.h"

namespace sbk::engine
{
    class named_parameter;
    class float_parameter;
    class named_parameter_value;

    /**
     * @brief Defines a database object with a changeable property.
     *
     * Used for changing effect parameters, choosing sounds and anything else.
     */
    template <typename parameter_type>
    class SB_CLASS parameter : public sbk::core::database_object
    {
        REGISTER_REFLECTION(parameter, database_object)

        static_assert(std::is_arithmetic<parameter_type>::value);

    public:
        /**
         * @brief Defines the underlying property type used to store and broadcast values.
         */
        using parameter_property = sbk::core::property<parameter_type>;

        /**
         * @brief Defines the type used for passing local versions/variations of this parameter.
         *
         * Uses a @ref SB_ID for the parameter ID instead of DatabasePtr<T> to help when used in child classes.
         */
        using local_parameter = std::pair<sbk_id, parameter_property>;

        /**
         * @brief Defines an ID to a parameter and a value for that parameter.
         *
         * Used when setting the value on a parameter. Local parameter values are not intended for storage.
         * Rather, they are intended to be passed as parameters for setting for permanent versions of the parameter.
         */
        using local_parameter_value_pair = std::pair<sbk_id, parameter_type>;

        parameter() = default;

        /**
         * @brief Creates a parameter with min and max values set.
         * @param min value of the parameter.
         * @param max value of the parameter.
         */
        parameter(parameter_type min, parameter_type max)
            : sbk::core::database_object(), m_property(min, min, max), m_defaultValue(min)
        {
        }

        /**
         * @brief Get the current value of the parameter.
         */
        [[nodiscard]] parameter_type get() const { return m_property.get(); }
        [[nodiscard]] parameter_type get_default() const { return m_defaultValue; }

        /**
         * @brief Set the value of the parameter.
         *
         * This sets the internal property and will therefore call any bound delegates.
         */
        void set(parameter_type value) { m_property.set(value); }
        void set_default(parameter_type value) { m_defaultValue = value; }

        /**
         * @brief Get the parameter delegate that fires when changing the value.
         */
        [[nodiscard]] typename parameter_property::property_changed_delegate& get_delegate()
        {
            return m_property.get_delegate();
        }

        /**
         * @brief Copies this parameter to a runtime version, suitable for
         * handling unique variations per game object etc.
         * @return The runtime version of this parameter.
         */
        [[nodiscard]] local_parameter create_local_parameter_from_this() const
        {
            return local_parameter(get_database_id(), m_property);
        }

    private:
        parameter_type m_defaultValue;
        sbk::core::property<parameter_type> m_property;
    };

    class SB_CLASS float_parameter : public parameter<float>
    {
        REGISTER_REFLECTION(float_parameter, parameter)
    };

    class SB_CLASS int_parameter : public parameter<int>
    {
        REGISTER_REFLECTION(int_parameter, parameter)
    };

    /**
     * @brief Represents a discrete value for a @ref named_parameter.
     *
     * The object inherits from SB::Core::DatabaseObject to be universally
     * referencable and have a display name. The object knows its get_parent
     * named_parameter.
     *
     * The object's ID is its parameter value.
     */
    class SB_CLASS named_parameter_value : public sbk::core::database_object
    {
    public:
        sbk::core::database_ptr<named_parameter> parentParameter;

        REGISTER_REFLECTION(named_parameter_value, database_object)
    };

    /**
     * @brief Holds discrete named integer values.
     *
     * Each named_parameter_value is a database object, therefore holding a unique ID.
     *
     * The named_parameter stores the unique id as its parameter value.
     */
    class SB_CLASS named_parameter : public parameter<sbk_id>
    {
        REGISTER_REFLECTION(named_parameter, parameter)

    public:
        /**
         * @brief Creates a named_parameter with min and max values.
         *
         * @warning Weird parentheses wrapping is required here for Windows min and max macro workaround.
         * See https://stackoverflow.com/questions/40492414/why-does-stdnumeric-limitslong-longmax-fail
         */
        named_parameter()
            : m_values(), parameter((std::numeric_limits<sbk_id>::min)(), (std::numeric_limits<sbk_id>::max)())
        {
        }

        /**
         * @brief Adds a new value to the parameter.
         * @param name Name of the parameter value
         * @return The newly created parameter value that's in the database
         */
        sbk::core::database_ptr<named_parameter_value> add_new_value(const std::string_view name)
        {
            sbk::core::database_ptr<named_parameter_value> result;

            if (!name.empty())
            {
                if (const std::shared_ptr<named_parameter_value> parameterValue =
                        create_database_object<named_parameter_value>())
                {
                    parameterValue->set_database_name(name);
                    parameterValue->parentParameter = this;

                    result = parameterValue;

                    m_values.emplace(parameterValue);
                }
            }

            return result;
        }

        /**
         * @brief Gets all named values in this parameter.
         *
         * If none exists, ensures at least the "None" value exists.
         */
        std::unordered_set<sbk::core::database_ptr<named_parameter_value>> get_values()
        {
            if (m_values.empty())
            {
                set_selected_value(add_new_value("None"));
            }

            return m_values;
        }

        /**
         * @brief Sets the parameter value with a DatabasePtr, ensuring the value exists in this parameter.
         *
         * Internally sets the parameter with the DatabasePtr's ID.
         * @param value
         */
        void set_selected_value(sbk::core::database_ptr<named_parameter_value> value)
        {
            if (m_values.contains(value))
            {
                set(value.id());
            }
            else
            {
                BOOST_ASSERT(false);
            }
        }

        /**
         * @brief Get the selected value as a DatabasePtr.
         *
         * Mainly used in reflection and for displaying in the editor.
         * @return
         */
        [[nodiscard]] sbk::core::database_ptr<named_parameter_value> get_selected_value() const
        {
            sbk::core::database_ptr<named_parameter_value> selected(get());
            selected.lookup();
            return selected;
        }

    private:
        std::unordered_set<sbk::core::database_ptr<named_parameter_value>> m_values;
    };

    using global_float_parameter = sbk::core::database_ptr<float_parameter>;
    using global_int_parameter   = sbk::core::database_ptr<named_parameter>;

    /**
     * @brief Holds a list of parameters.
     *
     * Setting values on raw/database parameters assumes the global scope.
     */
    struct SB_CLASS global_parameter_list
    {
        std::unordered_set<global_float_parameter> floatParameters;
        std::unordered_set<global_int_parameter> intParameters;
    };

    /**
     * @brief Holds a list of parameters and their local value.
     */
    struct SB_CLASS local_parameter_list
    {
        std::unordered_map<global_float_parameter, float_parameter::parameter_property> floatParameters;
        std::unordered_map<global_int_parameter, named_parameter::parameter_property> intParameters;
    };
}  // namespace sbk::engine