// this is a silly solution
// just to show you how different
// components of this framework work
// please bring your wise to write
// a 'real' solution :)

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include "IR.h"
#include "IRMutator.h"
#include "IRVisitor.h"
#include "IRPrinter.h"
#include "type.h"
#include "json2IRTree.h"

json_To_IRTree j2i;
int vectorS=0;

class MyMutator : public IRMutator {
 public:
  std::map<std::string, Expr> gradForVar;
  Expr leftVar;
  std::set<std::string> ins;
  std::vector<Expr> gradsVar;
  int moveNum=0;
  bool moveRight=false;

  Expr visit(Ref<const Var> op) override {
    std::cout<<"mutatot var in"<<"\n";
    if (gradForVar.find(op->name) == gradForVar.end())
      gradForVar[op->name] = op->grad; else
      gradForVar[op->name] = Binary::make(op->type(), BinaryOpType::Add, gradForVar[op->name], op->grad);
    std::cout<<"NNNNNN "<<op->name<<"\n";
    for (int i=0; i<j2i.grads.size(); ++i) {
      std::cout<<"NNNNNN "<<j2i.grads[i]<<"\n";
      bool isIn = false;
      for (Expr e : gradsVar){
        std::shared_ptr<const Var> _v = e.as<Var>();
        if (_v->name == op->name) isIn=true;
      }
      if (op->name == j2i.grads[i] && isIn == false) {
        gradsVar.push_back(*new Expr(op));
      }
    }
    std::cout<<"mutatot var out"<<"\n";
    return op;
  }



  Group visit(Ref<const Kernel> op) override {
    std::cout<<"mutatot kernel in"<<"\n";
    Expr lExpr = op->outputs[0];
    std::shared_ptr<const Var> lVar = lExpr.as<Var>();
    leftVar = Var::make(lVar->type(), "d"+lVar->name, j2i._leftAlist, lVar->shape);

    std::vector<Stmt> new_stmt_list;
    for (auto stmt : op->stmt_list) {
        new_stmt_list.push_back(mutate(stmt));
    }
    //need renew inputs
    for (auto s:ins)
      std::cout<<s<<"\n";
    std::cout<<"mutatot kernel end"<<"\n";
    // ins -> j2i.s2var    inputs
    std::vector<Expr> inputs;
    for (std::string s : ins){
        std::string aim;
        if (s[0]=='d')
          for (int i=1;i<s.length();++i) aim+=s[i];
        else aim=s;
        //std::map<std::string, std::vector<unsigned long> > s2Var; //变量名字到范围
        //std::vector<unsigned long> vv = j2i.s2Var[aim];
        std::vector<Expr> noUse; noUse.clear();
        inputs.push_back(Var::make(Type::float_scalar(32), s, noUse, j2i.s2Var[aim]));
    }
    return Kernel::make(op->name, inputs, gradsVar, new_stmt_list, op->kernel_type);
  }


  Expr change(const Expr o, const Expr nowGrad){
    std::shared_ptr<const Binary> b = o.as<Binary>();
    std::shared_ptr<const Var> v = o.as<Var>();
    Expr _ret = o;
    if (b != nullptr)
      _ret = Binary::make(b->type(), b->op_type, b->a, b->b, nowGrad);
    if (v != nullptr)
      _ret = Var::make(v->type(), v->name, v->args, v->shape, nowGrad);
    return _ret;
  }

  Expr visit(Ref<const Binary> op) override{
    if (moveRight == false) {
      Expr new_a = mutate(op->a);
      Expr new_b = mutate(op->b);
      return Binary::make(op->type(), op->op_type, new_a, new_b);
    }
    std::cout<<"mutatot op in"<<"\n";
    Expr nowGrad = op->grad;
    Expr new_a = op->a, new_b = op->b;
    if (op->op_type == BinaryOpType::Add){
      
      Expr newA = change(op->a, nowGrad);
      new_a = mutate(newA);

      Expr newB = change(op->b, nowGrad);
      new_b = mutate(newB);
    }
    else if (op->op_type == BinaryOpType::Mul){
      Expr gradA = Binary::make(op->type(), BinaryOpType::Mul, nowGrad, op->b);
      Expr newA = change(op->a, gradA);
      new_a = mutate(newA);
      
      Expr gradB = Binary::make(op->type(), BinaryOpType::Mul,  op->a,nowGrad);
      Expr newB = change(op->b, gradB);
      new_b = mutate(newB);
    } 
    else if (op->op_type == BinaryOpType::Div) {
      
      Expr gradA = Binary::make(op->type(), BinaryOpType::Div, nowGrad, op->b);
      Expr newA = change(op->a, gradA);
      new_a = mutate(newA);
      
      Expr minusA = Unary::make(op->a->type(), UnaryOpType::Neg, op->a);
      Expr bMulb = Binary::make(op->b->type(), BinaryOpType::Mul, op->b, op->b);
      Expr gradB = Binary::make(op->type(), BinaryOpType::Mul, nowGrad, 
                               Binary::make(op->type(), BinaryOpType::Div, minusA, bMulb));
      Expr newB = change(op->b, gradB);
      new_b = mutate(newB);
    }
    else if (op->op_type == BinaryOpType::Sub){
      Expr newA = change(op->a, nowGrad);
      new_a = mutate(newA);

      Expr newB = change(op->b, Unary::make(nowGrad->type(), UnaryOpType::Neg, nowGrad));
      new_b = mutate(newB);
    }
    std::cout<<"mutatot op out"<<"\n";
    return Binary::make(op->type(), op->op_type, new_a, new_b);
  }



  Stmt visit(Ref<const Move> op) override{
    //dst not change
    //src get from gradForVar
    
    std::cout<<"mutatot op in"<<"\n";
    std::cout<<gradsVar.size()<<"\n";
    gradForVar.clear();
    Expr newSrc = change(op->src, leftVar);
    moveRight=true;
    mutate(newSrc);
    moveRight=false;
    std::cout<<"move num"<<moveNum<<"\n";
    std::shared_ptr<const Var> _v = gradsVar[moveNum].as<Var>();
    std::cout<<"name is "<<_v->name<<" "<<moveNum<<" "<<gradsVar.size()<<"\n";
    Expr new_src = op->src;
    Expr new_dst = Var::make(_v->type(), "d"+_v->name, _v->args, _v->shape);
    if (gradForVar.find(_v->name) != gradForVar.end()){
	std::cout<<"563"<<"\n";
      new_src = gradForVar.find(_v->name)->second;
    }
    std::cout<<"123"<<"\n";
      IRVisitor visitor;
      new_src.visit_expr(&visitor);
    std::cout<<"234"<<"\n";
      ins.insert(visitor._ins.begin(), visitor._ins.end());
      std::cout<<"mutatot op out"<<"\n";
    moveNum+=1;
      return Move::make(new_dst, new_src, op->move_type);

  }
};

int main() {
    /* add your solutions */
    for(int i=1; i<=10; i++) {
        std::string s="case"+std::to_string(i);
        
        vectorS=0;
        Group kernel = j2i.go(s);
        IRPrinter printer;
        std::string code1 = printer.print(kernel);
        std::cout << code1;
        
        vectorS = j2i.grads.size();
        MyMutator mutator;
        kernel = mutator.mutate(kernel);
        IRPrinter printer2;
        std::string code = printer2.print(kernel);
        //std::cout<<"#include \"../run2.h\"\n";
        std::cout << code;
        //std::cout << "Success!\n";

        std::string outpath = "./kernels/grad_"+s+".cc";
        std::ofstream ofile(outpath);
        ofile<<"#include \"../run2.h\"\n";
        ofile<<code;
        ofile.close();
         
    }
    return 0;
}
