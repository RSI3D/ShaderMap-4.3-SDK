/*
	===============================================================

	SHADERMAP MAP PLUGIN CORE SOURCE FILE

	ShaderMap Map Plugins are used to create map pixels. Maps can
	be of two main types, Source and Map. A Source Map is an image
	loaded from file and a Map type Map is created inside the
	plugin and may have inputs such as other images or 3d models.
	Example maps include: Ambient Occlusion from Displacement, 
	Normal Map from 3D Model. 

	Include this source code file in a Win32 DLL project. 
	#include "map_plugin_core.cpp". You should ensure that the 
	DLL extension is *.smp. It must be placed in the 
	"plugins/maps" folder found in the ShaderMap root directory.

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
#include <vector>
#include "windows.h"


// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Plugin required defines

// SDK Version
#define SMSDK_VERSION_MAJOR						4
#define SMSDK_VERSION_MINOR						3

// Gamma control
#define GAMMA									2.2f
#define APPLY_GAMMA(x)							(pow(x, 1.0f / GAMMA))				// Converts linear pixel channel to sRGB color space.
#define REMOVE_GAMMA(x)							(pow(x, 1.0f / (1.0f / GAMMA)))		// Converts sRGB pixel channel to linear color space.

// Map plugin types
#define	MAP_PLUGIN_TYPE_SOURCE					0
#define	MAP_PLUGIN_TYPE_MAP						1

// Map input types
#define MAP_INPUT_TYPE_MAP						0
#define MAP_INPUT_TYPE_MODEL					1
#define MAP_INPUT_TYPE_LIGHTSCAN				2 

// Tile types
#define MAP_TILE_NONE							0
#define MAP_TILE_X								1
#define MAP_TILE_Y								2
#define MAP_TILE_XY								3


/*	Save Types and Extensions with Pixel Formats
---------------------------------------------------------------------
Type							Extensions						Pixel formats

Windows Bitmap					.bmp							INDEX_8, RGB_8, RGBA_8
DirectDraw Surface				.dds							RGB_8 (DXT1), RGBA_8 (DXT3, DXT5)
Jpeg							(.jpg), .jpe, .jpeg				RGB_8
ZSoft PCX						.pcx							INDEX_8, RGB_8
Portable Network Graphics		.png							INDEX_8, RGB_8, RGBA_8, RGB_16, RGBA_16
Adobe PhotoShop					.psd							INDEX_8, RGB_8, RGB_16
Targa							.tga							INDEX_8, RGB_8, RGBA_8
TIF								(.tif), .tiff					RGB_8, RGBA_8, RGB_16, RGBA_16
EXR								.exr,							16F, RGB_16F, RGBA_16F, 32F, RGB_32F, RGBA_32F
High Dynamic Range				.hdr							RGB_32F

---------------------------------------------------------------------
*/
#define MAP_FORMAT_BMP_INDEX_8					0
#define MAP_FORMAT_BMP_RGB_8					1
#define MAP_FORMAT_BMP_RGBA_8					2

#define MAP_FORMAT_DDS_DXT1						3
#define MAP_FORMAT_DDS_DXT3						4
#define MAP_FORMAT_DDS_DXT5						5

#define MAP_FORMAT_JPEG							6

#define MAP_FORMAT_PCX_INDEX_8					7
#define MAP_FORMAT_PCX_RGB_8					8

#define MAP_FORMAT_PNG_INDEX_8					9
#define MAP_FORMAT_PNG_RGB_8					10
#define MAP_FORMAT_PNG_RGBA_8					11
#define MAP_FORMAT_PNG_RGB_16					12
#define MAP_FORMAT_PNG_RGBA_16					13

#define MAP_FORMAT_PSD_INDEX_8					14
#define MAP_FORMAT_PSD_RGB_8					15
#define MAP_FORMAT_PSD_RGB_16					16

#define MAP_FORMAT_TGA_INDEX_8					17
#define MAP_FORMAT_TGA_RGB_8					18
#define MAP_FORMAT_TGA_RGBA_8					19

#define MAP_FORMAT_TIF_RGB_8					20
#define MAP_FORMAT_TIF_RGBA_8					21
#define MAP_FORMAT_TIF_RGB_16					22
#define MAP_FORMAT_TIF_RGBA_16					23

#define MAP_FORMAT_EXR_16F						24
#define MAP_FORMAT_EXR_RGB_16F					25
#define MAP_FORMAT_EXR_RGBA_16F					26
#define MAP_FORMAT_EXR_32F						27
#define MAP_FORMAT_EXR_RGB_32F					28
#define MAP_FORMAT_EXR_RGBA_32F					29

#define MAP_FORMAT_HDR_RGB_32F					30

// Normal map coordinate system
// 3 should be OR-ed together to define a coordinate system.
#define MAP_COORDSYS_X_POS_LEFT					0x00000001
#define MAP_COORDSYS_X_POS_RIGHT				0x00000002
#define MAP_COORDSYS_Y_POS_UP					0x00000004
#define MAP_COORDSYS_Y_POS_DOWN					0x00000008
#define MAP_COORDSYS_Z_POS_NEAR					0x00000010
#define MAP_COORDSYS_Z_POS_FAR					0x00000020

// Cache Types
#define CACHE_TYPE_MAP							0
#define CACHE_TYPE_MODEL						1
#define CACHE_TYPE_CAGE							2
#define CACHE_TYPE_ANY							3

// UDIM Postfix Formats
#define UDIM_POSTFIX_NONE						0							// No postfix
#define UDIM_POSTFIX_ID							1							// Format: [IMAGE FILENAME]_[UDIM ID].[EXT]
#define UDIM_POSTFIX_UV							2							// Format: [IMAGE FILENAME]_U[U OFFSET]_V[V OFFSET].[EXT]

// Error define - a simple way to log errors to the ShaderMap log files located in: "C:\Users\<USERNAME>\AppData\Roaming\SM3\log"
// Example: LOG_ERROR_MSG(map_id, filter_position, _T("Error description");
#define											LOG_ERROR_MSG(map_id, error) mp_log_map_error(map_id, error, _T(__FUNCTION__), _T(__FILE__), __LINE__);


// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Plugin required structs

// A struct containing information about the map plugin. 
// It is passed to ShaderMap with "mp_set_plugin_info()".
struct map_plugin_info_s
{
	unsigned int								version;					// An integer representing the version of the plugin. Examples: 101, 102, 103.
	BOOL										is_legacy;					// If TRUE then this plugin is only available on projects that require it, it is not shown on the ShaderMap add node list.
	int											type;						// MAP_PLUGIN_TYPE_SOURCE or MAP_PLUGIN_TYPE_MAP
	BOOL										is_normal_map;				// If TRUE the pixels are stored as normalized vectors else as rasterized (0...1) in either linear or sRGB color space.
	BOOL										is_maintain_color_space;	// If TRUE then pixels are not converted between sRGB and linear color space when exporting to file. Example: Exporting a normal map to an 8 bit image.
	int											default_save_format;		// One of the MAP_FORMAT definitions.
	const wchar_t*								name;						// The displayed name of the map.
	const wchar_t*								description;				// A description shown to the user.
	const wchar_t*								default_suffix;				// Default suffix added to filenames
	const wchar_t*								thumb_filename;				// A thumbnail file displayed in the Add Node dialog. Image file must be placed in plugins/maps/thumbs/ in the ShaderMap directory.
	BOOL										is_using_input_filter;		// If TRUE then the input filter will be displayed in the map properties, it is expected that the plugin will get the values and apply them.

	// c()
	map_plugin_info_s::map_plugin_info_s(void)
	{
		version									= 0;
		is_legacy								= FALSE;
		type									= MAP_PLUGIN_TYPE_MAP;
		is_normal_map							= FALSE;
		is_maintain_color_space					= FALSE;		
		default_save_format						= MAP_FORMAT_TGA_RGBA_8;
		name									= 0;
		description								= 0;
		default_suffix							= 0;
		thumb_filename							= 0;
		is_using_input_filter					= FALSE;
	}
};

// Struct containing an input filter's data
struct map_input_filter_data_s
{	
	float									r, y, g, c, b, m;				// Values should range from -200 to +300
	float									input_range[2];					// Range 0.0 to 1.0 - Low and High values
	float									output_range[2];				// Range 0.0 to 1.0 - Low and High values
	float									hue;							// Range -180 to 180
	float									saturation;						// Range -100 to 100

	// c()
	map_input_filter_data_s::map_input_filter_data_s(void)
	{	this->reset();
	}

	// Reset members
	void map_input_filter_data_s::reset(void)
	{	r = y = g = c = b = m = 100.0f;
		input_range[0]		= 0;
		input_range[1]		= 1;
		output_range[0]		= 0;
		output_range[1]		= 1;
		hue					= 0;
		saturation			= 0;
	}

	// Setup values for default color to grayscale conversion
	void map_input_filter_data_s::set_weights_for_convert_grayscale(void)
	{	r = 40.0f; y = 60.0f; g = 40.0f; c = 60.0f; b = 20.0f; m = 80.0f;
	}

	// Setup values for default color adjustment
	void map_input_filter_data_s::set_weights_for_adjust_color(void)
	{	r = y = g = c = b = m = 100.0f;
	}
};

// Struct contains information about the final created map.
// It is passed to "mp_create_map()" at end of "on_process()".
struct map_create_info_s
{
	unsigned int								width;						// Map with and height.
	unsigned int								height;						
	BOOL										is_grayscale;				// If true then 2 half floats per pixel else 4 half floats per pixel. 
	BOOL										is_sRGB;					// Pixels are in sRGB color space or linear color space (set to FALSE for normal maps).
	unsigned int								coord_system;				// Only set if map is a normal map. A combination of 3 OR-ed MAP_COORDSYS_XYZ values as defined.
	unsigned int								tile_type;					// One of the MAP_TILE definitions.
	const void*									pixel_array;				// Origin always UPPER LEFT, 2 or 4 half floats per channels depending on is_grayscale.
																			// Pixel array can be NULL if the purpose is to create the map but not copy pixel data into it.
																			// This array is made of half floating point values. If is_grayscale == TRUE then there are 2 half floats per pixel. If is_grayscale == FALSE then
																			// There are 4 half floats per pixel. 
																			// Grayscale is (Color, Alpha) in the range 0.0f - 1.0f.
																			// Color is (Red, Green, Blue, Alpha) in the range 0.0f - 1.0f. 
																			// If the map is_normal_map == TRUE then the half floats will be (X, Y, Z, Alpha) where X, Y, and Z are in the range -1.0f to 1.0f and the Alpha is 0.0f - 1.0f. 																			
	// c()
	map_create_info_s::map_create_info_s(void)
	{
		width									= 0;
		height									= 0;
		is_grayscale							= FALSE;
		is_sRGB									= FALSE;
		coord_system							= 0;
		tile_type								= 0;
		pixel_array								= 0;
	}
};

// A 2D vector used for 3d model inputs.
struct model_input_vector2_s
{	
	float										x;							// A position in 2d space.
	float										y;
};

// A 3D vector used for 3d model inputs.
struct model_input_vector3_s
{
	float										x;							// A position in 3d space.
	float										y;
	float										z;
};

// A vertex used for 3d model inputs.
struct model_input_vertex_s
{
	model_input_vector3_s						position;
	model_input_vector3_s						normal;	
};

// A tangent used for 3d model inputs.
struct model_input_tangent_s
{
	model_input_vector3_s						tangent;					// The XYZ components of the tangent.
	float										w;							// The W component contains the handedness of the tangent so that the Bi-Normal = dot_product(cross_product(N, T.xyz), T.w));
};

// A struct to get model input from ShaderMap using "mp_get_input_model()".
// Models are indexed triangles - UVs and tangents are in separate lists.
// The subset lookup table has 2 entries per subset, the first is the start index in index_array, 
// the second is the number of triangles in the subset.
struct model_input_data_s
{
	unsigned int								vertex_count;				// Number of vertices.
	unsigned int								uv_count;					// Number of texture coordinates.
	unsigned int								index_count;				// The number of triangle vertex indices.
	unsigned int								subset_count;				// Number of subsets in the 3d model.

	const model_input_vertex_s*					vertex_array;
	const model_input_vector2_s*				uv_array;					// An array of texture coordinates.
	const model_input_tangent_s*				tangent_array;				// 3 tangents per triangle or size (index_count / 7 * 3) - ordered to match triangle indices.
	const unsigned int*							index_array;				// 7 indices per triangle. Vertex ABC, UV ABC, and start index.
																			// which identifies where the triangle starts in index_array.

	const unsigned int*							subset_lookup_table;		// A table with 2 entries per subset. Values are (start_index, triangle_count [3 indices per triangle]).

	const unsigned int*							triangle_color_array;		// An array with 1 32 bit color per triangle. Will have index_count / 7 entries.

	// c()
	model_input_data_s::model_input_data_s(void)
	{	vertex_count = uv_count = index_count = subset_count = 0;
		vertex_array = 0; uv_array = 0; tangent_array = 0; index_array = 0;
		subset_lookup_table = 0; triangle_color_array = 0;
	}

	// Return if valid
	BOOL model_input_data_s::is_valid(void) const
	{
		if(vertex_count == 0 || uv_count == 0 || index_count == 0 || subset_count == 0)
		{	return FALSE;
		}
		if(vertex_array == 0 || uv_array == 0 || tangent_array == 0 || index_array == 0)
		{	return FALSE;
		}
		if(subset_lookup_table == 0 || triangle_color_array == 0)
		{	return FALSE;
		}
		return TRUE;
	}
};

// A struct to get light scan input from ShaderMap using "mp_get_input_light_scan()".
struct light_scan_input_data_s
{
	float										start_angle_degree;			// The angle (in degrees) of the first light scan image
	const wchar_t*								directory_path;				// The directory path where the images are located

	unsigned int								image_count;				// The number of image filenames in image_filename_list
	const wchar_t*								image_filename_list[64];	// An array of image filenames

	// c()
	light_scan_input_data_s::light_scan_input_data_s(void)
	{	start_angle_degree		= 0.0f;
		directory_path			= 0;
		image_count				= 0;
		ZeroMemory(image_filename_list, sizeof(wchar_t*) * 64);
	}
};


// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Definition of function types and pointers to functions

// **
// Setup and info functions

// Functions used in "on_initialize()" to start / stop initialization.
// These two functions should be called at start and end of "on_initialize()"
typedef void									(*mp_begin_initialize_type)(void);
mp_begin_initialize_type						mp_begin_initialize = 0;
typedef void									(*mp_end_initialize_type)(void);
mp_end_initialize_type							mp_end_initialize = 0;

// Function used to set the translation file to be used with this plugin
// This should be called in "on_initialize()" between "mp_begin_initialize()" and "mp_end_initialize()"
// An example: mp_define_translation_file(_T("my_plugin"), _T("en")); for a file [prefix].my_plugin.txt
// If a translation file for the current language is not set then the file with the default language prefix will be used.
// Returns the language file index which must be passed to mp_get_trans_string()
typedef unsigned int							(*mp_define_translation_file_type)(const wchar_t* /*file_title*/, const wchar_t* /*default_prefix*/);
mp_define_translation_file_type					mp_define_translation_file = 0;

// Function to get a string from the language translation file defined with mp_define_translation_file()
typedef const wchar_t*							(*mp_get_trans_string_type)(unsigned int /*file_index*/, unsigned int /*id*/);
mp_get_trans_string_type						mp_get_trans_string = 0;

// Function used to set the help file to be used with this plugin
// This should be called in "on_initialize()" between "mp_begin_initialize()" and "mp_end_initialize()"
// An example: mp_define_help_file(_T("plugins\\help_file.txt"), _T("en")); 
// If a help file for the current language is not found then the file with the default language will be used.
typedef void									(*mp_define_help_file_type)(const wchar_t* /*help_file*/, const wchar_t* /*default_language*/);
mp_define_help_file_type						mp_define_help_file = 0;

// Get the option for default tile type. Useful when setting up initial property control values.
typedef unsigned int							(*mp_get_option_default_tile_type_type)(void);
mp_get_option_default_tile_type_type			mp_get_option_default_tile_type = 0;

// Get the option for default Coordinate System. Useful when setting up initial property control values.
typedef unsigned int							(*mp_get_option_default_coord_sys_type)(void);
mp_get_option_default_coord_sys_type			mp_get_option_default_coord_sys = 0;

// Get the option for UDIM U Max. This value is the maximum number of horizontal tiles in a UDIM map.
typedef unsigned int							(*mp_get_option_udim_u_max_type)(void);
mp_get_option_udim_u_max_type					mp_get_option_udim_u_max = 0;

// Get the option for UDIM Postfix Format. It can be one of the following values: UDIM_POSTFIX_NONE, UDIM_POSTFIX_ID, or UDIM_POSTFIX_UV.
typedef unsigned int							(*mp_get_option_udim_postfix_format_type)(void);
mp_get_option_udim_postfix_format_type			mp_get_option_udim_postfix_format = 0;

// Define a map input ** Used by type MAP_PLUGIN_TYPE_MAP only
// Set input_type to MAP_INPUT_TYPE_ (MAP, MODEL, or LIGHTSCAN)
// If the input_type is MAP then set if is_input_filter_grayscale - this defines how the Map Input Filter interface displays the preview.
// If the input_type is MAP and will be a Normal Map then is_input_map_used_as_grayscale is ignored.
// If using the input filter then send a pointer to map_input_filter_data_s here, else send 0 (zero).
// This should be called in "on_initialize()" between "mp_begin_initialize()" and "mp_end_initialize()"
typedef void									(*mp_add_input_type)(const wchar_t* /*input_name*/, const wchar_t* /*input_description*/, int /*input_type*/, 
																	 BOOL /*is_input_filter_grayscale*/, map_input_filter_data_s* /*default_input_filter_data*/);
static mp_add_input_type						mp_add_input = 0;

// Define settings for an input filter used with a source map - Use only with maps that set is_using_input_filter to TRUE in the map_plugin_info_s.
// Set a map_input_filter_data_s pointer or 0 (zero) for base default values.
// This is only used with source maps.
typedef void									(*mp_setup_source_input_filter_type)(BOOL /*is_input_filter_grayscale*/, map_input_filter_data_s* /*default_input_filter_data*/);
static mp_setup_source_input_filter_type		mp_setup_source_input_filter = 0;

// Set the plugin info by passing a "filter_plugin_info_s" struct to ShaderMap.
// This should be called in "on_initialize()" between "mp_begin_initialize()" and "mp_end_initialize()"
typedef void									(*mp_set_plugin_info_type)(const map_plugin_info_s& /*plugin_info*/);
static mp_set_plugin_info_type					mp_set_plugin_info = 0;
												
// ** 
// Functions to add property controls to the map - added in order called - first will have index of 0 (zero).
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

// IMPORTANT - ShaderMap automatically adds 2 properties to every Map plugin. They are 2 checkboxes for Mask control. Even if no property controls are
// added by the user these 2 property controls will be added. The first checkbox controls if the Mask should be applied, the second checkbox 
// controls if the Mask should be inverted. The properties will be placed at the end of each property page and their indices will follow accordingly.
// See the examples included with this source code for a demonstration of how the auto added Mask properties can be accessed.

// The page list property (if used) must be the first property added, it defines the number of property pages - see above example.
typedef void									(*mp_add_property_pagelist_type)(const wchar_t* /*caption*/, const wchar_t** /*string_array*/, unsigned int /*string_count*/, unsigned int /*cur_select*/);
static mp_add_property_pagelist_type			mp_add_property_pagelist = 0;

// Add a file property. Set caption, an initial drive path, and extension filter. Allows the user to set a filepath control to the map.
typedef void									(*mp_add_property_file_type)(const wchar_t* /*caption*/, const wchar_t* /*initial_path*/, const wchar_t* /*extension_filter_pointer*/, unsigned int /*page_index*/);
static mp_add_property_file_type				mp_add_property_file = 0;

// Add a checkbox property. Set caption and initial check state.
typedef void									(*mp_add_property_checkbox_type)(const wchar_t* /*caption*/, BOOL /*is_checked*/, unsigned int /*page_index*/);
static mp_add_property_checkbox_type			mp_add_property_checkbox = 0;

// Add a list property. Set caption, string array and count, as well as initial selected item.
typedef void									(*mp_add_property_list_type)(const wchar_t* /*caption*/, const wchar_t** /*string_array*/, unsigned int /*string_count*/, unsigned int /*cur_select*/, unsigned int /*page_index*/);
static mp_add_property_list_type				mp_add_property_list = 0;

// Add integer numberbox property. Set the caption, min and max integers, and initial value.
typedef void									(*mp_add_property_numberbox_int_type)(const wchar_t* /*caption*/, int /*min*/, int /*max*/, int /*value*/, unsigned int /*page_index*/);
static mp_add_property_numberbox_int_type		mp_add_property_numberbox_int = 0;

// Add floating point numberbox property. Set the caption, min and max floats, and initial value.
typedef void									(*mp_add_property_numberbox_float_type)(const wchar_t* /*caption*/, float /*min*/, float /*max*/, float /*value*/, unsigned int /*page_index*/);
static mp_add_property_numberbox_float_type		mp_add_property_numberbox_float = 0;

// Add a colorbox property. Set the caption and initial color (use Windows RGB() macro).
typedef void									(*mp_add_property_colorbox_type)(const wchar_t* /*caption*/, COLORREF /*color*/, unsigned int /*page_index*/);
static mp_add_property_colorbox_type			mp_add_property_colorbox = 0;

// Add a slider property. Set the caption, min and max integers, initial position. Also can enable forced center to be at a set integer.
// Forced center can be useful, for example, when the min is -10 and the max is 100 but you want the center to be 0.
typedef void									(*mp_add_property_slider_type)(const wchar_t* /*caption*/, int /*min*/, int /*max*/, int /*position*/, unsigned int /*page_index*/, BOOL /*is_forced_center*/, int /*forced_center*/);
static mp_add_property_slider_type				mp_add_property_slider = 0;

// Add a range slider property. Set the 3 optional captions, min and max integers, initial min and max position.
typedef void									(*mp_add_property_range_slider_type)(const wchar_t* /*caption_low*/, const wchar_t* /*caption_mid*/, const wchar_t* /*caption_high*/,
																					 int /*min*/, int /*max*/, int /*position_min*/, int /*position_max*/, unsigned int /*page_index*/);
static mp_add_property_range_slider_type		mp_add_property_range_slider = 0;

// Add a coordinate system property. Set the caption and coordinate system.
// The coordinate system should be defined by OR-ing 3 coordinate system defines found above in this file.
// An example: (MAP_COORDSYS_X_POS_LEFT | MAP_COORDSYS_Y_POS_UP | MAP_COORDSYS_Z_POS_NEAR)
typedef void									(*mp_add_property_coordsys_type)(const wchar_t* /*caption*/, unsigned int /*coordinate_system*/, unsigned int /*page_index*/);
static mp_add_property_coordsys_type			mp_add_property_coordsys = 0;


// **
// Functions used only in "on_process()".
// Only use with map type MAP_PLUGIN_TYPE_SOURCE.
// Many of these functions require the map_id passed to "on_process()".

// Returns the source map width and height.
typedef unsigned int							(*mp_get_source_width_type)(unsigned int /*map_id*/);
static mp_get_source_width_type					mp_get_source_width = 0;
typedef unsigned int							(*mp_get_source_height_type)(unsigned int /*map_id*/);
static mp_get_source_height_type				mp_get_source_height = 0;

// Determine if the source pixels are in grayscale format (2 half floats per pixel vs 4 half floats per pixel).
typedef BOOL									(*mp_is_source_grayscale_type)(unsigned int /*map_id*/);
static mp_is_source_grayscale_type				mp_is_source_grayscale = 0;

// Returns if source pixel channels are all in the range 0.0f - 1.0f.
typedef BOOL									(*mp_is_source_rasterized_type)(unsigned int /*map_id*/);
static mp_is_source_rasterized_type				mp_is_source_rasterized = 0;

// Returns if source pixels are in sRGB or linear color space
typedef BOOL									(*mp_is_source_sRGB_type)(unsigned int /*map_id*/);
static mp_is_source_sRGB_type					mp_is_source_sRGB = 0;	

// Returns source pixels. 
// Pixels are 4 channels (color images RGBA or vector maps XYZA) or 2 channel (grayscale CA) format 16 bit half float per channel.
// Origin is always UPPER LEFT.
typedef const void*								(*mp_get_source_pixel_array_type)(unsigned int /*map_id*/);
static mp_get_source_pixel_array_type			mp_get_source_pixel_array = 0;


// **
// Functions used only in "on_process()".
// Only use with map type MAP_PLUGIN_TYPE_MAP.
// Many of these functions require the map_id passed to "on_process()" as well as the input index 
// which is a zero based index as id defined by the order the inputs were added to the map in "on_initialize()".

// Returns an input id given the input index. This can be used to identify the map_id of an input map.
typedef unsigned int							(*mp_get_input_id_type)(unsigned int /*map_id*/, unsigned int /*input_index*/);
static mp_get_input_id_type						mp_get_input_id = 0;

// -- 
// Map Type Input

// Returns the Map input's width and height.
typedef unsigned int							(*mp_get_input_width_type)(unsigned int /*map_id*/, unsigned int /*input_index*/);
static mp_get_input_width_type					mp_get_input_width = 0;
typedef unsigned int							(*mp_get_input_height_type)(unsigned int /*map_id*/, unsigned int /*input_index*/);
static mp_get_input_height_type					mp_get_input_height = 0;
typedef unsigned int							(*mp_get_input_coordsys_type)(unsigned int /*map_id*/, unsigned int /*input_index*/);

// Returns the Map input's coordinate system. Useful if the input is supposed to be a normal map.
static mp_get_input_coordsys_type				mp_get_input_coordsys = 0;
typedef unsigned int							(*mp_get_input_tile_type_type)(unsigned int /*map_id*/, unsigned int /*input_index*/);
static mp_get_input_tile_type_type				mp_get_input_tile_type = 0;

// Returns if an Map input's pixels are in grayscale format (2 half floats per pixel vs 4 half floats per pixel).
typedef BOOL									(*mp_is_input_grayscale_type)(unsigned int /*map_id*/, unsigned int /*input_index*/);
static mp_is_input_grayscale_type				mp_is_input_grayscale = 0;

// Returns if Map input's pixels are in sRGB or linear color space.
typedef BOOL									(*mp_is_input_sRGB_type)(unsigned int /*map_id*/, unsigned int /*input_index*/);
static mp_is_input_sRGB_type					mp_is_input_sRGB = 0;	

// Returns the Map input's pixels. 
// Pixels are 4 channels (color images RGBA or vector maps XYZA) or 2 channel (grayscale CA) format 16 bit half float per channel.
// Origin is always UPPER LEFT
typedef const void*								(*mp_get_input_pixel_array_type)(unsigned int /*map_id*/, unsigned int /*input_index*/);
static mp_get_input_pixel_array_type			mp_get_input_pixel_array = 0;

// --
// 3D Model Type Input

// Fills the 3D Model type input data parameter. 
// If is_cage == TRUE then the cage model data is returned else the base model data is set.
typedef void									(*mp_get_input_model_type)(unsigned int /*map_id*/, unsigned int /*input_index*/, BOOL /*is_cage*/, model_input_data_s& /*model_data_out*/);
static mp_get_input_model_type					mp_get_input_model = 0;

// Get a 3D Model input's subset list for a specific material id. subset_list_out must be an allocated array of at least subset_list_count_out. 
// If subset_list_out is NULL then only subset_list_count_out is returned.
// First call for getting the count, second for getting the list.
// If the material_id_in_out is invalid then the material_id_in_out 0 is returned which contains all subsets of the model.
typedef BOOL									(*mp_get_input_model_subset_list_type)(unsigned int /*map_id*/, unsigned int /*input_index*/, BOOL /*is_cage*/, unsigned int& /*material_id_in_out*/, unsigned int* /*subset_list_out*/, unsigned int* /*subset_list_count_out*/);
static mp_get_input_model_subset_list_type		mp_get_input_model_subset_list = 0;	

// Return if an input model has UVs loaded - used for error checking
typedef BOOL									(*mp_is_input_model_uvs_type)(unsigned int /*map_id*/, unsigned int /*input_index*/, BOOL /*is_cage*/);
static mp_is_input_model_uvs_type				mp_is_input_model_uvs = 0;

// --
// Light Scan Type Input

// Return a Light Scan input's data 
typedef void									(*mp_get_input_light_scan_type)(unsigned int /*map_id*/, unsigned int /*input_index*/, light_scan_input_data_s& /*light_scan_data_out*/);
static mp_get_input_light_scan_type				mp_get_input_light_scan = 0;


// --
// Input Filter Data

// Return an input's input filter data as a parameter
typedef void									(*mp_get_input_filter_data_type)(unsigned int /*map_id*/, unsigned int /*input_index*/, map_input_filter_data_s& /*input_filter_data_out*/);
static mp_get_input_filter_data_type			mp_get_input_filter_data = 0;

// Return an source map's input filter data as a parameter - used only with source map type maps.
typedef void									(*mp_get_source_input_filter_data_type)(unsigned int /*map_id*/, map_input_filter_data_s& /*input_filter_data_out*/);
static mp_get_source_input_filter_data_type		mp_get_source_input_filter_data = 0;

// **
// Functions for caching node data.

// Return if caching is enabled in the ShaderMap options.
typedef BOOL									(*mp_is_cache_enabled_type)(void);
static mp_is_cache_enabled_type					mp_is_cache_enabled = 0;

// Register data to the node cache registry. Data should be input specific and use a unique name to identify it.
// Registering data to the cache allows the read-only data to be shared across map plugins if they have access to the node id (input id).
// Data should be stored locally. It is not copied into the cache. The cache only stores a pointer to the data. 
// Be sure to delete data and update indices as needed in the plugin callbacks: on_node_cache_clear() and on_node_cache_clear_single().
// Function returns FALSE if either cache is disable, node_id is invalid, cache_type is invalid, or cache_name is already in use.
// Use mp_get_input_id() for node_id of an input, or use map_id for node_id of current map.
// data_size should be in bytes
typedef BOOL									(*mp_register_node_cache_type)(unsigned int /*node_id*/, unsigned int /*cache_type*/, const wchar_t* /*cache_name*/, const void* /*data_pointer*/, unsigned long long /*data_size*/);
static mp_register_node_cache_type				mp_register_node_cache = 0;

// Return a pointer to specific cached data that was stored with mp_register_node_cache.
// Returns 0 if node_id is invalid or cache_name was not found.
// Use this to check if data is already cached before registering it.
typedef const void*								(*mp_get_node_cache_type)(unsigned int /*node_id*/, const wchar_t* /*cache_name*/);
static mp_get_node_cache_type					mp_get_node_cache = 0;


// **
// Functions used in "on_process()" to get property values.
// Each of these functions requires the map id which is sent to "on_process()" as a parameter.
// The property index must match the index as is defined by the order it was added to the plugin. See the above example of adding property controls.
		
// Get page list (if used) will always be at property index 0 (zero)
typedef unsigned int							(*mp_get_property_pagelist_type)(unsigned int /*map_id*/, unsigned int /*property_index*/);
static mp_get_property_pagelist_type			mp_get_property_pagelist = 0;

// Get a file path from a file property.
typedef const wchar_t*							(*mp_get_property_file_type)(unsigned int /*map_id*/, unsigned int /*property_index*/);
static mp_get_property_file_type				mp_get_property_file = 0;

// Get the state of a checkbox property.
typedef BOOL									(*mp_get_property_checkbox_type)(unsigned int /*map_id*/, unsigned int /*property_index*/);
static mp_get_property_checkbox_type			mp_get_property_checkbox = 0;

// Get the selected index of a list property.
typedef unsigned int							(*mp_get_property_list_type)(unsigned int /*map_id*/, unsigned int /*property_index*/);
static mp_get_property_list_type				mp_get_property_list = 0;

// Get the integer value of a numberbox property.
typedef int										(*mp_get_property_numberbox_int_type)(unsigned int /*map_id*/, unsigned int /*property_index*/);
static mp_get_property_numberbox_int_type		mp_get_property_numberbox_int = 0;

// Get the floating point value of a numberbox property.
typedef float									(*mp_get_property_numberbox_float_type)(unsigned int /*map_id*/, unsigned int /*property_index*/);
static mp_get_property_numberbox_float_type		mp_get_property_numberbox_float = 0;

// Get the color of a colorbox property.
typedef COLORREF								(*mp_get_property_colorbox_type)(unsigned int /*map_id*/, unsigned int /*property_index*/);
static mp_get_property_colorbox_type			mp_get_property_colorbox = 0;

// Get the integer position of a slider property.
typedef int										(*mp_get_property_slider_type)(unsigned int /*map_id*/, unsigned int /*property_index*/);
static mp_get_property_slider_type				mp_get_property_slider = 0;

// Get the integer positions of a range slider property as parameters.
typedef void									(*mp_get_property_range_slider_type)(unsigned int /*map_id*/, unsigned int /*property_index*/, int& /*position_min_out*/, int& /*position_max_out*/);
static mp_get_property_range_slider_type		mp_get_property_range_slider = 0;

// Get the coordinate system of a coordinate system property.
typedef unsigned int							(*mp_get_property_coordsys_type)(unsigned int /*map_id*/, unsigned int /*property_index*/);
static mp_get_property_coordsys_type			mp_get_property_coordsys = 0;


// **
// Functions to set property control values. Can be called during "on_process()".
// These functions are useful to override user input during processing.
// All funtions require the map id and property index.

// Set the state of a checkbox property.
typedef void									(*mp_set_property_checkbox_type)(unsigned int /*map_id*/, unsigned int /*property_index*/, BOOL /*check_state*/);
static mp_set_property_checkbox_type			mp_set_property_checkbox = 0;

// Set the selected index of a list property.
typedef void									(*mp_set_property_list_type)(unsigned int /*map_id*/, unsigned int /*property_index*/, unsigned int /*cur_sel*/);
static mp_set_property_list_type				mp_set_property_list = 0;

// Set an integer to a numberbox property.
typedef void									(*mp_set_property_numberbox_int_type)(unsigned int /*map_id*/, unsigned int /*property_index*/, int /*value*/);
static mp_set_property_numberbox_int_type		mp_set_property_numberbox_int = 0;

// Set a floating point value to a numberbox property.
typedef void									(*mp_set_property_numberbox_float_type)(unsigned int /*map_id*/, unsigned int /*property_index*/, float /*value*/);
static mp_set_property_numberbox_float_type		mp_set_property_numberbox_float = 0;

// Set a color to a colorbox property.
typedef void									(*mp_set_property_colorbox_type)(unsigned int /*map_id*/, unsigned int /*property_index*/, COLORREF /*color*/);
static mp_set_property_colorbox_type			mp_set_property_colorbox = 0;

// Set a position to a slider property.
typedef void									(*mp_set_property_slider_type)(unsigned int /*map_id*/, unsigned int /*property_index*/, int /*position*/);
static mp_set_property_slider_type				mp_set_property_slider = 0;

// Set a positions to a range slider property.
typedef void									(*mp_set_property_range_slider_type)(unsigned int /*map_id*/, unsigned int /*property_index*/, int /*position_min*/, int /*position_max*/);
static mp_set_property_range_slider_type		mp_set_property_range_slider = 0;

// Set a coordinate system to a coordinate system property.
typedef void									(*mp_set_property_coordsys_type)(unsigned int /*map_id*/, unsigned int /*property_index*/, unsigned int /*coordsys*/);
static mp_set_property_coordsys_type			mp_set_property_coordsys = 0;


// **
// Utility functions using during processing in "on_process()".

// Determine if map render has been canceled - check often.
typedef BOOL									(*mp_is_cancel_process_type)(void);
static mp_is_cancel_process_type				mp_is_cancel_process = 0;

// Set the progress of processing - at minimum should call once at start with 0 and once at end with 100.
// Requires map id and a progress integer between 0-100.
typedef void									(*mp_set_map_progress_type)(unsigned int /*map_id*/, unsigned int /*progress*/);
static mp_set_map_progress_type					mp_set_map_progress = 0;

// Set the an animating progress of processing - The progress bar will be updated from min to max until the next call of mp_set_map_progress()
// Requires map id and min and max progress integers between 0-100.
typedef void									(*mp_set_map_progress_animation_type)(unsigned int /*map_id*/, unsigned int /*progress_min*/, unsigned int /*progress_max*/);
static mp_set_map_progress_animation_type		mp_set_map_progress_animation = 0;

// Log a filter error to the ShaderMap log file located: "C:\Users\<USERNAME>\AppData\Roaming\SM3\log".
// Use the "LOG_ERROR_MSG()" macro to simplify calling this function.
typedef void									(*mp_log_map_error_type)(unsigned int /*map_id*/, const wchar_t* /*error_message*/, const wchar_t* /*function*/, const wchar_t* /*source_filepath*/, int /*source_line_number*/);
static mp_log_map_error_type					mp_log_map_error = 0;

// Get the thread limit imposed by ShaderMap for map usage.
typedef unsigned int							(*mp_get_map_thread_limit_type)(void);
static mp_get_map_thread_limit_type				mp_get_map_thread_limit = 0;

// Set a status string which is displayed to the user in ShaderMap in the Map Preview section.
typedef void									(*mp_set_map_status_type)(unsigned int /*map_id*/, wchar_t* /*status_string*/);
static mp_set_map_status_type					mp_set_map_status = 0;

// Get the map's mask data. Requires map id, and returns, as parameters, the width and height of the mask as well as a pixel array with the mask data.
// pixel_array_out must be allocated by the plugin and be of at least size: sizof(unsigned short) * with * height.
// pixel_array_out can be set to 0 and only the width and height are returned. In this way the developer can determine the size of the pixel array to allocate before making the second call.
// Masks are single channel images. Each unsigned short represents a single pixel value. The origin of the Mask image is UPPER LEFT
typedef void									(*mp_get_map_mask_type)(unsigned int /*map_id*/, unsigned int& /*width_out*/, unsigned int& /*height_out*/, unsigned short** /*pixel_array_out*/);
static mp_get_map_mask_type						mp_get_map_mask = 0;

// Get / Set map output filename. The plugin can modify the filename if one exists. "mp_get_map_output_filename()" can return NULL (0) which means the output filename is not set.
// "mp_set_map_output_filename()" should not be called if "mp_get_map_output_filename()" returns 0.
typedef const wchar_t*							(*mp_get_map_output_filename_type)(unsigned int /*map_id*/);
static mp_get_map_output_filename_type			mp_get_map_output_filename = 0;
typedef void									(*mp_set_map_output_filename_type)(unsigned int /*map_id*/, const wchar_t* /*new_filename*/);
static mp_set_map_output_filename_type			mp_set_map_output_filename = 0;

// Create the final map. Map info is defined by settings in the "map_create_info_s" struct.
// If pixel_array_out is set then it will return a pointer to the map pixel data created. This is useful when you want to create the map at start of processing and
// use "mp_update_map_region()" to show a realtime progress of image creation. All maps from 3d model plugins that ship with ShaderMap use this method.
typedef BOOL									(*mp_create_map_type)(unsigned int /*map_id*/, const map_create_info_s& /*create_info*/, void** /*pixel_array_out*/);
static mp_create_map_type						mp_create_map = 0;

// Update the map pixels, created with "mp_create_map()", to the display textures in ShaderMap. Takes a RECT region on the image.
typedef void									(*mp_update_map_region_type)(unsigned int /*map_id*/, const RECT& /*region*/);
static mp_update_map_region_type				mp_update_map_region = 0;


// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Local function prototypes. These must be implemented by the plugin DLL. They are called by 
// the exported functions defined at the bottom of this source code page.
												
// This function is called when ShaderMap is attaching to the plugin.
// "fp_begin_initialize()" and "mp_end_initialize()" are called inside of "on_initialize()".
// In between those functions, the "mp_set_plugin_info()" and add property functions should be called.
BOOL											on_initialize(void);

// Called when a plugin is to be processed (rendered).
// The map_id is passed as a parameter and is needed for various function calls during "on_process()".
BOOL											on_process(unsigned int map_id);

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

// Called when an node has been removed from the project. 
// If the plugin is storing data by input ID then all stored input IDs > above_input_id should be subtracted by 1. 
void											on_input_id_change(unsigned int above_input_id);

// Called when either a node has been removed from the project or a part of it has changed.
// The type of clear is defined as type CACHE_TYPE_ANY, _MAP, _MODEL, or _CAGE. 
// Cached data with node_id and cache type should be released from memory.
void											on_node_cache_clear(unsigned int node_id, unsigned int type);

// Called when ShaderMap is deleting old cache entries. 
// Check local cache for matching data pointer, if found then free and remove that entry.
void											on_node_cache_clear_single(const void* data_pointer);


// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Definition of functions required by the plugin system. 

// These are functions called by ShaderMap. 
// "plugin_initialize()" sets the API function pointers then calls "on_initialize()".
// "plugin_process()", "plugin_shutdown()", and "plugin_custom_0()" each call the user defined "on_process()", "on_shutdown()", and "on_arrange_load_data()" functions.
// "plugin_custom_1()", "plugin_custom_2()", and "plugin_custom_3()" each call the user defined "on_input_id_change()", "on_input_cache_clear()", and "on_node_cache_clear_single()" functions.

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

		mp_begin_initialize						= (mp_begin_initialize_type)function_pointer_array[0];
		mp_end_initialize						= (mp_end_initialize_type)function_pointer_array[1];
		mp_define_translation_file				= (mp_define_translation_file_type)function_pointer_array[2];
		mp_get_trans_string						= (mp_get_trans_string_type)function_pointer_array[3];
		mp_define_help_file						= (mp_define_help_file_type)function_pointer_array[4];
		/*Elements 5 - 99 are reserved for future use*/

		mp_get_option_default_tile_type			= (mp_get_option_default_tile_type_type)function_pointer_array[100];
		mp_get_option_default_coord_sys			= (mp_get_option_default_coord_sys_type)function_pointer_array[101];
		mp_get_option_udim_u_max				= (mp_get_option_udim_u_max_type)function_pointer_array[102];
		mp_get_option_udim_postfix_format		= (mp_get_option_udim_postfix_format_type)function_pointer_array[103];
		mp_add_input							= (mp_add_input_type)function_pointer_array[104];
		mp_setup_source_input_filter			= (mp_setup_source_input_filter_type)function_pointer_array[105];
		/*Elements 106 - 199 are reserved for future use*/

		mp_set_plugin_info						= (mp_set_plugin_info_type)function_pointer_array[200];
		/*Elements 201 - 299 are reserved for future use*/

		mp_add_property_pagelist				= (mp_add_property_pagelist_type)function_pointer_array[300];
		mp_add_property_file					= (mp_add_property_file_type)function_pointer_array[301];
		mp_add_property_checkbox				= (mp_add_property_checkbox_type)function_pointer_array[302];
		mp_add_property_list					= (mp_add_property_list_type)function_pointer_array[303];
		mp_add_property_numberbox_int			= (mp_add_property_numberbox_int_type)function_pointer_array[304];
		mp_add_property_numberbox_float			= (mp_add_property_numberbox_float_type)function_pointer_array[305];
		mp_add_property_colorbox				= (mp_add_property_colorbox_type)function_pointer_array[306];
		mp_add_property_slider					= (mp_add_property_slider_type)function_pointer_array[307];
		mp_add_property_range_slider			= (mp_add_property_range_slider_type)function_pointer_array[308];
		mp_add_property_coordsys				= (mp_add_property_coordsys_type)function_pointer_array[309];
		/*Elements 310 - 399 are reserved for future use*/

		mp_get_source_width						= (mp_get_source_width_type)function_pointer_array[400];
		mp_get_source_height					= (mp_get_source_height_type)function_pointer_array[401];
		mp_is_source_grayscale					= (mp_is_source_grayscale_type)function_pointer_array[402];
		mp_is_source_rasterized					= (mp_is_source_rasterized_type)function_pointer_array[403];
		mp_is_source_sRGB						= (mp_is_source_sRGB_type)function_pointer_array[404];
		mp_get_source_pixel_array				= (mp_get_source_pixel_array_type)function_pointer_array[405];
		/*Elements 406 - 499 are reserved for future use*/

		mp_get_input_id							= (mp_get_input_id_type)function_pointer_array[500];
		mp_get_input_width						= (mp_get_input_width_type)function_pointer_array[501];
		mp_get_input_height						= (mp_get_input_height_type)function_pointer_array[502];
		mp_get_input_coordsys					= (mp_get_input_coordsys_type)function_pointer_array[503];
		mp_get_input_tile_type					= (mp_get_input_tile_type_type)function_pointer_array[504];
		mp_is_input_grayscale					= (mp_is_input_grayscale_type)function_pointer_array[505];
		mp_is_input_sRGB						= (mp_is_input_sRGB_type)function_pointer_array[506];
		mp_get_input_pixel_array				= (mp_get_input_pixel_array_type)function_pointer_array[507];
		mp_get_input_model						= (mp_get_input_model_type)function_pointer_array[508];
		mp_get_input_model_subset_list			= (mp_get_input_model_subset_list_type)function_pointer_array[509];
		mp_is_cache_enabled						= (mp_is_cache_enabled_type)function_pointer_array[510];
		mp_register_node_cache					= (mp_register_node_cache_type)function_pointer_array[511];
		mp_get_node_cache						= (mp_get_node_cache_type)function_pointer_array[512];
		mp_is_input_model_uvs					= (mp_is_input_model_uvs_type)function_pointer_array[513];
		mp_get_input_light_scan					= (mp_get_input_light_scan_type)function_pointer_array[514];
		mp_get_input_filter_data				= (mp_get_input_filter_data_type)function_pointer_array[515];
		mp_get_source_input_filter_data			= (mp_get_source_input_filter_data_type)function_pointer_array[516];
		/*Elements 517 - 599 are reserved for future use*/

		mp_get_property_pagelist				= (mp_get_property_pagelist_type)function_pointer_array[600];
		mp_get_property_file					= (mp_get_property_file_type)function_pointer_array[601];
		mp_get_property_checkbox				= (mp_get_property_checkbox_type)function_pointer_array[602];
		mp_get_property_list					= (mp_get_property_list_type)function_pointer_array[603];
		mp_get_property_numberbox_int			= (mp_get_property_numberbox_int_type)function_pointer_array[604];
		mp_get_property_numberbox_float			= (mp_get_property_numberbox_float_type)function_pointer_array[605];
		mp_get_property_colorbox				= (mp_get_property_colorbox_type)function_pointer_array[606];
		mp_get_property_slider					= (mp_get_property_slider_type)function_pointer_array[607];
		mp_get_property_range_slider			= (mp_get_property_range_slider_type)function_pointer_array[608];
		mp_get_property_coordsys				= (mp_get_property_coordsys_type)function_pointer_array[609];
		mp_set_property_checkbox				= (mp_set_property_checkbox_type)function_pointer_array[610];
		mp_set_property_list					= (mp_set_property_list_type)function_pointer_array[611];
		mp_set_property_numberbox_int			= (mp_set_property_numberbox_int_type)function_pointer_array[612];
		mp_set_property_numberbox_float			= (mp_set_property_numberbox_float_type)function_pointer_array[613];
		mp_set_property_colorbox				= (mp_set_property_colorbox_type)function_pointer_array[614];
		mp_set_property_slider					= (mp_set_property_slider_type)function_pointer_array[615];
		mp_set_property_range_slider			= (mp_set_property_range_slider_type)function_pointer_array[616];
		mp_set_property_coordsys				= (mp_set_property_coordsys_type)function_pointer_array[617];
		/*Elements 618 - 699 are reserved for future use*/

		mp_is_cancel_process					= (mp_is_cancel_process_type)function_pointer_array[700];
		mp_set_map_progress						= (mp_set_map_progress_type)function_pointer_array[701];
		mp_set_map_progress_animation			= (mp_set_map_progress_animation_type)function_pointer_array[702];
		mp_log_map_error						= (mp_log_map_error_type)function_pointer_array[703];
		mp_get_map_thread_limit					= (mp_get_map_thread_limit_type)function_pointer_array[704];
		mp_set_map_status						= (mp_set_map_status_type)function_pointer_array[705];
		mp_get_map_mask							= (mp_get_map_mask_type)function_pointer_array[706];
		mp_get_map_output_filename				= (mp_get_map_output_filename_type)function_pointer_array[707];
		mp_set_map_output_filename				= (mp_set_map_output_filename_type)function_pointer_array[708];
		/*Elements 709 - 799 are reserved for future use*/

		mp_create_map							= (mp_create_map_type)function_pointer_array[800];
		mp_update_map_region					= (mp_update_map_region_type)function_pointer_array[801];
		/*Elements 802 - 999 are reserved for future use*/

		return on_initialize();
	}

	// Process the plugin - param_0 (map_id), param_1 (0)
	DLL_EXPORT BOOL plugin_process(void* param_0, void* param_1)
	{	return on_process(*(unsigned int*)param_0);
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

	// Input ID change
	DLL_EXPORT BOOL plugin_custom_1(void* param_0, void* param_1, void* param_2)
	{	on_input_id_change(*(unsigned int*)param_0);
		return TRUE;
	}

	// Input cache clear
	DLL_EXPORT BOOL plugin_custom_2(void* param_0, void* param_1, void* param_2)
	{	on_node_cache_clear(*(unsigned int*)param_0, *(unsigned int*)param_1);
		return TRUE;
	}

	// Input cache clear single
	DLL_EXPORT BOOL plugin_custom_3(void* param_0, void* param_1, void* param_2)
	{	on_node_cache_clear_single((const void*)param_0);
		return TRUE;
	}
}