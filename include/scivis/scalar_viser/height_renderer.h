#ifndef SCIVIS_SCALAR_VISER_HEIGHT_RENDERER_H
#define SCIVIS_SCALAR_VISER_HEIGHT_RENDERER_H

#include <array>

#include <osg/CullFace>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>

#include <scivis/common/callback.h>
#include <scivis/common/util.h>
#include <scivis/common/zhongdian15.h>

namespace SciVis
{
	class HeightMapRenderer
	{
	public:
		osg::ref_ptr<osg::Group> grp;

		HeightMapRenderer()
		{
			grp = new osg::Group;
			osg::ref_ptr<osg::Shader> vertShader = osg::Shader::readShaderFile(
				osg::Shader::VERTEX,
				SciVis::GetDataPathPrefix() +
				SCIVIS_SHADER_PREFIX
				"scivis/scalar_viser/height_map_vert.glsl");
			osg::ref_ptr<osg::Shader> geomShader = osg::Shader::readShaderFile(
				osg::Shader::GEOMETRY,
				SciVis::GetDataPathPrefix() +
				SCIVIS_SHADER_PREFIX
				"scivis/scalar_viser/height_map_geom.glsl");
			osg::ref_ptr<osg::Shader> fragShader = osg::Shader::readShaderFile(
				osg::Shader::FRAGMENT,
				SciVis::GetDataPathPrefix() +
				SCIVIS_SHADER_PREFIX
				"scivis/scalar_viser/height_map_frag.glsl");
			program = new osg::Program;
			program->addShader(vertShader);
			program->addShader(geomShader);
			program->addShader(fragShader);
			program->setParameter(GL_GEOMETRY_VERTICES_OUT_EXT, 11);
			program->setParameter(GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES);
			program->setParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);

#define STATEMENT(name, val) name = new osg::Uniform(#name, val)
			STATEMENT(eyePos, osg::Vec3());

			STATEMENT(isPillar, MapParameters().heightMapTy == EMapType::Pillar);
			STATEMENT(heightRange, osg::Vec2());
#undef STATEMENT

			grp->setCullCallback(new Callback(eyePos));

			verts = new osg::Vec3Array;
			uvs = new osg::Vec2Array;
			geom = new osg::Geometry;
			geode = new osg::Geode;
			geode->addDrawable(geom);
			grp->addChild(geode);

			auto states = geode->getOrCreateStateSet();

			states->addUniform(eyePos);
			states->addUniform(isPillar);
			states->addUniform(heightRange);

			auto texUni = new osg::Uniform(osg::Uniform::SAMPLER_2D, "heightMapTex");
			texUni->set(0);
			states->addUniform(texUni);

			states->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
			states->setAttributeAndModes(program, osg::StateAttribute::ON);

			osg::ref_ptr<osg::CullFace> cf = new osg::CullFace(osg::CullFace::BACK);
			states->setAttributeAndModes(cf);
		}

		struct RenderParameters
		{
			std::array<float, 2> heightRange;
			bool volStartFromLonZero = false;
			osg::ref_ptr<osg::Texture2D> heightMapTex;
		};
		void SetRenderParameters(const RenderParameters& param)
		{
			rndrParam = param;
			heightRange->set(osg::Vec2(rndrParam.heightRange[0], rndrParam.heightRange[1]));

			auto states = geode->getOrCreateStateSet();
			states->setTextureAttributeAndModes(0, rndrParam.heightMapTex, osg::StateAttribute::ON);
		}
		RenderParameters GetRenderParamters() const
		{
			return rndrParam;
		}

		enum class EMapType
		{
			Pillar = 0,
			Surface
		};
		struct MapParameters
		{
			std::array<float, 2> longtitudeRange;
			std::array<float, 2> latitudeRange;
			std::array<float, 2> heightRange;
			EMapType heightMapTy = EMapType::Pillar;
			std::array<uint32_t, 2> tessel = { 32, 32 };
		};
		void SetupMap(const MapParameters& param)
		{
			mapParam = param;
			isPillar->set(mapParam.heightMapTy == EMapType::Pillar);

			std::array<float, 2> longtitudeRange = {
				Deg2Rad(param.longtitudeRange[0]), Deg2Rad(param.longtitudeRange[1])
			};
			std::array<float, 2> latitudeRange = {
				Deg2Rad(param.latitudeRange[0]), Deg2Rad(param.latitudeRange[1])
			};

			verts->clear();
			uvs->clear();

			auto lonExt = longtitudeRange[1] - longtitudeRange[0];
			auto latExt = latitudeRange[1] - latitudeRange[0];
			auto lonDlt = lonExt / param.tessel[0];
			auto latDlt = latExt / param.tessel[1];

			auto toSphere = [&](const osg::Vec2& v2) -> osg::Vec3 {
				auto x = rndrParam.volStartFromLonZero == 0 ? v2.x() :
					v2.x() < .5f ? v2.x() + .5f : v2.x() - .5f;
				float lon = longtitudeRange[0] + x * lonExt;
				float lat = latitudeRange[0] + v2.y() * latExt;
				auto h = param.heightRange[0];

				osg::Vec3 ret;
				ret.z() = h * sinf(lat);
				h = h * cosf(lat);
				ret.y() = h * sinf(lon);
				ret.x() = h * cosf(lon);

				return ret;
				};

			verts->reserve(param.tessel[0] * param.tessel[1]);
			uvs->reserve(param.tessel[0] * param.tessel[1]);
			for (int latIdx = 0; latIdx < param.tessel[1]; ++latIdx)
				for (int lonIdx = 0; lonIdx < param.tessel[0]; ++lonIdx) {
					osg::Vec2 uv(
						1.f * lonIdx / (param.tessel[0] - 1),
						1.f * latIdx / (param.tessel[1] - 1));

					auto pos = toSphere(uv);
					verts->push_back(pos);
					uvs->push_back(uv);
				}

			std::vector<GLuint> vertIndices;
			auto addTri = [&](std::array<GLuint, 3> triIndices) {
				for (uint8_t i = 0; i < 3; ++i)
					vertIndices.emplace_back(triIndices[i]);
				};
			auto addBotSurf = [&](int latIdx, int lonIdx) {
				std::array<GLuint, 4> quadIndices = {
					latIdx * param.tessel[0] + lonIdx,
					latIdx * param.tessel[0] + lonIdx + 1,
					(latIdx + 1) * param.tessel[0] + lonIdx + 1,
					(latIdx + 1) * param.tessel[0] + lonIdx
				};

				addTri({ quadIndices[0], quadIndices[1], quadIndices[2] });
				addTri({ quadIndices[2], quadIndices[3], quadIndices[0] });
				};
			for (int latIdx = 0; latIdx < param.tessel[1] - 1; ++latIdx)
				for (int lonIdx = 0; lonIdx < param.tessel[0] - 1; ++lonIdx)
					addBotSurf(latIdx, lonIdx);

			geom->setVertexArray(verts);
			geom->setTexCoordArray(0, uvs, osg::Array::BIND_PER_VERTEX);

			geom->getPrimitiveSetList().clear();
			geom->addPrimitiveSet(
				new osg::DrawElementsUInt(
					GL_TRIANGLES, vertIndices.size(), vertIndices.data()));
		}

	private:
		class Callback : public osg::NodeCallback
		{
		private:
			osg::ref_ptr<osg::Uniform> eyePos;

		public:
			Callback(osg::ref_ptr<osg::Uniform> eyePos) : eyePos(eyePos)
			{}
			virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
			{
				auto _eyePos = nv->getEyePoint();
				eyePos->setUpdateCallback(new UniformUpdateCallback<osg::Vec3>(_eyePos));

				traverse(node, nv);
			}
		};

		RenderParameters rndrParam;
		MapParameters mapParam;

		osg::ref_ptr<osg::Program> program;
		osg::ref_ptr<osg::Geometry> geom;
		osg::ref_ptr<osg::Geode> geode;
		osg::ref_ptr<osg::Vec3Array> verts;
		osg::ref_ptr<osg::Vec2Array> uvs;

		osg::ref_ptr<osg::Uniform> eyePos;
		osg::ref_ptr<osg::Uniform> heightRange;
		osg::ref_ptr<osg::Uniform> isPillar;
	};
}

#endif // !SCIVIS_SCALAR_VISER_HEIGHT_RENDERER_H
