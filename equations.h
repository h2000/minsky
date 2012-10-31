/*
  @copyright Steve Keen 2012
  @author Russell Standish
  This file is part of Minsky.

  Minsky is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Minsky is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Minsky.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
  Structures for building a mathematical representation of the Minsky
  model as a directed acyclic graph (DAG)
*/

#ifndef EQUATIONS_H
#define EQUATIONS_H

#include "variableManager.h"
#include "portManager.h"
#include "godley.h"

#include "operation.h"
#include <classdesc.h>
#include <ostream>
#include <vector>
#include <map>

namespace minsky
{
  class Minsky;
}

namespace MathDAG
{
  using namespace std;
  using classdesc::shared_ptr;
  using namespace minsky;

  struct Node;
  /// a manipulator to make iostream expressions easy 
  struct LaTeXManip
  {
    const Node& node;
    LaTeXManip(const Node& node): node(node) {}
  };

  struct Node
  {
    /// algebraic heirarchy level, used for working out whether
    /// brackets are necessary.
    virtual int BODMASlevel() const=0; 
    /// writes LaTeX representation of this DAG to the stream
    virtual ostream& latex(ostream&) const=0; 
    /// used within io streaming
    LaTeXManip latex() const {return LaTeXManip(*this);}
  };

  inline ostream& operator<<(ostream& o, LaTeXManip m)
  {return m.node.latex(o);}

  struct VariableDAG: public Node
  {
    string name;
    shared_ptr<Node> rhs;
    VariableDAG(const string& name=""): name(name) {}
    int BODMASlevel() const {return 0;}
    ostream& latex(ostream&) const;
    using Node::latex;
  };

  struct OperationDAG: public Node
  {
    Operation::Type type;
    string name;
    vector<vector<shared_ptr<Node> > > arguments;
    OperationDAG(Operation::Type type=Operation::numOps, 
                 const string& name=""): 
      type(type), name(name) {}
    int BODMASlevel() const;
    ostream& latex(ostream&) const; 
    using Node::latex;
  };

  /// represents a Godley column
  struct GodleyColumnDAG: public Node, public vector<string>
  {
    int BODMASlevel() const {return 2;}
    ostream& latex(ostream&) const; 
  };

  class SystemOfEquations
  {
    vector<VariableDAG> variables;
    vector<VariableDAG> integrationVariables;

    const VariableManager& vm;
    const Operations& ops;
    const PortManager& pm;
    map<int, int> portToOperation;

    VariableDAG makeDAG(const string& name);
    OperationDAG makeDAG(const Operation& op);

    // creates a node object representing what feeds the wire
    Node* createNodeFromWire(int wire);

    void processGodleyTable
    (map<string, GodleyColumnDAG>& godleyVariables, const GodleyTable& godley);

  public:
    /// construct the system of equations 
    SystemOfEquations(const Minsky&);
    ostream& latex(ostream&) const; ///< render as a LaTeX eqnarray
  };

}


#endif
