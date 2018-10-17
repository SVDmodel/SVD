#ifndef EXPRESSIONWRAPPERH
#define EXPRESSIONWRAPPERH

#include <vector>

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
