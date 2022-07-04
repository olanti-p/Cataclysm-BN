#pragma once
#ifndef CATA_SRC_HASH_TRAITS_H
#define CATA_SRC_HASH_TRAITS_H

#include <utility>
#include <vector>
#include <type_traits>
#include <cstdint>

#include "integer.h"

template<typename T>
struct hash_traits { };

/**
 * @brief Implement std::hash for type that implements hash_traits.
 */
#define IMPLEMENT_STD_HASH( T )                                 \
    template<>                                                  \
    struct std::hash<T> {                                       \
        std::size_t operator()( const T &v ) const noexcept {   \
            return hash_traits<T>{}( v );                       \
        }                                                       \
    }

/**
 * @brief This value can be used as hash seed.
 *
 * 3RD_PARTY_SNIPPET: taken from boost::hash_combine, but any other
 *                    random number should work as well.
 */
constexpr u64 HASH_SEED      = 0x9E3779B9234CECB3;

/**
 * @brief Call repeatedly to create a hash value from several variables.
 *
 * 3RD_PARTY_SNIPPET: famous boost::hash_combine
 */
template <class T, class Hasher = hash_traits<T>>
inline void hash_combine( u64 &seed, const T &v )
{
    Hasher hasher;
    u64 x = hasher( v );
    x += HASH_SEED;
    x += seed << 6;
    x += seed >> 2;
    seed ^= x;
}

template<typename T1, typename T2>
struct hash_traits<std::pair<T1, T2>> {
    u64 operator()( const std::pair<T1, T2> &v ) const noexcept {
        u64 ret = HASH_SEED;
        hash_combine( ret, v.first );
        hash_combine( ret, v.second );
        return ret;
    }
};

template<typename T>
struct hash_traits<std::vector<T>> {
    u64 operator()( const std::vector<T> &v ) const noexcept {
        u64 ret = HASH_SEED;
        for( const T &it : v ) {
            hash_combine( ret, it );
        }
        return ret;
    }
};

template<>
struct hash_traits<char> {
    u64 operator()( char v ) const noexcept {
        return v;
    }
};

template<>
struct hash_traits<u32> {
    u64 operator()( u32 v ) const noexcept {
        return v;
    }
};

template<>
struct hash_traits<u64> {
    u64 operator()( u64 v ) const noexcept {
        return v;
    }
};

template<>
struct hash_traits<i32> {
    u64 operator()( i32 v ) const noexcept {
        return static_cast<u64>( v );
    }
};

template<>
struct hash_traits<i64> {
    u64 operator()( i64 v ) const noexcept {
        return static_cast<u64>( v );
    }
};

template<>
struct hash_traits<std::string> {
    u64 operator()( const std::string &v ) const noexcept {
        u64 ret = HASH_SEED;
        for( char it : v ) {
            hash_combine( ret, it );
        }
        return ret;
    }
};

/**
 * @brief Allows using enum class as key value for std::unordered_map.
 */
struct EnumClassHasher {
    template <typename E>
    std::size_t operator()( E v ) const {
        return hash_traits<i64> {}( static_cast<i64>( v ) );
    }
};

/**
 * @brief Hasher for std::pair.
 */
struct StdPairHasher {
    template <class T1, class T2>
    std::size_t operator()( const std::pair<T1, T2> &v ) const {
        return hash_traits<std::pair<T1, T2>> {}( v );
    }
};

/**
 * @brief Hasher for std::vector.
 */
struct StdVectorHasher {
    template <class T>
    std::size_t operator()( const std::vector<T> &v ) const {
        return hash_traits<std::vector<T>> {}( v );
    }
};

#endif // CATA_SRC_HASH_TRAITS_H
