#include "json2IRTree.h"

using namespace Boost::Internal;

void IndexParse::insertPart(std::string s, std::set<std::string>& tmpVar) {
	// s: i*4/2, i, 1
	std::vector<tempStruct> r;
	int t=0;
	std::string v;
	v.clear();
	while(s[t] != '*' && s[t] != '/' && s[t] !='%' && s[t]!='\0') v+=s[t++];

	if (s[t]=='\0') {
		// s: 1, i
		std::stringstream sin(v);
		int tmp;
		if (!(sin>>tmp)) {
			r.push_back( tempStruct(v, "id") );
			tmpVar.insert(v);
		}
		else r.push_back( tempStruct(v, "num") );
		eleList.push_back(r);
		return;
	}
	// s: i*4/2
	r.push_back( tempStruct(v, "id") );
	tmpVar.insert(v);
	while(s[t]!='\0') {
		v.clear();
		v=s[t++];
		r.push_back( tempStruct(v, "op") );
		v.clear();
		while(s[t] != '*' && s[t] != '/' && s[t] !='%' && s[t]!='\0') v+=s[t++];
		r.push_back( tempStruct(v, "num") );
	}
	eleList.push_back(r);
}

void IndexParse::insertOp(std::string s) {
	std::vector<tempStruct> r;
	r.push_back( tempStruct(s, "op") );
	eleList.push_back(r);
}


void indexVar::setBound(unsigned long _bound){
	if (_bound < bound) bound = _bound;
}

Expr indexVar::getExp(std::string s){
	return Index::make(Type::int_scalar(32), s, Dom::make(Type::int_scalar(32), 0, (int)bound), IndexType::INT);
}


Group json_To_IRTree::go(std::string filename){
	brackNum = 0;
	s2indexVar.clear();
	s2Var.clear();
	leftShape.clear();
	inputs.clear();
	outputs.clear();
	nameInputs.clear();
    grads.clear();

	//freopen("1.in","r",stdin);
	std::fstream fin;
	fin.open(("../project2/cases/"+filename+".json").c_str(),std::ios::in);
	
	if (!fin) {
		std::cout<<filename<<std::endl;
		std::cout<<"Open read file error!"<<std::endl;
	}
	

	getline(fin,s);
	getline(fin,name);
	getline(fin,ins);
	getline(fin,outs);
	getline(fin,datatype);
	getline(fin,kernel);
	getline(fin,grad);
	getline(fin,s);
	
	std::cout<<kernel<<std::endl;
	getline(fin,s);
	parse_type();
	parse_grad();
	Group ans = parse_kernel();
		std::cout << "parse kernel success!\n";
	//对ans进行visit
	//IRPrinter printer;
	//std::string code = printer.print(ans);
	//std::cout << code;
	//std::cout << "Success!\n";
	fin.close();
	return ans;
	
}
void json_To_IRTree::printAll(){
	std::cout<<"ooooooooooooooooooooooooooooooooo"<<std::endl;
	for (auto s : s2Var){
		std::cout<<"var "<<s.first<<std::endl;
		for (auto i : s.second) std::cout<<"shape"<<i<<" ";std::cout<<std::endl;
	}
	for (auto s : s2indexVar){
		std::cout<<"index var"<<s.first<<std::endl;
		std::cout<<"bound "<<s.second.bound<<std::endl;
	}
}
void json_To_IRTree::deleteSpace(std::string &s){
	std::string ss;
	for (int i=0; i<s.size(); ++i)
		if (s[i] != ' ') ss += s[i];
	s=ss;
}

void json_To_IRTree::parse_type(){
	std::cout<<"parse_type "<<datatype<<" "<<datatype.find("float")<<std::endl;

	if (datatype.find("float") == -1) {
		data_type = Type::int_scalar(32);
		std::cout<<"    data_type : int"<<std::endl;
	}
	else {
		data_type = Type::float_scalar(32);
		std::cout<<"    data_type : float"<<std::endl;
	}
} 

void json_To_IRTree::parse_ins(){//解析到inputs里    "ins": ["B", "C"],
	deleteSpace(ins);
	std::cout<<"input parse"<<std::endl;
	std::vector<Expr> noUse; noUse.clear();
	for (int i=ins.find("[")+1; i<ins.find("]"); ++i){
		i+=1;
		std::string tmp="";
		while (ins[i] != '"') tmp += ins[i++];
		i+=1;
		inputs.push_back(makeVar(tmp, noUse, s2Var[tmp]));
		nameInputs.insert(tmp);
		for (int j : s2Var[tmp]) std::cout<<j<<" ";std::cout<<std::endl;
		std::cout<<"  "<<tmp<<std::endl;
	} 
}

void json_To_IRTree::parse_grad(){//解析到inputs里    "ins": ["B", "C"],
	deleteSpace(grad);
	std::cout<<"grad parse  "<<grad<<std::endl;
	for (int i=grad.find("[")+1; i<grad.find("]"); ++i){
		i+=1;
		std::string tmp="";
		while (grad[i] != '"') tmp += grad[i++];
		i+=1;
		grads.push_back(tmp);
		std::cout<<"  "<<tmp<<std::endl;
	} 
}


void json_To_IRTree::parse_outs(){//解析到outpus里  "outs": ["A"] 
	deleteSpace(outs);
	std::cout<<"output parse"<<std::endl;
	std::vector<Expr> noUse; noUse.clear();
	for (int i=outs.find("[")+1; i<outs.find("]"); ++i){
		i+=1;
		std::string tmp="";
		while (outs[i] != '"') tmp += outs[i++];
		i+=1;
		if (nameInputs.find(tmp) == nameInputs.end()){
			outputs.push_back(makeVar(tmp, noUse, s2Var[tmp]));
			for (int j : s2Var[tmp]) std::cout<<j<<" ";std::cout<<std::endl;
			std::cout<<"  "<<tmp<<std::endl;
		}
	}
}

void json_To_IRTree::parse_name(){//解析到namekernel里 
	deleteSpace(name);
	std::cout<<"parse name"<<std::endl;
	kernelname = "";
	for (int i=name.find(":")+2; i<name.find(",")-1; ++i) kernelname += name[i];
	std::cout<<"     "<<kernelname<<std::endl;
}

void json_To_IRTree::Clistin(int &i, std::vector<unsigned long>& Clist){ //解析<16,12>这种 放入Clist中
	std::string s;
	Clist.clear();
	i+=1;
	while (kernel[i] != '>'){
		s = "";
		while (kernel[i] != ',' && kernel[i] != '>') s += kernel[i++];
		if (kernel[i] == ',') i+=1;
		Clist.push_back(std::atoi(s.c_str()));
		std::cout<<"Clsitin "<<std::atoi(s.c_str())<<std::endl;
	}
	if (s2Var.find(nowVar) == s2Var.end()){
		s2Var.insert(std::pair<std::string, std::vector<unsigned long> >(nowVar, Clist));
		std::cout<<"-------------var in : "<<nowVar<<std::endl;
	} //会不会出现不一致? a<12,10> 后面又来了个 a<10,11> 
}

std::vector<Expr> json_To_IRTree::Alistin(int &i, std::vector<unsigned long>& Clist, bool tag){ //解析[i,j] 和前面的Clist对比 缩小i,j范围
	std::vector<Expr> _ret;
	std::string s,loops;
	std::set<std::string> tmpVar;

	std::string indexExpr;
	int k=0;//第几个 和Clist对应
	i+=1;
	while (kernel[i] != ']'){
		s = "";//整体的 i+2 就是i+2
		loops = "";//循环变量 i+2
		bool f=true;//是不是只出现一个符号 todo:出现i+2,i+i+2怎么处理 
		while (kernel[i] != ',' && kernel[i] != ']') {
			if (kernel[i] == '-' || kernel[i] == '+' || kernel[i] == '(' || kernel[i] == ')' || kernel[i] == '*' || kernel[i] == '/' || kernel[i] == '%'){
				f=false;

				if (kernel[i]=='/') i++; // IdExpr中的除法是整除，读取掉一个'/'便于处理

				while(kernel[i] != ',' && kernel[i] != ']')  loops += kernel[i++];
				
				if (s2indexVar.find(loops) == s2indexVar.end()){
					s2indexVar.insert(std::pair<std::string, indexVar>(loops, *new indexVar()));
					std::cout<<"..........s2indexVar insert "<<loops<<std::endl;
				}
				indexExpr=loops;
				loops = "";
			}
			//s += kernel[i];
			if(kernel[i] != ',' && kernel[i] != ']') loops += kernel[i++];
		}
		if (s2indexVar.find(loops) == s2indexVar.end()){
			s2indexVar.insert(std::pair<std::string, indexVar>(loops, *new indexVar()));
			std::cout<<"..........s2indexVar insert "<<loops<<std::endl;
		}
		if (kernel[i] == ',') i+=1;
		/*todo:如果出现的都是 i+2 j+1之类的 那就很难确定范围
		 而且返回值要用binary组合过 不能直接返回一个 loopVar对应的exp */

		if (f){
			s2indexVar[loops].setBound(Clist[k]); 
			std::cout<<"bound change : "<<loops<<" "<<Clist[k]<<std::endl;
			if (tag) {
				_ret.push_back(s2indexVar[loops].getExp(loops)); //只有在第二次遍历有用 第一次exp为空
				tmpVar.insert(loops);
				std::cout<<"loop var insert "<< loops<<std::endl;
			}
			std::cout<<"Alsitin "<<loops<<std::endl;
		}
		else {
			// indexExpr: i+1+j//4
			

			if (tag) {
				IndexParse indexParse = this->parseIndexExpr(indexExpr, tmpVar);
				indexVar indexvar = s2indexVar[indexExpr];
				_ret.push_back(getLongExpr(indexvar, indexParse.eleList));
			}
		}

		k+=1;
	}
	if (tag) loopVar.push(tmpVar);
	return _ret;
}

IndexParse json_To_IRTree::parseIndexExpr(std::string a, std::set<std::string>& tmpVar) {
	int t=0;
	IndexParse indexParse;
	while (t<a.length()) {
		std::string s;
		while(a[t]!='+' && a[t]!='-' && a[t]!='\0') s+=a[t++];
		indexParse.insertPart(s, tmpVar);
		s.clear();
		if (a[t]=='\0') break;
		s=a[t++];
		indexParse.insertOp(s);
		s.clear();
	}
	return indexParse;
}
Expr json_To_IRTree::getLongExpr_part(indexVar indexvar, std::vector<tempStruct> s) {
	Expr _ret = indexvar.getExp(s[0].ele);
	BinaryOpType op_type;
	for(int i=1; i<s.size(); i+=2) {
		switch((s[i].ele)[0]) {
			case '*':
				op_type = BinaryOpType::Mul;
				break;
			case '/':
				op_type = BinaryOpType::Div;
				break;
			case '%':
				op_type = BinaryOpType::Mod;
				break;
		}
		_ret = Binary::make(_ret->type(), op_type, _ret, indexvar.getExp(s[i+1].ele));
	}
	return _ret;
}
Expr json_To_IRTree::getLongExpr(indexVar indexvar, std::vector< std::vector<tempStruct> > s) {
	Expr _ret = this->getLongExpr_part(indexvar, s[0]);
	BinaryOpType op_type;
	for(int i=1; i<s.size(); i+=2) {
		switch((s[i][0].ele)[0]) {
			case '+':
				op_type = BinaryOpType::Add;
				break;
			case '-':
				op_type = BinaryOpType::Sub;
				break;
		}
		_ret = Binary::make(_ret->type(), op_type, _ret, this->getLongExpr_part(indexvar, s[i+1]) );
	}
	return _ret;
}

void json_To_IRTree::calcul(){ //计算栈顶的两个元素 
	std::set<std::string> s1,s2;
	Expr t1=num.top();num.pop();
	Expr t2=num.top();num.pop();
	s1=loopVar.top();loopVar.pop();
	s2=loopVar.top();loopVar.pop();
	bool f1=s1.empty(), f2=s2.empty();
	if (!f1 && !f2){
		Expr i1=ifThen.top();ifThen.pop();
		Expr i2=ifThen.top();ifThen.pop();
		ifThen.push(Binary::make(data_type, BinaryOpType::And, i1, i2));
	}
	std::cout<<"calcul ifthen push back once"<<f1<<" "<<f2<<std::endl;
	s1.insert(s2.begin(),s2.end());
	loopVar.push(s1);
	std::cout<<"calcul"<<std::endl;
	std::cout<<"new loop var"<<std::endl;
	for (auto i:s1) std::cout<<i<<" ";std::cout<<std::endl;
	std::cout<<"operation "<<op.top()<<std::endl;
	switch (op.top()){
		case '*':
			num.push(Binary::make(data_type, BinaryOpType::Mul, t2, t1));
			break;
		case '/':
			num.push(Binary::make(data_type, BinaryOpType::Div, t2, t1));
			break;
		case '+':
			num.push(Binary::make(data_type, BinaryOpType::Add, t2, t1));
			break;
		case '-':
			num.push(Binary::make(data_type, BinaryOpType::Sub, t2, t1));
			break;
		case '%':
			num.push(Binary::make(data_type, BinaryOpType::Mod, t2, t1));
			break;
	}
	op.pop();
}

Expr json_To_IRTree::makeVar(std::string name, std::vector<Expr> &Alist, std::vector<unsigned long> &Clist){
	return Var::make(data_type, name, Alist, Clist);
}

std::vector<Expr> json_To_IRTree::makeLoopVar(std::set<std::string> &v){
	std::vector<Expr> _ret;
	//for (std::string i : v) _ret.push_back(s2indexVar[i].exp);
	for (std::string i : v) _ret.push_back(s2indexVar[i].getExp(i));
	return _ret;
}

void json_To_IRTree::makeIfThen(std::vector<Expr> &exp, std::vector<unsigned long> &Clist) {
	std::cout<<"[[[[[[[[[[[[[[[[[[[[[\nmakeifthen\n";
	Expr tmp = Compare::make(data_type, CompareOpType::LT , exp[0], IntImm::make(Type::int_scalar(32), Clist[0]));
	std::cout<<Clist[0]<<" ";
	for (int i=1; i<exp.size(); ++i) {
		tmp = Binary::make(data_type, BinaryOpType::And, tmp, Compare::make(data_type, CompareOpType::LT , exp[i], IntImm::make(Type::int_scalar(32), Clist[i])));
		std::cout<<Clist[i]<<" ";
	}
	ifThen.push(tmp);
	std::cout<<"]]]]]]]]]]]]]]]]]]]]]\n";
}

Group json_To_IRTree::parse_kernel(){
	std::cout<<"parse kernel : "<<kernel<<std::endl;
	deleteSpace(kernel);
	std::string realkernel = kernel;
	std::cout<<"parse kernel : "<<kernel<<std::endl;
	/*A<16, 32>[i, j] = A<16, 32>[i, j] + B<16, 32>[i, k] * C<32, 32>[k, j];
	 *A = A + B * C  删去所有<>[]里的东西剩下的 
	 *<16,32>[i,j] <16,32>[i,j] <16,32>[i,k] <32,32>[k,j] 只保留<>[] 对应关系 
	 *i j k 只看[] 
	*/
	//假设现在只有一句话 只有一个;  todo:多个; 
	std::vector<Stmt> allStmt;
	int len = realkernel.size();
	std::cout<<"len is "<<len<<std::endl;
	for (int i=realkernel.find(":")+2; i<len-2; ++i){
		kernel = "";
		while (realkernel[i] != ';') kernel+=realkernel[i++];
		kernel+=";";
	    while(!num.empty()) num.pop();
	    while(!op.empty()) op.pop();
	    while(!loopVar.empty()) loopVar.pop();
	    while(!ifThen.empty()) ifThen.pop();
	    doOneTime(allStmt);
	}

	//Group kernel = Kernel::make("simple_gemm", {expr_A, expr_B}, {expr_C}, {loop_nest}, KernelType::CPU);
	parse_ins();
	parse_outs();
	parse_name();
	std::vector<Expr> noUse; noUse.clear();
	//outputs.push_back(makeVar("temp", noUse, leftShape)); // 多个语句左边不同?
	Group ans = Kernel::make(kernelname, inputs, outputs, allStmt, KernelType::CPU);
	return ans;
}

void json_To_IRTree::doOneTime(std::vector<Stmt> &allStmt){

	std::cout<<"kernel is : "<<kernel<<std::endl;
	int len = kernel.size();
	std::vector<unsigned long> Clist;
	/* 1.把循环变量全部分离出来,确定范围,放入map<string, indexVar>中
	     把参与运算的张量分离,确认每个张量的shape,放入map<string, vector<int> >中
	*/
	for (int i=kernel.find("=")+1; i<len; i++){
		if (kernel[i] == '=' || kernel[i] == '+' || kernel[i] == '-' || kernel[i] == '*' || kernel[i] == '/' || kernel[i] == '(' || kernel[i] == ')' || kernel[i] == '%') continue;
		if (kernel[i] >= '0' && kernel[i] <= '9') continue;
		if (kernel[i] == '<') Clistin(i, Clist); //范围加入Clist 然后放入 nowVar 对应的Var中  
		else if (kernel[i] == '[') Alistin(i, Clist);
		else if (kernel[i] == ';') break; //todo:多条语句不应该break 
		else {
			nowVar = "";
			while (kernel[i] != '<') nowVar += kernel[i++];
			std::cout<<"nowvar : "<<nowVar<<std::endl;
			i -= 1;
		}
	}

	std::cout<<"1 ok"<<std::endl;
	/* 2.生成循环变量的范围 也就是更新map<string, indexVar>中所有的indexVar 
		Expr dom_j = Dom::make(index_type, 0, N);
		Expr j = Index::make(index_type, "j", dom_j, IndexType::Spatial);*/ 


	std::cout<<"2 ok"<<std::endl;
	/* 3.设置3个栈来计算式子 一个符号栈 一个exp的栈 还有一个循环变量的栈 
	     遇到最外面的+或者-或者; 弹栈生成新的loop stmt  todo:这里要加上ifthenelse语句 保证范围的正确 
	     遇到优先级运算问题正常处理 
		 Expr expr_B = Var::make(data_type, "B", {k, j}, {K, N});
	*/

	/*等号左边要生成临时变量 
	  A<32,16>[i,j] =    ->    temp[i,j]   会不会本来就有temp? 
	  Var::make(data_type, "temp", {i , j}, { shape } ); 
	*/ 
	std::set<std::string> leftLoopVar;

	std::vector<Expr> leftAlist;
	std::vector<unsigned long> leftClist;
	std::string leftVar;
	for (int i=0; i<kernel.find("="); ++i) {
		nowVar = "";
		while (kernel[i] != '<') nowVar += kernel[i++];
		leftVar = nowVar;
		Clistin(i, leftClist); //仅仅是跳过<> 
		i+=1;
		if (kernel[i] == '[') {
			leftAlist = Alistin(i,leftClist,true);
			leftLoopVar = loopVar.top();
			loopVar.pop();
		} //标量不做处理,默认为空
	}
	_leftAlist.assign(leftAlist.begin(), leftAlist.end());
	std::cout<<"*****************************************left ok"<<std::endl;
	//清空temp变量为0
	/*Stmt clear_stmt;
	clear_stmt = Move::make(makeVar("temp", leftAlist, leftClist), IntImm::make(Type::int_scalar(32), 0), MoveType::MemToMem);
	std::vector<Expr> clear_index = makeLoopVar(leftLoopVar);
	if (!leftAlist.empty()) {
		makeIfThen(leftAlist, leftClist);
		Expr leftifThen = ifThen.top();
		ifThen.pop();
		clear_stmt = IfThenElse::make(leftifThen, clear_stmt, clear_stmt);
	}
	Stmt clear_loop_nest = LoopNest::make(clear_index, {clear_stmt});
	allStmt.push_back(clear_loop_nest);*/
	/*等号右边产生
	*/ 
	for (int i=kernel.find("=")+1; i<len; i++){
		if (kernel[i] == ';' || kernel[i] == '+' || kernel[i] == '-' || kernel[i] == '*' || kernel[i] == '/' || kernel[i] == '(' || kernel[i] == ')' || kernel[i] == '%'){
			if (kernel[i] == ')'){ //右括号弹到左括号为止 
				while (op.top() != '(') calcul();
				op.pop();
			}
			else if (kernel[i] == '(') {
				op.push('(');
			}
			else if (kernel[i] == ';'){ //最外面的+ - 号或者; 直接生成stmt
				while (num.size() > 1) calcul();
				std::cout<<"*******************************make new stmt"<<std::endl;
				/*main stmt 
					Stmt main_stmt = Move::make(
					expr_C,
						Binary::make(data_type, BinaryOpType::Add, expr_C,
						Binary::make(data_type, BinaryOpType::Mul, expr_A, expr_B)),
					MoveType::MemToMem
					);
				*/
				//Stmt loop_nest = LoopNest::make({i, j, k}, {main_stmt});
				int fuhao;
				Stmt main_stmt;
				if (op.empty()){
					std::cout<<"ok?"<<std::endl;
					main_stmt = Move::make(
						makeVar(leftVar, leftAlist, leftClist),
						num.top(),
						MoveType::MemToMem
					);
				} else {
					std::cout<<"ok!"<<std::endl;
					op.pop();
					main_stmt = Move::make(
						makeVar(leftVar, leftAlist, leftClist),
						Binary::make(data_type, BinaryOpType::Sub , makeVar(leftVar, leftAlist, leftClist), num.top()),
						MoveType::MemToMem
					);
				}
				Stmt ifThenElse;
				std::cout<<"ok"<<std::endl;
				if (kernel[i] == '-') op.push('-');
				std::set<std::string> real = loopVar.top();
				bool f = real.empty();
				if (!f) {
                            Expr tmp = ifThen.top();
                            if (!leftAlist.empty()) {
                                std::cout<<"420 ex!\n";
                                makeIfThen(leftAlist, leftClist);
                                Expr leftifThen = ifThen.top();
                                ifThen.pop();
                                tmp = Binary::make(data_type, BinaryOpType::And, tmp, leftifThen);
                            }
                            ifThenElse = IfThenElse::make(tmp, main_stmt, main_stmt);
                            ifThen.pop();
                        } else {
                            if (!leftAlist.empty()) {
                                std::cout<<"420 ex!\n";
                                makeIfThen(leftAlist, leftClist);
                                Expr leftifThen = ifThen.top();
                                ifThen.pop();
                                ifThenElse = IfThenElse::make(leftifThen, main_stmt, main_stmt);
                            } else ifThenElse = main_stmt;
                        }
				num.pop();
				loopVar.pop();
				real.insert(leftLoopVar.begin(), leftLoopVar.end());
				std::cout<<"real loop var"<<std::endl;
				for (std::string s : real) std::cout<<s<<" ";std::cout<<std::endl;
				std::vector<Expr> index = makeLoopVar(real);
				std::vector<Stmt> vvv;
				for (int i=0;i<grads.size();++i)
					vvv.push_back(main_stmt);
				Stmt loop_nest = LoopNest::make(index, vvv);
				allStmt.push_back(loop_nest);
				std::cout<<"*******************************make new stmt over"<<std::endl;
				if (kernel[i] == ';') break; //todo:多个句子
			}
			else {
				if (kernel[i+1] == '/') i+=1;
				while (!op.empty() && pri[kernel[i]] <= pri[op.top()]) calcul();
				op.push(kernel[i]);
			}
		}
		else if (kernel[i] >= '0' && kernel[i] <= '9') {
			std::string tmp = "";
			bool f = false;
			while (kernel[i] >= '0' && kernel[i] <= '9' || kernel[i] == '.'){ //可能是float 
				if (kernel[i] == '.') f=true;
				tmp += kernel[i++];
			}
			if (!f) num.push(IntImm::make(Type::int_scalar(32),std::atoi(tmp.c_str())));
			else num.push(FloatImm::make(Type::float_scalar(32),std::atof(tmp.c_str())));
			std::set<std::string> emptyS;
			emptyS.clear();
			loopVar.push(emptyS);
			std::cout<<"num in "<<std::atoi(tmp.c_str())<<std::endl;
			std::cout<<kernel[i]<<std::endl;
			i-=1;
		}
		else if (kernel[i] == '<') {//A<15,14>[i,j]  A<1>
			Clistin(i, Clist); //无副作用  
			if (Clist.size() == 1 && Clist[0] == 1) {
				std::vector<Expr> noUse;  noUse.clear();
				num.push(Var::make(data_type, nowVar, noUse, Clist));
				std::cout<<"biao liang num push in "<<nowVar<<std::endl;
				std::set<std::string> tmpVar; tmpVar.clear();
				loopVar.push(tmpVar);
			}
		}
		else if (kernel[i] == '[') { //todo:如果是标量只有<>没有[]不会生成exp到栈num中 
			std::vector<Expr> v = Alistin(i, Clist, true);
			num.push(Var::make(data_type, nowVar, v, Clist)); 
			makeIfThen(v, Clist);
			std::cout<<"num push in "<<nowVar<<std::endl;
		} 
		else {
			std::cout<<kernel[i]<<" "<<i<<std::endl;
			nowVar = "";
			while (kernel[i] != ' ' && kernel[i] != '<') nowVar += kernel[i++];
			i -= 1;
		}
	}
	// temp -> realleft
/*	Stmt main_stmt;
	main_stmt = Move::make(makeVar(leftVar, leftAlist, leftClist), makeVar("temp", leftAlist, leftClist), MoveType::MemToMem);
	std::vector<Expr> index = makeLoopVar(leftLoopVar);
	if (!leftAlist.empty()) {
		makeIfThen(leftAlist, leftClist);
		Expr leftifThen = ifThen.top();
		ifThen.pop();
		main_stmt = IfThenElse::make(leftifThen, main_stmt, main_stmt);
	}
	Stmt loop_nest = LoopNest::make(index, {main_stmt});
	allStmt.push_back(loop_nest);*/
    leftShape.assign(leftClist.begin(),leftClist.end());
}

