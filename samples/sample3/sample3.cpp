/* OpenSceneGraph example, osgterrain.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*  THE SOFTWARE.
*/

#include "ov_PLODTerrainTileInjection.h"
#include "ov_Utils.h"
#include <osg/ArgumentParser>
#include <osgDB/ReadFile>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/AnimationPathManipulator>
#include <osgGA/TerrainManipulator>

#include <osgTerrain/Terrain>
#include <osgTerrain/TerrainTile>
#include <osgTerrain/GeometryTechnique>
#include <osg/Version>
#include <osgTerrain/Layer>

#include <osgFX/MultiTextureControl>
#include <osg/PositionAttitudeTransform>
#include <osg/PatchParameter>
#include <osg/Fog>

#include <iostream>


int main(int argc, char** argv)
{
	osg::ArgumentParser arguments(&argc, argv);

	// construct the viewer.
	osgViewer::Viewer viewer(arguments);

	// set up the camera manipulators.
	{
		osg::ref_ptr<osgGA::KeySwitchMatrixManipulator> keyswitchManipulator = new osgGA::KeySwitchMatrixManipulator;

		keyswitchManipulator->addMatrixManipulator('1', "Trackball", new osgGA::TrackballManipulator());
		keyswitchManipulator->addMatrixManipulator('2', "Flight", new osgGA::FlightManipulator());
		keyswitchManipulator->addMatrixManipulator('3', "Drive", new osgGA::DriveManipulator());
		keyswitchManipulator->addMatrixManipulator('4', "Terrain", new osgGA::TerrainManipulator());

		std::string pathfile;
		char keyForAnimationPath = '5';
		while (arguments.read("-p", pathfile))
		{
			osgGA::AnimationPathManipulator* apm = new osgGA::AnimationPathManipulator(pathfile);
			if (apm || !apm->valid())
			{
				unsigned int num = keyswitchManipulator->getNumMatrixManipulators();
				keyswitchManipulator->addMatrixManipulator(keyForAnimationPath, "Path", apm);
				keyswitchManipulator->selectMatrixManipulator(num);
				++keyForAnimationPath;
			}
		}
		viewer.setCameraManipulator(keyswitchManipulator.get());
	}

	// add the state manipulator
	viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));

	// add the stats handler
	viewer.addEventHandler(new osgViewer::StatsHandler);

	// add the record camera path handler
	viewer.addEventHandler(new osgViewer::RecordCameraPathHandler);

	// add the window size toggle handler
	viewer.addEventHandler(new osgViewer::WindowSizeHandler);

	osg::DisplaySettings::instance()->setNumMultiSamples(4);

	//Add sample data path
	osgDB::Registry::instance()->getDataFilePathList().push_back("../data");


	osgVegetation::BillboardLayer grass_data(240, 0.02, 1.0, 0.3, 0.1, 5);
	grass_data.Type = osgVegetation::BillboardLayer::BLT_GRASS;
	grass_data.Billboards.push_back(osgVegetation::BillboardLayer::Billboard("billboards/veg_plant03.png", osg::Vec2f(4, 2), 0.9, 0.008));
	grass_data.Billboards.push_back(osgVegetation::BillboardLayer::Billboard("billboards/veg_plant01.png", osg::Vec2f(2, 2), 0.9, 0.002));
	grass_data.Billboards.push_back(osgVegetation::BillboardLayer::Billboard("billboards/grass2.png", osg::Vec2f(2, 1), 1.0, 1.0));
	
	osgVegetation::BillboardLayer tree_data(740, 0.01, 0.5, 0.7, 0.1, 2);
	tree_data.Type = osgVegetation::BillboardLayer::BLT_ROTATED_QUAD;
	tree_data.Billboards.push_back(osgVegetation::BillboardLayer::Billboard("billboards/fir01_bb.png", osg::Vec2f(10, 16),1.2,1.0));
	//tree_data.Billboards.push_back(osgVegetation::BillboardLayer::Billboard("billboards/tree0.rgba", osg::Vec2f(8, 16), 1.2));
	std::vector<osgVegetation::BillboardLayer> layers;
	layers.push_back(grass_data);
	layers.push_back(tree_data);
	
	/*terrain_data.VertexShader = "ov_terrain_detail_vertex.glsl";
	terrain_data.FragmentShader = "ov_terrain_detail_fragment.glsl";
	terrain_data.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_grass_mossy.dds"), 10));
	terrain_data.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_dirt.dds"), 10));
	terrain_data.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_grass_mossy.dds"), 10));
	terrain_data.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_dirt.dds"), 10));
	
	std::vector< osgVegetation::BillboardLayer> l2;
	l2.push_back(tree_data);
	osgVegetation::LODLayer tree_layer(2, l2);
	std::vector< osgVegetation::LODLayer> layers;
	layers.push_back(tree_layer);
	*/
	osgDB::Registry::instance()->setReadFileCallback(new osgVegetation::PLODTerrainTileInjection(layers));
	osg::ref_ptr<osg::Node> rootnode = osgDB::readNodeFile("terrain/us-terrain.zip/us-terrain.osg");

	osgVegetation::Terrain terrain_data;
	terrain_data.VertexShader = "ov_terrain_detail_vertex.glsl";
	terrain_data.FragmentShader = "ov_terrain_detail_fragment.glsl";
	terrain_data.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_grass_mossy.dds"), 10));
	terrain_data.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_dirt.dds"), 10));
	terrain_data.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_grass_mossy.dds"), 10));
	terrain_data.DetailLayers.push_back(osgVegetation::DetailLayer(std::string("terrain/detail/detail_dirt.dds"), 10));
	osgVegetation::PrepareTerrainForDetailMapping(rootnode, terrain_data);
	if (!rootnode)
	{
		osg::notify(osg::NOTICE) << "Warning: no valid data loaded, please specify a database on the command line." << std::endl;
		return 1;
	}
	
	//Add light and shadows
	osg::Light* pLight = new osg::Light;
	pLight->setDiffuse(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
	osg::Vec4 lightPos(1, 1.0, 1, 0);
	pLight->setPosition(lightPos);		// last param	w = 0.0 directional light (direction)
	osg::Vec3f lightDir(-lightPos.x(), -lightPos.y(), -lightPos.z());
	lightDir.normalize();
	pLight->setDirection(lightDir);
	pLight->setAmbient(osg::Vec4(0.5f, 0.5f, 0.5f, 1.0f));

	osg::LightSource* pLightSource = new osg::LightSource;
	pLightSource->setLight(pLight);
	rootnode->asGroup()->addChild(pLightSource);

	// add a viewport to the viewer and attach the scene graph.
	viewer.setSceneData(rootnode.get());
	bool use_fog = true;
	if (use_fog)
	{
		const osg::Vec4 fog_color(0.5, 0.6, 0.7, 1.0);
		//Add fog
		osg::StateSet* state = rootnode->getOrCreateStateSet();
		osg::ref_ptr<osg::Fog> fog = new osg::Fog();
		state->setMode(GL_FOG, osg::StateAttribute::ON);
		state->setAttributeAndModes(fog.get());
		fog->setMode(osg::Fog::EXP2);
		fog->setDensity(0.0005);
		fog->setColor(fog_color);
		viewer.getCamera()->setClearColor(fog_color);
		osg::StateSet::DefineList& defineList = state->getDefineList();
		defineList["FM_EXP2"].second = (osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
	}

	
	viewer.setUpViewInWindow(100, 100, 800, 600);

	viewer.getCamera()->getGraphicsContext()->getState()->setUseModelViewAndProjectionUniforms(true);
	//viewer.getCamera()->getGraphicsContext()->getState()->resetVertexAttributeAlias(false, 8);
	//viewer.getCamera()->getGraphicsContext()->getState()->setUseVertexAttributeAliasing(true);
	// run the viewers main loop

	while (!viewer.done())
	{
		//animate light if shadows enabled
		//if(shadow_type != osgVegetation::SM_DISABLED)
		{
			float t = viewer.getFrameStamp()->getSimulationTime() * 0.5;
		    lightPos.set(sinf(t), cosf(t), 0.5 + 0.45*cosf(t), 0.0f);
		   //lightPos.set(1.0, 0, 0.5 + 0.45*cosf(t), 0.0f);
			//lightPos.set(0.2f,0,1.1 + cosf(t),0.0f);
			pLight->setPosition(lightPos);
			lightDir.set(-lightPos.x(), -lightPos.y(), -lightPos.z());
			lightDir.normalize();
			pLight->setDirection(lightDir);
		}
		viewer.frame();
	}
	return 0;
	//return viewer.run();

}
