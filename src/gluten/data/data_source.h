#pragma once

#include "gluten/pch.h"

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <rttr/type>

namespace gluten
{
    std::filesystem::path get_config_file(const rttr::type& type);

    constexpr const char* dataSourceSerializeName = "DataSource";

    template<typename T>
    struct data_source_deleter
    {
        auto operator()(T* data) -> void
        {
            if (data)
            {
                const std::filesystem::path configFile = get_config_file(rttr::type::get<T>());
                std::filesystem::create_directories(configFile.parent_path());

                std::ofstream outputStream(configFile, std::ios_base::out);
                boost::archive::xml_oarchive archive(outputStream);

                // Need a better way of checking if a type can be serialized
                try
                {
                    archive & boost::serialization::make_nvp(dataSourceSerializeName, *data);
                }
                catch (...) {}
                
                delete data;
            }
        }
    };

    /**
     * @brief Data sources are global pieces of data that can be saved to disk upon destruction and their values reloaded.
     */
	template <typename T>
	class data_source
	{
    public:
        data_source() 
        { 
            m_localData = static_get_data();
        }

        auto get_data() const -> T*
        {
            assert(m_localData);
            assert(m_localData.get());
            return m_localData.get();
        }

        auto operator->() const -> T*
        {
            return get_data();
        }

        auto operator*() const -> T
        {
            return *get_data();
        }

        static auto static_get_data() -> std::shared_ptr<T>
        {
            std::shared_ptr<T> result;

            if (m_weakData.expired())
            {
                result = std::shared_ptr<T>(new T, data_source_deleter<T>());
                m_weakData = result;

                const std::filesystem::path configFile = get_config_file(rttr::type::get<T>());
                if (std::filesystem::exists(configFile))
                {
                    std::ifstream inputStream(configFile, std::ios_base::in);
                    boost::archive::xml_iarchive archive(inputStream);

                    // Need a better way of checking if a type can be serialized
                    try
                    {
                        archive & boost::serialization::make_nvp(dataSourceSerializeName, *result.get());
                    }
                    catch (...) {}
                }
            }
            else
            {
                result = m_weakData.lock();
            }

            return result;
        }

    private:
        static inline std::weak_ptr<T> m_weakData;
        std::shared_ptr<T> m_localData;
	};
}