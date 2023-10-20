#ifndef SCIVIS_GRAPH_VISER_GRAPH_LAYOUT_2D_H
#define SCIVIS_GRAPH_VISER_GRAPH_LAYOUT_2D_H

#include <fstream>
#include <algorithm>
#include <string>
#include <cmath>

#include <vector>
#include <string>
#include <map>
#include <vector>

#include <scivis/io/graph_io.h>

namespace SciVis
{
	namespace GraphViser
	{
		class Graph
		{
		private:
			// Network structure
			std::map<std::string, Node> nodes;
			std::vector<Edge> edges;
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
			};

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
				BuildCompatibilityLists();
			}
		};
	};
}

#endif // !SCIVIS_GRAPH_VISER_GRAPH_LAYOUT_2D_H
