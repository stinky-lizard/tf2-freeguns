/**
 * Prints an output to the console.
 * 
 * @param in1    Test input.
 * @return      Test return.
 */
native int testFunc(int in1);

/**
 * Load extension
 */
public Extension __ext_freeguns = 
{
	name = "Freeguns",
	file = "freeguns.ext.2.tf2",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};