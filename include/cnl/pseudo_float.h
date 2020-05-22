
//          Copyright John McFarlane 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file ../LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/// \file
/// \brief contains pseudo_float alias to scaled_integer

#if !defined(CNL_PSEUDO_FLOAT_H)
#define CNL_PSEUDO_FLOAT_H

#include "_impl/scaled/pseudo_float_tag.h"
#include "scaled_integer.h"

/// compositional numeric library
namespace cnl {
    template<typename Rep = int, int Exponent = 0, int Radix = 2>
    using pseudo_float = scaled_integer<Rep, pseudo_float_tag<Exponent, Radix>>;

    template<typename Value>
    constexpr auto make_pseudo_float(Value const& value)
    -> decltype(make_pseudo_float(value))
    {
        return make_pseudo_float(value);
    }
}

#endif  // CNL_PSEUDO_FLOAT_H
