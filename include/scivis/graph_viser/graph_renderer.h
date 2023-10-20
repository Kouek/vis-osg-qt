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

namespace SciVis
{
	namespace GraphViser
	{

		class GraphRenderer
		{
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
				bool volStartFromLonZero;

				std::shared_ptr<std::map<std::string, osg::Vec3>> nodes;
				std::shared_ptr<std::vector<std::array<std::string, 2>>> edges;

				osg::ref_ptr<osg::Group> grp;

			public:
				PerGraphParam(
					std::shared_ptr<std::map<std::string, osg::Vec3>> nodes,
					std::shared_ptr<std::vector<std::array<std::string, 2>>> edges,
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
				}
				void Update()
				{
					osg::Vec3 minPos(
						std::numeric_limits<float>::max(),
						std::numeric_limits<float>::max(),
						std::numeric_limits<float>::max());
					osg::Vec3 maxPos(
						std::numeric_limits<float>::min(),
						std::numeric_limits<float>::min(),
						std::numeric_limits<float>::min());
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
						minMax(itr->second);

					grp->removeChildren(0, grp->getNumChildren());
					auto dltPos = maxPos - minPos;
					auto tessl = new osg::TessellationHints;
					tessl->setDetailRatio(4.f);
					for (auto itr = nodes->begin(); itr != nodes->end(); ++itr) {
						auto p = itr->second - minPos;
						p.x() /= dltPos.x();
						p.y() /= dltPos.y();
						p.z() /= dltPos.z();
						p = vec3ToSphere(p);

						auto sphere = new osg::ShapeDrawable(new osg::Sphere(p, 10.f), tessl);
						grp->addChild(sphere);
					}
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
				void SetVolumeStartFromLongtituteZero(bool flag)
				{
					volStartFromLonZero = flag;
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
				std::shared_ptr<std::map<std::string, osg::Vec3>> nodes,
				std::shared_ptr<std::vector<std::array<std::string, 2>>> edges)
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
