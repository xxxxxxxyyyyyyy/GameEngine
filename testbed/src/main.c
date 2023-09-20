// #include <test.h>
#include <core/logger.h>
#include <core/asserts.h>

// TODO: Test
#include <platform/platform.h>

int main(void) {
	// print_number(42);
	KFATAL("a test message: %f", 3.14f);
	KERROR("a test message: %f", 3.14f);
	KWARN("a test message: %f", 3.14f);
	KINFO("a test message: %f", 3.14f);
	KDEBUG("a test message: %f", 3.14f);
	KTRACE("a test message: %f", 3.14f);

	// KASSERT_MSG(1 == 0, "King Of My Room!");
	// KASSERT(FALSE);
	platform_state state;
	if (platform_startup(&state, "Engine Testbed", 100, 100, 1280, 720)) {
		while (TRUE)
		{
			platform_pump_messages(&state);
		}
	}
	platform_shutdown(&state);

	return 0;
}