#pragma once

#include "sound_bakery/core/core_include.h"

namespace SB::Engine
{
    class IntParameter;
    class FloatParameter;
    class IntParameterValue;

    /**
     * @brief Defines a database object with a changeable property.
     *
     * Used for changing effect parameters, choosing sounds and anything else.
     */
    template <typename T>
    class Parameter : public SB::Core::DatabaseObject
    {
    public:
        /**
         * @brief Defines the type used for passing runtime versions/variations
         * of this parameter.
         */
        using RuntimeParameter = std::pair<SB_ID, SB::Core::Property<T>>;

        using RuntimeProperty = SB::Core::Property<T>;

        Parameter() = default;

        Parameter(T min, T max)
            : m_property(T(), min, max), SB::Core::DatabaseObject()
        {
        }

    public:
        T get() const { return m_property.get(); }
        void set(T value) { m_property.set(value); }
        SB::Core::Property<T>::PropertyChangedDelegate& getDelegate()
        {
            return m_property.getDelegate();
        }

        /**
         * @brief Copies this Parameter to a runtime version, suitable for
         * handling unique variations per game object etc.
         * @return The runtime version of this Parameter.
         */
        [[nodiscard]] RuntimeParameter createRuntimeParameterFromThis() const
        {
            return RuntimeParameter(getDatabaseID(), m_property);
        }

    protected:  // made protected so child classes can add it to reflection
        SB::Core::Property<T> m_property;

        RTTR_ENABLE(DatabaseObject)
    };

    class FloatParameter : public Parameter<float>
    {
    public:
        float m_min;
        float m_max;
        float m_default;

        RTTR_ENABLE(Parameter)
    };

    /**
     * @brief Represents a discrete value for an IntParameter.
     *
     * The object inherits from SB::Core::DatabaseObject to be universally
     * referencable and have a display name. The object knows its parent
     * IntParameter.
     *
     * The object's ID is its parameter value.
     */
    class IntParameterValue : public SB::Core::DatabaseObject
    {
    public:
        SB::Core::DatabasePtr<IntParameter> m_parentParameter;

        RTTR_ENABLE(DatabaseObject)
    };

    /**
     * @brief Holds discrete named integer values.
     *
     * Int parameters are what Wwise would call Switches and States. Sound
     * Bakery uses the same type for both but _how_ they're used changes.
     */
    class IntParameter : public Parameter<SB_ID>
    {
    public:
        IntParameter()
            : m_values(), Parameter(SB_ID(), 18'446'744'073'709'551'615llu)
        {
        }

        /**
         * @brief Adds a new value to the parameter.
         * @param name Name of the parameter value
         * @return The newly created parameter value that's in the database
         */
        SB::Core::DatabasePtr<IntParameterValue> addNewValue(
            const std::string_view name)
        {
            SB::Core::DatabasePtr<IntParameterValue> result;

            if (!name.empty())
            {
                if (IntParameterValue* parameterValue =
                        newDatabaseObject<IntParameterValue>())
                {
                    parameterValue->setDatabaseName(name);
                    parameterValue->m_parentParameter = this;

                    result = parameterValue;

                    m_values.emplace(parameterValue);
                }
            }

            return result;
        }

        std::unordered_set<SB::Core::DatabasePtr<IntParameterValue>> getValues()
        {
            if (m_values.empty())
            {
                setSelectedValue(addNewValue("None"));
            }

            return m_values;
        }

        void setSelectedValue(SB::Core::DatabasePtr<IntParameterValue> value)
        {
            if (m_values.contains(value))
            {
                set(value.id());
            }
        }

        SB::Core::DatabasePtr<IntParameterValue> getSelectedValue() const
        {
            return SB::Core::DatabasePtr<IntParameterValue>(get());
        }

    private:
        std::unordered_set<SB::Core::DatabasePtr<IntParameterValue>> m_values;

        RTTR_ENABLE(DatabaseObject)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine