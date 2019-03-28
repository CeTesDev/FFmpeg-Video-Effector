#pragma once

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC int init_effect(int width, int height );
EXTERNC void process_effect( void *, int, int, int );
EXTERNC bool parse_json_data( char*,char*, char*, char*, int* );
EXTERNC void generateFFmpegCommand( int* argCnt, char** arglist, int fps, int duration, char* iput_filepath, char* output_filepath );
EXTERNC int getCompositeWidth();
EXTERNC int getCompositeHeight();

