/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Host test for the HA100 IR completion guards. Build against this tree:
 * cc -std=c99 -Wall -Wextra -Werror \
 *    -Idrivers/misc/mediatek/irtx/mt6580 \
 *    tools/testing/ha100-irtx-completion-test.c -o /tmp/ha100-irtx-completion-test
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef uint32_t u32;
typedef int64_t s64;

#include "couch_irtx_complete.h"

int main(void)
{
    bool zero = true;

    for (s64 us = 0; us < 281; ++us)
        assert(!couch_irtx_complete(1, &zero, us, 281));
    assert(couch_irtx_complete(1, &zero, 281, 281));

    zero = false;
    for (s64 us = 0; us < 60000; us += 500)
        assert(!couch_irtx_complete(1, &zero, us, 281));
    assert(!zero);
    assert(!couch_irtx_complete(0, &zero, 0, 281));
    assert(zero);
    assert(!couch_irtx_complete(1, &zero, 280, 281));
    assert(couch_irtx_complete(1, &zero, 281, 281));

    assert(!couch_irtx_complete(1, &zero, 281, 68000));
    assert(!couch_irtx_complete(1, &zero, 67999, 68000));
    assert(couch_irtx_complete(1, &zero, 68000, 68000));
    assert(!couch_irtx_complete(0, &zero, 1000000, 68000));
    assert(!couch_irtx_complete(2, &zero, 1000000, 68000));
    assert(couch_irtx_first_complete(1, false, -1, 0) == -1);
    assert(couch_irtx_first_complete(0, true, -1, 500) == -1);
    assert(couch_irtx_first_complete(1, true, -1, 27000) == 27000);
    assert(couch_irtx_first_complete(1, true, 27000, 68000) == 27000);
    puts("HA100 IR completion guards passed");
    return 0;
}
