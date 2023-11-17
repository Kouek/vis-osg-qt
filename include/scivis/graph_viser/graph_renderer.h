#ifndef SCIVIS_GRAPH_VISER_GRAPH_RENDERER_H
#define SCIVIS_GRAPH_VISER_GRAPH_RENDERER_H

#include <algorithm>
#include <memory>
#include <numeric>
#include <string>

#include <array>
#include <map>
#include <vector>

#include <osg/CullFace>
#include <osg/CoordinateSystemNode>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/LineWidth>
#include <osg/Material>

namespace SciVis
{
	namespace GraphViser
	{

		class GraphRenderer
		{
		public:
			struct Node
			{
				osg::Vec3 pos;
				osg::Vec3 color;
			};
			struct Edge
			{
				std::string from;
				std::string to;
				std::vector<osg::Vec3> subDivs;
			};

		private:
			struct PerRendererParam
			{
				osg::ref_ptr<osg::Group> grp;

				PerRendererParam()
				{
					grp = new osg::Group;
				}
			};
			PerRendererParam param;

			class PerGraphParam
			{
			private:
				float minLongtitute, maxLongtitute;
				float minLatitute, maxLatitute;
				float minHeight, maxHeight;
				float nodeGeomSize;
				bool volStartFromLonZero;

				std::shared_ptr<std::map<std::string, Node>> nodes;
				std::shared_ptr<std::vector<Edge>> edges;

				osg::ref_ptr<osg::Group> grp;

			public:
				PerGraphParam(
					decltype(nodes) nodes,
					decltype(edges) edges,
					PerRendererParam* renderer)
					: nodes(nodes), edges(edges)
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

					grp = new osg::Group;

					auto states = grp->getOrCreateStateSet();
					states->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
				}
				void Update()
				{
					grp->removeChildren(0, grp->getNumChildren());

					osg::Vec3 minPos(
						std::numeric_limits<float>::max(),
						std::numeric_limits<float>::max(),
						std::numeric_limits<float>::max());
					osg::Vec3 maxPos(
						std::numeric_limits<float>::lowest(),
						std::numeric_limits<float>::lowest(),
						std::numeric_limits<float>::lowest());
					auto minMax = [&](const osg::Vec3& p) {
						if (minPos.x() > p.x())
							minPos.x() = p.x();
						if (minPos.y() > p.y())
							minPos.y() = p.y();
						if (minPos.z() > p.z())
							minPos.z() = p.z();

						if (maxPos.x() < p.x())
							maxPos.x() = p.x();
						if (maxPos.y() < p.y())
							maxPos.y() = p.y();
						if (maxPos.z() < p.z())
							maxPos.z() = p.z();
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

					for (auto itr = nodes->begin(); itr != nodes->end(); ++itr)
						minMax(itr->second.pos);
					auto dltPos = maxPos - minPos;

					auto tessl = new osg::TessellationHints;
					tessl->setDetailRatio(1.f);
					std::map<std::string, osg::ShapeDrawable*> osgNodes;
					for (auto itr = nodes->begin(); itr != nodes->end(); ++itr) {
						auto p = itr->second.pos - minPos;
						p.x() = dltPos.x() == 0.f ? p.x() : p.x() / dltPos.x();
						p.y() = dltPos.y() == 0.f ? p.y() : p.y() / dltPos.y();
						p.z() = dltPos.z() == 0.f ? p.z() : p.z() / dltPos.z();
						p = vec3ToSphere(p);

						auto sphere = new osg::ShapeDrawable(
							new osg::Sphere(p, .5f * nodeGeomSize), tessl);
						sphere->setColor(osg::Vec4(itr->second.color, 1.f));

						auto states = grp->getOrCreateStateSet();
						auto matr = new osg::Material;
						matr->setColorMode(osg::Material::DIFFUSE);
						states->setAttributeAndModes(matr, osg::StateAttribute::ON);
						states->setMode(GL_LIGHTING, osg::StateAttribute::ON);

						grp->addChild(sphere);

						osgNodes.emplace(std::make_pair(itr->first, sphere));
					}

					auto segVerts = new osg::Vec3Array;
					auto segCols = new osg::Vec4Array;
					for (auto itr = edges->begin(); itr != edges->end(); ++itr) {
						osg::Vec4 prevColor = osg::Vec4(nodes->at(itr->from).color, 1.f);
						auto dCol = osg::Vec4(nodes->at(itr->to).color, 1.f) - prevColor;
						dCol /= (itr->subDivs.size() == 1 ? 1 : itr->subDivs.size() - 1);

						osg::Vec3 prevPos;
						{
							prevPos = itr->subDivs.front() - minPos;
							prevPos.x() = dltPos.x() == 0.f ? prevPos.x() : prevPos.x() / dltPos.x();
							prevPos.y() = dltPos.y() == 0.f ? prevPos.y() : prevPos.y() / dltPos.y();
							prevPos.z() = dltPos.z() == 0.f ? prevPos.z() : prevPos.z() / dltPos.z();
							prevPos = vec3ToSphere(prevPos);
						}

						for (size_t i = 1; i < itr->subDivs.size(); ++i) {
							segVerts->push_back(prevPos);
							segCols->push_back(prevColor);

							auto p = itr->subDivs[i] - minPos;
							p.x() = dltPos.x() == 0.f ? p.x() : p.x() / dltPos.x();
							p.y() = dltPos.y() == 0.f ? p.y() : p.y() / dltPos.y();
							p.z() = dltPos.z() == 0.f ? p.z() : p.z() / dltPos.z();
							p = vec3ToSphere(p);

							auto color = prevColor + dCol;

							segVerts->push_back(p);
							segCols->push_back(color);

							prevPos = p;
							prevColor = color;
						}
					}

					auto geom = new osg::Geometry;
					{
						geom->setVertexArray(segVerts);
						geom->setColorArray(segCols);
						geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

						auto states = geom->getOrCreateStateSet();
						states->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
						auto lw = new osg::LineWidth(2.f);
						states->setAttribute(lw, osg::StateAttribute::ON);

						geom->addPrimitiveSet(new osg::DrawArrays(
							osg::PrimitiveSet::LINES, 0, segVerts->size()));
					}

					auto geode = new osg::Geode;
					geode->addDrawable(geom);

					grp->addChild(geode);
				}
				bool SetLongtituteRange(float minLonDeg, float maxLonDeg)
				{
					if (minLonDeg < -180.f) return false;
					if (maxLonDeg > +180.f) return false;
					if (minLonDeg >= maxLonDeg) return false;

					minLongtitute = deg2Rad(minLonDeg);
					maxLongtitute = deg2Rad(maxLonDeg);
					return true;
				}
				bool SetLatituteRange(float minLatDeg, float maxLatDeg)
				{
					if (minLatDeg < -90.f) return false;
					if (maxLatDeg > +90.f) return false;
					if (minLatDeg >= maxLatDeg) return false;

					minLatitute = deg2Rad(minLatDeg);
					maxLatitute = deg2Rad(maxLatDeg);
					return true;
				}
				bool SetHeightFromCenterRange(float minH, float maxH)
				{
					if (minH < 0.f) return false;
					if (minH >= maxH) return false;

					minHeight = minH;
					maxHeight = maxH;
					return true;
				}
				void SetGraphStartFromLongtituteZero(bool flag)
				{
					volStartFromLonZero = flag;
				}
				void SetNodeGeometrySize(float sz)
				{
					nodeGeomSize = sz;
				}

			private:
				float deg2Rad(float deg)
				{
					return deg * osg::PI / 180.f;
				};

				friend class GraphRenderer;
			};
			std::map<std::string, PerGraphParam> graphs;

		public:
			GraphRenderer() {}

			/*
			* 函数: GetGroup
			* 功能: 获取该绘制组件的OSG节点
			* 返回值: OSG节点
			*/
			osg::Group* GetGroup() { return param.grp.get(); }
			void AddGraph(
				const std::string& name,
				std::shared_ptr<std::map<std::string, Node>> nodes,
				std::shared_ptr<std::vector<Edge>> edges)
			{
				auto itr = graphs.find(name);
				if (itr != graphs.end()) {
					param.grp->removeChild(itr->second.grp);
					graphs.erase(itr);
				}
				auto opt = graphs.emplace(std::pair<std::string, PerGraphParam>
					(name, PerGraphParam(nodes, edges, &param)));
				param.grp->addChild(opt.first->second.grp);
			}
			PerGraphParam* GetGraph(const std::string& name)
			{
				auto itr = graphs.find(name);
				if (itr == graphs.end())
					return nullptr;
				return &(itr->second);
			}
			std::map<std::string, PerGraphParam>& GetGraphs()
			{
				return graphs;
			}
		};

	}
}

#endif // !SCIVIS_GRAPH_VISER_GRAPH_RENDERER_H
