#ifndef SCIVIS_IO_VOL_IO_H
#define SCIVIS_IO_VOL_IO_H

#include <fstream>
#include <limits>

#include <array>
#include <vector>
#include <unordered_set>

namespace SciVis
{
	namespace Loader
	{
		class RAWVolume
		{
		public:
			static std::vector<uint8_t> LoadU8FromFile(
				const std::string& filePath, const std::array<uint32_t, 3>& dim,
				std::string* errMsg = nullptr)
			{
				std::ifstream is(filePath, std::ios::in | std::ios::binary | std::ios::ate);
				if (!is.is_open()) {
					if (errMsg) {
						*errMsg = "Invalid File Path: ";
						errMsg->append(filePath);
					}
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
					if (errMsg) {
						*errMsg = "Invalid File Path: ";
						errMsg->append(filePath);
					}
					return false;
				}

				os.write(reinterpret_cast<const char*>(dat.data()), sizeof(uint8_t) * dat.size());
				os.close();

				return true;
			}
		};

		class TXTVolume
		{
		public:
			std::array<float, 2> valRng;
			std::vector<float> dat;

		public:
			static TXTVolume LoadFromFile(
				const std::string& filePath, const std::array<uint32_t, 3>& dim,
				float nullVal, bool flipZ = false,
				std::string* errMsg = nullptr)
			{
				TXTVolume ret;

				FILE* fp = fopen(filePath.c_str(), "r");
				if (!fp) {
					if (errMsg) {
						*errMsg = "Invalid File Path: ";
						errMsg->append(filePath);
					}
					return ret;
				}

				auto voxNum = static_cast<size_t>(dim[0]) * dim[1] * dim[2];
				ret.dat.resize(voxNum);
				for (size_t i = 0; i < voxNum; ++i)
					fscanf(fp, "%f", &ret.dat[i]);
				fclose(fp);

				if (flipZ) {
					auto dimYxX = static_cast<size_t>(dim[1]) * dim[0];
					for (size_t z = 0; z < dim[2] / 2; ++z) {
						auto z1 = dim[2] - 1 - z;
						if (z == z1) continue;

						auto zBeg = ret.dat.begin() + z * dimYxX;
						auto zEnd = ret.dat.begin() + (z + 1) * dimYxX;
						auto z1Beg = ret.dat.begin() + z1 * dimYxX;
						std::swap_ranges(zBeg, zEnd, z1Beg);
					}
				}

				for (auto v : ret.dat) {
					if (v == nullVal) continue;
					if (ret.valRng[0] > v)
						ret.valRng[0] = v;
					if (ret.valRng[1] < v)
						ret.valRng[1] = v;
				}

				return ret;
			}

		private:
			TXTVolume()
			{
				valRng[0] = std::numeric_limits <float>::max();
				valRng[1] = std::numeric_limits <float>::lowest();
			}
		};

		class LabeledTXTVolume
		{
		public:
			bool isDense;
			std::array<uint32_t, 3> dim;
			std::array<float, 2> valRng;
			std::array<float, 2> lonRng;
			std::array<float, 2> latRng;
			std::array<float, 2> hRng;
			std::vector<float> dat;

			void Normalize(const std::array<float, 2>* valRng = nullptr)
			{
				if (valRng && (*valRng)[1] > (*valRng)[0])
					this->valRng = *valRng;

				auto rngWid = this->valRng[1] - this->valRng[0];
				for (auto itr = dat.begin(); itr != dat.end(); ++itr) {
					if (isnan(*itr))
						*itr = this->valRng[0];
					else
					{
						*itr = (*itr - this->valRng[0]) / rngWid;
						if (*itr < 0.f)
							*itr = 0.f;
					}
				}

				this->valRng[0] = 0.f;
				this->valRng[1] = 1.f;
			}

		public:
			static LabeledTXTVolume LoadFromFile(
				const std::string& filePath, std::string* errMsg = nullptr)
			{
				LabeledTXTVolume ret;

				std::ifstream is(filePath, std::ios::in);
				if (!is.is_open()) {
					if (errMsg) {
						*errMsg = "Invalid File Path: ";
						errMsg->append(filePath);
					}
					return ret;
				}

				auto computeMinMax = [](std::array<float, 2>& rng, float val) {
					if (rng[0] > val)
						rng[0] = val;
					if (rng[1] < val)
						rng[1] = val;
				};

				std::unordered_set<float> hSets, latSets, lonSets;
				std::string buf;
				std::getline(is, buf); // 跳过第一行
				while (std::getline(is, buf)) {
					std::array<float, 5> f5;

					auto validRead = sscanf(buf.c_str(), "%f%f%f%f%f",
						&f5[0], &f5[1], &f5[2], &f5[3], &f5[4]);
					if (validRead < 4) continue;

					hSets.emplace(f5[1]);
					latSets.emplace(f5[2]);
					lonSets.emplace(f5[3]);

					computeMinMax(ret.hRng, f5[1]);
					computeMinMax(ret.latRng, f5[2]);
					computeMinMax(ret.lonRng, f5[3]);
					if (validRead == 5)
						computeMinMax(ret.valRng, f5[4]);
					else
						f5[4] = std::numeric_limits<float>::quiet_NaN();

					ret.dat.emplace_back(f5[4]);
				}

				ret.dim[2] = hSets.size();
				ret.dim[1] = latSets.size();
				ret.dim[0] = lonSets.size();
				ret.isDense = ret.dat.size() ==
					static_cast<size_t>(ret.dim[2]) * ret.dim[1] * ret.dim[0];

				return ret;
			}

		private:
			LabeledTXTVolume()
			{
				dim[0] = dim[1] = dim[2] = 0;
				valRng[0] = lonRng[0] = latRng[0] = hRng[0]
					= std::numeric_limits <float>::max();
				valRng[1] = lonRng[1] = latRng[1] = hRng[1]
					= std::numeric_limits <float>::lowest();
			}
		};
	}

	namespace Convertor
	{
		class RAWVolume
		{
		public:
			static std::vector<float> U8ToNormalizedFloat(const std::vector<uint8_t>& u8Dat)
			{
				std::vector<float> dat(u8Dat.size());
				size_t i = 0;
				for (size_t i = 0; i < u8Dat.size(); ++i)
					dat[i] = u8Dat[i] / 255.f;
				return dat;
			}

			static std::vector<float> FloatToNormalizedFloat(
				const std::vector<float>& floatDat,
				const std::array<float, 2>& valRng,
				float nullVal, float nullValMap = 0.f)
			{
				std::vector<float> dat(floatDat.size());
				size_t i = 0;
				auto dlt = valRng[1] - valRng[0];
				for (size_t i = 0; i < floatDat.size(); ++i) {
					if (floatDat[i] == nullVal)
						dat[i] = nullValMap;
					else
						dat[i] = (floatDat[i] - valRng[0]) / dlt;
				}
				return dat;
			}

			static std::vector<uint8_t> NormalizedFloatToU8(const std::vector<float>& fDat)
			{
				std::vector<uint8_t> dat(fDat.size());
				size_t i = 0;
				for (size_t i = 0; i < fDat.size(); ++i)
					dat[i] = fDat[i] * 255.f;
				return dat;
			}

			static std::vector<float> RoughFloatToSmooth(const std::vector<float>& fDat, const std::array<uint32_t, 3>& dim)
			{
				std::vector<float> smoothed(fDat.size());
				size_t dimYxX = dim[1] * dim[0];
				auto sample = [&](uint32_t x, uint32_t y, uint32_t z) -> float {
					x = std::min(x, dim[0] - 1);
					y = std::min(y, dim[1] - 1);
					z = std::min(z, dim[2] - 1);
					return fDat[z * dimYxX + y * dim[0] + x];
				};

				for (uint32_t z = 0; z < dim[2]; ++z)
					for (uint32_t y = 0; y < dim[1]; ++y)
						for (uint32_t x = 0; x < dim[0]; ++x) {
							std::array<float, 27> field;
							for (int8_t dz = -1; dz < 2; ++dz)
								for (int8_t dy = -1; dy < 2; ++dy)
									for (int8_t dx = -1; dx < 2; ++dx) {
										auto i = (dz + 1) * 9 + (dy + 1) * 3 + (dx + 1);
										field[i] = sample(
											x == 0 && dx < 0 ? x : x + dx,
											y == 0 && dy < 0 ? y : y + dy,
											z == 0 && dz < 0 ? z : z + dz);
									}
							auto& val = smoothed[z * dimYxX + y * dim[0] + x];
							val = 0.f;
							for (uint8_t i = 0; i < 27; ++i)
								val += field[i];
							val /= 27.f;
						}

				return smoothed;
			}
		};
	}
}

#endif // !SCIVIS_IO_VOL_IO_H
