#ifndef SCIVIS_SCALAR_VISER_MULTI_VOLUME_RENDERER_H
#define SCIVIS_SCALAR_VISER_MULTI_VOLUME_RENDERER_H

#include <scivis/common/callback.h>
#include <scivis/common/util.h>

#include <osg/CullFace>
#include <osg/Group>
#include <osg/Texture3D>
#include <osg/Texture2D>
#include <osg/Texture1D>

#include <scivis/common/zhongdian15.h>

namespace SciVis
{
	class MultiVolumeRenderer
	{
	public:
		constexpr static int MaxVolumeNum = 2;

		osg::ref_ptr<osg::Group> grp;

		MultiVolumeRenderer()
		{
			grp = new osg::Group;

			osg::ref_ptr<osg::Shader> vertShader = osg::Shader::readShaderFile(
				osg::Shader::VERTEX,
				GetDataPathPrefix() +
				SCIVIS_SHADER_PREFIX
				"scivis/scalar_viser/mdvr_vert.glsl");
			osg::ref_ptr<osg::Shader> fragShader = osg::Shader::readShaderFile(
				osg::Shader::FRAGMENT,
				GetDataPathPrefix() +
				SCIVIS_SHADER_PREFIX
				"scivis/scalar_viser/mdvr_frag.glsl");
			program = new osg::Program;
			program->addShader(vertShader);
			program->addShader(fragShader);

			{
				auto tessl = new osg::TessellationHints;
				tessl->setDetailRatio(10.f);
				sphere = new osg::ShapeDrawable(
					new osg::Sphere(osg::Vec3(0.f, 0.f, 0.f),
						static_cast<float>(osg::WGS_84_RADIUS_EQUATOR)), tessl);
			}
			grp->addChild(sphere);

			auto states = sphere->getOrCreateStateSet();
#define STATEMENT(name, val) name = new osg::Uniform(#name, val);\
			states->addUniform(name)

			STATEMENT(eyePos, osg::Vec3());
			STATEMENT(dt, static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * .008f);
			STATEMENT(maxStepCnt, 100);

			STATEMENT(longtitudeRange, osg::Vec2());
			STATEMENT(latitudeRange, osg::Vec2());
			STATEMENT(heightRange, osg::Vec2());
			STATEMENT(volStartFromZeroLon, false);
#undef STATEMENT

			auto volTexUni = new osg::Uniform(osg::Uniform::SAMPLER_3D, "volTex0");
			volTexUni->set(0);
			states->addUniform(volTexUni);
			volTexUni = new osg::Uniform(osg::Uniform::SAMPLER_3D, "volTex1");
			volTexUni->set(1);
			states->addUniform(volTexUni);

			auto tfTexPreIntUni = new osg::Uniform(osg::Uniform::SAMPLER_2D, "tfTexPreInt0");
			tfTexPreIntUni->set(2);
			states->addUniform(tfTexPreIntUni);
			tfTexPreIntUni = new osg::Uniform(osg::Uniform::SAMPLER_2D, "tfTexPreInt1");
			tfTexPreIntUni->set(3);
			states->addUniform(tfTexPreIntUni);

			osg::ref_ptr<osg::CullFace> cf = new osg::CullFace(osg::CullFace::BACK);
			states->setAttributeAndModes(cf);

			states->setAttributeAndModes(program, osg::StateAttribute::ON);
			states->setMode(GL_BLEND, osg::StateAttribute::ON);
			states->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

			grp->setCullCallback(new Callback(eyePos));
		}

		struct RenderParameters
		{
			std::array<float, 2> longtitudeRange;
			std::array<float, 2> latitudeRange;
			std::array<float, 2> heightRange;
			bool volStartFromLonZero = false;
			float dt;
			int maxStepCnt;
			std::array<osg::ref_ptr<osg::Texture2D>, MaxVolumeNum> tfPreInts;
			std::array<osg::ref_ptr<osg::Texture3D>, MaxVolumeNum> vols;
		};
		void SetRenderParameters(const RenderParameters& param)
		{
			rndrParam = param;

			sphere->setShape(new osg::Sphere(osg::Vec3(0.f, 0.f, 0.f), rndrParam.heightRange[1]));

			longtitudeRange->set(osg::Vec2(
				Deg2Rad(rndrParam.longtitudeRange[0]),
				Deg2Rad(rndrParam.longtitudeRange[1])));
			latitudeRange->set(osg::Vec2(
				Deg2Rad(rndrParam.latitudeRange[0]),
				Deg2Rad(rndrParam.latitudeRange[1])));
			heightRange->set(osg::Vec2(rndrParam.heightRange[0], rndrParam.heightRange[1]));
			volStartFromZeroLon->set(rndrParam.volStartFromLonZero);

			dt->set(rndrParam.dt);
			maxStepCnt->set(rndrParam.maxStepCnt);

			auto states = sphere->getOrCreateStateSet();
			states->setTextureAttributeAndModes(0, rndrParam.vols[0], osg::StateAttribute::ON);
			states->setTextureAttributeAndModes(1, rndrParam.vols[1], osg::StateAttribute::ON);
			states->setTextureAttributeAndModes(2, rndrParam.tfPreInts[0], osg::StateAttribute::ON);
			states->setTextureAttributeAndModes(3, rndrParam.tfPreInts[1], osg::StateAttribute::ON);
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
		osg::ref_ptr<osg::Program> program;

		osg::ref_ptr<osg::Uniform> eyePos;
		osg::ref_ptr<osg::Uniform> dt;
		osg::ref_ptr<osg::Uniform> maxStepCnt;

		osg::ref_ptr<osg::Uniform> longtitudeRange;
		osg::ref_ptr<osg::Uniform> latitudeRange;
		osg::ref_ptr<osg::Uniform> heightRange;
		osg::ref_ptr<osg::Uniform> volStartFromZeroLon;

		osg::ref_ptr<osg::ShapeDrawable> sphere;
	};
}

#endif // !SCIVIS_SCALAR_VISER_MULTI_VOLUME_RENDERER_H
