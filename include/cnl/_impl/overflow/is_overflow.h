
//          Copyright John McFarlane 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file ../LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef CNL_IMPL_OVERFLOW_IS_OVERFLOW_H
#define CNL_IMPL_OVERFLOW_IS_OVERFLOW_H

#include "polarity.h"
#include "../num_traits/digits.h"
#include "../operators.h"
#include "../type_traits/enable_if.h"
#include "../type_traits/is_signed.h"
#include "../../limits.h"
#include "../../numeric.h"

#include <type_traits>

/// compositional numeric library
namespace cnl {
    namespace _impl {
        ////////////////////////////////////////////////////////////////////////////////
        // cnl::_impl::overflow_digits

        template<class T, polarity>
        struct overflow_digits;

        template<class T>
        struct overflow_digits<T, polarity::positive>
                : public std::integral_constant<int, numeric_limits<T>::digits> {
        };

        template<class T>
        struct overflow_digits<T, polarity::negative>
                : public std::integral_constant<int, cnl::is_signed<T>::value ? digits<T>::value : 0> {
        };

        ////////////////////////////////////////////////////////////////////////////////
        // cnl::_impl::has_most_negative_number

        // implementation assumes one of three things:
        // 1. type is unsigned
        // 2. type is symmetrical around zero (e.g. elastic_integer)
        // 3. type has most negative number
        template<typename Operand, bool IsSigned = is_signed<Operand>::value>
        struct has_most_negative_number : std::false_type {
        };

        template<typename Operand>
        struct has_most_negative_number<Operand, true> : std::integral_constant<bool,
                numeric_limits<Operand>::lowest() < -numeric_limits<Operand>::max()> {
        };

        ////////////////////////////////////////////////////////////////////////////////
        // cnl::_impl::is_overflow_convert

        template<polarity Polarity, bool DestinationIsFloat, bool SourceIsFloat>
        struct is_overflow_convert;

        template<>
        struct is_overflow_convert<polarity::positive, false, false> {
            template<typename Destination, typename Source>
            constexpr bool operator()(Source const &rhs) const {
                return overflow_digits<Destination, polarity::positive>::value<
                        overflow_digits<Source, polarity::positive>::value
                                && rhs>
                static_cast<Source>(numeric_limits<Destination>::max());
            }
        };

        template<>
        struct is_overflow_convert<polarity::positive, false, true> {
            template<typename Destination, typename Source>
            constexpr bool operator()(Source const &rhs) const
            {
                return rhs > static_cast<Source>(numeric_limits<Destination>::max());
            }
        };

        template<>
        struct is_overflow_convert<polarity::positive, true, false> {
            template<typename Destination, typename Source>
            constexpr bool operator()(Source const& rhs) const
            {
                return static_cast<Destination>(rhs) > numeric_limits<Destination>::max();
            }
        };

        template<>
        struct is_overflow_convert<polarity::positive, true, true> {
            template<typename Destination, typename Source>
            constexpr bool operator()(Source const&) const
            {
                return false;
            }
        };

        template<>
        struct is_overflow_convert<polarity::negative, false, false> {
            template<typename Destination, typename Source>
            constexpr bool operator()(Source const &rhs) const {
                return overflow_digits<Destination, polarity::negative>::value<
                        overflow_digits<Source, polarity::negative>::value
                                && rhs < static_cast<Source>(numeric_limits<Destination>::lowest());
            }
        };

        template<>
        struct is_overflow_convert<polarity::negative, false, true> {
            template<typename Destination, typename Source>
            constexpr bool operator()(Source const &rhs) const
            {
                return rhs < static_cast<Source>(numeric_limits<Destination>::lowest());
            }
        };

        template<>
        struct is_overflow_convert<polarity::negative, true, false> {
            template<typename Destination, typename Source>
            constexpr bool operator()(Source const& rhs) const
            {
                return static_cast<Destination>(rhs) < numeric_limits<Destination>::lowest();
            }
        };

        template<>
        struct is_overflow_convert<polarity::negative, true, true> {
            template<typename Destination, typename Source>
            constexpr bool operator()(Source const&) const
            {
                return false;
            }
        };

        ////////////////////////////////////////////////////////////////////////////////
        // cnl::_impl::is_overflow

        template<typename Operator, polarity Polarity>
        struct is_overflow;

        template<polarity Polarity>
        struct is_overflow<convert_op, Polarity> {
            template<typename Destination, typename Source>
            constexpr bool operator()(Source const& from) const
            {
                using is_overflow_convert = cnl::_impl::is_overflow_convert<
                        Polarity,
                        std::is_floating_point<Destination>::value,
                        std::is_floating_point<Source>::value>;
                return is_overflow_convert{}.template operator()<Destination>(from);
            }
        };

        template<typename Operator, polarity Polarity>
        struct is_overflow {
            template<typename Rhs>
            constexpr bool operator()(Rhs const&) const
            {
                return false;
            }
        };

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4146)
#endif
        template<>
        struct is_overflow<minus_op, polarity::positive> {
            template<typename Rhs>
            constexpr bool operator()(Rhs const& from) const
            {
                return has_most_negative_number<Rhs>::value && from < -numeric_limits<Rhs>::max();
            }
        };
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

        ////////////////////////////////////////////////////////////////////////////////
        // cnl::_impl::operator_overflow_traits

        template<class Operator, typename ... Operands>
        struct operator_overflow_traits {
            using result = op_result<Operator, Operands...>;
            using numeric_limits = cnl::numeric_limits<result>;

            static constexpr int positive_digits = _impl::overflow_digits<result, polarity::positive>::value;
            static constexpr int negative_digits = _impl::overflow_digits<result, polarity::negative>::value;

            static constexpr result lowest()
            {
                return numeric_limits::lowest();
            }
            static constexpr result max()
            {
                return numeric_limits::max();
            }

            template<typename Operand>
            static constexpr int leading_bits(Operand const& operand)
            {
                return cnl::leading_bits(static_cast<result>(operand));
            }
        };

        // overflow tests
        template<class Operator, typename ... Operands>
        struct overflow_test_base {
            // by default, an operation is safe
            static constexpr bool positive(Operands const &...)
            {
                return false;
            }

            static constexpr bool negative(Operands const &...)
            {
                return false;
            }
        };

        template<class Operator, typename ... Operands>
        struct overflow_test : overflow_test_base<Operator, Operands...> {
        };

        template<typename Rhs>
        struct overflow_test<minus_op, Rhs> : overflow_test_base<minus_op, Rhs> {
            using traits = operator_overflow_traits<minus_op, Rhs>;

            static constexpr bool positive(Rhs const &rhs) {
                return has_most_negative_number<Rhs>::value
#if defined(_MSC_VER)
                #pragma warning(push)
#pragma warning(disable: 4146)
#endif
                        // Causes a warning about unsigned unary minus despite the fact that it should not be being
                        // evaluated when Rhs is unsigned due to an is_signed test in has_most_negative_number.
                        ? (-traits::max()) > rhs
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
                        : 0;
            }

            static constexpr bool negative(Rhs const &rhs) {
                return !is_signed<Rhs>::value && rhs;
            }
        };

        template<typename Lhs, typename Rhs>
        struct overflow_test<add_op, Lhs, Rhs> : overflow_test_base<add_op, Lhs, Rhs> {
            using traits = operator_overflow_traits<add_op, Lhs, Rhs>;

            static constexpr bool positive(Lhs const& lhs, Rhs const& rhs)
            {
                return (max(overflow_digits<Lhs, polarity::positive>::value,
                        overflow_digits<Rhs, polarity::positive>::value)+1
                        >traits::positive_digits)
                        && lhs>Lhs{0}
                        && rhs>Rhs{0}
                        && typename traits::result(lhs)>traits::max()-rhs;
            }

            static constexpr bool negative(Lhs const& lhs, Rhs const& rhs)
            {
                return (max(overflow_digits<Lhs, polarity::positive>::value,
                        overflow_digits<Rhs, polarity::positive>::value)+1
                        >traits::positive_digits)
                        && lhs<Lhs{0}
                        && rhs<Rhs{0}
                        && typename traits::result(lhs)<traits::lowest()-rhs;
            }
        };

        template<typename Lhs, typename Rhs>
        struct overflow_test<subtract_op, Lhs, Rhs> : overflow_test_base<subtract_op, Lhs, Rhs> {
            using traits = operator_overflow_traits<subtract_op, Lhs, Rhs>;

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif
            static constexpr bool positive(Lhs const& lhs, Rhs const& rhs)
            {
                return (max(overflow_digits<Lhs, polarity::positive>::value,
                        overflow_digits<Rhs, polarity::positive>::value)+1
                        >traits::positive_digits)
                        && lhs>Lhs{0}
                        && rhs<Rhs{0}
                        && lhs>traits::max()+rhs;
            }

            static constexpr bool negative(Lhs const& lhs, Rhs const& rhs)
            {
                return (max(overflow_digits<Lhs, polarity::positive>::value,
                        overflow_digits<Rhs, polarity::positive>::value)+1
                        >traits::positive_digits)
                        && (rhs>=0)
                        && lhs<traits::lowest()+rhs;
            }
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
        };

        template<typename Lhs, typename Rhs>
        struct overflow_test<multiply_op, Lhs, Rhs> : overflow_test_base<multiply_op, Lhs, Rhs> {
            using traits = operator_overflow_traits<multiply_op, Lhs, Rhs>;

#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif
            static constexpr bool positive(Lhs const& lhs, Rhs const& rhs)
            {
                return (overflow_digits<Lhs, polarity::positive>::value+overflow_digits<Rhs, polarity::positive>::value
                        > traits::positive_digits)
                        && ((lhs>Lhs{0})
                                ? (rhs>Rhs{0}) && (traits::max()/rhs)<lhs
                                : (rhs<Rhs{0}) && (traits::max()/rhs)>lhs);
            }

            static constexpr bool negative(Lhs const& lhs, Rhs const& rhs)
            {
                return (overflow_digits<Lhs, polarity::positive>::value+overflow_digits<Rhs, polarity::positive>::value
                        > traits::positive_digits)
                        && ((lhs<Lhs{0})
                                ? (rhs>Rhs{0}) && (traits::lowest()/rhs)>lhs
                                : (rhs<Rhs{0}) && (traits::lowest()/rhs)<lhs);
            }
#if defined(__clang__)
#pragma GCC diagnostic pop
#endif
        };

        template<typename Lhs, typename Rhs>
        struct overflow_test<divide_op, Lhs, Rhs>
                : overflow_test_base<divide_op, Lhs, Rhs> {
            using traits = operator_overflow_traits<divide_op, Lhs, Rhs>;

            static constexpr bool positive(Lhs const &lhs, Rhs const &rhs) {
                return (has_most_negative_number<Lhs>::value && has_most_negative_number<Rhs>::value)
                        ? rhs == -1 && lhs == traits::lowest()
                        : false;
            }
        };

        template<typename Lhs, typename Rhs, bool IsSigned>
        struct shift_left_overflow_test;

        template<typename Lhs, typename Rhs>
        struct shift_left_overflow_test<Lhs, Rhs, false>
                : overflow_test_base<Lhs, Rhs> {
            using traits = operator_overflow_traits<shift_left_op, Lhs, Rhs>;

            static constexpr bool negative(Lhs const&, Rhs const&)
            {
                return false;
            }
        };

        template<typename Lhs, typename Rhs>
        struct shift_left_overflow_test<Lhs, Rhs, true>
                : overflow_test_base<Lhs, Rhs> {
            using traits = operator_overflow_traits<shift_left_op, Lhs, Rhs>;

            static constexpr bool negative(Lhs const& lhs, Rhs const& rhs)
            {
                return lhs<0
                        ? rhs>0
                                ? rhs<traits::positive_digits
                                        ? (lhs >> (traits::positive_digits-rhs))!=-1
                                        : true
                                : false
                        : false;
            }
        };

        // shift-left test that is common to signed and unsigned, i.e. positive overflow
        template<typename Lhs, typename Rhs>
        struct overflow_test<shift_left_op, Lhs, Rhs>
                : shift_left_overflow_test<Lhs, Rhs, is_signed<Lhs>::value> {
            using traits = operator_overflow_traits<shift_left_op, Lhs, Rhs>;

            static constexpr bool positive(Lhs const& lhs, Rhs const& rhs)
            {
                return lhs>0
                        ? rhs>0
                                ? rhs<traits::positive_digits
                                        ? (lhs >> (traits::positive_digits-rhs))!=0
                                        : true
                                : false
                        : false;
            }
        };
    }
}

#endif  // CNL_IMPL_OVERFLOW_IS_OVERFLOW_H
