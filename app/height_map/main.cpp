#include <iostream>

#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>

#include <common/osg.h>

#include <scivis/data/vol_data.h>
#include <scivis/scalar_viser/height_renderer.h>

static const std::string volPath = DATA_PATH_PREFIX"OSS/OSS000.raw";
static const std::array<uint32_t, 3> voxPerVol = { 300, 350, 50 };
static const std::array<float, 2> lonRng = { 100.05f, 129.95f };
static const std::array<float, 2> latRng = { -4.95f, 29.95f };
static const std::array<float, 2> hRng = { 1.f, 5316.f };
static const float hScale = 150.f;

int main(int argc, char** argv)
{
	auto volDat = SciVis::RAWVolumeData::LoadFromFile(
		SciVis::RAWVolumeData::FromFileParameters{
			voxPerVol,
			SciVis::ESupportedVoxelType::UInt8,
			volPath
		});
	if (!volDat.ok)
	{
		std::cerr << volDat.result.errMsg << std::endl;
		return 1;
	}

	std::array<float, 2> realHRng = {
		static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[0],
			static_cast<float>(osg::WGS_84_RADIUS_EQUATOR) + hScale * hRng[1] };

	SciVis::HeightMapRenderer::RenderParameters rndrParam;
	{
		osg::ref_ptr<osg::Image> img = new osg::Image;
		img->allocateImage(voxPerVol[0], voxPerVol[1], 1, GL_RED, GL_UNSIGNED_BYTE);
		img->setInternalTextureFormat(GL_RED);

		auto& vol = volDat.result.dat;
		memcpy(img->data(),
			vol.GetData().data() + vol.GetVoxelSize() * voxPerVol[0] * voxPerVol[1] * 10,
			vol.GetVoxelSize() * voxPerVol[0] * voxPerVol[1]);

		rndrParam.heightMapTex = new osg::Texture2D;
		rndrParam.heightMapTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::FilterMode::LINEAR);
		rndrParam.heightMapTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::FilterMode::LINEAR);
		rndrParam.heightMapTex->setWrap(osg::Texture::WRAP_S, osg::Texture::WrapMode::CLAMP_TO_EDGE);
		rndrParam.heightMapTex->setInternalFormatMode(osg::Texture::InternalFormatMode::USE_IMAGE_DATA_FORMAT);
		rndrParam.heightMapTex->setImage(img);
	}
	rndrParam.heightRange = realHRng;

	auto hmp = std::make_shared<SciVis::HeightMapRenderer>();
	hmp->SetRenderParameters(rndrParam);
	hmp->SetupMap(SciVis::HeightMapRenderer::MapParameters{
		lonRng, latRng, realHRng, SciVis::HeightMapRenderer::EMapType::Pillar
		});

	auto* viewer = new osgViewer::Viewer;
	viewer->setUpViewInWindow(200, 50, 800, 600);
	auto* manipulator = new osgGA::TrackballManipulator;
	viewer->setCameraManipulator(manipulator);

	osg::ref_ptr<osg::Group> grp = new osg::Group;
	grp->addChild(createEarth());

	grp->addChild(hmp->grp);
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
