//
// Part of Metta OS. Check https://atta-metta.net for latest version.
//
// Copyright 2007 - 2017, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
/**
 * @brief Test area_t.
 */

/*============================================================================*/

#include <string.h>

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "merge_mmap.cpp"

BOOST_AUTO_TEST_SUITE( test_suite )

BOOST_AUTO_TEST_CASE(test_area_t)
{
    //(1,2).intersect(-1,0) == none
    //(1,2).intersect(3,4) == none
    //(1,2).intersect(0,1) == left_edge
    //(1,2).intersect(2,3) == right_edge
    //(1,3).intersect(1,2) == left_edge
    //(1,3).intersect(2,3) == right_edge
    //(1,4).intersect(2,3) == within
    BOOST_CHECK_EQUAL(bit_array::INDEX_TO_BIT(0), 0);
    BOOST_CHECK_EQUAL(bit_array::INDEX_TO_BIT(10), 320);

    BOOST_CHECK_EQUAL(bit_array::INDEX_FROM_BIT(0), 0);
    BOOST_CHECK_EQUAL(bit_array::INDEX_FROM_BIT(32), 1);
    BOOST_CHECK_EQUAL(bit_array::INDEX_FROM_BIT(35), 1);

    BOOST_CHECK_EQUAL(bit_array::OFFSET_FROM_BIT(0), 0);
    BOOST_CHECK_EQUAL(bit_array::OFFSET_FROM_BIT(32), 0);
    BOOST_CHECK_EQUAL(bit_array::OFFSET_FROM_BIT(33), 1);
    BOOST_CHECK_EQUAL(bit_array::OFFSET_FROM_BIT(35), 3);

    bit_array array(32);
    array.set(1);
    BOOST_CHECK_EQUAL(array.test(1), true);
    BOOST_CHECK_EQUAL(array.test(0), false);
    BOOST_CHECK_EQUAL(array.test(0), false);
}

BOOST_AUTO_TEST_SUITE_END()
