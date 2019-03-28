#pragma once

#include <fstream>
#include <json/reader.h>

union PropertyValue
{
	float f;
	float v[4];
	int i;
};

enum PropertyType
{
	PROPERTY_TYPE_INT,
	PROPERTY_TYPE_FLOAT,
	PROPERTY_TYPE_VEC4
};
struct ShaderProperty
{
	char* name;
	char* type;
	PropertyType ptType;
	PropertyValue* value;
}; 

struct EffectShader
{
	char* title;
	char* description;
	char* vertexShader;
	char* fragmentShader;
	std::vector<ShaderProperty*> properties;
	std::vector<char*> inputs;

};

struct FadeData
{
	EffectShader* glslData;
	float startAt;
	float stopAt;
};

struct Transitions
{
	FadeData* intro;
	FadeData* outro;
};

struct EffectGlslData
{
	std::vector<float> defaultV;
	std::vector<char*> params;
	EffectShader* description;
};

struct EffectChainElement
{
	EffectGlslData* glslData;
	char* id;
	char* name;
};

struct LayerData
{
	char* id;
	char* mediaPath;
	char* outputFileName;
	float timeOfEntry;
	float timeOfExit;
	std::vector<EffectChainElement*> effectsChain;
	Transitions* transitions;

	bool bImage;
};

struct CompositionData
{
	int compositionWidth;
	int compositionHeight;
	int compositionDuration;
};

CompositionData g_CompositionData;
LayerData* g_layerData = NULL;

bool readCompositionData( Json::Value& compositionData )
{
	if( !compositionData.isMember( "compositionWidth" ) || !compositionData["compositionWidth"].isInt() )
	{
		printf( "Don't find compositionWidth member or value is not integer" );
		return false;
	}
	g_CompositionData.compositionWidth = compositionData["compositionWidth"].asInt();

	if( !compositionData.isMember( "compositionHeight" ) || !compositionData["compositionHeight"].isInt() )
	{
		printf( "Don't find compositionHeight member or value is not integer" );
		return false;
	}
	g_CompositionData.compositionHeight = compositionData["compositionHeight"].asInt();

	if( !compositionData.isMember( "duration" ) || !compositionData["duration"].isInt() )
	{
		printf( "Don't find duration member or value is not integer" );
		return false;
	}
	g_CompositionData.compositionDuration = compositionData["duration"].asInt();

	return true;
}

char* readString( const Json::Value& v)
{
	//return "";
	std::string str = v.asCString();
	char* c = new char[str.size() + 1 ];
	strcpy( c, str.data() );
	c[str.size()] = '\0';
	return c;
}

int readInt( const Json::Value& v )
{
	return v.asInt();
}

float readDouble( const Json::Value& v )
{
	return v.asDouble();
}

bool readEffectShader(Json::Value& description, EffectShader* effectShader )
{
	if( description.isMember( "title" ) && description["title"].isString() )
	{
		effectShader->title = readString( description["title"] );
	}

	if( description.isMember( "description" ) && description["description"].isString() )
	{
		effectShader->description = readString( description["description"] );
	}

	if( description.isMember( "vertexShader" ) && description["vertexShader"].isString() )
	{
		effectShader->vertexShader = readString( description["vertexShader"] );

	}
	else
	{
		printf( "Can't find vertex shader");
		return false;
	}

	if( description.isMember( "fragmentShader" ) && description["fragmentShader"].isString() )
	{
		effectShader->fragmentShader = readString( description["fragmentShader"] );

	}
	else
	{
		printf( "Can't find fragment shader");
		return false;
	}

	if( description.isMember( "properties" ) )
	{
		int propertyCnt = description["properties"].getMemberNames().size();
		std::vector<std::string> list = description["properties"].getMemberNames();
		for( int j = 0; j < propertyCnt; j++ )
		{
			ShaderProperty* sp = new ShaderProperty;
			sp->value = new PropertyValue;

			sp->name = readString( description["properties"].getMemberNames().at( j ) );
// 			std::string name = description["properties"].getMemberNames().at( j );
// 			const char* cc = description["properties"].getMemberNames().at( j ).c_str();
// 			sp->name = new char[name.size()];
// 			strcpy( sp->name, name.c_str() );

			std::string sptype = description["properties"][sp->name]["type"].asString();
			sp->type = new char[sptype.size()];
			strcpy( sp->type, sptype.data() );

			if( description["properties"][sp->name]["value"].isDouble() )
			{
				sp->ptType = PROPERTY_TYPE_FLOAT;
				sp->value->f = description["properties"][sp->name]["value"].asFloat();
			}
			else if( description["properties"][sp->name]["value"].isInt() )
			{
				sp->ptType = PROPERTY_TYPE_INT;
				sp->value->i = description["properties"][sp->name]["value"].asInt();
			}
			else if( description["properties"][sp->name]["value"].isArray() && description["properties"][sp->name]["value"].size() == 4 )
			{
				sp->ptType = PROPERTY_TYPE_VEC4;
				for( int k = 0; k < description["properties"][sp->name]["value"].size(); k++ )
				{
					sp->value->v[k] = description["properties"][sp->name]["value"][k].asFloat();
				}
			}
			effectShader->properties.push_back( sp );
		}
	}
	else
	{
		printf( "Can't find shader properties");
		return false;
	}

	if( description.isMember( "inputs" ) && description["inputs"].isArray() )
	{
		for( int i = 0; i < description["inputs"].size(); i++ )
		{
			effectShader->inputs.push_back( readString( description["inputs"][i] ) );
		}
	}
	else
	{
		printf( "Can't find shader inputs" );
		return false;
	}

	return true;
}

bool readLayerData(Json::Value& layerData )
{
	g_layerData = new LayerData;

	if( layerData.isMember("id") && layerData["id"].isString() )
	{
		g_layerData->id = readString( layerData["id"] );
		
	}

	if( layerData.isMember("timeOfEntry") && layerData["timeOfEntry"].isDouble() )
	{
		g_layerData->timeOfEntry = readDouble( layerData["timeOfEntry"] );
	}
	else
	{
		printf( "Don't find timeOfEntry member or value is not integer" );
		return false;
	}

	if( layerData.isMember("timeOfExit") && layerData["timeOfExit"].isDouble() )
	{
		g_layerData->timeOfExit = readDouble( layerData["timeOfExit"] );
	}
	else
	{
		printf( "Don't find timeOfExit member or value is not integer" );
		return false;
	}
	if( layerData.isMember("mediaPath") && layerData["mediaPath"].isString() )
	{
		g_layerData->mediaPath = readString( layerData["mediaPath"] );
		std::string inputFile( g_layerData->mediaPath );
		int lastCommnadIndex = inputFile.find_last_of( "." );
		std::string exten = inputFile.substr( inputFile.find_last_of( "." ) + 1);
		if( exten == "png" || exten == "PNG" )
			g_layerData->bImage = true;
		else
			g_layerData->bImage = false;
	}
	else
	{
		printf( "Don't find mediaPath member or value is not string" );
		return false;
	}

	if( layerData.isMember( "outputFileName" ) && layerData["outputFileName"].isString() )
	{
		g_layerData->outputFileName = readString( layerData["outputFileName"] );
	}
	else
	{
		printf( "Don't find mediaPath member or value is not string" );
		return false;
	}

	if( layerData.isMember("effectsChain") )
	{
		Json::Value& layerEffects = layerData["effectsChain"];


		int chainSize = layerEffects.size();
		std::vector<Json::String> effectsChainMembers = layerEffects.getMemberNames();


		for( int i = 0; i < chainSize; i++ )
		{
			EffectChainElement* effectChainElement = new EffectChainElement;

			Json::Value& effect = layerEffects[effectsChainMembers.at( i )];

			if( effect.isMember( "id" ) && effect["id"].isString() )
			{
				effectChainElement->id = readString( effect["id"] );
			}

			if( effect.isMember( "name" ) && effect["name"].isString() )
			{
				effectChainElement->name = readString( effect["name"] );
			}

			if( effect.isMember( "glslData" ) )
			{
				effectChainElement->glslData = new EffectGlslData;
				EffectGlslData* effectGlslData = effectChainElement->glslData;

				Json::Value& glslData = effect["glslData"];

				if(glslData.isMember("defaults") && glslData["defaults"].isArray())
				{
					for( const Json::Value& v : glslData["defaults"] )
					{
						if(v.isDouble())
						{
							effectGlslData->defaultV.push_back( v.asFloat() );
						}
					}
				}

				if( glslData.isMember( "parameters" ) && glslData["parameters"].isArray() )
				{
					for( const Json::Value& v : glslData["parameters"] )
					{
						if( v.isString() )
						{
							effectGlslData->params.push_back( readString(v) );
						}
					}
				}

				if( glslData.isMember( "description" ) )
				{
					effectGlslData->description = new EffectShader;
					EffectShader* effectShader = effectGlslData->description;

					Json::Value& description = glslData["description"];

					if( !readEffectShader( description, effectShader ) )
						return false;

				}
				else
				{
					printf( "Don't find description member in glslData of %d-th effectChaing", i );
					return false;
				}
 			}
			else
			{
				printf( "Don't find glsData member of %d-th effectChaing", i );
				return false;

 			}

			g_layerData->effectsChain.push_back( effectChainElement );

		}
	}
	else
	{
		printf( "Don't find effectsChain member" );
		return false;
	}

	if( layerData.isMember( "transitions" ) )
	{
		g_layerData->transitions = new Transitions;
		Transitions* transition = g_layerData->transitions;

		if(layerData["transitions"].isMember("intro"))
		{
			transition->intro = new FadeData;
			FadeData* fadeData = transition->intro;
			Json::Value& intro = layerData["transitions"]["intro"];

			if( intro.isMember( "startAt" ) && intro["startAt"].isDouble() )
			{
				fadeData->startAt = readDouble( intro["startAt"] );
			}
			else
			{
				printf( "Don't find startAt member or intro transition or is not float" );
				return false;
			}

			if( intro.isMember( "stopAt" ) && intro["stopAt"].isDouble() )
			{
				fadeData->stopAt = readDouble( intro["stopAt"] );
			}
			else
			{
				printf( "Don't find stopAt member or intro transition or is not float" );
				return false;
			}

			if(intro.isMember("glslData"))
			{
				fadeData->glslData = new EffectShader;
				EffectShader* glslData = fadeData->glslData;
				
				if( !readEffectShader( intro["glslData"], glslData ) )
					return false;
			}
			else
			{
				printf( "Don't find glslData member or intro transition" );
				return false;
			}
		}
		else
			g_layerData->transitions->intro = NULL;

		if( layerData["transitions"].isMember( "outro" ) )
		{
			transition->outro = new FadeData;
			FadeData* fadeData = transition->outro;
			Json::Value& outro = layerData["transitions"]["outro"];

			if( outro.isMember( "startAt" ) && outro["startAt"].isDouble() )
			{
				fadeData->startAt = readDouble( outro["startAt"] );
			}
			else
			{
				printf( "Don't find startAt member or outro transition or is not float" );
				return false;
			}

			if( outro.isMember( "stopAt" ) && outro["stopAt"].isDouble() )
			{
				fadeData->stopAt = readDouble( outro["stopAt"] );
			}
			else
			{
				printf( "Don't find stopAt member or outro transition or is not float" );
				return false;
			}

			if( outro.isMember( "glslData" ) )
			{
				fadeData->glslData = new EffectShader;
				EffectShader* glslData = fadeData->glslData;

				if( !readEffectShader( outro["glslData"], glslData ) )
					return false;
			}
			else
			{
				printf( "Don't find glslData member or outro transition" );
				return false;
			}

		}
		else
			g_layerData->transitions->outro = NULL;
	}
	else
	{
		g_layerData->transitions->intro = NULL;
		g_layerData->transitions->outro = NULL;
	}

	return true;
}
bool readJson(char* file_full_name, char* layer, int* bImage)			
{
	Json::Value root;
	Json::Reader reader;

	std::ifstream file( file_full_name );

	if( !reader.parse( file, root, true ) )
	{
		std::cout << reader.getFormattedErrorMessages() << "\n";
		return false;
	}

	//parsing compositon data
	if( !root.isMember( "compositionData" ))
	{
		printf( "Don't find compositionData  member, please check compositionData member exists" );
		return false;
	}

	Json::Value& compositionData = root["compositionData"];

	if( !readCompositionData( compositionData ) )
		return false;

	//parsing layer data
	if( !root.isMember( "layerData" ) )
	{
		printf( "Don't find layerData array member, please check layerData member exists" );
		return false;
	}

	Json::Value& layerData = root["layerData"];

	if( !layerData.isMember( layer ) )
	{
		printf( "Don't find proper layer member\n" );
	}

	if( !readLayerData( layerData[layer] ) )
		return false;

	*bImage = g_layerData->bImage;

	return true;
}
