#pragma once

#include <EASTL/vector.h>

#include <boost/mpl/bool_fwd.hpp>
#include <boost/serialization/array_wrapper.hpp>
#include <boost/serialization/collection_size_type.hpp>
#include <boost/serialization/collections_load_imp.hpp>
#include <boost/serialization/collections_save_imp.hpp>
#include <boost/serialization/item_version_type.hpp>
#include <boost/serialization/library_version_type.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_free.hpp>

#include <type_traits>

// boost/serialization ships built-in support for std::vector via
// <boost/serialization/vector.hpp>. eastl::vector is a different type so boost cannot find
// those overloads via ADL. This header mirrors that file for eastl::vector: same dispatch,
// same wire format. eastl::vector<T> serializes identically to std::vector<T>.
//
// The dispatch matters: for archives that opt into array_optimization (binary_iarchive /
// binary_oarchive) AND a POD/primitive element type, both save and load use a single
// binary blob rather than a per-element loop. Without this, an existing bakery file
// written under std::vector<uint8_t> (or any primitive vector) will misalign on load
// via the slow path and blow up with stream errors.
//
// No vector<bool> specialisation: eastl::vector<bool> stores real bool elements, not
// bits, so the primary template handles it correctly.

#ifndef BOOST_SERIALIZATION_VECTOR_VERSIONED
    #define BOOST_SERIALIZATION_VECTOR_VERSIONED(V) (V == 4 || V == 5)
#endif

namespace boost::serialization
{
    // ---- default (non-optimized) path: any element type ---------------------------------

    template <class Archive, class U, class Allocator>
    inline void save(Archive& ar, const eastl::vector<U, Allocator>& t, const unsigned int /*version*/, boost::mpl::false_)
    {
        boost::serialization::stl::save_collection<Archive, eastl::vector<U, Allocator>>(ar, t);
    }

    template <class Archive, class U, class Allocator>
    inline void load(Archive& ar, eastl::vector<U, Allocator>& t, const unsigned int /*version*/, boost::mpl::false_)
    {
        const boost::serialization::library_version_type library_version(ar.get_library_version());
        item_version_type item_version(0);
        collection_size_type count;
        ar >> BOOST_SERIALIZATION_NVP(count);
        if (boost::serialization::library_version_type(3) < library_version)
        {
            ar >> BOOST_SERIALIZATION_NVP(item_version);
        }
        t.reserve(static_cast<typename eastl::vector<U, Allocator>::size_type>(count));
        boost::serialization::stl::collection_load_impl(ar, t, count, item_version);
    }

    // ---- optimized path: primitive element + array-optimization-friendly archive --------

    template <class Archive, class U, class Allocator>
    inline void save(Archive& ar, const eastl::vector<U, Allocator>& t, const unsigned int /*version*/, boost::mpl::true_)
    {
        const collection_size_type count(t.size());
        ar << BOOST_SERIALIZATION_NVP(count);
        if (!t.empty())
        {
            ar << boost::serialization::make_array<const U, collection_size_type>(
                static_cast<const U*>(&t[0]), count);
        }
    }

    template <class Archive, class U, class Allocator>
    inline void load(Archive& ar, eastl::vector<U, Allocator>& t, const unsigned int /*version*/, boost::mpl::true_)
    {
        collection_size_type count(t.size());
        ar >> BOOST_SERIALIZATION_NVP(count);
        t.resize(static_cast<typename eastl::vector<U, Allocator>::size_type>(count));
        unsigned int item_version = 0;
        if (BOOST_SERIALIZATION_VECTOR_VERSIONED(ar.get_library_version()))
        {
            ar >> BOOST_SERIALIZATION_NVP(item_version);
        }
        if (!t.empty())
        {
            ar >> boost::serialization::make_array<U, collection_size_type>(
                static_cast<U*>(&t[0]), count);
        }
    }

    // ---- dispatch -----------------------------------------------------------------------

    template <class Archive, class U, class Allocator>
    inline void save(Archive& ar, const eastl::vector<U, Allocator>& t, const unsigned int file_version)
    {
        using use_optimized = typename boost::serialization::use_array_optimization<Archive>::template apply<
            typename std::remove_const<U>::type>::type;
        save(ar, t, file_version, use_optimized());
    }

    template <class Archive, class U, class Allocator>
    inline void load(Archive& ar, eastl::vector<U, Allocator>& t, const unsigned int file_version)
    {
        using use_optimized = typename boost::serialization::use_array_optimization<Archive>::template apply<
            typename std::remove_const<U>::type>::type;
        load(ar, t, file_version, use_optimized());
    }

    template <class Archive, class U, class Allocator>
    inline void serialize(Archive& ar, eastl::vector<U, Allocator>& t, const unsigned int file_version)
    {
        boost::serialization::split_free(ar, t, file_version);
    }
}  // namespace boost::serialization
