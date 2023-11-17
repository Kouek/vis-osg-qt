#ifndef SCIVIS_IO_GRAPH_IO_H
#define SCIVIS_IO_GRAPH_IO_H

#include <algorithm>
#include <string>
#include <fstream>
#include <cmath>

#include <vector>
#include <map>

#define EPSILON 1e-6
#define INV_SQRT_2PI 0.3989422804

namespace SciVis
{
	class Vec2D
	{
	private:
		double x;
		double y;


	public:
		// Constructors
		Vec2D() { x = 0.0; y = 0.0; };
		Vec2D(double xx, double yy) { x = xx; y = yy; };
		// Setters and getters
		double X() const { return x; };
		double Y() const { return y; };
		void setX(const double xx) { x = xx; };
		void setY(const double yy) { y = yy; };
		void set(const double xx, const double yy) { x = xx; y = yy; };
		void zero() { x = 0.0; y = 0.0; };
		Vec2D random(double range)
		{
			double lower_bound = -range;
			double upper_bound = range;
			double rand_x = (double)rand() / RAND_MAX * (upper_bound - lower_bound) + lower_bound;
			double rand_y = (double)rand() / RAND_MAX * (upper_bound - lower_bound) + lower_bound;

			return Vec2D(rand_x, rand_y);
		}
		// Basic operators
		Vec2D& operator=(const Vec2D& my_vector)
		{
			x = my_vector.x;
			y = my_vector.y;

			return *this;
		}
		Vec2D& operator+=(const Vec2D& my_vector)
		{
			x += my_vector.x;
			y += my_vector.y;

			return *this;
		}
		const Vec2D operator+(const Vec2D& my_vector) const
		{
			Vec2D result = *this;
			result += my_vector;

			return result;
		}
		Vec2D& operator-=(const Vec2D& my_vector)
		{
			x -= my_vector.x;
			y -= my_vector.y;
			return *this;
		}
		const Vec2D operator-(const Vec2D& my_vector) const
		{
			Vec2D result = *this;
			result -= my_vector;

			return result;
		}
		const double operator*(const Vec2D& my_vector) const
		{
			return x * my_vector.x + y * my_vector.y;
		}
		Vec2D& operator*=(const double my_scalar)
		{
			x *= my_scalar;
			y *= my_scalar;

			return *this;
		}
		const Vec2D operator*(const double my_scalar) const
		{
			Vec2D result = *this;
			result *= my_scalar;

			return result;
		}
		Vec2D& operator/=(const double my_scalar)
		{
			x /= my_scalar;
			y /= my_scalar;

			return *this;
		}
		const Vec2D operator/(const double my_scalar) const
		{
			Vec2D result = *this;
			result /= my_scalar;

			return result;
		}

		// Other operations
		const double length() const { return sqrt(x * x + y * y); };
		const double angle() const { return atan2(y, x); };
		void set_length(const double length) {
			double a = atan2(y, x);
			set(length * cos(a), length * sin(a));
		};
		void set_angle(const double angle) {
			double l = length();
			set(l * cos(angle), l * sin(angle));
		};

		void normalize() { set_length(1.0); };
		void rotate(const double myangle) {
			double a = angle();
			set_angle(a + myangle);
		};
	};

	struct Node
	{
		int id;
		std::string data;
		// Physical properties
		Vec2D pos;
		int degree;
		Vec2D vel;
		Vec2D acc;
		Vec2D force;
		double mass;
		int radius;
		double repulsion;
		double stiffness;
		double damping;

		Node() {
			pos.zero();
			degree = 0;
			vel = Vec2D().random(1.0);
			acc = Vec2D().random(1.0);
			mass = 1.0;
			radius = 1.0;
			repulsion = 1.0;
			stiffness = 1.0;
			damping = 1.0;
		};
		Node(double x, double y) {
			pos.set(x, y);
			degree = 0;
			vel = Vec2D().random(1.0);
			acc = Vec2D().random(1.0);
			mass = 1.0;
			radius = 1.0;
			repulsion = 1.0;
			stiffness = 1.0;
			damping = 1.0;
		};
		void changePos(const Vec2D& newPos) {
			pos = pos + newPos;
		}
	};
	double gauss_weight(int my_dist, double my_sigma)
	{
		return INV_SQRT_2PI * exp(-0.5 * pow(double(my_dist) / my_sigma, 2.0)) / my_sigma;
	};
	struct Edge
	{
		// Variables
		std::string sourceLabel;                       // Label of source node.
		std::string targetLabel;                       // Label of target node.
		Vec2D start;                     // Start point.
		Vec2D end;                       // End point.
		std::vector<Vec2D> subdivs;      // Subdivision points.
		double width;                                  // Width.
		std::vector<int> compatibleEdges;              // List of compatible edge indices.

		/**
		 * @brief Edge Constructor.
		 * Sets end points, arranges direction and adds first subdivision point.
		 * @param mySourceLabel Source node label.
		 * @param myTargetLabel Target node label.
		 * @param myStart       Start coordinates.
		 * @param myEnd        End coordinates.
		 * @param myWidth       Edge width.
		 */
		Edge(const std::string& mySourceLabel, const std::string& myTargetLabel,
			const Vec2D& myStart, const Vec2D& myEnd, double myWidth)
		{
			sourceLabel = mySourceLabel;
			targetLabel = myTargetLabel;
			start = myStart;
			end = myEnd;
			width = myWidth;
			ArrangeDirection();
			AddSubdivisions();
		};

		void AddSubdivisions()
		{
			int oldSubdivsNum = (int)subdivs.size();
			if (oldSubdivsNum == 0)
				subdivs.assign(1, center(start, end));
			else
			{
				int newSubdivsNum = 2 * oldSubdivsNum, subdivIndex = 0, v1Index = -1, v2Index = 0;
				double segmentLength = double(oldSubdivsNum + 1) / double(newSubdivsNum + 1);
				std::vector<Vec2D> subdivisions(newSubdivsNum, Vec2D());
				Vec2D v1 = start, v2 = subdivs[0];
				double r = segmentLength;
				while (subdivIndex < newSubdivsNum)
				{
					subdivisions[subdivIndex] = v1 + (v2 - v1) * r;
					subdivIndex++;
					if (r + segmentLength > 1.0)
					{
						r = segmentLength - (1.0 - r);
						v1Index++;
						v2Index++;

						if (v1Index >= subdivs.size() || v2Index > subdivs.size())
							break;

						if (v1Index >= 0)
							v1 = subdivs[v1Index];
						if (v2Index < oldSubdivsNum)
							v2 = subdivs[v2Index];
						else
							v2 = end;
					}
					else
						r += segmentLength;
				}
				subdivs = subdivisions;
			}
		};

		void ArrangeDirection()
		{
			Vec2D v = vector(), t;
			if ((fabs(v.X()) > fabs(v.Y()) && end.X() < start.X())
				|| (fabs(v.X()) < fabs(v.Y()) && end.Y() < start.Y()))
			{
				std::swap(start, end);
				std::swap(sourceLabel, targetLabel);
			}
		};

		void Update(std::vector<Vec2D>& my_forces, double my_S)
		{
			int len = (int)subdivs.size();
			double flen = 0.0;
			for (int i = 0; i < len; i++)
			{
				flen = my_forces[i].length();
				if (flen > EPSILON)
					subdivs[i] += my_forces[i] * my_S / flen;
			}
		};

		void Smooth(double my_sigma)
		{
			int len = (int)subdivs.size();
			std::vector<Vec2D> subdivisions(len, Vec2D(0.0, 0.0));
			double weight, total_weight;
			for (int i = 0; i < len; i++)
			{
				total_weight = 0.0;

				// start point
				weight = gauss_weight(i + 1, my_sigma);
				subdivisions[i] = subdivisions[i] + start * weight;
				total_weight += weight;
				// inner points
				for (int j = 0; j < len; j++)
				{
					weight = gauss_weight(i - j, my_sigma);
					subdivisions[i] = subdivisions[i] + subdivs[j] * weight;
					total_weight += weight;
				}
				// end point
				weight = gauss_weight(len - i + 1, my_sigma);
				subdivisions[i] = subdivisions[i] + end * weight;
				total_weight += weight;

				// normalize
				subdivisions[i] = subdivisions[i] / total_weight;
			}

			// replace division points
			subdivs = subdivisions;
		};

		void AddSpringForces(std::vector<Vec2D>& my_forces, double my_K)
		{
			int len = (int)subdivs.size();
			double kP = my_K / ((end - start).length() * double(len + 1));

			if (len == 1)
				my_forces[0] += (start + end - subdivs[0] * 2.0) * kP;
			else
			{
				// first division point
				my_forces[0] += (start + subdivs[1] - subdivs[0] * 2.0) * kP;
				// inner division points
				for (int i = 1; i < len - 1; i++)
					my_forces[i] += (subdivs[i - 1] + subdivs[i + 1] - subdivs[i] * 2.0) * kP;
				// last division point
				my_forces[len - 1] += (subdivs[len - 2] + end - subdivs[len - 1] * 2.0) * kP;
			}
		};

		void AddElectrostaticForces(std::vector<Vec2D>& my_forces, const Edge& my_edge, double my_epsilon)
		{
			int len = (int)subdivs.size();
			Vec2D dist;
			double dlen;

			for (int i = 0; i < len; i++)
			{
				dist = (my_edge.subdivs[i] - subdivs[i]);
				dlen = dist.length();
				if (dlen > my_epsilon)
					my_forces[i] += dist / dlen;
			}
		};

		void AddGravitationalForces(std::vector<Vec2D>& my_forces, const Vec2D& my_center, double my_exponent)
		{
			int len = (int)subdivs.size();
			Vec2D dist;
			double dlen;

			for (int i = 0; i < len; i++)
			{
				dist = (my_center - subdivs[i]);
				dlen = dist.length();
				my_forces[i] += dist * 0.1 * pow(dlen + 1.0, my_exponent);
			}
		};

		Vec2D vector() const { return end - start; };
		/**
		 * @brief vector Converts edge into a vector.
		 * @return Vector pointing from the start to the end of the edge.
		 */
		static Vec2D center(const Vec2D& p1, const Vec2D& p2)
		{
			return (p1 + p2) / 2.0;
		};

		/**
		 * @brief project Projects a point to a line.
		 * @param my_point     Point to project.
		 * @param my_lineStart First point of line.
		 * @param my_lineEnd   Second point of line.
		 * @return           Projected point.
		 */
		static Vec2D project(const Vec2D& my_point, const Vec2D& my_lineStart, const Vec2D& my_lineEnd)
		{
			double L = (my_lineStart - my_lineEnd).length();
			double r = ((my_lineStart.Y() - my_point.Y()) * (my_lineStart.Y() - my_lineEnd.Y())
				- (my_lineStart.X() - my_point.X()) * (my_lineEnd.X() - my_lineStart.X())) / (L * L);
			return my_lineStart + (my_lineEnd - my_lineStart) * r;
		};

		static double AngleCompatilibity(const Edge& edge1, const Edge& edge2)
		{
			Vec2D v1 = edge1.vector();
			Vec2D v2 = edge2.vector();
			v1.normalize();
			v2.normalize();
			return fabs(v1 * v2);
		};


		static double ScaleCompatibility(const Edge& edge1, const Edge& edge2)
		{
			double l1 = edge1.vector().length();
			double l2 = edge2.vector().length();
			double lavg = (l1 + l2) / 2.0;
			if (lavg > EPSILON)
				return 2.0 / (lavg / std::min(l1, l2) + std::max(l1, l2) / lavg);
			else
				return 0.0;
		};

		static double PositionCompatibility(const Edge& edge1, const Edge& edge2)
		{
			double lavg = (edge1.vector().length() + edge2.vector().length()) / 2.0;
			if (lavg > EPSILON)
			{
				Vec2D mid1 = center(edge1.start, edge1.end);
				Vec2D mid2 = center(edge2.start, edge2.end);
				return lavg / (lavg + (mid1 - mid2).length());
			}
			else
				return 0.0;
		};

		static double EdgeVisibility(const Edge& edge1, const Edge& edge2)
		{
			Vec2D I0 = project(edge1.start, edge2.start, edge2.end);
			Vec2D I1 = project(edge1.end, edge2.start, edge2.end);
			Vec2D midI = center(I0, I1);
			Vec2D midP = center(edge2.start, edge2.end);
			return std::max(0.0, 1.0 - 2.0 * (midP - midI).length() / (I0 - I1).length());
		};

		static double VisibilityCompability(const Edge& edge1, const Edge& edge2)
		{
			return std::min(EdgeVisibility(edge1, edge2),
				EdgeVisibility(edge2, edge1));
		};

	};



	namespace GraphLoader
	{

		class MkFileManager
		{
		private:
			FILE* filePointer;
			int fileState;
			std::string fileName;

			enum MK_FILE_STATE { MK_EMPTY, MK_WRITE, MK_READ };
			void perror(const char* errorMessage) const
			{
				if (fileName == "_UNDEFINED_")
					printf("%s\n", errorMessage);
				else
					printf("%s (\"%s\").\n", errorMessage, fileName.c_str());

				if (filePointer != NULL)
					fclose(filePointer);

				exit(0);
			};
			void CheckState(const char* functionName, MK_FILE_STATE requiredState) const
			{
				if (fileState == requiredState)
					return;
				else
				{
					std::string err_msg = std::string(functionName) + " error: ";
					switch (fileState)
					{
					case MK_EMPTY:
					{
						err_msg += "file is not open";
						break;
					}
					case MK_WRITE:
					{
						err_msg += "file is open for writing";
						break;
					}
					case MK_READ:
					{
						err_msg += "file is open for reading";
						break;
					}
					}
					perror(err_msg.c_str());
				}
			};

		public:
			MkFileManager()
			{
				filePointer = NULL;
				fileState = MK_EMPTY;
				fileName = "_UNDEFINED_";
			};
			MkFileManager(const std::string& myFileName, const char* mode)
			{
				// set file name
				fileName = std::string(myFileName);

				// set state
				fileState = MK_READ;
				if (strcmp(mode, "w") == 0)
					fileState = MK_WRITE;

				// try to Open depending on the state
				filePointer = NULL;
				switch (fileState)
				{
				case MK_READ:
				{
					filePointer = fopen(fileName.c_str(), "r");
					break;
				}
				case MK_WRITE:
				{
					filePointer = fopen(fileName.c_str(), "w");
					break;
				}
				}

				// if failed, print error
				if (filePointer == NULL)
					perror("constructor error: file not found");
			};
			~MkFileManager()
			{
				if (fileState != MK_EMPTY && filePointer != NULL)
					fclose(filePointer);
			};

			bool Open(const std::string& myFileName, const char* mode)
			{
				// set state
				fileState = MK_READ;
				if (strcmp(mode, "w") == 0)
					fileState = MK_WRITE;

				// try to Open depending on the file state
				filePointer = NULL;
				switch (fileState)
				{
				case MK_READ:
				{
					filePointer = fopen(myFileName.c_str(), "r");
					break;
				}
				case MK_WRITE:
				{
					filePointer = fopen(myFileName.c_str(), "w");
					break;
				}
				}

				// if failed, print message and exit
				if (filePointer == NULL)
					return false;
				else
					return true;
			}

			void Close()
			{
				switch (fileState)
				{
				case MK_EMPTY:
					perror("close error: file is not opened");

				case MK_READ:
				case MK_WRITE:
				{
					if (filePointer != NULL)
						fclose(filePointer);
					fileState = MK_EMPTY;
					fileName = "_UNDEFINED_";
				}
				}
			};
			void GoToTop()
			{
				CheckState("GoToTop", MK_READ);
				fseek(filePointer, 0, SEEK_SET);
			};
			int Rows() const
			{
				CheckState("rows", MK_READ);
				char line[1024] = { "" };
				int _rows = 0;
				fseek(filePointer, 0, SEEK_SET);
				while (!feof(filePointer))
				{
					fgets(line, sizeof(line), filePointer);
					_rows++;
				}
				fseek(filePointer, 0, SEEK_SET);

				return _rows - 1;
			};
			int Columns() const {
				CheckState("columns", MK_READ);

				char line[1024] = { "" };
				char* line_ptr = fgets(line, sizeof(line), filePointer);
				char col[1024] = { "" };
				int _columns = 0, shift = 0;
				fseek(filePointer, 0, SEEK_SET);
				while (sscanf(line_ptr, "%s%n", col, &shift) == 1)
				{
					_columns++;
					line_ptr += shift;
				}
				fseek(filePointer, 0, SEEK_SET);

				return _columns;
			};
			void PutData(const std::vector<double>& data)
			{
				CheckState("PutData", MK_WRITE);

				int len = (int)data.size();
				fprintf(filePointer, "%lg", data[0]);
				for (int i = 1; i < len; i++)
					fprintf(filePointer, " %lg", data[i]);
				fprintf(filePointer, "\n");
			};
			void PutText(char* text) {
				CheckState("PutText", MK_WRITE);
				fprintf(filePointer, "%s\n", text);
			};
			int GetData(std::vector<double>& data)
			{
				CheckState("GetData", MK_READ);

				int num_data_element = 0;
				int shift = 0;
				double data_element = 0.0;
				char* data_line, line[1024] = { "" };

				data.clear();
				data_line = fgets(line, sizeof(line), filePointer);
				while (sscanf(data_line, "%lf%n", &data_element, &shift) == 1)
				{
					data.push_back(data_element);
					num_data_element++;
					data_line += shift;
				}

				return num_data_element;
			};
			void GetText(char* text, int length)
			{
				CheckState("GetText", MK_READ);

				fgets(text, length - 1, filePointer);
				text[strlen(text) - 1] = '\0';
			};
			// read csv file to graph
			// claim data dimension while loadfile
			std::map<std::string, Node> ReadGraphNodes(const std::string& nodesFile)
			{
				SciVis::GraphLoader::MkFileManager f;
				int rows = 0;
				char line[1024] = { "" };

				// read nodes
				if (!f.Open(nodesFile, "r"))
				{
					exit(0);
				}
				rows = f.Rows();
				f.GetText(line, 1024);
				char labelCh[128] = { "" };
				double x, y;
				std::map<std::string, Node> read_nodes;
				for (int r = 1; r < rows; r++)
				{
					f.GetText(line, 1024);
					sscanf(line, "%s %lg %lg", labelCh, &x, &y);
					read_nodes.insert(std::pair<std::string, Node>(std::string(labelCh), Node(x, y)));
				}
				f.Close();
				return read_nodes;
			};

			std::vector<Edge> ReadGraphEdges(const std::string& nodesFile, const std::string& edgesFile)
			{
				SciVis::GraphLoader::MkFileManager f;
				int rows = 0;
				char line[1024] = { "" };
				if (!f.Open(edgesFile, "r"))
				{
					exit(0);
				}
				rows = f.Rows();
				f.GetText(line, 1024);
				char src[128], dst[128];
				double w, wmax = 0.0;
				std::map<std::string, Node> temp_node = ReadGraphNodes(nodesFile);
				// read in all edges first
				std::vector<Edge> allEdges;
				for (int r = 0; r < rows - 1; r++)
				{
					f.GetText(line, 1024);
					w = 1.0;
					sscanf(line, "%s %s %lg", src, dst, &w);
					allEdges.push_back(Edge(std::string(src), std::string(dst),
						temp_node[std::string(src)].pos,
						temp_node[std::string(dst)].pos, w + 1.0));
				}
				f.Close();
				return allEdges;
			};

		};


	};
}

#endif // !SCIVIS_IO_GRAPH_IO_H
