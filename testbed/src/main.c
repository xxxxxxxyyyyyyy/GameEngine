// #include <test.h>
#include <core/logger.h>
#include <core/asserts.h>

int main(void) {
	// print_number(42);
	KFATAL("a test message: %f", 3.14f);
	KERROR("a test message: %f", 3.14f);
	KWARN("a test message: %f", 3.14f);
	KINFO("a test message: %f", 3.14f);
	KDEBUG("a test message: %f", 3.14f);
	KTRACE("a test message: %f", 3.14f);

	KASSERT(FALSE);
	return 0;
}