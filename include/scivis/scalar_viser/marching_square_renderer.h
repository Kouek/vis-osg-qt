#ifndef SCIVIS_SCALAR_VISER_MCR_H
#define SCIVIS_SCALAR_VISER_MCR_H

#include <algorithm>
#include <memory>
#include <numeric>
#include <string>

#include <array>
#include <unordered_set>
#include <map>

#include <osg/CullFace>
#include <osg/CoordinateSystemNode>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture3D>

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
						SCIVIS_SHADER_PREFIX
						"scivis/scalar_viser/mcsqr_vert.glsl");
					osg::ref_ptr<osg::Shader> fragShader = osg::Shader::readShaderFile(
						osg::Shader::FRAGMENT,
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

				osg::ref_ptr<osg::Geometry> geom;
				osg::ref_ptr<osg::Geode> geode;
				osg::ref_ptr<osg::Vec3Array> verts;

			public:
				PerVolParam(decltype(volDat) volDat, const std::array<uint32_t, 3>& volDim,
					PerRendererParam* renderer)
					: volDat(volDat), volDim(volDim)
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
				* -- heights: 需要产生等值线的面的高度
				*/
				void MarchingSquare(float isoVal, const std::vector<float>& heights)
				{
					this->isoVal = isoVal;

					auto volDimYxX = static_cast<size_t>(volDim[1]) * volDim[0];
					auto vertInterp = [&](const osg::Vec2& p0, const osg::Vec2& p1,
						float t) -> osg::Vec2 {
							auto oneMinusT = 1.f - t;
							return osg::Vec2(
								oneMinusT * p0.x() + t * p1.x(),
								oneMinusT * p0.y() + t * p1.y());
					};
					auto vec2ToSphere = [&](const osg::Vec2& v2, float h) -> osg::Vec3 {
						float dlt = maxLongtitute - minLongtitute;
						float x = volStartFromLonZero == 0 ? v2.x() :
							v2.x() < .5f ? v2.x() + .5f : v2.x() - .5f;
						float lon = minLongtitute + x * dlt;
						dlt = maxLatitute - minLatitute;
						float lat = minLatitute + v2.y() * dlt;
						dlt = maxHeight - minHeight;
						h = minHeight + h * dlt;

						osg::Vec3 ret;
						ret.z() = h * sinf(lat);
						h = h * cosf(lat);
						ret.y() = h * sinf(lon);
						ret.x() = h * cosf(lon);

						return ret;
					};
					auto addTriangle = [&](
						const osg::Vec2& v0, const osg::Vec2& v1,
						const osg::Vec2& v2, float h) {
							auto _v0 = vec2ToSphere(v0, h);
							auto _v1 = vec2ToSphere(v1, h);
							auto _v2 = vec2ToSphere(v2, h);

							verts->push_back(_v0);
							verts->push_back(_v1);
							verts->push_back(_v2);
					};
					auto addQuad = [&](
						const osg::Vec2& v0, const osg::Vec2& v1,
						const osg::Vec2& v2, const osg::Vec2& v3, float h
						) {
							auto _v0 = vec2ToSphere(v0, h);
							auto _v1 = vec2ToSphere(v1, h);
							auto _v2 = vec2ToSphere(v2, h);
							auto _v3 = vec2ToSphere(v3, h);

							verts->push_back(_v0);
							verts->push_back(_v1);
							verts->push_back(_v2);
							verts->push_back(_v0);
							verts->push_back(_v2);
							verts->push_back(_v3);
					};
					auto addPentagon = [&](
						const osg::Vec2& v0, const osg::Vec2& v1,
						const osg::Vec2& v2, const osg::Vec2& v3,
						const osg::Vec2& v4, float h
						) {
							auto _v0 = vec2ToSphere(v0, h);
							auto _v1 = vec2ToSphere(v1, h);
							auto _v2 = vec2ToSphere(v2, h);
							auto _v3 = vec2ToSphere(v3, h);
							auto _v4 = vec2ToSphere(v4, h);

							verts->push_back(_v0);
							verts->push_back(_v1);
							verts->push_back(_v2);
							verts->push_back(_v0);
							verts->push_back(_v2);
							verts->push_back(_v3);
							verts->push_back(_v0);
							verts->push_back(_v3);
							verts->push_back(_v4);
					};
					auto mcsqrVox = [&](uint32_t x, uint32_t y, uint32_t z, float h) {
						/* 下标
						*  ^
						* [2]--3--[3]
						*  |      |
						*  2      1
						*  |      |
						* [0]--0--[1] >
						*/
						auto surfStart = volDat->data() + z * volDimYxX;
						std::array<float, 4> val4 = {
							surfStart[y * volDim[0] + x],
							surfStart[y * volDim[0] + x + 1],
							surfStart[(y + 1) * volDim[0] + x],
							surfStart[(y + 1) * volDim[0] + x + 1]
						};
						std::array<float, 4> t4 = {
							(isoVal - val4[0]) / (val4[1] - val4[0]),
							(isoVal - val4[1]) / (val4[3] - val4[1]),
							(isoVal - val4[0]) / (val4[2] - val4[0]),
							(isoVal - val4[2]) / (val4[3] - val4[2])
						};
						std::array<osg::Vec2, 4> pos4;
						pos4[0] = osg::Vec2(x * voxSz.x(), y * voxSz.y());
						pos4[1] = pos4[0] + osg::Vec2(voxSz.x(), 0.f);
						pos4[2] = pos4[0] + osg::Vec2(0.f, voxSz.y());
						pos4[3] = pos4[0] + osg::Vec2(voxSz.x(), voxSz.y());
						std::array<osg::Vec2, 4> vert4 = {
							vertInterp(pos4[0], pos4[1], t4[0]),
							vertInterp(pos4[1], pos4[3], t4[1]),
							vertInterp(pos4[0], pos4[2], t4[2]),
							vertInterp(pos4[2], pos4[3], t4[3])
						};

						uint8_t flag = 0;
						if (val4[0] >= isoVal) flag |= 0b0001;
						if (val4[1] >= isoVal) flag |= 0b0010;
						if (val4[2] >= isoVal) flag |= 0b0100;
						if (val4[3] >= isoVal) flag |= 0b1000;
						switch (flag) {
						case 0: break;
						case 1: addTriangle(vert4[0], vert4[2], pos4[0], h); break;
						case 2: addTriangle(vert4[1], vert4[0], pos4[1], h); break;
						case 4: addTriangle(vert4[2], vert4[3], pos4[2], h); break;
						case 8: addTriangle(vert4[3], vert4[1], pos4[3], h); break;
						case 3: addQuad(pos4[1], vert4[1], vert4[2], pos4[0], h); break;
						case 5: addQuad(vert4[0], vert4[3], pos4[2], pos4[0], h); break;
						case 10: addQuad(pos4[1], pos4[3], vert4[3], vert4[0], h); break;
						case 12: addQuad(vert4[1], pos4[3], pos4[2], vert4[2], h); break;
						case  7: addPentagon(pos4[1], vert4[1], vert4[3], pos4[2], pos4[0], h); break;
						case 11: addPentagon(pos4[3], vert4[3], vert4[2], pos4[0], pos4[1], h); break;
						case 13: addPentagon(pos4[0], vert4[0], vert4[1], pos4[3], pos4[2], h); break;
						case 14: addPentagon(pos4[2], vert4[2], vert4[0], pos4[1], pos4[3], h); break;
						case 6:
							addTriangle(vert4[1], vert4[0], pos4[1], h);
							addTriangle(vert4[2], vert4[3], pos4[2], h);
							break;
						case 9:
							addTriangle(vert4[0], vert4[2], pos4[0], h);
							addTriangle(vert4[3], vert4[1], pos4[3], h);
							break;
						}
					};

					verts->clear();
					std::unordered_set<uint32_t> mcsqredHs;
					auto hDlt = maxHeight - minHeight;
					for (size_t hsi = 0; hsi < heights.size(); ++hsi) {
						auto h = (heights[hsi] - minHeight) / hDlt;
						if (h < 0.f || h >= 1.f) continue;
						auto z = static_cast<uint32_t>(h * volDim[2]);
						if (mcsqredHs.find(z) != mcsqredHs.end()) continue;
						mcsqredHs.emplace(z);

						for (uint32_t y = 0; y < volDim[1] - 1; ++y)
							for (uint32_t x = 0; x < volDim[0] - 1; ++x)
								mcsqrVox(x, y, z, h);
					}

					geom->setVertexArray(verts);

					geom->getPrimitiveSetList().clear();
					geom->addPrimitiveSet(new osg::DrawArrays(
						osg::PrimitiveSet::TRIANGLES, 0, verts->size()));
				}
				float GetIsosurfaceValue() const
				{
					return isoVal;
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
			* -- dim: 体数据的三维尺寸（XYZ顺序）
			*/
			void AddVolume(const std::string& name, std::shared_ptr<std::vector<float>> volDat,
				const std::array<uint32_t, 3>& volDim)
			{
				auto itr = vols.find(name);
				if (itr != vols.end()) {
					param.grp->removeChild(itr->second.geode);
					vols.erase(itr);
				}
				auto opt = vols.emplace(std::pair<std::string, PerVolParam>
					(name, PerVolParam(volDat, volDim, &param)));
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

#endif // !SCIVIS_SCALAR_VISER_MCR_H
