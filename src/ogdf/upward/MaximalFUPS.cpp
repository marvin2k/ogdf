/** \file
 * \brief Implementation of class MaximalFUPS, which implements
 *        the maximal feasible upward planar subgraph computation based on
 * 		  satisfiability (Chimani, Zeranski, 2012+)
 *
 * \author Robert Zeranski
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.txt in the root directory of the OGDF installation for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * \see  http://www.gnu.org/copyleft/gpl.html
 ***************************************************************/

#include <ogdf/upward/MaximalFUPS.h>
#include <ogdf/internal/upward/UpSAT.h>
//#include <ogdf/basic/Graph.h>
//#include <ogdf/basic/Graph_d.h>
//#include <ogdf/basic/List.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Stopwatch.h>
//#include <vector>


namespace ogdf {

	Module::ReturnType MaximalFUPS::doCall(UpwardPlanRep &UPR, List<edge> &delEdges) {
		const Graph& G = UPR.original();
		delEdges.clear();

		OGDF_ASSERT(isSimple(G));
		node source;
		bool singleSource = hasSingleSource(G,source);
		//OGDF_ASSERT( singleSource );

		GraphCopy GC;
		GC.createEmpty(G);
		for(node n : G.nodes) {
			GC.newNode(n);
		}
		if(singleSource) {
			for(adjEntry adj : source->adjEntries) {
				edge eG = adj->theEdge();
				OGDF_ASSERT( source == eG->source() );
				GC.newEdge( eG );
			}
		} else source = nullptr;

		StopwatchWallClock timer;
		timer.start();

		List<edge> edges;
		G.allEdges(edges);
		edges.permute();
		while (!edges.empty()) {
			if (m_timelimit != 0 && timer.seconds()>m_timelimit) break;
			edge fG = edges.popFrontRet();
			if( fG->source() == source ) continue;
			edge f = GC.newEdge( fG );
			UpSAT tester(GC, true);
			if (!tester.testUpwardPlanarity()) {
				GC.delEdge(f);
				delEdges.pushBack(fG);
			}
		}
		timer.stop();
		UpSAT embedder(GC, true);
		adjEntry externalToItsRight;
		NodeArray<int>* nodeOrder = nullptr;
		if(!singleSource) nodeOrder = new NodeArray<int>(GC);
		if (!embedder.embedUpwardPlanar(externalToItsRight,nodeOrder))
			return Module::retError;

		if(!singleSource) { //make single source
			for(node n : G.nodes) {
				if(n->indeg() == 0 && (*nodeOrder)[n]>0) {
					adjEntry adj = n->lastAdj();
					do {
						adj = adj->faceCycleSucc();
					} while( (*nodeOrder)[adj->theNode()] >= (*nodeOrder)[n] );
					GC.newEdge(adj,n->lastAdj());
				}
			}
			delete nodeOrder;
		}
		OGDF_ASSERT( hasSingleSource(GC,source) );

		OGDF_ASSERT( externalToItsRight->theNode() == source );

		UPR = UpwardPlanRep(GC,externalToItsRight);
		OGDF_ASSERT( isSimple(UPR) );
		if(timer.seconds()>m_timelimit) return Module::retTimeoutFeasible;
		return Module::retOptimal;
	}

}
