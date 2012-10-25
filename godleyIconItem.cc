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
namespace
{
  struct GodleyIconItem: public XGLItem
  {
    void draw()
    {
      if (cairoSurface)
        {
          CairoRenderer renderer(cairoSurface->surface());
          xgl drawing(renderer);
          drawing.load(xglRes.c_str());
          drawing.render();
          array<double> bbox=boundingBox();

          

          cairoSurface->blit
            (bbox[0]+0.5*cairoSurface->width(), 
             bbox[1]+0.5*cairoSurface->height(), 
             bbox[2]-bbox[0], bbox[3]-bbox[1]);
    }
  }
  }
}

void updateVars(map<string, Variable> vars, const set<string> varNames, 
                Variable::Type varType)
{
  // update the map of variables from the Godley table
  map<string, Variable> oldVars;

  oldVars.swap(vars);
  for (set<string>::const_iterator nm=varNames.begin(); nm!=varNames.end(); ++nm)
    {
      map<string, Variable>::const_iterator v=oldVars.find(*nm);
      if (v==oldVars.end())
        // add new variable
        vars.insert(make_pair(*nm, Variable(varType, *nm)));
      else
        // copy existing variable
        vars.insert(make_pair(*nm, v->second));
    }
  for (map<string, Variable>::iterator v=oldVars.begin(); v!=oldVars.end(); ++v)
    if (varNames.count(v->first)==0)
      {
        // remove ports associated with deleted variables
        portManager.delPort(v->second.inPort());
        portManager.delPort(v->second.outPort());
      }
}

void GodleyIcon::update()
{
  updateVariables(stockVars, table.getColumnVariables(), Variable::stock);
  updateVariables(flowVars, table.getVariables(), Variable::flow);

  // adjust positioning of the variables.
}


