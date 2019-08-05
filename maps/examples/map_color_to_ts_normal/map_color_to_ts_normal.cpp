/*
	===============================================================

	SHADERMAP SDK LICENSE
	
	The ShaderMap SDK is released under The MIT License (MIT)
	http://opensource.org/licenses/MIT

	Copyright (c) 2018 Rendering Systems Inc.
	
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

	This project builds a map plugin for ShaderMap 4.1. The plugin 
	uses a single map input type. The pixels from this input are
	used to create a tangent space normal map of the same size. It
	does this by simply converting rasterized (range 0 to 1)
	colors to normalized vectors (range -1 to 1). If the normal
	turns out to be in the negative Z direction then it is made to
	face positive Z in tangent space. This map is an example on
	how to add inputs, get data from inputs, and generate a map
	from input pixels.

	All map plugins have the extension .smp and are 
	stored in the ShaderMap installation directory at:
	"plugins\bin\maps"

	===============================================================
*/
/*
	===============================================================

	!!! IMPORTANT - SETUP YOUR SYSTEM FOR DEVELOPMENT

	You should have ShaderMap 4.1 installed on your system. This 
	Visual Studio project will copy a number of files to a 
	ShaderMap installation directory (Working Directory).

	--

	* STEP 1: Copy the ShaderMap 4.1 installation directory to your
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
	copy /Y "example_map_color_to_ts_normal.png" "C:\Users\Neil\Desktop\ShaderMap 4 x64\plugins\bin\maps\thumbs\example_map_color_to_ts_normal.png"
	to something like:
	copy /Y "$(OutDir)$(TargetName)$(TargetExt)" "C:\Users\YOUR USERNAME\Desktop\ShaderMap 4 x64\plugins\bin\maps\$(TargetName)$(TargetExt)"
	copy /Y "example_map_color_to_ts_normal.png" "C:\Users\YOUR USERNAME\Desktop\ShaderMap 4 x64\plugins\bin\maps\thumbs\example_map_color_to_ts_normal.png"

	Do this for both Debug and Release configurations.

	--

	* STEP 3: Copy the folder "_example_map_color_to_ts_normal_sm4_projects"
	to the same directory as the ShaderMap 4 working folder.

	"_example_map_color_to_ts_normal_sm4_projects" is located in the SDK 
	folder:	"maps\examples\map_color_to_ts_normal".

	Example - Copy "_example_map_color_to_ts_normal_sm4_projects" to the
	Desktop at: 
	"C:\Users\YOUR USERNAME\Desktop\_example_map_color_to_ts_normal_sm4_projects"

	Inside the folder are 2 projects. One loads and uses the
	debug plugin and the other uses the release version. Other than
	that the ShaderMap project files are identical.

	--

	* STEP 4: Update VS Debug Project Settings.

	Debugging -> Command
	Debugging -> Command Arguments

	Example - Change the following filepaths:
	C:\Users\Neil\Desktop\ShaderMap 4 x86\bin\ShaderMap.exe
	"C:\Users\Neil\Desktop\_example_map_color_to_ts_normal_sm4_projects\acrylic_rose_d.smpx"
	to something like:
	C:\Users\YOUR USERNAME\Desktop\ShaderMap 4 x86\bin\ShaderMap.exe
	"C:\Users\YOUR USERNAME\Desktop\_example_map_color_to_ts_normal_sm4_projects\acrylic_rose_d.smpx"

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
	also add this map to the project grid by opening the 
	"Add Node to Project" dialog and selecting 
	"Example Color to TS Normal" from the list.

	--

	!!! THINGS TO REMEMBER

	ShaderMap Maps have a filename extension .SMP even though
	they are DLL files.

	Be sure to use x64 or Win32 configurations depending on the
	version of ShaderMap 4 you have copied to the Working 
	Directory in Step 1.

	--

	Acrylic Rose Macro Photograph by:
	Nicolas Raymond
	https://www.flickr.com/photos/82955120@N05/
	
	===============================================================
*/


// ----------------------------------------------------------------
// ----------------------------------------------------------------
// Plugin includes

#include "..\..\map_plugin_core.cpp"
#include <vector>
#include <algorithm>
#include "assert.h"

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
// Local defines and structs

// Blending defines.
#define CHANNEL_BLEND_ALPHA_R32(B,L,O)		((float)(O * B + (1.0f - O) * L))
#define CHANNEL_BLEND_ALPHA_R_R32(B,L,F,O)	(CHANNEL_BLEND_ALPHA_R32(F(B, L), B, O))
#define CHANNEL_BLEND_NORMAL_R32(B,L)		((float)(L))

// A 3 element vector.
struct vector_3_s
{	float						x, y, z;
};


// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Helper function prototypes - defined at bottom of this source code page.

void							normalize_vector(vector_3_s& normal_in_out);

unsigned short*					resize_mask_pixels(unsigned short* mask_pixel_array, unsigned int mask_width, unsigned int mask_height, 
												   unsigned int new_width, unsigned int new_height);


// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Local functions called during init, process, and shutdown

// Initialize plugin - called when plugin is attached to ShaderMap.
BOOL on_initialize(void)
{
	// Local data
	map_plugin_info_s			plugin_info;
	

	// Tell app we are starting initialize
	mp_begin_initialize();

		// Send plugin info to ShaderMap
		plugin_info.version						= 101;												// Version integer
		plugin_info.type						= MAP_PLUGIN_TYPE_MAP;								// The map type - can be source or map. This is a map type map which means it
																									// generates a map from input maps or models though inputs are not required.
		plugin_info.default_save_format			= MAP_FORMAT_TGA_RGB_8;								// The default file format ShaderMap will use to export this map type. 
#ifdef _DEBUG
		plugin_info.name						= _T("Example Color to TS Normal - DEBUG");			// Display name
#else
		plugin_info.name						= _T("Example Color to TS Normal");					// Display name
#endif
		plugin_info.description					= _T("Converts colors in an image to normalized vectors.\n\nUses any map as an input.");	// Description of map.
		plugin_info.thumb_filename				= _T("example_map_color_to_ts_normal.png");			// Thumbnail. This must be located in plugins/maps/thumbs/ in the ShaderMap directory. 
																									// In this project the thumbnail image is copied to the ShaderMap directory after each build. See above notes.
		plugin_info.is_normal_map				= TRUE;												// Set to TRUE if this map is a normal map. It is in this case.
		plugin_info.is_maintain_color_space		= TRUE;												// Set to TRUE so that on export ShaderMap does not modify the color space. 
																									// Normals are in linear color space and should not be converted to sRGB.
		plugin_info.default_suffix				= _T("_NORM");										// The suffix for batch processing of maps.

		mp_set_plugin_info(plugin_info);

		// -----------------

		// Add input. A single color map input. 
		// Set the parameter "is_3d_model" to FALSE to tell ShaderMap the input is a map.
		mp_add_input(_T("Color Texture"), _T("A diffuse image such as a color image or texture."), FALSE);

		// -----------------

		// Add properties
		mp_add_property_slider(_T("Intensity: "), 0, 500, 100, 0, FALSE, 0);			// 0		// Will be converted to floating point multiplier in "on_process()" by / 100.0f.

		// The following are mask properties that are added automatically to every map type map.
		// AUTO PROPERTY: Use Mask														// 1
		// AUTO PROPERTY: Invert Mask													// 2

	// Tell app initialize was success - map is added
	mp_end_initialize();
	
	return TRUE;
}

// Process plugin - called when plugin is asked by ShaderMap to process Map Pixels.
BOOL on_process(unsigned int map_id)
{
	// Local structs
	struct pixel_64_s
	{	half_float::half		r, g, b, a;
	};

	// Local data
	unsigned int				i, count_i, width, height, tile_type, thread_limit, mask_width, mask_height;	
	float						intensity, opacity;
	BOOL						is_use_mask, is_invert_mask;
	unsigned short*				mask_pixel_array, *local_mask_pixel_array;
	pixel_64_s*					local_normal_map_pixels, *input_pixel_pointer;
	vector_3_s					v3_0;
	map_create_info_s			create_info;
	

	// Update map progress.
	mp_set_map_progress(map_id, 0);

	// -----------------

	// Get Map thread limit from ShaderMap. Would use this if processing map in multiple threads.
	// This map does not use multithreading so this line is for demonstration only.
	thread_limit				= mp_get_map_thread_limit();

	// -----------------

	// Ensure input map is not grayscale. We need RGBA format pixels.
	if(mp_is_input_grayscale(map_id, 0))
	{	LOG_ERROR_MSG(map_id, _T("Invalid input format. Grayscale images are not allowed."));
		return FALSE;
	}

	// -----------------
		
	// Get size of input map. Ensure we have valid size.
	width						= mp_get_input_width(map_id, 0);
	height						= mp_get_input_height(map_id, 0);
	if(!width || !height)
	{	LOG_ERROR_MSG(map_id, _T("Invalid input size. Width or height is zero."));
		return FALSE;
	}

	// -----------------
	
	// Get property values - pay special attention to the property index requested.	
	intensity					= mp_get_property_slider(map_id, 0) / 100.0f;		// Convert to floating point multiplier.
	// Don't forget to get the values from the auto added mask properties.
	is_use_mask					= mp_get_property_checkbox(map_id, 1);
	is_invert_mask				= mp_get_property_checkbox(map_id, 2);
		
	// -----------------

	// Get tile type of input. 
	tile_type					= mp_get_input_tile_type(map_id, 0);
	
	// Get pointer to input map's pixels.
	input_pixel_pointer			= (pixel_64_s*)mp_get_input_pixel_array(map_id, 0);

	// -----------------

	// Allocate source map normal map pixels. 
	local_normal_map_pixels = new (std::nothrow) pixel_64_s[width * height];
	if(!local_normal_map_pixels)
	{	LOG_ERROR_MSG(map_id, _T("Memory Allocation Error: Failed to allocate local_normal_map_pixels."));
		return FALSE;
	}

	// Copy the pixels from the input map into this array.
	memcpy_s(local_normal_map_pixels, sizeof(pixel_64_s) * width * height, input_pixel_pointer, sizeof(pixel_64_s) * width * height);	

	// -----------------

	// The local dynamic pixel array we use to store mask pixels in.
	local_mask_pixel_array = 0;

	// Get mask data if enabled	
	if(is_use_mask)
	{	
		// Get mask size and pixels from ShaderMap.
		mp_get_map_mask(map_id, mask_width, mask_height, &mask_pixel_array);

		// If mask is set to the map then apply the mask to the local_normal_map_pixels.
		if(mask_pixel_array)
		{
			// Create local copy of mask pixels.
			local_mask_pixel_array = new (std::nothrow) unsigned short[mask_width * mask_height];
			if(!local_mask_pixel_array)
			{	LOG_ERROR_MSG(map_id, _T("Memory Allocation Error: Failed to allocate local_mask_pixel_array."));
				delete [] local_normal_map_pixels;
				return FALSE;
			}
			memcpy(local_mask_pixel_array, mask_pixel_array, sizeof(unsigned short) * mask_width * mask_height);

			// Resize the mask to the input map size if not already the same size.
			// Using a simple nearest neighbor scale.
			if(mask_width != width || mask_height != height)
			{
				// Resize mask - local_mask_pixel_array is released by the function and a new array is allocated and returned with scaled pixels.
				local_mask_pixel_array = resize_mask_pixels(local_mask_pixel_array, mask_width, mask_height, width, height);
				if(!local_mask_pixel_array)
				{	LOG_ERROR_MSG(map_id, _T("Resize mask pixels failed. Most likely caused by a memory allocation error."));
					delete [] local_normal_map_pixels;
					return FALSE;
				}
			}

			// Invert local (resized) mask if required.
			if(is_invert_mask)
			{	
				count_i = width * height;
				for(i=0; i<count_i; i++)
				{	local_mask_pixel_array[i] = USHRT_MAX - local_mask_pixel_array[i];
				}
			}

			// For every pixel, blend the color (0.5f, 0.5f, 1.0f) to the "local_normal_map_pixels" array using inverted mask as blending weight.
			// This will cause darker mask pixels to be closer to (0.5f, 0.5f, 1.0f) which will result in an "up" vector when converted to normals below.
			count_i = width * height;
			for(i=0; i<count_i; i++)
			{	
				// Get mask opacity value for blending.
				opacity = 1.0f - (local_mask_pixel_array[i] / 65535.0f);

				// Ensure the Blue channel will result in a positive Z value when converted to normal, else invert the Blue channel.
				if(local_normal_map_pixels[i].b < 0.5f)
				{	local_normal_map_pixels[i].b = 1.0f - local_normal_map_pixels[i].b;
				}
				
				// Blend the color (0.5f, 0.5f, 1.0f) to the "local_normal_map_pixels" using the mask opacity value.
				local_normal_map_pixels[i].r = CHANNEL_BLEND_ALPHA_R_R32(local_normal_map_pixels[i].r, 0.5f, CHANNEL_BLEND_NORMAL_R32, opacity);
				local_normal_map_pixels[i].g = CHANNEL_BLEND_ALPHA_R_R32(local_normal_map_pixels[i].g, 0.5f, CHANNEL_BLEND_NORMAL_R32, opacity);
				local_normal_map_pixels[i].b = CHANNEL_BLEND_ALPHA_R_R32(local_normal_map_pixels[i].b, 1.0f, CHANNEL_BLEND_NORMAL_R32, opacity);
			}

			// Free mask pixels.
			delete [] local_mask_pixel_array;
			local_mask_pixel_array = 0;
		}
	}

	// -----------------

	// Update map progress.
	mp_set_map_progress(map_id, 50);

	// -----------------

	// For every pixel, convert the color to a normalized vector in tangent space.
	count_i = width * height;
	for(i=0; i<count_i; i++)
	{
		v3_0.x = (local_normal_map_pixels[i].r * 2.0f - 1.0f) * intensity;
		v3_0.y = (local_normal_map_pixels[i].g * 2.0f - 1.0f) * intensity;
		v3_0.z = (local_normal_map_pixels[i].b * 2.0f - 1.0f);

		if(v3_0.z < 0)
		{	v3_0.z = -v3_0.z;
		}
		
		normalize_vector(v3_0);

		local_normal_map_pixels[i].r = v3_0.x;
		local_normal_map_pixels[i].g = v3_0.y;
		local_normal_map_pixels[i].b = v3_0.z;
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
	create_info.tile_type		= tile_type;								// The tile type from the input.
	create_info.coord_system	= MAP_COORDSYS_X_POS_RIGHT | MAP_COORDSYS_Y_POS_DOWN | MAP_COORDSYS_Z_POS_NEAR;	// The coordinate system of the normal map.
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

// Resize the mask pixels using nearest neighbor scaling
unsigned short* resize_mask_pixels(unsigned short* mask_pixel_array, unsigned int mask_width, unsigned int mask_height, 
								   unsigned int new_width, unsigned int new_height)
{
	// Local data
	unsigned int					i, j, x, y, x_delta, y_delta, index;;
	unsigned short*					resized_pixel_array, *source_line;


	// Check for same size and early exit.
	if(mask_width == new_width && mask_height != new_height)
	{	return mask_pixel_array;
	}

	// Allocate new size pixel array.
	resized_pixel_array = new (std::nothrow) unsigned short[new_width * new_height];
	if(!resized_pixel_array)
	{	return 0;
	}

	// Resize using nearest neighbor scaling.
	x_delta = (mask_width << 16) / new_width;
    y_delta = (mask_height << 16) / new_height;
	y		= 0;
	index	= 0;	
	for(j=0; j<new_height; j++)
	{
		source_line = &mask_pixel_array[(y >> 16) * mask_width];
		x			= 0;
		for(i=0; i<new_width; i++)
		{
			resized_pixel_array[index] = source_line[(x >> 16)];
			x += x_delta;
			index++;
		}
		y += y_delta;
	}

	// Free old mask pixel array.
	delete [] mask_pixel_array;
	mask_pixel_array = 0;

	// Return resized pixels.
	return resized_pixel_array;
}