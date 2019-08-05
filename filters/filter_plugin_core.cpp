/*
	===============================================================

	SHADERMAP FILTER PLUGIN CORE SOURCE FILE

	ShaderMap Filter Plugins are used to alter map pixels that are
	sent to the plugin. Example filters include: Brightness, Blur,
	Normalize, etc.

	Include this source code file in a Win32 DLL project. 
	#include "filter_plugin_core.cpp". You should ensure that the 
	DLL extension is *.smf. It must be placed in the 
	"plugins/filters" folder found in the ShaderMap root directory.

	There are examples included with this source code that should
	help you get started.


	SHADERMAP SDK LICENSE
	
	The ShaderMap SDK is released under The MIT License (MIT)
	http://opensource.org/licenses/MIT

	Copyright (c) 2007-2019 Rendering Systems Inc.
	
	Permission is hereby granted, free of charge, to any person 
	obtaining a copy of this software and associated documentation 
	files (the "Software"), to deal	in the Software without 
	restriction, including without limitation the rights to use, 
	copy, modify, merge, publish, distribute, sublicense, and/or 
	sell copies of the Software, and to permit persons to whom the 
	Software is	furnished to do so, subject to the following 
	conditions:

	The above copyright notice and this permission notice shall be 
	included in	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
	OF MERCHANTABILITY,	FITNESS FOR A PARTICULAR PURPOSE AND 
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
	OTHER DEALINGS IN THE SOFTWARE.
	
	Developed by: Neil Kemp at Rendering Systems Inc.
	
	Online:		http://shadermap.com
	Corporate:	http://renderingsystems.com

	===============================================================
*/


// ----------------------------------------------------------------
// ----------------------------------------------------------------
// General includes

#include <stdio.h>
#include <tchar.h>
#include "windows.h"


// ----------------------------------------------------------------
// ----------------------------------------------------------------
// Plugin required defines

// SDK Version
#define SMSDK_VERSION_MAJOR						4
#define SMSDK_VERSION_MINOR						3

// Gamma control
#define GAMMA									2.2f
#define APPLY_GAMMA(x)							(pow(x, 1.0f / GAMMA))				// Converts linear pixel channel to sRGB color space.
#define REMOVE_GAMMA(x)							(pow(x, 1.0f / (1.0f / GAMMA)))		// Converts sRGB pixel channel to linear color space.

// Tile types
#define MAP_TILE_NONE							0
#define MAP_TILE_X								1
#define MAP_TILE_Y								2
#define MAP_TILE_XY								3

// Normal Map Support types
#define FILTER_NORMAL_NONE						0	// No normal map support, will not be displayed when adding a filter to a normal map.
#define FILTER_NORMAL_ONLY						1	// Normal map only support, will only be displayed when add a filter to a normal map.
#define FILTER_NORMAL_PLUS						2	// Normal map PLUS color / grayscale support. Will be displayed for both normal and color based maps.

// Map coordinate system
// 3 should be OR-ed together to define a coordinate system.
#define MAP_COORDSYS_X_POS_LEFT					0x00000001
#define MAP_COORDSYS_X_POS_RIGHT				0x00000002
#define MAP_COORDSYS_Y_POS_UP					0x00000004
#define MAP_COORDSYS_Y_POS_DOWN					0x00000008
#define MAP_COORDSYS_Z_POS_NEAR					0x00000010
#define MAP_COORDSYS_Z_POS_FAR					0x00000020

// Error define - a simple way to log errors to the ShaderMap log files located in: "C:\Users\<USERNAME>\AppData\Roaming\SM3\log"
// Example: LOG_ERROR_MSG(map_id, filter_position, _T("Error description");
#define											LOG_ERROR_MSG(map_id, filter_position, error) fp_log_filter_error(map_id, filter_position, error, _T(__FUNCTION__), _T(__FILE__), __LINE__);


// ----------------------------------------------------------------
// ----------------------------------------------------------------
// Plugin required structs

// A struct to pass filter plugin info from plugin to ShaderMap
// Used with "fp_set_plugin_info()" in "on_initialize()".
struct filter_plugin_info_s
{
	unsigned int								version;				// An integer representing the version of the plugin. Examples: 101, 102, 103.
	const wchar_t*								name;					// The name of the plugin. Example: "Contrast / Brightness"
	const wchar_t*								description;			// A text description of the plugin.
	const wchar_t*								thumb_filename;			// A thumbnail file displayed in the Filter Stack dialog. Image file must be placed in plugins/filters/thumbs/ in the ShaderMap directory.
	unsigned int								normal_support_type;	// FILTER_NORMAL_(NONE | ONLY | PLUS). See comments above for details.
	BOOL										is_legacy;				// If TRUE then this plugin is only available on projects that require it, it is not shown on the ShaderMap add filter list.
	

	// c()
	filter_plugin_info_s::filter_plugin_info_s(void)
	{
		version									= 0;
		name									= 0;
		description								= 0;
		thumb_filename							= 0;
		normal_support_type						= FILTER_NORMAL_NONE;
		is_legacy								= FALSE;
	}
};

// Struct for the "on_process()" function contains all data needed to 
// access property values and apply the filter to image pixels.
struct process_data_s
{
	unsigned int								map_id;					// Needed to call get property functions.
	int											filter_position;		// ^

	BOOL										is_grayscale;			// If TRUE then 2 channels of float type, else 4 channels of float type.
	BOOL										is_sRGB;				// If TRUE then pixels in sRGB color space else in linear color space - will be FALSE for normal maps.
	BOOL										is_normal_map;			// If TRUE then floats are vectors in range of -1.0f to 1.0f else colors in range 0.0f to 1.0f.
	unsigned int								map_width;				// Map with and height.
	unsigned int								map_height;
	unsigned int								map_tile_type;			// Contains one of the following values: (MAP_TILE_NONE, MAP_TILE_X, MAP_TILE_Y, MAP_TILE_XY)
	unsigned int								map_coordinate_system;	// Only set if map is a normal map. A combination of 3 OR-ed MAP_COORDSYS_XYZ values as defined.

	void*										map_pixel_data;			// Map pixel data - pixels read and write pixel data here. This array is allocated in ShaderMap and can not be deallocated in the plugin.
																		// The origin of the map pixels is UPPER LEFT.
																		// This array is made of floating point values. If is_grayscale == TRUE then there are 2 floats per pixel. If is_grayscale == FALSE then
																		// There are 4 floats per pixel. 
																		// Grayscale is (Color, Alpha) in the range 0.0f - 1.0f.
																		// Color is (Red, Green, Blue, Alpha) in the range 0.0f - 1.0f. 
																		// If the map is_normal_map == TRUE then the floats will be (X, Y, Z, Alpha) where X, Y, and Z are in the range -1.0f to 1.0f and the Alpha is 0.0f - 1.0f. 
																		// The map_coordinate_system will contain the normal map's coordinate system.

	// c()
	process_data_s::process_data_s(void)
	{
		map_id									= 0;
		filter_position							= 0;

		is_grayscale							= FALSE;
		is_sRGB									= FALSE;
		is_normal_map							= FALSE;
		map_width								= 0;
		map_height								= 0;
		map_tile_type							= 0;
		map_coordinate_system					= 0;

		map_pixel_data							= 0;
	}
};


// ----------------------------------------------------------------
// ----------------------------------------------------------------
// Definition of function types and pointers to functions

// **
// Setup and info functions

// Functions used in "on_initialize()" to start / stop initialization.
// These two functions should be called at start and end of "on_initialize()"
typedef void									(*fp_begin_initialize_type)(void);
fp_begin_initialize_type						fp_begin_initialize = 0;
typedef void									(*fp_end_initialize_type)(void);
fp_end_initialize_type							fp_end_initialize = 0;

// Function used to set the translation file to be used with this plugin
// This should be called in "on_initialize()" between "fp_begin_initialize()" and "fp_end_initialize()"
// An example: fp_define_translation_file(_T("my_plugin"), _T("en")); for a file [prefix].my_plugin.txt
// If a translation file for the current language is not set then the file with the default language prefix will be used.
// Returns the language file index which must be passed to fp_get_trans_string()
typedef unsigned int							(*fp_define_translation_file_type)(const wchar_t* /*file_title*/, const wchar_t* /*default_prefix*/);
fp_define_translation_file_type					fp_define_translation_file = 0;

// Function to get a string from the language translation file defined with fp_define_translation_file()
typedef const wchar_t*							(*fp_get_trans_string_type)(unsigned int /*file_index*/, unsigned int /*id*/);
fp_get_trans_string_type						fp_get_trans_string = 0;

// Function used to set the help file to be used with this plugin
// This should be called in "on_initialize()" between "fp_begin_initialize()" and "fp_end_initialize()"
// An example: fp_define_help_file(_T("plugins\\help_file.txt"), _T("en")); 
// If a help file for the current language is not found then the file with the default language will be used.
typedef void									(*fp_define_help_file_type)(const wchar_t* /*help_file*/, const wchar_t* /*default_language*/);
fp_define_help_file_type						fp_define_help_file = 0;

// Set the plugin info by passing a "filter_plugin_info_s" struct to ShaderMap.
// This should be called in "on_initialize()" between "fp_begin_initialize()" and "fp_end_initialize()"
typedef void									(*fp_set_plugin_info_type)(const filter_plugin_info_s& /*plugin_info*/);
static fp_set_plugin_info_type					fp_set_plugin_info = 0;

// Get the option for default coord sys - Useful when setting up the initial value when adding a coordinate system property with "fp_add_property_coordsys()"
typedef unsigned int							(*fp_get_option_default_coord_sys_type)(void);
fp_get_option_default_coord_sys_type			fp_get_option_default_coord_sys = 0;
									

// ** 
// Functions to add property controls to the filter - added in order called - first will have index of 0 (zero).
// Each control (except page list) can be assigned to a page index. All controls should be added sorted by page index (if pagelist property is used). See example below:
/*
	wchar_t string_array[] = {_T("0"), _T("1") };					// Property Index:
	fp_add_property_pagelist(_T("Page: "), string_array, 2, 0);		// 0

	// Page 0
	fp_add_property_checkbox(_T("Page 0 Checkbox 1"), TRUE, 0);		// 1 
	fp_add_property_checkbox(_T("Page 0 Checkbox 2"), TRUE, 0);		// 2 

	// Page 1
	fp_add_property_checkbox(_T("Page 1 Checkbox 1"), TRUE, 1);		// 1 
	fp_add_property_checkbox(_T("Page 1 Checkbox 2"), TRUE, 1);		// 2 
*/

// IMPORTANT - ShaderMap automatically adds 2 properties to every Filter plugin. They are 2 checkboxes for Mask control. Even if no property controls are
// added by the user these 2 property controls will be added. The first checkbox controls if the Mask should be applied, the second checkbox 
// controls if the Mask should be inverted. The properties will be placed at the end of each property page and their indices will follow accordingly.
// See the examples included with this source code for a demonstration of how the auto added Mask properties can be accessed.

// The page list property (if used) must be the first property added, it defines the number of property pages - see above example.
typedef void									(*fp_add_property_pagelist_type)(const wchar_t* /*caption*/, const wchar_t** /*string_array*/, unsigned int /*string_count*/, unsigned int /*cur_select*/);
static fp_add_property_pagelist_type			fp_add_property_pagelist = 0;

// Add a file property. Set caption, an initial drive path, and extension filter. Allows the user to set a filepath control to the filter.
typedef void									(*fp_add_property_file_type)(const wchar_t* /*caption*/, const wchar_t* /*initial_path*/, const wchar_t* /*extension_filter_pointer*/, unsigned int /*page_index*/);
static fp_add_property_file_type				fp_add_property_file = 0;

// Add a checkbox property. Set caption and initial check state.
typedef void									(*fp_add_property_checkbox_type)(const wchar_t* /*caption*/, BOOL /*is_checked*/, unsigned int /*page_index*/);
static fp_add_property_checkbox_type			fp_add_property_checkbox = 0;

// Add a list property. Set caption, string array and count, as well as initial selected item.
typedef void									(*fp_add_property_list_type)(const wchar_t* /*caption*/, const wchar_t** /*string_array*/, unsigned int /*string_count*/, unsigned int /*cur_select*/, unsigned int /*page_index*/);
static fp_add_property_list_type				fp_add_property_list = 0;

// Add integer numberbox property. Set the caption, min and max integers, and initial value.
typedef void									(*fp_add_property_numberbox_int_type)(const wchar_t* /*caption*/, int /*min*/, int /*max*/, int /*value*/, unsigned int /*page_index*/);
static fp_add_property_numberbox_int_type		fp_add_property_numberbox_int = 0;

// Add floating point numberbox property. Set the caption, min and max floats, and initial value.
typedef void									(*fp_add_property_numberbox_float_type)(const wchar_t* /*caption*/, float /*min*/, float /*max*/, float /*value*/, unsigned int /*page_index*/);
static fp_add_property_numberbox_float_type		fp_add_property_numberbox_float = 0;

// Add a colorbox property. Set the caption and initial color (use Windows RGB() macro).
typedef void									(*fp_add_property_colorbox_type)(const wchar_t* /*caption*/, COLORREF /*color*/, unsigned int /*page_index*/);
static fp_add_property_colorbox_type			fp_add_property_colorbox = 0;

// Add a slider property. Set the caption, min and max integers, initial position. Also can enable forced center to be at a set integer.
// Forced center can be useful, for example, when the min is -10 and the max is 100 but you want the center to be 0.
typedef void									(*fp_add_property_slider_type)(const wchar_t* /*caption*/, int /*min*/, int /*max*/, int /*position*/, unsigned int /*page_index*/, BOOL /*is_forced_center*/, int /*forced_center*/);
static fp_add_property_slider_type				fp_add_property_slider = 0;

// Add a range slider property. Set the 3 optional captions, min and max integers, initial min and max position.
typedef void									(*fp_add_property_range_slider_type)(const wchar_t* /*caption_low*/, const wchar_t* /*caption_mid*/, const wchar_t* /*caption_high*/,
																					 int /*min*/, int /*max*/, int /*position_min*/, int /*position_max*/, unsigned int /*page_index*/);
static fp_add_property_range_slider_type		fp_add_property_range_slider = 0;

// Add a coordinate system property. Set the caption and coordinate system.
// The coordinate system should be defined by OR-ing 3 coordinate system defines found above in this file.
// An example: (MAP_COORDSYS_X_POS_LEFT | MAP_COORDSYS_Y_POS_UP | MAP_COORDSYS_Z_POS_NEAR)
typedef void									(*fp_add_property_coordsys_type)(const wchar_t* /*caption*/, unsigned int /*coordinate_system*/, unsigned int /*page_index*/);
static fp_add_property_coordsys_type			fp_add_property_coordsys = 0;


// **
// Functions used in "on_process()" to get property values.
// Each of these functions requires the map id and filter position which is sent to "on_process()" in the struct "process_data_s".
// The property index must match the index as is defined by the order it was added to the plugin. See the above example of adding property controls.
														
// Get page list (if used) will always be at property index 0 (zero)
typedef unsigned int							(*fp_get_property_pagelist_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/);
static fp_get_property_pagelist_type			fp_get_property_pagelist = 0;

// Get a file path from a file property.
typedef const wchar_t*							(*fp_get_property_file_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/);
static fp_get_property_file_type				fp_get_property_file = 0;

// Get the state of a checkbox property.
typedef BOOL									(*fp_get_property_checkbox_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/);
static fp_get_property_checkbox_type			fp_get_property_checkbox = 0;

// Get the selected index of a list property.
typedef unsigned int							(*fp_get_property_list_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/);
static fp_get_property_list_type				fp_get_property_list = 0;

// Get the integer value of a numberbox property.
typedef int										(*fp_get_property_numberbox_int_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/);
static fp_get_property_numberbox_int_type		fp_get_property_numberbox_int = 0;

// Get the floating point value of a numberbox property.
typedef float									(*fp_get_property_numberbox_float_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/);
static fp_get_property_numberbox_float_type		fp_get_property_numberbox_float = 0;

// Get the color of a colorbox property.
typedef COLORREF								(*fp_get_property_colorbox_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/);
static fp_get_property_colorbox_type			fp_get_property_colorbox = 0;

// Get the integer position of a slider property.
typedef int										(*fp_get_property_slider_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/);
static fp_get_property_slider_type				fp_get_property_slider = 0;

// Get the integer positions of a range slider property as parameters.
typedef void									(*fp_get_property_range_slider_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/, int& /*position_min_out*/, int& /*position_max_out*/);
static fp_get_property_range_slider_type		fp_get_property_range_slider = 0;

// Get the coordinate system of a coordinate system property.
typedef unsigned int							(*fp_get_property_coordsys_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/);
static fp_get_property_coordsys_type			fp_get_property_coordsys = 0;


// **
// Functions to set property control values. Can be called during "on_process()".
// These functions are useful to override user input during processing.
// All functions require the map id, filter position, and property index.

// Set the state of a checkbox property.
typedef void									(*fp_set_property_checkbox_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/, BOOL /*check_state*/);
static fp_set_property_checkbox_type			fp_set_property_checkbox = 0;

// Set the selected index of a list property.
typedef void									(*fp_set_property_list_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/, unsigned int /*cur_sel*/);
static fp_set_property_list_type				fp_set_property_list = 0;

// Set an integer to a numberbox property.
typedef void									(*fp_set_property_numberbox_int_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/, int /*value*/);
static fp_set_property_numberbox_int_type		fp_set_property_numberbox_int = 0;

// Set a floating point value to a numberbox property.
typedef void									(*fp_set_property_numberbox_float_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/, float /*value*/);
static fp_set_property_numberbox_float_type		fp_set_property_numberbox_float = 0;

// Set a color to a colorbox property.
typedef void									(*fp_set_property_colorbox_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/, COLORREF /*color*/);
static fp_set_property_colorbox_type			fp_set_property_colorbox = 0;

// Set a position to a slider property.
typedef void									(*fp_set_property_slider_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/, int /*position*/);
static fp_set_property_slider_type				fp_set_property_slider = 0;

// Set a positions to a range slider property.
typedef void									(*fp_set_property_range_slider_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/, int /*position_min*/, int /*position_max*/);
static fp_set_property_range_slider_type		fp_set_property_range_slider = 0;

// Set a coordinate system to a coordinate system property.
typedef void									(*fp_set_property_coordsys_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*property_index*/, unsigned int /*coordsys*/);
static fp_set_property_coordsys_type			fp_set_property_coordsys = 0;


// **
// Utility functions using during processing in "on_process()".

// Determine if map render has been canceled - check often.
typedef BOOL									(*fp_is_cancel_process_type)(void);
static fp_is_cancel_process_type				fp_is_cancel_process = 0;

// Set the progress of processing - at minimum should call once at start with 0 and once at end with 100.
// Requires map id, filter position, and a progress integer between 0-100.
typedef void									(*fp_set_filter_progress_type)(unsigned int /*map_id*/, int /*filter_position*/, unsigned int /*progress*/);
static fp_set_filter_progress_type				fp_set_filter_progress = 0;

// Log a filter error to the ShaderMap log file located: "C:\Users\<USERNAME>\AppData\Roaming\SM3\log".
// Use the "LOG_ERROR_MSG()" macro to simplify calling this function.
typedef void									(*fp_log_filter_error_type)(unsigned int /*map_id*/, int /*filter_position*/, const wchar_t* /*error_message*/, const wchar_t* /*function*/, const wchar_t* /*source_filepath*/, int /*source_line_number*/);
static fp_log_filter_error_type					fp_log_filter_error = 0;

// Get the thread limit imposed by ShaderMap for map usage.
typedef unsigned int							(*fp_get_map_thread_limit_type)(void);
static fp_get_map_thread_limit_type				fp_get_map_thread_limit = 0;

// Get the map's mask data. Requires map id, and returns, as parameters, the width and height of the mask as well as a pixel array with the mask data.
// pixel_array_out must be allocated by the plugin and be of at least size: sizof(unsigned short) * with * height.
// pixel_array_out can be set to 0 and only the width and height are returned. In this way the developer can determine the size of the pixel array to allocate before making the second call.
// Masks are single channel images. Each unsigned short represents a single pixel value. The origin of the Mask image is UPPER LEFT
typedef void									(*fp_get_map_mask_type)(unsigned int /*map_id*/, unsigned int& /*width_out*/, unsigned int& /*height_out*/, unsigned short** /*pixel_array_out*/);
static fp_get_map_mask_type						fp_get_map_mask = 0;


// ----------------------------------------------------------------
// ----------------------------------------------------------------
// Local function prototypes. These must be implemented by the plugin DLL. They are called by 
// the exported functions defined at the bottom of this source code page.
												
// This function is called when ShaderMap is attaching to the plugin.
// "fp_begin_initialize()" and "fp_end_initialize()" are called inside of "on_initialize()".
// In between those functions, the "fp_set_plugin_info()" and add property functions should be called.
BOOL											on_initialize(void);

// Called when a plugin is to be processed (the filter is applied to the base image).
// The "process_data_s" struct contains the map info and pixels. 
// is_sRGB_out is a pointer to a boolean. The value of this property must be set. If TRUE the pixel data is in sRGB else in Linear color space.
// If the filter changes the color space of the pixels then the value of is_sRGB_out should be changed. Example: (*is_sRGB_out = FALSE);
BOOL											on_process(const process_data_s& data, BOOL* is_sRGB_out);

// Called before the plugin is released from ShaderMap at application shutdown. 
// Use this function to release/free any allocated resources.
BOOL											on_shutdown(void);

// This function is called each time a plugin's data is loaded from a project file. The purpose of this function is to assign data to the appropriate
// property index for the current version of the plugin. This allows the developer to add or remove property controls from version to version and
// ensure data from previous versions is loaded into the proper controls.
// The index_array[index_count] array will contain indices for each property value loaded from file. If the plugin has 3 property controls then
// the index_array[] == {0, 1, 2}.
// If in the next version you had inserted a control between 2 and 3 so that there are now 4 property controls then if the version == old version the
// developer should change the index_array[] == {0, 1, 3}. This way the third value loaded is placed in the 4th control at index 3.
/*
// Example:

	Version 101 has the follow controls - Since this is the first version nothing needs to be done inside "on_arrange_load_data()".
		Checkbox	0
		Slider		1
		Slider		2
	
	Version 102 adds a new property control between the slider controls.
		Checkbox	0
		Slider		1
		Colorbox	2
		Slider		3

	In the version 102 plugin the developer should write code that makes room for the new control when loading data from version 101 plugin.

	void on_arrange_load_data(unsigned int version, unsigned int index_count, unsigned int* index_array)
	{
		if(version < 102)
		{	
			// Shift all indices 2 and above up 1 index place
			for(unsigned int i=0; i<index_count; i++)
			{	if(index_array[i] >= 2)
				{	index_array[i]++;
				}
			}
		}
	}
*/
void											on_arrange_load_data(unsigned int version, unsigned int index_count, unsigned int* index_array);


// ----------------------------------------------------------------
// ----------------------------------------------------------------
// Definition of functions required by the plugin system. 

// These are functions called by ShaderMap. 
// "plugin_initialize()" sets the API function pointers then calls "on_initialize()".
// "plugin_process()", "plugin_shutdown()", and "plugin_custom_0()" each call the user defined "on_process()", "on_shutdown()", and "on_arrange_load_data()" functions.

#define DLL_EXPORT								__declspec(dllexport)

extern "C" {

	// Exported functions

	// Return SDK Version
	DLL_EXPORT void plugin_version(unsigned int& version_major_out, unsigned int& version_minor_out)
	{	
		version_major_out = SMSDK_VERSION_MAJOR;
		version_minor_out = SMSDK_VERSION_MINOR;
	}

	// Initialize the plugin with function pointers.
	DLL_EXPORT BOOL plugin_initialize(void** function_pointer_array)
	{	
		// Store function pointers - main app must know order and types.

		fp_begin_initialize						= (fp_begin_initialize_type)function_pointer_array[0];
		fp_end_initialize						= (fp_end_initialize_type)function_pointer_array[1];
		fp_define_translation_file				= (fp_define_translation_file_type)function_pointer_array[2];
		fp_get_trans_string						= (fp_get_trans_string_type)function_pointer_array[3];
		fp_define_help_file						= (fp_define_help_file_type)function_pointer_array[4];
		/*Elements 5 - 99 are reserved for future use*/

		fp_set_plugin_info						= (fp_set_plugin_info_type)function_pointer_array[100];
		fp_get_option_default_coord_sys			= (fp_get_option_default_coord_sys_type)function_pointer_array[101];
		/*Elements 102 - 199 are reserved for future use*/

		fp_add_property_pagelist				= (fp_add_property_pagelist_type)function_pointer_array[200];
		fp_add_property_file					= (fp_add_property_file_type)function_pointer_array[201];
		fp_add_property_checkbox				= (fp_add_property_checkbox_type)function_pointer_array[202];
		fp_add_property_list					= (fp_add_property_list_type)function_pointer_array[203];
		fp_add_property_numberbox_int			= (fp_add_property_numberbox_int_type)function_pointer_array[204];
		fp_add_property_numberbox_float			= (fp_add_property_numberbox_float_type)function_pointer_array[205];
		fp_add_property_colorbox				= (fp_add_property_colorbox_type)function_pointer_array[206];
		fp_add_property_slider					= (fp_add_property_slider_type)function_pointer_array[207];
		fp_add_property_range_slider			= (fp_add_property_range_slider_type)function_pointer_array[208];
		fp_add_property_coordsys				= (fp_add_property_coordsys_type)function_pointer_array[209];
		/*Elements 210 - 299 are reserved for future use*/

		fp_get_property_pagelist				= (fp_get_property_pagelist_type)function_pointer_array[300];
		fp_get_property_file					= (fp_get_property_file_type)function_pointer_array[301];
		fp_get_property_checkbox				= (fp_get_property_checkbox_type)function_pointer_array[302];
		fp_get_property_list					= (fp_get_property_list_type)function_pointer_array[303];
		fp_get_property_numberbox_int			= (fp_get_property_numberbox_int_type)function_pointer_array[304];
		fp_get_property_numberbox_float			= (fp_get_property_numberbox_float_type)function_pointer_array[305];
		fp_get_property_colorbox				= (fp_get_property_colorbox_type)function_pointer_array[306];
		fp_get_property_slider					= (fp_get_property_slider_type)function_pointer_array[307];
		fp_get_property_range_slider			= (fp_get_property_range_slider_type)function_pointer_array[308];
		fp_get_property_coordsys				= (fp_get_property_coordsys_type)function_pointer_array[309];
		fp_set_property_checkbox				= (fp_set_property_checkbox_type)function_pointer_array[310];
		fp_set_property_list					= (fp_set_property_list_type)function_pointer_array[311];
		fp_set_property_numberbox_int			= (fp_set_property_numberbox_int_type)function_pointer_array[312];
		fp_set_property_numberbox_float			= (fp_set_property_numberbox_float_type)function_pointer_array[313];
		fp_set_property_colorbox				= (fp_set_property_colorbox_type)function_pointer_array[314];
		fp_set_property_slider					= (fp_set_property_slider_type)function_pointer_array[315];
		fp_set_property_range_slider			= (fp_set_property_range_slider_type)function_pointer_array[316];
		fp_set_property_coordsys				= (fp_set_property_coordsys_type)function_pointer_array[317];
		/*Elements 318 - 399 are reserved for future use*/

		fp_is_cancel_process					= (fp_is_cancel_process_type)function_pointer_array[400];
		fp_set_filter_progress					= (fp_set_filter_progress_type)function_pointer_array[401];
		fp_log_filter_error						= (fp_log_filter_error_type)function_pointer_array[402];
		fp_get_map_thread_limit					= (fp_get_map_thread_limit_type)function_pointer_array[403];
		fp_get_map_mask							= (fp_get_map_mask_type)function_pointer_array[404];
		/*Elements 405 - 999 are reserved for future use*/

		return on_initialize();
	}

	// Process the plugin - param_0 (process_data_s), param_1 (is_sRGB_out pointer)
	DLL_EXPORT BOOL plugin_process(void* param_0, void* param_1)
	{	return on_process(*(process_data_s*)param_0, (BOOL*)param_1);
	}

	// Shutdown the plugin
	DLL_EXPORT BOOL plugin_shutdown(void)
	{	return on_shutdown();
	}

	// Arrange the load data
	DLL_EXPORT BOOL plugin_custom_0(void* param_0, void* param_1, void* param_2)
	{	on_arrange_load_data(*(unsigned int*)param_0, *(unsigned int*)param_1, (unsigned int*)param_2);
		return TRUE;
	}	
}
