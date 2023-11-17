#ifndef SCIVIS_GRAPH_VISER_GRAPH_LAYOUT_2D_H
#define SCIVIS_GRAPH_VISER_GRAPH_LAYOUT_2D_H

#include <fstream>
#include <algorithm>
#include <string>
#include <cmath>
#include <limits>

#include <vector>
#include <string>
#include <map>
#include <vector>
#include <unordered_set>
#include <scivis/io/graph_io.h>

namespace SciVis
{
	namespace GraphViser
	{
		class Graph
		{
			struct pair_hash
			{
				template <class T1, class T2>
				std::size_t operator () (std::pair<T1, T2> const& pair) const
				{
					std::size_t h1 = std::hash<T1>()(pair.first);
					std::size_t h2 = std::hash<T2>()(pair.second);

					return h1 ^ h2;
				}
			};

		private:
			// Network structure
			std::map<std::string, Node> nodes;
			std::vector<Edge> edges;
			std::unordered_set<std::pair<int, int>, pair_hash> nodePairs;
			std::unordered_set<int> nodesNotMoved;

			// Algorithm parameters
			double K;									// Global spring constant (K).
			int I;										// Number of iterations in cycle.
			int iter;									// Number of remaining iterations.
			int cycles;									// Cycles left;
			double compatibilityThreshold;				// Compatibility threshold.
			double smoothWidth;							// Width of the Gaussian smoothing.

			// Physical parameters
			double S;									// Displacement of division points in a single iteration.
			double edgeDistance;						// Minimum distance between edges.
			bool gravitationIsOn;						// Marks whether gravitation is on.
			Vec2D gravitationCenter;					// Gravitation center.
			double gravitationExponent;					// Gravitation exponent.

			// Network parameters
			double edgeWeightThreshold;					// Threshold on edge weights (for dense graphs).
			double edgePercentageThreshold;				// Percentage of edges being kept (for dense graphs).

			// Graphics parameters
			double edgeOpacity;							// Opacity.

		public:
			// GraphLayout parameters
			double dt;
			double repulsion;
			double spring_k;
			double attraction;
			double edgeLength;
			int n_iterations;
			Vec2D layoutCenter;

			// constuctor
			Graph()
			{
				K = 0.000001;
				I = 10;
				iter = I;
				cycles = 6;
				compatibilityThreshold = 0.6;
				smoothWidth = 30.0;

				S = 0.000001;
				edgeDistance = 1e-4;
				gravitationIsOn = false;
				gravitationCenter.set(0.0, 0.0);
				gravitationExponent = -2.0;

				edgeWeightThreshold = -1.0;
				edgePercentageThreshold = -1.0;

				edgeOpacity = 0.1;

				repulsion = 0.2;
				spring_k = 1.;
				attraction = 0.32;
				edgeLength = 50.;
			};
			const Vec2D multi(const Vec2D& lhs, const double rhs)
			{
				return Vec2D(
					rhs * lhs.X(),
					rhs * lhs.Y());
			}

			const double distance(Vec2D v1, Vec2D v2)
			{
				//double distance = sqrt(pow(v1.x - v2.x, 2.0) + pow(v1.y - v2.y, 2.0) + pow(v1.z - v2.z, 2.0));
				double distance = sqrt(pow(v1.X() - v2.X(), 2.0) + pow(v1.Y() - v2.Y(), 2.0));
				return distance;
			}
			const Vec2D compute_spring_force(Node n1, Node n2) {
				return multi((n2.pos - n1.pos), spring_k);
			}
			const Vec2D compute_spring_force_general(double k, Vec2D v1, Vec2D v2) {
				return multi((v1 - v2), k);
			}

			// Coloumb's Law: F_repulsion = k (q1 * q2) / r^2
			const Vec2D compute_repulsion_force(Node n1, Node n2) {
				double dist = distance(n1.pos, n2.pos);
				return multi(Vec2D(
					(n1.pos.X() - n2.pos.X()),
					(n1.pos.Y() - n2.pos.Y())
				), (1 / pow(dist, 2.0)));
			}

			const std::map<std::string, Node>& GetNodes() const
			{
				return nodes;
			}

			const std::vector<Edge>& GetEdges() const
			{
				return edges;
			}

			void SetNetworkParams(double edgeWeightThreshold_, double edgePercentageThreshold_)
			{
				if (edgeWeightThreshold_ > 0.0)
					edgeWeightThreshold = edgeWeightThreshold_;
				else if (edgePercentageThreshold_ > 0.0)
					edgePercentageThreshold = edgePercentageThreshold_;
			}
			void SetAlgorithmParams(double K_, int cycles_, int I0_, double compat_, double sigma_)
			{
				K = K_;
				cycles = cycles_;
				I = I0_;
				iter = I;
				compatibilityThreshold = compat_;
				smoothWidth = sigma_;
			}
			void SetPhysicsParams(double S0_, double edgeDistance_, Vec2D gravCenter_, double gravExponent_)
			{
				S = S0_;
				edgeDistance = edgeDistance_;
				gravitationCenter = gravCenter_;
				gravitationExponent = gravExponent_;
			}
			void EnableGravitation()
			{
				gravitationIsOn = true;
			}
			void SetGraphicsParams(double alpha_)
			{
				edgeOpacity = alpha_;
			}
			void BuildCompatibilityLists()
			{
				int edgesNum = (int)edges.size(), compEdgePairs = 0;
				double comp = 0.0;
				for (int i = 0; i < edgesNum; i++)
				{
					for (int j = i + 1; j < edgesNum; j++)
					{
						comp = Edge::AngleCompatilibity(edges[i], edges[j])
							* Edge::ScaleCompatibility(edges[i], edges[j])
							* Edge::PositionCompatibility(edges[i], edges[j])
							* Edge::VisibilityCompability(edges[i], edges[j]);
						if (comp >= compatibilityThreshold)
						{
							edges[i].compatibleEdges.push_back(j);
							edges[j].compatibleEdges.push_back(i);
							compEdgePairs++;
						}
					}
				}
			}

			void updateReplusion() {
				double dx, dy, f, fx, fy, d, dsq;
				for (int i = 0; i < nodes.size(); ++i) {
					if (nodesNotMoved.find(i) != nodesNotMoved.end()) continue;

					auto n1_id = std::to_string(i);
					for (int j = 0; j < nodes.size(); ++j) {
						if (i == j) continue;

						auto n2_id = std::to_string(j);
						auto& n1 = nodes[n1_id];
						auto& n2 = nodes[n2_id];

						dx = n1.pos.X() - n2.pos.X();
						dy = n1.pos.Y() - n2.pos.Y();

						d = distance(n1.pos, n2.pos);
						if (d > edgeLength) dsq = edgeLength;
						else dsq = d;
						dsq *= dsq;

						f = repulsion * 128 * 128 / dsq;
						fx = f * dx / d;
						fy = f * dy / d;
						auto new_f = Vec2D(fx, fy);
						n1.force += new_f;
					}
				}
			}

			void updateSpring()
			{
				double dx, dy, f, fx, fy, d, dsq;
				for (int i = 0; i < nodes.size(); ++i) {
					if (nodesNotMoved.find(i) != nodesNotMoved.end()) continue;

					auto n1_id = std::to_string(i);
					for (int j = 0; j < nodes.size(); ++j) {
						if (i == j) continue;

						auto n2_id = std::to_string(j);
						auto& n1 = nodes[n1_id];
						auto& n2 = nodes[n2_id];

						std::pair<int, int> edge(i, j);
						if (nodePairs.find(edge) != nodePairs.end()) {
							dx = n2.pos.X() - n1.pos.X();
							dy = n2.pos.Y() - n1.pos.Y();

							d = distance(n1.pos, n2.pos);
							if (d > edgeLength) d = edgeLength;

							f = spring_k * d;
							fx = f * dx / d;
							fy = f * dy / d;
							auto new_f = Vec2D(fx, fy);
							n1.force += new_f;
						}
					}
				}
			}

			void updateCenterSpring()
			{
				double dx, dy, f, fx, fy, d, dsq;
				for (int i = 0; i < nodes.size(); ++i) {
					if (nodesNotMoved.find(i) != nodesNotMoved.end()) continue;

					auto n1_id = std::to_string(i);
					auto& n1 = nodes[n1_id];
					dx = n1.pos.X() - layoutCenter.X();
					dy = n1.pos.Y() - layoutCenter.Y();
					d = distance(n1.pos, layoutCenter);

					f = attraction * d;
					fx = f * dx / d;
					fy = f * dy / d;
					auto new_f = Vec2D(fx, fy);
					n1.force -= new_f;
				}
			}

			void updateAllEdges()
			{
				for (int k = 0; k < edges.size(); ++k) {
					edges[k].start = nodes[edges[k].sourceLabel].pos;
					edges[k].end = nodes[edges[k].targetLabel].pos;
					edges[k].subdivs.front() = Edge::center(edges[k].start, edges[k].end);
				}
			}

			void SetNodesNotMoved(const std::unordered_set<int>& nodesNotMoved)
			{
				this->nodesNotMoved = nodesNotMoved;
			}

			void Update(double deltaT) {
				updateReplusion();
				updateSpring();
				updateCenterSpring();

				double dx, dy, dsq, s;
				for (int i = 0; i < nodes.size(); ++i) {
					auto& node = nodes[std::to_string(i)];

					node.acc = node.force;
					auto dlt = node.acc * deltaT;
					node.vel += dlt;

					dlt = node.vel * deltaT;
					node.pos += dlt;

					node.force = node.vel = node.acc = Vec2D(0, 0);
				}

				updateAllEdges();
				++n_iterations;
			}

			int Iterate()
			{
				int edgesNum = (int)edges.size();
				std::vector<std::vector<Vec2D> > forces(
					edgesNum,
					std::vector<Vec2D>(
						(int)edges[0].subdivs.size(), Vec2D(0.0, 0.0))
				);

				// spring forces
				for (int i = 0; i < edgesNum; i++)
					edges[i].AddSpringForces(forces[i], K);

				// electrostatic forces
				for (int i = 0; i < edgesNum; i++)
				{
					int compatibleEdgesNum = (int)edges[i].compatibleEdges.size();
					for (int j = 0; j < compatibleEdgesNum; j++)
						edges[i].AddElectrostaticForces(forces[i],
							edges[edges[i].compatibleEdges[j]],
							edgeDistance);
				}

				// gravitation
				if (gravitationIsOn)
				{
					for (int i = 0; i < edgesNum; i++)
						edges[i].AddGravitationalForces(forces[i],
							gravitationCenter,
							gravitationExponent);
				}

				// update edges
				for (int i = 0; i < edgesNum; i++)
					edges[i].Update(forces[i], S);

				iter--;
				return iter;
			}
			int UpdateCycle()
			{
				S *= 0.5;
				I = 2 * I / 3;
				iter = I;
				cycles--;
				return cycles;
			}

			void AddSubvisions()
			{
				int edgesNum = (int)edges.size();
				for (int i = 0; i < edgesNum; i++)
					edges[i].AddSubdivisions();
			}

			void Smooth()
			{
				int edgesNum = (int)edges.size();
				for (int i = 0; i < edgesNum; i++)
					edges[i].Smooth(smoothWidth);
			}

			void Set(const std::map<std::string, Node>& myNodes, std::vector<Edge>& myEdges)
			{
				nodes = myNodes;
				sort(myEdges.begin(), myEdges.end(), [](const Edge& edge1, const Edge& edge2) -> bool {
					if (edge1.width < edge2.width)
						return true;
					else
						return false;
					});
				int wmax = myEdges[0].width;
				// filter out edges by weight
				if (edgeWeightThreshold > 0.0)
				{
					for (int i = 0; i < myEdges.size(); i++)
					{
						if (myEdges[i].width > edgeWeightThreshold)
						{
							edges.push_back(myEdges[i]);
							nodes[myEdges[i].sourceLabel].degree++;
							nodes[myEdges[i].targetLabel].degree++;
						}
					}
				}
				// filter out edges by percentage
				else if (edgePercentageThreshold > 0.0)
				{
					int nEdges = int(edgePercentageThreshold * (myEdges.size()) / 100);
					for (int i = 0; i < nEdges; i++)
					{
						edges.push_back(myEdges[i]);
						nodes[myEdges[i].sourceLabel].degree++;
						nodes[myEdges[i].targetLabel].degree++;
					}
				}
				// if no filter option set, take all edges
				else
				{
					for (int i = 0; i < myEdges.size(); i++)
					{
						edges.push_back(myEdges[i]);
						nodes[myEdges[i].sourceLabel].degree++;
						nodes[myEdges[i].targetLabel].degree++;
					}
				}
				// normalize edge widths
				int edgesNum = (int)edges.size();
				for (int i = 0; i < edgesNum; i++)
					edges[i].width *= 1.0 / (wmax + 1.0);
				// generate nodePairs
				for (int i = 0; i < edges.size(); i++) {
					std::pair <int, int> temp;
					temp = std::make_pair(atoi(edges[i].sourceLabel.c_str()), atoi(edges[i].targetLabel.c_str()));
					nodePairs.insert(temp);
				}
				BuildCompatibilityLists();

				Vec2D minPos(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
				Vec2D maxPos(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
				for (auto& pair : nodes) {
					if (minPos.X() > pair.second.pos.X())
						minPos.setX(pair.second.pos.X());
					if (minPos.Y() > pair.second.pos.Y())
						minPos.setY(pair.second.pos.Y());
					if (maxPos.X() < pair.second.pos.X())
						maxPos.setX(pair.second.pos.X());
					if (maxPos.Y() < pair.second.pos.Y())
						maxPos.setY(pair.second.pos.Y());
				}

				layoutCenter = (minPos + maxPos) * .5f;
			}
		};
	};
}

#endif // !SCIVIS_GRAPH_VISER_GRAPH_LAYOUT_2D_H
