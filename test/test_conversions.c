#include "conversions.c"

#include <cmocka.h>

void test_region_coords(void) {
    { // topright quarter, bottomleft region
        struct lli_xy expected = { .x = 0ll, .y = 0ll};

        { // bottomleft block
            struct lli_xy res = region_coords(0ll, 0ll);
            assert_true(lli_xy_equals(res, expected));
        }

        { // topleft block
            struct lli_xy res = region_coords(0ll, 511ll);
            assert_true(lli_xy_equals(res, expected));
        }

        { // topright block
            struct lli_xy res = region_coords(511ll, 511ll);
            assert_true(lli_xy_equals(res, expected));
        }

        { // bottomright block
            struct lli_xy res = region_coords(511ll, 0ll);
            assert_true(lli_xy_equals(res, expected));
        }
    } // end topright quarter, bottomleft region


    { // topright quarter, extra region
        struct lli_xy expected = { .x = 1ll, .y = 1ll};

        { // bottomleft block
            struct lli_xy res = region_coords(512ll, 512ll);
            assert_true(lli_xy_equals(res, expected));
        }

        { // topleft block
            struct lli_xy res = region_coords(512ll, 1023ll);
            assert_true(lli_xy_equals(res, expected));
        }

        { // topright block
            struct lli_xy res = region_coords(1023ll, 1023ll);
            assert_true(lli_xy_equals(res, expected));
        }

        { // bottomright block
            struct lli_xy res = region_coords(1023ll, 512ll);
            assert_true(lli_xy_equals(res, expected));
        }
    } // end topright quarter, extra region


     // bottomright quarter, topleft region
    {
        struct lli_xy expected = { .x = 0ll, .y = -1ll};

        { // bottomleft block
            struct lli_xy res = region_coords(0ll, -512ll);
            assert_true(lli_xy_equals(res, expected));
        }

        { // topleft block
            struct lli_xy res = region_coords(0ll, -1ll);
            assert_true(lli_xy_equals(res, expected));
        }

        { // topright block
            struct lli_xy res = region_coords(511ll, -1ll);
            assert_true(lli_xy_equals(res, expected));
        }

        { // bottomright block
            struct lli_xy res = region_coords(511ll, -512ll);
            assert_true(lli_xy_equals(res, expected));
        }
    } // end bottomright quarter, topleft region


    // bottomleft quarter, topright region
    {
        struct lli_xy expected = { .x = -1ll, .y = -1ll};

        { // bottomleft block
            struct lli_xy res = region_coords(-512ll, -512ll);
            assert_true(lli_xy_equals(res, expected));
        }

        { // topleft block
            struct lli_xy res = region_coords(-512ll, -1ll);
            assert_true(lli_xy_equals(res, expected));
        }

        { // topright block
            struct lli_xy res = region_coords(-1ll, -1ll);
            assert_true(lli_xy_equals(res, expected));
        }

        { // bottomright block
            struct lli_xy res = region_coords(-1ll, -512ll);
            assert_true(lli_xy_equals(res, expected));
        }
    } // end bottomleft quarter, topright region    


    // topleft quarter, bottomright region
     {
        struct lli_xy expected = { .x = -1ll, .y = 0ll};

        { // bottomleft block
            struct lli_xy res = region_coords(-512ll, 0ll);
            assert_true(lli_xy_equals(res, expected));
        }

        { // topleft block
            struct lli_xy res = region_coords(-512ll, 511ll);
            assert_true(lli_xy_equals(res, expected));
        }

        { // topright block
            struct lli_xy res = region_coords(-1ll, 511ll);
            assert_true(lli_xy_equals(res, expected));
        }

        { // bottomright block
            struct lli_xy res = region_coords(-1ll, 0ll);
            assert_true(lli_xy_equals(res, expected));
        }
    } // end topleft quarter, bottomright region
}
