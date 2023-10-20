#include <memory>

#include <array>

#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>

#include <common/osg.h>

#include <scivis/graph_viser/graph_layout_2D.h>
#include <scivis/graph_viser/graph_renderer.h>

static const std::string nodesPath = DATA_PATH_PREFIX"network/nodes.csv";
static const std::string edgesPath = DATA_PATH_PREFIX"network/edges.csv";
static const std::string graphName = "network";
static const std::array<uint32_t, 3> dim = { 300, 350, 50 };
static const std::array<uint8_t, 3> log2Dim = { 9, 9, 6 };
static const std::array<float, 2> lonRng = { 100.05f, 129.95f };
static const std::array<float, 2> latRng = { -4.95f, 29.95 };
static const std::array<float, 2> hRng = { 1.f, 5316.f };
static const float hScale = 100.f;

static SciVis::GraphViser::Graph graph;

static void initGraph()
{
	SciVis::GraphLoader::MkFileManager fm;
	auto nodes = fm.ReadGraphNodes(nodesPath);
	auto edges = fm.ReadGraphEdges(nodesPath, edgesPath);

	graph.Set(nodes, edges);

	do {
		while (graph.Iterate() > 0);
		graph.AddSubvisions();
	} while (graph.UpdateCycle() > 0);

	graph.Smooth();
}

int main(int argc, char** argv)
{
	auto* viewer = new osgViewer::Viewer;
	viewer->setUpViewInWindow(200, 50, 800, 600);
	auto* manipulator = new osgGA::TrackballManipulator;
	viewer->setCameraManipulator(manipulator);

	osg::ref_ptr<osg::Group> grp = new osg::Group;
	grp->addChild(createEarth());

	std::string errMsg;
	auto nodes = std::make_shared<std::map<std::string, osg::Vec3>>();
	auto edges = std::make_shared<std::vector<std::array<std::string, 2>>>();

	auto renderer = std::make_shared<SciVis::GraphViser::GraphRenderer>();
	{
		initGraph();
		for (auto itr = graph.GetNodes().begin(); itr != graph.GetNodes().end(); ++itr)
			nodes->emplace(std::make_pair(
				itr->first, osg::Vec3(itr->second.pos.X(), itr->second.pos.Y(), 0.f)));
		for (auto itr = graph.GetEdges().begin(); itr != graph.GetEdges().end(); ++itr)
			edges->emplace_back(std::array<std::string, 2>{itr->sourceLabel, itr->targetLabel});

		renderer->AddGraph(graphName, nodes, edges);
		
		auto graph = renderer->GetGraph(graphName);
		graph->SetLongtituteRange(lonRng[0], lonRng[1]);
		graph->SetLatituteRange(latRng[0], latRng[1]);
		graph->SetHeightFromCenterRange(hRng[0], hRng[1]);

		graph->Update();
	}

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