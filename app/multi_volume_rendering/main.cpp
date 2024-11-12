#include <iostream>

#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>

#include <common/osg.h>

#include <scivis/data/vol_data.h>
#include <scivis/data/tf_data.h>
#include <scivis/scalar_viser/multi_volume_renderer.h>

static const std::array<std::string, SciVis::MultiVolumeRenderer::MaxVolumeNum> volPaths = {
	DATA_PATH_PREFIX"OSS/OSS000.raw",
	DATA_PATH_PREFIX"bonsai_256x256x256_uint8.raw"
};
static const std::array<std::string, SciVis::MultiVolumeRenderer::MaxVolumeNum> tfPaths = {
	DATA_PATH_PREFIX"OSS/oss_tf.txt",
	DATA_PATH_PREFIX"bonsai_tf.txt"
};
static const std::array<std::array<uint32_t, 3>, SciVis::MultiVolumeRenderer::MaxVolumeNum> voxPerVols = {
	std::array<uint32_t, 3>{ 300, 350, 50 },
	std::array<uint32_t, 3>{ 256, 256, 256 },
};
static const std::array<float, 2> lonRng = { 100.05f, 129.95f };
static const std::array<float, 2> latRng = { -4.95f, 29.95f };
static const std::array<float, 2> hRng = { 1.f, 5316.f };
static const float hScale = 200.f;

int main()
{
	std::array<osg::ref_ptr<osg::Texture3D>, SciVis::MultiVolumeRenderer::MaxVolumeNum> vols;
	std::array<osg::ref_ptr<osg::Texture2D>, SciVis::MultiVolumeRenderer::MaxVolumeNum> tfPreInts;
	for (int i = 0; i < 2; ++i)
	{
		auto volDat = SciVis::RAWVolumeData::LoadFromFile(
			SciVis::RAWVolumeData::FromFileParameters{
				voxPerVols[i],
				SciVis::ESupportedVoxelType::UInt8,
				volPaths[i]
			});
		if (!volDat.ok)
		{
			std::cerr << volDat.result.errMsg << std::endl;
			return 1;
		}
		auto tfDat = SciVis::TransferFunctionData::LoadFromFile(
			SciVis::TransferFunctionData::FromFileParameters{ tfPaths[i] });
		if (!tfDat.ok)
		{
			std::cerr << tfDat.result.errMsg << std::endl;
			return 1;
		}

		vols[i] = volDat.result.dat.ToOSGTexture3D();
		tfPreInts[i] = tfDat.result.dat.ToPreIntegratedOSGTexture();
	}

	auto* viewer = new osgViewer::Viewer;
	viewer->setUpViewInWindow(200, 50, 800, 600);
	auto* manipulator = new osgGA::TrackballManipulator;
	viewer->setCameraManipulator(manipulator);

	osg::ref_ptr<osg::Group> grp = new osg::Group;
	grp->addChild(createEarth());

	auto mvol = std::make_shared<SciVis::MultiVolumeRenderer>();
	{
		SciVis::MultiVolumeRenderer::RenderParameters param;
		param.longtitudeRange = lonRng;
		param.latitudeRange = latRng;
		param.heightRange[0] = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0];
		param.heightRange[1] = static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1];
		param.vols = vols;
		param.tfPreInts = tfPreInts;
		param.dt = hScale * (hRng[1] - hRng[0]) / voxPerVols[0][2] * .3f;
		param.maxStepCnt = 800;

		mvol->SetRenderParameters(param);
	}

	grp->addChild(mvol->grp);
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
}
