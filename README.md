# PcodeCompiler
Pcode compiler, compiles pascal code into Intermediate code

# Usage:

```
int main()
{
	AST* ast;
	ifstream myfile("PATH TO ABSTRACT SYNTAX TREE");
	if (myfile.is_open())
	{
  
    //GENERATS TREE
		ast = AST::createAST(myfile);
		myfile.close();

    //GENERATS SYMBOL TABLE
		SymbolTable symbolTable = SymbolTable::generateSymbolTable(ast);

    //GENERATS PCODE
		generatePCode(ast, symbolTable);

	}
	else cout << "Unable to open file";
	getchar();
	return 0;

}

```
