#pragma once

#include "sound_bakery/core/core_include.h"

namespace sbk::engine
{
    class NamedParameter;
    class FloatParameter;
    class NamedParameterValue;

    /**
     * @brief Defines a database object with a changeable property.
     *
     * Used for changing effect parameters, choosing sounds and anything else.
     */
    template <typename ParameterType>
    class SB_CLASS Parameter : public sbk::core::database_object
    {
        static_assert(std::is_arithmetic<ParameterType>::value);

    public:
        /**
         * @brief Defines the underlying property type used to store and broadcast values.
         */
        using ParameterProperty = sbk::core::Property<ParameterType>;

        /**
         * @brief Defines the type used for passing local versions/variations of this parameter.
         *
         * Uses a @ref SB_ID for the parameter ID instead of DatabasePtr<T> to help when used in child classes.
         */
        using LocalParameter = std::pair<sbk_id, ParameterProperty>;

        /**
         * @brief Defines an ID to a parameter and a value for that parameter.
         *
         * Used when setting the value on a parameter.
         */
        using LocalParameterValuePair = std::pair<sbk_id, ParameterType>;

        Parameter() = default;

        /**
         * @brief Creates a Parameter with min and max values set.
         * @param min value of the parameter.
         * @param max value of the parameter.
         */
        Parameter(ParameterType min, ParameterType max)
            : sbk::core::database_object(), m_property(min, min, max), m_defaultValue(min)
        {
        }

        /**
         * @brief Get the current value of the parameter.
         */
        [[nodiscard]] ParameterType get() const { return m_property.get(); }

        [[nodiscard]] ParameterType getDefault() const { return m_defaultValue; }

        /**
         * @brief Set the value of the parameter.
         *
         * This sets the internal property and will therefore call any bound delegates.
         */
        void set(ParameterType value) { m_property.set(value); }

        void setDefault(ParameterType value) { m_defaultValue = value; }

        /**
         * @brief Get the parameter delegate that fires when changing the value.
         */
        [[nodiscard]] typename ParameterProperty::PropertyChangedDelegate& getDelegate()
        {
            return m_property.getDelegate();
        }

        /**
         * @brief Copies this Parameter to a runtime version, suitable for
         * handling unique variations per game object etc.
         * @return The runtime version of this Parameter.
         */
        [[nodiscard]] LocalParameter createLocalParameterFromThis() const
        {
            return LocalParameter(get_database_id(), m_property);
        }

    protected:
        ParameterType m_defaultValue;
        sbk::core::Property<ParameterType> m_property;

        REGISTER_REFLECTION(Parameter, database_object)
    };

    class SB_CLASS FloatParameter : public Parameter<float>
    {
        REGISTER_REFLECTION(FloatParameter, Parameter)
    };

    class SB_CLASS IntParameter : public Parameter<int>
    {
        REGISTER_REFLECTION(IntParameter, Parameter)
    };

    /**
     * @brief Represents a discrete value for a @ref NamedParameter.
     *
     * The object inherits from SB::Core::DatabaseObject to be universally
     * referencable and have a display name. The object knows its parent
     * NamedParameter.
     *
     * The object's ID is its parameter value.
     */
    class SB_CLASS NamedParameterValue : public sbk::core::database_object
    {
    public:
        sbk::core::DatabasePtr<NamedParameter> parentParameter;

        REGISTER_REFLECTION(NamedParameterValue, database_object)
    };

    /**
     * @brief Holds discrete named integer values.
     */
    class SB_CLASS NamedParameter : public Parameter<sbk_id>
    {
        REGISTER_REFLECTION(NamedParameter, Parameter)

    public:
        /**
         * @brief Creates a NamedParameter with min and max values.
         *
         * @warning Weird parentheses wrapping is required here for Windows min and max macro workaround.
         * See https://stackoverflow.com/questions/40492414/why-does-stdnumeric-limitslong-longmax-fail
         */
        NamedParameter()
            : m_values(), Parameter((std::numeric_limits<sbk_id>::min)(), (std::numeric_limits<sbk_id>::max)())
        {
        }

        /**
         * @brief Adds a new value to the parameter.
         * @param name Name of the parameter value
         * @return The newly created parameter value that's in the database
         */
        sbk::core::DatabasePtr<NamedParameterValue> addNewValue(const std::string_view name)
        {
            sbk::core::DatabasePtr<NamedParameterValue> result;

            if (name.empty() == false)
            {
                if (std::shared_ptr<NamedParameterValue> parameterValue =
                        sbk::new_database_object<NamedParameterValue>())
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
        std::unordered_set<sbk::core::DatabasePtr<NamedParameterValue>> getValues()
        {
            if (m_values.empty())
            {
                setSelectedValue(addNewValue("None"));
            }

            return m_values;
        }

        /**
         * @brief Sets the parameter value with a DatabasePtr, ensuring the value exists in this parameter.
         *
         * Internally sets the parameter with the DatabasePtr's ID.
         * @param value
         */
        void setSelectedValue(sbk::core::DatabasePtr<NamedParameterValue> value)
        {
            if (m_values.contains(value))
            {
                set(value.id());
            }
            else
            {
                assert(false);
            }
        }

        /**
         * @brief Get the selected value as a DatabasePtr.
         *
         * Mainly used in reflection and for displaying in the editor.
         * @return
         */
        sbk::core::DatabasePtr<NamedParameterValue> getSelectedValue() const
        {
            sbk::core::DatabasePtr<NamedParameterValue> selected(get());
            selected.lookup();
            return selected;
        }

    private:
        std::unordered_set<sbk::core::DatabasePtr<NamedParameterValue>> m_values;
    };

    using GlobalFloatParameter = sbk::core::DatabasePtr<FloatParameter>;
    using GlobalIntParameter   = sbk::core::DatabasePtr<NamedParameter>;

    /**
     * @brief Holds a list of parameters.
     *
     * Setting values on raw/database parameters assumes the global scope.
     */
    struct SB_CLASS GlobalParameterList
    {
        std::unordered_set<GlobalFloatParameter> floatParameters;
        std::unordered_set<GlobalIntParameter> intParameters;
    };

    /**
     * @brief Holds a list of parameters and their local value.
     */
    struct SB_CLASS LocalParameterList
    {
        std::unordered_map<GlobalFloatParameter, FloatParameter::ParameterProperty> floatParameters;
        std::unordered_map<GlobalIntParameter, NamedParameter::ParameterProperty> intParameters;
    };
}  // namespace sbk::engine