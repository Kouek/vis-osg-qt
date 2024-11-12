#ifndef SCIVIS_SCALAR_VISER_MARCHING_CUBE_RENDERER_H
#define SCIVIS_SCALAR_VISER_MARCHING_CUBE_RENDERER_H

#include <algorithm>
#include <memory>
#include <numeric>
#include <string>

#include <array>
#include <map>
#include <unordered_map>
#include <set>

#include <osg/CullFace>
#include <osg/CoordinateSystemNode>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture3D>

#include <scivis/common/callback.h>
#include <scivis/common/zhongdian15.h>

#include "marching_cube_table.h"
#include <cassert>

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
			enum class MeshSmoothingType {
				None,
				Laplacian,
				Curvature
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
				float isoVal;
				float minLongtitute, maxLongtitute;
				float minLatitute, maxLatitute;
				float minHeight, maxHeight;
				bool volStartFromLonZero;
				bool useSmoothedVol;
				MeshSmoothingType meshSmoothingType;

				std::shared_ptr<std::vector<float>> volDat;
				std::shared_ptr<std::vector<float>> volDatSmoothed;

				osg::ref_ptr<osg::Geometry> geom;
				osg::ref_ptr<osg::Geode> geode;
				osg::ref_ptr<osg::Vec3Array> verts;
				osg::ref_ptr<osg::Vec3Array> smoothedVerts;
				osg::ref_ptr<osg::Vec3Array> norms;
				osg::ref_ptr<osg::Vec3Array> smoothedNorms;

				std::vector<GLuint> vertIndices;
				std::set<std::array<GLuint, 2>> edges;

			public:
				PerVolParam(
					decltype(volDat) volDat,
					decltype(volDat) volDatSmoothed,
					const std::array<uint32_t, 3>& volDim,
					PerRendererParam* renderer)
					: volDat(volDat), volDatSmoothed(volDatSmoothed), volDim(volDim),
					meshSmoothingType(MeshSmoothingType::None)
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

					verts = new osg::Vec3Array;
					norms = new osg::Vec3Array;
					smoothedVerts = new osg::Vec3Array;
					smoothedNorms = new osg::Vec3Array;

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
					std::array<float, 2> ret{ minHeight, maxHeight };
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
					auto sample = [&](const osg::Vec3i& pos) -> float {
						auto i = pos.z() * volDimYxX + pos.y() * volDim[0] + pos.x();
						if (useSmoothedVol)
							return (*volDatSmoothed)[i];
						return (*volDat)[i];
						};
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

					auto lonExt = maxLongtitute - minLongtitute;
					auto latExt = maxLatitute - minLatitute;
					auto hExt = maxHeight - minHeight;

					struct HashEdge {
						size_t operator()(const std::array<int, 3>& edgeID) const {
							size_t hash = edgeID[0];
							hash = (hash << 32) | edgeID[1];
							hash = (hash << 2) | edgeID[2];
							return std::hash<size_t>()(hash);
						};
					};
					std::array<std::unordered_map<std::array<int, 3>, GLuint, HashEdge>, 2> edge2vertIDs;

					vertIndices.clear();
					verts->clear();
					norms->clear();
					edges.clear();
					osg::Vec3i startPos;
					for (startPos.z() = 0; startPos.z() < volDim[2] - 1; ++startPos.z()) {
						if (startPos.z() != 0) {
							edge2vertIDs[0] = std::move(edge2vertIDs[1]);
							edge2vertIDs[1].clear(); // hash map only stores vertices of 2 consecutive heights
						}

						for (startPos.y() = 0; startPos.y() < volDim[1] - 1; ++startPos.y())
							for (startPos.x() = 0; startPos.x() < volDim[0] - 1; ++startPos.x()) {
								// Voxels in CCW order form a grid
								// +-----------------+
								// |       3 <--- 2  |
								// |       |     /|\ |
								// |      \|/     |  |
								// |       0 ---> 1  |
								// |      /          |
								// |  7 <--- 6       |
								// |  | /   /|\      |
								// | \|/_    |       |
								// |  4 ---> 5       |
								// +-----------------+
								uint8_t cornerState = 0;
								std::array<float, 8> scalars;
								for (int i = 0; i < 8; ++i) {
									scalars[i] = sample(startPos);
									if (scalars[i] >= isoVal)
										cornerState |= 1 << i;

									startPos.x() += i == 0 || i == 4 ? 1 : i == 2 || i == 6 ? -1 : 0;
									startPos.y() += i == 1 || i == 5 ? 1 : i == 3 || i == 7 ? -1 : 0;
									startPos.z() += i == 3 ? 1 : i == 7 ? -1 : 0;
								}
								std::array<float, 12> omegas = {
									scalars[0] / (scalars[1] + scalars[0]),
									scalars[1] / (scalars[2] + scalars[1]),
									scalars[3] / (scalars[3] + scalars[2]),
									scalars[0] / (scalars[0] + scalars[3]),
									scalars[4] / (scalars[5] + scalars[4]),
									scalars[5] / (scalars[6] + scalars[5]),
									scalars[7] / (scalars[7] + scalars[6]),
									scalars[4] / (scalars[4] + scalars[7]),
									scalars[0] / (scalars[0] + scalars[4]),
									scalars[1] / (scalars[1] + scalars[5]),
									scalars[2] / (scalars[2] + scalars[6]),
									scalars[3] / (scalars[3] + scalars[7])
								};

								// Edge indexed by Start Voxel Position
								// +----------+
								// | /*\  *|  |
								// |  |  /    |
								// | e1 e2    |
								// |  * e0 *> |
								// +----------+
								// *:   startPos
								// *>:  startPos + (1,0,0)
								// /*\: startPos + (0,1,0)
								// *|:  startPos + (0,0,1)
								// ID(e0) = (startPos.xy, 00)
								// ID(e1) = (startPos.xy, 01)
								// ID(e2) = (startPos.xy, 10)
								for (uint32_t i = 0; i < VertNumTable[cornerState]; i += 3) {
									for (int32_t ii = 0; ii < 3; ++ii) {
										auto ei = TriangleTable[cornerState][i + ii];
										std::array<int, 3> edgeID = {
											startPos.x() + (ei == 1 || ei == 5 || ei == 9 || ei == 10 ? 1 : 0),
											startPos.y() + (ei == 2 || ei == 6 || ei == 10 || ei == 11 ? 1 : 0),
											ei >= 8 ? 2
											: ei == 1 || ei == 3 || ei == 5 || ei == 7 ? 1
											: 0
										};
										auto edge2vertIDIdx = ei >= 4 && ei < 8 ? 1 : 0;
										auto itr = edge2vertIDs[edge2vertIDIdx].find(edgeID);
										if (itr != edge2vertIDs[edge2vertIDIdx].end()) {
											vertIndices.emplace_back(itr->second);
											continue;
										}

										osg::Vec3 pos(
											startPos.x() + (ei == 0 || ei == 2 || ei == 4 || ei == 6
												? omegas[ei]
												: ei == 1 || ei == 5 || ei == 9 || ei == 10 ? 1.f
												: 0.f),
											startPos.y() + (ei == 1 || ei == 3 || ei == 5 || ei == 7
												? omegas[ei]
												: ei == 2 || ei == 6 || ei == 10 || ei == 11 ? 1.f
												: 0.f),
											startPos.z() + (ei >= 8
												? omegas[ei]
												: ei >= 4 ? 1.f
												: 0.f));
										pos.x() /= volDim[0];
										pos.y() /= volDim[1];
										pos.z() /= volDim[2];
										pos = vec3ToSphere(pos);

										float scalar;
										switch (ei) {
										case 0:
											scalar = omegas[0] * scalars[0] + (1.f - omegas[0]) * scalars[1];
											break;
										case 1:
											scalar = omegas[1] * scalars[1] + (1.f - omegas[1]) * scalars[2];
											break;
										case 2:
											scalar = omegas[2] * scalars[3] + (1.f - omegas[2]) * scalars[2];
											break;
										case 3:
											scalar = omegas[3] * scalars[0] + (1.f - omegas[3]) * scalars[3];
											break;
										case 4:
											scalar = omegas[4] * scalars[4] + (1.f - omegas[4]) * scalars[5];
											break;
										case 5:
											scalar = omegas[5] * scalars[5] + (1.f - omegas[5]) * scalars[6];
											break;
										case 6:
											scalar = omegas[6] * scalars[7] + (1.f - omegas[6]) * scalars[6];
											break;
										case 7:
											scalar = omegas[7] * scalars[4] + (1.f - omegas[7]) * scalars[7];
											break;
										default:
											scalar = omegas[ei] * scalars[ei - 8] + (1.f - omegas[ei]) * scalars[ei - 4];
										}

										vertIndices.emplace_back(verts->size());
										verts->push_back(pos);
										norms->push_back(osg::Vec3(0.f, 0.f, 0.f));
										edge2vertIDs[edge2vertIDIdx].emplace(edgeID, vertIndices.back());
									}

									std::array<GLuint, 3> triVertIdxs = {
										vertIndices[vertIndices.size() - 3],
										vertIndices[vertIndices.size() - 2],
										vertIndices[vertIndices.size() - 1]
									};
									osg::Vec3 norm;
									{
										auto e0 = (*verts)[triVertIdxs[1]] -
											(*verts)[triVertIdxs[0]];
										auto e1 = (*verts)[triVertIdxs[2]] -
											(*verts)[triVertIdxs[0]];
										norm = e1 ^ e0;
										norm.normalize();
									}

									(*norms)[triVertIdxs[0]] += norm;
									(*norms)[triVertIdxs[1]] += norm;
									(*norms)[triVertIdxs[2]] += norm;

									edges.emplace(std::array<GLuint, 2>{triVertIdxs[0], triVertIdxs[1]});
									edges.emplace(std::array<GLuint, 2>{triVertIdxs[1], triVertIdxs[0]});
									edges.emplace(std::array<GLuint, 2>{triVertIdxs[1], triVertIdxs[2]});
									edges.emplace(std::array<GLuint, 2>{triVertIdxs[2], triVertIdxs[1]});
									edges.emplace(std::array<GLuint, 2>{triVertIdxs[2], triVertIdxs[0]});
									edges.emplace(std::array<GLuint, 2>{triVertIdxs[0], triVertIdxs[2]});
								}
							}
					}

					for (auto& norm : *norms)
						norm.normalize();

					updateGeometry();
				}
				void SetMeshSmoothingType(MeshSmoothingType type) {
					if (meshSmoothingType == type) return;

					meshSmoothingType = type;
					updateGeometry();
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
				void updateGeometry() {
					if (vertIndices.empty())
						return;

					auto laplacianSmooth = [&]() {
						for (GLuint vIdx = 0; vIdx < verts->size(); ++vIdx) {
							auto itr = edges.lower_bound(std::array<GLuint, 2>{vIdx, 0});
							assert(itr != edges.end());

							auto& smoothedVert = (*smoothedVerts)[vIdx];
							auto& smoothedNorm = (*smoothedNorms)[vIdx];
							int lnkNum = 1;
							while (itr != edges.end() && (*itr)[0] == vIdx) {
								smoothedVert += (*verts)[(*itr)[1]];
								smoothedNorm += (*norms)[(*itr)[1]];
								++itr;
								++lnkNum;
							}
							smoothedVert /= lnkNum;
							smoothedNorm /= lnkNum;
						}
						};
					auto curvatureSmooth = [&]() {
						for (GLuint vIdx = 0; vIdx < verts->size(); ++vIdx) {
							auto itr = edges.lower_bound(std::array<GLuint, 2>{vIdx, 0});
							assert(itr != edges.end());

							auto& smoothedVert = (*smoothedVerts)[vIdx];
							auto& norm = (*smoothedNorms)[vIdx];
							auto projLen = 0.f;
							while (itr != edges.end() && (*itr)[0] == vIdx) {
								auto dlt = (*verts)[(*itr)[1]] - smoothedVert;
								projLen = dlt * norm;
								++itr;
							}

							smoothedVert = smoothedVert + norm * projLen;
						}
						};

					if (meshSmoothingType != MeshSmoothingType::None)
						switch (meshSmoothingType) {
						case MeshSmoothingType::Laplacian:
						case MeshSmoothingType::Curvature:
							smoothedVerts->assign(verts->begin(), verts->end());
							smoothedNorms->assign(norms->begin(), norms->end());
							break;
						}
					switch (meshSmoothingType) {
					case MeshSmoothingType::Laplacian:
						laplacianSmooth();
						break;
					case MeshSmoothingType::Curvature:
						curvatureSmooth();
						break;
					}

					switch (meshSmoothingType) {
					case MeshSmoothingType::Laplacian:
					case MeshSmoothingType::Curvature:
						geom->setVertexArray(smoothedVerts);
						geom->setNormalArray(smoothedNorms);
						break;
					default:
						geom->setVertexArray(verts);
						geom->setNormalArray(norms);
					}

					geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

					geom->getPrimitiveSetList().clear();
					geom->addPrimitiveSet(
						new osg::DrawElementsUInt(
							GL_TRIANGLES, vertIndices.size(), vertIndices.data()));
				}

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
