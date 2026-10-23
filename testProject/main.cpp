#include "test_functions.h"

int main()
{
	/*test_empty_class_size();
	test_shared_ptr_circular_reference();
	test_new_and_malloc();
	test_function_pointer_polymorphism();
	test_virtual_inheritance();
	test_function_overload();
	test_value_copy_and_alias();*/

	// Run this test by itself because it intentionally invokes undefined behavior.
	// test_unsafe_vector_concurrent_insert();
	//test_map_and_unorder_map();
	test_vector_shrink_to_fit();
	return 0;
}
