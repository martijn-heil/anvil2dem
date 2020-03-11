#include <stddef.h>

#include "conversions.c"

#include <cmocka.h>

struct test_region 
{
    long long minx;
    long long maxx;
    long long miny;
    long long maxy;
    long long region_x;
    long long region_y;
};

struct test_region extremes[] = {
    { .region_x = 0,    .region_y = 0,  .minx = 0ll,    .maxx = 511ll,  .miny = 0ll,    .maxy = 511ll },    // topright quarter, bottomleft region
    { .region_x = 1,    .region_y = 1,  .minx = 512ll,  .maxx = 1023ll, .miny = 512ll,  .maxy = 1023ll },   // topright quarter, extra region
    { .region_x = 0,    .region_y = -1, .minx = 0ll,    .maxx = 511ll,  .miny = -512ll, .maxy = -1ll },     // bottomright quarter, topleft region
    { .region_x = -1,   .region_y = -1, .minx = -512ll, .maxx = -1ll,   .miny = -512ll, .maxy = -1ll },     // bottomleft quarter, topright region
    { .region_x = -1,   .region_y = 0,  .minx = -512ll, .maxx = -1ll,   .miny = 0ll,    .maxy = 511ll },    // topleft quarter, bottomright region
};

void test_region_coords(void) 
{
    for (size_t i = 0; i < sizeof(extremes); i++)
    {
        struct test_region *test_case = extremes + i;
        assert_true(lli_xy_equals(region_coords(test_case.minx, test_case.miny), struct lli_xy { .x = test_case.region_x, .y = test_case.region_y}));
        assert_true(lli_xy_equals(region_coords(test_case.minx, test_case.maxy), struct lli_xy { .x = test_case.region_x, .y = test_case.region_y}));
        assert_true(lli_xy_equals(region_coords(test_case.maxx, test_case.maxy), struct lli_xy { .x = test_case.region_x, .y = test_case.region_y}));
        assert_true(lli_xy_equals(region_coords(test_case.maxx, test_case.miny), struct lli_xy { .x = test_case.region_x, .y = test_case.region_y}));
    }
}

void test_region_origin_topleft(void) 
{
    for (size_t i = 0; i < sizeof(extremes); i++)
    {
        struct test_region *test_case = extremes + i;
        struct lli_xy correct_origin = { .x = test_case.minx, .y = test_case.maxy };
        assert_true(lli_xy_equals(region_origin_topleft(test_region.region_x, test_region.region_y), correct_origin));
    }
}
