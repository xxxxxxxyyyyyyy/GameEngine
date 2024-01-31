#include "test_manager.h"

#include "memory/linear_allocator_tests.h"

#include <core/logger.h>
#include <stdio.h>

int main() {
    test_manager_init();

    linear_allocator_register_tests();

    KDEBUG("Starting tests");

    test_manager_run_tests();

	u32 x;
	scanf_s("%d", &x);
    return 0;
}