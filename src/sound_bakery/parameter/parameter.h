#pragma once

#include "sound_bakery/core/core_include.h"

namespace SB::Engine
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
    class Parameter : public SB::Core::DatabaseObject
    {
        static_assert(std::is_arithmetic<ParameterType>::value);

    public:
        /**
         * @brief Defines the underlying property type used to store and broadcast values.
        */
        using ParameterProperty = SB::Core::Property<ParameterType>;

        /**
         * @brief Defines the type used for passing local versions/variations of this parameter.
         * 
         * Uses a @ref SB_ID for the parameter ID instead of DatabasePtr<T> to help when used in child classes.
         */
        using LocalParameter = std::pair<SB_ID, typename ParameterProperty>;

        /**
         * @brief Defines an ID to a parameter and a value for that parameter.
         * 
         * Used when setting the value on a parameter.
        */
        using LocalParameterValuePair = std::pair<SB_ID, ParameterType>;

        Parameter() = default;

        /**
         * @brief Creates a Parameter with min and max values set.
         * @param min value of the parameter.
         * @param max value of the parameter.
        */
        Parameter(ParameterType min, ParameterType max)
            : SB::Core::DatabaseObject(), m_property(min, min, max), m_defaultValue(min) {} 

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
        [[nodiscard]] typename ParameterProperty::PropertyChangedDelegate& getDelegate() { return m_property.getDelegate(); }

        /**
         * @brief Copies this Parameter to a runtime version, suitable for
         * handling unique variations per game object etc.
         * @return The runtime version of this Parameter.
         */
        [[nodiscard]] LocalParameter createLocalParameterFromThis() const { return LocalParameter(getDatabaseID(), m_property); }

    protected:
        ParameterType m_defaultValue;
        SB::Core::Property<ParameterType> m_property;

        RTTR_ENABLE(DatabaseObject)
    };

    class FloatParameter : public Parameter<float>
    {
        RTTR_ENABLE(Parameter)
    };

    class IntParameter : public Parameter<int>
    {
        RTTR_ENABLE(Parameter)
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
    class NamedParameterValue : public SB::Core::DatabaseObject
    {
    public:
        SB::Core::DatabasePtr<NamedParameter> parentParameter;

        RTTR_ENABLE(DatabaseObject)
    };

    /**
     * @brief Holds discrete named integer values.
     */
    class NamedParameter : public Parameter<SB_ID>
    {
    public:
        /**
         * @brief Creates a NamedParameter with min and max values.
         * 
         * @warning Weird parentheses wrapping is required here for Windows min and max macro workaround.
         * See https://stackoverflow.com/questions/40492414/why-does-stdnumeric-limitslong-longmax-fail
        */
        NamedParameter() : m_values(), Parameter((std::numeric_limits<SB_ID>::min)(), (std::numeric_limits<SB_ID>::max)()) {}

        /**
         * @brief Adds a new value to the parameter.
         * @param name Name of the parameter value
         * @return The newly created parameter value that's in the database
         */
        SB::Core::DatabasePtr<NamedParameterValue> addNewValue(const std::string_view name)
        {
            SB::Core::DatabasePtr<NamedParameterValue> result;

            if (name.empty() == false)
            {
                if (NamedParameterValue* const parameterValue = newDatabaseObject<NamedParameterValue>())
                {
                    parameterValue->setDatabaseName(name);
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
        std::unordered_set<SB::Core::DatabasePtr<NamedParameterValue>> getValues()
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
        void setSelectedValue(SB::Core::DatabasePtr<NamedParameterValue> value)
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
        SB::Core::DatabasePtr<NamedParameterValue> getSelectedValue() const
        {
            SB::Core::DatabasePtr<NamedParameterValue> selected(get());
            selected.lookup();
            return selected;
        }

    private:
        std::unordered_set<SB::Core::DatabasePtr<NamedParameterValue>> m_values;

        RTTR_ENABLE(DatabaseObject)
        RTTR_REGISTRATION_FRIEND
    };

    using GlobalFloatParameter = SB::Core::DatabasePtr<FloatParameter>;
    using GlobalIntParameter = SB::Core::DatabasePtr<NamedParameter>;

    /**
     * @brief Holds a list of parameters.
     * 
     * Setting values on raw/database parameters assumes the global scope.
    */
    struct GlobalParameterList
    {
        std::unordered_set<GlobalFloatParameter> floatParameters;
        std::unordered_set<GlobalIntParameter> intParameters;
    };

    /**
     * @brief Holds a list of parameters and their local value.
    */
    struct LocalParameterList
    {
        std::unordered_map<GlobalFloatParameter, FloatParameter::ParameterProperty> floatParameters;
        std::unordered_map<GlobalIntParameter, NamedParameter::ParameterProperty> intParameters;
    };
}  // namespace SB::Engine