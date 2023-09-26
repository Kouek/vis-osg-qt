#ifndef SCIVIS_IO_VOL_LOADER_H
#define SCIVIS_IO_VOL_LOADER_H

#include <fstream>
#include <array>
#include <vector>

namespace SciVis
{
	namespace Loader
	{
		class RAWVolume
		{
		public:
			static std::vector<uint8_t> LoadFromFile(
				const std::string& filePath, const std::array<uint32_t, 3>& dim,
				std::string* errMsg = nullptr)
			{
				std::ifstream is(filePath, std::ios::in | std::ios::binary | std::ios::ate);
				if (!is.is_open()) {
					if (errMsg)
						*errMsg = "Invalid File Path";
					return std::vector<uint8_t>();
				}

				auto voxNum = static_cast<size_t>(is.tellg()) / sizeof(uint8_t);
				{
					auto _voxNum = (size_t)dim[0] * dim[1] * dim[2];
					if (voxNum < _voxNum) {
						if (errMsg)
							*errMsg = "File Size is Smaller than Volume Size";
						is.close();
						return std::vector<uint8_t>();
					}
					voxNum = std::min(voxNum, _voxNum);
				}

				std::vector<uint8_t> dat(voxNum);
				is.seekg(0);
				is.read(reinterpret_cast<char*>(dat.data()), sizeof(uint8_t) * voxNum);
				is.close();

				return dat;
			}

			static bool DumpToFile(
				const std::string& filePath, const std::vector<uint8_t>& dat,
				std::string* errMsg = nullptr)
			{
				std::ofstream os(filePath, std::ios::out | std::ios::binary);
				if (!os.is_open()) {
					if (errMsg)
						*errMsg = "Invalid File Path";
					return false;
				}

				os.write(reinterpret_cast<const char*>(dat.data()), sizeof(uint8_t) * dat.size());
				os.close();

				return true;
			}
		};
	}

	namespace Convertor
	{
		class RAWVolume
		{
		public:
			static std::vector<float> U8ToFloat(const std::vector<uint8_t>& u8Dat)
			{
				std::vector<float> dat(u8Dat.size());
				size_t i = 0;
				for (size_t i = 0; i < u8Dat.size(); ++i)
					dat[i] = u8Dat[i] / 255.f;
				return dat;
			}

			static std::vector<uint8_t> FloatToU8(const std::vector<float>& fDat)
			{
				std::vector<uint8_t> dat(fDat.size());
				size_t i = 0;
				for (size_t i = 0; i < fDat.size(); ++i)
					dat[i] = fDat[i] * 255.f;
				return dat;
			}
		};
	}
}

#endif // !SCIVIS_IO_VOL_LOADER_H
