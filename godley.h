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
#ifndef GODLEY_H
#define GODLEY_H

#include <set>
#include <vector>
using namespace std;

#include <ecolab.h>

#include "variable.h"

namespace minsky
{
  class GodleyTable
  {
  public:

    enum AssetClass {noAssetClass, asset, liability, equity};
    friend struct SchemaHelper;
  private:
    vector<vector<string> > data;
    CLASSDESC_ACCESS(GodleyTable);
    /// class of each column (used in DE compliant mode)
    vector<AssetClass> m_assetClass;
  
  public:

    bool doubleEntryCompliant;

    std::string title;
  
    static const char* initialConditions;
    GodleyTable(): doubleEntryCompliant(false)
    {
      Dimension(2,2);
      cell(0,0)="Flows V / Stock Variables ->";
      cell(1,0)=initialConditions;
    }

    /// class of each column (used in DE compliant mode)
    AssetClass _assetClass(int col) const;
    AssetClass _assetClass(int col, AssetClass cls);

    /// The usual mathematical sign convention is reversed in double
    /// entry book keeping conventions if the asset class is a liability
    /// or equity
    bool signConventionReversed(int col) const
    {
      return doubleEntryCompliant && 
        (_assetClass(col)==liability || _assetClass(col)==equity);
    }
    /**
       TCL accessor method 
       @param col - column number
       @param [opt] assetClass (symbolic name).
       @return current asset class value for column \a col
       sets if assetClass present, otherwise gets
    */
    string assetClass(TCL_args args);
  
    // returns true if \a row is an "Initial Conditions" row
    bool initialConditionRow(unsigned row) const;

    size_t rows() const {return data.size();}
    size_t cols() const {return data.empty()? 0: data[0].size();}

    void clear() {data.clear();}
    void Resize(unsigned rows, unsigned cols) {
      // resize existing
      for (size_t i=0; i<data.size(); ++i) data[i].resize(cols);
      data.resize(rows, vector<string>(cols));
      m_assetClass.resize(cols, noAssetClass);
    }
    void resize(TCL_args args) {Resize(args[0],args[1]);}

    void InsertRow(unsigned row);
    void insertRow(TCL_args args) {InsertRow(args);}
    void DeleteRow(unsigned row);
    void deleteRow(TCL_args args) {DeleteRow(args);}
    void InsertCol(unsigned col);
    void insertCol(TCL_args args) {InsertCol(args);}
    void DeleteCol(unsigned col);
    void deleteCol(TCL_args args) {DeleteCol(args);}
  
    void Dimension(unsigned rows, unsigned cols) {clear(); Resize(rows,cols);}
    void dimension(TCL_args args) {Dimension(args[0], args[1]);}

    string& cell(unsigned row, unsigned col) {
      assert(row<rows() && col<cols());
      return data[row][col];
    }
    const string& cell(unsigned row, unsigned col) const {return data[row][col];}
    string getCell(TCL_args args) const {
      unsigned row=args, col=args;
      if (row<rows() && col<cols())
        return cell(row,col);
      else
        return "";
    }
    void setCell(TCL_args args) {cell(args[0],args[1])=(char*)args[2];}

    /// get the set of column labels, in column order
    vector<string> getColumnVariables() const;
    /// get the vector of unique variable names from the interior of the
    /// table, in row, then column order
    vector<string> getVariables() const;

    /// toggle flow signs according to double entry compliant mode
    void SetDEmode(bool doubleEntryCompliant);
    void setDEmode(TCL_args args) {SetDEmode(args);}

    /// return the symbolic sum across a row
    string RowSum(int row) const;
    string rowSum(TCL_args args) {return RowSum(args);}

    /// accessor for schema access
    const vector<vector<string> >& getData() const {return data;}
  };

  // needed for remove_if below
  inline bool IsNotalnum(char x) {return !std::isalnum(x);}
  // strip non alphanum characters - eg signs
  inline void stripNonAlnum(string& x) {
    x.erase(remove_if(x.begin(), x.end(), IsNotalnum), x.end());
  } 
}

#include "godley.cd"
#endif
