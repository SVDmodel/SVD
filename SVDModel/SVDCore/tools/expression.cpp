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

#include "expression.h"
#include "strtools.h"

#include "expressionwrapper.h"

#include <algorithm>
#include <cassert>
#include <mutex>
#include "randomgen.h"
#include "strtools.h"

/** @class Expression
  An expression engine for mathematical expressions provided as strings.
  @ingroup tools
  @ingroup script
  The main purpose is fast execution speed.
  notes regarding the syntax:
  +,-,*,/ as expected, additionally "^" for power.
  mod(x,y): modulo division, gets remainder of x/y
  functions:
    - sin cos tan
    - exp ln sqrt
    - min max: variable number of arguments, e.g: min(x,y,z)
    - if: if(condition, true, false): if condition=true, return true-case, else false-case. note: both (true, false) are evaluated anyway!
    - incsum: ?? incremental sum - currently not supported.
    - polygon: special function for polygons. polygon(value, x1,y1, x2,y2, x3,y3, ..., xn,yn): return is: y1 if value<x1, yn if value>xn, or the lineraly interpolated numeric y-value.
    - sigmoid: returns a sigmoid function. sigmoid(value, type, param1, param2). see udfSigmoid() for details.
    - rnd rndg: random functions; rnd(from, to): uniform random number, rndg(mean, stddev): gaussian randomnumber (mean and stddev in percent!)
    - limit: combined min/max, lets you limt a number in a provided range
    The Expression class also supports some logical operations:
    (logical) True equals to "1", "False" to zero. The precedence rules for parentheses...
    - and
    - or
    - not
  @par Using Model Variables
  With the help of descendants of ExpressionWrapper values of model objects can be accessed. Example Usage:
  @code
  TreeWrapper wrapper;
  Expression basalArea("dbh*dbh*3.1415/4", &wrapper); // expression for basal area, add wrapper (see also setModelObject())
  AllTreeIterator at(GlobalSettings::instance()->model()); // iterator to iterate over all tree in the model
  double sum;
  while (Tree *tree = at.next()) {
      wrapper.setTree(tree); // set actual tree
      sum += basalArea.execute(); // execute calculation
  }
  @endcode

  Be careful with multithreading:
  Now the calculate(double v1, double v2) as well as the calculate(wrapper, v1,v2) are thread safe. execute() accesses the internal variable list and is therefore not thredsafe.
  A threadsafe version exists (executeLocked()). Special attention is needed when using setVar() or addVar().

*/


#define opEqual 1
#define opGreaterThen 2
#define opLowerThen 3
#define opNotEqual 4
#define opLowerOrEqual 5
#define opGreaterOrEqual 6
#define opAnd 7
#define opOr  8

static std::vector<std::string> mathFuncList={"sin", "cos", "tan",
                                       "exp", "ln", "sqrt",
                                       "min", "max", "if",
                                       "incsum", "polygon", "mod", "sigmoid", "rnd", "rndg", "limit",  "round", "in",
                                             "localNB", "intermediateNB", "globalNB"};
const int  MaxArgCount[21]={1,1,1,1,  1, 1,   -1, -1, 3, 1, -1, 2, 4, 2, 2, 3, 1, -1,    -1,-1,-1};
#define    AGGFUNCCOUNT 6
static std::string AggFuncList[AGGFUNCCOUNT]={"sum", "avg", "max", "min", "stddev", "variance"};

bool Expression::mLinearizationAllowed = false;
Expression::Expression()
{
    mModelObject = nullptr;
    m_externVarSpace=nullptr;
    m_execList=nullptr;
}


Expression::ETokType  Expression::next_token()
{
    m_tokCount++;
    m_lastState=m_state;
    // eliminate whitespaces
    while (strchr(" \t\n\r", *m_pos) && *m_pos)
        m_pos++;

    if (*m_pos==0) {
        m_state=etStop;
        m_token="";
        return etStop;
    }
    // eliminate whitespaces
    while (strchr(" \t\n\r", *m_pos))
        m_pos++;
    if (*m_pos==',')
    {

        m_token=*m_pos++;
        m_state=etDelimeter;
        return etDelimeter;
    }
    if (strchr("+-*/(){}^", *m_pos)) {
        m_token=*m_pos++;
        m_state=etOperator;
        return etOperator;
    }
    if (strchr("=<>", *m_pos)) {
        m_token=*m_pos++;
        if (*m_pos=='>' || *m_pos=='=')
            m_token+=*m_pos++;
        m_state=etCompare;
        return etCompare;
    }
    if (*m_pos>='0' && *m_pos<='9') {
        // number
        m_token = to_string(atof(m_pos));

        while (strchr("0123456789.",*m_pos) && *m_pos!=0)
            m_pos++;  // allowed values

        m_state=etNumber;
        return etNumber;
    }

    if ((*m_pos>='a' && *m_pos<='z') || (*m_pos>='A' && *m_pos<='Z') || (*m_pos=='_')) {
        // function ... find brace
        m_token="";
        while (( (*m_pos>='a' && *m_pos<='z') || (*m_pos>='A' && *m_pos<='Z')
                || (*m_pos>='0' && *m_pos<='9') || (*m_pos=='_' || *m_pos=='.') )
            && *m_pos!='(' && m_pos!=nullptr )
            m_token+=*m_pos++;
        // brace -> function, else variable.
        if (*m_pos=='(' || *m_pos=='{') {
            m_pos++; // skip brace
            m_state=etFunction;
            return etFunction;
        } else {
            if (lowercase(m_token)=="and" || lowercase(m_token)=="or") {
                m_state=etLogical;
                return etLogical;
            } else {
                m_state=etVariable;
                // support for pseudo-literals 'true' and 'false'
                if (m_token=="true") { m_state=etNumber; m_token="1"; return etNumber; }
                if (m_token=="false") { m_state=etNumber; m_token="0"; return etNumber; }
                return etVariable;
            }
        }
    }
    m_state=etUnknown;
    return etUnknown; // in case no match was found

}

Expression::~Expression()
{
    if (m_execList)
        delete[] m_execList;
}


/** sets expression @p expr and checks the syntax (parse).
    Expressions are setup with strict = false, i.e. no fixed binding of variable names.
  */
void Expression::setAndParse(const std::string &expr)
{
    setExpression(expr);
    m_strict = false; 
    parse();
}

/// set the current expression.
/// do some preprocessing (e.g. handle the different use of ",", ".", ";")
void Expression::setExpression(const std::string& aExpression)
{
    m_expression=trimmed(aExpression);

    m_expr=const_cast<char*>(m_expression.c_str());

    m_pos=m_expr;  // set starting point...

    for (int i=0; i<MAXLOCALVAR; i++)
        m_varSpace[i]=0.;
    m_parsed=false;
    m_catchExceptions = false;
    m_errorMsg = "";

    mModelObject = nullptr;
    m_externVarSpace=nullptr;

    m_strict=true; // default....
    m_incSumEnabled=false;
    m_empty= (m_expression=="") ;
    // Buffer:
    m_execListSize = 5; // inital value...
    m_execList = new ExtExecListItem[m_execListSize]; // init

    mLinearizeMode = 0; // linearization is switched off
    mScriptIndexFunc=nullptr;
    mScriptValueFunc=nullptr;
}


static std::mutex parse_mutex;
void  Expression::parse(ExpressionWrapper *wrapper)
{
    std::lock_guard<std::mutex> guard(parse_mutex);
    if (m_parsed)
        return;
    try {
        ExpressionWrapper *old_wrap=mModelObject;
        if (wrapper) {
           mModelObject = wrapper;
        }
        // Picus compatibility with old functions:
        if (m_expression.find(';')!=std::string::npos) {
            // e.g. change from "polygon(x; 0; 0,3; 0.5)" -> "polygon(x, 0.3, 0.5)"
            //std::replace(m_expression.begin(), m_expression.end(), ',', '.');
            //std::replace(m_expression.begin(), m_expression.end(), ';', ',');
            throw std::logic_error("Expression contains ';':" + m_expression);
       }

        //
        m_tokString="";
        m_state=etUnknown;
        m_lastState=etUnknown;
        m_constExpression=true;
        m_execIndex=0;
        m_tokCount=0;
        int AktTok;
        next_token();
        while (m_state!=etStop) {
            m_tokString+="\n"+m_token;
            AktTok=m_tokCount;
            parse_levelL0();  // start with logical level 0
            if (AktTok==m_tokCount)
                throw std::logic_error("Expression::parse(): Unbalanced Braces.");
            if (m_state==etUnknown){
                m_tokString+="\n***Error***";
                throw std::logic_error("Expression::parse(): Syntax error, token: " + m_token);
            }
        }
        m_empty = (m_execIndex == 0);
        m_execList[m_execIndex].Type=etStop;
        m_execList[m_execIndex].Value=0;
        m_execList[m_execIndex++].Index=0;
        checkBuffer(m_execIndex);
        m_parsed=true;

        mModelObject = old_wrap;

    } catch (const std::exception& e) {
        m_errorMsg ="Expression::parse: Error in: " + m_expression +":" +  e.what();
        throw std::logic_error(m_errorMsg);
    }
}

void  Expression::parse_levelL0()
{
    // logical operations  (and, or, not)
    std::string op;
    parse_levelL1();

    while (m_state==etLogical)  {
        op=lowercase(m_token);
        next_token();
        parse_levelL1();
        int logicaltok=0;
        if (op=="and") logicaltok=opAnd;
        if (op=="or") logicaltok=opOr;


        m_execList[m_execIndex].Type=etLogical;
        m_execList[m_execIndex].Value=0;
        m_execList[m_execIndex++].Index=logicaltok;
        checkBuffer(m_execIndex);
    }
}

void  Expression::parse_levelL1()
{
    // logic operations (<,>,=,...)
    std::string op;
    parse_level0();
    while (m_state==etCompare)  {
        op=m_token;
        next_token();
        parse_level0();
        int logicaltok=0;
        if (op=="<") logicaltok=opLowerThen;
        if (op==">") logicaltok=opGreaterThen;
        if (op=="<>") logicaltok=opNotEqual;
        if (op=="<=") logicaltok=opLowerOrEqual;
        if (op==">=") logicaltok=opGreaterOrEqual;
        if (op=="=")  logicaltok=opEqual;

        m_execList[m_execIndex].Type=etCompare;
        m_execList[m_execIndex].Value=0;
        m_execList[m_execIndex++].Index=logicaltok;
        checkBuffer(m_execIndex);
    }
}

void  Expression::parse_level0()
{
    // plus and minus
    std::string op;
    parse_level1();

    while (m_token=="+" || m_token=="-")  {
        op=m_token;
        next_token();
        parse_level1();
        m_execList[m_execIndex].Type=etOperator;
        m_execList[m_execIndex].Value=0;
        m_execList[m_execIndex++].Index=op.at(0);///op.constData()[0];
        checkBuffer(m_execIndex);
    }

}

void  Expression::parse_level1()
{
    // divide and multiply
    std::string op;
    parse_level2();
    while (m_token=="*" || m_token=="/") {
        op=m_token;
        next_token();
        parse_level2();
        m_execList[m_execIndex].Type=etOperator;
        m_execList[m_execIndex].Value=0;
        m_execList[m_execIndex++].Index=op.at(0);
        checkBuffer(m_execIndex);
    }
}

void  Expression::atom()
{
    if (m_state==etVariable || m_state==etNumber) {
        if (m_state==etNumber) {
            double result=atof(m_token.c_str());
            m_execList[m_execIndex].Type=etNumber;
            m_execList[m_execIndex].Value=result;
            m_execList[m_execIndex++].Index=-1;
            checkBuffer(m_execIndex);
        }
        if (m_state==etVariable) {
            if (!m_strict) // in strict mode, the variable must be available by external bindings. in "lax" mode, the variable is added when encountered first.
                addVar(m_token);
            m_execList[m_execIndex].Type=etVariable;
            m_execList[m_execIndex].Value=0;
            m_execList[m_execIndex++].Index=getVarIndex(m_token);
            checkBuffer(m_execIndex);
            m_constExpression=false;
        }
        next_token();
    } else if (m_state==etStop || m_state==etUnknown)
        throw std::logic_error("Unexpected end of Expression: " + m_expression);
}


void  Expression::parse_level2()
{
    // x^y
    parse_level3();
    while (m_token=="^") {
        next_token();
        parse_level3();
        m_execList[m_execIndex].Type=etOperator;
        m_execList[m_execIndex].Value=0;
        m_execList[m_execIndex++].Index='^';
        checkBuffer(m_execIndex);
    }
}
void  Expression::parse_level3()
{
    // unary operator (- and +)
    std::string op;
    op=m_token;
    bool Unary=false;
    if (op=="-" && (m_lastState==etOperator || m_lastState==etUnknown || m_lastState==etCompare || m_lastState==etLogical || m_lastState==etFunction)) {
        next_token();
        Unary=true;
    }
    parse_level4();
    if (Unary && op=="-") {
        m_execList[m_execIndex].Type=etOperator;
        m_execList[m_execIndex].Value=0;
        m_execList[m_execIndex++].Index='_';
        checkBuffer(m_execIndex);
    }

}

void  Expression::parse_level4()
{
    // braces and functions
    std::string func;
    atom();
    if (m_token=="(" || m_state==etFunction) {
        func=m_token;
        if (func=="(")   // brace
        {
            next_token();
            parse_levelL0();
        }   else {       // function...
            int argcount=0;
            int idx=getFuncIndex(func);
            next_token();
            // multiple args
            while (m_token!=")") {
                argcount++;
                parse_levelL0();
                if (m_state==etDelimeter)
                    next_token();
            }
            if (MaxArgCount[idx]>0 && MaxArgCount[idx]!=argcount)
                throw std::logic_error( "Function " + func + " assumes " + to_string(MaxArgCount[idx]) + " arguments!");
            m_execList[m_execIndex].Type=etFunction;
            m_execList[m_execIndex].Value=argcount;
            m_execList[m_execIndex++].Index=idx;
            checkBuffer(m_execIndex);
        }
        if (m_token!="}" && m_token!=")") // error
            throw std::logic_error("Expression::unbalanced number of parentheses in [" + m_expression + "].");
        next_token();
    }
}

void Expression::setVar(const std::string& Var, double Value)
{
    if (!m_parsed)
        parse();
    int idx=getVarIndex(Var);
    if (idx>=0 && idx<MAXLOCALVAR)
        m_varSpace[idx]=Value;
    else
        throw std::logic_error("Invalid variable " + Var);
}

double Expression::calculate(const double Val1, const double Val2, const bool forceExecution) const
{
    if (mLinearizeMode>0 && !forceExecution) {
        if (mLinearizeMode==1)
            return linearizedValue(Val1);
        return linearizedValue2d(Val1, Val2); // matrix case
    }
    double var_space[MAXLOCALVAR];
    var_space[0]=Val1;
    var_space[1]=Val2;
    m_strict=false;
    return execute(var_space); // execute with local variables on stack
}

double Expression::calculate(ExpressionWrapper &object, const double variable_value1, const double variable_value2) const
{
    double var_space[MAXLOCALVAR];
    var_space[0] = variable_value1;
    var_space[1]=variable_value2;
    // m_strict=false;
    return execute(var_space,&object); // execute with local variables on stack
}


int Expression::getFuncIndex(const std::string& functionName)
{
    int idx=index_of(mathFuncList, functionName);
    if (idx<0)
        throw std::logic_error("Function " + functionName + " not defined!");
    return idx;
}

double Expression::execute(double *varlist, ExpressionWrapper *object, bool *rLogicResult) const
{
    if (!m_parsed)
        const_cast<Expression*>(this)->parse(object);
    const double *varSpace = varlist?varlist:m_varSpace;
    ExtExecListItem *exec=m_execList;
    int i;
    double result;
    double Stack[20];
    bool   LogicStack[20];
    bool   *lp=LogicStack;
    double *p=Stack;  // p=head pointer
    *lp++=true;
    if (isEmpty()) {
        return 0.;
    }
    while (exec->Type!=etStop) {
        switch (exec->Type) {
        case etOperator:
            p--;
            switch (exec->Index) {
                  case '+': *(p-1)=*(p-1) + *p;  break;
                  case '-': *(p-1)=*(p-1)-*p;  break;
                  case '*': *(p-1)=*(p-1) * *p;  break;
                  case '/': *(p-1)=*(p-1) / *p;  break;
                  case '^': *(p-1)=pow(*(p-1), *p);  break;
                  case '_': *p=-*p; p++; break;  // unary operator -
                  }
            break;
        case etVariable:
            if (exec->Index<100)
                *p++=varSpace[exec->Index];
            else if (exec->Index<1000)
                *p++=getModelVar(exec->Index,object);
            else
                *p++=getExternVar(exec->Index);
            break;
        case etNumber:
            *p++=exec->Value;
            break;
        case etFunction:
            p--;
            switch (exec->Index) {
            case 0: *p=sin(*p); break;
            case 1: *p=cos(*p); break;
            case 2: *p=tan(*p); break;
            case 3: *p=exp(*p); break;
            case 4: *p=log(*p); break;
            case 5: *p=sqrt(*p); break;
                // min, max, if:  variable number of arguments
            case 6:      // min
                for (i=0;i<exec->Value-1;i++,p--)
                    *(p-1)=(*p<*(p-1))?*p:*(p-1);
                break;
            case 7:  //max
                for (i=0;i<exec->Value-1;i++,p--)
                    *(p-1)=(*p>*(p-1))?*p:*(p-1);
                break;
            case 8: // if
                if (*(p-2)==1) // true
                    *(p-2)=*(p-1);
                else
                    *(p-2)=*p; // false
                p-= 2; // drop both arguments
                break;
            case 9: // incremental sum
                m_incSumVar+=*p;
                *p=m_incSumVar;
                break;
            case 10: // polygon-function
                *(p-(int)(exec->Value-1))=udfPolygon(*(p-(int)(exec->Value-1)), p, (int)exec->Value);
                p-=(int) (exec->Value-1);
                break;
            case 11: // modulo division: result= remainder of arg1/arg2
                p--;
                *p=fmod(*p, *(p+1));
                break;
            case 12: // user-defined-function: sigmoid
                *(p-3)=udfSigmoid(*(p-3), *(p-2), *(p-1), *p);
                p-=3; // drop three args (4-1) ...
                break;
            case 13: case 14: // rnd(from, to) bzw. rndg(mean, stddev)
                p--;
                *p=udfRandom(exec->Index-13, *p, *(p+1));
                break;
            case 15: {// limit(value, lower_bound, upper_bound)
                double m = *(p-2)<*(p-1)?*(p-1):*(p-2); // limit to lower
                *(p-2) = m>*p?*p:m;  // ... and then to upper bound
                p-=2; // drop the arguments
                break;
            }
            case 16: // round number
                *p=floor(*p + 0.5); break;
            case 17: // in(x, ....) - return true if x equals one of the arguments
                *(p-(int)(exec->Value-1))=udfIn(*(p-(int)(exec->Value-1)), p, (int)exec->Value);
                p-=(int) (exec->Value-1);
                break;
            case 18: // localNB
                *(p-(int)(exec->Value-1)) = udfNeighborhood(object, 1, p, (int)exec->Value);
                p-=(int) (exec->Value-1);
                break;
            case 19: // intermediateNB
                *(p-(int)(exec->Value-1)) = udfNeighborhood(object, 2, p, (int)exec->Value);
                p-=(int) (exec->Value-1);
                break;
            case 20: // globalNB
                *(p-(int)(exec->Value-1)) = udfNeighborhood(object, 3, p, (int)exec->Value);
                p-=(int) (exec->Value-1);
                break;
            }
            p++;
            break;
        case etLogical:
            p--;
            lp--;
            switch (exec->Index) {
                case opAnd: *(lp-1)=(*(lp-1) && *lp);  break;
                case opOr:  *(lp-1)=(*(lp-1) || *lp);  break;
            }
            if (*(lp-1))
                *(p-1)=1;
            else
                *(p-1)=0;
            break;
        case etCompare: {
            p--;
            bool LogicResult=false;
            switch (exec->Index) {
                 case opEqual: LogicResult=(*(p-1)==*p); break;
                 case opNotEqual: LogicResult=(*(p-1)!=*p); break;
                 case opLowerThen: LogicResult=(*(p-1)<*p); break;
                 case opGreaterThen: LogicResult=(*(p-1)>*p); break;
                 case opGreaterOrEqual: LogicResult=(*(p-1)>=*p); break;
                 case opLowerOrEqual: LogicResult=(*(p-1)<=*p); break;
                 }
            if (LogicResult) {
                *(p-1)=1;   // 1 means true...
            } else {
                *(p-1)=0;
            }

            *lp++=LogicResult;
            break; }
        case etStop: case etUnknown: case etDelimeter: throw std::logic_error("invalid token during execution.");
        } // switch()

        exec++;
    }
    if (p-Stack!=1)
        throw std::logic_error("Expression::execute: stack unbalanced!");
    result=*(p-1);
    if (rLogicResult)
        *rLogicResult = *(lp-1);

    return result;
}

double * Expression::addVar(const std::string& VarName)
{
    // add var
    int idx=index_of(m_varList, VarName);
    if (idx==-1) {
        m_varList.push_back(VarName);
    }
    return &m_varSpace[getVarIndex(VarName)];
}

double *  Expression::getVarAdress(const std::string& VarName)
{
    if (!m_parsed)
        parse();
    int idx=getVarIndex(VarName);
    if (idx>=0 && idx<MAXLOCALVAR)
        return &m_varSpace[idx];
    else
        throw std::logic_error("Expression::getVarAdress: Invalid variable <"+ VarName +"> ");
}

int  Expression::getVarIndex(const std::string& variableName)
{
    int idx;

    if (mModelObject) {
        idx = mModelObject->variableIndex(variableName); // was lowercase(variableName) - I think we are strict in SVD
        if (idx>-1)
            return 100 + idx;
    }
    if (mScriptIndexFunc) {
        idx = (*mScriptIndexFunc)(variableName);
        if (idx>-1)
            return 1000 + idx;
    }

    // external variables
    if (!(m_externVarNames.size()==0))
    {
        idx=index_of(m_externVarNames, variableName);
        if (idx>-1)
            return 1000 + idx;
    }
    idx = index_of(m_varList, variableName);
    if (idx>-1)
        return idx;
    // if in strict mode, all variables must be already available at this stage.
    if (m_strict) {
        m_errorMsg = "Variable '" + variableName + "' in (strict) expression '" + m_expression + "' not available!";
        if (!m_catchExceptions)
            throw std::logic_error(m_errorMsg);
   }
    return -1;
}

inline double Expression::getModelVar(const int varIdx, ExpressionWrapper *object) const
{

    ExpressionWrapper *model_object = object?object:mModelObject;
    int idx=varIdx - 100; // saved as 100+x
    if (model_object)
        return model_object->value(idx);
    throw std::logic_error("Expression::getModelVar: invalid modell variable!");

}

void Expression::setExternalVarSpace(const std::vector<std::string>& ExternSpaceNames, double* ExternSpace)
{
    m_externVarSpace=ExternSpace;
    m_externVarNames=ExternSpaceNames;
}

double Expression::getExternVar(int Index) const
{
    if (mScriptValueFunc)
        return (*mScriptValueFunc)(Index-1000);
    else
        return m_externVarSpace[Index-1000];
}

void Expression::enableIncSum()
{
    m_incSumEnabled=true;
    m_incSumVar=0.;
}

// "Userdefined Function" Polygon
double  Expression::udfPolygon(double Value, double* Stack, int ArgCount) const
{
    // the stack contains points (pairs of x/y) which define a line
    // return value is y=f(x)
    // Note: *Stack points to the last argument (y-coord of last point)
    if (ArgCount%2!=1)
        throw std::logic_error("Expression::polygon: wrong number of arguments. polygon(<val>, x0, y0, x1, y1, ....)");
    int PointCnt = (ArgCount-1) / 2;
    if (PointCnt<2)
        throw std::logic_error("Expression::polygon: wrong number of arguments. polygon(<val>, x0, y0, x1, y1, ....)");
    double x,y, xold, yold;
    y=*Stack--;   // 1. argument
    x=*Stack--;
    if (Value>x)
        return y;
    for (int i=0; i<PointCnt-1; i++) {
        xold=x;
        yold=y;
        y=*Stack--;   // x,y-pair from stack
        x=*Stack--;
        if (Value>x) {
            return (yold-y)/(xold-x) * (Value-x) + y;
        }

    }
    // x smaller than the first point
    return y;
}

// userdefined func sigmoid....
double Expression::udfSigmoid(double Value, double sType, double p1, double p2) const
{
    // sType: typ of function:
    // 0: logistic f
    // 1: Hill-function
    // 2: 1 - logistic (1 to 0)
    // 3: 1- hill
    double Result;

    double x=std::max(std::min(Value, 1.), 0.);  // limit to [0..1]
    int typ=(int) sType;
    switch (typ) {
         case 0: case 2: // logistic function: f(x)=1 / (1 + p1 e^(-p2 * x))
                     Result=1. / (1. + p1 * exp(-p2 * x));
             break;
         case 1: case 3:     // Hill-function: f(x)=(x^p1)/(p2^p1+x^p1)
                     Result=pow(x, p1) / ( pow(p2,p1) + pow(x,p1));
             break;
         default:
             throw std::logic_error("sigmoid-function: invalid type of function: 0..3");
         }
    if (typ==2 || typ==3)
        Result=1. - Result;

    return Result;
}

double Expression::udfIn(double Value, double *Stack, int ArgCount) const
{
    // signature: in(x, v1, v2, v3, ..., vn)
    if (ArgCount<2)
        throw std::logic_error("Expression: in() function: not enough parameters");
    double *p = Stack - (ArgCount-2); // point at the first value (v1)
    while (p <= Stack) {
        if (*p == Value)
            return static_cast<double>(True);
        ++p;
    }
    return static_cast<double>(False);
}


// SVD specific neighborhood functions
double Expression::udfNeighborhood(ExpressionWrapper *object, int neighbor_class, double *Stack, int ArgCount) const
{
    // signature: f(
    if (!object) return 0.;
    CellWrapper *wrap = dynamic_cast<CellWrapper*>(object);
    if (!wrap) return 0.;
    double *p = Stack - (ArgCount-1);
    double result = 0.;
    while (p <= Stack) {
        size_t stateId = static_cast<size_t>( *p );
        switch (neighbor_class) {
        case 1: result += wrap->localStateAverage(stateId); break;
        case 2: result += wrap->intermediateStateAverage(stateId); break;
        case 3: result +=  wrap->globalStateAverage(stateId); break;
        default: break;
        }
        ++p;
    }
    return result;
}


void Expression::checkBuffer(int Index)
{
    // manage the buffer: increase size if necessary
    if (Index<m_execListSize)
        return;
    int NewSize=m_execListSize * 2; // double size every time: 5->10->20->40->80->160
    // (1) create new buffer
    ExtExecListItem *NewBuf=new ExtExecListItem[NewSize];
    // (2) copy values
    for (int i=0;i<m_execListSize;i++)
        NewBuf[i]=m_execList[i];
    // (3) use new buffer
    delete[] m_execList;
    m_execList = NewBuf;
    m_execListSize=NewSize;
}


double Expression::udfRandom(int type, double p1, double p2) const
{
    // random
    if (type == 0)
        return nrandom(p1, p2);
    else    // gaussian
        //return mtRand().randNorm(p1, p2);
        throw std::logic_error("udfRandom: gaussian random number not implemented");
}

/** Linarize an expression, i.e. approximate the function by linear interpolation.
    This is an option for performance critical calculations that include time consuming mathematic functions (e.g. exp())
    low_value: linearization start at this value. values below produce an error
    high_value: upper limit
    steps: number of steps the function is split into
  */
void Expression::linearize(const double low_value, const double high_value, const int steps)
{
    if (!mLinearizationAllowed)
        return;

    mLinearized.clear();
    mLinearLow = low_value;
    mLinearHigh  = high_value;
    mLinearStep = (high_value - low_value) / (double(steps));
    for (int i=0;i<=steps;i++) {
        double x = mLinearLow + i*mLinearStep;
        double r = calculate(x);
        mLinearized.push_back(r);
    }
    mLinearizeMode = 1;
}

/// like 'linearize()' but for 2d-matrices
void Expression::linearize2d(const double low_x, const double high_x,
                             const double low_y, const double high_y,
                             const int stepsx, const int stepsy)
{
    if (!mLinearizationAllowed)
        return;
    mLinearized.clear();
    mLinearLow = low_x;
    mLinearHigh  = high_x;
    mLinearLowY = low_y;
    mLinearHighY = high_y;

    mLinearStep = (high_x - low_x) / (double(stepsx));
    mLinearStepY = (high_y - low_y) / (double(stepsy));
    for (int i=0;i<=stepsx;i++) {
        for (int j=0;j<=stepsy;j++) {
            double x = mLinearLow + i*mLinearStep;
            double y = mLinearLowY + j*mLinearStepY;
            double r = calculate(x,y);
            mLinearized.push_back(r);
        }
    }
    mLinearStepCountY = stepsy + 1;
    mLinearizeMode = 2;

}


/// calculate the linear approximation of the result value
double Expression::linearizedValue(const double x) const
{
    if (x<mLinearLow || x>=mLinearHigh)
        return calculate(x,0.,true); // standard calculation without linear optimization- but force calculation to avoid infinite loop
    size_t lower = int((x-mLinearLow) / mLinearStep); // the lower point
    if (lower+1>=mLinearized.size())
      assert(lower+1<mLinearized.size());
    const std::vector<double> &data = mLinearized;
    // linear interpolation
    double result = data[lower] + (data[lower+1]-data[lower])/mLinearStep*(x-(mLinearLow+lower*mLinearStep));
    return result;
}

/// calculate the linear approximation of the result value
double Expression::linearizedValue2d(const double x, const double y) const
{
    if (x<mLinearLow || x>=mLinearHigh || y<mLinearLowY || y>=mLinearHighY)
        return calculate(x,y,true); // standard calculation without linear optimization- but force calculation to avoid infinite loop
    int lowerx = int((x-mLinearLow) / mLinearStep); // the lower point (x-axis)
    int lowery = int((y-mLinearLowY) / mLinearStepY); // the lower point (y-axis)
    size_t idx = mLinearStepCountY*lowerx + lowery;
    assert(idx + mLinearStepCountY+1 <mLinearized.size());
    const std::vector<double> &data = mLinearized;
    // linear interpolation
    // mean slope in x - direction
    double slope_x = ( (data[idx+mLinearStepCountY]-data[idx])/mLinearStepY + (data[idx+mLinearStepCountY+1]-data[idx+1])/mLinearStepY ) / 2.;
    double slope_y = ( (data[idx+1]-data[idx])/mLinearStep + (data[idx+mLinearStepCountY+1]-data[idx+mLinearStepCountY])/mLinearStep ) / 2.;
    double result = data[idx] + (x-(mLinearLow+lowerx*mLinearStep))*slope_x + (y-(mLinearLowY+lowery*mLinearStepY))*slope_y;
    return result;
}
