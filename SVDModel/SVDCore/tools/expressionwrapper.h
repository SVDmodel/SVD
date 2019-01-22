/********************************************************************************************
**    SVD - the scalable vegetation dynamics model
**    https://github.com/SVDmodel/SVD
**    Copyright (C) 2018-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/
#ifndef EXPRESSIONWRAPPERH
#define EXPRESSIONWRAPPERH

#include <vector>
#include <string>

class ExpressionWrapper
{
public:
    ExpressionWrapper();
    virtual ~ExpressionWrapper() {}
    virtual const std::vector<std::string> &getVariablesList();
    virtual double value(const size_t variableIndex);
    virtual double valueByName(const std::string &variableName);
    virtual int variableIndex(const std::string &variableName);
};

class InferenceData; // forward

/// Wrapper for inference data
class InferenceDataWrapper: public ExpressionWrapper
{
public:
    InferenceDataWrapper(const InferenceData *data) : mData(data) {}
    void setData(const InferenceData *data) {mData = data; }

    virtual const std::vector<std::string> &getVariablesList() { return mVariableList; }
    virtual double value(const size_t variableIndex);
private:
    static std::vector<std::string> mVariableList;
    const InferenceData *mData;
};


class EnvironmentCell; // forward
class State; // forward
class Cell; // forward

/// Wrapper for Cell data (environment, state meta data, ...)
class CellWrapper: public ExpressionWrapper
{
public:
    CellWrapper(const Cell *data) : mData(data) {}
    void setData(const Cell *data) { mData = data; }

    /// fetch variable names from environment and state and add to the wrapper object.
    static void setupVariables(EnvironmentCell *ecell, const State *astate);

    virtual const std::vector<std::string> &getVariablesList() { return mVariableList; }
    virtual double value(const size_t variableIndex);

private:
    static std::vector<std::string> mVariableList;
    static size_t mMaxStateVar;

    const Cell *mData;
};

/*
class TTree;
class TreeWrapper: public ExpressionWrapper
{
public:
    TreeWrapper() : mTree(0) {}
    TreeWrapper(const TTree* tree) : mTree(tree) {}
    void setTree(const TTree* tree) { mTree = tree; }
    virtual const std::vector<std::string> &getVariablesList();
    virtual double value(const int variableIndex);

private:
    const TTree *mTree;
};*/


#endif // EXPRESSIONWRAPPERH
