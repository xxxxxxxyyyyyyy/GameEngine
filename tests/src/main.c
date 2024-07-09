#include "test_manager.h"

#include "memory/linear_allocator_tests.h"
#include "containers/hashtable_tests.h"
#include "containers/freelist_tests.h"
#include "memory/dynamic_allocator_tests.h"

#include <core/logger.h>

int main() {
    test_manager_init();

    linear_allocator_register_tests();
    hashtable_register_tests();
    freelist_register_tests();
    dynamic_allocator_register_tests();

    DDEBUG("Starting tests");

    test_manager_run_tests();

    return 0;
}