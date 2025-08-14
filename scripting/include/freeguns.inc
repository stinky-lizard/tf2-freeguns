
native int native_enableDetours();
native int native_disableDetours();

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