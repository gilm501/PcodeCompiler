
#include <iostream>
#include <algorithm>
#include <string>
#include <fstream>
#include <vector>
#include <stack>
#include <stdio.h>

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
    //basic
	string name;
	string type;
	int size = -1;


    //address
	string recType = "-";
    int nd = 0;
    int offset = 0;
    Variable* parentFunc;

    //suppor function reference
    Variable* funcReference = nullptr;

	//support functions
	int paramsSize = 0;
    int localVarSize = 0;
    vector<Variable*> params;


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
	vector<Variable*> varsSymboleTable; // vectore of all current variables
    int currentOffset = 5;
public:

    static SymbolTable* returnSymbolTable() {
        static SymbolTable* allSymbolsTable;
        if(allSymbolsTable == nullptr) {
            allSymbolsTable = new SymbolTable();
	    }

	    return allSymbolsTable;
    }

    //return vaaiable
    Variable* returnVar(string name,Variable* currentSearch) {
        if (currentSearch == nullptr) { //support "program" level
            for (int i = 0; i < varsSymboleTable.size(); i++) {
                if (varsSymboleTable[i]->name == name) // if they have the same father
                    return varsSymboleTable[i];
            }
            return nullptr;
        }

		for (int i = 0; i < varsSymboleTable.size(); i++) {
			if (varsSymboleTable[i]->name == name && varsSymboleTable[i]->parentFunc != nullptr && varsSymboleTable[i]->parentFunc->name == currentSearch->name && varsSymboleTable[i]->parentFunc->nd == currentSearch->nd) // if they have the same father
				return varsSymboleTable[i];
		}

		return returnVar(name,currentSearch->parentFunc);
	}

    //addes array ranges
	static void addRangesToArray(Variable* currrentVar,AST* rangeList) {
        if (rangeList == nullptr)
            return;

        SymbolTable::addRangesToArray(currrentVar,rangeList->getLeft());


        if(rangeList->getVal() == "range") {
            Range current(stoi(rangeList->getLeft()->getLeft()->getVal()),stoi(rangeList->getRight()->getLeft()->getVal()));
            currrentVar->arrayRanges.push_back(current);
        }

        if((rangeList->getRight() != nullptr && rangeList->getRight()->getVal() == "range")) {
            SymbolTable::addRangesToArray(currrentVar,rangeList->getRight());
        }

	}
    //starts from d1,d2,d3..
	static int returnD(int dNum,Variable* currArray) {
        return currArray->arrayRanges[dNum-1].to - currArray->arrayRanges[dNum-1].from +1;
	}

	//returns mul of all d
	static int returnDMulFrom(int fromD,Variable* currArray ) {
	    int mul = 1;
	    for(int i = fromD-1;i<currArray->arrayRanges.size();i++) {
            mul *= SymbolTable::returnD(i + 1,currArray);
	    }

        return mul;
	}

	//returns the usb part of array
	static int returnSubPart(Variable* currArray) {
        int subPart = 0;

        for(int i = 0;i<currArray->arrayRanges.size();i++) {
            subPart += currArray->arrayRanges[i].from * SymbolTable::returnDMulFrom(i+2,currArray);
	    }
	    subPart *= currArray->g;

	    return subPart;
	}

    int returnRecordOffeset(string name) {
        int offset = 0;
        bool found = false;

        for(int i = this->varsSymboleTable.size()-1;i>=0;i--) {
            if(found == true && this->varsSymboleTable[i]->type == "record")
                break;

            if(found)
                offset += this->varsSymboleTable[i]->size;

            if(this->varsSymboleTable[i]->name == name)
                found = true;
	    }

	    return offset;
	}

	int returnSizeOf(string name) {
	    for (int i = 0; i < varsSymboleTable.size() ;i++) {
			Variable* currentVar = varsSymboleTable[i];
			if(currentVar->name == name)
                return currentVar->size;
		}

		return 1;
	}


    int sumSizes(int from, int to) {

        int sum =0;
          for (int i = from; i <= to ;i++) {
			Variable* currentVar = varsSymboleTable[i];

			//skip records
			if(currentVar->type == "record") {
                continue;
			}

			sum += currentVar->size;
			to -= currentVar->numSubData;
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

    static void generateSymbolTableHelper(AST* tree,Variable* parentFunc) {
        SymbolTable* generalSymbolTabel = returnSymbolTable();
        //cout << "current tree : -" << tree->getVal() << "- good" << endl;

		//if reached the end
		if (tree == nullptr) {
			return;
		}


        //function/procudure/main
		if (tree->getVal() == "procedure" || tree->getVal() == "program" || tree->getVal() == "function") {
            generalSymbolTabel->currentOffset = 5;

            string currentType = tree->getVal();
            Variable* currentVar = new Variable();

            //default data
            currentVar->name = tree->getLeft()->getLeft()->getLeft()->getVal();
            currentVar->type = currentType;

            currentVar->offset = 0;
            currentVar->parentFunc = parentFunc;

             //set nd to 0 for program
            if(tree->getVal() == "program")
                currentVar->nd =0;
            else
                currentVar->nd = currentVar->parentFunc->nd+1;


            generalSymbolTabel->varsSymboleTable.push_back(currentVar);
            int currentIndex = generalSymbolTabel->varsSymboleTable.size();

            //SUPORT PARAMETERS
            generateSymbolTableHelper(tree->getLeft()->getRight(),currentVar); //paramters
            generalSymbolTabel->varsSymboleTable[currentIndex-1]->paramsSize= generalSymbolTabel->sumSizes((currentIndex),generalSymbolTabel->varsSymboleTable.size()-1);

            //add parameters to vector
            for(int i = currentIndex;i<generalSymbolTabel->varsSymboleTable.size();i++){
                generalSymbolTabel->varsSymboleTable[currentIndex-1]->params.push_back(generalSymbolTabel->varsSymboleTable[i]);
            }

            //SUPPORT LOCAL CARS
            int sumLocalVarsFrom = generalSymbolTabel->varsSymboleTable.size();
            if (tree->getRight()->getLeft() != nullptr)
                generateSymbolTableHelper(tree->getRight()->getLeft()->getLeft(),currentVar); //local vars

            generalSymbolTabel->varsSymboleTable[currentIndex-1]->localVarSize = generalSymbolTabel->sumSizes((sumLocalVarsFrom),generalSymbolTabel->varsSymboleTable.size()-1);


            //call inherited function
            if (tree->getRight()->getLeft() != nullptr && tree->getRight()->getLeft()->getRight() != nullptr)
                generateSymbolTableHelper(tree->getRight()->getLeft()->getRight(),currentVar);

            return;

		}

        //get recursive left first, then handle from bottom to up
        generateSymbolTableHelper(tree->getLeft(),parentFunc);


        //run on all functions
        if(tree->getVal() == "functionsList") {
            generateSymbolTableHelper(tree->getRight(),parentFunc);
            return;
        }

		//check if declartion list
		if (tree->getVal() == "declarationsList" || tree->getVal() == "parametersList") {
            string recType = "-";
            if(tree->getVal() == "parametersList")
                recType = tree->getRight()->getVal();

            string currentType = tree->getRight()->getRight()->getVal();
            Variable* currentVar = new Variable();
            currentVar->recType = recType;

            //default data
            currentVar->name = tree->getRight()->getLeft()->getLeft()->getVal();
            currentVar->type = currentType;

            currentVar->offset = generalSymbolTabel->currentOffset;
            currentVar->parentFunc = parentFunc;
            currentVar->nd = currentVar->parentFunc->nd +1;

            //function type / pointer
            if (tree->getRight()->getLeft() != nullptr && tree->getRight()->getRight() != nullptr && tree->getRight()->getRight()->getVal() == "identifier") {
                Variable* pointTo = generalSymbolTabel->returnVar(tree->getRight()->getRight()->getLeft()->getVal(),nullptr);

                //if function
                if(pointTo != nullptr && (pointTo->type == "function" || pointTo->type == "procedure")){
                    currentType = "functionReference";
                    currentVar->type = currentType;
                    currentVar->funcReference = pointTo;
                } else { //if reference
                    if(currentVar->recType == "byValue") {
                        currentType = "byValreference";
                    }else
                        currentType = "pointer";
                }
            }

            if(currentType == "array") {
                //add coordinates
                SymbolTable::addRangesToArray(currentVar,tree->getRight()->getRight()->getLeft());

                if(tree->getRight()->getRight()->getRight()->getVal() == "identifier") {
                    currentVar->g = generalSymbolTabel->returnSizeOf(tree->getRight()->getRight()->getRight()->getLeft()->getVal());
                    currentVar->pointerTo = tree->getRight()->getRight()->getRight()->getLeft()->getVal();
                }else
                    currentVar->g = 1;

                currentVar->subPart = generalSymbolTabel->returnSubPart(currentVar);
                currentVar->size = generalSymbolTabel->returnDMulFrom(1,currentVar) * currentVar->g; // d0*d1*****g
                generalSymbolTabel->varsSymboleTable.push_back(currentVar);

            } else if (currentType == "record") {

                generalSymbolTabel->varsSymboleTable.push_back(currentVar);
                int currentIndex = generalSymbolTabel->varsSymboleTable.size();
                generateSymbolTableHelper(tree->getRight()->getRight()->getLeft(),parentFunc);
                generalSymbolTabel->varsSymboleTable[currentIndex-1]->numSubData = generalSymbolTabel->returnNumChilren(tree->getRight()->getRight()->getLeft());
                generalSymbolTabel->varsSymboleTable[currentIndex-1]->size = generalSymbolTabel->sumSizes((currentIndex),currentIndex+generalSymbolTabel->varsSymboleTable[currentIndex-1]->numSubData-1);

            }else if (currentType == "pointer" || currentType == "byValreference") {
                currentVar->size = 1;

                if(currentVar->recType == "byReference") { //if reference to
                    currentVar->type = "pointer";
                    currentVar->pointerTo = tree->getRight()->getRight()->getLeft()->getVal();
                } else if (currentVar->recType == "byValue") { //if by value reference
                    currentVar->type = "byValReference";
                    currentVar->pointerTo = tree->getRight()->getRight()->getLeft()->getVal();

                    Variable* pointTo = generalSymbolTabel->returnVar(currentVar->pointerTo,currentVar->parentFunc);
                    currentVar->size = pointTo->size;
                } else {
                    currentVar->pointerTo = tree->getRight()->getRight()->getLeft()->getLeft()->getVal();
                }

                generalSymbolTabel->varsSymboleTable.push_back(currentVar);
            } else if (currentType == "functionReference") {
                currentVar->size = 2;
                generalSymbolTabel->varsSymboleTable.push_back(currentVar);
            }else {
                currentVar->size = 1;
                generalSymbolTabel->varsSymboleTable.push_back(currentVar);
            }

            if(currentVar->type != "record")
                generalSymbolTabel->currentOffset += currentVar->size;

		}


    }

	//create the symbol table
	static SymbolTable generateSymbolTable(AST* tree) {

        generateSymbolTableHelper(tree,nullptr);
        SymbolTable* generalSymbolTabel = SymbolTable::returnSymbolTable();

        return *generalSymbolTabel;

	}

	//print data
	 void print() {
		cout << endl << "Symbol Table: " << endl;
		for (int i = 0; i < varsSymboleTable.size() ;i++) {
			Variable* currentVar = varsSymboleTable[i];
			cout << "name:" <<currentVar->name << " type:" << currentVar->type << " size:" << currentVar->size  << " sub : " << currentVar->numSubData ;
            cout << " nd:"<<currentVar->nd << " offset : "<<currentVar->offset << " currRec:" << currentVar->recType;
			if (currentVar->parentFunc != nullptr)
                cout << " parentFunc : "<<currentVar->parentFunc->name << " ";

			if(currentVar->type == "array") {
                cout <<  " g:" << currentVar->g << " , subpart:" << currentVar->subPart;
            }else if(currentVar->type == "functionReference") {
                cout <<  " functionReference:" << currentVar->funcReference->name;
			}else if (currentVar->type == "program" || currentVar->type == "procedure" || currentVar->type == "function"){
                cout << "paramSize:" << currentVar->paramsSize << " localVarsSize:"<<currentVar->localVarSize << " numParams:" << currentVar->params.size() << "Params : " ;
                for (int j=0; j< currentVar->params.size();j++) {
                    cout << "- " << currentVar->params[j]->name << " ";
                }
            }


			cout << endl;
			cout << endl;

		}

		 cout << endl;
	}
};

//methods for genereate p code
void codeL(AST* ast, SymbolTable symbolTable, Variable* currFunction);
void codeR(AST* ast, SymbolTable symbolTable, Variable* currFunction);
void codeM(AST* ast, SymbolTable SymbolTable, Variable* currFunction);
int codeI(AST* ast,Variable* currentVar, SymbolTable symbolTable, Variable* currFunction);
void code(AST* ast, SymbolTable symbolTable,string currentEndLabel,string cameFrom, Variable* currFunction);
void generatePCode(AST* ast, SymbolTable symbolTable);
string returnType(AST* ast, SymbolTable symbolTable);

int currentLabel = 0;

//support sep
int currentStackSize = 0;
int currentMaxSize = 0;

//returns current sub tree in actual value
void codeR(AST* ast, SymbolTable symbolTable, Variable* currFunction) {
	if (ast->getVal() == "constInt" || ast->getVal() == "constReal") {
		cout << "ldc " << ast->getLeft()->getVal() << endl;

		currentStackSize+=1;
	}
	else if(ast->getVal() == "array") {
        codeL(ast,symbolTable,currFunction);
        cout << "ind" << endl;
	}
	else if (ast->getVal() == "plus") {
		codeR(ast->getLeft(),symbolTable,currFunction);
		codeR(ast->getRight(), symbolTable,currFunction);
		cout << "add" << endl;

		//sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;

        currentStackSize-=1;
	}
	else if (ast->getVal() == "minus") {
		codeR(ast->getLeft(), symbolTable,currFunction);
		codeR(ast->getRight(), symbolTable,currFunction);
		cout << "sub" << endl;

		//sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;

        currentStackSize-=1;
	}
	else if (ast->getVal() == "identifier") {
		codeL(ast,symbolTable,currFunction);
		cout << "ind" << endl;
	}
	else if (ast->getVal() == "record") {
        codeL(ast,symbolTable,currFunction);
        cout << "ind" << endl;
    }
	else if (ast->getVal() == "true") {
		cout << "ldc 1" << endl;

		currentStackSize+=1;
	}
	else if (ast->getVal() == "false") {
		cout << "ldc 0" << endl;

        currentStackSize+=1;
	}
	else if (ast->getVal() == "multiply") {
		codeR(ast->getLeft(), symbolTable,currFunction);
		codeR(ast->getRight(), symbolTable,currFunction);
		cout << "mul" << endl;


        //sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;

        currentStackSize-=1;
	}
	else if (ast->getVal() == "divide") {
		codeR(ast->getLeft(), symbolTable,currFunction);
		codeR(ast->getRight(), symbolTable,currFunction);
		cout << "div" << endl;

		//sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;

        currentStackSize-=1;
	}
	else if (ast->getVal() == "lessThan") {
		codeR(ast->getLeft(), symbolTable,currFunction);
		codeR(ast->getRight(), symbolTable,currFunction);
		cout << "les" << endl;

        //sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;

        currentStackSize-=1;
	}
	else if (ast->getVal() == "greaterThan") {
		codeR(ast->getLeft(), symbolTable,currFunction);
		codeR(ast->getRight(), symbolTable,currFunction);
		cout << "grt" << endl;

		//sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;

        currentStackSize-=1;
	}
	else if (ast->getVal() == "or") {
		codeR(ast->getLeft(), symbolTable,currFunction);
		codeR(ast->getRight(), symbolTable,currFunction);
		cout << "or" << endl;

		//sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;

        currentStackSize-=1;
	}
	else if (ast->getVal() == "and") {
		codeR(ast->getLeft(), symbolTable,currFunction);
		codeR(ast->getRight(), symbolTable,currFunction);
		cout << "and" << endl;

		//sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;

        currentStackSize-=1;
	}
	else if (ast->getVal() == "equals") {
		codeR(ast->getLeft(), symbolTable,currFunction);
		codeR(ast->getRight(), symbolTable,currFunction);
		cout << "equ" << endl;

		//sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;

        currentStackSize-=1;
	}
	else if (ast->getVal() == "lessOrEquals") {
		codeR(ast->getLeft(), symbolTable,currFunction);
		codeR(ast->getRight(), symbolTable,currFunction);
		cout << "leq" << endl;

		//sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;

        currentStackSize-=1;
	}
	else if (ast->getVal() == "greaterOrEquals") {
		codeR(ast->getLeft(), symbolTable,currFunction);
		codeR(ast->getRight(), symbolTable,currFunction);
		cout << "geq" << endl;

		//sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;

        currentStackSize-=1;
	}
	else if (ast->getVal() == "notEquals") {
		codeR(ast->getLeft(), symbolTable,currFunction);
		codeR(ast->getRight(), symbolTable,currFunction);
		cout << "neq" << endl;

		//sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;

        currentStackSize-=1;
	}
	else if (ast->getVal() == "not") {
		codeR(ast->getLeft(), symbolTable,currFunction);
		cout << "not" << endl;
	}
	else if (ast->getVal() == "negative") {
		codeR(ast->getLeft(), symbolTable,currFunction);
		cout << "neg" << endl;
	}
	else if (ast->getVal() == "pointer") {
		codeL(ast, symbolTable,currFunction);
		cout << "ind" << endl;
	} else if (ast->getVal() == "call") {
        //call function
        code(ast,symbolTable,"","",currFunction);

        //get return value
	}

}


void codeM(AST* ast, SymbolTable SymbolTable, Variable* currFunction) {
    if (ast->getVal() == "pointer") {
		cout << "ind" << endl;
	}

	if (ast->getVal() == "record") {
        codeL(ast->getLeft(),SymbolTable,currFunction);

		int getOffset = SymbolTable.returnRecordOffeset(ast->getRight()->getLeft()->getVal());
		cout << "inc " << getOffset << endl;
	}

}

int codeI(AST* ast,Variable* currentVar, SymbolTable symbolTable, Variable* currFunction) {
    if (ast == nullptr)
        return 1;

   int startFrom = codeI(ast->getLeft(),currentVar,symbolTable,currFunction);

   if(ast->getRight() != nullptr) {
        codeR(ast->getRight(),symbolTable,currFunction);

        cout << "ixa " << symbolTable.returnDMulFrom(startFrom+1,currentVar)*(currentVar->g) << endl;

        //sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;

        currentStackSize-=1;

        return startFrom + 1;
    }


}

string returnType(AST* ast, SymbolTable symbolTable,Variable* currFunction){
    if(ast == nullptr)
        return nullptr;

    if(ast->getVal() == "identifier") {
	    return ast->getLeft()->getVal();
    }
    if(ast->getVal() == "pointer") {
        Variable* currentVar = symbolTable.returnVar(returnType(ast->getLeft(),symbolTable,currFunction),currFunction);

	    return currentVar->pointerTo;
    }
    if(ast->getVal() == "record") {
	    return returnType(ast->getRight(),symbolTable,currFunction);
    }
    if(ast->getVal() == "array"){
        Variable* currentVar = symbolTable.returnVar(returnType(ast->getLeft(),symbolTable,currFunction),currFunction);
        return  currentVar->pointerTo;
    }
}

//return current sub tree in adress
void codeL(AST* ast, SymbolTable symbolTable, Variable* currFunction) {
	if (ast->getVal() == "identifier") {
        Variable* inFunction = currFunction;
        Variable* currentVar = symbolTable.returnVar(ast->getLeft()->getVal(),currFunction);
        //cout << "start:I am : "<<currentVar->name << "  in : " << currFunction->name << endl;

        int mst = inFunction->nd + 1 - currentVar->nd;
        if(inFunction->name == currentVar->name)
            mst = 0;

        cout << "lda " << mst << " "<<currentVar->offset << endl;

        currentStackSize+=1;

        if(currentVar->recType == "byReference")
            cout << "ind" << endl;

	} else if (ast->getVal() == "array") {
	    Variable* currentVar = symbolTable.returnVar(returnType(ast->getLeft(),symbolTable,currFunction),currFunction);

        codeL(ast->getLeft(),symbolTable,currFunction);

        codeI(ast->getRight(),currentVar, symbolTable,currFunction);
        cout << "dec " <<currentVar->subPart << endl;


	}else if (ast->getVal() == "record") {
        codeM(ast,symbolTable,currFunction);
	}
	else if (ast->getVal() == "pointer") {
		codeL(ast->getLeft(), symbolTable,currFunction);
		cout << "ind" << endl;
	}
	else {
        codeL(ast->getLeft(),symbolTable,currFunction);
        codeM(ast,symbolTable,currFunction);
	}
}

void codec(AST* ast,int endOfLoop, stack<int>& endOfLabels, SymbolTable symbolTable, Variable* currentFunction) {
    if(ast == nullptr)
        return;

    codec(ast->getLeft(),endOfLoop,endOfLabels,symbolTable,currentFunction);

    if(ast->getVal() == "case") {

        int L0 = stoi(ast->getLeft()->getLeft()->getVal());
		endOfLabels.push(L0);

        cout << "case_"<<endOfLoop << "_"<<L0<<":"<<endl;

        string endOfLoopString = "L" + to_string(endOfLoop);
        code(ast->getRight(),symbolTable,endOfLoopString,"switch",currentFunction);

        cout << "ujp switch_end_"<<endOfLoop << ""<<endl;



    }else if (ast->getRight() != nullptr && ast->getRight()->getVal() == "case") {
        codec(ast->getRight(),endOfLoop,endOfLabels,symbolTable,currentFunction);

    }


}

//handle function parameters
void codeEA(AST* ast, SymbolTable symbolTable,Variable* sendingToFunc, Variable* currFunction,int index) {
    if (ast == nullptr)
        return;

    codeEA(ast->getLeft(),symbolTable,sendingToFunc,currFunction,index-1);

    if(ast->getVal() == "argumentList") { //if found parameter
        Variable* passingFrom = symbolTable.returnVar(ast->getRight()->getLeft()->getVal(),currFunction);
        Variable* passingTo = sendingToFunc->params[index];

        //if passing by reference
        if(ast->getRight()->getVal() == "identifier") { // By reference -> XXX


            //cout << "Identifier, passing to var: " << passingTo->name << " in index "<<index<<", from func"<<sendingToFunc->name << endl;
            Variable* inFunction = currFunction;
            int mst = inFunction->nd + 1 - passingFrom->nd;

            if(inFunction->name == passingFrom->name)
                mst = 0;

            if(passingTo->recType == "byValue") { // reference -> Value

                //cout << "reference->value" << endl;

                //value -> value
                if(passingFrom->type == "int" || passingFrom->type == "real" || passingFrom->type == "pointer" || passingFrom->type == "byValreference") {
                    cout << "lda " << (mst) << " "<<passingFrom->offset << endl;
                    cout << "ind" << endl;


                    currentStackSize+=1;
                }
                else if(passingFrom->type == "function" || passingFrom->type == "procedure") {
                    cout << "ldc " << (char)toupper(passingFrom->name[0]) << endl;
                    cout << "lda " << (mst) << " "<<passingFrom->offset << endl;


                    currentStackSize+=2;
                }else { // reference -> value
                    cout << "lda " << (mst) << " "<<passingFrom->offset << endl;
                    cout << "movs " << passingFrom->size << endl;

                    currentStackSize+=  passingFrom->size;
                }
            } else { // reference -> reference

                    //cout << "reference->reference" << endl;
                    codeL(ast->getRight(),symbolTable,currFunction);

                    //cout << "lod " << (mst) << " "<<passingFrom->offset << endl;
            }
        }else { // by value -> XXX
            //cout << "value->Value" << endl;
            //if(passingTo->recType == "byValue")

            codeR(ast->getRight(),symbolTable,currFunction);
        }
    }
}

//return sep
int returnSEP(AST* ast, SymbolTable symbolTable, Variable* currFunction) {
    const string fileName = "output.txt";
    //analize sep
    currentStackSize = 0;
    currentMaxSize = 0;
    int tempLabel = currentLabel;

    //redirect out stream
    ofstream out(fileName);
    streambuf *coutBuf = cout.rdbuf();
    cout.rdbuf(out.rdbuf());

    code(ast->getRight()->getRight(),symbolTable,"","",currFunction); //main program

    //redirect back
    cout.rdbuf(coutBuf);

    //sep support
    if(currentStackSize > currentMaxSize)
        currentMaxSize = currentStackSize;

    //if (currFunction->name == "g")
    //    exit(EXIT_FAILURE);

    //remove file
    std::remove(fileName.c_str());
    currentLabel = tempLabel;


    return currentMaxSize;
}

//returns the actual pcode
void code(AST* ast, SymbolTable symbolTable,string currentEndLabel,string cameFrom, Variable* currFunction) {
	if (ast == nullptr)
		return;
    //make the functions start from bottom to up

    if(ast->getVal() == "program" || ast->getVal() == "procedure" || ast->getVal() == "function") {
        string currentSectionName = ast->getLeft()->getLeft()->getLeft()->getVal();
        Variable* currentVar = symbolTable.returnVar(currentSectionName,currFunction);

        cout << (char)toupper(currentSectionName[0]) << ":" <<endl;
        cout << "ssp " << (currentVar->paramsSize + currentVar->localVarSize + 5) << endl;
        cout << "sep " << returnSEP(ast,symbolTable,currentVar) << endl;

        cout << "ujp "<< (char)toupper(currentSectionName[0]) <<"_begin" << endl;

        //if has function
        if(ast->getRight()->getLeft() != nullptr && ast->getRight()->getLeft()->getRight() != nullptr)
            code(ast->getRight()->getLeft()->getRight(),symbolTable,"","",currentVar); //init functions

        cout << (char)toupper(currentSectionName[0]) << "_begin:" << endl;
        code(ast->getRight()->getRight(),symbolTable,"","",currentVar); //main program

        //end section
        if (ast->getVal() == "procedure")
            cout << "retp"<<endl;
        else if (ast->getVal() == "function")
            cout << "retf"<<endl;
        else
            cout << "stp" << endl;

        return;
    } else if(ast->getVal() == "functionsList") {
        code(ast->getLeft(),symbolTable,"","",currFunction);
        code(ast->getRight(),symbolTable,"","",currFunction);
    } else if(ast->getVal() == "call") {
            Variable* calledFunc = symbolTable.returnVar(ast->getLeft()->getLeft()->getVal(),currFunction);
            Variable* callingFrom = currFunction;

            int mst = callingFrom->nd + 1 - calledFunc->nd;
            //cout << "calling from: " << callingFrom->name << " called func: " << calledFunc->name << endl;

            currentStackSize += 5;

            //handling function calles
            if(calledFunc->type == "functionReference") {
                int offset = calledFunc->offset;
                cout << "mstf "<< mst << " " << offset << endl;

                //support reference
                calledFunc = calledFunc->funcReference;

                codeEA(ast->getRight(),symbolTable,calledFunc,currFunction,calledFunc->params.size()-1);

                cout << "smp "<<calledFunc->paramsSize<<endl;
                cout << "cupi " << mst << " " << offset << endl;


            //handling regular
            } else {
                 cout << "mst "<<mst<<endl;

                codeEA(ast->getRight(),symbolTable,calledFunc,currFunction,calledFunc->params.size()-1);

                cout << "cup " << calledFunc->paramsSize << " " << (char)toupper(ast->getLeft()->getLeft()->getVal()[0]) << endl;
            }

            if(currentStackSize > currentMaxSize)
                currentMaxSize = currentStackSize;
            currentStackSize -=(5 + calledFunc->paramsSize);

            //save 1 for return value
            if(calledFunc->type == "function") {
                currentStackSize += 1;
            }




    }
	//check what operation it is
	else if (ast->getVal() == "print") {
		codeR(ast->getLeft(),symbolTable,currFunction);
		cout << "print" << endl;

		//sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;
        currentStackSize -=1;
	}
	else if (ast->getVal() == "assignment" ) {
	    //cout <<" stack size :" << currentStackSize << " - " << endl;

		codeL(ast->getLeft(),symbolTable,currFunction);
		codeR(ast->getRight(),symbolTable,currFunction);
		cout << "sto" << endl;

		//sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;
        currentStackSize -=2;
	}
	else if (ast->getVal() == "if") {
		codeR(ast->getLeft(),symbolTable,currFunction);
		cout << "fjp L" << currentLabel << endl;
		int oldLabelVal = currentLabel;
        currentLabel++;

        //sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;
        currentStackSize -=1;

		if (ast->getRight()->getVal() == "else") { //if it is if - else
			int newLabelVal = currentLabel;
			currentLabel++;

			code(ast->getRight()->getLeft(), symbolTable,currentEndLabel,cameFrom,currFunction);
			cout << "ujp L" << newLabelVal << endl;
			cout << "L" << oldLabelVal << ":" << endl;
			code(ast->getRight()->getRight(), symbolTable,currentEndLabel,cameFrom,currFunction);
			cout << "L" << newLabelVal << ":" << endl;
		}
		else { //if only if
			code(ast->getRight(), symbolTable,currentEndLabel,cameFrom,currFunction);
			cout << "L" << oldLabelVal << ":" << endl;
		}

	}
	else if (ast->getVal() == "while") {
		int L0 = currentLabel;
		currentLabel++;
		int L1 = currentLabel;
		currentLabel++;

		cout << "L" << L0 << ":" << endl;
		codeR(ast->getLeft(),symbolTable,currFunction);
		cout << "fjp L" << L1 << endl; //if false

        //sep support
		if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;
        currentStackSize -=1;

		code(ast->getRight(),symbolTable,to_string(L1),"while",currFunction);
		cout << "ujp L" << L0 << endl;

		cout << "L" << L1 << ":" << endl; //end of loop


	} else if(ast->getVal() == "break") {
	    if(cameFrom == "while")
            cout << "ujp L" << currentEndLabel << endl; //if false
        else if(cameFrom == "switch")
             cout << "ujp L"<<currentEndLabel << ""<<endl;

	} else if(ast->getVal() == "switch") {

	    stack<int> switchLabels;
	    int L0 = currentLabel;
		currentLabel++;

        codeR(ast->getLeft(),symbolTable,currFunction);
        cout << "neg" <<endl;
        cout << "ixj switch_end_" <<L0 <<endl;

          //sep support
        if(currentStackSize > currentMaxSize)
            currentMaxSize = currentStackSize;
        currentStackSize -=1;

        codec(ast->getRight(),L0,switchLabels,symbolTable,currFunction);

        while(switchLabels.size() != 0) {
            int current = switchLabels.top();
            cout << "ujp switch_end_"<<L0 << "_"<<current<<endl;
            switchLabels.pop();

        }
        cout << "switch_end_"<<L0 << ":"<<endl;

	}
	else { //recurse first to left, then to right
		code(ast->getLeft(), symbolTable,currentEndLabel,cameFrom,currFunction);
		code(ast->getRight(), symbolTable,currentEndLabel,cameFrom,currFunction);
	}

}

//call code recursivly
void generatePCode(AST* ast, SymbolTable symbolTable) {
	code(ast, symbolTable,"","",nullptr);
}

/*
int main()
{
	AST* ast;

	ifstream myfile("C:\\Users\\Gil\ Maman\\Desktop\\hw2\\TestsHw2\\tree11.txt");
	//ifstream myfile("C:\\Users\\Gil Maman\\Desktop\\hw3\\TestsHw3\\tree13.txt");
	if (myfile.is_open())
	{
		ast = AST::createAST(myfile);
		myfile.close();


		SymbolTable symbolTable = SymbolTable::generateSymbolTable(ast);
		//symbolTable.print();

		generatePCode(ast, symbolTable);

	}
	else cout << "Unable to open file";
	getchar();
	return 0;

}
*/
