// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2023-2024 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2013-2022.
// Modifications copyright (c) 2013-2023, Oracle and/or its affiliates.
// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_RELATE_POINT_POINT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_RELATE_POINT_POINT_HPP

#include <algorithm>
#include <vector>

#include <boost/range/empty.hpp>
#include <boost/range/size.hpp>

#include <boost/geometry/algorithms/detail/equals/point_point.hpp>
#include <boost/geometry/algorithms/detail/within/point_in_geometry.hpp>
#include <boost/geometry/algorithms/detail/relate/result.hpp>

#include <boost/geometry/policies/compare.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace relate {

template <typename Point1, typename Point2>
struct point_point
{
    static const bool interruption_enabled = false;

    template <typename Result, typename Strategy>
    static inline void apply(Point1 const& point1, Point2 const& point2,
                             Result & result,
                             Strategy const& strategy)
    {
        bool equal = detail::equals::equals_point_point(point1, point2, strategy);
        if ( equal )
        {
            update<interior, interior, '0'>(result);
        }
        else
        {
            update<interior, exterior, '0'>(result);
            update<exterior, interior, '0'>(result);
        }

        update<exterior, exterior, result_dimension<Point1>::value>(result);
    }
};

template <typename Point, typename MultiPoint, typename EqPPStrategy>
std::pair<bool, bool> point_multipoint_check(Point const& point,
                                             MultiPoint const& multi_point,
                                             EqPPStrategy const& strategy)
{
    bool found_inside = false;
    bool found_outside = false;

    // point_in_geometry could be used here but why iterate over MultiPoint twice?
    // we must search for a point in the exterior because all points in MultiPoint can be equal


    auto const end = boost::end(multi_point);
    for (auto it = boost::begin(multi_point); it != end; ++it)
    {
        bool ii = detail::equals::equals_point_point(point, *it, strategy);

        if (ii)
        {
            found_inside = true;
        }
        else
        {
            found_outside = true;
        }

        if (found_inside && found_outside)
        {
            break;
        }
    }

    return std::make_pair(found_inside, found_outside);
}

template <typename Point, typename MultiPoint, bool Transpose = false>
struct point_multipoint
{
    static const bool interruption_enabled = false;

    template <typename Result, typename Strategy>
    static inline void apply(Point const& point, MultiPoint const& multi_point,
                             Result & result,
                             Strategy const& strategy)
    {
        if ( boost::empty(multi_point) )
        {
            // TODO: throw on empty input?
            update<interior, exterior, '0', Transpose>(result);
            return;
        }

        std::pair<bool, bool> rel = point_multipoint_check(point, multi_point, strategy);

        if ( rel.first ) // some point of MP is equal to P
        {
            update<interior, interior, '0', Transpose>(result);

            if ( rel.second ) // a point of MP was found outside P
            {
                update<exterior, interior, '0', Transpose>(result);
            }
        }
        else
        {
            update<interior, exterior, '0', Transpose>(result);
            update<exterior, interior, '0', Transpose>(result);
        }

        update<exterior, exterior, result_dimension<Point>::value, Transpose>(result);
    }
};

template <typename MultiPoint, typename Point>
struct multipoint_point
{
    static const bool interruption_enabled = false;

    template <typename Result, typename Strategy>
    static inline void apply(MultiPoint const& multi_point, Point const& point,
                             Result & result,
                             Strategy const& strategy)
    {
        point_multipoint<Point, MultiPoint, true>::apply(point, multi_point, result, strategy);
    }
};

template <typename MultiPoint1, typename MultiPoint2>
struct multipoint_multipoint
{
    static const bool interruption_enabled = true;

    template <typename Result, typename Strategy>
    static inline void apply(MultiPoint1 const& multi_point1, MultiPoint2 const& multi_point2,
                             Result & result,
                             Strategy const& /*strategy*/)
    {
        {
            // TODO: throw on empty input?
            bool empty1 = boost::empty(multi_point1);
            bool empty2 = boost::empty(multi_point2);
            if ( empty1 && empty2 )
            {
                return;
            }
            else if ( empty1 )
            {
                update<exterior, interior, '0'>(result);
                return;
            }
            else if ( empty2 )
            {
                update<interior, exterior, '0'>(result);
                return;
            }
        }

        // The geometry containing smaller number of points will be analysed first
        if ( boost::size(multi_point1) < boost::size(multi_point2) )
        {
            search_both<false, Strategy>(multi_point1, multi_point2, result);
        }
        else
        {
            search_both<true, Strategy>(multi_point2, multi_point1, result);
        }

        update<exterior, exterior, result_dimension<MultiPoint1>::value>(result);
    }

    template <bool Transpose, typename Strategy, typename MPt1, typename MPt2, typename Result>
    static inline void search_both(MPt1 const& first_sorted_mpt, MPt2 const& first_iterated_mpt,
                                   Result & result)
    {
        if ( relate::may_update<interior, interior, '0'>(result)
          || relate::may_update<interior, exterior, '0'>(result)
          || relate::may_update<exterior, interior, '0'>(result) )
        {
            // NlogN + MlogN
            bool const is_disjoint = search<Transpose, Strategy>(first_sorted_mpt, first_iterated_mpt, result);

            if (is_disjoint || BOOST_GEOMETRY_CONDITION(result.interrupt) )
            {
                return;
            }
        }

        if ( relate::may_update<interior, interior, '0'>(result)
          || relate::may_update<interior, exterior, '0'>(result)
          || relate::may_update<exterior, interior, '0'>(result) )
        {
            // MlogM + NlogM
            search<! Transpose, Strategy>(first_iterated_mpt, first_sorted_mpt, result);
        }
    }

    template <bool Transpose,
              typename Strategy,
              typename SortedMultiPoint,
              typename IteratedMultiPoint,
              typename Result>
    static inline bool search(SortedMultiPoint const& sorted_mpt,
                              IteratedMultiPoint const& iterated_mpt,
                              Result & result)
    {
        // sort points from the 1 MPt
        using point_type = geometry::point_type_t<SortedMultiPoint>;
        using less_type = geometry::less<void, -1, Strategy>;

        std::vector<point_type> points(boost::begin(sorted_mpt), boost::end(sorted_mpt));

        less_type const less = less_type();
        std::sort(points.begin(), points.end(), less);

        bool found_inside = false;
        bool found_outside = false;

        // for each point in the second MPt
        for (auto it = boost::begin(iterated_mpt); it != boost::end(iterated_mpt); ++it)
        {
            bool ii = std::binary_search(points.begin(), points.end(), *it, less);
            if (ii)
            {
                found_inside = true;
            }
            else
            {
                found_outside = true;
            }

            if (found_inside && found_outside)
            {
                break;
            }
        }

        if ( found_inside ) // some point of MP2 is equal to some of MP1
        {
// TODO: if I/I is set for one MPt, this won't be changed when the other one in analysed
//       so if e.g. only I/I must be analysed we musn't check the other MPt

            update<interior, interior, '0', Transpose>(result);

            if ( found_outside ) // some point of MP2 was found outside of MP1
            {
                update<exterior, interior, '0', Transpose>(result);
            }
        }
        else
        {
            update<interior, exterior, '0', Transpose>(result);
            update<exterior, interior, '0', Transpose>(result);
        }

        // if no point is intersecting the other MPt then we musn't analyse the reversed case
        return ! found_inside;
    }
};

}} // namespace detail::relate
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_RELATE_POINT_POINT_HPP
