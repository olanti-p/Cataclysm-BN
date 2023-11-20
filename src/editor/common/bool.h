#ifndef CATA_SRC_EDITOR_BOOL_H
#define CATA_SRC_EDITOR_BOOL_H

namespace editor
{

/**
 * Workaround for std::vector<bool> being special. Just use std::vector<Bool> instead.
 */
struct Bool {
    public:
        constexpr Bool() = default;
        constexpr Bool( bool v ) : value( v ) {}

        constexpr Bool( const Bool & ) = default;
        constexpr Bool( Bool && ) = default;

        constexpr Bool &operator=( const Bool & ) = default;
        constexpr Bool &operator=( Bool && ) = default;

        constexpr operator bool() const {
            return value;
        }
        constexpr bool operator==( const Bool &rhs ) const {
            return value == rhs.value;
        }
        constexpr bool operator!=( const Bool &rhs ) const {
            return value != rhs.value;
        }

    private:
        bool value = false;
};

} // namespace editor

#endif // CATA_SRC_EDITOR_BOOL_H
