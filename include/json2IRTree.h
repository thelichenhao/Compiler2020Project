#include<iostream>
#include<cstdio>
#include<cstring>
#include<map>
#include<vector>
#include<stack>
#include<set>
#include<algorithm>
#include<sstream>
#include<fstream>

#include "IR.h"
#include "IRMutator.h"
#include "IRVisitor.h"
#include "IRPrinter.h"
#include "type.h"

using namespace Boost::Internal;

struct tempStruct{
    std::string ele;
    std::string type;
    tempStruct(std::string _e, std::string _t) : ele(_e), type(_t) {}
};

class IndexParse{
  public:
    std::vector<std::vector<tempStruct> > eleList;
    IndexParse() {eleList.clear();}

    void insertPart(std::string s, std::set<std::string>& tmpVar);
    void insertOp(std::string s);
};
class indexVar{
	public:
		unsigned long bound = 1<<29;
		void setBound(unsigned long _bound);
		Expr getExp(std::string s);
};

class json_To_IRTree {
	public:
		std::vector<Expr> _leftAlist;
		std::string s, name, ins, outs, datatype, kernel, grad;
		Type data_type;
		std::vector<Expr> inputs, outputs;
		std::string kernelname;
		std::stack<Expr> num;
		std::stack<char> op;
		std::stack<std::set<std::string>> loopVar;
		std::stack<Expr> ifThen;
		int pri[300];
		int brackNum = 0;
		std::map<std::string, indexVar> s2indexVar; //循环变量名到indexVar
		std::map<std::string, std::vector<unsigned long> > s2Var; //变量名字到范围
		std::string nowVar;
        std::vector<unsigned long> leftShape;
		std::set<std::string> nameInputs;
		std::vector<std::string> grads;
		
		json_To_IRTree(){
			pri['(']=0;pri['+']=pri['-']=1;pri['*']=pri['/']=pri['%']=2;
		}

		Group go(std::string filename);
		void printAll();
		void deleteSpace(std::string &s);
		void parse_type();
		void parse_ins();
		void parse_outs();
		void parse_name();
		void parse_grad();
		void Clistin(int &i, std::vector<unsigned long>& Clist);
		std::vector<Expr> Alistin(int &i, std::vector<unsigned long>& Clist, bool tag=false);
		IndexParse parseIndexExpr(std::string a, std::set<std::string>& tmpVar);
		Expr getLongExpr_part(indexVar indexvar, std::vector<tempStruct> s);
		Expr getLongExpr(indexVar indexvar, std::vector< std::vector<tempStruct> > s);
		void calcul();
		Expr makeVar(std::string name, std::vector<Expr> &Alist, std::vector<unsigned long> &Clist);
		std::vector<Expr> makeLoopVar(std::set<std::string> &v);
		void makeIfThen(std::vector<Expr> &exp, std::vector<unsigned long> &Clist);
		Group parse_kernel();
		void doOneTime(std::vector<Stmt> &allStmt);
};

