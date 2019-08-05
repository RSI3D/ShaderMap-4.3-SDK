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

	This project builds a filter plugin for ShaderMap 4.1.
	The plugin takes map pixels from ShaderMap and modifies them 
	in a read/write pixel array. This specific filter allows the
	user to set the intensity of each channel in the map.

	All filter plugins have the extension .smf and are 
	stored in the ShaderMap installation directory at:
	"plugins\bin\filters"

	===============================================================
*/
/*
	===============================================================

	!!! IMPORTANT - SETUP YOUR SYSTEM FOR DEVELOPMENT

	You should have ShaderMap 4.1 installed on your system. This 
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
	copy /Y "$(OutDir)$(TargetName)$(TargetExt)" "C:\Users\Neil\Desktop\ShaderMap 4 x64\plugins\bin\filters\$(TargetName)$(TargetExt)"
	copy /Y "example_filter_rgba.png" "C:\Users\Neil\Desktop\ShaderMap 4 x64\plugins\bin\filters\thumbs\example_filter_rgba.png"
	to something like:
	copy /Y "$(OutDir)$(TargetName)$(TargetExt)" "C:\Users\YOUR USERNAME\Desktop\ShaderMap 4 x64\plugins\bin\filters\$(TargetName)$(TargetExt)"
	copy /Y "example_filter_rgba.png" "C:\Users\YOUR USERNAME\Desktop\ShaderMap 4 x64\plugins\bin\filters\thumbs\example_filter_rgba.png"

	Do this for both Debug and Release configurations.

	--

	* STEP 3: Copy the folder "_example_filter_rgba_sm4_projects"
	to the same directory as the ShaderMap 4 working folder.

	"_example_filter_rgba_sm4_projects" is located in the SDK 
	folder:	"filters\examples\filter_rgba".

	Example - Copy "_example_filter_rgba_sm4_projects" to the
	Desktop at: 
	"C:\Users\YOUR USERNAME\Desktop\_example_filter_rgba_sm4_projects"

	Inside the folder are 2 projects. One loads and uses the
	debug plugin and the other uses the release version. Other than
	that the ShaderMap project files are identical.

	--

	* STEP 4: Update VS Debug Project Settings.

	Debugging -> Command
	Debugging -> Command Arguments

	Example - Change the following filepaths:
	C:\Users\Neil\Desktop\ShaderMap 4 x86\bin\ShaderMap.exe
	"C:\Users\Neil\Desktop\_example_filter_rgba_sm4_projects\forest_d.smpx"
	to something like:
	C:\Users\YOUR USERNAME\Desktop\ShaderMap 4 x86\bin\ShaderMap.exe
	"C:\Users\YOUR USERNAME\Desktop\_example_filter_rgba_sm4_projects\forest_d.smpx"

	Do this for both Debug and Release configurations.
	
	--

	* STEP 5: Select a Visual Studio configuration based on your 
	system (Win32 or x64).

	Compile the project. After the project is complete the plugin
	binary *.SMF will be copied to the ShaderMap 4 working folder.
	Also the plugin thumbnail will be copied.

	Press Ctrl+F5 will start ShaderMap 4 in the working directory
	with the project file defined in the Command Arguments setting.

	When debugging you may get a "No Debugging Information" popup 
	saying that ShaderMap was not built with debug info. Just press
	to Continue.

	Once ShaderMap 4 is launched with the Forest image, go to the
	Filter Stack on the image and adjust the properties of the 
	Example Filter RGBA filter.

	--

	!!! THINGS TO REMEMBER

	ShaderMap Filters have a filename extension .SMF even though
	they are DLL files.

	Be sure to use x64 or Win32 configurations depending on the
	version of ShaderMap 4 you have copied to the Working 
	Directory in Step 1.

	--

	Forest Photograph by:
	Moyan Brenn
	https://www.flickr.com/photos/aigle_dore/

	===============================================================
*/


// ----------------------------------------------------------------
// ----------------------------------------------------------------
// Plugin includes

#include "..\..\filter_plugin_core.cpp"
#include <string>

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
// Helper function prototypes - defined at bottom of this source code page.

float							clamp_f(float v);
unsigned short*					resize_mask_pixels(unsigned short* mask_pixel_array, unsigned int mask_width, unsigned int mask_height, 
												   unsigned int new_width, unsigned int new_height);


// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Local functions called during init, process, and shutdown

// Initialize plugin - called when plugin is attached to ShaderMap.
BOOL on_initialize(void)
{	
	// Local data
	filter_plugin_info_s		plugin_info;
	

	// Tell ShaderMap we are starting initialization.
	fp_begin_initialize();

		// Send plugin info to ShaderMap
		plugin_info.version			= 101;															// Version integer

#ifdef _DEBUG
		plugin_info.name			= _T("Eample RGBA - DEBUG");									// Display name
#else
		plugin_info.name			= _T("Eample RGBA");											// Display name
#endif
		plugin_info.description		= _T("Control individual channels strengths in an image.");		// Display descroption.
		plugin_info.thumb_filename	= _T("example_filter_rgba.png");								// Thumbnail. This must be located in plugins/filters/thumbs/ in the ShaderMap directory. 
																									// In this project the thumbnail image is copied to the ShaderMap directory after each build. See above notes.
		fp_set_plugin_info(plugin_info);
			
		// Add properties															
		fp_add_property_slider(_T("Red"), -100, 100, 0, 0, FALSE, 0);				// 0
		fp_add_property_slider(_T("Green"), -100, 100, 0, 0, FALSE, 0);				// 1
		fp_add_property_slider(_T("Blue"), -100, 100, 0, 0, FALSE, 0);				// 2
		fp_add_property_slider(_T("Alpha"), -100, 100, 0, 0, FALSE, 0);				// 3

		// The following are mask properties that are added automatically to every filter.
		// AUTO PROPERTY: Checkbox Use Mask											// 4
		// AUTO PROPERTY: Checkbox Invert Mask										// 5

	// Tell ShaderMap initialization is done.
	fp_end_initialize();
	
	return TRUE;
}

// Process plugin - called when plugin is asked by ShaderMap to apply a filter to Map Pixels.
BOOL on_process(const process_data_s& data, BOOL* is_sRGB_out)
{
	// Local structs
	struct pixel_32_s
	{	half_float::half		c, a;
	};

	struct pixel_64_s
	{	half_float::half		r, g, b, a;
	};

	// Local data
	float						r, g, b, a;
	unsigned int				i, count_i, thread_limit, mask_width, mask_height;
	BOOL						is_use_mask, is_invert_mask;
	unsigned short*				mask_pixel_array, *local_mask_pixel_array;
	pixel_32_s*					pixel_array_32;
	pixel_64_s*					pixel_array_64;


	// Set filter progress.
	fp_set_filter_progress(data.map_id, data.filter_position, 0);	

	// -----------------

	// Exit early if the map is a normal map - only want to work on color and grayscale images here.
	if(data.is_normal_map && data.filter_position > 0)
	{	*is_sRGB_out = data.is_sRGB;
		fp_set_filter_progress(data.map_id, data.filter_position, 100);	
		return TRUE;
	}

	// -----------------

	// Get Map thread limit from ShaderMap. Would use this if processing map in multiple threads.
	// This filter does not use multithreading so this line is for demonstration only.
	thread_limit = fp_get_map_thread_limit();

	// -----------------

	// Get property values - pay special attention to the property index requested.	
	// Converting red, green, blue, and alpha to floating points with range -1.0f to 1.0f.
	r						= fp_get_property_slider(data.map_id, data.filter_position, 0) / 100.0f;
	g						= fp_get_property_slider(data.map_id, data.filter_position, 1) / 100.0f;
	b						= fp_get_property_slider(data.map_id, data.filter_position, 2) / 100.0f;
	a						= fp_get_property_slider(data.map_id, data.filter_position, 3) / 100.0f;
	// Don't forget to get the values from the auto added mask properties.
	is_use_mask				= fp_get_property_checkbox(data.map_id, data.filter_position, 4);
	is_invert_mask			= fp_get_property_checkbox(data.map_id, data.filter_position, 5);

	// Exit early if nothing to do
	if(r == 0 && g == 0 && b == 0 && a == 0)
	{	*is_sRGB_out = data.is_sRGB;
		fp_set_filter_progress(data.map_id, data.filter_position, 100);	
		return TRUE;
	}

	// -----------------

	// The local dynamic pixel array we use to store mask pixels in.
	local_mask_pixel_array = 0;

	// Get mask data if enabled	
	if(is_use_mask)
	{	
		// Get mask size and pixels from ShaderMap.
		fp_get_map_mask(data.map_id, mask_width, mask_height, &mask_pixel_array);

		// If no mask is set then disable mask usage.
		if(!mask_pixel_array)
		{	is_use_mask = FALSE;
		}
		else
		{
			// Create local copy of mask pixels.
			local_mask_pixel_array = new (std::nothrow) unsigned short[mask_width * mask_height];
			if(!local_mask_pixel_array)
			{	LOG_ERROR_MSG(data.map_id, data.filter_position, _T("Memory Allocation Error: Failed to allocate local_mask_pixel_array."));
				return FALSE;
			}
			memcpy(local_mask_pixel_array, mask_pixel_array, sizeof(unsigned short) * mask_width * mask_height);

			// Resize the mask to the map size if not already the same size.
			// Using a simple nearest neighbor scale.
			if(mask_width != data.map_width || mask_height != data.map_height)
			{
				// Resize mask - local_mask_pixel_array is released by the function and a new array is allocated and returned with scaled pixels.
				local_mask_pixel_array = resize_mask_pixels(local_mask_pixel_array, mask_width, mask_height, data.map_width, data.map_height);
				if(!local_mask_pixel_array)
				{	LOG_ERROR_MSG(data.map_id, data.filter_position, _T("Resize mask pixels failed. Most likely caused by a memory allocation error."));
					return FALSE;
				}
			}

			// Invert local (resized) mask if required.
			if(is_invert_mask)
			{	
				count_i = data.map_width * data.map_height;
				for(i=0; i<count_i; i++)
				{	local_mask_pixel_array[i] = USHRT_MAX - local_mask_pixel_array[i];
				}
			}
		}
	}

	// -----------------

	// Check for cancel.
	if(fp_is_cancel_process())
	{	if(local_mask_pixel_array)
		{	delete [] local_mask_pixel_array;
		}
		return FALSE;
	}

	// -----------------
	
	// If map is grayscale
	if(data.is_grayscale)
	{
		// Cast the map pixel array to pixel_32_s.
		pixel_array_32 = (pixel_32_s*)data.map_pixel_data;

		// If using mask
		if(is_use_mask && local_mask_pixel_array)
		{
			// For every map pixel
			count_i = data.map_width * data.map_height;
			for(i=0; i<count_i; i++)
			{
				// Add the channel modifiers to each channel - use red for grayscale color.
				// Multiply the channel modifier by the mask pixel converted to scalar.
				// Clamp to range 0.0f to 1.0f.
				pixel_array_32[i].c = clamp_f(pixel_array_32[i].c + (r * (local_mask_pixel_array[i] / (float)USHRT_MAX)));
				pixel_array_32[i].a = clamp_f(pixel_array_32[i].a + (a * (local_mask_pixel_array[i] / (float)USHRT_MAX)));
			}
		}
		// Else no mask
		else
		{
			// For every map pixel
			count_i = data.map_width * data.map_height;
			for(i=0; i<count_i; i++)
			{
				// Add the channel modifiers to each channel - use red for grayscale color.
				// Clamp to range 0.0f to 1.0f.
				pixel_array_32[i].c = clamp_f(pixel_array_32[i].c + r);
				pixel_array_32[i].a = clamp_f(pixel_array_32[i].a + a);
			}
		}
	}
	// Else map is RGBA
	else
	{
		// Cast the map pixel array to pixel_64_s.
		pixel_array_64 = (pixel_64_s*)data.map_pixel_data;

		// If using mask
		if(is_use_mask && local_mask_pixel_array)
		{
			// For every map pixel
			count_i = data.map_width * data.map_height;
			for(i=0; i<count_i; i++)
			{
				// Add the channel modifiers to each channel. 
				// Multiply the channel modifier by the mask pixel converted to scalar.
				// Clamp to range 0.0f to 1.0f.
				pixel_array_64[i].r = clamp_f(pixel_array_64[i].r + (r * (local_mask_pixel_array[i] / (float)USHRT_MAX)));
				pixel_array_64[i].g = clamp_f(pixel_array_64[i].g + (g * (local_mask_pixel_array[i] / (float)USHRT_MAX)));
				pixel_array_64[i].b = clamp_f(pixel_array_64[i].b + (b * (local_mask_pixel_array[i] / (float)USHRT_MAX)));
				pixel_array_64[i].a = clamp_f(pixel_array_64[i].a + (a * (local_mask_pixel_array[i] / (float)USHRT_MAX)));
			}
		}
		// Else no mask
		else
		{
			// For every map pixel
			count_i = data.map_width * data.map_height;
			for(i=0; i<count_i; i++)
			{
				// Add the channel modifiers to each channel.
				// Clamp to range 0.0f to 1.0f.
				pixel_array_64[i].r = clamp_f(pixel_array_64[i].r + r);
				pixel_array_64[i].g = clamp_f(pixel_array_64[i].g + g);
				pixel_array_64[i].b = clamp_f(pixel_array_64[i].b + b);
				pixel_array_64[i].a = clamp_f(pixel_array_64[i].a + a);
			}
		}
	}

	// -----------------

	// Check for cancel.
	if(fp_is_cancel_process())
	{	if(local_mask_pixel_array)
		{	delete [] local_mask_pixel_array;
		}
		return FALSE;
	}
	
	// -----------------

	// Set the output parameter of the color space the pixels are in. It was not chaged.
	*is_sRGB_out = data.is_sRGB;

	// -----------------

	// Set progress
	fp_set_filter_progress(data.map_id, data.filter_position, 100);	

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


// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Helper functions

// Clamp float to range 0.0f to 1.0f.
float clamp_f(float v)
{	
	if(v < 0.0f)
	{	return 0.0f;
	}
	if(v > 1.0f)
	{	return 1.0f;
	}
	return v;
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