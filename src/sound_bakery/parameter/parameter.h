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
	 * Used for changing effect parameters, choosing sounds and anything
	*/
	template<typename T>
	class Parameter : public SB::Core::DatabaseObject
	{
	public:
		virtual T get() const											{ return m_property.get(); }
		virtual void set(T value)										{ m_property.set(value); }
		SB::Core::Property<T>::PropertyChangedDelegate& getDelegate()	{ return m_property.getDelegate(); }

	protected:	// made protected so child classes can add it to reflection
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
	 * The object inherits from SB::Core::DatabaseObject to be universally referencable and have a display name.
	 * The object knows its parameter IntParameter.
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
	 * Int parameters are what Wwise would call Switches and States. Sound Bakery makes uses the same type for both but _how_ they're used changes.
	*/
	class IntParameter : public Parameter<SB_ID>
	{
	public:
		/**
		 * @brief Adds a new value to the 
		 * @param name 
		 * @return 
		*/
		SB::Core::DatabasePtr<IntParameterValue> addNewValue(const std::string_view name)
		{
			SB::Core::DatabasePtr<IntParameterValue> result;

			if (!name.empty())
			{
				if (IntParameterValue* parameterValue = newDatabaseObject<IntParameterValue>())
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
				set(addNewValue("None").id());
			}

			return m_values;
		}

	private:
		std::unordered_set<SB::Core::DatabasePtr<IntParameterValue>> m_values;

		RTTR_ENABLE(Parameter)
		RTTR_REGISTRATION_FRIEND
	};
}