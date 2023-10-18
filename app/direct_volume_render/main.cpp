#include <memory>

#include <array>

#include <QtWidgets/qapplication.h>

#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>

#include <common/osg.h>

#include <scivis/io/tf_io.h>
#include <scivis/io/tf_osg_io.h>
#include <scivis/io/vol_io.h>
#include <scivis/io/vol_osg_io.h>
#include <scivis/scalar_viser/direct_volume_renderer.h>

#include "main_window.h"

static const std::array<std::string, 11> volPaths = {
		DATA_PATH_PREFIX"OSS/OSS000.raw",
		DATA_PATH_PREFIX"OSS/OSS006.raw",
		DATA_PATH_PREFIX"OSS/OSS015.raw",
		DATA_PATH_PREFIX"OSS/OSS024.raw",
		DATA_PATH_PREFIX"OSS/OSS036.raw",
		DATA_PATH_PREFIX"OSS/OSS042.raw",
		DATA_PATH_PREFIX"OSS/OSS051.raw",
		DATA_PATH_PREFIX"OSS/OSS069.raw",
		DATA_PATH_PREFIX"OSS/OSS090.raw",
		DATA_PATH_PREFIX"OSS/OSS108.raw",
		DATA_PATH_PREFIX"OSS/OSS120.raw"
};
static const std::array<std::string, 11> volNames = {
		"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"
};
static const std::array<uint32_t, 3> dim = { 300, 350, 50 };
static const std::array<uint8_t, 3> log2Dim = { 9, 9, 6 };
static const std::array<float, 2> lonRng = { 100.05f, 129.95f };
static const std::array<float, 2> latRng = { -4.95f, 29.95 };
static const std::array<float, 2> hRng = { 1.f, 5316.f };
static const float hScale = 100.f;

class DVRSwitchVolumeCallback : public osg::NodeCallback {
private:
	size_t currIdx;
	clock_t prevClk;
	std::shared_ptr<SciVis::ScalarViser::DirectVolumeRenderer> renderer;

public:
	DVRSwitchVolumeCallback(std::shared_ptr<SciVis::ScalarViser::DirectVolumeRenderer> renderer)
		: renderer(renderer), currIdx(0) {
		prevClk = clock();
	}
	virtual void operator()(osg::Node* node, osg::NodeVisitor* nv) {
		auto currClk = clock();
		auto duration = currClk - prevClk;
		if (duration >= CLOCKS_PER_SEC / 3) {
			++currIdx;
			if (currIdx == renderer->GetVolumeNum())
				currIdx = 0;
			renderer->DisplayVolume(volNames[currIdx]);

			prevClk = clock();
		}

		traverse(node, nv);
	}
};

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	
	auto* viewer = new osgViewer::Viewer;
	viewer->setUpViewInWindow(200, 50, 800, 600);
	auto* manipulator = new osgGA::TrackballManipulator;
	viewer->setCameraManipulator(manipulator);

	osg::ref_ptr<osg::Group> grp = new osg::Group;
	grp->addChild(createEarth());

	std::shared_ptr<SciVis::ScalarViser::DirectVolumeRenderer> dvr
		= std::make_shared<SciVis::ScalarViser::DirectVolumeRenderer>();
	dvr->SetDeltaT(hScale * (hRng[1] - hRng[0]) / dim[2] * .3f);
	dvr->SetMaxStepCount(800);

	MainWindow mainWnd(dvr);
	mainWnd.show();

	auto tfTex = mainWnd.GetTFTexture();

	std::string errMsg;
	for (size_t i = 0; i < volPaths.size(); ++i) {
		auto volU8Dat = SciVis::Loader::RAWVolume::LoadU8FromFile(
			volPaths[i], dim, &errMsg);
		if (!errMsg.empty())
			goto ERR;

		auto volDat = SciVis::Convertor::RAWVolume::U8ToNormalizedFloat(volU8Dat);
		auto volTex = SciVis::OSGConvertor::RAWVolume::
			NormalizedFloatToTexture(volDat, dim, log2Dim);

		dvr->AddVolume(volNames[i], volTex, tfTex, false);
		auto vol = dvr->GetVolume(volNames[i]);
		vol->SetLongtituteRange(lonRng[0], lonRng[1]);
		vol->SetLatituteRange(latRng[0], latRng[1]);
		vol->SetHeightFromCenterRange(
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0],
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1]);
	}

	dvr->DisplayVolume(volNames[0]);
	dvr->GetGroup()->addEventCallback(new DVRSwitchVolumeCallback(dvr));
	grp->addChild(dvr->GetGroup());
	
	viewer->setSceneData(grp);

	auto prevClk = clock();
	while (!viewer->done()) {
		auto currClk = clock();
		auto duration = currClk - prevClk;

		app.processEvents();

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
