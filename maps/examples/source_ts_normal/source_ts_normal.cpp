/*
	===============================================================

	SHADERMAP SDK LICENSE
	
	The ShaderMap SDK is released under The MIT License (MIT)
	http://opensource.org/licenses/MIT

	Copyright (c) 2017 Rendering Systems Inc.
	
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
/*
	===============================================================

	ABOUT:

	This project builds a source map plugin for ShaderMap 4.
	The plugin takes loaded normal map pixels (rasterized or in 
	vector form), applies normalization and adjustment to the 
	normals, then sends the tangent space normal map back to 
	ShaderMap.

	All map plugins have the extension .smp and are 
	stored in the ShaderMap installation directory at:
	"plugins\bin\maps"

	===============================================================
*/
/*
	===============================================================

	!!! IMPORTANT - SETUP YOUR SYSTEM FOR DEVELOPMENT

	You should have ShaderMap 4 installed on your system. This 
	Visual Studio project will copy a number of files to a 
	ShaderMap installation directory (Working Directory).

	--

	* STEP 1: Copy the ShaderMap 4 installation directory to your
	Desktop or somewhere else where Visual Studio can write to it
	without Administrator Privileges. This is the ShaderMap 
	Working	Directory.

	Example - Copy the installation folder found at:
	"C:\Program Files\ShaderMap 4" to the Desktop and rename it to 
	have x64 or x86 depending on your system:
	"‪C:\Users\Neil\Desktop\ShaderMap 4 x64"

	NOTE: If installed on a 64 bit system then the 32 bit version is located in the x86 directory
	Example - Copy the x86 folder found at:
	"C:\Program Files\ShaderMap 4\x86" to the Desktop and rename it to 
	"‪C:\Users\Neil\Desktop\ShaderMap 4 x86"

	--

	* STEP 2: Update the VS Build Events Project Settings.

	Build Events -> Post-Build Event.

	Change the filepath of the copy commands to the location where
	you copied the ShaderMap 4 folder.

	Example - Change the following filepaths:
	copy /Y "$(OutDir)$(TargetName)$(TargetExt)" "C:\Users\Neil\Desktop\ShaderMap 4 x64\plugins\bin\maps\$(TargetName)$(TargetExt)"
	copy /Y "example_source_ts_normal.png" "C:\Users\Neil\Desktop\ShaderMap 4 x64\plugins\bin\maps\thumbs\example_source_ts_normal.png"
	to something like:
	copy /Y "$(OutDir)$(TargetName)$(TargetExt)" "C:\Users\YOUR USERNAME\Desktop\ShaderMap 4 x64\plugins\bin\maps\$(TargetName)$(TargetExt)"
	copy /Y "example_source_ts_normal.png" "C:\Users\YOUR USERNAME\Desktop\ShaderMap 4 x64\plugins\bin\maps\thumbs\example_source_ts_normal.png"

	Do this for both Debug and Release configurations.

	--

	* STEP 3: Copy the folder "_example_source_ts_normal_sm4_projects"
	to the same directory as the ShaderMap 4 working folder.

	"_example_source_ts_normal_sm4_projects" is located in the SDK 
	folder:	"maps\examples\source_ts_normal".

	Example - Copy "_example_source_ts_normal_sm4_projects" to the
	Desktop at: 
	"C:\Users\YOUR USERNAME\Desktop\_example_source_ts_normal_sm4_projects"

	Inside the folder are 2 projects. One loads and uses the
	debug plugin and the other uses the release version. Other than
	that the ShaderMap project files are identical.

	--

	* STEP 4: Update VS Debug Project Settings.

	Debugging -> Command
	Debugging -> Command Arguments

	Example - Change the following filepaths:
	C:\Users\Neil\Desktop\ShaderMap 4 x86\bin\ShaderMap.exe
	"C:\Users\Neil\Desktop\_example_source_ts_normal_sm4_projects\normal_map_d.smpx"
	to something like:
	C:\Users\YOUR USERNAME\Desktop\ShaderMap 4 x86\bin\ShaderMap.exe
	"C:\Users\YOUR USERNAME\Desktop\_example_source_ts_normal_sm4_projects\normal_map_d.smpx"

	Do this for both Debug and Release configurations.
	
	--

	* STEP 5: Select a Visual Studio configuration based on your 
	system (Win32 or x64).

	Compile the project. After the project is complete the plugin
	binary *.SMP will be copied to the ShaderMap 4 working folder.
	Also the plugin thumbnail will be copied.

	Press Ctrl+F5 will start ShaderMap 4 in the working directory
	with the project file defined in the Command Arguments setting.

	When debugging you may get a "No Debugging Information" popup 
	saying that ShaderMap was not built with debug info. Just press
	to Continue.

	Once ShaderMap 4 is launched with the Normal Map image, simply
	adjust the property "Intensity" to change the normals. You can
	also add additional Normal Maps using this plugin by adding it
	to the project grid with the "Add Node to Project" dialog and
	selecting "Example TS Normal" from the list.

	--

	!!! THINGS TO REMEMBER

	ShaderMap Maps have a filename extension .SMP even though
	they are DLL files.

	Be sure to use x64 or Win32 configurations depending on the
	version of ShaderMap 4 you have copied to the Working 
	Directory in Step 1.
	
	===============================================================
*/



// ----------------------------------------------------------------
// ----------------------------------------------------------------
// Plugin includes

#include "..\..\map_plugin_core.cpp"

// Have to undefine Min and Max macros so they don't interfere with the half.hpp file
// These are redefined after the file is included
#undef min
#undef max

#include <float.h>
#include "half.hpp"

// Redefine min and max
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif


// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Local structs

struct vector_3_s
{	float						x, y, z;
};

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Helper function prototypes - defined at bottom of this source code page.

void							normalize_vector(vector_3_s& normal_in_out);


// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Local functions called during init, process, and shutdown

// Initialize plugin - called when plugin is attached to ShaderMap.
BOOL on_initialize(void)
{	
	// Local data
	map_plugin_info_s			plugin_info;
	unsigned int				default_tile_type, default_coord_sys;
	const wchar_t*				tile_list[] = { _T("None"), _T("On X"), _T("On Y"), _T("On XY") };


	// Tell app we are starting initialize
	mp_begin_initialize();

		// Send plugin info to ShaderMap
		plugin_info.version						= 101;												// Version integer
		plugin_info.type						= MAP_PLUGIN_TYPE_SOURCE;							// The map type - can be source or map. This is a source map which means it
																									// works similar to a filter. Pixels are retrieved from ShaderMap via "mp_get_source_pixel_array()".
		plugin_info.default_save_format			= MAP_FORMAT_TGA_RGB_8;								// The default file format ShaderMap will use to export this map type. 
#ifdef _DEBUG
		plugin_info.name						= _T("Example TS Normal - DEBUG");					// Display name
#else
		plugin_info.name						= _T("Example TS Normal");							// Display name
#endif
		plugin_info.description					= _T("A normal map contains normal vectors in tangent space.\n\nSource images are loaded from file.");	// Description of map.
		plugin_info.thumb_filename				= _T("example_source_ts_normal.png");				// Thumbnail. This must be located in plugins/maps/thumbs/ in the ShaderMap directory. 
																									// In this project the thumbnail image is copied to the ShaderMap directory after each build. See above notes.
		plugin_info.is_normal_map				= TRUE;												// Set to TRUE if this map is a normal map. It is in this case.
		plugin_info.is_maintain_color_space		= TRUE;												// Set to TRUE so that on export ShaderMap does not modify the color space. 
																									// Normals are in linear color space and should not be converted to sRGB.
		plugin_info.default_suffix				= _T("");											// The suffix for batch processing of maps. Source maps are not batched so leave blank.
		
		mp_set_plugin_info(plugin_info);

		// -----------------
				
		// Get the default tile type from the ShaderMap options.
		default_tile_type						= mp_get_option_default_tile_type();
		if(default_tile_type > MAP_TILE_XY)
		{	default_tile_type = MAP_TILE_NONE;
		}

		// Get the default coordinate system from the ShaderMap options.
		default_coord_sys						= mp_get_option_default_coord_sys();
		if(default_coord_sys == 0)
		{	default_coord_sys					= MAP_COORDSYS_X_POS_RIGHT | MAP_COORDSYS_Y_POS_DOWN | MAP_COORDSYS_Z_POS_NEAR;
		}

		// -----------------

		// Add properties
		mp_add_property_list(_T("Tile: "), tile_list, 4, default_tile_type, 0);		// 0		// Set list to the default_tile_type.
		mp_add_property_coordsys(_T("Coord System"), default_coord_sys, 0);				// 1		// Set coordinate system to default_coord_sys.
		mp_add_property_slider(_T("Intensity: "), 0, 500, 100, 0, FALSE, 0);			// 2		// Will be converted to floating point multiplier in "on_process()" by / 100.0f.
		
		// NOTE: ShaderMap does NOT auto add any properties for maps of type MAP_PLUGIN_TYPE_SOURCE.
		
	// Tell ShaderMap initialization is done.
	mp_end_initialize();
	
	return TRUE;
}

// Process plugin - called when plugin is asked by ShaderMap to process Source Map Pixels.
BOOL on_process(unsigned int map_id)
{
	// Local structs
	struct pixel_64_s
	{	half_float::half		r, g, b, a;
	};
	
	// Local data
	unsigned int				i, count_i, width, height, tile_type, coord_system, thread_limit;
	float						intensity;
	BOOL						is_rasterized;
	pixel_64_s*					local_normal_map_pixels;
	vector_3_s					v3_0;
	map_create_info_s			create_info;

	
	// Update map progress.
	mp_set_map_progress(map_id, 0);

	// -----------------

	// Ensure source map is not grayscale. We need RGBA format pixels.
	if(mp_is_source_grayscale(map_id))
	{	LOG_ERROR_MSG(map_id, _T("Invalid source format. Grayscale images are not allowed."));
		return FALSE;
	}

	// -----------------

	// Get size of source map. Ensure we have valid size.
	width						= mp_get_source_width(map_id);
	height						= mp_get_source_height(map_id);
	if(!width || !height)
	{	LOG_ERROR_MSG(map_id, _T("Invalid source size. Width or height is zero."));
		return FALSE;
	}

	// -----------------

	// Determine if the pixels are rasterized (0 - 1 rage) or in vector format.
	is_rasterized				= mp_is_source_rasterized(map_id);

	// -----------------

	// Get Map thread limit from ShaderMap. Would use this if processing map in multiple threads.
	// This map does not use multithreading so this line is for demonstration only.
	thread_limit				= mp_get_map_thread_limit();
	
	// -----------------

	// Get property values - pay special attention to the property index requested.	
	tile_type					= mp_get_property_list(map_id, 0);
	coord_system				= mp_get_property_coordsys(map_id, 1);
	intensity					= mp_get_property_slider(map_id, 2) / 100.0f;		// Convert to floating point multiplier.

	// -----------------

	// Allocate source map normal map pixels. 
	local_normal_map_pixels = new (std::nothrow) pixel_64_s[width * height];
	if(!local_normal_map_pixels)
	{	LOG_ERROR_MSG(map_id, _T("Memory Allocation Error: Failed to allocate local_normal_map_pixels."));
		return FALSE;
	}

	// Copy the pixels from ShaderMap into this array. Use "mp_get_source_pixel_array()".
	memcpy_s(local_normal_map_pixels, sizeof(pixel_64_s) * width * height, mp_get_source_pixel_array(map_id), sizeof(pixel_64_s) * width * height);	

	// -----------------

	// Update map progress.
	mp_set_map_progress(map_id, 25);

	// -----------------

	// If pixels are in range 0 to 1
	if(is_rasterized)
	{	
		// All maps of Normal Map type pixels are expected to be in normalized vector form.
		// For every pixel: Convert to vector range -1 to 1, apply intensity multiplier, and normalize.
		// Alpha value is untouched and should always be in rasterized range.
		count_i = width * height;
		for(i=0; i<count_i; i++)
		{
			v3_0.x = (local_normal_map_pixels[i].r * 2.0f - 1.0f) * intensity;
			v3_0.y = (local_normal_map_pixels[i].g * 2.0f - 1.0f) * intensity;
			v3_0.z = (local_normal_map_pixels[i].b * 2.0f - 1.0f);
			
			normalize_vector(v3_0);

			local_normal_map_pixels[i].r = v3_0.x;
			local_normal_map_pixels[i].g = v3_0.y;
			local_normal_map_pixels[i].b = v3_0.z;
		}
	}
	// Else already in normalized vector range.
	else
	{	
		// For every pixel: Apply intensity multiplier and normalize.
		// Alpha value is untouched and should always be in rasterized range, even when is_rasterized == FALSE.
		count_i = width * height;
		for(i=0; i<count_i; i++)
		{	
			v3_0.x = local_normal_map_pixels[i].r * intensity;
			v3_0.y = local_normal_map_pixels[i].g * intensity;
			v3_0.z = local_normal_map_pixels[i].b;

			normalize_vector(v3_0);

			local_normal_map_pixels[i].r = v3_0.x;
			local_normal_map_pixels[i].g = v3_0.y;
			local_normal_map_pixels[i].b = v3_0.z;
		}
	}

	// -----------------

	// Update map progress.
	mp_set_map_progress(map_id, 75);
	
	// -----------------
		
	// Check for cancel
	if(mp_is_cancel_process())
	{	delete [] local_normal_map_pixels;
		return FALSE;
	}

	// -----------------
	
	// Setup the create map info struct.
	create_info.width			= width;									// Size of source map.
	create_info.height			= height;
	create_info.is_grayscale	= FALSE;									// Not in grayscale.
	create_info.is_sRGB			= FALSE;									// Linear color space pixels.
	create_info.tile_type		= tile_type;								// The tile type from the property.
	create_info.coord_system	= coord_system;								// The coordinate system from the property.
	create_info.pixel_array		= (const void*)local_normal_map_pixels;		// The pixels.

	// Send the create_info struct / pixels to ShaderMap to create the map.
	if(!mp_create_map(map_id, create_info, 0))
	{	LOG_ERROR_MSG(map_id, _T("Failed to create map with mp_create_map()."));
		delete [] local_normal_map_pixels;
		return FALSE;
	}

	// -----------------

	// Cleanup
	delete [] local_normal_map_pixels;
	local_normal_map_pixels = 0;

	// -----------------

	// Update map progress.
	mp_set_map_progress(map_id, 100);	
	
	return TRUE;
}

// Free any local plugin resources allocated - called by ShaderMap before detaching from the plugin.
BOOL on_shutdown(void)
{		
	// Nothing to do.

	return TRUE;
}

// Arrange map data being loaded. Do this by index of properties. 
void on_arrange_load_data(unsigned int version, unsigned int index_count, unsigned int* index_array)
{
	// Nothing to do, no version control needed - all indices match original version 101 positions.
}

// Called when an node has been removed from the project. 
// Any data stored by input IDs > above_input_id should be subtracted by 1. 
void on_input_id_change(unsigned int above_input_id)
{
	// Nothing to do.
}

// Called when either a node has been removed from the project or a part of it has changed.
// The type of clear is defined in type (CACHE_TYPE_ANY, _MAP, _MODEL, or _CAGE).
void on_node_cache_clear(unsigned int input_id, unsigned int type)
{
	// Nothing to do.
}

// Called when ShaderMap is deleting old cache entries. 
// Check local cache for matching data pointer, if found free and remove that entry.
void on_node_cache_clear_single(const void* data_pointer)
{
	// Nothing to do.
}


// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Helper functions

// Normalize the parameter vector.
void normalize_vector(vector_3_s& normal_in_out)
{
	float t = sqrtf(normal_in_out.x * normal_in_out.x + 
					normal_in_out.y * normal_in_out.y + 
					normal_in_out.z * normal_in_out.z);
	if (t > 0.0f) 
	{	normal_in_out.x /= t; normal_in_out.y /= t; normal_in_out.z /= t;
	}
	else
	{	normal_in_out.x = normal_in_out.y = normal_in_out.z = 0.0f;
	}	
}