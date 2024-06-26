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
#include <scivis/scalar_viser/marching_square_renderer.h>

#include "mcsqr_main_window.h"

static const std::string volPath = DATA_PATH_PREFIX"OSS/OSS000.raw";
static const std::string volName = "0";
static const std::array<uint32_t, 3> dim = { 300, 350, 50 };
static const std::array<uint8_t, 3> log2Dim = { 9, 9, 6 };
static const std::array<float, 2> lonRng = { 100.05f, 129.95f };
static const std::array<float, 2> latRng = { -4.95f, 29.95f };
static const std::array<float, 2> hRng = { 1.f, 5316.f };
static const float hScale = 100.f;

int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	auto* viewer = new osgViewer::Viewer;
	viewer->setUpViewInWindow(200, 50, 800, 600);
	auto* manipulator = new osgGA::TrackballManipulator;
	viewer->setCameraManipulator(manipulator);

	osg::ref_ptr<osg::Group> grp = new osg::Group;
	grp->addChild(createEarth());

	auto mcb = std::make_shared<SciVis::ScalarViser::MarchingSquareCPURenderer>();
	auto heights = std::make_shared<std::vector<uint32_t>>();

	MCSQRMainWindow mainWnd(mcb);

	std::string errMsg;
	{
		auto volU8Dat = SciVis::Loader::RAWVolume::LoadU8FromFile(volPath, dim, &errMsg);
		if (!errMsg.empty())
			goto ERR;

		auto volDat = SciVis::Convertor::RAWVolume::U8ToNormalizedFloat(volU8Dat);
		auto volDatSmoothed = SciVis::Convertor::RAWVolume::RoughFloatToSmooth(volDat, dim);
		auto volDatShrd = std::make_shared<std::vector<float>>(volDat);
		auto volDatSmoothedShrd = std::make_shared<std::vector<float>>(volDatSmoothed);
		mcb->AddVolume(volName, volDatShrd, volDatSmoothedShrd, dim);

		auto vol = mcb->GetVolume(volName);
		vol->SetLongtituteRange(lonRng[0], lonRng[1]);
		vol->SetLatituteRange(latRng[0], latRng[1]);
		vol->SetHeightFromCenterRange(
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0],
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1]);

		for (uint8_t h = 0; h < dim[2]; ++h)
			heights->emplace_back(h);
		vol->MarchingSquare(30.f / 255.f, *heights);

		mainWnd.SetVolumeAndHeights(volDatShrd, volDatSmoothedShrd, dim, heights);
	}

	mainWnd.UpdateFromRenderer();
	mainWnd.show();

	grp->addChild(mcb->GetGroup());
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
