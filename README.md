# PcodeCompiler
- Pcode compiler, compiles pascal code into Intermediate code - pcode
- Written in Compilers Course in the Univeristy of Haifa.
- Sorry for the messy code, Was written quick and dirty

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
