//  A compiler from a very simple Pascal-like structured language LL(k)
//  to 64-bit 80x86 Assembly langage
//  Copyright (C) 2019 Pierre Jourlin
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Build with "make compilateur"


#include <string>
#include <iostream>
#include <cstdlib>
#include <set>
#include <map>
#include <FlexLexer.h>
#include "tokeniser.h"
#include <cstring>

using namespace std;

enum OPREL {EQU, DIFF, INF, SUP, INFE, SUPE, WTFR};
enum OPADD {ADD, SUB, OR, WTFA};
enum OPMUL {MUL, DIV, MOD, AND ,WTFM};
enum TYPES {INTEGER, BOOLEAN};

TOKEN cur;				// cur token


FlexLexer* lexer = new yyFlexLexer; // This is the flex tokeniser
// tokens can be read using lexer->yylex()
// lexer->yylex() returns the type of the lexicon entry (see enum TOKEN in tokeniser.h)
// and lexer->YYText() returns the lexicon entry as a string

	
map<string, enum TYPES> DeclaredVariables;	// Store declared variables and their types
unsigned long long TagNumber=0;

bool IsDeclared(const char *id){
	return DeclaredVariables.find(id)!=DeclaredVariables.end();
}


void Error(string s){
	cerr << "Ligne n°"<<lexer->lineno()<<", lu : '"<<lexer->YYText()<<"'("<<cur<<"), mais ";
	cerr<< s << endl;
	exit(-1);
}

// Program := [DeclarationPart] StatementPart
// DeclarationPart := "[" Letter {"," Letter} "]"
// StatementPart := Statement {";" Statement} "."
// Statement := AssignementStatement
// AssignementStatement := Letter "=" Expression

// Expression := SimpleExpression [RelationalOperator SimpleExpression]
// SimpleExpression := Term {AdditiveOperator Term}
// Term := Factor {MultiplicativeOperator Factor}
// Factor := Number | Letter | "(" Expression ")"| "!" Factor
// Number := Digit{Digit}

// AdditiveOperator := "+" | "-" | "||"
// MultiplicativeOperator := "*" | "/" | "%" | "&&"
// RelationalOperator := "==" | "!=" | "<" | ">" | "<=" | ">="  
// Digit := "0"|"1"|"2"|"3"|"4"|"5"|"6"|"7"|"8"|"9"
// Letter := "a"|...|"z"
	
// Statement := AssignementStatement | IfStatement | WhileStatement | ForStatement | BlockStatement
// IfStatement := "IF" Expression "THEN" Statement [ "ELSE" Statement ]
// WhileStatement := "WHILE" Expression DO Statement
// ForStatement := "FOR" AssignementStatement "To" Expression "DO" Statement
// BlockStatement := "BEGIN" Statement { ";" Statement } "END"
		
// OLD VERSION
// void Identifier(void){
// 	cout << "\tpush "<<lexer->YYText()<<endl;
// 	cur=(TOKEN) lexer->yylex();
// }

// void Number(void){
// 	cout <<"\tpush $"<<atoi(lexer->YYText())<<endl;
// 	cur=(TOKEN) lexer->yylex();
// }

// void Expression(void);			// Called by Term() and calls Term()

// void Factor(void){
// 	if(cur==RPARENT){
// 		cur=(TOKEN) lexer->yylex();
// 		Expression();
// 		if(cur!=LPARENT)
// 			Error("')' était attendu");		
// 		else
// 			cur=(TOKEN) lexer->yylex();
// 	}
// 	else 
// 		if (cur==NUMBER)
// 			Number();
// 	     	else
// 				if(cur==ID)
// 					Identifier();
// 				else
// 					Error("'(' ou chiffre ou lettre attendue");
// }
enum TYPES Expression(void);			// Called by Term() and calls Term()
void Statement(void);
void StatementPart(void);

enum TYPES Identifier(void){
	enum TYPES type;
	if(!IsDeclared(lexer->YYText())){
		cerr << "Erreur : Variable '"<<lexer->YYText()<<"' non déclarée"<<endl;
		exit(-1);
	}
	type=DeclaredVariables[lexer->YYText()];
	cout << "\tpush "<<lexer->YYText()<<endl;
	cur=(TOKEN) lexer->yylex();
	return type;
}

enum TYPES Number(void){
	cout <<"\tpush $"<<atoi(lexer->YYText())<<endl;
	cur=(TOKEN) lexer->yylex();
	return INTEGER;
}

enum TYPES Factor(void){
	enum TYPES type;
	if(cur==RPARENT){
		cur=(TOKEN) lexer->yylex();
		type=Expression();
		if(cur!=LPARENT)
			Error("erreur parentese droite était attendu");		// ")" expected
		else
			cur=(TOKEN) lexer->yylex();
	}
	else 
		if (cur==NUMBER)
			type=Number();
	     	else
				if(cur==ID)
					type=Identifier();
				else
					Error("erreur : parentese gauche ou chiffre attendue");
	return type;
}



// MultiplicativeOperator := "*" | "/" | "%" | "&&"
OPMUL MultiplicativeOperator(void){
	OPMUL opmul;
	if(strcmp(lexer->YYText(),"*")==0)
		opmul=MUL;
	else if(strcmp(lexer->YYText(),"/")==0)
		opmul=DIV;
	else if(strcmp(lexer->YYText(),"%")==0)
		opmul=MOD;
	else if(strcmp(lexer->YYText(),"&&")==0)
		opmul=AND;
	else opmul=WTFM;
	cur=(TOKEN) lexer->yylex();
	return opmul;
}

// Term := Factor {MultiplicativeOperator Factor}
enum TYPES Term(void){
	TYPES type1, type2;
	OPMUL mulop;
	type1=Factor();
	while(cur==MULOP){
		mulop=MultiplicativeOperator();		// Save operator in local variable
		type2=Factor();
		if(type2!=type1)
			Error("types incompatibles dans l'expression");
		cout << "\tpop %rbx"<<endl;	// get first operand
		cout << "\tpop %rax"<<endl;	// get second operand
		switch(mulop){
			case AND:
				if(type2!=BOOLEAN)
					Error("le type doit être BOOLEAN dans l'expression");
				cout << "\tmulq	%rbx"<<endl;	// a * b -> %rdx:%rax
				cout << "\tpush %rax\t# AND"<<endl;	// store result
				break;
			case MUL:
				if(type2!=INTEGER)
					Error("le type doit être INTEGER dans l'expression");
				cout << "\tmulq	%rbx"<<endl;	// a * b -> %rdx:%rax
				cout << "\tpush %rax\t# MUL"<<endl;	// store result
				break;
			case DIV:
				if(type2!=INTEGER)
					Error("le type doit être INTEGER dans l'expression");
				cout << "\tmovq $0, %rdx"<<endl; 	// Higher part of numerator  
				cout << "\tdiv %rbx"<<endl;			// quotient goes to %rax
				cout << "\tpush %rax\t# DIV"<<endl;		// store result
				break;
			case MOD:
				if(type2!=INTEGER)
					Error("le type doit être INTEGER dans l'expression");
				cout << "\tmovq $0, %rdx"<<endl; 	// Higher part of numerator  
				cout << "\tdiv %rbx"<<endl;			// remainder goes to %rdx
				cout << "\tpush %rdx\t# MOD"<<endl;		// store result
				break;
			default:
				Error("opérateur multiplicatif attendu");
		}
	}
	return type1;
}


// OLD VERSION
// Term := Factor {MultiplicativeOperator Factor}
// void Term(void){
// 	OPMUL mulop;
// 	Factor();
// 	while(cur==MULOP){
// 		mulop=MultiplicativeOperator();		// Save operator in local variable
// 		Factor();
// 		cout << "\tpop %rbx"<<endl;	// get first operand
// 		cout << "\tpop %rax"<<endl;	// get second operand
// 		switch(mulop){
// 			case AND:
// 				cout << "\tmulq	%rbx"<<endl;	// a * b -> %rdx:%rax
// 				cout << "\tpush %rax\t# AND"<<endl;	// store result
// 				break;
// 			case MUL:
// 				cout << "\tmulq	%rbx"<<endl;	// a * b -> %rdx:%rax
// 				cout << "\tpush %rax\t# MUL"<<endl;	// store result
// 				break;
// 			case DIV:
// 				cout << "\tmovq $0, %rdx"<<endl; 	// Higher part of numerator  
// 				cout << "\tdiv %rbx"<<endl;			// quotient goes to %rax
// 				cout << "\tpush %rax\t# DIV"<<endl;		// store result
// 				break;
// 			case MOD:
// 				cout << "\tmovq $0, %rdx"<<endl; 	// Higher part of numerator  
// 				cout << "\tdiv %rbx"<<endl;			// remainder goes to %rdx
// 				cout << "\tpush %rdx\t# MOD"<<endl;		// store result
// 				break;
// 			default:
// 				Error("opérateur multiplicatif attendu");
// 		}
// 	}
// }

// AdditiveOperator := "+" | "-" | "||"
OPADD AdditiveOperator(void){
	OPADD opadd;
	if(strcmp(lexer->YYText(),"+")==0)
		opadd=ADD;
	else if(strcmp(lexer->YYText(),"-")==0)
		opadd=SUB;
	else if(strcmp(lexer->YYText(),"||")==0)
		opadd=OR;
	else opadd=WTFA;
	cur=(TOKEN) lexer->yylex();
	return opadd;
}

// SimpleExpression := Term {AdditiveOperator Term}
enum TYPES SimpleExpression(void){
	enum TYPES type1, type2;
	OPADD adop;
	type1=Term();
	while(cur==ADDOP){
		adop=AdditiveOperator();		// Save operator in local variable
		type2=Term();
		if(type2!=type1)
			Error("types incompatibles dans l'expression");
		cout << "\tpop %rbx"<<endl;	// get first operand
		cout << "\tpop %rax"<<endl;	// get second operand
		switch(adop){
			case OR:
				if(type2!=BOOLEAN)
					Error("le type doit être BOOLEAN dans l'expression");
				cout << "\taddq	%rbx, %rax\t# OR"<<endl;// operand1 OR operand2
				break;			
			case ADD:
				if(type2!=INTEGER)
					Error("le type doit être INTEGER dans l'expression");
				cout << "\taddq	%rbx, %rax\t# ADD"<<endl;	// add both operands
				break;			
			case SUB:	
				if(type2!=INTEGER)
					Error("le type doit être INTEGER dans l'expression");
				cout << "\tsubq	%rbx, %rax\t# SUB"<<endl;	// substract both operands
				break;
			default:
				Error("opérateur additif inconnu");
		}
		cout << "\tpush %rax"<<endl;			// store result
	}
	return type1;
}


// OLD VERSION
// // SimpleExpression := Term {AdditiveOperator Term}
// void SimpleExpression(void){
// 	OPADD adop;
// 	Term();
// 	while(cur==ADDOP){
// 		adop=AdditiveOperator();		// Save operator in local variable
// 		Term();
// 		cout << "\tpop %rbx"<<endl;	// get first operand
// 		cout << "\tpop %rax"<<endl;	// get second operand
// 		switch(adop){
// 			case OR:
// 				cout << "\taddq	%rbx, %rax\t# OR"<<endl;// operand1 OR operand2
// 				break;			
// 			case ADD:
// 				cout << "\taddq	%rbx, %rax\t# ADD"<<endl;	// add both operands
// 				break;			
// 			case SUB:	
// 				cout << "\tsubq	%rbx, %rax\t# SUB"<<endl;	// substract both operands
// 				break;
// 			default:
// 				Error("opérateur additif inconnu");
// 		}
// 		cout << "\tpush %rax"<<endl;			// store result
// 	}

// }

enum TYPES Type(void){
	if(cur!=MOTCLEF)
		Error("type attendu");
	if(strcmp(lexer->YYText(),"BOOLEAN")==0){
		cur=(TOKEN) lexer->yylex();
		return BOOLEAN;
	}	
	else if(strcmp(lexer->YYText(),"INTEGER")==0){
		cur=(TOKEN) lexer->yylex();
		return INTEGER;
	}
	else
		Error("type inconnu");

	cur=(TOKEN) lexer->yylex();

}



// Declaration := Ident {"," Ident} ":" Type
void VarDeclaration(void){
	set<string> idents;
	enum TYPES vdtype;
	if(cur!=ID)
		Error("Un identificater était attendu");
	idents.insert(lexer->YYText());
	cur=(TOKEN) lexer->yylex();
	while(cur==COMMA){
		cur=(TOKEN) lexer->yylex();
		if(cur!=ID)
			Error("Un identificateur était attendu");
		idents.insert(lexer->YYText());
		cur=(TOKEN) lexer->yylex();
	}
	if(cur!=COLON)
		Error("caractère ':' attendu");
	cur=(TOKEN) lexer->yylex();
	vdtype=Type();
	for (set<string>::iterator it=idents.begin(); it!=idents.end(); ++it){
	    cout << *it << ":\t.quad 0"<<endl;
            DeclaredVariables[*it]=vdtype;
	}
}

// DeclarationPart := "[" Ident {"," Ident} "]"
// DEVIENS
// VarDeclarationPart := "VAR" VarDeclaration {";" VarDeclaration} "."
void VarDeclarationPart(void){
	cur=(TOKEN) lexer->yylex();
	VarDeclaration();
	while(cur==SEMICOLON){
		cur=(TOKEN) lexer->yylex();
		VarDeclaration();
	}
	if(cur!=DOT)
		Error("'.' point attendu");
	cur=(TOKEN) lexer->yylex();
}

// RelationalOperator := "==" | "!=" | "<" | ">" | "<=" | ">="  
OPREL RelationalOperator(void){
	OPREL oprel;
	if(strcmp(lexer->YYText(),"==")==0)
		oprel=EQU;
	else if(strcmp(lexer->YYText(),"!=")==0)
		oprel=DIFF;
	else if(strcmp(lexer->YYText(),"<")==0)
		oprel=INF;
	else if(strcmp(lexer->YYText(),">")==0)
		oprel=SUP;
	else if(strcmp(lexer->YYText(),"<=")==0)
		oprel=INFE;
	else if(strcmp(lexer->YYText(),">=")==0)
		oprel=SUPE;
	else oprel=WTFR;
	cur=(TOKEN) lexer->yylex();
	return oprel;
}

// Expression := SimpleExpression [RelationalOperator SimpleExpression]
enum TYPES Expression(void){
	enum TYPES type1, type2;
	unsigned long long tag;
	OPREL oprel;
	type1=SimpleExpression();
	if(cur==RELOP){
		tag=++TagNumber;
		oprel=RelationalOperator();
		type2=SimpleExpression();
		if(type2!=type1)
			Error("types incompatibles pour la comparaison");
		cout << "\tpop %rax"<<endl;
		cout << "\tpop %rbx"<<endl;
		cout << "\tcmpq %rax, %rbx"<<endl;
		switch(oprel){
			case EQU:
				cout << "\tje Vrai"<<tag<<"\t# If equal"<<endl;
				break;
			case DIFF:
				cout << "\tjne Vrai"<<tag<<"\t# If different"<<endl;
				break;
			case SUPE:
				cout << "\tjae Vrai"<<tag<<"\t# If above ou equal"<<endl;
				break;
			case INFE:
				cout << "\tjbe Vrai"<<tag<<"\t# If below ou equal"<<endl;
				break;
			case INF:
				cout << "\tjb Vrai"<<tag<<"\t# If below"<<endl;
				break;
			case SUP:
				cout << "\tja Vrai"<<tag<<"\t# If above"<<endl;
				break;
			default:
				Error("Opérateur de comparaison inconnu");
		}
		cout << "\tpush $0\t\t# False"<<endl;
		cout << "\tjmp Suite"<<tag<<endl;
		cout << "Vrai"<<tag<<":\tpush $0xFFFFFFFFFFFFFFFF\t\t# True"<<endl;	
		cout << "Suite"<<tag<<":"<<endl;
		return BOOLEAN;
	}
	return type1;
}


// OLD VERSION
// // Expression := SimpleExpression [RelationalOperator SimpleExpression]
// void Expression(void){
// 	OPREL oprel;
// 	SimpleExpression();
// 	if(cur==RELOP){
// 		oprel=RelationalOperator();
// 		SimpleExpression();
// 		cout << "\tpop %rax"<<endl;
// 		cout << "\tpop %rbx"<<endl;
// 		cout << "\tcmpq %rax, %rbx"<<endl;
// 		switch(oprel){
// 			case EQU:
// 				cout << "\tje Vrai"<<++TagNumber<<"\t# If equal"<<endl;
// 				break;
// 			case DIFF:
// 				cout << "\tjne Vrai"<<++TagNumber<<"\t# If different"<<endl;
// 				break;
// 			case SUPE:
// 				cout << "\tjae Vrai"<<++TagNumber<<"\t# If above ou equal"<<endl;
// 				break;
// 			case INFE:
// 				cout << "\tjbe Vrai"<<++TagNumber<<"\t# If below ou equal"<<endl;
// 				break;
// 			case INF:
// 				cout << "\tjb Vrai"<<++TagNumber<<"\t# If below"<<endl;
// 				break;
// 			case SUP:
// 				cout << "\tja Vrai"<<++TagNumber<<"\t# If above"<<endl;
// 				break;
// 			default:
// 				Error("Opérateur de comparaison inconnu");
// 		}
// 		cout << "\tpush $0\t\t# False"<<endl;
// 		cout << "\tjmp Suite"<<TagNumber<<endl;
// 		cout << "Vrai"<<TagNumber<<":\tpush $0xFFFFFFFFFFFFFFFF\t\t# True"<<endl;	
// 		cout << "Suite"<<TagNumber<<":"<<endl;
// 	}
// }

void AssignementStatement(void){
	enum TYPES type1, type2;
	string variable;
	if(cur!=ID)
		Error("Identificateur attendu");
	if(!IsDeclared(lexer->YYText())){
		cerr << "Erreur : Variable '"<<lexer->YYText()<<"' non déclarée"<<endl;
		exit(-1);
	}
	variable=lexer->YYText();
	type1=DeclaredVariables[variable];
	cur=(TOKEN) lexer->yylex();
	if(cur!=ASSIGN)
		Error("caractères ':=' attendus");
	cur=(TOKEN) lexer->yylex();
	type2=Expression();
	if(type2!=type1){
		cerr<<"Type variable "<<type1<<endl;
		cerr<<"Type Expression "<<type2<<endl;
		Error("types incompatibles dans l'affectation");
	}
	cout << "\tpop "<<variable<<endl;
}


// // AssignementStatement := Identifier ":=" Expression
// void AssignementStatement(void){
// 	string variable;
// 	if(cur!=ID)
// 		Error("Identificateur attendu");
// 	if(!IsDeclared(lexer->YYText())){
// 		cerr << "Erreur : Variable '"<<lexer->YYText()<<"' non déclarée"<<endl;
// 		exit(-1);
// 	}
// 	variable=lexer->YYText();
// 	cur=(TOKEN) lexer->yylex();
// 	if(cur!=ASSIGN)
// 		Error("caractères ':=' attendus");
// 	cur=(TOKEN) lexer->yylex();
// 	Expression();
// 	cout << "\tpop "<<variable<<endl;
// }

// DisplayStatement := "DISPLAY" Expression
void DisplayStatement(void){
	enum TYPES type;
	unsigned long long tag=++TagNumber;
	cur=(TOKEN) lexer->yylex();
	type=Expression();
	if(type==INTEGER){
		cout << "\tpop %rdx\t# valeur à afficher"<<endl;
		cout << "\tmovq $FormatString1, %rsi\t# \"%llu\\n\""<<endl;
	}
	else
		if(type==BOOLEAN){
			cout << "\tpop %rdx\t# zero : False, !zero : true"<<endl;
			cout << "\tcmpq $0, %rdx"<<endl;
			cout << "\tje False"<<tag<<endl;
			cout << "\tmovq $TrueString, %rsi\t# \"TRUE\\n\""<<endl;
			cout << "\tjmp Next"<<tag<<endl;
			cout << "False"<<tag<<":"<<endl;
			cout << "\tmovq $FalseString, %rsi\t# \"FALSE\\n\""<<endl;
			cout << "Next"<<tag<<":"<<endl;
		}
		else
			Error("DISPLAY ne fonctionne que pour les nombres entiers");
	cout << "\tmovl	$1, %edi"<<endl;
	cout << "\tmovl	$0, %eax"<<endl;
	cout << "\tcall	__printf_chk@PLT"<<endl;
}

void IfStatement(void);
void BlockStatement(void);
void WhileStatement(void);
void ForStatement(void);


// Statement := AssignementStatement
void Statement(void){
	if(cur==MOTCLEF)
		{	if (strcmp(lexer -> YYText(),"IF")==0){IfStatement();}
			else if (strcmp(lexer -> YYText(),"WHILE")==0){WhileStatement();}
			else if (strcmp(lexer -> YYText(),"FOR")==0){ForStatement();}
			else if (strcmp(lexer -> YYText(),"BEGIN")==0){BlockStatement();}
			else if(strcmp(lexer->YYText(),"DISPLAY")==0){DisplayStatement();}
			else Error("mot inconnu");
		}
	else
	if(cur==ID)
		AssignementStatement();
	else
		Error("instrction attendu");
}

// IfStatement
void IfStatement(void){
		/*cur=(TOKEN) lexer->yylex();
		Expression();
		if (strcmp(lexer -> YYText(),"THEN")==0){
			cur=(TOKEN) lexer->yylex();
			Statement();
			if (strcmp(lexer -> YYText(),"ELSE")==0){
				cur=(TOKEN) lexer->yylex();
				Statement();
			}
		}
		else{
			Error("caractère 'THEN' attendu");
		}*/
	unsigned long long tag=TagNumber++;
	cur=(TOKEN) lexer->yylex();
	if(Expression()!=BOOLEAN)
		Error("le type de l'expression doit être BOOLEAN");
	cout<<"\tpop %rax\t# resultat de l'expression"<<endl;
	cout<<"\tcmpq $0, %rax"<<endl;
	cout<<"\tje Else"<<tag<<"\t# if FALSE, go to else"<<tag<<endl;
	if(cur!=MOTCLEF||strcmp(lexer->YYText(),"THEN")!=0)
		Error("mot-clé 'THEN' attendu");
	cur=(TOKEN) lexer->yylex();
	Statement();
	cout<<"\tjmp Next"<<tag<<"\t# on rentre pas dans le else"<<endl;
	cout<<"Else"<<tag<<":"<<endl; // Might be the same effective adress than Next:
	if(cur==MOTCLEF&&strcmp(lexer->YYText(),"ELSE")==0){
		cur=(TOKEN) lexer->yylex();
		Statement();
	}
	cout<<"Next"<<tag<<":"<<endl;
}


// WhileStatement
void WhileStatement(void){
	unsigned long long tag=TagNumber++;
	cout<<"While"<<tag<<":"<<endl;
	cur=(TOKEN) lexer->yylex();
	if(Expression()!=BOOLEAN)
		Error("expression booléene attendue");
	cout<<"\tpop %rax\t# Get the result of expression"<<endl;
	cout<<"\tcmpq $0, %rax"<<endl;
	cout<<"\tje EndWhile"<<tag<<"\t# if FALSE, sortir de la boucle"<<tag<<endl;
	if(cur!=MOTCLEF||strcmp(lexer->YYText(), "DO")!=0)
		Error("mot-clé DO attendu");
	cur=(TOKEN) lexer->yylex();
	Statement();
	cout<<"\tjmp While"<<tag<<endl;
	cout<<"EndWhile"<<tag<<":"<<endl;
}


// ForStatement
void ForStatement(void){
	AssignementStatement();
	if (strcmp(lexer -> YYText(),"To")==0){
		cur=(TOKEN) lexer->yylex();
		Expression();
		if (strcmp(lexer -> YYText(),"DO")==0){
			cur=(TOKEN) lexer->yylex();
			Statement();
		}
		else {
		Error("caractère 'DO' attendu");
	}
	}
	else {
		Error("caractère 'To' attendu");
	}
}

// BlockStatement
void BlockStatement(void){
	cur=(TOKEN) lexer->yylex();
	Statement();
	while(cur==SEMICOLON){
		cur=(TOKEN) lexer->yylex();	// Skip the ";"
		Statement();
	};
	if(cur!=MOTCLEF||strcmp(lexer->YYText(), "END")!=0)
		Error("mot-clé END attendu");
	cur=(TOKEN) lexer->yylex();
}



// StatementPart := Statement {";" Statement} "."
void StatementPart(void){
	cout << "\t.align 8"<<endl;	// Alignement on addresses that are a multiple of 8 (64 bits = 8 bytes)
	cout << "\t.text\t\t# The following lines contain the program"<<endl;
	cout << "\t.globl main\t# The main function must be visible from outside"<<endl;
	cout << "main:\t\t\t# The main function body :"<<endl;
	cout << "\tmovq %rsp, %rbp\t# Save the position of the stack's top"<<endl;
	Statement();
	while(cur==SEMICOLON){
		cur=(TOKEN) lexer->yylex();
		Statement();
	}
	if(cur!=DOT)
		Error("caractère '.' attendu");
	cur=(TOKEN) lexer->yylex();
}

// Program := [VARDeclarationPart] StatementPart
void Program(void){
	if(cur==MOTCLEF && strcmp(lexer->YYText(),"VAR")==0)
		VarDeclarationPart();
	StatementPart();	
}

int main(void){	// First version : Source code on standard input and assembly code on standard output
	// Header for gcc assembler / linker
	cout << "\t\t\t# This code was produced by the CERI Compiler"<<endl;
	cout << ".data"<<endl;
	cout << "FormatString1:\t.string \"%llu\\n\"\t# used by printf to display 64-bit unsigned integers"<<endl;
	cout << "TrueString:\t.string \"TRUE\\n\"\t# used by printf to display the boolean value TRUE"<<endl; 
	cout << "FalseString:\t.string \"FALSE\\n\"\t# used by printf to display the boolean value FALSE"<<endl;  
	// Let's proceed to the analysis and code production
	cur=(TOKEN) lexer->yylex();
	Program();
	// Trailer for the gcc assembler / linker
	cout << "\tmovq %rbp, %rsp\t\t# Restore the position of the stack's top"<<endl;
	cout << "\tret\t\t\t# Return from main function"<<endl;
	if(cur!=FEOF){
		cerr <<"Caractères en trop à la fin du programme : ["<<cur<<"]";
		Error("."); // unexpected characters at the end of program
	}

}
		
			




