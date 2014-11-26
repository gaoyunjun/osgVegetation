#include "VRTShaderInstancing.h"
#include <osg/AlphaFunc>
#include <osg/Billboard>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/Geode>
#include <osg/Material>
#include <osg/Math>
#include <osg/MatrixTransform>
#include <osg/PolygonOffset>
#include <osg/Projection>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>
#include <osg/TextureBuffer>
#include <osg/Image>
#include <osg/Texture2DArray>
#include <osgDB/ReadFile>
#include "VegetationCell.h"
#include "VegetationLayer.h"


namespace osgVegetation
{
	osg::StateSet* VRTShaderInstancing::createStateSet(BillboardVegetationLayerVector &layers) 
	{
		//Load textures
		const osg::ref_ptr<osgDB::ReaderWriter::Options> options = new osgDB::ReaderWriter::Options(); 
		options->setOptionString("dds_flip");
		std::map<std::string, osg::Image*> image_map;
		std::map<std::string, int> index_map;
		int num_textures = 0;
		for(size_t i = 0; i < layers.size();i++)
		{
			if(image_map.find(layers[i].TextureName) == image_map.end() )
			{
				image_map[layers[i].TextureName] = osgDB::readImageFile(layers[i].TextureName,options);
				index_map[layers[i].TextureName] = num_textures;
				layers[i].TextureUnit = num_textures;
				num_textures++;
			}
			else
				layers[i].TextureUnit = index_map[layers[i].TextureName];

		}
		osg::Texture2DArray* tex = new osg::Texture2DArray;
		tex->setTextureSize(512, 512, num_textures);
		tex->setUseHardwareMipMapGeneration(true);   
		
		for(size_t i = 0; i < layers.size();i++)
		{
			tex->setImage(index_map[layers[i].TextureName], image_map[layers[i].TextureName]);
		}

		osg::StateSet *dstate = new osg::StateSet;
		dstate->setTextureAttribute(0, tex,	osg::StateAttribute::ON);
		
		dstate->setAttributeAndModes( new osg::BlendFunc, osg::StateAttribute::ON );
		osg::AlphaFunc* alphaFunc = new osg::AlphaFunc;
		alphaFunc->setFunction(osg::AlphaFunc::GEQUAL,0.05f);
		dstate->setAttributeAndModes( alphaFunc, osg::StateAttribute::ON );
		dstate->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
		dstate->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

		osg::Uniform* baseTextureSampler = new osg::Uniform(osg::Uniform::SAMPLER_2D_ARRAY, "baseTexture", num_textures);
		dstate->addUniform(baseTextureSampler);
		dstate->setMode( GL_LIGHTING, osg::StateAttribute::OFF );

		{
			osg::Program* program = new osg::Program;
			dstate->setAttribute(program);

			char vertexShaderSource[] =
				"#version 440 compatibility\n"
				"#extension GL_ARB_uniform_buffer_object : enable\n"
				"uniform samplerBuffer dataBuffer;\n"
				"layout(location = 0) in vec3 VertexPosition;\n"
				"layout(location = 8) in vec3 VertexTexCoord;\n"
				"out vec2 TexCoord;\n"
				"out vec4 Color;\n"
				"out float veg_type; \n"
				"void main()\n"
				"{\n"
				"   int instanceAddress = gl_InstanceID * 3;\n"
				"   vec3 position = texelFetch(dataBuffer, instanceAddress).xyz;\n"
				"   Color         = texelFetch(dataBuffer, instanceAddress + 1);\n"
				"   vec4 data     = texelFetch(dataBuffer, instanceAddress + 2);\n"
				"   mat4 mvpMatrix = gl_ModelViewProjectionMatrix *\n"
				"        mat4( data.x, 0.0, 0.0, 0.0,\n"
				"              0.0, data.x, 0.0, 0.0,\n"
				"              0.0, 0.0, data.y, 0.0,\n"
				"              position.x, position.y, position.z, 1.0);\n"
				"   gl_Position = mvpMatrix * vec4(VertexPosition,1.0) ;\n"
				"   veg_type = data.z;\n"
				"   TexCoord = VertexTexCoord.xy;\n"
				"}\n";

			char fragmentShaderSource[] =
				"#version 440 core\n"
				"#extension GL_EXT_gpu_shader4 : enable\n"
				"#extension GL_EXT_texture_array : enable\n"
				"uniform sampler2DArray baseTexture; \n"
				"in float veg_type; \n"
				"in vec2 TexCoord;\n"
				"in vec4 Color;\n"
				"layout(location = 0, index = 0) out vec4 FragData0;\n"
				"void main(void) \n"
				"{\n"
				"    vec4 finalColor = texture2DArray( baseTexture, vec3(TexCoord, veg_type)); \n"
				"    FragData0 = Color*finalColor;\n"
				"}\n";

			osg::Shader* vertex_shader = new osg::Shader(osg::Shader::VERTEX, vertexShaderSource);
			program->addShader(vertex_shader);

			osg::Shader* fragment_shader = new osg::Shader(osg::Shader::FRAGMENT, fragmentShaderSource);
			program->addShader(fragment_shader);

			//osg::Uniform* baseTextureSampler = new osg::Uniform("baseTexture",0);
			//dstate->addUniform(baseTextureSampler);
			m_StateSet = dstate; 
			
		}
		return dstate;
	}

	osg::Geometry* VRTShaderInstancing::createOrthogonalQuadsNoColor( const osg::Vec3& pos, float w, float h)
	{
		// set up the coords
		osg::Vec3Array& v = *(new osg::Vec3Array(8));
		osg::Vec2Array& t = *(new osg::Vec2Array(8));

		float rotation = random(0.0f,osg::PI/2.0f);
		float sw = sinf(rotation)*w*0.5f;
		float cw = cosf(rotation)*w*0.5f;

		v[0].set(pos.x()-sw,pos.y()-cw,pos.z()+0.0f);
		v[1].set(pos.x()+sw,pos.y()+cw,pos.z()+0.0f);
		v[2].set(pos.x()+sw,pos.y()+cw,pos.z()+h);
		v[3].set(pos.x()-sw,pos.y()-cw,pos.z()+h);

		v[4].set(pos.x()-cw,pos.y()+sw,pos.z()+0.0f);
		v[5].set(pos.x()+cw,pos.y()-sw,pos.z()+0.0f);
		v[6].set(pos.x()+cw,pos.y()-sw,pos.z()+h);
		v[7].set(pos.x()-cw,pos.y()+sw,pos.z()+h);

		t[0].set(0.0f,0.0f);
		t[1].set(1.0f,0.0f);
		t[2].set(1.0f,1.0f);
		t[3].set(0.0f,1.0f);

		t[4].set(0.0f,0.0f);
		t[5].set(1.0f,0.0f);
		t[6].set(1.0f,1.0f);
		t[7].set(0.0f,1.0f);

		osg::Geometry *geom = new osg::Geometry;

		geom->setVertexArray( &v );

		geom->setTexCoordArray( 0, &t );

		geom->addPrimitiveSet( new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,8) );

		return geom;
	}


	/*osg::Node* VRTShaderInstancing::create(Cell* cell)
	{
		osg::ref_ptr<osg::Geometry> templateGeometry = createOrthogonalQuadsNoColor(osg::Vec3(0.0f,0.0f,0.0f),1.0f,1.0f);
		templateGeometry->setUseVertexBufferObjects(true);
		templateGeometry->setUseDisplayList(false);
		osg::Node* textureBufferGraph = createRec(cell, templateGeometry.get());
		textureBufferGraph->setStateSet(m_StateSet);
		return textureBufferGraph;
	}

	osg::Node* VRTShaderInstancing::createRec(Cell* cell, osg::Geometry* templateGeometry)
	{
		bool needGroup = !(cell->_cells.empty());
		bool needTrees = !(cell->_trees.empty());

		osg::Geode* geode = 0;
		osg::Group* group = 0;

		if (needTrees)
		{
			osg::Geometry* geometry = (osg::Geometry*)templateGeometry->clone( osg::CopyOp::DEEP_COPY_PRIMITIVES );
			geometry->setUseDisplayList(false);
			osg::DrawArrays* primSet = dynamic_cast<osg::DrawArrays*>( geometry->getPrimitiveSet(0) );
			primSet->setNumInstances( cell->_trees.size() );
			geode = new osg::Geode;
			geode->addDrawable(geometry);
			
			//osg::StateSet* new_state_set = (osg::StateSet*) m_StateSet->clone(osg::CopyOp::DEEP_COPY_ALL); 

			//geometry->setStateSet(new_state_set);

			osg::ref_ptr<osg::Image> treeParamsImage = new osg::Image;
			treeParamsImage->allocateImage( 3*cell->_trees.size(), 1, 1, GL_RGBA, GL_FLOAT );

			unsigned int i=0;
			for(BillboardVegetationObjectVector::iterator itr= cell->_trees.begin();
				itr!=cell->_trees.end();
				++itr,++i)
			{
				osg::Vec4f* ptr = (osg::Vec4f*)treeParamsImage->data(3*i);
				VegetationObject& tree = **itr;
				ptr[0] = osg::Vec4f(tree.Position.x(),tree.Position.y(),tree.Position.z(),1.0);
				ptr[1] = osg::Vec4f((float)tree.Color.r()/255.0f,(float)tree.Color.g()/255.0f, (float)tree.Color.b()/255.0f, 1.0);
				ptr[2] = osg::Vec4f(tree.Width, tree.Height, tree.TextureUnit, 1.0);
			}
			osg::ref_ptr<osg::TextureBuffer> tbo = new osg::TextureBuffer;
			tbo->setImage( treeParamsImage.get() );
			tbo->setInternalFormat(GL_RGBA32F_ARB);
			geometry->getOrCreateStateSet()->setTextureAttribute(1, tbo.get(),osg::StateAttribute::ON);
			geometry->setInitialBound( cell->_bb );
			osg::Uniform* dataBufferSampler = new osg::Uniform("dataBuffer",1);
			geometry->getOrCreateStateSet()->addUniform(dataBufferSampler);
		}

		if (needGroup)
		{
			group = new osg::Group;
			for(Cell::CellList::iterator itr=cell->_cells.begin();
				itr!=cell->_cells.end();
				++itr)
			{
				group->addChild(createRec(itr->get(),templateGeometry));
			}

			if (geode) group->addChild(geode);

		}
		if (group) return group;
		else return geode;
	}*/

	osg::Node* VRTShaderInstancing::create(const BillboardVegetationObjectVector &trees, const osg::BoundingBox &bb)
	{
		osg::Geode* geode = 0;
		osg::Group* group = 0;
		if(trees.size() > 0)
		{
			osg::ref_ptr<osg::Geometry> templateGeometry = createOrthogonalQuadsNoColor(osg::Vec3(0.0f,0.0f,0.0f),1.0f,1.0f);
			templateGeometry->setUseVertexBufferObjects(true);
			templateGeometry->setUseDisplayList(false);
			osg::Geometry* geometry = (osg::Geometry*)templateGeometry->clone( osg::CopyOp::DEEP_COPY_PRIMITIVES );
			geometry->setUseDisplayList(false);
			osg::DrawArrays* primSet = dynamic_cast<osg::DrawArrays*>( geometry->getPrimitiveSet(0) );
			primSet->setNumInstances( trees.size() );
			geode = new osg::Geode;
			geode->addDrawable(geometry);
			unsigned int i=0;
			osg::ref_ptr<osg::Image> treeParamsImage = new osg::Image;
			treeParamsImage->allocateImage( 3*trees.size(), 1, 1, GL_RGBA, GL_FLOAT );
			for(BillboardVegetationObjectVector::const_iterator itr= trees.begin();
				itr!= trees.end();
				++itr,++i)
			{
				osg::Vec4f* ptr = (osg::Vec4f*)treeParamsImage->data(3*i);
				BillboardVegetationObject& tree = **itr;
				ptr[0] = osg::Vec4f(tree.Position.x(),tree.Position.y(),tree.Position.z(),1.0);
				ptr[1] = osg::Vec4f((float)tree.Color.r()/255.0f,(float)tree.Color.g()/255.0f, (float)tree.Color.b()/255.0f, 1.0);
				ptr[2] = osg::Vec4f(tree.Width, tree.Height, tree.TextureUnit, 1.0);

			}
			osg::ref_ptr<osg::TextureBuffer> tbo = new osg::TextureBuffer;
			tbo->setImage( treeParamsImage.get() );
			tbo->setInternalFormat(GL_RGBA32F_ARB);
			geometry->getOrCreateStateSet()->setTextureAttribute(1, tbo.get(),osg::StateAttribute::ON);
			geometry->setInitialBound( bb );
			osg::Uniform* dataBufferSampler = new osg::Uniform("dataBuffer",1);
			geometry->getOrCreateStateSet()->addUniform(dataBufferSampler);
			
			geode->setStateSet(m_StateSet);

		}
		return geode;
	}
}