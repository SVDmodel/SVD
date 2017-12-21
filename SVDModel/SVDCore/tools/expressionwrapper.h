#ifndef EXPRESSIONWRAPPERH
#define EXPRESSIONWRAPPERH

#include <vector>

class ExpressionWrapper
{
public:
    ExpressionWrapper();
    virtual const std::vector<std::string> &getVariablesList();
    virtual double value(const int variableIndex);
    virtual double valueByName(const std::string &variableName);
    virtual int variableIndex(const std::string &variableName);
};

class InferenceData; // forward
class CellWrapper: public ExpressionWrapper
{
public:
    CellWrapper(const InferenceData *data) : mData(data) {}
    void setData(const InferenceData *data) {mData = data; }
    virtual const std::vector<std::string> &getVariablesList() { return mVariableList; }
    virtual double value(const int variableIndex);
private:
    static std::vector<std::string> mVariableList;
    const InferenceData *mData;
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
