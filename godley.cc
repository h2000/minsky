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
#include "godley.h"
#include "port.h"
#include "variableManager.h"

const char* GodleyTable::initialConditions="Initial Conditions";

bool GodleyTable::initialConditionRow(unsigned row) const
{
  const string& label=cell(row,0);
  static size_t initialConditionsSz=strlen(initialConditions);
  size_t i, j;
  // trim any leading whitespaces
  for (i=0; isspace(label[i]); ++i);
  // compare case insensitively
  for (j=0; j<initialConditionsSz && i<label.size() && 
         toupper(label[i])==toupper(initialConditions[j]); ++i, ++j);
  return j==initialConditionsSz;
}

void GodleyTable::InsertRow(unsigned row)
{
  if (row<=data.size())
    data.insert(data.begin()+row, vector<string>(cols()));
}

void GodleyTable::DeleteRow(unsigned row)
{
  if (row>0 && row<=data.size())
    data.erase(data.begin()+row-1);
}

void GodleyTable::InsertCol(unsigned col)
{
  m_assetClass.insert(m_assetClass.begin()+col,noAssetClass);
  if (data.size()>0 && col<=data[0].size())
    for (unsigned row=0; row<data.size(); ++row)
      data[row].insert(data[row].begin()+col, "");
}

void GodleyTable::DeleteCol(unsigned col)
{
  m_assetClass.erase(m_assetClass.begin()+col-1);
  if (col>0 && col<=data[0].size())
    for (unsigned row=0; row<rows(); ++row)
      data[row].erase(data[row].begin()+col-1);
}

vector<string> GodleyTable::getColumnVariables() const
{
  set<string> uvars; //set for uniqueness checking
  vector<string> vars;
  for (size_t c=1; c<cols(); ++c)
    {
      string var=cell(0,c);
      stripNonAlnum(var);
      if (!var.empty())
        {
          if (!uvars.insert(var).second)
            throw error("Duplicate column label detected");
          vars.push_back(var);
        }
    }
  return vars;
}

vector<string> GodleyTable::getVariables() const
{
  vector<string> vars; 
  set<string> uvars; //set for uniqueness checking
  for (size_t r=1; r<rows(); ++r)
    if (!initialConditionRow(r))
      for (size_t c=1; c<cols(); ++c)
        {
          string var=cell(r,c);
          stripNonAlnum(var);
          if (!var.empty() && uvars.insert(var).second)
            vars.push_back(var);
        }
  return vars;
}

GodleyTable::AssetClass GodleyTable::_assetClass(int col) const 
{
  return col<m_assetClass.size()? m_assetClass[col]: noAssetClass;
}

GodleyTable::AssetClass GodleyTable::_assetClass
(int col, GodleyTable::AssetClass cls) 
{
  if (col>=m_assetClass.size())
    m_assetClass.resize(cols(), noAssetClass);
  m_assetClass[col]=cls;
  return _assetClass(col);
}


string GodleyTable::assetClass(TCL_args args)
{
  int col=args;
  if (args.count) 
    return classdesc::enumKey<AssetClass>
      (_assetClass
       (col, AssetClass(classdesc::enumKey<AssetClass>((char*)args))));
  else
    return classdesc::enumKey<AssetClass>(_assetClass(col));
}

string GodleyTable::RowSum(int row) const
{
  // accumulate the total for each variable
  map<string,double> sum;
  for (int c=1; c<cols(); ++c)
    {
      const string& formula = cell(row,c);
      const char* f=formula.c_str();
      char* tail;
      // attempt to read leading numerical value
      double coef=strtod(f,&tail);
      if (tail==f) // oops, that failed, check if there's a leading - sign
        {
          // skip whitespace
          while (*tail != '\0' && isspace(*tail)) ++tail;
          if (*tail=='\0') continue; // empty cell, nothing to do
          if (*tail=='-') 
            {
              coef=-1; // treat leading - sign as -1
              tail++;
            }
          else
            coef=1;
        }

      // whatever's left is the name of the variable. Add characters,
      // but avoid adding trailing whitspace
      string varName;
      int numWhite=0;
      for (;*tail!='\0'; ++tail) 
        {
          if (isspace(*tail))
            numWhite++;
          else if (numWhite)
            {
              varName.append(' ',numWhite);
              numWhite=0;
            }
          varName+=*tail;
        }

      sum[varName]+=coef;
    }

  // create symbolic representation of each term
  ostringstream ret;
  for (map<string,double>::iterator i=sum.begin(); i!=sum.end(); ++i)
    if (i->second!=0)
      {
        if (!ret.str().empty() &&i->second>0)
          ret<<"+";
        if (i->second==-1)
          ret<<"-";
        else if (i->second!=1)
          ret<<i->second;
        ret<<i->first;
      }

  //if completely empty, substitute a zero
  if (ret.str().empty()) 
    return "0";
  else 
    return ret.str();

}

void GodleyTable::SetDEmode(bool mode)
{
  if (mode==doubleEntryCompliant) return;
  doubleEntryCompliant=true; // to allow signConventionReversed to work
  for (int r=1; r<rows(); ++r)
    if (!initialConditionRow(r))
      for (int c=1; c<cols(); ++c)
        if (signConventionReversed(c))
          {
            string& formula=cell(r,c);
            unsigned start=0;
            while (start<formula.length() && isspace(formula[start])) start++;
            if (start==formula.length()) continue; // empty cell
            if (formula[start]=='-')
              formula.erase(start,1); // turns a negative into a positive
            else
              formula.insert(start,"-");
          }
  doubleEntryCompliant=mode;
}
