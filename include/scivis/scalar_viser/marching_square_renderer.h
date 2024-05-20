#ifndef SCIVIS_SCALAR_VISER_MARCHING_SQUARE_RENDERER_H
#define SCIVIS_SCALAR_VISER_MARCHING_SQUARE_RENDERER_H

#include <algorithm>
#include <memory>
#include <numeric>
#include <string>

#include <array>
#include <unordered_map>
#include <map>

#include <osg/CullFace>
#include <osg/CoordinateSystemNode>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture3D>

#include <scivis/common/zhongdian15.h>

namespace SciVis
{
	namespace ScalarViser
	{

		class MarchingSquareCPURenderer
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
						GetDataPathPrefix() +
						SCIVIS_SHADER_PREFIX
						"scivis/scalar_viser/mcsqr_vert.glsl");
					osg::ref_ptr<osg::Shader> fragShader = osg::Shader::readShaderFile(
						osg::Shader::FRAGMENT,
						GetDataPathPrefix() +
						SCIVIS_SHADER_PREFIX
						"scivis/scalar_viser/mcsqr_frag.glsl");
					program = new osg::Program;
					program->addShader(vertShader);
					program->addShader(fragShader);
				}
			};
			PerRendererParam param;

			class PerVolParam
			{
			private:
				std::array<uint32_t, 3> volDim;
				osg::Vec2 voxSz;
				float isoVal;
				float minLongtitute, maxLongtitute;
				float minLatitute, maxLatitute;
				float minHeight, maxHeight;
				bool volStartFromLonZero;

				osg::ref_ptr<osg::Uniform> minLatituteUni;
				osg::ref_ptr<osg::Uniform> maxLatituteUni;
				osg::ref_ptr<osg::Uniform> minLongtituteUni;
				osg::ref_ptr<osg::Uniform> maxLongtituteUni;
				osg::ref_ptr<osg::Uniform> minHeightUni;
				osg::ref_ptr<osg::Uniform> maxHeightUni;
				osg::ref_ptr<osg::Uniform> volStartFromLonZeroUni;

				std::shared_ptr<std::vector<float>> volDat;
				std::shared_ptr<std::vector<float>> volDatSmoothed;

				osg::ref_ptr<osg::Geometry> geom;
				osg::ref_ptr<osg::Geode> geode;
				osg::ref_ptr<osg::Vec3Array> verts;

				std::vector<GLuint> vertIndices;

			public:
				PerVolParam(
					decltype(volDat) volDat,
					decltype(volDat) volDatSmoothed,
					const std::array<uint32_t, 3>& volDim,
					PerRendererParam* renderer)
					: volDat(volDat), volDatSmoothed(volDatSmoothed), volDim(volDim)
				{
					const auto MinHeight = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * 1.1f;
					const auto MaxHeight = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) * 1.3f;

					minLongtitute = deg2Rad(-10.f);
					maxLongtitute = deg2Rad(+10.f);
					minLatitute = deg2Rad(-20.f);
					maxLatitute = deg2Rad(+20.f);
					minHeight = MinHeight;
					maxHeight = MaxHeight;
					volStartFromLonZero = false;

					voxSz = osg::Vec2(1.f / volDim[0], 1.f / volDim[1]);

					verts = new osg::Vec3Array;

					geom = new osg::Geometry;
					geode = new osg::Geode;
					geode->addDrawable(geom);

					auto states = geode->getOrCreateStateSet();

#define STATEMENT(name, val) \
    name##Uni = new osg::Uniform(#name, val); \
    states->addUniform(name##Uni)
					STATEMENT(minLatitute, deg2Rad(-10.f));
					STATEMENT(maxLatitute, deg2Rad(+10.f));
					STATEMENT(minLongtitute, deg2Rad(-20.f));
					STATEMENT(maxLongtitute, deg2Rad(+20.f));
					STATEMENT(minHeight, MinHeight);
					STATEMENT(maxHeight, MaxHeight);
					STATEMENT(volStartFromLonZero, 0);
#undef STATEMENT
					osg::ref_ptr<osg::CullFace> cf = new osg::CullFace(osg::CullFace::BACK);
					states->setAttributeAndModes(cf);

					states->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
					states->setAttributeAndModes(renderer->program, osg::StateAttribute::ON);
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

					minLongtitute = deg2Rad(minLonDeg);
					maxLongtitute = deg2Rad(maxLonDeg);
					minLongtituteUni->set(minLatitute);
					maxLongtituteUni->set(maxLatitute);
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

					minLatitute = deg2Rad(minLatDeg);
					maxLatitute = deg2Rad(maxLatDeg);
					minLatituteUni->set(minLatitute);
					maxLatituteUni->set(maxLatitute);
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

					minHeight = minH;
					maxHeight = maxH;
					minHeightUni->set(minHeight);
					maxHeightUni->set(maxHeight);
					return true;
				}
				/*
				* 函数: SetVolumeStartFromLongtituteZero
				* 功能: 若全球体数据X=0对应的经度为0度，需要开启该功能
				* 参数:
				* -- flag: 为true时，开启该功能。为false时，关闭该功能
				*/
				void SetVolumeStartFromLongtituteZero(bool flag)
				{
					volStartFromLonZero = flag;
					if (flag) volStartFromLonZeroUni->set(1);
					else volStartFromLonZeroUni->set(0);
				}
				/*
				* 函数: MarchingSquare
				* 功能: 在CPU端执行Marching Square算法，产生等值线
				* 参数:
				* -- isoVal: 产生等值线依据的标量值
				* -- heights: 需要产生等值线的面的高度，范围为[0, VolDim.z - 1]
				* -- useSmoothedVol: 为true时，使用平滑的体数据
				*/
				void MarchingSquare(
					float isoVal,
					const std::vector<uint32_t>& heights,
					bool useSmoothedVol = false)
				{
					this->isoVal = isoVal;

					auto volDimYxX = static_cast<size_t>(volDim[1]) * volDim[0];
					auto vec2ToSphere = [&](const osg::Vec3& v3) -> osg::Vec3 {
						float dlt = maxLongtitute - minLongtitute;
						float x = volStartFromLonZero == 0 ? v3.x() :
							v3.x() < .5f ? v3.x() + .5f : v3.x() - .5f;
						float lon = minLongtitute + x * dlt;
						dlt = maxLatitute - minLatitute;
						float lat = minLatitute + v3.y() * dlt;
						dlt = maxHeight - minHeight;
						auto h = minHeight + v3.z() * dlt;

						osg::Vec3 ret;
						ret.z() = h * sinf(lat);
						h = h * cosf(lat);
						ret.y() = h * sinf(lon);
						ret.x() = h * cosf(lon);

						return ret;
						};

					std::unordered_map<GLuint, GLuint> edge2vertIDs;
					GLuint prevHeightVertNum = 0;
					auto addLineSeg = [&](const osg::Vec3ui& startPos, const osg::Vec4f& scalars,
						const osg::Vec4f& omegas, uint8_t mask) {
							for (uint8_t i = 0; i < 4; ++i) {
								if (((mask >> i) & 0b1) == 0)
									continue;

								// Edge indexed by Start Voxel Position
								// +----------+
								// | /*\      |
								// |  e1      |
								// |  * e0 *> |
								// +----------+
								// *:   startPos
								// *>:  startPos + (1,0)
								// /*\: startPos + (0,1)
								// ID(e0): [ ID(*) : 63bit | 0 : 1bit ]
								// ID(e1): [ ID(*) : 63bit | 1 : 1bit ]
								auto edgeID = static_cast<GLuint>(startPos.y() + (i == 2 ? 1 : 0)) * volDim[0] +
									startPos.x() + (i == 1 ? 1 : 0);
								edgeID = (edgeID << 1) + (i == 1 || i == 3 ? 1 : 0);
								auto itr = edge2vertIDs.find(edgeID);
								if (itr != edge2vertIDs.end()) {
									vertIndices.push_back(itr->second + prevHeightVertNum);
									continue;
								}

								osg::Vec3 pos(startPos.x() + (i == 0 || i == 2 ? omegas[i]
									: i == 1 ? 1.f
									: 0.f),
									startPos.y() + (i == 1 || i == 3 ? omegas[i]
										: i == 2 ? 1.f
										: 0.f),
									startPos.z());
								for (uint8_t i = 0; i < 3; ++i)
									pos[i] /= volDim[i];

								vertIndices.push_back(verts->size());
								verts->push_back(vec2ToSphere(pos));
								edge2vertIDs.emplace(edgeID, vertIndices.back() - prevHeightVertNum);
							}
						};

					verts->clear();
					vertIndices.clear();

					osg::Vec3ui pos;
					for (auto height : heights) {
						pos.z() = height;

						edge2vertIDs.clear(); // hash map only stores vertices on the same height
						prevHeightVertNum = verts->size();

						for (pos.y() = 0; pos.y() < volDim[1] - 1; ++pos.y())
							for (pos.x() = 0; pos.x() < volDim[0] - 1; ++pos.x()) {
								// Voxels in CCW order form a grid
								// +------------+
								// |  3 <--- 2  |
								// |  |     /|\ |
								// | \|/     |  |
								// |  0 ---> 1  |
								// +------------+
								uint8_t cornerState = 0;
								auto surfStart =
									(useSmoothedVol ? volDatSmoothed->data() : volDat->data())
									+ pos.z() * volDimYxX;
								osg::Vec4 scalars(
									surfStart[pos.y() * volDim[0] + pos.x()],
									surfStart[pos.y() * volDim[0] + pos.x() + 1],
									surfStart[(pos.y() + 1) * volDim[0] + pos.x() + 1],
									surfStart[(pos.y() + 1) * volDim[0] + pos.x()]
								);
								for (uint8_t i = 0; i < 4; ++i)
									if (scalars[i] >= isoVal)
										cornerState |= 1 << i;

								osg::Vec4 omegas(scalars[0] / (scalars[1] + scalars[0]),
									scalars[1] / (scalars[2] + scalars[1]),
									scalars[3] / (scalars[3] + scalars[2]),
									scalars[0] / (scalars[0] + scalars[3]));

								switch (cornerState) {
								case 0b0001:
								case 0b1110:
									addLineSeg(pos, scalars, omegas, 0b1001);
									break;
								case 0b0010:
								case 0b1101:
									addLineSeg(pos, scalars, omegas, 0b0011);
									break;
								case 0b0011:
								case 0b1100:
									addLineSeg(pos, scalars, omegas, 0b1010);
									break;
								case 0b0100:
								case 0b1011:
									addLineSeg(pos, scalars, omegas, 0b0110);
									break;
								case 0b0101:
									addLineSeg(pos, scalars, omegas, 0b0011);
									addLineSeg(pos, scalars, omegas, 0b1100);
									break;
								case 0b1010:
									addLineSeg(pos, scalars, omegas, 0b0110);
									addLineSeg(pos, scalars, omegas, 0b1001);
									break;
								case 0b0110:
								case 0b1001:
									addLineSeg(pos, scalars, omegas, 0b0101);
									break;
								case 0b0111:
								case 0b1000:
									addLineSeg(pos, scalars, omegas, 0b1100);
									break;
								}
							}
					}

					if (vertIndices.empty()) return;

					geom->setVertexArray(verts);

					geom->getPrimitiveSetList().clear();
					geom->addPrimitiveSet(new osg::DrawElementsUInt(
						GL_LINES, vertIndices.size(), vertIndices.data()));
				}
				float GetIsoplethValue() const
				{
					return isoVal;
				}
				std::array<float, 2> GetHeightFromCenterRange() const
				{
					std::array<float, 2> ret = { minHeight, maxHeight };
					return ret;
				}

			private:
				float deg2Rad(float deg)
				{
					return deg * osg::PI / 180.f;
				};

				friend class MarchingSquareCPURenderer;
			};
			std::map<std::string, PerVolParam> vols;

		public:
			MarchingSquareCPURenderer() {}

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
			* -- volDat: 体数据，需按Z-Y-X的顺序存放体素
			* -- volDatSmoothed: 光滑处理过的体数据
			* -- dim: 体数据的三维尺寸（XYZ顺序）
			*/
			void AddVolume(
				const std::string& name,
				std::shared_ptr<std::vector<float>> volDat,
				std::shared_ptr<std::vector<float>> volDatSmoothed,
				const std::array<uint32_t, 3>& volDim)
			{
				auto itr = vols.find(name);
				if (itr != vols.end()) {
					param.grp->removeChild(itr->second.geode);
					vols.erase(itr);
				}
				auto opt = vols.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(name),
					std::forward_as_tuple(volDat, volDatSmoothed, volDim, &param));
				param.grp->addChild(opt.first->second.geode);
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

#endif // !SCIVIS_SCALAR_VISER_MARCHING_SQUARE_RENDERER_H
