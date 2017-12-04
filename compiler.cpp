
#include <iostream>
#include <algorithm>
#include <string>
#include <fstream>
#include <vector>
#include <stack>

using namespace std;



// Abstract Syntax Tree
class AST
{
	string _value;
	AST* _left; // can be null
	AST* _right; // can be null

public:

	AST(string value, AST* right, AST* left) {
		_value = value;
		_left = left;
		_right = right;
	}

	//getters
	AST* getRight() {
		return this->_right;
	}

	AST* getLeft() {
		return this->_left;
	}

	string getVal() {
		return this->_value;
	}

	//create tree
	static AST* createAST(ifstream& input) {
		if (!(input))
			return nullptr;

		string line;
		getline(input, line);
		if (line == "~")
			return nullptr;

        AST* left = createAST(input);
        AST* right = createAST(input);
		return new AST(line, right, left);
	}
};

class Range {
public:
    int from;
    int to;

    Range(int _from, int _to) {
        from = _from;
        to = _to;
    }
};
//variable class
class Variable {
public:
	string name;
	string type;
	int address;
	int size = -1;

	//pointer support
	string pointerTo;

	//array support
    vector<Range> arrayRanges;
    int subPart = 0;
    int g = 0;

    //record support
    int numSubData = 0;
};





//symbol table
class SymbolTable {
private:
	//current address free
	int currentAdress = 5;
	vector<Variable> varsSymboleTable; // vectore of all current variables

public:

    static SymbolTable* returnSymbolTable() {
        static SymbolTable* allSymbolsTable;
        if(allSymbolsTable == nullptr) {
            allSymbolsTable = new SymbolTable();
	    }

	    return allSymbolsTable;
    }

    //return vaaiable
    Variable* returnVar(string name) {
		for (int i = 0; i < varsSymboleTable.size(); i++) {
			if (varsSymboleTable[i].name == name)
				return &(varsSymboleTable[i]);
		}

		return nullptr;
	}

	//return address of var in import table, if doesent exists return -1
    int returnVarAddress(string name) {
		for (int i = 0; i < varsSymboleTable.size(); i++) {
			if (varsSymboleTable[i].name == name)
				return varsSymboleTable[i].address;
		}

		return -1;
	}

	static void addRangesToArray(Variable& currrentVar,AST* rangeList) {
        if (rangeList == nullptr)
            return;

        SymbolTable::addRangesToArray(currrentVar,rangeList->getLeft());


        if(rangeList->getVal() == "range") {
            Range current(stoi(rangeList->getLeft()->getLeft()->getVal()),stoi(rangeList->getRight()->getLeft()->getVal()));
            currrentVar.arrayRanges.push_back(current);
        }

        if((rangeList->getRight() != nullptr && rangeList->getRight()->getVal() == "range")) {
            SymbolTable::addRangesToArray(currrentVar,rangeList->getRight());
        }

	}
    //starts from d1,d2,d3..
	static int returnD(int dNum,Variable currArray) {
        return currArray.arrayRanges[dNum-1].to - currArray.arrayRanges[dNum-1].from +1;
	}

	static int returnDMulFrom(int fromD,Variable currArray ) {
	    int mul = 1;
	    for(int i = fromD-1;i<currArray.arrayRanges.size();i++) {
            mul *= SymbolTable::returnD(i + 1,currArray);
	    }

        return mul;
	}

	static int returnSubPart(Variable currArray) {
        int subPart = 0;

        for(int i = 0;i<currArray.arrayRanges.size();i++) {
            subPart += currArray.arrayRanges[i].from * SymbolTable::returnDMulFrom(i+2,currArray);
	    }
	    subPart *= currArray.g;

	    return subPart;
	}

    int returnRecordOffeset(string name) {
        int offset = 0;
        bool found = false;

        for(int i = this->varsSymboleTable.size()-1;i>=0;i--) {
            if(found == true && this->varsSymboleTable[i].type == "record")
                break;

            if(found)
                offset += this->varsSymboleTable[i].size;

            if(this->varsSymboleTable[i].name == name)
                found = true;
	    }

	    return offset;
	}

	int returnSizeOf(string name) {
	    for (int i = 0; i < varsSymboleTable.size() ;i++) {
			Variable currentVar = varsSymboleTable[i];
			if(currentVar.name == name)
                return currentVar.size;
		}

		return 1;
	}


    int sumSizes(int from, int to) {

        int sum =0;
          for (int i = from; i <= to ;i++) {
			Variable currentVar = varsSymboleTable[i];
			sum += currentVar.size;
			to -= currentVar.numSubData;
		}

		return sum;
    }

    int returnNumChilren(AST* node) {
        if (node == nullptr)
            return 0;

        if(node->getVal() == "declarationsList")
            return 1 + this->returnNumChilren(node->getLeft()) +  this->returnNumChilren(node->getRight());

        return this->returnNumChilren(node->getLeft()) +  this->returnNumChilren(node->getRight());
    }

    static void generateSymbolTableHelper(AST* tree) {
        SymbolTable* generalSymbolTabel = returnSymbolTable();
        //cout << "current tree : -" << tree->getVal() << "- good" << endl;

		//if reached the end
		if (tree == nullptr) {
			return;
		}

		//if first node
		if (tree->getVal() == "program") {
			return generateSymbolTableHelper(tree->getRight());
		}


		generateSymbolTableHelper(tree->getLeft());

		//check if declartion list
		if (tree->getVal() == "declarationsList") {
            string currentType = tree->getRight()->getRight()->getVal();
            Variable currentVar;

            //default data
            currentVar.address = 0;
            currentVar.name = tree->getRight()->getLeft()->getLeft()->getVal();
            currentVar.type = currentType;

            if(currentType == "array") {
                //add coordinates
                SymbolTable::addRangesToArray(currentVar,tree->getRight()->getRight()->getLeft());

                if(tree->getRight()->getRight()->getRight()->getVal() == "identifier") {
                    currentVar.g = generalSymbolTabel->returnSizeOf(tree->getRight()->getRight()->getRight()->getLeft()->getVal());
                    currentVar.pointerTo = tree->getRight()->getRight()->getRight()->getLeft()->getVal();
                }else
                    currentVar.g = 1;

                currentVar.subPart = generalSymbolTabel->returnSubPart(currentVar);
                currentVar.size = generalSymbolTabel->returnDMulFrom(1,currentVar) * currentVar.g; // d0*d1*****g
                generalSymbolTabel->varsSymboleTable.push_back(currentVar);

            } else if (currentType == "record") {

                generalSymbolTabel->varsSymboleTable.push_back(currentVar);
                int currentIndex = generalSymbolTabel->varsSymboleTable.size();

                generateSymbolTableHelper(tree->getRight()->getRight()->getLeft());
                generalSymbolTabel->varsSymboleTable[currentIndex-1].numSubData = generalSymbolTabel->returnNumChilren(tree->getRight()->getRight()->getLeft());
                generalSymbolTabel->varsSymboleTable[currentIndex-1].size = generalSymbolTabel->sumSizes((currentIndex),currentIndex+generalSymbolTabel->varsSymboleTable[currentIndex-1].numSubData-1);

            }else if (currentType == "pointer") {
                currentVar.pointerTo = tree->getRight()->getRight()->getLeft()->getLeft()->getVal();
                currentVar.size = 1;
                generalSymbolTabel->varsSymboleTable.push_back(currentVar);
            }else {
                currentVar.size = 1;
                generalSymbolTabel->varsSymboleTable.push_back(currentVar);
            }



		}


    }



	//create the symbol table
	static SymbolTable generateSymbolTable(AST* tree) {

        generateSymbolTableHelper(tree);

        SymbolTable* generalSymbolTabel = SymbolTable::returnSymbolTable();
        generalSymbolTabel->currentAdress = 5;



        //set address
        //set addresses
        for (int i = 0; i < generalSymbolTabel->varsSymboleTable.size(); i++) {
            int addAddress;
            if( generalSymbolTabel->varsSymboleTable[i].type == "record")
                addAddress = 0;
            else
                addAddress = generalSymbolTabel->varsSymboleTable[i].size;

            generalSymbolTabel->varsSymboleTable[i].address = generalSymbolTabel->currentAdress;
            generalSymbolTabel->currentAdress += addAddress;
        }



        return *generalSymbolTabel;

	}

	//print data
	 void print() {
		cout << endl << "Symbol Table: " << endl;
		for (int i = 0; i < varsSymboleTable.size() ;i++) {
			Variable currentVar = varsSymboleTable[i];
			cout << "name:" <<currentVar.name << " type:" << currentVar.type << " size:" << currentVar.size << " address:" << currentVar.address << " sub : " << currentVar.numSubData ;
			if(currentVar.type == "array") {
                cout << "g:" << currentVar.g << " , subpart:" << currentVar.subPart ;

			}
			cout << endl;

		}

		 cout << endl;
	}
};

//methods for genereate p code
void codeL(AST* ast, SymbolTable symbolTable);
void codeR(AST* ast, SymbolTable symbolTable);
void codeM(AST* ast, SymbolTable SymbolTable);
int codeI(AST* ast,Variable* currentVar, SymbolTable symbolTable);
void code(AST* ast, SymbolTable symbolTable,string currentEndLabel,string cameFrom);
void generatePCode(AST* ast, SymbolTable symbolTable);
string returnType(AST* ast, SymbolTable symbolTable);

int currentLabel = 0;

//returns current sub tree in actual value
void codeR(AST* ast, SymbolTable symbolTable) {
	if (ast->getVal() == "constInt" || ast->getVal() == "constReal") {
		cout << "ldc " << ast->getLeft()->getVal() << endl;
	}
	else if(ast->getVal() == "array") {
        codeL(ast,symbolTable);
        cout << "ind" << endl;
	}
	else if (ast->getVal() == "plus") {
		codeR(ast->getLeft(),symbolTable);
		codeR(ast->getRight(), symbolTable);
		cout << "add" << endl;
	}
	else if (ast->getVal() == "minus") {
		codeR(ast->getLeft(), symbolTable);
		codeR(ast->getRight(), symbolTable);
		cout << "sub" << endl;
	}
	else if (ast->getVal() == "identifier") {
		codeL(ast,symbolTable);
		cout << "ind" << endl;
	}
	else if (ast->getVal() == "record") {
        codeL(ast,symbolTable);
        cout << "ind" << endl;
    }
	else if (ast->getVal() == "true") {
		cout << "ldc 1" << endl;
	}
	else if (ast->getVal() == "false") {
		cout << "ldc 0" << endl;
	}
	else if (ast->getVal() == "multiply") {
		codeR(ast->getLeft(), symbolTable);
		codeR(ast->getRight(), symbolTable);
		cout << "mul" << endl;
	}
	else if (ast->getVal() == "divide") {
		codeR(ast->getLeft(), symbolTable);
		codeR(ast->getRight(), symbolTable);
		cout << "div" << endl;
	}
	else if (ast->getVal() == "lessThan") {
		codeR(ast->getLeft(), symbolTable);
		codeR(ast->getRight(), symbolTable);
		cout << "les" << endl;
	}
	else if (ast->getVal() == "greaterThan") {
		codeR(ast->getLeft(), symbolTable);
		codeR(ast->getRight(), symbolTable);
		cout << "grt" << endl;
	}
	else if (ast->getVal() == "or") {
		codeR(ast->getLeft(), symbolTable);
		codeR(ast->getRight(), symbolTable);
		cout << "or" << endl;
	}
	else if (ast->getVal() == "and") {
		codeR(ast->getLeft(), symbolTable);
		codeR(ast->getRight(), symbolTable);
		cout << "and" << endl;
	}
	else if (ast->getVal() == "equals") {
		codeR(ast->getLeft(), symbolTable);
		codeR(ast->getRight(), symbolTable);
		cout << "equ" << endl;
	}
	else if (ast->getVal() == "lessOrEquals") {
		codeR(ast->getLeft(), symbolTable);
		codeR(ast->getRight(), symbolTable);
		cout << "leq" << endl;
	}
	else if (ast->getVal() == "greaterOrEquals") {
		codeR(ast->getLeft(), symbolTable);
		codeR(ast->getRight(), symbolTable);
		cout << "geq" << endl;
	}
	else if (ast->getVal() == "notEquals") {
		codeR(ast->getLeft(), symbolTable);
		codeR(ast->getRight(), symbolTable);
		cout << "neq" << endl;
	}
	else if (ast->getVal() == "not") {
		codeR(ast->getLeft(), symbolTable);
		cout << "not" << endl;
	}
	else if (ast->getVal() == "negative") {
		codeR(ast->getLeft(), symbolTable);
		cout << "neg" << endl;
	}
	else if (ast->getVal() == "pointer") {
		codeL(ast, symbolTable);
		cout << "ind " << endl;
	}

}


void codeM(AST* ast, SymbolTable SymbolTable) {
    if (ast->getVal() == "pointer") {
		cout << "ind " << endl;
	}

	if (ast->getVal() == "record") {
        codeL(ast->getLeft(),SymbolTable);

		int getOffset = SymbolTable.returnRecordOffeset(ast->getRight()->getLeft()->getVal());
		cout << "inc " << getOffset << endl;
	}

}

int codeI(AST* ast,Variable* currentVar, SymbolTable symbolTable) {
    if (ast == nullptr)
        return 1;

   int startFrom = codeI(ast->getLeft(),currentVar,symbolTable);

   if(ast->getRight() != nullptr) {
        codeR(ast->getRight(),symbolTable);

        cout << "ixa " << symbolTable.returnDMulFrom(startFrom+1,*currentVar)*(currentVar->g) << endl;

        return startFrom + 1;
    }


}

string returnType(AST* ast, SymbolTable symbolTable){
    if(ast == nullptr)
        return nullptr;

    if(ast->getVal() == "identifier") {
	    return ast->getLeft()->getVal();
    }
    if(ast->getVal() == "pointer") {
        Variable* currentVar = symbolTable.returnVar(returnType(ast->getLeft(),symbolTable));

	    return currentVar->pointerTo;
    }
    if(ast->getVal() == "record") {
	    return returnType(ast->getRight(),symbolTable);
    }
    if(ast->getVal() == "array"){
        Variable* currentVar = symbolTable.returnVar(returnType(ast->getLeft(),symbolTable));
        return  currentVar->pointerTo;
    }
}

//return current sub tree in adress
void codeL(AST* ast, SymbolTable symbolTable) {
	if (ast->getVal() == "identifier") {
		int getAddress = symbolTable.returnVarAddress(ast->getLeft()->getVal());
		cout << "ldc " << getAddress << endl;
	} else if (ast->getVal() == "array") {
	    Variable* currentVar = symbolTable.returnVar(returnType(ast->getLeft(),symbolTable));

        codeL(ast->getLeft(),symbolTable);

        codeI(ast->getRight(),currentVar, symbolTable);
        cout << "dec " <<currentVar->subPart << endl;
	}else if (ast->getVal() == "record") {
        codeM(ast,symbolTable);
	}
	else if (ast->getVal() == "pointer") {
		codeL(ast->getLeft(), symbolTable);
		cout << "ind " << endl;
	}
	else {
        codeL(ast->getLeft(),symbolTable);
        codeM(ast,symbolTable);
	}
}

void codec(AST* ast,int endOfLoop, stack<int>& endOfLabels, SymbolTable symbolTable) {
    if(ast == nullptr)
        return;

    codec(ast->getLeft(),endOfLoop,endOfLabels,symbolTable);

    if(ast->getVal() == "case") {

        int L0 = stoi(ast->getLeft()->getLeft()->getVal());
		endOfLabels.push(L0);

        cout << "case_"<<endOfLoop << "_"<<L0<<":"<<endl;

        string endOfLoopString = "L" + to_string(endOfLoop);
        code(ast->getRight(),symbolTable,endOfLoopString,"switch");

        cout << "ujp switch_end_"<<endOfLoop << ""<<endl;



    }else if (ast->getRight() != nullptr && ast->getRight()->getVal() == "case") {
        codec(ast->getRight(),endOfLoop,endOfLabels,symbolTable);

    }


}


//returns the actual pcode
void code(AST* ast, SymbolTable symbolTable,string currentEndLabel,string cameFrom) {
	if (ast == nullptr)
		return;

	//check what operation it is
	if (ast->getVal() == "print") {
		codeR(ast->getLeft(),symbolTable);
		cout << "print" << endl;
	}
	else if (ast->getVal() == "assignment" ) {
		codeL(ast->getLeft(),symbolTable);
		codeR(ast->getRight(),symbolTable);
		cout << "sto" << endl;
	}
	else if (ast->getVal() == "if") {
		codeR(ast->getLeft(),symbolTable);
		cout << "fjp skip_if_" << currentLabel << endl;
		if (ast->getRight()->getVal() == "else") { //if it is if - else
			int oldLabelVal = currentLabel;
			currentLabel++;
			int newLabelVal = currentLabel;
			currentLabel++;

			code(ast->getRight()->getLeft(), symbolTable,currentEndLabel,cameFrom);
			cout << "ujp skip_else_" << newLabelVal << endl;
			cout << "skip_if_" << oldLabelVal << ":" << endl;
			code(ast->getRight()->getRight(), symbolTable,currentEndLabel,cameFrom);
			cout << "skip_else_" << newLabelVal << ":" << endl;
		}
		else { //if only if
			code(ast->getRight(), symbolTable,currentEndLabel,cameFrom);
			cout << "L" << currentLabel << ":" << endl;
		}

	}
	else if (ast->getVal() == "while") {
		int L0 = currentLabel;
		currentLabel++;
		int L1 = currentLabel;
		currentLabel++;

		cout << "while_" << L0 << ":" << endl;
		codeR(ast->getLeft(),symbolTable);
		cout << "fjp while_out_" << L1 << endl; //if false

		code(ast->getRight(),symbolTable,to_string(L1),"while");
		cout << "ujp while_" << L0 << endl;

		cout << "while_out_" << L1 << ":" << endl; //end of loop


	} else if(ast->getVal() == "break") {
	    if(cameFrom == "while")
            cout << "ujp while_out_" << currentEndLabel << endl; //if false
        else if(cameFrom == "switch")
             cout << "ujp switch_end_"<<currentEndLabel << ""<<endl;

	} else if(ast->getVal() == "switch") {

	    stack<int> switchLabels;
	    int L0 = currentLabel;
		currentLabel++;

        codeR(ast->getLeft(),symbolTable);
        cout << "neg" <<endl;
        cout << "ixj switch_end_" <<L0 <<endl;
        codec(ast->getRight(),L0,switchLabels,symbolTable);

        while(switchLabels.size() != 0) {
            int current = switchLabels.top();
            cout << "ujp case_"<<L0 << "_"<<current<<endl;
            switchLabels.pop();

        }
        cout << "switch_end_"<<L0 << ":"<<endl;

	}
	else { //recurse first to left, then to right
		code(ast->getLeft(), symbolTable,currentEndLabel,cameFrom);
		code(ast->getRight(), symbolTable,currentEndLabel,cameFrom);
	}

}

//call code recursivly
void generatePCode(AST* ast, SymbolTable symbolTable) {
	ast = ast->getRight()->getRight();
	code(ast, symbolTable,"","");
}


