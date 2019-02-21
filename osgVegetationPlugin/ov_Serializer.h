#pragma once
#include <osg/Node>
#include <vector>
#include "ov_BillboardLayer.h"
#include "ov_Terrain.h"
class TiXmlElement;
namespace osgVegetation
{
	class XMLSerializer
	{
	public:
		XMLSerializer(){}
		virtual ~XMLSerializer(){}
		static void ReadTerrainData(const std::string &filename, TerrainConfiguration &terrain);
	};
}
