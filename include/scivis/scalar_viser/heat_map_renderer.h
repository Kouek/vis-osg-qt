#ifndef SCIVIS_SCALAR_VISER_HMR_H
#define SCIVIS_SCALAR_VISER_HMR_H

#include <string>

#include <map>

#include <osg/CoordinateSystemNode>
#include <osg/CullFace>
#include <osg/ShapeDrawable>
#include <osg/Texture1D>
#include <osg/Texture3D>

namespace SciVis
{
	namespace ScalarViser
	{

		class HeatMap3DRenderer
		{
		private:
			struct PerRendererParam
			{
				osg::ref_ptr<osg::Group> grp;
				osg::ref_ptr<osg::Program> program;

				PerRendererParam()
				{
					grp = new osg::Group;

					osg::ref_ptr<osg::Shader> vertShader = osg::Shader::readShaderFile(
						osg::Shader::VERTEX,
						SCIVIS_SHADER_PREFIX
						"scivis/scalar_viser/hmp_vert.glsl");
					osg::ref_ptr<osg::Shader> fragShader = osg::Shader::readShaderFile(
						osg::Shader::FRAGMENT,
						SCIVIS_SHADER_PREFIX
						"scivis/scalar_viser/hmp_frag.glsl");
					program = new osg::Program;
					program->addShader(vertShader);
					program->addShader(fragShader);
				}
			};
			PerRendererParam param;

			class PerVolParam
			{
			private:
				osg::ref_ptr<osg::Uniform> minLatitute;
				osg::ref_ptr<osg::Uniform> maxLatitute;
				osg::ref_ptr<osg::Uniform> minLongtitute;
				osg::ref_ptr<osg::Uniform> maxLongtitute;
				osg::ref_ptr<osg::Uniform> minHeight;
				osg::ref_ptr<osg::Uniform> maxHeight;
				osg::ref_ptr<osg::Uniform> height;
				osg::ref_ptr<osg::Uniform> volStartFromZeroLon;

				osg::ref_ptr<osg::ShapeDrawable> sphere;
				osg::ref_ptr<osg::Texture3D> volTex;
				osg::ref_ptr<osg::Texture1D> colTblTex;

			public:
				PerVolParam(osg::ref_ptr<osg::Texture3D> volTex, osg::ref_ptr<osg::Texture1D> colTblTex,
					PerRendererParam* renderer)
					: volTex(volTex), colTblTex(colTblTex)
				{
					const auto MinHeight = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * 1.1f;
					const auto MaxHeight = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * 1.3f;

					auto tessl = new osg::TessellationHints;
					tessl->setDetailRatio(10.f);
					sphere = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(0.f, 0.f, 0.f), MinHeight), tessl);

					auto states = sphere->getOrCreateStateSet();
#define STATEMENT(name, val)                                                                       \
    name = new osg::Uniform(#name, val);                                                           \
    states->addUniform(name)
					STATEMENT(minLatitute, deg2Rad(-10.f));
					STATEMENT(maxLatitute, deg2Rad(+10.f));
					STATEMENT(minLongtitute, deg2Rad(-20.f));
					STATEMENT(maxLongtitute, deg2Rad(+20.f));
					STATEMENT(minHeight, MinHeight);
					STATEMENT(maxHeight, MaxHeight);
					STATEMENT(height, .5f * (MinHeight + MaxHeight));
					STATEMENT(volStartFromZeroLon, 0);
#undef STATEMENT

					states->setTextureAttributeAndModes(0, volTex, osg::StateAttribute::ON);
					states->setTextureAttributeAndModes(1, colTblTex, osg::StateAttribute::ON);

					auto volTexUni = new osg::Uniform(osg::Uniform::SAMPLER_3D, "volTex");
					volTexUni->set(0);
					auto colTblTexUni = new osg::Uniform(osg::Uniform::SAMPLER_1D, "colTblTex");
					colTblTexUni->set(1);
					states->addUniform(volTexUni);
					states->addUniform(colTblTexUni);

					osg::ref_ptr<osg::CullFace> cf = new osg::CullFace(osg::CullFace::BACK);
					states->setAttributeAndModes(cf);

					states->setAttributeAndModes(renderer->program, osg::StateAttribute::ON);
					states->setMode(GL_BLEND, osg::StateAttribute::ON);
					states->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
				}
				/*
				* 函数: SetColorTable
				* 功能: 设置该体绘制时的颜色映射表
				* 参数:
				* -- colTblTex: 热力图颜色映射的OSG一维纹理
				*/
				void SetColorTable(osg::ref_ptr<osg::Texture1D> colTblTex)
				{
					this->colTblTex = colTblTex;
					auto states = sphere->getOrCreateStateSet();
					states->setTextureAttributeAndModes(1, this->colTblTex, osg::StateAttribute::ON);
				}
				/*
				* 函数: SetLongtituteRange
				* 功能: 设置该体绘制时的经度范围（单位为角度）
				* 参数:
				* -- minLonDeg: 经度最小值
				* -- maxLonDeg: 经度最大值
				* 返回值: 若输入的参数不合法，返回false。若设置成功，返回true
				*/
				bool SetLongtituteRange(float minLonDeg, float maxLonDeg)
				{
					if (minLonDeg < -180.f) return false;
					if (maxLonDeg > +180.f) return false;
					if (minLonDeg >= maxLonDeg) return false;

					minLongtitute->set(deg2Rad(minLonDeg));
					maxLongtitute->set(deg2Rad(maxLonDeg));
					return true;
				}
				/*
				* 函数: SetLatituteRange
				* 功能: 设置该体绘制时的纬度范围（单位为角度）
				* 参数:
				* -- minLatDeg: 纬度最小值
				* -- maxLatDeg: 纬度最大值
				* 返回值: 若输入的参数不合法，返回false。若设置成功，返回true
				*/
				bool SetLatituteRange(float minLatDeg, float maxLatDeg)
				{
					if (minLatDeg < -90.f) return false;
					if (maxLatDeg > +90.f) return false;
					if (minLatDeg >= maxLatDeg) return false;

					minLatitute->set(deg2Rad(minLatDeg));
					maxLatitute->set(deg2Rad(maxLatDeg));
					return true;
				}
				/*
				* 函数: SetHeightFromCenterRange
				* 功能: 设置该体绘制时的高度（距球心）范围
				* 参数:
				* -- minH: 高度最小值
				* -- maxH: 高度最大值
				* 返回值: 若输入的参数不合法，返回false。若设置成功，返回true
				*/
				bool SetHeightFromCenterRange(float minH, float maxH)
				{
					if (minH < 0.f) return false;
					if (minH >= maxH) return false;

					minHeight->set(minH);
					maxHeight->set(maxH);
					height->set(.5f * (minH + maxH));

					sphere->setShape(new osg::Sphere(osg::Vec3(0.f, 0.f, 0.f), minH));

					return true;
				}
				std::array<float, 2> GetHeightFromCenterRange() const
				{
					std::array<float, 2> ret;
					minHeight->get(ret[0]);
					maxHeight->get(ret[1]);
					return ret;
				}
				/*
				* 函数: SetHeightFromCenter
				* 功能: 设置该体绘制时的高度（距球心）
				* 参数:
				* -- h: 高度
				* 返回值: 若输入的参数不合法，返回false。若设置成功，返回true
				*/
				bool SetHeightFromCenter(float h)
				{
					float minH, maxH;
					minHeight->get(minH);
					maxHeight->get(maxH);
					if (h < minH || h > maxH) return false;

					height->set(h);
					return true;
				}
				float GetHeightFromCenter() const
				{
					float ret;
					height->get(ret);
					return ret;
				}
				/*
				* 函数: SetVolumeStartFromLongtituteZero
				* 功能: 若全球体数据X=0对应的经度为0度，需要开启该功能
				* 参数:
				* -- flag: 为true时，开启该功能。为false时，关闭该功能
				*/
				void SetVolumeStartFromLongtituteZero(bool flag)
				{
					if (flag) volStartFromZeroLon->set(1);
					else volStartFromZeroLon->set(0);
				}

			private:
				float deg2Rad(float deg)
				{
					return deg * osg::PI / 180.f;
				};

				friend class HeatMap3DRenderer;
			};
			std::map<std::string, PerVolParam> vols;

		public:
			HeatMap3DRenderer() {}

			/*
			* 函数: GetGroup
			* 功能: 获取该绘制组件的OSG节点
			* 返回值: OSG节点
			*/
			osg::Group* GetGroup() { return param.grp.get(); }
			/*
			* 函数: AddVolume
			* 功能: 向该绘制组件添加一个体
			* 参数:
			* -- name: 添加体的名称。不同体的名称需不同，用于区分
			* -- volTex: 体的OSG三维纹理
			* -- colTblTex: 热力图颜色映射的OSG一维纹理
			*/
			void AddVolume(const std::string& name, osg::ref_ptr<osg::Texture3D> volTex,
				osg::ref_ptr<osg::Texture1D> colTblTex)
			{
				auto itr = vols.find(name);
				if (itr != vols.end()) {
					param.grp->removeChild(itr->second.sphere);
					vols.erase(itr);
				}
				auto opt = vols.emplace(std::pair<std::string, PerVolParam>(name, PerVolParam(volTex, colTblTex, &param)));
				param.grp->addChild(opt.first->second.sphere);
			}
			/*
			* 函数: GetVolumes
			* 功能: 获取该组件中，体在绘制时所需的所有数据
			*/
			std::map<std::string, PerVolParam>& GetVolumes()
			{
				return vols;
			}
			/*
			* 函数: GetVolume
			* 功能: 获取该组件中，体在绘制时所需的数据
			* 参数:
			* -- name: 体的名称
			* 返回值: 体的绘制数据
			*/
			PerVolParam* GetVolume(const std::string& name)
			{
				auto itr = vols.find(name);
				if (itr == vols.end())
					return nullptr;
				return &(itr->second);
			}
			/*
			* 函数: GetVolumeNum
			* 功能: 获取该绘制组件中体的数量
			* 返回值: 体的数量
			*/
			size_t GetVolumeNum() const
			{
				return vols.size();
			}
		};

	} // namespace ScalarViser
} // namespace SciVis

#endif // !SCIVIS_SCALAR_VISER_HMR_H
