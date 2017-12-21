#ifndef expressionH
#define expressionH

#include <string>
#include <vector>

#define MAXLOCALVAR 15
class ExpressionWrapper;
class Expression
{
public:
        ~Expression();
        Expression();
        Expression(const std::string &aExpression) { setExpression(aExpression); }
        Expression(const std::string &expression, ExpressionWrapper *wrapper) { setExpression(expression); mModelObject = wrapper;  }
        // intialization
        void setExpression(const std::string &aExpression); ///< set expression
        void setAndParse(const std::string &expr); ///< set expression and parse instantly
        void setModelObject(ExpressionWrapper *wrapper) { mModelObject = wrapper; }
        const std::string &expression() const { return m_expression; }
        void  parse(ExpressionWrapper *wrapper=0); ///< force a parsing of the expression
        void linearize(const double low_value, const double high_value, const int steps=1000);
        void linearize2d(const double low_x, const double high_x, const double low_y, const double high_y, const int stepsx=50, const int stepsy=50);
        static void setLinearizationEnabled(const bool enable) {mLinearizationAllowed = enable; }
        /// access from external scripting (e.g. Picus Script engine)
        /// @param get_index a function with the signature int func(const std::string &var_name) -> returns the index of the variable
        /// @param get_value a function with the signature double func(int var_index) --> to retrieve the current value of the associated function
        void setScriptingFunctions( int (*get_index)(const std::string &), double (*get_value)(int)) { mScriptIndexFunc=get_index; mScriptValueFunc = get_value; }
        // calculations
        double execute(double *varlist=0, ExpressionWrapper *object=0, bool *rLogicResult=0) const; ///< calculate formula and return result. variable values need to be set using "setVar()"
        /** calculate formula. the first two variables are assigned the values Val1 and Val2. This function is for convenience.
           the return is the result of the calculation.
           e.g.: x+3*y --> Val1->x, Val2->y
           forceExecution: do not apply linearization */
        double calculate(const double Val1=0., const double Val2=0., const bool forceExecution=false) const;
        /// calculate formula with object
        ///
        double calculate(ExpressionWrapper &object, const double variable_value1=0., const double variable_value2=0.) const;

        //variables
        /// set the value of the variable named "Var". Note: using addVar to obtain a pointer may be more efficient for multiple executions.
        void  setVar(const std::string& Var, double Value);
        /// adds variable "VarName" and returns a double pointer to the variable. Use *ptr to set the value (before calling execute())
        double *addVar(const std::string& VarName);
        /// retrieve again the value pointer of a variable.
        double *  getVarAdress(const std::string& VarName);


        bool isConstExpression() const { return m_constExpression; } ///< returns true if current expression is a constant.
        bool isEmpty() const { return m_empty; } ///< returns true if expression is empty
        const std::string &lastError() const { return m_errorMsg; }
        /** strict property: if true, variables must be named before execution.
          When strict=true, all variables in the expression must be added by setVar or addVar.
          if false, variable values are assigned depending on occurence. strict is false by default for calls to "calculate()".
        */
        bool isStrict() { return m_strict;}
        void setStrict(bool str) { m_strict=str; }
        void setCatchExceptions(bool docatch=true) { m_catchExceptions = docatch; }
        void   setExternalVarSpace(const std::vector<std::string>& ExternSpaceNames, double* ExternSpace);
        void enableIncSum();
        double udfRandom(int type, double p1, double p2) const; ///< user defined function rnd() (normal distribution does not work now!)
private:
        enum ETokType {etNumber, etOperator, etVariable, etFunction, etLogical, etCompare, etStop, etUnknown, etDelimeter};
        enum EValueClasses {evcBHD, evcHoehe, evcAlter};
        struct ExtExecListItem {
            ETokType Type;
            double  Value;
            int     Index;
        };
        enum EDatatype {edtInfo, edtNumber, edtString, edtObject, edtVoid, edtObjVar, edtReference, edtObjectReference};
        bool m_catchExceptions;
        std::string m_errorMsg;

        bool m_parsed;
        mutable bool m_strict;
        bool m_empty; // empty expression
        bool m_constExpression;
        std::string m_tokString;
        std::string m_expression;
        Expression::ExtExecListItem *m_execList;
        int m_execListSize; // size of buffer
        int m_execIndex;
        double m_varSpace[MAXLOCALVAR];
        std::vector<std::string> m_varList;
        std::vector<std::string> m_externVarNames;
        double *m_externVarSpace;
        Expression::ETokType m_state;
        Expression::ETokType m_lastState;
        char *m_pos;
        char *m_expr;
        std::string m_token;
        std::string m_prepStr;
        int   m_tokCount;
        Expression::ETokType  next_token();
        void  atom();
        void  parse_levelL0();
        void  parse_levelL1();
        void  parse_level0();
        void  parse_level1();
        void  parse_level2();
        void  parse_level3();
        void  parse_level4();
        int  getFuncIndex(const std::string& functionName);
        int  getVarIndex(const std::string& variableName);
        inline double getModelVar(const int varIdx, ExpressionWrapper *object=0) const ;

        // link to external model variable
        ExpressionWrapper *mModelObject;

        double getExternVar(const int Index) const;
        // inc-sum
        mutable double m_incSumVar;
        bool   m_incSumEnabled;
        double  udfPolygon(double Value, double* Stack, int ArgCount) const; ///< special function polygon()
        double udfSigmoid(double Value, double sType, double p1, double p2) const; ///< special function sigmoid()
        void checkBuffer(int Index);

        // linearization
        inline double linearizedValue(const double x) const;
        inline double linearizedValue2d(const double x, const double y) const;
        int mLinearizeMode;
        std::vector<double> mLinearized;
        double mLinearLow, mLinearHigh;
        double mLinearStep;
        double mLinearLowY, mLinearHighY;
        double mLinearStepY;
        int mLinearStepCountY;
        static bool mLinearizationAllowed;
        // access to scripting...
        int (*mScriptIndexFunc)(const std::string &);
        double (*mScriptValueFunc)(int);
};

#endif   // EXPRESSIONH
