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

	This project builds a geometry import plugin for ShaderMap 4.1.
	The plugin loads a custom 3d model file and sends it to 
	ShaderMap in one of two formats: render or node.

	All geometry import plugins have the extension .smg and are 
	stored in the ShaderMap installation directory at:
	"plugins\bin\geometry"

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
	copy /Y "$(OutDir)$(TargetName)$(TargetExt)" "C:\Users\Neil\Desktop\ShaderMap 4 x64\plugins\bin\geometry\$(TargetName)$(TargetExt)"	
	to something like:
	copy /Y "$(OutDir)$(TargetName)$(TargetExt)" "C:\Users\YOUR USERNAME\Desktop\ShaderMap 4 x64\plugins\bin\geometry\$(TargetName)$(TargetExt)"	

	Do this for both Debug and Release configurations.

	--

	* STEP 3: Update VS Debug Project Settings.

	Debugging -> Command

	Example - Change the following filepath:
	C:\Users\Neil\Desktop\ShaderMap 4 x86\bin\ShaderMap.exe	
	to something like:
	C:\Users\YOUR USERNAME\Desktop\ShaderMap 4 x86\bin\ShaderMap.exe	

	Do this for both Debug and Release configurations.
	
	--

	* STEP 4: Select a Visual Studio configuration based on your 
	system (Win32 or x64).

	Compile the project. After the project is complete the plugin
	binary *.SMG will be copied to the ShaderMap 4 working folder.	

	Press Ctrl+F5 will start ShaderMap 4 in the working directory.
	You can then load the file "teapot.custom" as a 3d model in
	both the Project Grid and in the Material Visualizer.

	"teapot.custom" is located in the SDK folder at:
	"geometry\examples\geo_custom\teapot.custom".

	When debugging you may get a "No Debugging Information" popup 
	saying that ShaderMap was not built with debug info. Just press
	to Continue.
	
	--

	!!! THINGS TO REMEMBER

	ShaderMap Geometry Plugins have a filename extension .SMG even 
	though they are DLL files.

	Be sure to use x64 or Win32 configurations depending on the
	version of ShaderMap 4 you have copied to the Working 
	Directory in Step 1.

	===============================================================
*/


// ----------------------------------------------------------------
// ----------------------------------------------------------------
// Plugin includes

#include "..\..\geo_plugin_core.cpp"
#include <string>
#include <vector>


// ------------------------------------------------------------------
// ------------------------------------------------------------------
// Local functions called during init, process, and shutdown

// Initialize plugin - called when plugin is attached to ShaderMap.
BOOL on_initialize(void)
{	
	// Local data
	const wchar_t*	ext_array[] = {_T("custom")};


	// Tell ShaderMap we are starting initialization.
	gp_begin_initialize();

		// Set file format name and extension list. 
		// For this project we will be loading a custom binary 3D file format.
#ifdef _DEBUG		
		gp_set_file_info(_T("Custom 3D File - DEBUG"), ext_array, 1);
#else
		gp_set_file_info(_T("Custom 3D File"), ext_array, 1);
#endif
	
	// Tell ShaderMap initialization is done.
	gp_end_initialize();
	
	return TRUE;
}

// Process plugin - called when plugin is asked by ShaderMap to import a 3D Model (geometry).
BOOL on_process(unsigned int plugin_index, const wchar_t* file_path)
{
	// Local structs
	struct vector_2_s
	{	float						x, y;
	};

	struct vector_3_s
	{	float						x, y, z;
	};

	struct vertex_s
	{	vector_3_s					position;
		vector_3_s					normal;
	};

	// Local data
	FILE*							fp;
	unsigned int					i, ui_0, ui_1, vertex_count, index_count, uv_count, face_count, geometry_type;
	unsigned int*					index_array;
	BOOL							is_success;
	vector_2_s*						uv_array;
	vertex_s*						vertex_array;
	
	gp_render_vertex_s				render_vertex;
	gp_render_face_s				render_face;
	std::vector<gp_render_vertex_s>	render_vertex_list;
	std::vector<gp_render_face_s>	render_face_list;
	
	std::vector<gp_node_vertex_s>	node_vertex_list;
	std::vector<gp_node_face_s>		node_face_list;	

	gp_node_uv_data_s				node_uv_data;
	
	
	// Get geometry type. This can be of type render or of type node. 
	// They each have a unique format for the geometry. See below.
	geometry_type = gp_get_geometry_type();

	// -----------------

	// This plugin is loading a CUSTOM file format that was designed for this example.
	// The CUSTOM file is a binary and contains the following data.
	/*	
		unsigned int vertex_count
		unsigned int uv_count
		unsigned int index_count
		vertex_s * vertex_count			// An array of vertex_s structs
		vector_2_s * uv_count			// An array of vector_2_s structs
		unsigned int * index_count		// An array of undinged int indices - 7 unsigned int per 1 vertex (3 for vertices, 3 for uv, and 1 for start index).
	*/

	// Set return value
	is_success		= TRUE;

	// Null arrays
	vertex_array	= 0;
	index_array		= 0;
	uv_array		= 0;

	// Open binary file for read.
	_wfopen_s(&fp, file_path, _T("rb"));
	if(!fp)
	{	LOG_ERROR_MSG(plugin_index, _T("Failed to open file at file_path."));
		is_success = FALSE;
		goto ON_PROCESS_CLEANUP;
	}

	// Read vertex count.
	fread(&vertex_count, sizeof(unsigned int), 1, fp);
	// Read uv count.
	fread(&uv_count, sizeof(unsigned int), 1, fp);
	// Read index count.
	fread(&index_count, sizeof(unsigned int), 1, fp);

	// Ensure none of the counts are zero.
	if(vertex_count == 0 || uv_count == 0 || index_count == 0)
	{	LOG_ERROR_MSG(plugin_index, _T("One of the geometry counts was zero."));
		fclose(fp);
		is_success = FALSE;
		goto ON_PROCESS_CLEANUP;
	}

	// Allocate local arrays to store the geometry data.
	vertex_array = new (std::nothrow) vertex_s[vertex_count];
	if(!vertex_array)
	{	LOG_ERROR_MSG(plugin_index, _T("Memory Allocation Error: Failed to allocate vertex_array."));
		fclose(fp);
		is_success = FALSE;
		goto ON_PROCESS_CLEANUP;
	}
	uv_array = new (std::nothrow) vector_2_s[uv_count];
	if(!uv_array)
	{	LOG_ERROR_MSG(plugin_index, _T("Memory Allocation Error: Failed to allocate uv_array."));		
		fclose(fp);
		is_success = FALSE;
		goto ON_PROCESS_CLEANUP;
	}
	index_array = new (std::nothrow) unsigned int[index_count];
	if(!index_array)
	{	LOG_ERROR_MSG(plugin_index, _T("Memory Allocation Error: Failed to allocate index_array."));		
		fclose(fp);
		is_success = FALSE;
		goto ON_PROCESS_CLEANUP;
	}

	// Read in the arrays
	fread(vertex_array, sizeof(vertex_s), vertex_count, fp);
	fread(uv_array, sizeof(vector_2_s), uv_count, fp);
	fread(index_array, sizeof(unsigned int), index_count, fp);

	// Close file.
	fclose(fp);

	// Get the face count
	face_count = index_count / 7;

	// -----------------

	// Now that the file data is loaded into arrays. Build the arrays that ShaderMap wants the 
	// data in depending on the requested geometry type.
	
	// GP_GEOMETRY_TYPE_RENDER
	// This format for geometry is used for 3d models in the material visualizer.
	if(geometry_type == GP_GEOMETRY_TYPE_RENDER)
	{
		// Build the render geometry lists.
		ui_0 = 0;
		for(i=0; i<index_count; i+=7)
		{
			// Face vertex A
			render_vertex.x		= vertex_array[index_array[i]].position.x;
			render_vertex.nx	= vertex_array[index_array[i]].normal.x;
			render_vertex.y		= vertex_array[index_array[i]].position.y;
			render_vertex.ny	= vertex_array[index_array[i]].normal.y;
			render_vertex.z		= vertex_array[index_array[i]].position.z;
			render_vertex.nz	= vertex_array[index_array[i]].normal.z;
			render_vertex.u		= uv_array[index_array[i+3]].x;
			render_vertex.v		= -uv_array[index_array[i+3]].y;

			render_vertex_list.push_back(render_vertex);
			render_face.a		= (unsigned int)render_vertex_list.size() - 1;

			// Face vertex B
			render_vertex.x		= vertex_array[index_array[i+1]].position.x;
			render_vertex.nx	= vertex_array[index_array[i+1]].normal.x;
			render_vertex.y		= vertex_array[index_array[i+1]].position.y;
			render_vertex.ny	= vertex_array[index_array[i+1]].normal.y;
			render_vertex.z		= vertex_array[index_array[i+1]].position.z;
			render_vertex.nz	= vertex_array[index_array[i+1]].normal.z;
			render_vertex.u		= uv_array[index_array[i+4]].x;
			render_vertex.v		= -uv_array[index_array[i+4]].y;

			render_vertex_list.push_back(render_vertex);
			render_face.b		= (unsigned int)render_vertex_list.size() - 1;

			// Face vertex C
			render_vertex.x		= vertex_array[index_array[i+2]].position.x;
			render_vertex.nx	= vertex_array[index_array[i+2]].normal.x;
			render_vertex.y		= vertex_array[index_array[i+2]].position.y;
			render_vertex.ny	= vertex_array[index_array[i+2]].normal.y;
			render_vertex.z		= vertex_array[index_array[i+2]].position.z;
			render_vertex.nz	= vertex_array[index_array[i+2]].normal.z;
			render_vertex.u		= uv_array[index_array[i+5]].x;
			render_vertex.v		= -uv_array[index_array[i+5]].y;

			render_vertex_list.push_back(render_vertex);
			render_face.c		= (unsigned int)render_vertex_list.size() - 1;
			
			render_face.subset_index	= 0;	// The CUSTOM format does not have subsets so all are subset zero.

			render_face_list.push_back(render_face);
		}

		// Send the render lists to ShaderMap. No additional UV arrays.
		if(!gp_create_render_geometry(render_vertex_list.data(), (unsigned int)render_vertex_list.size(), render_face_list.data(), (unsigned int)render_face_list.size(), 1, FALSE, 0, 0))
		{	LOG_ERROR_MSG(plugin_index, _T("Failed to create render geometry with gp_create_render_geometry."));
			is_success = FALSE;
			goto ON_PROCESS_CLEANUP;
		}
	}
	// GP_GEOMETRY_TYPE_NODE
	// This format for geometry is used for 3d model nodes in the project grid.
	else if(geometry_type == GP_GEOMETRY_TYPE_NODE)
	{

		// Create the node uv data struct

		// Allocate an array of gp_node_uv_s arrays - 1 for each uv channel. In this case we have only 1 uv channel.
		node_uv_data.uv_channels_array = new (std::nothrow) gp_node_uv_s*[1];
		if(!node_uv_data.uv_channels_array)
		{	LOG_ERROR_MSG(plugin_index, _T("Memory Allocation Error: Faile to allocate node_uv_data.uv_channels_array."));
			is_success = FALSE;
			goto ON_PROCESS_CLEANUP;
		}

		// Allocate an single array of gp_node_uv_s for the 1 uv channel.
		node_uv_data.uv_channels_array[0] = new (std::nothrow) gp_node_uv_s[uv_count];
		if(!node_uv_data.uv_channels_array[0])
		{	LOG_ERROR_MSG(plugin_index, _T("Memory Allocation Error: Faile to allocate node_uv_data.uv_channels_array[0]."));
			is_success = FALSE;
			goto ON_PROCESS_CLEANUP;
		}

		// Copy in the uv array
		memcpy(node_uv_data.uv_channels_array[0], uv_array, uv_count * sizeof(gp_node_uv_s));

		// Allocate an array of unsigned int arrays - 1 for each uv channel. In this case we have only 1 uv channel.
		node_uv_data.uv_indices_array = new (std::nothrow) unsigned int*[1];
		if(!node_uv_data.uv_indices_array)
		{	LOG_ERROR_MSG(plugin_index, _T("Memory Allocation Error: Faile to allocate node_uv_data.uv_indices_array."));
			is_success = FALSE;
			goto ON_PROCESS_CLEANUP;
		}

		// Allocate an single array of gp_node_uv_s for the 1 uv channel.
		node_uv_data.uv_indices_array[0] = new (std::nothrow) unsigned int[face_count * 3];
		if(!node_uv_data.uv_indices_array[0])
		{	LOG_ERROR_MSG(plugin_index, _T("Memory Allocation Error: Faile to allocate node_uv_data.uv_indices_array[0]."));
			is_success = FALSE;
			goto ON_PROCESS_CLEANUP;
		}

		// Allocate an array of unsigned int - 1 for each uv channel. In this case we have only 1 uv channel.
		node_uv_data.uv_count_array = new (std::nothrow) unsigned int[1];
		if(!node_uv_data.uv_count_array)
		{	LOG_ERROR_MSG(plugin_index, _T("Memory Allocation Error: Faile to allocate node_uv_data.uv_count_array."));
			is_success = FALSE;
			goto ON_PROCESS_CLEANUP;
		}

		node_uv_data.uv_count_array[0]	= uv_count;
		node_uv_data.uv_channel_count	= 1;		

		// Build the node geometry lists
		node_vertex_list.resize(vertex_count);		
		for(i=0; i<vertex_count; i++)
		{	node_vertex_list[i].x		= vertex_array[i].position.x;
			node_vertex_list[i].y		= vertex_array[i].position.y;
			node_vertex_list[i].z		= vertex_array[i].position.z;
			node_vertex_list[i].nx		= vertex_array[i].normal.x;
			node_vertex_list[i].ny		= vertex_array[i].normal.y;
			node_vertex_list[i].nz		= vertex_array[i].normal.z;
		}

		node_face_list.resize(face_count);
		ui_0	= 0;
		ui_1	= 0;
		for(i=0; i<index_count; i+=7)
		{	
			// Copy in the face list.
			node_face_list[ui_0].a		= index_array[i];
			node_face_list[ui_0].b		= index_array[i+1];
			node_face_list[ui_0].c		= index_array[i+2];			
			node_face_list[ui_0].subset_index = 0;			// The CUSTOM format does not have subsets so all are subset zero.
			ui_0++;

			// Copy in the uv indices
			node_uv_data.uv_indices_array[0][ui_1]	= index_array[i+3]; ui_1++;
			node_uv_data.uv_indices_array[0][ui_1]	= index_array[i+4]; ui_1++;
			node_uv_data.uv_indices_array[0][ui_1]	= index_array[i+5]; ui_1++;
		}
		
		// Send the node lists to ShaderMap.
		if(!gp_create_node_geometry(node_vertex_list.data(), vertex_count, node_face_list.data(), face_count, &node_uv_data, 1, FALSE))
		{	LOG_ERROR_MSG(plugin_index, _T("Failed to create node geometry with gp_create_node_geometry."));
			is_success = FALSE;
			goto ON_PROCESS_CLEANUP;
		}
	}	

ON_PROCESS_CLEANUP:

	// Cleanup loaded geometry arrays.
	delete [] vertex_array;
	vertex_array	= 0;
	delete [] uv_array;
	uv_array		= 0;
	delete [] index_array;
	index_array		= 0;
	
	// Clear lists.
	node_vertex_list.clear();
	node_face_list.clear();
		
	return is_success;
}

// Free any local plugin resources allocated - called by ShaderMap before detaching from the plugin.
BOOL on_shutdown(void)
{
	// Nothing to do.

	return TRUE;
}