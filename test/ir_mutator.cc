#include <string>
#include <iostream>
#include <map>


#include "IR.h"
#include "IRMutator.h"
#include "IRVisitor.h"
#include "IRPrinter.h"
#include "type.h"
#include "json2IRTree.h"

using namespace Boost::Internal;


json_To_IRTree j2i;
int vectorS=0;

class MyMutator : public IRMutator {
 public:
  std::map<std::string, Expr> gradForVar;
  Expr leftVar;
  std::set<std::string> ins;
  std::vector<Expr> gradsVar;
  int moveNum=0;

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



  Expr visit(Ref<const Binary> op) override{
    std::cout<<"mutatot op in"<<"\n";
    Expr nowGrad = op->grad;
    Expr new_a = op->a, new_b = op->b;
    if (op->op_type == BinaryOpType::Add){
      std::shared_ptr<const Binary> b = op->a.as<Binary>();
      std::shared_ptr<const Var> v = op->a.as<Var>();
      Expr newA = op->a, newB = op->b;
      if (b != nullptr)
        newA = Binary::make(b->type(), b->op_type, b->a, b->b, nowGrad);
      if (v != nullptr)
        newA = Var::make(v->type(), v->name, v->args, v->shape, nowGrad);
      new_a = mutate(newA);

      b = op->b.as<const Binary>();
      v = op->b.as<const Var>();
      if (b != nullptr) 
        newB = Binary::make(b->type(), b->op_type, b->a, b->b, nowGrad);
      if (v != nullptr) 
        newB = Var::make(v->type(), v->name, v->args, v->shape, nowGrad);
      new_b = mutate(newB);
    }
    else if (op->op_type == BinaryOpType::Mul){
      std::shared_ptr<const Binary> b = op->a.as<Binary>();
      std::shared_ptr<const Var> v = op->a.as<Var>();
      Expr gradA = Binary::make(op->type(), BinaryOpType::Mul, nowGrad, op->b);
      Expr newA = op->a;
      if (b != nullptr) 
        newA = Binary::make(b->type(), b->op_type, b->a, b->b, gradA);
      if (v != nullptr) 
        newA = Var::make(v->type(), v->name, v->args, v->shape, gradA);
      new_a = mutate(newA);
      
      b = op->b.as<const Binary>();
      v = op->b.as<const Var>();
      Expr gradB = Binary::make(op->type(), BinaryOpType::Mul,  op->a,nowGrad);
      Expr newB = op->b;
      if (b != nullptr) 
        newB = Binary::make(b->type(), b->op_type, b->a, b->b, gradB);
      if (v != nullptr) 
        newB = Var::make(v->type(), v->name, v->args, v->shape, gradB);
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
    std::shared_ptr<const Binary> b = op->src.as<Binary>();
    std::shared_ptr<const Var> v = op->src.as<Var>();
    Expr newSrc = op->src;
    if (b != nullptr) 
      newSrc = Binary::make(b->type(), b->op_type, b->a, b->b, leftVar);
    if (v != nullptr) 
      newSrc = Var::make(v->type(), v->name, v->args, v->shape, leftVar);
    mutate(newSrc);
    std::cout<<"move num"<<moveNum<<"\n";
    std::shared_ptr<const Var> _v = gradsVar[moveNum].as<Var>();
    std::cout<<"name is "<<_v->name<<" "<<moveNum<<" "<<gradsVar.size()<<"\n";
    Expr new_src = op->src;
    Expr new_dst = Var::make(_v->type(), "d"+_v->name, _v->args, _v->shape);
    if (gradForVar.find(_v->name) != gradForVar.end())
      new_src = gradForVar.find(_v->name)->second;
      IRVisitor visitor;
      new_src.visit_expr(&visitor);
      ins.insert(visitor._ins.begin(), visitor._ins.end());
      std::cout<<"mutatot op out"<<"\n";
    moveNum+=1;
      return Move::make(new_dst, new_src, op->move_type);

  }
  
};


int main() {

/*    const int M = 4;
    const int N = 6;

    Type index_type = Type::int_scalar(32);
    Type data_type = Type::float_scalar(32);

    // index i
    Expr dom_i = Dom::make(index_type, 0, M);
    Expr i = Index::make(index_type, "i", dom_i, IndexType::INT);

    // index j
    Expr dom_j = Dom::make(index_type, 0, N);
    Expr j = Index::make(index_type, "j", dom_j, IndexType::INT);
    
    //B<4, 6>[i, j] = A<4>[i];"
    // A
    Expr expr_A = Var::make(data_type, "A", {i}, {M});

    // B
    Expr expr_B = Var::make(data_type, "B", {i, j}, {M, N});

    // main stmt
    Stmt main_stmt = Move::make(
        expr_A,
        expr_A,
        MoveType::MemToMem
    );

    // loop nest
    Stmt loop_nest = LoopNest::make({i, j}, {main_stmt});

    // kernel
    Group kernel = Kernel::make("simple_gemm", {expr_A}, {expr_B}, {loop_nest}, KernelType::CPU);
*/


    Group kernel = j2i.go("case9");

    //std::cout << "Success!\n";
    //for (auto s : j2i.grads)
    //  std::cout<<s<<"\n";
    //std::cout << "Success!\n";


    
    IRPrinter printer;
    //std::string code = printer.print(kernel);
  //std::cout << code;
    // mutator
    vectorS = j2i.grads.size();
    MyMutator mutator;
    kernel = mutator.mutate(kernel);
  //for (auto s:mutator.ins) std::cout<<"!@#!# "<<s<<"\n";
    // printer
   std::string code = printer.print(kernel);

    std::cout << code;

    std::cout << "Success!\n";

    return 0;
}
