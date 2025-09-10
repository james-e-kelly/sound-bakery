#pragma once

#include "gluten/pch.h"

#include "concurrencpp/concurrencpp.h"
#include "tl/expected.hpp"

namespace gluten
{
    template<typename key_type>
    struct key_cache_key
    {
        key_cache_key() = default;
        ~key_cache_key() = default;

        key_cache_key(const key_type& key) : m_key(key) {}

        key_type m_key;

        bool operator==(const key_cache_key& rhs) const
        {
            return m_key == rhs.m_key;
        }
    };

    template <typename key_type>
    struct key_cache_key_hasher
    {
        std::size_t operator()(const key_cache_key<key_type>& defaultCacheKey) const
        {
            return std::hash<key_type>()(defaultCacheKey.m_key);
        }
    };

    template<typename token_type>
    struct token_cache_key
    {
        token_cache_key() = default;
        ~token_cache_key() = default;

        token_cache_key(const token_type& token) : m_token(token) {}

        token_type m_token;

        bool operator==(const token_cache_key& rhs) const
        {
            return m_token == rhs.m_token;
        }
    };

    template<typename token_type>
    struct token_cache_key_hasher
    {
        std::size_t operator()(const token_cache_key<token_type>& tokenCacheKey) const
        {
            return std::hash<token_type>()(tokenCacheKey.m_token);
        }
    };

    template <typename key_type, typename token_type>
    struct key_and_token_cache_key : public key_cache_key<key_type>, public token_cache_key<token_type>
    {
        key_and_token_cache_key()  = default;
        ~key_and_token_cache_key() = default;

        key_and_token_cache_key(const key_type& key, const token_type& token)
            : key_cache_key<key_type>(key), token_cache_key<token_type>(token)
        {
        }

        bool operator==(const key_and_token_cache_key& rhs) const
        {
            return key_cache_key<key_type>::operator==(rhs) && token_cache_key<token_type>::operator==(rhs);
        }
    };

    template <typename key_type, typename token_type>
    struct key_and_token_cache_key_hasher
    {
        std::size_t operator()(const key_and_token_cache_key<key_type, token_type>& tokenCacheKey) const
        {
            return std::hash<key_type>()(tokenCacheKey.m_key) ^ std::hash<token_type>()(tokenCacheKey.m_token);
        }
    };

    enum class cache_state
    {
        no_data,        //< Either initial state or has not been supplied any data after requesting it
        loading,        //< Has an async function that is loading the data. The data will be available later
        has_data,       //< Has data and is ready to use
        expired         //< Any current data is out of date and needs a new load
    };

    template <typename data_type, typename key_type, typename key_hasher>
	class data_cache
	{
    public:
        data_cache() = default;
        ~data_cache() = default;

        data_cache(const std::chrono::seconds& expirySeconds)
            : m_expirySeconds(expirySeconds) {}

        /**
         * @brief Stores the cached data and when the cache was created.
         *
         * Allows for expiration of the cached data.
         */
        struct cached_data
        {
            cached_data()
                : m_createdAt(std::chrono::steady_clock::now()) 
            {
            }

            cached_data(data_type data) : m_cache(std::move(data)), m_createdAt(std::chrono::steady_clock::now())
            {
            }

            data_type m_cache;
            cache_state m_state = cache_state::no_data;
            std::chrono::steady_clock::time_point m_createdAt;

            auto has_data() const -> bool
            {
                // Expired and has_data should both look the same to the end user
                // A loading spinner looks slow but data instatly changing looks fast
                return m_state != cache_state::no_data && m_state != cache_state::loading;
            }

            bool operator==(const cached_data& rhs)
            {
                return false;
            }
        };

        using async_cache_result    = concurrencpp::result<data_type>;
        using cache_result          = const cached_data&;

        /**
         * @brief Query the cache state. The user should should begin filling new data if the state is == no_data || expired
         */
        [[nodiscard]] auto get_cache_state(const key_type& key) const -> cache_state
        {
            if (m_asyncCache.contains(key))
            {
                // Mark the cache as loading only on the first time
                // If the cache becomes expired, we make it look like the data
                // is still there and just load in the background
                if (m_cache[key].m_state == cache_state::no_data || m_cache[key].m_state == cache_state::loading)
                {
                    m_cache[key].m_state = cache_state::loading;
                }
            }
            else if (m_cache.contains(key))
            {
                const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
                const std::chrono::seconds expirySeconds(m_expirySeconds);

                if ((now - m_cache[key].m_createdAt) > expirySeconds)
                {
                    m_cache[key].m_state = cache_state::expired;
                }
            }
            return m_cache[key].m_state;
        }

        /**
         * @brief Returns true if the cache is empty or expired and the user should fill it.
         */
        [[nodiscard]] auto get_cache_needs_filling(const key_type& key) const -> bool
        {
            const cache_state state = get_cache_state(key);
            return (state == cache_state::no_data || state == cache_state::expired) && !m_asyncCache.contains(key);
        }

        /**
         * @brief Give this cache an async result that can be used later to get the latest version of the data.
         */
        auto set_async_fill_cache(const key_type& key, async_cache_result asyncResult) -> void
        {
            assert(get_cache_state(key) != cache_state::loading);
            assert(static_cast<bool>(m_asyncCache[key]) == false);
            m_asyncCache[key] = std::move(asyncResult);
        }

        /**
         * @brief Set the cached data manually.
         * @warn This is not thread safe! Ensure this method is called on the reader thread
         */
        auto set_cache_data(const key_type& key, const data_type& data) -> void
        {
            m_cache[key].m_cache = data;
        }

        /**
         * @brief Get the cached data.
         * 
         * This returns a const reference to the data to avoid copying.
         * Read the cache state to know if the data is valid, loading, etc.
         * 
         * This function checks if the async loading is ready and, if so, fills the cache.
         * Therefore, this is not a const function as the data can change.
         */
        [[nodiscard]] auto get_cached_data(const key_type& key) -> cache_result
        {
            if (m_asyncCache.contains(key))
            {
                async_cache_result& asyncResult = m_asyncCache[key];
                if (asyncResult && asyncResult.status() == concurrencpp::result_status::value)
                {
                    switch (asyncResult.status())
                    {
                        case concurrencpp::result_status::value:
                            m_cache[key] = asyncResult.get();
                            m_cache[key].m_state = cache_state::has_data;
                            m_asyncCache.erase(key);
                            break;
                        case concurrencpp::result_status::exception:
                            BOOST_ASSERT_MSG(false, "Async data threw an exception");
                            m_cache[key].m_state = cache_state::no_data;
                            m_asyncCache.erase(key);
                            break;
                        default:
                            break;
                    }
                }
            }

            return m_cache[key];
        }

        /**
         * @brief Set the cache expired so it can be filled again.
         */
        auto set_cache_expired(const key_type& key) -> void
        {
            m_cache[key].m_state = cache_state::expired;
        }

        /**
         * @brief Empty everything.
         */
        auto clear() -> void
        {
            m_asyncCache.clear();
            m_cache.clear();
        }

        [[nodiscard]] auto get_raw_data(const key_type& key) -> data_type&
        {
            return m_cache[key].m_cache;
        }

    private:
        mutable std::unordered_map<key_type, cached_data, key_hasher> m_cache;      //< The cache, ready to be used by the caller
        std::unordered_map<key_type, async_cache_result, key_hasher> m_asyncCache;  //< The async request to fill the cache

        std::chrono::seconds m_expirySeconds = std::chrono::seconds(60 * 10); 
	};
}