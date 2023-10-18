#include <memory>

#include <array>

#include <QtWidgets/qapplication.h>

#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>

#include <common/osg.h>

#include <common_gui/heat_map_widget.h>

#include <scivis/io/tf_io.h>
#include <scivis/io/tf_osg_io.h>
#include <scivis/io/vol_io.h>
#include <scivis/io/vol_osg_io.h>
#include <scivis/scalar_viser/heat_map_renderer.h>

#include "main_window.h"

static const std::string volPath = DATA_PATH_PREFIX"OSS/OSS000.raw";
static const std::string volName = "0";
static const std::array<uint32_t, 3> dim = { 300, 350, 50 };
static const std::array<uint8_t, 3> log2Dim = { 9, 9, 6 };
static const std::array<float, 2> lonRng = { 100.05f, 129.95f };
static const std::array<float, 2> latRng = { -4.95f, 29.95 };
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

	std::shared_ptr<SciVis::ScalarViser::HeatMap3DRenderer> hmp
		= std::make_shared<SciVis::ScalarViser::HeatMap3DRenderer>();

	MainWindow mainWnd(hmp);
	mainWnd.show();
	HeatMapWidget hmpWdgt;
	hmpWdgt.show();

	auto tfTex = mainWnd.GetTFTexture();

	std::string errMsg;
	{
		auto volU8Dat = SciVis::Loader::RAWVolume::LoadU8FromFile(volPath, dim, &errMsg);
		if (!errMsg.empty())
			goto ERR;

		auto volDat = SciVis::Convertor::RAWVolume::U8ToNormalizedFloat(volU8Dat);
		auto volTex = SciVis::OSGConvertor::RAWVolume::
			NormalizedFloatToTexture(volDat, dim, log2Dim);

		hmp->AddVolume(volName, volTex, tfTex);

		auto vol = hmp->GetVolume(volName);
		vol->SetLongtituteRange(lonRng[0], lonRng[1]);
		vol->SetLatituteRange(latRng[0], latRng[1]);
		vol->SetHeightFromCenterRange(
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0],
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1]);
		vol->SetHeightFromCenter(.5f * (
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0] +
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1]));
	}

	mainWnd.UpdateFromRenderer();

	grp->addChild(hmp->GetGroup());
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
