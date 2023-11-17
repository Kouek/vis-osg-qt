#include <memory>

#include <array>

#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>

#include <common/osg.h>

#include <scivis/common/zhongdian15.h>
#include <scivis/graph_viser/graph_layout_2D.h>
#include <scivis/graph_viser/graph_renderer.h>

static const std::string nodesPath = DATA_PATH_PREFIX"network/nodes.csv";
static const std::string edgesPath = DATA_PATH_PREFIX"network/edges.csv";
static std::string graphName = "network";
static std::array<float, 2> lonRng = { -60.f, -30.f };
const std::array<float, 2> latRng = { -20.f, 20.f };
const std::array<float, 2> hRng = { 10000.f, 15000.f };
const float hScale = 100.f;

int main(int argc, char** argv)
{
	auto* viewer = new osgViewer::Viewer;
	viewer->setUpViewInWindow(200, 50, 800, 600);
	auto* manipulator = new osgGA::TrackballManipulator;
	viewer->setCameraManipulator(manipulator);

	osg::ref_ptr<osg::Group> grp = new osg::Group;
	grp->addChild(createEarth());

	std::string errMsg;

	std::unordered_set<int> nodesNotMoved;
	nodesNotMoved.emplace(25);

	auto renderer = std::make_shared<SciVis::GraphViser::GraphRenderer>();

	SciVis::GraphViser::Graph graph;
	std::vector<osg::Vec3> colors;

	auto initGraph = [&]() {
		SciVis::GraphLoader::MkFileManager fm;
		auto nodes = fm.ReadGraphNodes(SciVis::GetDataPathPrefix() + nodesPath);
		auto edges = fm.ReadGraphEdges(
			SciVis::GetDataPathPrefix() + nodesPath, SciVis::GetDataPathPrefix() + edgesPath);

		graph.Set(nodes, edges);

		colors.resize(graph.GetNodes().size());
		for (auto& col : colors) {
			col.x() = 1.f * rand() / RAND_MAX;
			col.y() = 1.f * rand() / RAND_MAX;
			col.z() = 1.f * rand() / RAND_MAX;
		}
	};
	auto layoutGraph = [&]() {
		graph.SetNodesNotMoved(nodesNotMoved);

		graph.n_iterations = 0;
		while (graph.n_iterations < 1000) {
			graph.Update(.1);
		}
	};
	auto edgeClusterGraph = [&]() {
		graph.SetAlgorithmParams(0.1, 5, 90, 0.6, 3);
		graph.SetNetworkParams(-1.0, -1.0);
		SciVis::Vec2D gravitationCenter;
		gravitationCenter.set(0.0, 0.0);
		graph.SetPhysicsParams(0.4, 1e-4, gravitationCenter, 1.0);

		do {
			while (graph.Iterate() > 0);
			graph.AddSubvisions();
		} while (graph.UpdateCycle() > 0);

		graph.Smooth();
	};
	auto loadGraphToOSG = [&]() {
		auto nodes = std::make_shared<std::map<std::string, SciVis::GraphViser::GraphRenderer::Node>>();
		auto edges = std::make_shared<std::vector<SciVis::GraphViser::GraphRenderer::Edge>>();

		size_t i = 0;
		for (auto itr = graph.GetNodes().begin(); itr != graph.GetNodes().end(); ++itr) {
			SciVis::GraphViser::GraphRenderer::Node node;
			node.pos = osg::Vec3(itr->second.pos.X(), itr->second.pos.Y(), 1.f);
			node.color = colors[i];

			nodes->emplace(std::make_pair(itr->first, node));
			++i;
		}

		for (auto itr = graph.GetEdges().begin(); itr != graph.GetEdges().end(); ++itr) {
			edges->emplace_back();

			auto& edge = edges->back();
			edge.from = itr->sourceLabel;
			edge.to = itr->targetLabel;
			if (itr->subdivs.empty()) {
				edge.subDivs.emplace_back(osg::Vec3(itr->start.X(), itr->start.Y(), 1.f));
				edge.subDivs.emplace_back(osg::Vec3(itr->end.X(), itr->end.Y(), 1.f));
			}
			else {
				edge.subDivs.emplace_back(osg::Vec3(itr->start.X(), itr->start.Y(), 1.f));
				for (auto& subdiv : itr->subdivs)
					edge.subDivs.emplace_back(osg::Vec3(subdiv.X(), subdiv.Y(), 1.f));
				edge.subDivs.emplace_back(osg::Vec3(itr->end.X(), itr->end.Y(), 1.f));
			}
		}

		renderer->AddGraph(graphName, nodes, edges);
	};
	auto initOSGGraph = [&]() {
		auto graph = renderer->GetGraph(graphName);
		graph->SetLongtituteRange(lonRng[0], lonRng[1]);
		graph->SetLatituteRange(latRng[0], latRng[1]);
		graph->SetHeightFromCenterRange(
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0],
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1]);
		graph->SetNodeGeometrySize(.02f * static_cast<float>(osg::WGS_84_RADIUS_EQUATOR));

		graph->Update();
	};

	initGraph();
	loadGraphToOSG();
	initOSGGraph();

	layoutGraph();
	auto lonOffs = 1.5f * (lonRng[1] - lonRng[0]);
	lonRng[0] += lonOffs;
	lonRng[1] += lonOffs;
	graphName += "_layouted";
	loadGraphToOSG();
	initOSGGraph();

	edgeClusterGraph();
	lonRng[0] += lonOffs;
	lonRng[1] += lonOffs;
	graphName += "_edge_clustered";
	loadGraphToOSG();
	initOSGGraph();

	grp->addChild(renderer->GetGroup());

	viewer->setSceneData(grp);

	auto prevClk = clock();
	while (!viewer->done()) {
		auto currClk = clock();
		auto duration = currClk - prevClk;

		if (duration >= CLOCKS_PER_SEC / 45) {
			viewer->frame();
			prevClk = clock();
		}
	}

	return 0;

ERR:
	std::cerr << errMsg << std::endl;
	return 1;
}
