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

  struct MatlabManip
  {
    const Node& node;
    MatlabManip(const Node& node): node(node) {}
  };

  /// convert double to a LaTeX string representing that value
  string latex(double);

  struct Node
  {
    /// algebraic heirarchy level, used for working out whether
    /// brackets are necessary.
    virtual int BODMASlevel() const=0; 
    /// writes LaTeX representation of this DAG to the stream
    virtual ostream& latex(ostream&) const=0; 
    /// writes a matlab representation of this DAG to the stream
    virtual ostream& matlab(ostream&) const=0; 
    /// returns evaluation order in sequence of variable defintions
    virtual int order() const=0;
    /// used within io streaming
    LaTeXManip latex() const {return LaTeXManip(*this);}
    MatlabManip matlab() const {return MatlabManip(*this);}
  };

  inline ostream& operator<<(ostream& o, LaTeXManip m)
  {return m.node.latex(o);}
  inline ostream& operator<<(ostream& o, MatlabManip m)
  {return m.node.matlab(o);}

  struct ConstantDAG: public Node
  {
    double value;
    ConstantDAG(double value=0): value(value) {}
    int BODMASlevel() const {return 0;}
    int order() const {return 0;}
    ostream& latex(ostream& o) const {return o<<MathDAG::latex(value);}
    ostream& matlab(ostream& o) const {return o<<value;}
  };

  class VariableDAG: public Node
  {
  public:
    string name;
    double init;
    shared_ptr<Node> rhs;
    VariableDAG(const string& name=""): name(name) {}
    int BODMASlevel() const {return 0;}
    int order() const {return rhs? rhs->order()+1: 0;}
    ostream& latex(ostream&) const;
    ostream& matlab(ostream&) const;
    using Node::latex;
    using Node::matlab;
  };

  struct OperationDAGBase: public Node, public OperationType  
  {
    vector<vector<shared_ptr<Node> > > arguments;
    string name;
    double init;
    OperationDAGBase(const string& name=""): name(name) {}
    virtual Type type() const=0;
    /// factory method 
    static OperationDAGBase* create(Type type, const string& name="");
    int order() const;
  };

  template <OperationType::Type T>
  struct OperationDAG: public OperationDAGBase
  {
    Type type() const {return T;}
    OperationDAG(const string& name=""): OperationDAGBase(name) {}
    int BODMASlevel() const {
      switch (type())
        {
        case OperationType::multiply:
        case OperationType::divide:
          return 1;
        case OperationType::subtract:
        case OperationType::add:
          return 2;
        case OperationType::constant: // varies, depending on what's in it
          if (name.find_first_of("+-")!=string::npos)
            return 2;
          else
            return 1;
        default:
          return 0;
        }
    }
    ostream& latex(ostream&) const; 
    ostream& matlab(ostream& o) const;
    using Node::latex;
    using Node::matlab;
  };

  /// represents a Godley column
  struct GodleyColumnDAG: public Node, public vector<string>
  {
    int BODMASlevel() const {return 2;}
    ostream& latex(ostream&) const; 
    ostream& matlab(ostream&) const;
    int order() const {return 0;} // Godley columns define integration vars
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
    OperationDAGBase* makeDAG(const OperationBase& op);

    // creates a node object representing what feeds the wire
    Node* createNodeFromWire(int wire);

    void processGodleyTable
    (map<string, GodleyColumnDAG>& godleyVariables, const GodleyTable& godley);

  public:
    /// construct the system of equations 
    SystemOfEquations(const Minsky&);
    ostream& latex(ostream&) const; ///< render as a LaTeX eqnarray
    ostream& matlab(ostream&) const; ///< render as MatLab code
  };

}


#endif
