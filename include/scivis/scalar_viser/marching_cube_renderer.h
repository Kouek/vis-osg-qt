#ifndef SCIVIS_SCALAR_VISER_MARCHING_CUBE_RENDERER_H
#define SCIVIS_SCALAR_VISER_MARCHING_CUBE_RENDERER_H

#include <algorithm>
#include <memory>
#include <numeric>
#include <string>

#include <array>
#include <map>
#include <unordered_set>

#include <osg/CullFace>
#include <osg/CoordinateSystemNode>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture3D>

#include <scivis/common/callback.h>
#include <scivis/common/zhongdian15.h>

#include "marching_cube_table.h"

namespace SciVis
{
	namespace ScalarViser
	{

		class MarchingCubeRenderer
		{
		public:
			struct ShadingParam {
				bool useShading;
				float ka;
				float kd;
				float ks;
				float shininess;
				osg::Vec3 lightPos;
			};

		private:
			struct PerRendererParam
			{
				osg::ref_ptr<osg::Group> grp;
				osg::ref_ptr<osg::Program> program;

				osg::ref_ptr<osg::Uniform> eyePos;
				osg::ref_ptr<osg::Uniform> useShading;
				osg::ref_ptr<osg::Uniform> ka;
				osg::ref_ptr<osg::Uniform> kd;
				osg::ref_ptr<osg::Uniform> ks;
				osg::ref_ptr<osg::Uniform> shininess;
				osg::ref_ptr<osg::Uniform> lightPos;

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

				PerRendererParam()
				{
					grp = new osg::Group;
					osg::ref_ptr<osg::Shader> vertShader = osg::Shader::readShaderFile(
						osg::Shader::VERTEX,
						SciVis::GetDataPathPrefix() +
						SCIVIS_SHADER_PREFIX
						"scivis/scalar_viser/mcb_vert.glsl");
					osg::ref_ptr<osg::Shader> fragShader = osg::Shader::readShaderFile(
						osg::Shader::FRAGMENT,
						SciVis::GetDataPathPrefix() +
						SCIVIS_SHADER_PREFIX
						"scivis/scalar_viser/mcb_frag.glsl");
					program = new osg::Program;
					program->addShader(vertShader);
					program->addShader(fragShader);

#define STATEMENT(name, val) name = new osg::Uniform(#name, val)
					STATEMENT(eyePos, osg::Vec3());

					STATEMENT(useShading, 0);
					STATEMENT(ka, .5f);
					STATEMENT(kd, .5f);
					STATEMENT(ks, .5f);
					STATEMENT(shininess, 16.f);
					STATEMENT(lightPos, osg::Vec3());
#undef STATEMENT

					grp->setCullCallback(new Callback(eyePos));
				}
			};
			PerRendererParam param;

			class PerVolParam
			{
			private:
				std::array<uint32_t, 3> volDim;
				osg::Vec3 voxSz;
				float isoVal;
				float minLongtitute, maxLongtitute;
				float minLatitute, maxLatitute;
				float minHeight, maxHeight;
				bool volStartFromLonZero;
				bool useSmoothedVol;

				std::shared_ptr<std::vector<float>> volDat;
				std::shared_ptr<std::vector<float>> volDatSmoothed;

				osg::ref_ptr<osg::Geometry> geom;
				osg::ref_ptr<osg::Geode> geode;
				osg::ref_ptr<osg::Vec3Array> verts;
				osg::ref_ptr<osg::Vec3Array> norms;

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
					useSmoothedVol = false;

					voxSz = osg::Vec3(1.f / volDim[0], 1.f / volDim[1], 1.f / volDim[2]);

					verts = new osg::Vec3Array;
					norms = new osg::Vec3Array;

					geom = new osg::Geometry;
					geode = new osg::Geode;
					geode->addDrawable(geom);

					auto states = geode->getOrCreateStateSet();

					states->addUniform(renderer->eyePos);
					states->addUniform(renderer->useShading);
					states->addUniform(renderer->ka);
					states->addUniform(renderer->kd);
					states->addUniform(renderer->ks);
					states->addUniform(renderer->shininess);
					states->addUniform(renderer->lightPos);

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
					return true;
				}
				std::array<float, 2> GetLongtituteRange() const
				{
					std::array<float, 2> ret{ minLongtitute, maxLongtitute };
					return ret;
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
					return true;
				}
				std::array<float, 2> GetLatituteRange() const
				{
					std::array<float, 2> ret{ minLatitute, maxLatitute };
					return ret;
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
					return true;
				}
				std::array<float, 2> GetHeightFromCenterRange() const
				{
					std::array<float, 2> ret{ minHeight, maxHeight};
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
					volStartFromLonZero = flag;
				}
				/*
				* 函数: MarchingCube
				* 功能: 在CPU端执行Marching Cube算法，产生等值面
				* 参数:
				* -- isoVal: 产生等值面依据的标量值
				* -- useSmoothedVol: 为true时，使用平滑的体数据
				*/
				void MarchingCube(float isoVal, bool useSmoothedVol = false)
				{
					if (this->isoVal == isoVal && this->useSmoothedVol == useSmoothedVol)
						return;

					this->isoVal = isoVal;
					this->useSmoothedVol = useSmoothedVol;

					auto volDimYxX = static_cast<size_t>(volDim[1]) * volDim[0];
					auto sample = [&](uint32_t x, uint32_t y, uint32_t z) -> float {
						x = std::min(x, volDim[0] - 1);
						y = std::min(y, volDim[1] - 1);
						z = std::min(z, volDim[2] - 1);
						auto i = z * volDimYxX + y * volDim[0] + x;
						if (useSmoothedVol)
							return (*volDatSmoothed)[i];
						return (*volDat)[i];
					};
					auto cmptField = [&](int x, int y, int z)->std::array<float, 8> {
						std::array<float, 8> field;
						field[0] = sample(x + 0, y + 0, z + 0);
						field[1] = sample(x + 1, y + 0, z + 0);
						field[2] = sample(x + 1, y + 1, z + 0);
						field[3] = sample(x + 0, y + 1, z + 0);
						field[4] = sample(x + 0, y + 0, z + 1);
						field[5] = sample(x + 1, y + 0, z + 1);
						field[6] = sample(x + 1, y + 1, z + 1);
						field[7] = sample(x + 0, y + 1, z + 1);
						return field;
					};
					auto cmptCubeIdx = [&](const std::array<float, 8>& field) -> uint32_t {
						uint32_t cubeIdx = 0;
						cubeIdx |= field[0] < isoVal ? (1 << 0) : 0;
						cubeIdx |= field[1] < isoVal ? (1 << 1) : 0;
						cubeIdx |= field[2] < isoVal ? (1 << 2) : 0;
						cubeIdx |= field[3] < isoVal ? (1 << 3) : 0;
						cubeIdx |= field[4] < isoVal ? (1 << 4) : 0;
						cubeIdx |= field[5] < isoVal ? (1 << 5) : 0;
						cubeIdx |= field[6] < isoVal ? (1 << 6) : 0;
						cubeIdx |= field[7] < isoVal ? (1 << 7) : 0;
						return cubeIdx;
					};

					std::vector<size_t> voxVertNums(volDat->size(), 0);
					for (size_t i = 0; i < volDat->size(); ++i) {
						uint32_t z = i / volDimYxX;
						uint32_t y = (i - z * volDimYxX) / volDim[0];
						uint32_t x = i - z * volDimYxX - y * volDim[0];

						auto field = cmptField(x, y, z);
						auto cubeIdx = cmptCubeIdx(field);

						voxVertNums[i] = VertNumTable[cubeIdx];
					}

					auto vertNum = std::accumulate(voxVertNums.begin(), voxVertNums.end(), 0);

					verts->clear();
					verts->reserve(vertNum);
					norms->clear();
					norms->reserve(vertNum);

					for (size_t i = 0; i < volDat->size(); ++i) {
						if (voxVertNums[i] == 0)
							continue;

						uint32_t z = i / volDimYxX;
						uint32_t y = (i - z * volDimYxX) / volDim[0];
						uint32_t x = i - z * volDimYxX - y * volDim[0];

						std::array<osg::Vec3, 8> v;
						{
							osg::Vec3f p(x * voxSz.x(), y * voxSz.y(), z * voxSz.z());
							v[0] = p;
							v[1] = p + osg::Vec3(voxSz.x(), 0.f, 0.f);
							v[2] = p + osg::Vec3(voxSz.x(), voxSz.y(), 0.f);
							v[3] = p + osg::Vec3(0.f, voxSz.y(), 0.f);
							v[4] = p + osg::Vec3(0.f, 0.f, voxSz.z());
							v[5] = p + osg::Vec3(voxSz.x(), 0.f, voxSz.z());
							v[6] = p + osg::Vec3(voxSz.x(), voxSz.y(), voxSz.z());
							v[7] = p + osg::Vec3(0.f, voxSz.y(), voxSz.z());
						}
						auto field = cmptField(x, y, z);
						auto cubeIdx = cmptCubeIdx(field);

						std::array<osg::Vec3, 12> vertList;
						auto vertInterp = [&](const osg::Vec3& p0, const osg::Vec3& p1, float f0,
							float f1) -> osg::Vec3 {
								float t = (isoVal - f0) / (f1 - f0);
								auto dlt = p1 - p0;
								return osg::Vec3(p0.x() + t * dlt.x(), p0.y() + t * dlt.y(),
									p0.z() + t * dlt.z());
						};
						vertList[0] = vertInterp(v[0], v[1], field[0], field[1]);
						vertList[1] = vertInterp(v[1], v[2], field[1], field[2]);
						vertList[2] = vertInterp(v[2], v[3], field[2], field[3]);
						vertList[3] = vertInterp(v[3], v[0], field[3], field[0]);

						vertList[4] = vertInterp(v[4], v[5], field[4], field[5]);
						vertList[5] = vertInterp(v[5], v[6], field[5], field[6]);
						vertList[6] = vertInterp(v[6], v[7], field[6], field[7]);
						vertList[7] = vertInterp(v[7], v[4], field[7], field[4]);

						vertList[8] = vertInterp(v[0], v[4], field[0], field[4]);
						vertList[9] = vertInterp(v[1], v[5], field[1], field[5]);
						vertList[10] = vertInterp(v[2], v[6], field[2], field[6]);
						vertList[11] = vertInterp(v[3], v[7], field[3], field[7]);

						auto vec3ToSphere = [&](const osg::Vec3& v3) -> osg::Vec3 {
							float dlt = maxLongtitute - minLongtitute;
							float x = volStartFromLonZero == 0 ? v3.x() :
								v3.x() < .5f ? v3.x() + .5f : v3.x() - .5f;
							float lon = minLongtitute + x * dlt;
							dlt = maxLatitute - minLatitute;
							float lat = minLatitute + v3.y() * dlt;
							dlt = maxHeight - minHeight;
							float h = minHeight + v3.z() * dlt;

							osg::Vec3 ret;
							ret.z() = h * sinf(lat);
							h = h * cosf(lat);
							ret.y() = h * sinf(lon);
							ret.x() = h * cosf(lon);

							return ret;
						};
						for (uint32_t j = 0; j < voxVertNums[i]; j += 3) {
							auto edge = TriangleTable[cubeIdx][j];
							verts->push_back(vec3ToSphere(vertList[edge]));
							edge = TriangleTable[cubeIdx][j + 1];
							verts->push_back(vec3ToSphere(vertList[edge]));
							edge = TriangleTable[cubeIdx][j + 2];
							verts->push_back(vec3ToSphere(vertList[edge]));

							auto n = verts->size();
							auto e0 = (*verts)[n - 2] - (*verts)[n - 3];
							auto e1 = (*verts)[n - 1] - (*verts)[n - 3];
							e0 = e0 ^ e1;
							e0.normalize();
							norms->push_back(e0);
							norms->push_back(e0);
							norms->push_back(e0);
						}
					}

					geom->setVertexArray(verts);
					geom->setNormalArray(norms);
					geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

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

				friend class MarchingCubeRenderer;
			};
			std::map<std::string, PerVolParam> vols;

		public:
			MarchingCubeRenderer() {}

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
				auto opt = vols.emplace(std::pair<std::string, PerVolParam>
					(name, PerVolParam(volDat, volDatSmoothed, volDim, &param)));
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
			/*
			* 函数: SetShading
			* 功能: 设置绘制中的光照着色参数
			* 参数:
			* -- param: Blinn-Phong光照着色参数
			*/
			void SetShading(const ShadingParam& param)
			{
				if (!param.useShading)
					this->param.useShading->set(0);
				else {
					this->param.useShading->set(1);
					this->param.ka->set(param.ka);
					this->param.kd->set(param.kd);
					this->param.ks->set(param.ks);
					this->param.shininess->set(param.shininess);
					this->param.lightPos->set(param.lightPos);
				}
			}
		};

	} // namespace ScalarViser
} // namespace SciVis

#endif // !SCIVIS_SCALAR_VISER_MARCHING_CUBE_RENDERER_H
