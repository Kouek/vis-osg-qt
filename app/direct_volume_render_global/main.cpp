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

#include "dvr_main_window.h"

static const std::string volPath = DATA_PATH_PREFIX"salt.txt";
static const float nullVal = 9999.f;
static const std::array<uint32_t, 3> dim = { 720, 348, 35 };
static const std::array<uint8_t, 3> log2Dim = { 10, 9, 6 };
static const std::array<float, 2> lonRng = { -180.f, +180.f };
static const std::array<float, 2> latRng = { -75.24f, +84.75f };
static const std::array<float, 2> hRng = { 1.f, 5316.f };
static const float hScale = 150.f;

int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	auto* viewer = new osgViewer::Viewer;
	viewer->setUpViewInWindow(200, 50, 800, 600);
	auto* manipulator = new osgGA::TrackballManipulator;
	viewer->setCameraManipulator(manipulator);

	osg::ref_ptr<osg::Group> grp = new osg::Group;
	grp->addChild(createEarth());

	auto dvr = std::make_shared<SciVis::ScalarViser::DirectVolumeRenderer>();
	dvr->SetDeltaT(hScale * (hRng[1] - hRng[0]) / dim[2] * .3f);
	dvr->SetMaxStepCount(800);

	DVRMainWindow mainWnd(dvr);
	auto tfTex = mainWnd.GetTFTexture();

	std::string errMsg;
	{
		auto txtVol = SciVis::Loader::TXTVolume::LoadFromFile(
			SciVis::GetDataPathPrefix() + volPath, dim, nullVal, true, &errMsg);
		if (!errMsg.empty())
			goto ERR;

		auto volDat = SciVis::Convertor::RAWVolume::FloatToNormalizedFloat(
			txtVol.dat, txtVol.valRng, nullVal);
		auto volTex = SciVis::OSGConvertor::RAWVolume::
			NormalizedFloatToTexture(volDat, dim, log2Dim);

		dvr->AddVolume(volPath, volTex, tfTex, dim);
		auto vol = dvr->GetVolume(volPath);
		vol->SetLongtituteRange(lonRng[0], lonRng[1]);
		vol->SetLatituteRange(latRng[0], latRng[1]);
		vol->SetHeightFromCenterRange(
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0],
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1]);
		vol->SetVolumeStartFromLongtituteZero(true);
	}

	mainWnd.UpdateFromRenderer();
	mainWnd.show();

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
