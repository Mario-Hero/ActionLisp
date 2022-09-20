#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <array>
#include <cmath>
#include <sstream> 
#include <stdio.h>
#include <algorithm>
#include <random>

using namespace std;
enum class AtomType {
	VALUE,    // When VALUE, sval is its string.
	SYMBOL,   // When SYMBOL, sval is its symbol name.
	LIST,     // When LIST, there is no sval.
	FUNCTION, // When FUNCTION, it's v.ptr points to a function. vclass is it's class. 
	VARIABLE  // When VARIABLE, it's v.ptr points to a variable
};

enum class AtomValueType {
	BOOL,LONG,DOUBLE,STRING,OBJECT,POINTER 
	//When AtomType is LIST, this value is useless.
	// If is POINTER, the vclass points to its class name.
};
enum class AtomFlag {
	RETURN, CASE
};

struct Variable;
struct Atom {
	AtomType type= AtomType::VALUE;
	AtomValueType vType = AtomValueType::BOOL;
	vector<AtomFlag> flag;  // used for return and switch-case.
	string vclass="";
	string sval = "";
	vector<Atom> list;
	vector<Variable> childList; 
	Variable* target=nullptr; // When AtomType == FUNCTION, this is used to save [this].
	union {
		bool bval;
		long lval;
		double dval;
		void* ptr;
	} v;
};

struct Expression { // args[0].sval is function name.
	vector<Expression> args;
	Atom atom;  //result
};

typedef unsigned long long LPOS;

enum class ExpressionState {
	IDLE,
	EXPRESSION,
	COMMENT,
	MULTILINE_COMMENT
};

struct Variable {
	string name;
	Atom atom;
};

struct actionFun {
	string name="";
	vector<Variable> input;
	vector<Expression> body;
};

struct actionClass {
	string name = "";
	vector<Variable> memberVariables; //will be copied to atom.childList
	vector<Variable> staticVariables;
	vector<actionFun> memberFunctions;
	vector<actionFun> staticFunctions;
};

class Interpreter {
private:
	ExpressionState state = ExpressionState::IDLE;
	const static array<string, 2> keyWords;
	const static string SWITCH_VAR_NAME;
	const static string ACTIONLISP_CLASS_FSTREAM;
	LPOS nowLine = 1;
	LPOS nowPos = 0;
	string readTemp;
	vector<Variable> globalVariables;
	vector<vector<Variable> > variables = { globalVariables };
	vector<actionFun> functions;
	vector<actionClass> classes;
	Variable NULLVAR;      // Empty Var
	Atom NULLATOM;         // Empty Atom
	actionClass NULLCLASS; // Empty Class
	bool REPLMode = false; // write a line and then execute a line.
	inline Atom doAtomFunction(const Atom& fun, vector<Expression>& in) {
		if (fun.vclass != "") 
			return doFunction(*static_cast<actionFun*>(fun.v.ptr), in, fun.target);
		else
			return doFunction(*static_cast<actionFun*>(fun.v.ptr), in);
	}
	inline bool varIsNull(const Variable& v) {
		return v.name=="";
	}
	inline actionClass& getClassByName(const string& s) {
		for (auto& c : classes) {
			if (c.name == s) {
				return c;
			}
		}
		return NULLCLASS;
	}
	inline bool findFlag(const vector<AtomFlag>& v, const AtomFlag& fl) {
		return find(v.cbegin(), v.cend(), fl) != v.cend();
	}
	inline void removeFlag(vector<AtomFlag>& v, const AtomFlag& fl) {
		const auto& getFlag = find(v.cbegin(), v.cend(), fl);
		if (getFlag != v.cend()) 
			v.erase(getFlag);
	}
	Atom doFunction(const actionFun& constFun, vector<Expression>& in, Variable* actionThis=nullptr) { //in.args[0] is function name, which is useless
		if (in.size() + 1 < constFun.input.size()) {
			throwError("There are not enough arguments for function " + constFun.name);
			return NULLATOM;
		}
		actionFun fun = constFun;
		Atom a;
		if (fun.input.size() == 0) {
			addVariableLevel();
			if (actionThis != nullptr) {
				Variable t;
				t.name = "this";
				t.atom.type = AtomType::VARIABLE;
				t.atom.v.ptr = static_cast<void*>(actionThis);
				variables.back().push_back(t);
			}
			for (size_t i = 0; i < constFun.body.size(); ++i) {
				const Atom& res = doExp(fun.body[i]);
				if (findFlag(res.flag, AtomFlag::RETURN)) {
					a = res;
					break;
				}
			}
			popVariableLevel();
			return a;
		}
		else {
			addVariableLevel();
			if (actionThis != nullptr) {
				Variable t;
				t.name = "this";
				t.atom.type = AtomType::VARIABLE;
				t.atom.v.ptr = static_cast<void*>(actionThis);
				variables.back().push_back(t);
			}
			for (size_t i = 0; i < constFun.input.size(); ++i) {
				Variable t;
				t.name = constFun.input[i].name;
				t.atom = atom2V(doExp(in[i+1]));
				variables.back().push_back(t);
			}
			for (size_t i = 0; i < constFun.body.size(); ++i) {
				const Atom& res = doExp(fun.body[i]);
				if (findFlag(res.flag, AtomFlag::RETURN)) {
					a = res;
					break;
				}
			}
			popVariableLevel();
			return a;
		}
	}
	static inline ios::openmode fileTypeRead(const string& s="r") {
		if (s == "r")
			return ios::in;
		else if (s == "rw")
			return ios::in | ios::out;
		else if (s == "w")
			return ios::out;
		else if (s == "a")
			return ios::app;
		else if (s == "rb")
			return ios::in | ios::binary;
		else if (s == "rwb")
			return ios::in | ios::out | ios::binary;
		else if (s == "wb")
			return ios::out | ios::binary;
		else if (s == "ab")
			return ios::app | ios::binary;
		else
			return ios::in;
	}
	inline void addVariableLevel() {
		variables.push_back(vector<Variable>());
	}
	inline void delVar(Variable& var) {
		if (var.atom.type == AtomType::VALUE && var.atom.vType == AtomValueType::POINTER) {
			if (var.atom.vclass == ACTIONLISP_CLASS_FSTREAM) {
				delete static_cast<fstream*>(var.atom.v.ptr);
			}
			////.......
		}
	}
	inline void popVariableLevel() {
		for (Variable& var : variables.back()) {
			delVar(var);
		}
		variables.pop_back();
	}
	inline void pushBackLong(Atom& a, const long b) {
		Atom t;
		t.vType = AtomValueType::LONG;
		t.type = AtomType::VALUE;
		t.v.lval = b;
		a.list.push_back(t);
	}
	inline bool isKeyWord(const Atom & a) {
		if (a.type == AtomType::SYMBOL) {
			for (const string& key : keyWords) {
				if (key == a.sval)
					return true;
			}
		}
		return false;
	}
	inline void throwError(const string& errorMessage) {
		cout << "Line: " << nowLine << " " << errorMessage << endl;
		if (!REPLMode) {
			throw 0;
			//exit(1);
		}
	}
	const int findVarOnlyInBlock(const string& name) {
		const auto& topBlock = variables.back();
		int findI = -1;
		for (size_t i = 0; i < topBlock.size(); ++i) {
			if (topBlock[i].name == name) {
				findI = i;
				break;
			}
		}
		return findI;
	}
	Variable& findVarInAll(const string& name, const int varBlock=0) {
		int findI = -1;
		const long block = variables.size() - varBlock - 1;
		for (size_t i = 0; i < variables[block].size(); ++i) {
			if (variables[block][i].name == name) {
				findI = i;
				break;
			}
		}
		if (findI == -1) {
			if (block > 0) {
				return findVarInAll(name, varBlock + 1);
			}
			else {
				//throwError("There is no variable named " + name);
				return NULLVAR;
			}
		}
		else {
			return variables[block][findI];
		}
	}
	const Variable& constfindVarInAll(const string& name, const int varBlock = 0) {
		int findI = -1;
		const long block = variables.size() - varBlock - 1;
		for (size_t i = 0; i < variables[block].size(); ++i) {
			if (variables[block][i].name == name) {
				findI = i;
				break;
			}
		}
		if (findI == -1) {
			if (block > 0) {
				return constfindVarInAll(name, varBlock + 1);
			}
			else {
				throwError("There is no variable named " + name);
				return NULLVAR;
			}
		}
		else {
			return variables[block][findI];
		}
	}
	const Atom& atom2V(const Atom& argAtom) {
		if (argAtom.type == AtomType::VALUE || argAtom.type == AtomType::LIST) {
			return argAtom;
		}
		else if (argAtom.type == AtomType::SYMBOL) {
			if (!isKeyWord(argAtom)) {
				return constfindVarInAll(argAtom.sval).atom;
			}
			else {
				return argAtom;
			}
		}
		else if (argAtom.type == AtomType::VARIABLE) {
			return atom2V(static_cast<Variable*>(argAtom.v.ptr)->atom);
		}
		else if (argAtom.type == AtomType::FUNCTION) {
			vector<Expression> emptyExpression;
			return atom2V(doFunction(*static_cast<actionFun*>(argAtom.v.ptr), emptyExpression));
		}
		/*
		else if (argAtom.type == AtomType::LIST) {
			Atom p;
			p.type = AtomType::LIST;
			for (const Atom& a : argAtom.list) {
				p.list.push_back(atom2V(a));
			}
			return p;
		}
		*/
		else {
			return argAtom;
		}
	}
	Variable& atom2Variable(const Atom& argAtom) {
		if (argAtom.type == AtomType::SYMBOL) {
			if (!isKeyWord(argAtom)) {
				return findVarInAll(argAtom.sval);
			}
		}
		else if (argAtom.type == AtomType::VARIABLE) {
			return *static_cast<Variable*>(argAtom.v.ptr);
		}
		else {
			//throwError("Not a variable");
		}
		return NULLVAR;
	}
	string toString(const Atom& a) { //if Atom is VALUE, it doesn't need varBlock
		if (a.type == AtomType::LIST) {
			string res = "{ ";
			for (const Atom& ch : a.list) {
				res += toString(ch) + " ";
			}
			return res + "}";
		}
		Atom t = atom2V(a);
		if (t.type == AtomType::LIST) {
			string res = "{ ";
			for (const Atom& ch : t.list) {
				res += toString(ch) + " ";
			}
			return res + "}";
		}
		switch (t.vType) {
			case AtomValueType::BOOL: {
				return (t.v.bval ? "true" : "false");
			}
			case AtomValueType::LONG: {
				return to_string(t.v.lval);
			}
			case AtomValueType::DOUBLE: {
				return to_string(t.v.dval);
			}
			case AtomValueType::STRING: {
				return t.sval;
			}
			case AtomValueType::POINTER: {
				return "pointer";
			}
			break;
		}
		return "";
	}
	Atom& doExp(Expression& e) {
		if (e.args.size() == 0) {
			return e.atom;
		}
		Atom expGet = doExp(e.args[0]);
		switch (expGet.type) {
			case AtomType::SYMBOL:break;
			case AtomType::VARIABLE:
			case AtomType::LIST:
			case AtomType::VALUE:return expGet;
			case AtomType::FUNCTION:{
				e.atom = doAtomFunction(expGet, e.args);
				return e.atom;
			}
		}
		const string function = expGet.sval;
		if (function == "") {
			return e.atom;
		}
		else {
			if (e.args.size() == 1 && isKeyWord(e.args[0].atom))
				return e.args[0].atom;
			//cout << "Function: " << function << endl;
			e.atom.type = AtomType::VALUE;
			e.atom.vType = AtomValueType::BOOL;
			e.atom.v.bval = false;
			if (function == "error") {
				if (e.args.size() == 2) {
					throwError(atom2V(doExp(e.args[1])).sval);
				}
				else {
					throwError("");
				}
				return e.atom;
			}
			else if (function == "import") {
				if (e.args.size() >= 2) {
					auto bnowLine = nowLine;
					auto bnowPos = nowPos;
					for (size_t i = 1; i < e.args.size(); ++i) {
						readFile(atom2V(doExp(e.args[i])).sval);
					}
					nowLine = bnowLine;
					nowPos = bnowPos;
				}
				else {
					throwError("No enough arguments for " + function);
				}
				return e.atom;
			}
			// class
			else if (function == "class") {
				if (e.args.size() >= 2) {
					const string className = doExp(e.args[1]).sval; //SYMBOL
					for (const auto& haveClass : classes) {
						if (haveClass.name == className) {
							throwError("Class " + className + " has been defined.");
							return e.atom;
						}
					}
					classes.push_back(actionClass());
					classes.back().name = className;
					if (e.args.size() >= 3) {
						for (size_t i = 2; i < e.args.size(); ++i) {
							doExp(e.args[i]);
						}
					}
				}
				else {
					throwError("No enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "defVar") {
				if (e.args.size() >= 2) {
					auto& targetClass = classes.back();
					if (e.args.size() >= 3) {
						const string v = doExp(e.args[1]).sval;
						for (const auto& mem : targetClass.memberVariables) {
							if (mem.name == v) {
								throwError("Class variable " + v + " has been defined.");
								return e.atom;
							}
						}
						targetClass.memberVariables.push_back(Variable());
						targetClass.memberVariables.back().name = v;
						if (e.args.size() == 3) {
							targetClass.memberVariables.back().atom = atom2V(doExp(e.args[2]));
						}
					}
					else {
						throwError("Not correct number of arguments for " + function);
					}
				}
				else {
					throwError("No enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "defCVar") {
				if (e.args.size() >= 2) {
					auto& targetClass = classes.back();
					if (e.args.size() >= 3) {
						const string v = doExp(e.args[1]).sval;
						for (const auto& mem : targetClass.staticVariables) {
							if (mem.name == v) {
								throwError("Class static const " + v + " has been defined.");
								return e.atom;
							}
						}
						targetClass.staticVariables.push_back(Variable());
						targetClass.staticVariables.back().name = v;
						if (e.args.size() == 3) {
							targetClass.staticVariables.back().atom = atom2V(doExp(e.args[2]));
						}
					}
					else {
						throwError("Not correct number of arguments for " + function);
					}
				}
				else {
					throwError("No enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "defMethod") {
				if (e.args.size() >= 3) {
					auto& targetClass = classes.back();
					const Atom& functionName = doExp(e.args[1]);
					if (functionName.type == AtomType::SYMBOL) {
						for (const auto& fun : targetClass.memberFunctions) {
							if (functionName.sval == fun.name) {
								throwError("The method " + fun.name + " has been defined.");
								return e.atom;
							}
						}
						actionFun t;
						t.name = functionName.sval;
						if (e.args.size() >= 4) {
							const Atom& arguments = doExp(e.args[2]);
							if (arguments.type == AtomType::LIST) {
								for (size_t i = 0; i < arguments.list.size(); ++i) {
									Variable v;
									v.name = arguments.list[i].sval;
									t.input.push_back(v);
								}
							}
							else if (arguments.type == AtomType::SYMBOL) {
								Variable v;
								v.name = arguments.sval;
								t.input.push_back(v);
							}
							else {
								throwError("The arguments of def should be symbol.");
								return e.atom;
							}
							for (size_t i = 3; i < e.args.size(); ++i) {
								t.body.push_back(e.args[i]);
							}
							targetClass.memberFunctions.push_back(t);
							return e.atom;
						}
						else {
							for (size_t i = 2; i < e.args.size(); ++i) {
								t.body.push_back(e.args[i]);
							}
							targetClass.memberFunctions.push_back(t);
							return e.atom;
						}

					}
					else {
						throwError("The first argument of def should be a symbol.");
					}
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
			}
			else if (function == "defCMethod") {
				if (e.args.size() >= 3) {
					auto& targetClass = classes.back();
					const Atom& functionName = doExp(e.args[1]);
					if (functionName.type == AtomType::SYMBOL) {
						for (const auto& fun : targetClass.staticFunctions) {
							if (functionName.sval == fun.name) {
								throwError("The static method " + fun.name + " has been defined.");
								return e.atom;
							}
						}
						actionFun t;
						t.name = functionName.sval;
						if (e.args.size() >= 4) {
							const Atom& arguments = doExp(e.args[2]);
							if (arguments.type == AtomType::LIST) {
								for (size_t i = 0; i < arguments.list.size(); ++i) {
									Variable v;
									v.name = arguments.list[i].sval;
									t.input.push_back(v);
								}
							}
							else if (arguments.type == AtomType::SYMBOL) {
								Variable v;
								v.name = arguments.sval;
								t.input.push_back(v);
							}
							else {
								throwError("The arguments of def should be symbol.");
								return e.atom;
							}
							for (size_t i = 3; i < e.args.size(); ++i) {
								t.body.push_back(e.args[i]);
							}
							targetClass.staticFunctions.push_back(t);
							return e.atom;
						}
						else {
							for (size_t i = 2; i < e.args.size(); ++i) {
								t.body.push_back(e.args[i]);
							}
							targetClass.staticFunctions.push_back(t);
							return e.atom;
						}

					}
					else {
						throwError("The first argument of def should be a symbol.");
					}
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
			}
			// logic
			else if (function == "and") {
				e.atom.type = AtomType::VALUE;
				e.atom.vType = AtomValueType::BOOL;
				e.atom.v.bval = true;
				if (e.args.size() >= 2) {
					for (size_t i = 1; i < e.args.size(); ++i) {
						if (!isTrue(atom2V(doExp(e.args[i])))) {
							e.atom.v.bval = false;
							break;
						}
					}
				}
				else {
					throwError("No enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "or") {
				e.atom.type = AtomType::VALUE;
				e.atom.vType = AtomValueType::BOOL;
				e.atom.v.bval = false;
				if (e.args.size() >= 2) {
					for (size_t i = 1; i < e.args.size(); ++i) {
						if (isTrue(atom2V(doExp(e.args[i])))) {
							e.atom.v.bval = true;
							break;
						}
					}
				}
				else {
					throwError("No enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "not") {
				e.atom.type = AtomType::VALUE;
				e.atom.vType = AtomValueType::BOOL;
				if (e.args.size() == 2) {
					e.atom.v.bval = !isTrue(atom2V(doExp(e.args[1])));
				}
				else {
					throwError("No correct number of arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "makeChild") {
				if (e.args.size() >= 3) {
					const string varName = doExp(e.args[1]).sval; //e.args[1] is symbol
					Variable* var = &findVarInAll(varName);
					if (varIsNull(*var)) {
						throwError("Variable " + varName + " doesn't exist.");
						return e.atom;
					}
					for (size_t i = 2; i < e.args.size(); ++i) {
						const string childName = doExp(e.args[i]).sval;
						bool findChild = false;
						for (Variable& ch : var->atom.childList) {
							if (ch.name == childName) {
								var = &ch;
								findChild = true;
								break;
							}
						}
						if (!findChild) {
							Variable t;
							t.name = childName;
							var->atom.childList.push_back(t);
							e.atom.type = AtomType::VARIABLE;
							e.atom.v.ptr =  static_cast<void*>(&var->atom.childList.back());
							return e.atom;
						}
					}
					e.atom.type = AtomType::VARIABLE;
					e.atom.v.ptr = static_cast<void*>(var);
					return e.atom;
				}
				else {
					throwError("No enough arguments for " + function);
					return e.atom;
				}
			}
			else if (function == "$") {
				if (e.args.size() >= 2) {
					e.atom.sval = "";
					for (size_t i=1; i < e.args.size(); ++i) {
						e.atom.sval.append(toString(atom2V(doExp(e.args[i]))));
					}
					e.atom.type = AtomType::SYMBOL;
				}
				else {
					throwError("No enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "@") {  
				if (e.args.size() >= 3) {
					Atom varName = doExp(e.args[1]);
					Variable* var = nullptr;
					switch (varName.type) {
						case AtomType::SYMBOL:{
							var = &findVarInAll(varName.sval);
							if (varIsNull(*var)) {
								//Class
								bool findClass = false;
								for (actionClass& aClass : classes) {
									if (aClass.name == varName.sval) {
										findClass = true;
										for (size_t i = 2; i < e.args.size(); ++i) {
											const string childName = doExp(e.args[i]).sval;
											bool findChild = false;
											for (Variable& ch : aClass.staticVariables) {
												if (ch.name == childName) {
													var = &ch;
													findChild = true;
													break;
												}
											}
											for (actionFun& fun : aClass.staticFunctions) {
												if (fun.name == childName) {
													e.atom.type = AtomType::FUNCTION;
													e.atom.v.ptr = static_cast<void*>(&fun);
													e.atom.vclass = var->atom.vclass;
													e.atom.target = nullptr;
													return e.atom;
												}
											}
											if (!findChild) {
												throwError(var->name + " doesn't have child " + childName);
												return e.atom;
											}
										}
										e.atom.type = AtomType::VARIABLE;
										e.atom.v.ptr = static_cast<void*>(var);
										return e.atom;
									}
								}
								if (!findClass) {
									throwError("Variable " + varName.sval + " doesn't exist.");
									return e.atom;
								}
							}
							break;
						}
						case AtomType::VARIABLE:
						{
							var = static_cast<Variable*>(varName.v.ptr);
							break;
						}
						case AtomType::FUNCTION:
						{
							vector<Expression> in;
							var = new Variable();
							var->atom = doAtomFunction(varName,in);
							break;
						}
						case AtomType::LIST:
						case AtomType::VALUE:
						{
							throwError("Type Error for @");
							return e.atom;
						}
					}
					while (var->atom.type == AtomType::VARIABLE) {
						var = static_cast<Variable*>(var->atom.v.ptr);
					}
					for (size_t i = 2; i < e.args.size(); ++i) {
						const string childName = doExp(e.args[i]).sval;
						bool findChild = false;
						for (Variable& ch : var->atom.childList) {
							if (ch.name == childName){
								var = &ch;
								findChild = true;
								break;
							}
						}
						if (var->atom.vType==AtomValueType::OBJECT && var->atom.vclass != "") {
							actionClass& c = getClassByName(var->atom.vclass);
							for (actionFun& fun : c.memberFunctions) {
								if (fun.name == childName) {
									e.atom.type = AtomType::FUNCTION;
									e.atom.v.ptr = static_cast<void*>(&fun);
									e.atom.vclass = var->atom.vclass;
									e.atom.target = var;
									return e.atom;
								}
							}
						}
						if (!findChild) {
							throwError(var->name + " doesn't have child " + childName);
							return e.atom;
						}
					}
					e.atom.type = AtomType::VARIABLE;
					e.atom.v.ptr = static_cast<void*>(var);
					return e.atom;
				}
				else {
					throwError("No enough arguments for " + function);
					return e.atom;
				}
			}
			else if (function == "let") {
				if (e.args.size() == 3 || e.args.size() == 2) {
					const Atom& symbolName = doExp(e.args[1]);
					if (symbolName.type == AtomType::SYMBOL) {
						const string symbolString = symbolName.sval;
						const int FindI = findVarOnlyInBlock(symbolString);
						if (FindI != -1) {
							throwError("Variable " + symbolName.sval + " already existed.");
							return e.atom;
						}
						else {
							Variable t;
							if (e.args.size() == 3) {
								Atom v = doExp(e.args[2]);
								if (v.type == AtomType::SYMBOL) {
									for (const auto& aClass : classes) {
										if (aClass.name == v.sval) {
											t.atom.type = AtomType::VALUE;
											t.atom.vType = AtomValueType::OBJECT;
											t.atom.vclass = v.sval;
											t.name = symbolName.sval;
											for (Variable mem : aClass.memberVariables) {
												t.atom.childList.push_back(mem);
											}
											variables.back().push_back(t);
											//cout << "var " << t.name << "=" << v.sval << endl;
											return t.atom;
										}
									}
								}
								t.atom = atom2V(v);
							}
							else {
								t.atom.type = AtomType::VALUE;
							}
							t.name = symbolName.sval;
							variables.back().push_back(t);
							//cout << "var " << t.name << "=" << toString(t.atom) << endl;
							return t.atom;
						}
					}
					else {
						throwError(symbolName.sval + "is not symbol");
					}
				}
				else {
					for (auto& arg : e.args) {
						cout << "Wrong argument: " << doExp(arg).sval << endl;
					}
					throwError("wrong arguments number: " + to_string(e.args.size()));
				}
			}
			else if (function == "global") {
				if (e.args.size() >= 3) {
					const Atom& symbolName = doExp(e.args[1]);
					if (symbolName.type == AtomType::SYMBOL) {
						const int FindI = findVarOnlyInBlock(symbolName.sval);
						if (FindI != -1) {
							throwError("Variable " + symbolName.sval + " already existed.");
						}
						else {
							Variable t;
							if (e.args.size() == 3) {
								Atom v = doExp(e.args[2]);
								if (v.type == AtomType::SYMBOL) {
									for (const auto& aClass : classes) {
										if (aClass.name == v.sval) {
											t.atom.type = AtomType::VALUE;
											t.atom.vType = AtomValueType::OBJECT;
											t.atom.vclass = v.sval;
											for (Variable mem : aClass.memberVariables) {
												t.atom.childList.push_back(mem);
											}
											variables.front().push_back(t);
											//cout << "global " << t.name << "=" << v.sval << endl;
											return t.atom;
										}
									}
								}
								t.atom = atom2V(v);
							}
							else {
								t.atom.type = AtomType::VALUE;
							}
							t.name = symbolName.sval;
							variables.front().push_back(t);
							//cout << "global " << t.name << "=" << toString(t.atom) << endl;
							return t.atom;
						}
					}
					else {
						throwError(symbolName.sval + "is not symbol");
					}
				}
				else {
					for (auto& arg : e.args) {
						cout << "Wrong argument: " << doExp(arg).sval << endl;
					}
					throwError("wrong arguments number: " + to_string(e.args.size()));
				}
			}
			else if (function == "set") {
				if (e.args.size() >= 3) {
					Atom symbolName = doExp(e.args[1]);
					if (symbolName.type == AtomType::SYMBOL) {
						Variable& var = findVarInAll(doExp(e.args[1]).sval);
						if (var.name != "") {
							var.atom = atom2V(doExp(e.args[2]));
							//cout << "set " << doExp(e.args[1]).sval << "=" << toString(var.atom, e.block) << endl;
						}
						return var.atom;
					}
					else if (symbolName.type == AtomType::VARIABLE) {
						(static_cast<Variable*>(symbolName.v.ptr))->atom = atom2V(doExp(e.args[2]));
					}
					else {
						throwError(doExp(e.args[1]).sval + "is not symbol");
					}
				}
				else {
					for (auto& arg : e.args) {
						cout << "Wrong argument: " << doExp(arg).sval << endl;
					}
					throwError("wrong arguments number: " + to_string(e.args.size()));
				}
			}
			else if (function == "<=") {
				e.atom.type = AtomType::VALUE;
				e.atom.vType = AtomValueType::BOOL;
				if (e.args.size() >= 3) {
					Atom firstAtom = atom2V(doExp(e.args[1]));
					for (int i = 2; i < e.args.size(); ++i) {
						Atom secondAtom = atom2V(doExp(e.args[i]));
						if (bigger(firstAtom, secondAtom)) {
							e.atom.v.bval = false;
							return e.atom;
						}
					}
					e.atom.v.bval = true;
					return e.atom;
				}
				else {
					throwError("Not enough arguments for " + function);
				}
			}
			else if (function == ">=") {
				e.atom.type = AtomType::VALUE;
				e.atom.vType = AtomValueType::BOOL;
				if (e.args.size() >= 3) {
					Atom firstAtom = atom2V(doExp(e.args[1]));
					for (int i = 2; i < e.args.size(); ++i) {
						Atom secondAtom = atom2V(doExp(e.args[i]));
						if (bigger(secondAtom, firstAtom)) {
							e.atom.v.bval = false;
							return e.atom;
						}
					}
					e.atom.v.bval = true;
					return e.atom;
				}
				else {
					throwError("Not enough arguments for " + function);
				}
			}
			else if (function == "<") {
				e.atom.type = AtomType::VALUE;
				e.atom.vType = AtomValueType::BOOL;
				if (e.args.size() >= 3) {
					const Atom firstAtom = atom2V(doExp(e.args[1]));
					for (int i = 2; i < e.args.size(); ++i) {
						const Atom secondAtom = atom2V(doExp(e.args[i]));
						if (!bigger(secondAtom, firstAtom)) {
							e.atom.v.bval = false;
							return e.atom;
						}
					}
					e.atom.v.bval = true;
					return e.atom;
				}
				else {
					throwError("Not enough arguments for " + function);
				}
			}
			else if (function == ">") {
				e.atom.type = AtomType::VALUE;
				e.atom.vType = AtomValueType::BOOL;
				if (e.args.size() >= 3) {
					const Atom firstAtom = atom2V(doExp(e.args[1]));
					for (int i = 2; i < e.args.size(); ++i) {
						const Atom secondAtom = atom2V(doExp(e.args[i]));
						if (!bigger(firstAtom, secondAtom)) {
							e.atom.v.bval = false;
							return e.atom;
						}
					}
					e.atom.v.bval = true;
					return e.atom;
				}
				else {
					throwError("Not enough arguments for " + function);
				}
			}
			else if (function == "=" || function == "equal") {
				e.atom.type = AtomType::VALUE;
				e.atom.vType = AtomValueType::BOOL;
				if (e.args.size() >= 3) {
					const Atom firstAtom = atom2V(doExp(e.args[1]));
					for (int i = 2; i < e.args.size(); ++i) {
						const Atom secondAtom = atom2V(doExp(e.args[i]));
						if (!equal(firstAtom, secondAtom)) {
							e.atom.v.bval = false;
							return e.atom;
						}
					}
					e.atom.v.bval = true;
					return e.atom;
				}
				else {
					throwError("Not enough arguments for " + function);
				}
			}
			else if (function == "*") {
				for (int i = 1; i < e.args.size(); ++i) {
					const Atom& argAtom = doExp(e.args[i]);
					if (argAtom.type == AtomType::LIST)
						throwError("Cannot do operation * on list.");
					if (i == 1) {
						e.atom = atom2V(argAtom);
						switch (e.atom.vType) {
						case AtomValueType::BOOL: {
							e.atom.vType = AtomValueType::LONG;
							if (e.atom.v.bval) {
								e.atom.v.lval = 1;
							}
							else {
								e.atom.v.lval = 0;
								return e.atom;
							}
							break;
						}//can do * operation on String, which means n times of string.
						default: break;
						}
					}
					else {
						const Atom& arg = atom2V(argAtom);
						switch (e.atom.vType) {
						case AtomValueType::LONG: {
							switch (arg.vType) {
							case AtomValueType::BOOL: {
								if (!arg.v.bval) {
									e.atom.v.lval = 0;
									return e.atom;
								}
								break;
							}
							case AtomValueType::LONG: {
								if (arg.v.lval == 0) {
									e.atom.v.lval = 0;
									return e.atom;
								}
								else {
									e.atom.v.lval *= arg.v.lval;
								}
								break;
							}
							case AtomValueType::DOUBLE: {
								if (arg.v.dval == 0.0) {
									e.atom.v.lval = 0;
									return e.atom;
								}
								else {
									e.atom.vType = AtomValueType::DOUBLE;
									e.atom.v.dval = (double)e.atom.v.dval * arg.v.dval;
								}
								break;
							}
							case AtomValueType::STRING: {
								e.atom.vType = AtomValueType::STRING;
								while (e.atom.v.lval > 0) {
									e.atom.v.lval -= 1;
									e.atom.sval += arg.sval;
								}
								break;
							}
							}
							break;
						}
						case AtomValueType::DOUBLE: {
							switch (arg.vType) {
							case AtomValueType::BOOL: {
								if (!arg.v.bval) {
									e.atom.v.dval = 0.0;
									return e.atom;
								}
								break;
							}
							case AtomValueType::LONG: {
								if (arg.v.lval == 0) {
									e.atom.v.dval = 0;
									return e.atom;
								}
								else {
									e.atom.v.dval *= (double)arg.v.lval;
								}
								break;
							}
							case AtomValueType::DOUBLE: {
								if (arg.v.dval == 0.0) {
									e.atom.v.dval = 0.0;
									return e.atom;
								}
								else {
									e.atom.v.dval *= arg.v.dval;
								}
								break;
							}
							case AtomValueType::STRING: {
								throwError("Cannot do (* String Double)");
								break;
							}
							}
							break;
						}
						case AtomValueType::STRING: {
							switch (arg.vType) {
							case AtomValueType::BOOL: {
								if (!arg.v.bval) {
									e.atom.sval = "";
									return e.atom;
								}
								break;
							}
							case AtomValueType::LONG: {
								if (arg.v.lval == 0) {
									e.atom.sval = "";
									return e.atom;
								}
								else {
									while (e.atom.v.lval > 1) {
										e.atom.v.lval -= 1;
										e.atom.sval += arg.sval;
									}
									break;
								}
								break;
							}
							case AtomValueType::DOUBLE: {
								if (arg.v.dval == 0.0) {
									e.atom.sval = "";
									return e.atom;
								}
								else {
									throwError("Cannot do (* String Double)");
								}
								break;
							}
							case AtomValueType::STRING: {
								throwError("Cannot do (* String String)");
								break;
							}
							}
							break;
						}
						}
					}
				}
				return e.atom;
			}
			else if (function == "-") {
				Atom t;
				for (int i = 1; i < e.args.size(); ++i) {
					const Atom& argAtom = doExp(e.args[i]);
					if (argAtom.type == AtomType::LIST)
						throwError("Cannot do operation - on list.");
					if (i == 1) {
						e.atom = atom2V(argAtom);
						switch (e.atom.vType) {
						case AtomValueType::BOOL: {
							e.atom.vType = AtomValueType::LONG;
							e.atom.v.lval = e.atom.v.bval ? 1 : 0;
							break;
						}
						case AtomValueType::STRING: {
							throwError("Cannot do \"-\" operation on string value");
							break;
						}
						default: break;
						}
					}
					else {
						const Atom& arg = atom2V(argAtom);
						switch (e.atom.vType) {
						case AtomValueType::LONG: {
							switch (arg.vType) {
							case AtomValueType::BOOL: {
								e.atom.v.lval -= arg.v.bval ? 1 : 0;
								break;
							}
							case AtomValueType::LONG: {
								e.atom.v.lval -= arg.v.lval;
								break;
							}
							case AtomValueType::DOUBLE: {
								e.atom.vType = AtomValueType::DOUBLE;
								e.atom.v.dval = (double)e.atom.v.lval - arg.v.dval;
								break;
							}
							case AtomValueType::STRING: {
								throwError("Cannot do \"-\" operation on string value");
								break;
							}
							}
							break;
						}
						case AtomValueType::DOUBLE: {
							switch (arg.vType) {
							case AtomValueType::BOOL: {
								e.atom.v.dval -= arg.v.bval ? 1.0 : 0;
								break;
							}
							case AtomValueType::LONG: {
								e.atom.v.dval -= (double)arg.v.lval;
								break;
							}
							case AtomValueType::DOUBLE: {
								e.atom.v.dval -= arg.v.dval;
								break;
							}
							case AtomValueType::STRING: {
								throwError("Cannot do \"-\" operation on string value");
								break;
							}
							}
							break;
						}
						}
					}
				}
				return e.atom;
			}
			else if (function == "/") {
				Atom t;
				for (int i = 1; i < e.args.size(); ++i) {
					const Atom& argAtom = doExp(e.args[i]);
					if (argAtom.type == AtomType::LIST)
						throwError("Cannot do operation / on list.");
					if (i == 1) {
						e.atom = atom2V(argAtom);
						switch (e.atom.vType) {
						case AtomValueType::BOOL: {
							e.atom.vType = AtomValueType::LONG;
							e.atom.v.lval = e.atom.v.bval ? 1 : 0;
							break;
						}
						case AtomValueType::STRING: {
							throwError("Cannot do / operation on string value");
							break;
						}
						default: break;
						}
					}
					else {
						const Atom& arg = atom2V(argAtom);
						switch (e.atom.vType) {
						case AtomValueType::LONG: {
							switch (arg.vType) {
							case AtomValueType::BOOL: {
								if (!arg.v.bval) {
									throwError("Error: divided by 0");
								}
								break;
							}
							case AtomValueType::LONG: {
								if (arg.v.lval == 0) {
									throwError("Error: divided by 0");
								}
								else {
									e.atom.vType = AtomValueType::DOUBLE;
									e.atom.v.dval = (double)e.atom.v.lval / (double)arg.v.lval;
								}
								break;
							}
							case AtomValueType::DOUBLE: {
								if (arg.v.dval == 0) {
									throwError("Error: divided by 0");
								}
								else {
									e.atom.vType = AtomValueType::DOUBLE;
									e.atom.v.dval = (double)e.atom.v.lval / arg.v.dval;
								}
								break;
							}
							case AtomValueType::STRING: {
								throwError("Cannot do / operation on string value");
								break;
							}
							}
							break;
						}
						case AtomValueType::DOUBLE: {
							switch (arg.vType) {
							case AtomValueType::BOOL: {
								if (!arg.v.bval) {
									throwError("Error: divided by 0");
								}
								break;
							}
							case AtomValueType::LONG: {
								if (arg.v.lval == 0) {
									throwError("Error: divided by 0");
								}
								else {
									e.atom.v.dval /= (double)arg.v.lval;
								}
								break;
							}
							case AtomValueType::DOUBLE: {
								if (arg.v.lval == 0) {
									throwError("Error: divided by 0");
								}
								else {
									e.atom.v.dval /= arg.v.dval;
								}
								break;
								break;
							}
							case AtomValueType::STRING: {
								throwError("Cannot do / operation on string value");
								break;
							}
							}
							break;
						}
						}
					}
				}
				return e.atom;
			}
			else if (function == "system") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::LONG;
					e.atom.v.lval = system(atom2V(doExp(e.args[1])).sval.c_str());
				}
				else {
					throwError("Not correct arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "++") {
				if (e.args.size() >= 2) {
					const Atom& p = doExp(e.args[1]);
					Variable& var = atom2Variable(p);
					if (var.name != "") {
						switch (var.atom.vType) {
							case AtomValueType::LONG: {
								if (e.args.size() == 2) {
									++var.atom.v.lval;
									break;
								}
								else {
									for (size_t i = 2; i < e.args.size(); ++i) {
										const Atom a = atom2V(doExp(e.args[i]));
										if (a.vType == AtomValueType::LONG) {
											var.atom.v.lval += a.v.lval;
										}
										else {
											throwError("Cannot do operation " + function + " to this type.");
											break;
										}
									}
								}


							}
							default: {
								throwError("Cannot do operation " + function + " to this type.");
								break;
							}
						}
					}
					return var.atom;
					
				}
				else {
					throwError("Not enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "--") {
				if (e.args.size() >= 2) {
					const Atom& p = doExp(e.args[1]);
					Variable & var = atom2Variable(p);
					if (var.name != "") {
						switch (var.atom.vType) {
							case AtomValueType::LONG: {
								if (e.args.size() == 2) {
									--var.atom.v.lval;
									break;
								}
								else {
									for (size_t i = 2; i < e.args.size(); ++i) {
										const Atom a = atom2V(doExp(e.args[i]));
										if (a.vType == AtomValueType::LONG) {
											var.atom.v.lval -= a.v.lval;
										}
										else {
											throwError("Cannot do operation " + function + " to this type.");
											break;
										}
									}
								}


							}
							default: {
								throwError("Cannot do operation " + function + " to this type.");
								break;
							}
						}
					}
					return var.atom;
					
				}
				else {
					throwError("Not enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "+") {
				for (int i = 1; i < e.args.size(); ++i) {
					if (i == 1) {
						e.atom = atom2V(doExp(e.args[i]));
						if(e.atom.type == AtomType::LIST){
							throwError("Cannot do operation + on list.");
							return e.atom;
						}
						switch (e.atom.vType) {
							case AtomValueType::BOOL:
							{
								e.atom.vType = AtomValueType::LONG;
								e.atom.v.lval = e.atom.v.bval ? 1 : 0;
								break;
							}
							default: break;
						}
					}
					else {
						const Atom& arg = atom2V(doExp(e.args[i]));
						switch (e.atom.vType) {
						case AtomValueType::LONG: {
							switch (arg.vType) {
							case AtomValueType::BOOL: {
								e.atom.v.lval += arg.v.bval ? 1 : 0;
								break;
							}
							case AtomValueType::LONG: {
								e.atom.v.lval += arg.v.lval;
								break;
							}
							case AtomValueType::DOUBLE: {
								e.atom.vType = AtomValueType::DOUBLE;
								e.atom.v.dval = (double)e.atom.v.lval + arg.v.dval;
								break;
							}
							case AtomValueType::STRING: {
								e.atom.sval = to_string(e.atom.v.lval) + arg.sval;
								e.atom.vType = AtomValueType::STRING;
								break;
							}
							}
							break;
						}

						case AtomValueType::DOUBLE: {
							switch (arg.vType) {
							case AtomValueType::BOOL: {
								e.atom.v.dval += arg.v.bval ? 1.0 : 0;
								break;
							}
							case AtomValueType::LONG: {
								e.atom.v.dval += (double)arg.v.lval;
								break;
							}
							case AtomValueType::DOUBLE: {
								e.atom.v.dval += arg.v.dval;
								break;
							}
							case AtomValueType::STRING: {
								e.atom.sval = to_string(e.atom.v.dval) + arg.sval;
								e.atom.vType = AtomValueType::STRING;
								break;
							}
							}
							break;
						}
						case AtomValueType::STRING: {
							switch (arg.vType) {
							case AtomValueType::BOOL: {
								e.atom.sval += (arg.v.bval ? "true" : "false");
								break;
							}
							case AtomValueType::LONG: {
								e.atom.sval += to_string(arg.v.lval);
								break;
							}
							case AtomValueType::DOUBLE: {
								e.atom.sval += to_string(arg.v.dval);
								break;
							}
							case AtomValueType::STRING: {
								e.atom.sval += arg.sval;
								break;
							}
							}
							break;
						}
						}
					}
				}
				return e.atom;
			}
			else if (function == "mod") {
				if (e.args.size() >= 3) {
					e.atom = doExp(e.args[1]);
					switch (e.atom.vType) {
						case AtomValueType::STRING: {
							throwError("cannot do mod operation on string");
							e.atom.vType = AtomValueType::LONG;
							e.atom.v.lval = 0;
							return e.atom;
						}
						case AtomValueType::BOOL: {
							e.atom.vType = AtomValueType::LONG;
							e.atom.v.lval = 0;
							return e.atom;
						}
					}
					for (size_t i = 2; i < e.args.size(); ++i) {
						const Atom& arg = atom2V(doExp(e.args[i]));
						switch (e.atom.vType) {
						case AtomValueType::LONG: {
							switch (arg.vType) {
							case AtomValueType::LONG: {
								e.atom.v.lval %= arg.v.lval;
								break;
							}
							case AtomValueType::DOUBLE: {
								e.atom.vType = AtomValueType::DOUBLE;
								e.atom.v.dval = fmod((double)e.atom.v.lval,arg.v.dval);
								break;
							}
							case AtomValueType::STRING: {
								throwError("cannot do mod operation on string");
								e.atom.vType = AtomValueType::LONG;
								e.atom.v.lval = 0;
								return e.atom;
							}
							case AtomValueType::BOOL: {
								e.atom.vType = AtomValueType::LONG;
								e.atom.v.lval = 0;
								return e.atom;
							}
							}
							break;
						}
						case AtomValueType::DOUBLE: {
							switch (arg.vType) {
							case AtomValueType::LONG: {
								e.atom.v.dval = fmod(e.atom.v.dval,(double)arg.v.lval);
								break;
							}
							case AtomValueType::DOUBLE: {
								e.atom.v.dval = fmod(e.atom.v.dval, arg.v.dval);
								break;
							}
							case AtomValueType::STRING: {
								throwError("cannot do mod operation on string");
								e.atom.vType = AtomValueType::LONG;
								e.atom.v.lval = 0;
								return e.atom;
							}
							case AtomValueType::BOOL: {
								e.atom.vType = AtomValueType::LONG;
								e.atom.v.lval = 0;
								return e.atom;
							}
							}
							break;
						}
						}
					}
					return e.atom;
				}
			}
			else if (function == "if") {
				if (e.args.size() == 4) {
					if (isTrue(atom2V(doExp(e.args[1])))) {
						addVariableLevel();
						e.atom = doExp(e.args[2]);
					}
					else {
						addVariableLevel();
						e.atom = doExp(e.args[3]);
					}
					popVariableLevel();
					return e.atom;
				}
				else if (e.args.size() == 3) {
					if (isTrue(atom2V(doExp(e.args[1])))) {
						addVariableLevel();
						e.atom = doExp(e.args[2]);
						popVariableLevel();
					}
					else {
						e.atom.type = AtomType::VALUE;
						e.atom.vType = AtomValueType::BOOL;
						e.atom.v.bval = true;   // return value when !if is not defined
					}
					return e.atom;
				}
				else {
					throwError("Not correct number of arguments for " + function + ", which is " + to_string(e.args.size()-1));
				}
			}
			else if (function == "switch") {
				if (e.args.size() >= 3) {
					addVariableLevel();
					Variable t;
					t.name = SWITCH_VAR_NAME;
					t.atom = atom2V(doExp(e.args[1]));
					variables.back().push_back(t);
					for (size_t i = 2; i < e.args.size(); ++i) {
						const Atom& res = doExp(e.args[i]);
						if (findFlag(res.flag, AtomFlag::CASE)) {
							e.atom = res;
							removeFlag(e.atom.flag, AtomFlag::CASE);
							break;
						}
					}
					popVariableLevel();
					return e.atom;
				}
				else {
					throwError("Not correct number of arguments for " + function + ", which is " + to_string(e.args.size() - 1));
				}
			}
			else if (function == "case") {
				if (e.args.size() >= 2) {
					const Atom& t = atom2V(doExp(e.args[1]));
					for (const Variable& v : variables.back()) {
						if (v.name == SWITCH_VAR_NAME) {
							if (t.type == AtomType::LIST) {
								for (const Atom& ta : t.list) {
									if (equal(ta, v.atom)) {
										for (size_t i = 2; i < e.args.size(); ++i) {
											if (i == e.args.size() - 1) {
												e.atom = atom2V(doExp(e.args[i]));
											}
											else {
												doExp(e.args[i]);
											}
										}
										e.atom.flag.push_back(AtomFlag::CASE);
										return e.atom;
									}
								}
								return e.atom;
							}
							else {
								if (equal(t, v.atom)) {
									for (size_t i = 2; i < e.args.size(); ++i) {
										if (i == e.args.size() - 1) {
											e.atom = atom2V(doExp(e.args[i]));
										}
										else {
											doExp(e.args[i]);
										}
									}
									e.atom.flag.push_back(AtomFlag::CASE);
									return e.atom;
								}
								return e.atom;
							}
							break;
						}
					}
				}
				else {
					throwError("Not correct number of arguments for " + function + ", which is " + to_string(e.args.size() - 1));
					return e.atom;
				}
				return e.atom;
			}
			else if (function == "while") {
				if (e.args.size() >= 2) {
					while (isTrue(atom2V(doExp(e.args[1])))) {
						addVariableLevel();
						bool keepWhile = true;
						for (size_t i = 2; i < e.args.size(); ++i) {
							const Atom& t = doExp(e.args[i]);
							if (t.type == AtomType::SYMBOL) {
								if (t.sval == "break") {
									keepWhile = false;
									break;
								}
								else if (t.sval == "continue") {
									keepWhile = true;
									break;
								}
							}
						}
						popVariableLevel();
						if (!keepWhile)
							break;
					}
					return e.atom;
				}
				else {
					throwError("Not enough arguments for " + function);
				}
			}
			else if (function == "for") {
			if (e.args.size() >= 4) {
				addVariableLevel();
				Variable t;
				//t.atom.type = AtomType::VALUE;
				t.name = doExp(e.args[1]).sval;
				variables.back().push_back(t);
				const size_t tloc = variables.back().size() - 1;
				const Atom& l = doExp(e.args[2]);
				if (l.type == AtomType::LIST) {
					for (size_t j = 0; j < l.list.size(); ++j) {
						bool keepWhile = true;
						variables.back()[tloc].atom = l.list[j];
						for (size_t i = 3; i < e.args.size(); ++i) {
							const Atom& res = doExp(e.args[i]);
							if (res.type == AtomType::SYMBOL) {
								if (res.sval == "break") {
									keepWhile = false;
									break;
								}
								else if (res.sval == "continue") {
									keepWhile = true;
									break;
								}
							}
						}
						if (!keepWhile) 
							break;
					}
					popVariableLevel();
					return e.atom;
				}
				else {
					throwError("the second argument is not a list.");
					return e.atom;
				}

			}
			}
			else if (function == "range") {
			if (e.args.size() == 3 || e.args.size() == 4) {
				e.atom.type = AtomType::LIST;
				if (e.args.size() == 4) {
					long j = ceil((double)(e.args[2].atom.v.lval - e.args[1].atom.v.lval) / e.args[3].atom.v.lval);
					if (j < 0) {
						throwError("Not correct arguments for " + function);
						return e.atom;
					}
					for (long i = e.args[1].atom.v.lval; j > 0; i += e.args[3].atom.v.lval) {
						pushBackLong(e.atom, i);
						--j;
					}
				}
				else {
					long j = (e.args[2].atom.v.lval - e.args[1].atom.v.lval);
					if (j < 0) {
						throwError("Not correct arguments for " + function);
						return e.atom;
					}
					for (long i = e.args[1].atom.v.lval; j > 0; i += 1) {
						pushBackLong(e.atom, i);
						--j;
					}
				}
				return e.atom;
			}
			else {
				throwError("Not correct numbers of arguments for " + function);
			}
			}
			// sin cos tan
			else if (function == "sin") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = sin(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "cos") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = cos(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "tan") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = tan(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "asin") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = asin(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "acos") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = acos(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "atan") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = atan(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			// sinh cosh tanh
			else if (function == "sinh") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = sinh(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "cosh") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = cosh(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "tanh") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = tanh(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "asinh") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = asinh(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "acosh") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = acosh(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "atanh") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = atanh(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			// math functions
			else if (function == "sqrt") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = sqrt(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "exp") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = exp(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "pow") {
				if (e.args.size() == 3) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = pow(toDouble(atom2V(doExp(e.args[1]))), toDouble(doExp(e.args[2])));
					return e.atom;
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
			}
			else if (function == "log") {
				e.atom.type = AtomType::VALUE;
				e.atom.vType = AtomValueType::DOUBLE;
				if (e.args.size() == 3) {
					e.atom.v.dval = log(toDouble(atom2V(doExp(e.args[1]))))/log(toDouble(doExp(e.args[2])));
				}
				else if (e.args.size() == 2) {
					e.atom.v.dval = log(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			// rand
			else if (function == "rand") {
				if (e.args.size() == 1) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = rand()/((double)RAND_MAX);
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "randInt") { //include begin and end
				if (e.args.size() == 3) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::LONG;
					const long rangeStart = atom2V(doExp(e.args[1])).v.lval;
					const long rangeLong = atom2V(doExp(e.args[2])).v.lval - rangeStart;
					e.atom.v.lval = rangeStart + (long)(((double)rangeLong) * (rand() / ((double)RAND_MAX)) + 0.5);
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			// round floor ceil
			else if (function == "round") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::LONG;
					e.atom.v.lval = (long)(toDouble(atom2V(doExp(e.args[1]))) + 0.5);
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "ceil") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::LONG;
					e.atom.v.lval = ceil(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "floor") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::LONG;
					e.atom.v.lval = floor(toDouble(atom2V(doExp(e.args[1]))));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			// bool long double string
			else if (function == "bool") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::BOOL;
					e.atom.v.bval = isTrue(doExp(e.args[1]));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "long") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::LONG;
					e.atom.v.lval = toLong(atom2V(doExp(e.args[1])));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "double") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::DOUBLE;
					e.atom.v.dval = toDouble(atom2V(doExp(e.args[1])));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "string") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::STRING;
					e.atom.sval = toString(atom2V(doExp(e.args[1])));
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "progn") {
				if (e.args.size() >= 1) {
					for (size_t i = 1; i < e.args.size() - 1; ++i) {
						doExp(e.args[i]);
					}
					e.atom = doExp(e.args[e.args.size() - 1]);
					return e.atom;
				}
				else {
					throwError("Not enough arguments for " + function);
				}
			}
			else if (function == "open") {
				if (e.args.size() == 2 || e.args.size() == 3) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::POINTER;
					const string filePath = toString(doExp(e.args[1]));
					e.atom.v.ptr = static_cast<void*>(new fstream());
					e.atom.vclass = ACTIONLISP_CLASS_FSTREAM;
					static_cast<fstream*>(e.atom.v.ptr)->open(filePath, e.args.size() == 2 ? ios::in: fileTypeRead(toString(doExp(e.args[2]))));
					return e.atom;
				}else {
					throwError("Not enough arguments for " + function);
					return e.atom;
				}
			}
			else if (function == "close") {
				if (e.args.size() == 2) {
					static_cast<fstream*>(atom2V(doExp(e.args[1])).v.ptr)->close();
				}
				else {
					throwError("Not enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "read") {
				if (e.args.size() == 2) {
					stringstream s; 
					s << (static_cast<fstream*>(atom2V(doExp(e.args[1])).v.ptr)->rdbuf());
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::STRING;
					e.atom.sval = s.str();
				}
				else {
					throwError("Not enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "seekRead") {
				if (e.args.size() == 3) {
					static_cast<fstream*>(atom2V(doExp(e.args[1])).v.ptr)->seekg(atom2V(doExp(e.args[2])).v.lval);
				}
				else {
					throwError("Not enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "seekWrite") {
				if (e.args.size() == 3) {
					static_cast<fstream*>(atom2V(doExp(e.args[1])).v.ptr)->seekp(atom2V(doExp(e.args[2])).v.lval);
				}
				else {
					throwError("Not enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "tellRead") {
				if (e.args.size() == 2) {
					e.atom.vType = AtomValueType::LONG;
					e.atom.v.lval=static_cast<fstream*>(atom2V(doExp(e.args[1])).v.ptr)->tellg();
				}
				else {
					throwError("Not enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "tellWrite") {
				if (e.args.size() == 2) {
					e.atom.vType = AtomValueType::LONG;
					e.atom.v.lval = static_cast<fstream*>(atom2V(doExp(e.args[1])).v.ptr)->tellp();
				}
				else {
					throwError("Not enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "getLine") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::STRING;
					const auto& res = getline(*static_cast<fstream*>(atom2V(doExp(e.args[1])).v.ptr), e.atom.sval);
					if (!res) {
						throwError(function + " failed.");
						e.atom.sval = "";
					}
				}
				else {
					throwError("Not enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "write") {
				if (e.args.size() == 3) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::STRING;
					const string writeString = atom2V(doExp(e.args[2])).sval;
					const auto& res = *static_cast<fstream*>(atom2V(doExp(e.args[1])).v.ptr) << writeString;
					if (!res) {
						throwError(function + " failed.");
						e.atom.sval = "";
					}
				}
				else {
					throwError("Not enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "writeLine") {
				if (e.args.size() == 3) {
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::STRING;
					const string writeString = atom2V(doExp(e.args[2])).sval + '\n';
					const auto& res = *static_cast<fstream*>(atom2V(doExp(e.args[1])).v.ptr) << writeString;
					if (!res) {
						throwError(function + " failed.");
						e.atom.sval = "";
					}
				}
				else {
					throwError("Not enough arguments for " + function);
				}
				return e.atom;
			}
			//vector
			else if (function == "ref") {
				if (e.args.size() == 3) {
					long n = atom2V(doExp(e.args[2])).v.lval;
					auto& l = doExp(e.args[1]).list;
					if (n < 0)
						n += l.size();
					return l[n];
				}
				else {
					throwError("Not enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "sub") {
				if (e.args.size() >= 3) {
					e.atom.type = AtomType::LIST;
					const auto& l = atom2V(doExp(e.args[1])).list;
					const size_t llen = l.size();
					const size_t p1 = atom2V(doExp(e.args[2])).v.lval % llen;
					const size_t p2 = e.args.size() == 4 ? (atom2V(doExp(e.args[3])).v.lval % llen) : llen - 1;
					const auto start = l.cbegin() + p1;
					const auto end = l.cbegin() + p2 + 1;
					e.atom.list = vector<Atom>(p2 - p1 + 1);
					copy(start, end, e.atom.list.begin());
					return e.atom;
				}
				else {
					throwError("Not enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "append") {
				if (e.args.size() >= 3) {
					e.atom.type = AtomType::LIST;
					e.atom.list.clear();
					for (size_t i = 1; i < e.args.size(); ++i) {
						const auto& l = atom2V(doExp(e.args[i])).list;
						e.atom.list.insert(e.atom.list.cend(), l.cbegin(), l.cend());
					}
					return e.atom;
				}
				else {
					throwError("Not enough arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "reverse") {
				if (e.args.size() == 2) {
					Atom& p = doExp(e.args[1]);
					if (p.type == AtomType::SYMBOL) {
						Variable& v = findVarInAll(p.sval);
						reverse(v.atom.list.begin(), v.atom.list.end());
						return v.atom;
					}
					else if (p.type == AtomType::VARIABLE) {
						Variable& v = *static_cast<Variable*>(p.v.ptr);
						reverse(v.atom.list.begin(), v.atom.list.end());
						return v.atom;
					}
					else if (p.type == AtomType::LIST) {
						reverse(p.list.begin(), p.list.end());
						return p;
					}
					else {
						Atom t = atom2V(p);
						reverse(t.list.begin(), t.list.end());
						return t;
					}
				}
				else {
					throwError("Not correct number of arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "sort") {
				if (e.args.size() == 2) {
					Atom& p = doExp(e.args[1]);
					if (p.type == AtomType::SYMBOL || p.type == AtomType::VARIABLE) {
						Variable& v = atom2Variable(p);
						sort(v.atom.list.begin(), v.atom.list.end(), [](Atom a, Atom b) {return !bigger(a, b); });
						return v.atom;
					}
					else {
						e.atom = atom2V(p);
						sort(e.atom.list.begin(), e.atom.list.end(), [](Atom a, Atom b) {return !bigger(a, b); });
						return e.atom;
					}
				}
				else {
					throwError("Not correct number of arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "rsort") {
				if (e.args.size() == 2) {
					Atom& p = doExp(e.args[1]);
					if (p.type == AtomType::SYMBOL || p.type == AtomType::VARIABLE) {
						Variable& v = atom2Variable(p);
						sort(v.atom.list.begin(), v.atom.list.end(), [](Atom a, Atom b) {return bigger(a, b); });
						return v.atom;
					}
					else {
						e.atom = atom2V(p);
						sort(e.atom.list.begin(), e.atom.list.end(), [](Atom a, Atom b) {return bigger(a, b); });
						return e.atom;
					}
				}
				else {
					throwError("Not correct number of arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "shuffle") {
				if (e.args.size() == 2) {
					Atom& p = doExp(e.args[1]);
					if (p.type == AtomType::SYMBOL || p.type == AtomType::VARIABLE) {
						Variable& v = atom2Variable(p);
						shuffle(v.atom.list.begin(), v.atom.list.end(), default_random_engine(random_device()()));
						return v.atom;
					}
					else {
						e.atom = atom2V(p);
						shuffle(e.atom.list.begin(), e.atom.list.end(), default_random_engine(random_device()()));
						return e.atom;
					}
				}
				else {
					throwError("Not correct number of arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "reverseCopy") {
				if (e.args.size() == 2) {
					e.atom.type = AtomType::LIST;
					e.atom.list.clear();
					const Atom& p = atom2V(doExp(e.args[1]));
					e.atom.list.resize(p.list.size());
					reverse_copy(p.list.cbegin(),p.list.cend(), e.atom.list.begin());
				}
				else {
					throwError("Not correct number of arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "push") {
				if (e.args.size() >= 2) {
					e.atom = doExp(e.args[1]);
					if (e.atom.type == AtomType::SYMBOL || e.atom.type == AtomType::VARIABLE) {
						Variable& var = atom2Variable(e.atom);
						if (var.name != "") {
							for (size_t i = 2; i < e.args.size(); ++i)
								var.atom.list.push_back(atom2V(doExp(e.args[i])));
							return var.atom;
						}
						return e.atom;
					}
					else {
						if (e.atom.type == AtomType::LIST) {
							e.atom = atom2V(e.atom);
							for (size_t i = 2; i < e.args.size(); ++i) {
								e.atom.list.push_back(atom2V(doExp(e.args[i])));
							}
							return e.atom;
						}
						return e.atom;
					}
				}
				else {
					throwError("Not enough numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "pop") {
				if (e.args.size() >= 2) {
					e.atom = doExp(e.args[1]);
					if (e.atom.type == AtomType::SYMBOL || e.atom.type == AtomType::VARIABLE) {
						Variable& var = atom2Variable(e.atom);
						if (var.name != "") {
							if (e.args.size() >= 3) {
								var.atom.list.erase(var.atom.list.begin() + atom2V(doExp(e.args[2])).v.lval);
							}
							else {
								var.atom.list.pop_back();
							}
							return var.atom;
						}
					}
					else {
						if (e.atom.type == AtomType::LIST) {
							e.atom = atom2V(e.atom);
							if (e.args.size() >= 3) {
								e.atom.list.erase(e.atom.list.begin() + atom2V(doExp(e.args[2])).v.lval);
							}
							else {
								e.atom.list.pop_back();
							}
							return e.atom;
						}
					}
				}
				else {
					throwError("Not enough numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "del") {
				if (e.args.size() >= 2) {
					e.atom = doExp(e.args[1]);
					if (e.atom.type == AtomType::SYMBOL || e.atom.type == AtomType::VARIABLE) {
						Variable& var = atom2Variable(e.atom);
						if (var.name != "") {
							if (e.args.size() >= 3) {
								for (size_t j = 2; j < e.args.size(); ++j) {
									size_t i = 0;
									const Atom p = atom2V(doExp(e.args[j]));
									while (true) {
										if (i < var.atom.list.size()) {
											if (equal(var.atom.list[i], p)) {
												var.atom.list.erase(var.atom.list.begin() + i);
											}
											else {
												++i;
											}
										}
										else {
											break;
										}
									}
								}
								return var.atom;
							}
							else {
								var.atom.list.clear();
							}
							return var.atom;
						}
					}
					else {
						if (e.atom.type == AtomType::LIST) {
							if (e.args.size() >= 3) {
								for (size_t j = 2; j < e.args.size(); ++j) {
									size_t i = 0;
									const Atom p = atom2V(doExp(e.args[j]));
									while (true) {
										if (i < e.atom.list.size()) {
											if (equal(e.atom.list[i], p)) {
												e.atom.list.erase(e.atom.list.begin() + i);
											}
											else {
												++i;
											}
										}
										else {
											break;
										}
									}
								}
								return e.atom;
							}
							else {
								e.atom.list.clear();
							}
							return e.atom;
						}
					}
				}
				else {
					throwError("Not enough numbers arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "find") {
				if (e.args.size() == 3) {
					e.atom = doExp(e.args[1]);
					e.atom.vType = AtomValueType::LONG;
					e.atom.v.lval = -1;
					if (e.atom.type == AtomType::SYMBOL || e.atom.type == AtomType::VARIABLE) {
						Variable& var = atom2Variable(e.atom);
						e.atom.type = AtomType::VALUE;
						if (var.name != "") {
							const Atom p = atom2V(doExp(e.args[2]));
							for (size_t i = 0; i < var.atom.list.size(); ++i) {
								if (equal(var.atom.list[i], p)) {
									e.atom.v.lval = i;
									return e.atom;
								}
							}
							return e.atom;
						}
					}
					else {
						if (e.atom.type == AtomType::LIST) {
							e.atom.type = AtomType::VALUE;
							const Atom& p = atom2V(doExp(e.args[2]));
							for (size_t i = 0; i < e.atom.list.size(); ++i) {
								if (equal(e.atom.list[i], p)) {
									e.atom.v.lval = i;
									return e.atom;
								}
							}
							return e.atom;
						}
					}
				}
				else {
					throwError("Not correct number of arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "rfind") {
				if (e.args.size() == 3) {
					e.atom = doExp(e.args[1]);
					e.atom.vType = AtomValueType::LONG;
					e.atom.v.lval = -1;
					if (e.atom.type == AtomType::SYMBOL || e.atom.type == AtomType::VARIABLE) {
						Variable& var = atom2Variable(e.atom);
						e.atom.type = AtomType::VALUE;
						if (var.name != "") {
							const Atom p = atom2V(doExp(e.args[2]));
							for (int i = var.atom.list.size() - 1; i >= 0; --i) {
								if (equal(var.atom.list[i], p)) {
									e.atom.v.lval = i;
									return e.atom;
								}
							}
							return e.atom;
						}
					}
					else {
						if (e.atom.type == AtomType::LIST) {
							e.atom.type = AtomType::VALUE;
							const Atom p = atom2V(doExp(e.args[2]));
							for (int i = e.atom.list.size() - 1; i >= 0; --i) {
								if (equal(e.atom.list[i], p)) {
									e.atom.v.lval = i;
									return e.atom;
								}
							}
							return e.atom;
						}
					}
				}
				else {
					throwError("Not correct number of arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "split") {
				if (e.args.size() == 3) {
					Atom& p = doExp(e.args[1]);
					const char splitChar = atom2V(doExp(e.args[2])).sval[0];
                    Variable &v = atom2Variable(p);
					e.atom.type = AtomType::LIST;
					if (!varIsNull(v))
						p = v.atom;
					size_t cutIn = 0;
					for (size_t i = 0; i < p.sval.size(); ++i) {
						if (p.sval[i] == splitChar) {
							Atom t;
							t.sval = p.sval.substr(cutIn, i - cutIn);
							t.type = AtomType::VALUE;
							t.vType = AtomValueType::STRING;
							e.atom.list.push_back(t);
							cutIn = i+1;
						}
					}
					if (cutIn < p.sval.size()) {
						Atom t;
						t.sval = p.sval.substr(cutIn, p.sval.size() - cutIn);
						t.type = AtomType::VALUE;
						t.vType = AtomValueType::STRING;
						e.atom.list.push_back(t);
					}
				}
				else {
					throwError("Not correct number of arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "join") {
				if (e.args.size() == 3) {
					Atom& p = doExp(e.args[1]);
					const char splitChar = atom2V(doExp(e.args[2])).sval[0];
					Variable& v = atom2Variable(p);
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::STRING;
					if (!varIsNull(v))
						p = v.atom;
					e.atom.sval = p.list[0].sval;
					for (size_t i = 1; i < p.list.size(); ++i) {
						e.atom.sval = e.atom.sval + splitChar + p.list[i].sval;
					}
				}
				else if (e.args.size() == 2) {
					Atom& p = doExp(e.args[1]);
					Variable& v = atom2Variable(p);
					e.atom.type = AtomType::VALUE;
					e.atom.vType = AtomValueType::STRING;
					if (!varIsNull(v))
						p = v.atom;
					e.atom.sval = p.list[0].sval;
					for (size_t i = 1; i < p.list.size(); ++i) {
						e.atom.sval = e.atom.sval + p.list[i].sval;
					}
				}
				else {
					throwError("Not correct number of arguments for " + function);
				}
				return e.atom;
			}
			else if (function == "print") {
				for (int i = 1; i < e.args.size(); ++i) {
					cout << toString(atom2V(doExp(e.args[i])));
				}
				cout << endl;
				return e.atom;
			}
			else if (function == "gets") {
				if (e.args.size() == 2) {
					Variable& t = findVarInAll(doExp(e.args[1]).sval);
					t.atom.vType = AtomValueType::STRING;
					t.atom.type = AtomType::VALUE;
					cin >> t.atom.sval;
					e.atom = t.atom;
					return e.atom;
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}	
			}
			else if (function == "def") {
				if (e.args.size() >= 3) {
					const Atom& functionName = doExp(e.args[1]);
					if (functionName.type == AtomType::SYMBOL) {
						for (const auto& fun : functions) {
							if (functionName.sval == fun.name) {
								throwError("The function " + fun.name + " has been defined.");
								return e.atom;
							}
						}
						actionFun t;
						t.name = functionName.sval;
						if (e.args.size() >= 4) {
							const Atom& arguments = doExp(e.args[2]);
							if (arguments.type == AtomType::LIST) {
								for (size_t i = 0; i < arguments.list.size(); ++i) {
									Variable v;
									v.name = arguments.list[i].sval;
									t.input.push_back(v);
								}
							}
							else if(arguments.type == AtomType::SYMBOL) {
								Variable v;
								v.name = arguments.sval;
								t.input.push_back(v);
							}
							else {
								throwError("The arguments of def should be symbol.");
								return e.atom;
							}
							for (size_t i = 3; i < e.args.size(); ++i) {
								t.body.push_back(e.args[i]);
							}
							functions.push_back(t);
							return e.atom;
						}
						else {
							for (size_t i = 2; i < e.args.size(); ++i) {
								t.body.push_back(e.args[i]);
							}
							functions.push_back(t);
							return e.atom;
						}
						
					}
					else {
						throwError("The first argument of def should be a symbol.");
					}
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
			}
			else if (function == "return") {
				if (e.args.size() == 2) {
					e.atom = atom2V(doExp(e.args[1]));
					e.atom.flag.push_back(AtomFlag::RETURN);
				}
				else if (e.args.size() == 1) {
					e.atom.flag.push_back(AtomFlag::RETURN);
				}
				else {
					throwError("Not correct numbers arguments for " + function);
				}
				return e.atom;
			}
			else{
				for (const actionFun& constFun : functions) {
					if (constFun.name == function) {
						e.atom = doFunction(constFun, e.args);
						return e.atom;
					}
				}
				throwError("Unknown Function: " + function);
			}
			return e.atom;
		}
	}

	Expression readAtom2Exp(const string& str, const LPOS posStart, const LPOS posEnd) {
		Expression exp;
		exp.atom = readAtom(str, posStart, posEnd);
		return exp;
	}
	Atom readAtom(const string& str, const LPOS posStart=0, const LPOS posEnd=-1) {
		// This function will not read the symbol value
		Atom t;
		t.type = AtomType::VALUE;
		const string sstr = posEnd == -1 ? str.substr(posStart):str.substr(posStart, posEnd - posStart + 1);
		if (str[posStart] == '\"') {
			t.sval = posEnd == -1 ? str.substr(posStart + 1, str.size() - posStart - 2):str.substr(posStart + 1, posEnd - posStart - 1);
			t.vType = AtomValueType::STRING;
			return t;
		}
		else if (isdigit(str[posStart])) {
			const double dig = stod(sstr);
			if (long(dig) == dig) {
				t.vType = AtomValueType::LONG;
				t.v.lval = long(dig);
			}
			else {
				t.vType = AtomValueType::DOUBLE;
				t.v.dval = dig;
			}

			return t;
		}
		else {
			if (sstr == "true") {
				t.vType = AtomValueType::BOOL;
				t.v.bval = true;
			}
			else if (sstr == "false") {
				t.vType = AtomValueType::BOOL;
				t.v.bval = false;
			}
			else if (sstr == "pi") {
				t.vType = AtomValueType::DOUBLE;
				t.v.dval = 3.14159265358979323846;
			}
			else {
				t.type = AtomType::SYMBOL;
				t.sval = sstr;
			}
			return t;
		}
	}
	Atom readList(const string& str) {
		Atom t;
		LPOS posArg = nowPos;
		t.type = AtomType::LIST;
		ExpressionState state = ExpressionState::IDLE;
		for (;;) {
			if (nowPos >= str.size())
				return t;
			const char s = str[nowPos];
			switch (state) {
			case ExpressionState::IDLE: {
				if (s != ' ' && s != '\n') {
					state = ExpressionState::EXPRESSION;
					posArg = nowPos;
					//DONT'T USE BREAK
				}
				else {
					++nowPos;
					break;
				}
			}
			case ExpressionState::EXPRESSION: {
				switch (s) {
				case '(': {
					++nowPos; 
					Expression exp = readExpression(str);
					t.list.push_back(doExp(exp));
					switch (str[nowPos]) {
					case '}': {
						++nowPos;
						return t;
					}
					case ' ':
					case '\n': {
						++nowPos;
						posArg = nowPos;
						state = ExpressionState::IDLE;
						break;
					}
					}
					break;
				}
				case ' ':
				case '\n': {
					t.list.push_back(readAtom(str, posArg, nowPos - 1));
					++nowPos;
					posArg = nowPos;
					state = ExpressionState::IDLE;
					break;
				}
				case '{': {
					t.list.push_back(readList(str));
					state = ExpressionState::IDLE;
					++nowPos;
					posArg = nowPos;
					break;
				}
				case '}': {
					if (posArg != nowPos) {
						t.list.push_back(readAtom(str, posArg, nowPos - 1));
						++nowPos;
						return t;
					}
					else {
						++nowPos;
						return t;
					}
				}
				default: {
					++nowPos;
					break;
				}
				}
			}
			}
		}
		return t;
	}
	double static toDouble(const Atom& a) {
		if (a.type == AtomType::VALUE) {
			switch (a.vType) {
			case AtomValueType::BOOL: {
				return a.v.bval ? 1.0 : 0.0;
			}
			case AtomValueType::DOUBLE: {
				return a.v.dval;
			}
			case AtomValueType::LONG: {
				return (double)a.v.lval;
			}
			case AtomValueType::STRING: {
				//throwError("Cannot turn string to double.");
				return 0.0;
			}
			}
		}
		else {
			//throwError("cannot transform symbol to double in toDouble()");
		}
		return 0.0;
	}
	long static toLong(const Atom& a) {
		if (a.type == AtomType::VALUE) {
			switch (a.vType) {
				case AtomValueType::BOOL: {
					return a.v.bval ? 1 : 0;
				}
				case AtomValueType::DOUBLE: {
					return (long)a.v.dval;
				}
				case AtomValueType::LONG: {
					return a.v.lval;
				}
				case AtomValueType::STRING: {
					//throwError("Cannot turn string to long.");
					return 0;
				}
			}
		}
		else {
			//throwError("cannot transform symbol to long in toLong()");
		}
		return 0;
	}
	bool static isTrue(const Atom& a) {
		if (a.type == AtomType::VALUE){
			switch (a.vType) {
			case AtomValueType::BOOL: return a.v.bval;
			case AtomValueType::LONG: return a.v.lval != 0;
			case AtomValueType::DOUBLE: return a.v.dval != 0.0;
			case AtomValueType::STRING: return a.sval != "";
			}
		}
		return false;
	}
	bool static bigger(const Atom& a, const Atom& b) {//only compare atom with VALUE type
		if (a.type == AtomType::VALUE && b.type == AtomType::VALUE) {
			if ((a.vType == AtomValueType::STRING) && (b.vType == AtomValueType::STRING)) {
				return a.sval > b.sval;
			}
			else if ((a.vType == AtomValueType::STRING) != (b.vType == AtomValueType::STRING)) {
				//throwError("Cannot do compare operation between STRING and other type");
				return false;
			}
			else {
				if (a.vType == b.vType) {
					switch (a.vType) {
					case AtomValueType::BOOL: return a.v.bval > b.v.bval;
					case AtomValueType::LONG: return a.v.lval > b.v.lval;
					case AtomValueType::DOUBLE: return a.v.dval > b.v.dval;
					}
				}
				else {
					return (toDouble(a) > toDouble(b));
				}
			}
		}
		else if (a.type == AtomType::LIST && b.type == AtomType::LIST) {
			const size_t listMinSize = (a.list.size() < b.list.size()) ? a.list.size() : b.list.size();
			for (size_t i = 0; i < listMinSize; ++i) {
				if (!bigger(a.list[i], b.list[i]))
					return false;
			}
			return true;
		}
		else {
			//throwError("Cannot do compare operation on SYMBOL/FUNCTION/VARIABLE");
			return false;
		}
		return false;
	}
	bool equal(const Atom& a, const Atom& b) { //only compare atom with VALUE and LIST type
		if (a.type == AtomType::VALUE && b.type == AtomType::VALUE) {
			if ((a.vType == AtomValueType::STRING) != (b.vType == AtomValueType::STRING)) {
				return false;
			}
			else {
				if (a.vType == b.vType) {
					switch (a.vType) {
						case AtomValueType::BOOL: return a.v.bval == b.v.bval;
						case AtomValueType::LONG: return a.v.lval == b.v.lval;
						case AtomValueType::DOUBLE: return a.v.dval == b.v.dval;
						case AtomValueType::STRING: return a.sval == b.sval;
					}
				}
				else {
					return (toDouble(a) == toDouble(b));
				}
			}
		}
		else if (a.type == AtomType::LIST && b.type == AtomType::LIST) {
			if (a.list.size() != b.list.size())
				return false;
			for (size_t i = 0; i < a.list.size(); ++i) {
				if (!equal(a.list[i], b.list[i]))
					return false;
			}
			return true;
		}
		else {
			throwError("Cannot do equal operation on SYMBOL");
			return false;
		}
		return false;
	}
	inline static Expression atom2Exp(const Atom a) {
		Expression t;
		t.atom = a;
		return t;
	}
	Expression readExpression(const string& str) {  // will set nowPos to pos[)]+1;
		LPOS posArg = nowPos;
		ExpressionState state = ExpressionState::IDLE;
		Expression t;
		bool nowInString = false;
		for (;;) {
			//cout << nowPos << ": " << str[nowPos] << endl;
			if (nowPos >= str.size())
				return t;
			const char s = str[nowPos];
			switch (state) {
			case ExpressionState::IDLE: {
				if (s != ' ' && s != '\n') {
					if(s != ';'){
						state = ExpressionState::EXPRESSION;
						posArg = nowPos;
						//DONT'T USE BREAK
					}
					else if (s == '#') {
						if (nowPos < str.size() - 1) {
							if (str[nowPos + 1] == '|') {
								state = ExpressionState::MULTILINE_COMMENT;
								nowPos += 2;
							}
							else {
								++nowPos;
							}
						}
						else {
							++nowPos;
						}
						break;
					}
					else {
						state = ExpressionState::COMMENT;
						++nowPos;
						break;
					}
				}
				else {
					++nowPos;
					break;
				}
			}
			case ExpressionState::EXPRESSION: {
				switch (s) {
				case '\"': {
					nowInString = !nowInString;
					++nowPos;
					break;
				}
				case ';': {
					if (!nowInString) {
						state = ExpressionState::COMMENT;
						++nowPos;
						break;
					}
				}
				case '#': {
					if (!nowInString) {
						if (nowPos < str.size() - 1) {
							if (str[nowPos + 1] == '|') {
								state = ExpressionState::MULTILINE_COMMENT;
								nowPos += 2;
							}
							else {
								++nowPos;
							}
						}else {
							++nowPos;
						}
					}
					else {
						++nowPos;
					}
					break;
				}
				case '(': {
					++nowPos;
					t.args.push_back(readExpression(str));
					switch (str[nowPos]) {
						case ')': {
							++nowPos;
							return t;
						}
						case ' ': 
						case '\n':{
							++nowPos;
							posArg = nowPos;
							state = ExpressionState::IDLE;
							break;
						}
						case '(': {
							posArg = nowPos + 1;
							break;
						}
						case ';': {
							state = ExpressionState::COMMENT;
							++nowPos;
							break;
						}
						case '#': {
							if (nowPos < str.size() - 1) {
								if (str[nowPos + 1] == '|') {
									state = ExpressionState::MULTILINE_COMMENT;
									nowPos+=2;
								}
							}
							else {
								++nowPos;
							}
							break;
						}
					}
					break;
				}
				case ')': {
					t.args.push_back(readAtom2Exp(str, posArg, nowPos - 1));
					++nowPos;
					return t;
				}
				case ' ':
				case '\n':
				{
					if (!nowInString) {
						t.args.push_back(readAtom2Exp(str, posArg, nowPos - 1));
						posArg = nowPos + 1;
						state = ExpressionState::IDLE;
					}
					++nowPos;
					break;
				}
				case '{': {
					++nowPos;
					t.args.push_back(atom2Exp(readList(str)));
					posArg = nowPos + 1;
					if (str[nowPos] == ')') {
						++nowPos;
						return t;
					}
					state = ExpressionState::IDLE;
					break;
				}
				default: {
					++nowPos;
					break;
				}
				}
				break;
			}
			case ExpressionState::COMMENT: {
				switch (s) {
				case '\n': {
					state = ExpressionState::IDLE;
					nowPos++;
					break;
				}
				default: {
					nowPos++;
					break;
				}
				}
			}
			case ExpressionState::MULTILINE_COMMENT: {
				switch (s) {
					case '|': {
						if (nowPos < str.size() - 1) {
							if (str[nowPos + 1] == '#') {
								state = ExpressionState::IDLE;
								nowPos+=2;
							}
						}
						else {
							++nowPos;
						}
						break;
					}
					default: {
						++nowPos;
						break;
					}
				}
			}
			}
		}
		return t;
	}
	void readString(const string& str) {
		nowPos = 0;
		nowLine = 1;
		for (;;) {
			if (nowPos >= str.size())
				break;
			switch (state) {
			case ExpressionState::IDLE: {
				switch (str[nowPos]) {
				case '(':state = ExpressionState::EXPRESSION; break;
				case ' ':break;
				case '\n':++nowLine; break;
				case ';':state = ExpressionState::COMMENT; break;
				case '#': {
					if (nowPos < str.size() - 1) {
						if (str[nowPos + 1] == '|') {
							state = ExpressionState::MULTILINE_COMMENT;
							++nowPos;
						}
					}
					break;
				}
				default: {
					throwError("should start with ( instead of " + to_string(str[nowPos]));
					break;
				}
				}
				++nowPos;
				break;
			}
			case ExpressionState::EXPRESSION: {
				Expression exp = readExpression(str);
				doExp(exp);
				state = ExpressionState::IDLE;
				break;
			}
			case ExpressionState::COMMENT: {
				if (str[nowPos] == '\n') {
					state = ExpressionState::IDLE;
					++nowLine;
				}
				++nowPos;
				break;
			}
			case ExpressionState::MULTILINE_COMMENT: {
				switch (str[nowPos]) {
					case '|': {
						if (nowPos < str.size() - 1) {
							if (str[nowPos + 1] == '#') {
								state = ExpressionState::IDLE;
								nowPos+=2;
							}
						}
						else {
							++nowPos;
						}
						break;
					}
					case '\n': {
						++nowLine;
						++nowPos;
						break;
					}
					default: {
						++nowPos;
						break;
					}
				}
			}
			default: break;
			}
		}
	}
	int bracketCount(const string& line) {
		bool inString = false;  //false:Normal true:in " "
		int bracket = 0;
		for (size_t i = 0; i < line.size(); ++i) {
			if (!inString) {
				switch (line[i]) {
					case '(': {
						bracket++; break;
					}
					case ')': {
						bracket--; break;
					}
					case '\"': {
						inString = true; break;
					}
				}
			}
			else {
				switch (line[i]) {
					case '\"': {
						inString = false; break;
					}
				}
				break;
			}
		}
		return bracket;
		
	}
public:
	Interpreter(){};
	~Interpreter() {
		while (variables.size() > 0) {
			popVariableLevel();
		}
	};
	void readFile(const string& fileName) {
		REPLMode = false;
		nowPos = 0;
		nowLine = 1;
		fstream ifs(fileName);
		if (!ifs) {
			throwError("Cannot read file " + fileName);
		}
		string line;
		string result; 
		while (getline(ifs, line))
			result += line + '\n';
		ifs.close();
		readString(result);
	}
	void waitForInput() {
		REPLMode = true;
		int bracket = 0;
		string str;
		string line;
		cout << ">>";
		while (getline(cin, line)) {
			bracket += bracketCount(line);
			if (bracket > 0) {
				str += line + '\n';
				cout << "    ";
				continue;
			}
			else if (bracket == 0) {
				str += line;
				readString(str);
				str = "";
				cout << ">>";
			}
			else {
				throwError("Too many right brackets.");
				bracket = 0;
			}
		}
	}
};



const array<string, 2> Interpreter::keyWords = { "break","continue" };
const string Interpreter::SWITCH_VAR_NAME = "ACTIONLISP_SWITCH_VARIABLE";
const string Interpreter::ACTIONLISP_CLASS_FSTREAM = "ACTIONLISP_CLASS_FSTREAM";