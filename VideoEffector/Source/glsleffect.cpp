#include "glsleffect.h"
#include "SOIL.h"

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "jsonParser.h"

GLFWwindow* window; // (In the accompanying source code, this variable is global for simplicity)

GLuint LoadShaders( const char * vertex_file_path, const char * fragment_file_path );

int g_fps;

bool parse_json_data( char* json_file_path, char* layer, char* input_file_path, char* output_file_path, int* bImage )
{
	if( !readJson( json_file_path, layer, bImage ) )
		return false;

	std::string s( json_file_path );
	std::string ss = s.substr( 0, s.find_last_of( "\\/" ) );
	strcpy( input_file_path, ss.data() );

	std::string i( g_layerData->mediaPath );
	std::string si = ss + std::string("\\") +i;
	strcpy( input_file_path, si.data() );

	std::string o( g_layerData->outputFileName );
	std::string so = ss + std::string( "\\" ) + o;
	strcpy( output_file_path, so.data() );
}

int init_effect(int width, int height )
{

	if( !glfwInit() )
	{
		fprintf( stderr, "Glfw initial fail\n" );
		return -1;
	}

	glfwWindowHint( GLFW_VISIBLE, GLFW_FALSE );
	glfwWindowHint( GLFW_SAMPLES, 4 ); // 4x antialiasing
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 ); // We want OpenGL 3.3
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
	glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE ); // To make MacOS happy; should not be needed
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE ); // We don't want the old OpenGL 

																	 // Open a window and create its OpenGL context
	window = glfwCreateWindow( width, height, "VideoEffector", NULL, NULL );
	//glfwHideWindow( window );
	if( window == NULL )
	{
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent( window ); // Initialize GLEW
	glewExperimental = true; // Needed in core profile
	if( glewInit() != GLEW_OK )
	{
		fprintf( stderr, "Failed to initialize GLEW\n" );
		return -1;
	}
	return 0;
}

void process_effect_shader(int shaderNum, void* inData, int w, int h )
{
	EffectShader* shader = g_layerData->effectsChain.at( shaderNum )->glslData->description;
	// Create and compile our GLSL program from the shaders
	//GLuint programID = LoadShaders( "SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader" );
	GLuint programID = LoadShaders(shader->vertexShader, shader->fragmentShader );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	//SOIL_save_image( "indata_test.tga", SOIL_SAVE_TYPE_TGA, 2*w, h/2, 4, (unsigned char*)inData );

	// Use our shader
	glUseProgram( programID );

	for (int i = 0; i < shader->properties.size(); i++)
	{
		int propertyLocation = glGetUniformLocation( programID, shader->properties.at( i )->name );
		switch( shader->properties.at(i)->ptType )
		{
			case PROPERTY_TYPE_FLOAT:
				glUniform1f( propertyLocation, shader->properties.at( i )->value->f );
				break;
			case PROPERTY_TYPE_INT:
				glUniform1i( propertyLocation, shader->properties.at( i )->value->i );
				break;
			case PROPERTY_TYPE_VEC4:
				glUniform4f( propertyLocation, shader->properties.at( i )->value->v[0], shader->properties.at( i )->value->v[1],shader->properties.at( i )->value->v[2],shader->properties.at( i )->value->v[3] );
				break;
			default:
				break;
		}
	}


	GLuint VertexArrayID;
	glGenVertexArrays( 1, &VertexArrayID );
	glBindVertexArray( VertexArrayID );


	static const GLfloat g_vertex_buffer_data[] = {
		0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f,  0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		1.0f,  0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
	};

	static const GLfloat g_uv_buffer_data[] = {
		0.0, 1.0,
		1.0, 1.0,
		1.0,  0.0,
		0.0, 1.0,
		1.0,  0.0,
		0.0, 0.0,
	};
	// This will identify our vertex buffer
	GLuint vertexbuffer;
	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers( 1, &vertexbuffer );
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer( GL_ARRAY_BUFFER, vertexbuffer );
	// Give our vertices to OpenGL.
	glBufferData( GL_ARRAY_BUFFER, sizeof( g_vertex_buffer_data ), g_vertex_buffer_data, GL_STATIC_DRAW );

	GLuint uvbuffer;
	glGenBuffers( 1, &uvbuffer );
	glBindBuffer( GL_ARRAY_BUFFER, uvbuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( g_uv_buffer_data ), g_uv_buffer_data, GL_STATIC_DRAW );

	//////////////////////////////////////////////////////////////////////////
	//render target texture
	//The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	GLuint FramebufferName = 0;
	glGenFramebuffers( 1, &FramebufferName );
	glBindFramebuffer( GL_FRAMEBUFFER, FramebufferName );

	// The texture we're going to render to
	GLuint renderedTexture;
	glGenTextures( 1, &renderedTexture );
	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture( GL_TEXTURE_2D, renderedTexture );

	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0 );
	// Poor filtering. Needed !
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	// Set "renderedTexture" as our colour attachement #0
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0 );

	// Set the list of draw buffers.
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers( 1, DrawBuffers ); // "1" is the size of DrawBuffers

									 // Always check that our framebuffer is ok
	if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
		return;

	//////////////////////////////////////////////////////////////////////////
	// data -> txture
	GLuint textureID;
	glGenTextures( 1, &textureID );
	glActiveTexture( GL_TEXTURE0 );

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture( GL_TEXTURE_2D, textureID );

	// Give the image to OpenGL
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, inData );
	//glUniform1i( glGetUniformLocation( programID, g_layerData->effectsChain.at( shaderNum )->glslData->description->inputs.at( 0 ) ), 0 );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	// 
	// 	// Render to our framebuffer
	//glBindFramebuffer( GL_FRAMEBUFFER, FramebufferName );
	// 	glViewport( 0, 0, w, h ); // Render on the whole framebuffer, complete from the lower left corner to the upper right
	//	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	//////////////////////////////////////////////////////////////////////////
	//render 
	// 1st attribute buffer : vertices
	glEnableVertexAttribArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, vertexbuffer );
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : colors
	glEnableVertexAttribArray( 1 );
	glBindBuffer( GL_ARRAY_BUFFER, uvbuffer );
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		2,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	// Draw the triangle !
	glDrawArrays( GL_TRIANGLES, 0, 6 ); // Starting from vertex 0; 3 vertices total -> 1 triangle
	glDisableVertexAttribArray( 0 );

	//glDeleteProgram( programID );

	glfwSwapBuffers( window );
	glfwPollEvents();


	glBindTexture( GL_TEXTURE_2D, renderedTexture );
	glGetTexImage( GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, inData );
}

void process_effect_transparency( void* inData, int w, int h )
{
	EffectShader* shader = g_layerData->effectsChain.at( 0 )->glslData->description;
	// Create and compile our GLSL program from the shaders
	//GLuint programID = LoadShaders( "SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader" );
	GLuint programID = LoadShaders( "#version 120\n       attribute vec2 a_position;\nattribute vec2 a_texCoord; \nvoid main() {\n    gl_Position = vec4(vec2(2.0,2.0)*a_position-vec2(1.0, 1.0), 0.0, 1.0); \n}", "#version 120\n        precision mediump float; \nvoid main()	{\n    gl_FragColor = vec4( 0.0 ); \n}" );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Use our shader
	glUseProgram( programID );


	GLuint VertexArrayID;
	glGenVertexArrays( 1, &VertexArrayID );
	glBindVertexArray( VertexArrayID );


	static const GLfloat g_vertex_buffer_data[] = {
		0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f,  0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		1.0f,  0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
	};

	static const GLfloat g_uv_buffer_data[] = {
		0.0, 1.0,
		1.0, 1.0,
		1.0,  0.0,
		0.0, 1.0,
		1.0,  0.0,
		0.0, 0.0,
	};
	// This will identify our vertex buffer
	GLuint vertexbuffer;
	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers( 1, &vertexbuffer );
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer( GL_ARRAY_BUFFER, vertexbuffer );
	// Give our vertices to OpenGL.
	glBufferData( GL_ARRAY_BUFFER, sizeof( g_vertex_buffer_data ), g_vertex_buffer_data, GL_STATIC_DRAW );

	GLuint uvbuffer;
	glGenBuffers( 1, &uvbuffer );
	glBindBuffer( GL_ARRAY_BUFFER, uvbuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( g_uv_buffer_data ), g_uv_buffer_data, GL_STATIC_DRAW );

	//////////////////////////////////////////////////////////////////////////
	//render target texture
	//The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	GLuint FramebufferName = 0;
	glGenFramebuffers( 1, &FramebufferName );
	glBindFramebuffer( GL_FRAMEBUFFER, FramebufferName );

	// The texture we're going to render to
	GLuint renderedTexture;
	glGenTextures( 1, &renderedTexture );

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture( GL_TEXTURE_2D, renderedTexture );

	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0 );

	// Poor filtering. Needed !
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	// Set "renderedTexture" as our colour attachement #0
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0 );

	// Set the list of draw buffers.
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers( 1, DrawBuffers ); // "1" is the size of DrawBuffers

									 // Always check that our framebuffer is ok
	if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
		return;

	//////////////////////////////////////////////////////////////////////////
	// data -> txture
	GLuint textureID;
	glGenTextures( 1, &textureID );
	glActiveTexture( GL_TEXTURE0 );

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture( GL_TEXTURE_2D, textureID );

	// Give the image to OpenGL
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, inData );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	// 
	// 	// Render to our framebuffer
	//glBindFramebuffer( GL_FRAMEBUFFER, FramebufferName );
	// 	glViewport( 0, 0, w, h ); // Render on the whole framebuffer, complete from the lower left corner to the upper right
	//	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	//////////////////////////////////////////////////////////////////////////
	//render 
	// 1st attribute buffer : vertices
	glEnableVertexAttribArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, vertexbuffer );
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : colors
	glEnableVertexAttribArray( 1 );
	glBindBuffer( GL_ARRAY_BUFFER, uvbuffer );
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		2,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	// Draw the triangle !
	glDrawArrays( GL_TRIANGLES, 0, 6 ); // Starting from vertex 0; 3 vertices total -> 1 triangle
	glDisableVertexAttribArray( 0 );

	glBindTexture( GL_TEXTURE_2D, renderedTexture );
	glGetTexImage( GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, inData );

	glfwSwapBuffers( window );
	glfwPollEvents();
}

void process_effect_intro( void* inData, int w, int h, float alpha )
{
	EffectShader* shader = g_layerData->transitions->intro->glslData;
	// Create and compile our GLSL program from the shaders
	//GLuint programID = LoadShaders( "SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader" );
	GLuint programID = LoadShaders( shader->vertexShader, shader->fragmentShader );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Use our shader
	glUseProgram( programID );

	for( int i = 0; i < shader->properties.size(); i++ )
	{
		int propertyLocation = glGetUniformLocation( programID, shader->properties.at( i )->name );
		switch( shader->properties.at( i )->ptType )
		{
			case PROPERTY_TYPE_FLOAT:
				//glUniform1f( propertyLocation, shader->properties.at( i )->value->f );
				glUniform1f( propertyLocation, alpha );
				break;
			case PROPERTY_TYPE_INT:
				glUniform1i( propertyLocation, shader->properties.at( i )->value->i );
				break;
			case PROPERTY_TYPE_VEC4:
				glUniform4f( propertyLocation, shader->properties.at( i )->value->v[0], shader->properties.at( i )->value->v[1], shader->properties.at( i )->value->v[2], shader->properties.at( i )->value->v[3] );
				break;
			default:
				break;
		}
	}


	GLuint VertexArrayID;
	glGenVertexArrays( 1, &VertexArrayID );
	glBindVertexArray( VertexArrayID );


	static const GLfloat g_vertex_buffer_data[] = {
		0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f,  0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		1.0f,  0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
	};

	static const GLfloat g_uv_buffer_data[] = {
		0.0, 1.0,
		1.0, 1.0,
		1.0,  0.0,
		0.0, 1.0,
		1.0,  0.0,
		0.0, 0.0,
	};
	// This will identify our vertex buffer
	GLuint vertexbuffer;
	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers( 1, &vertexbuffer );
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer( GL_ARRAY_BUFFER, vertexbuffer );
	// Give our vertices to OpenGL.
	glBufferData( GL_ARRAY_BUFFER, sizeof( g_vertex_buffer_data ), g_vertex_buffer_data, GL_STATIC_DRAW );

	GLuint uvbuffer;
	glGenBuffers( 1, &uvbuffer );
	glBindBuffer( GL_ARRAY_BUFFER, uvbuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( g_uv_buffer_data ), g_uv_buffer_data, GL_STATIC_DRAW );

	//////////////////////////////////////////////////////////////////////////
	//render target texture
	//The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	GLuint FramebufferName = 0;
	glGenFramebuffers( 1, &FramebufferName );
	glBindFramebuffer( GL_FRAMEBUFFER, FramebufferName );

	// The texture we're going to render to
	GLuint renderedTexture;
	glGenTextures( 1, &renderedTexture );

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture( GL_TEXTURE_2D, renderedTexture );

	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0 );

	// Poor filtering. Needed !
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	// Set "renderedTexture" as our colour attachement #0
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0 );

	// Set the list of draw buffers.
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers( 1, DrawBuffers ); // "1" is the size of DrawBuffers

									 // Always check that our framebuffer is ok
	if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
		return;

	//////////////////////////////////////////////////////////////////////////
	// data -> txture
	GLuint textureIDs[2];
	glGenTextures( 2, textureIDs );

	glActiveTexture( GL_TEXTURE0 );
	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture( GL_TEXTURE_2D, textureIDs[0] );

	// Give the image to OpenGL
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, inData );
	glUniform1i( glGetUniformLocation( programID, g_layerData->transitions->intro->glslData->inputs.at( 0 ) ), 0 );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	glActiveTexture( GL_TEXTURE1 );
	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture( GL_TEXTURE_2D, textureIDs[1] );
	int width, height;
	unsigned char* tranImg = SOIL_load_image( "1280x720-transparent.png", &width, &height, 0, SOIL_LOAD_RGBA );
	// Give the image to OpenGL
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, tranImg );
	SOIL_free_image_data( tranImg );
	glUniform1i( glGetUniformLocation( programID, g_layerData->transitions->intro->glslData->inputs.at( 1 ) ), 1 );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );


	//////////////////////////////////////////////////////////////////////////
	//render 
	// 1st attribute buffer : vertices
	glEnableVertexAttribArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, vertexbuffer );
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : colors
	glEnableVertexAttribArray( 1 );
	glBindBuffer( GL_ARRAY_BUFFER, uvbuffer );
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		2,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	// Draw the triangle !
	glDrawArrays( GL_TRIANGLES, 0, 6 ); // Starting from vertex 0; 3 vertices total -> 1 triangle
	glDisableVertexAttribArray( 0 );

	glBindTexture( GL_TEXTURE_2D, renderedTexture );
	glGetTexImage( GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, inData );

	glDeleteProgram( programID );

	glfwSwapBuffers( window );
	glfwPollEvents();
}

void process_effect_outro( void* inData, int w, int h,  float alpha )
{
	EffectShader* shader = g_layerData->transitions->outro->glslData;
	// Create and compile our GLSL program from the shaders
	//GLuint programID = LoadShaders( "SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader" );
	GLuint programID = LoadShaders( shader->vertexShader, shader->fragmentShader );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Use our shader
	glUseProgram( programID );

	for( int i = 0; i < shader->properties.size(); i++ )
	{
		int propertyLocation = glGetUniformLocation( programID, shader->properties.at( i )->name );
		switch( shader->properties.at( i )->ptType )
		{
			case PROPERTY_TYPE_FLOAT:
				glUniform1f( propertyLocation, alpha );
				//glUniform1f( propertyLocation, shader->properties.at( i )->value->f );
				break;
			case PROPERTY_TYPE_INT:
				glUniform1i( propertyLocation, shader->properties.at( i )->value->i );
				break;
			case PROPERTY_TYPE_VEC4:
				glUniform4f( propertyLocation, shader->properties.at( i )->value->v[0], shader->properties.at( i )->value->v[1], shader->properties.at( i )->value->v[2], shader->properties.at( i )->value->v[3] );
				break;
			default:
				break;
		}
	}

	GLuint VertexArrayID;
	glGenVertexArrays( 1, &VertexArrayID );
	glBindVertexArray( VertexArrayID );

	static const GLfloat g_vertex_buffer_data[] = {
		0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f,  0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		1.0f,  0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
	};

	static const GLfloat g_uv_buffer_data[] = {
		0.0, 1.0,
		1.0, 1.0,
		1.0,  0.0,
		0.0, 1.0,
		1.0,  0.0,
		0.0, 0.0,
	};
	// This will identify our vertex buffer
	GLuint vertexbuffer;
	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers( 1, &vertexbuffer );
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer( GL_ARRAY_BUFFER, vertexbuffer );
	// Give our vertices to OpenGL.
	glBufferData( GL_ARRAY_BUFFER, sizeof( g_vertex_buffer_data ), g_vertex_buffer_data, GL_STATIC_DRAW );

	GLuint uvbuffer;
	glGenBuffers( 1, &uvbuffer );
	glBindBuffer( GL_ARRAY_BUFFER, uvbuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( g_uv_buffer_data ), g_uv_buffer_data, GL_STATIC_DRAW );

	//////////////////////////////////////////////////////////////////////////
	//render target texture
	//The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	GLuint FramebufferName = 0;
	glGenFramebuffers( 1, &FramebufferName );
	glBindFramebuffer( GL_FRAMEBUFFER, FramebufferName );

	// The texture we're going to render to
	GLuint renderedTexture;
	glGenTextures( 1, &renderedTexture );

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture( GL_TEXTURE_2D, renderedTexture );

	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0 );

	// Poor filtering. Needed !
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	// Set "renderedTexture" as our colour attachement #0
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0 );

	// Set the list of draw buffers.
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers( 1, DrawBuffers ); // "1" is the size of DrawBuffers

									 // Always check that our framebuffer is ok
	if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
		return;

	//////////////////////////////////////////////////////////////////////////
	// data -> txture
	GLuint textureIDs[2];
	glGenTextures( 2, textureIDs );

	glActiveTexture( GL_TEXTURE0 );
	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture( GL_TEXTURE_2D, textureIDs[0] );

	// Give the image to OpenGL
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, inData );
	glUniform1i( glGetUniformLocation( programID, g_layerData->transitions->outro->glslData->inputs.at( 0 ) ), 0 );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	glActiveTexture( GL_TEXTURE1 );
	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture( GL_TEXTURE_2D, textureIDs[1] );
	int width, height;
	unsigned char* tranImg = SOIL_load_image( "1280x720-transparent.png", &width, &height, 0, SOIL_LOAD_RGBA );
	// Give the image to OpenGL
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, tranImg );
	SOIL_free_image_data( tranImg );
	glUniform1i( glGetUniformLocation( programID, g_layerData->transitions->outro->glslData->inputs.at( 1 ) ), 1 );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	// 
	// 	// Render to our framebuffer
	//glBindFramebuffer( GL_FRAMEBUFFER, FramebufferName );
	// 	glViewport( 0, 0, w, h ); // Render on the whole framebuffer, complete from the lower left corner to the upper right
	//	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	//////////////////////////////////////////////////////////////////////////
	//render 
	// 1st attribute buffer : vertices
	glEnableVertexAttribArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, vertexbuffer );
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : colors
	glEnableVertexAttribArray( 1 );
	glBindBuffer( GL_ARRAY_BUFFER, uvbuffer );
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		2,                                // size
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	// Draw the triangle !
	glDrawArrays( GL_TRIANGLES, 0, 6 ); // Starting from vertex 0; 3 vertices total -> 1 triangle
	glDisableVertexAttribArray( 0 );

	glBindTexture( GL_TEXTURE_2D, renderedTexture );
	glGetTexImage( GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, inData );

	glDeleteProgram( programID );

	glfwSwapBuffers( window );
	glfwPollEvents();
}

void process_effect( void *inData, int w, int h, int frameNumber )
{
	float currentTime = (float)frameNumber / (float)g_fps;
	printf( "currentTime: %f", currentTime );
	
	for (int i = 0; i < g_layerData->effectsChain.size(); i++)
	{
		//SOIL_save_image( "image_test.tga", SOIL_SAVE_TYPE_TGA, w, h, 4, (unsigned char*)inData );
		process_effect_shader( i, inData, w, h );
	}

	if( currentTime < g_layerData->timeOfEntry || currentTime > g_layerData->timeOfExit )
		process_effect_transparency( inData, w, h );
	else if( currentTime > g_layerData->timeOfEntry && g_layerData->transitions->intro && currentTime < g_layerData->transitions->intro->stopAt )
		process_effect_intro( inData, w, h, ( currentTime - g_layerData->timeOfEntry ) / ( g_layerData->transitions->intro->stopAt - g_layerData->timeOfEntry ) );
	else if( currentTime < g_layerData->timeOfExit && g_layerData->transitions->outro && currentTime > g_layerData->transitions->outro->startAt )
		process_effect_outro( inData, w, h,  ( g_layerData->timeOfExit - currentTime )/ ( g_layerData->timeOfExit - g_layerData->transitions->outro->startAt));

}


GLuint LoadShaders( const char * vertex_file_path, const char * fragment_file_path )
{
	EffectShader* shader = g_layerData->effectsChain.at( 0 )->glslData->description;
	// Create the shaders
	GLuint VertexShaderID = glCreateShader( GL_VERTEX_SHADER );
	GLuint FragmentShaderID = glCreateShader( GL_FRAGMENT_SHADER );

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	//printf( "Compiling shader : %s\n", vertex_file_path );
	
//	char const * VertexSourcePointer = VertexShaderCode.c_str();
	char const * VertexSourcePointer = vertex_file_path;

	glShaderSource( VertexShaderID, 1, &VertexSourcePointer, NULL );
	glCompileShader( VertexShaderID );

	// Check Vertex Shader
	glGetShaderiv( VertexShaderID, GL_COMPILE_STATUS, &Result );
	glGetShaderiv( VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength );
	if( InfoLogLength > 0 )
	{
		std::vector<char> VertexShaderErrorMessage( InfoLogLength + 1 );
		glGetShaderInfoLog( VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0] );
		printf( "%s\n", &VertexShaderErrorMessage[0] );
	}

	// Compile Fragment Shader
	//printf( "Compiling shader : %s\n", fragment_file_path );
	//char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	char const * FragmentSourcePointer = fragment_file_path;
	glShaderSource( FragmentShaderID, 1, &FragmentSourcePointer, NULL );
	glCompileShader( FragmentShaderID );

	// Check Fragment Shader
	glGetShaderiv( FragmentShaderID, GL_COMPILE_STATUS, &Result );
	glGetShaderiv( FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength );
	if( InfoLogLength > 0 )
	{
		std::vector<char> FragmentShaderErrorMessage( InfoLogLength + 1 );
		glGetShaderInfoLog( FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0] );
		printf( "%s\n", &FragmentShaderErrorMessage[0] );
	}

	// Link the program
	//printf( "Linking program\n" );
	GLuint ProgramID = glCreateProgram();
	glAttachShader( ProgramID, VertexShaderID );
	glAttachShader( ProgramID, FragmentShaderID );
	glLinkProgram( ProgramID );

	// Check the program
	glGetProgramiv( ProgramID, GL_LINK_STATUS, &Result );
	glGetProgramiv( ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength );
	if( InfoLogLength > 0 )
	{
		std::vector<char> ProgramErrorMessage( InfoLogLength + 1 );
		glGetProgramInfoLog( ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0] );
		printf( "%s\n", &ProgramErrorMessage[0] );
	}

	glDetachShader( ProgramID, VertexShaderID );
	glDetachShader( ProgramID, FragmentShaderID );

	glDeleteShader( VertexShaderID );
	glDeleteShader( FragmentShaderID );

	return ProgramID;
}

void generateFFmpegCommand( int* argCnt, char** arglist, int fps, int duration, char* iput_filepath, char* output_filepath )
{
	float s_duration = (float)duration / 1000000.f;
	float timeOfEntry = g_layerData->timeOfEntry;
	float timeOfExit = g_layerData->timeOfExit;
	float compositionDuration = g_CompositionData.compositionDuration;
	if( timeOfEntry < 0 )
	{
		timeOfEntry = 0;
		g_layerData->timeOfEntry = 0;
	}

	if( timeOfExit > compositionDuration )
	{
		timeOfExit = compositionDuration;
		g_layerData->timeOfExit = compositionDuration;
	}
	float blackStartDuration = timeOfEntry;
	float blackEndDuration = compositionDuration - timeOfExit;
	float blackVideoDuration = ( blackStartDuration> blackEndDuration ) ? ( blackStartDuration ) : ( blackEndDuration );
	if( blackVideoDuration == 0 )
		blackVideoDuration = 1;

	float videoDuration = timeOfExit - timeOfEntry;
	int repeatCnt = videoDuration / s_duration;
	float additioanalDuration = videoDuration - repeatCnt * s_duration;

	if(g_layerData->bImage)
	{
		*argCnt = 12;

		arglist[0] = "videoeffect.exe";

		arglist[1] = "-loop";

		arglist[2] = "1";

		arglist[3] = "-i";

		arglist[4] = iput_filepath;

		arglist[5] = "-t";
	
		arglist[6] = new char[256];
		sprintf( arglist[6], "%f", compositionDuration );
		
		arglist[7] = "-r";

		arglist[8] = "24";

		arglist[9] = "-s";

		arglist[10] = "1280x720";

 		arglist[11] = output_filepath;

		g_fps = 24;

		return;

	}
	*argCnt = 14;

 	arglist[0] = "videoeffect.exe";

	arglist[1] = "-f";

	arglist[2] = "lavfi";

	arglist[3] = "-i";

	arglist[4] = new char[256];
	sprintf( arglist[4], "color=c=black:s=1280x720:r=%d:d=%f", fps, blackVideoDuration );

	arglist[5] = "-i";

	arglist[6] = iput_filepath;

	arglist[7] = "-filter_complex";

	int concat = 1;
	std::string blackstart;
	std::string blackend;
	std::string remainvideo;
	std::string mainvideo1;
	std::string mainvideo2;
	std::string mainvideo3;
	std::string mainvideo4;
	std::string concatString;

	if( blackStartDuration > 0 )
	{
		char c[256];
		sprintf( c, "[0:v] trim=start_frame=1:end_frame=%d,scale=1280x720,setsar=1:1 [blackstart]; ", (int)blackStartDuration * fps );
		blackstart = std::string( c );
		mainvideo1 = std::string( "[blackstart]" );
	}

	if( blackEndDuration > 0 )
	{
		char c[256];
		sprintf( c, "[0:v] trim=start_frame=1:end_frame=%d [blackend]; ", (int)blackEndDuration * fps );
		blackend = std::string( c );
		mainvideo4 = std::string( "[blackend] " );
	}

	if( additioanalDuration > 0)
	{
		char c[256];
		sprintf( c, "[1:v] trim=start_frame=1:end_frame=%d [remainVideo]; ", (int)additioanalDuration * fps );
		remainvideo = std::string( c );
		mainvideo3 = std::string( "[remainVideo] " );
		sprintf( c, "concat=n=%d:v=1:a=0 [out]", repeatCnt + 1 );
		concatString = std::string( c );
	}
	else
	{
		char c[256];
		sprintf( c, "concat=n=%d:v=1:a=0 [out]", repeatCnt );
		concatString = std::string( c );
	}


	for (int i = 0; i < repeatCnt; i++)
	{
		mainvideo3 = std::string( "[1:v] " ) + mainvideo3;
	}

	std::string sumString = blackstart + blackend + remainvideo + mainvideo1 + mainvideo2 + mainvideo3 + mainvideo4 + concatString;
	int i = sumString.size();
	arglist[8] = new char[sumString.size()];
	strcpy( arglist[8], sumString.data() );

	arglist[9] = "-map";

	arglist[10] = "[out]";

	arglist[11] = "-s";

	arglist[12] = "1280x720";

	arglist[13] = output_filepath;
	g_fps = fps;
}


int getCompositeWidth()
{
	return g_CompositionData.compositionWidth;
}

int getCompositeHeight()
{
	return g_CompositionData.compositionHeight;
}