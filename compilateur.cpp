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
enum TYPES {INTEGER, BOOLEAN, CHAR, DOUBLE};

TOKEN cur;				// cur token


FlexLexer* lexer = new yyFlexLexer; // This is the flex tokeniser
// tokens can be read using lexer->yylex()
// lexer->yylex() returns the type of the lexicon entry (see enum TOKEN in tokeniser.h)
// and lexer->YYText() returns the lexicon entry as a string

	
map<string, enum TYPES> DeclaredVariables;
unsigned long TagNumber=0;

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
	bool decimal=false;
	double d;
	unsigned int *i;
	string nombre=lexer->YYText();
	if(nombre.find(".")!=string::npos){
		d=atof(lexer->YYText());
		i=(unsigned int *) &d;
		cout <<"\tsubq $8,%rsp"<<endl;
		cout <<"\tmovl	$"<<*i<<", (%rsp)"<<endl;
		cout <<"\tmovl	$"<<*(i+1)<<", 4(%rsp)"<<endl;
		cur=(TOKEN) lexer->yylex();
		return DOUBLE;
	}
	else{
		cout <<"\tpush $"<<atoi(lexer->YYText())<<endl;
		cur=(TOKEN) lexer->yylex();
		return INTEGER;
	}
}



enum TYPES CharConst(void){
	cout<<"\tmovq $0, %rax"<<endl;
	cout<<"\tmovb $"<<lexer->YYText()<<",%al"<<endl;
	cout<<"\tpush %rax"<<endl;
	cur=(TOKEN) lexer->yylex();
	return CHAR;
}

// PLUS RIEN QUI MARCHE JE VAIS VRILLER
enum TYPES Factor(void){
	enum TYPES type;

	switch(cur){
		case RPARENT:
			cur=(TOKEN) lexer->yylex();
			type=Expression();
			if(cur!=LPARENT)
				Error("')' était attendu");		// ")" expected
			else
				cur=(TOKEN) lexer->yylex();
			break;
		case NUMBER:
			type=Number();
			break;
		case ID:
			type=Identifier();
			break;
		case CHARCONST:
			type=CharConst();
			break;
		default:
			Error("'(', ou constante ou variable attendue.");
	};
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
	TYPES t1, t2;
	OPMUL mulop;
	t1=Factor();
	while(cur==MULOP){
		mulop=MultiplicativeOperator();		// Save operator in local variable
		t2=Factor();
		if(t2!=t1)
			Error("types incompatibles dans l'expression");

		switch(mulop){
			case AND:
				if(t2!=BOOLEAN)
					Error("le type doit être BOOLEAN dans l'expression");
				cout << "\tpop %rbx"<<endl;	// get first operand
				cout << "\tpop %rax"<<endl;	// get second operand
				cout << "\tmulq	%rbx"<<endl;	// a * b -> %rdx:%rax
				cout << "\tpush %rax\t# AND"<<endl;	// store result
				break;
			case MUL:
				if(t2!=INTEGER)
					Error("le type doit être INTEGER dans l'expression");
				if(t2==INTEGER){
					cout << "\tpop %rbx"<<endl;	// get first operand
					cout << "\tpop %rax"<<endl;	// get second operand
					cout << "\tmulq	%rbx"<<endl;	// a * b -> %rdx:%rax
					cout << "\tpush %rax\t# MUL"<<endl;	// store result
				}
				else{
					cout<<"\tfldl	8(%rsp)\t"<<endl;
					cout<<"\tfldl	(%rsp)\t# first operand -> %st(0) ; second operand -> %st(1)"<<endl;
					cout<<"\tfmulp	%st(0),%st(1)\t# %st(0) <- op1 + op2 ; %st(1)=null"<<endl;
					cout<<"\tfstpl 8(%rsp)"<<endl;
					cout<<"\taddq	$8, %rsp\t# result on stack's top"<<endl; 
				}
				break;
			case DIV:
				if(t2!=INTEGER)
					Error("le type doit être INTEGER dans l'expression");
				cout << "\tpop %rbx"<<endl;		// get first operand
				cout << "\tpop %rax"<<endl;		// get second operand
				cout << "\tmovq $0, %rdx"<<endl; 	// Higher part of numerator  
				cout << "\tdiv %rbx"<<endl;			// quotient goes to %rax
				cout << "\tpush %rax\t# DIV"<<endl;		// store result
				break;
			case MOD:
				if(t2!=INTEGER)
					Error("le type doit être INTEGER dans l'expression");
				cout << "\tmovq $0, %rdx"<<endl; 	// Higher part of numerator  
				cout << "\tdiv %rbx"<<endl;			// remainder goes to %rdx
				cout << "\tpush %rdx\t# MOD"<<endl;		// store result
				break;
			default:
				Error("opérateur multiplicatif attendu");
		}
	}
	return t1;
}




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
TYPES SimpleExpression(void){
	OPADD adop;
	TYPES t1, t2;
	t1 = Term();
	while(cur==ADDOP){
		adop = AdditiveOperator();		// Save operator in local variable
		t2 = Term();
		if(t1!=t2) Error("Les operandes ne sont pas du meme type");
		switch(adop){
			case OR:
				if(t2!=BOOLEAN)
					Error("booleen expected");
				cout << "\tpop %rbx"<<endl;	// get first operand
				cout << "\tpop %rax"<<endl;	// get second operand
				cout << "\torq	%rbx, %rax\t# OR"<<endl;// operand1 OR operand2
				cout << "\tpush %rax"<<endl;			// store result
				break;			
			case ADD:
				if(t2!=INTEGER&&t2!=DOUBLE)
					Error("numerique expected");
				if(t2==INTEGER){
					cout << "\tpop %rbx"<<endl;	// get first operand
					cout << "\tpop %rax"<<endl;	// get second operand
					cout << "\taddq	%rbx, %rax\t# ADD"<<endl;	// add both operands
					cout << "\tpush %rax"<<endl;			// store result
				}
				else{
					cout<<"\tfldl	8(%rsp)\t"<<endl;
					cout<<"\tfldl	(%rsp)\t# first operand -> %st(0) ; second operand -> %st(1)"<<endl;
					cout<<"\tfaddp	%st(0),%st(1)\t# %st(0) <- op1 + op2 ; %st(1)=null"<<endl;
					cout<<"\tfstpl 8(%rsp)"<<endl;
					cout<<"\taddq	$8, %rsp\t# result on stack's top"<<endl; 
				}
				break;						
			case SUB:	
				if(t2!=INTEGER&&t2!=DOUBLE)
					Error("numerique expeced ");
				if(t2==INTEGER){
					cout << "\tpop %rbx"<<endl;	// get first operand
					cout << "\tpop %rax"<<endl;	// get second operand
					cout << "\tsubq	%rbx, %rax\t# ADD"<<endl;	// add both operands
					cout << "\tpush %rax"<<endl;			// store result
				}
				else{
					cout<<"\tfldl	(%rsp)\t"<<endl;
					cout<<"\tfldl	8(%rsp)\t# first operand -> %st(0) ; second operand -> %st(1)"<<endl;
					cout<<"\tfsubp	%st(0),%st(1)\t# %st(0) <- op1 - op2 ; %st(1)=null"<<endl;
					cout<<"\tfstpl 8(%rsp)"<<endl;
					cout<<"\taddq	$8, %rsp\t# result on stack's top"<<endl; 
				}
				break;	
			default:
				Error("operateur inconnu");
		}
	}
	return t1;
}



enum TYPES Type(void){
	if(cur!=MOTCLEF)
		Error("type expected");
	if(strcmp(lexer->YYText(),"BOOLEAN")==0){
		cur=(TOKEN) lexer->yylex();
		return BOOLEAN;
	}	
	else if(strcmp(lexer->YYText(),"INTEGER")==0){
		cur=(TOKEN) lexer->yylex();
		return INTEGER;
	}
	else if(strcmp(lexer->YYText(),"CHAR")==0){
		cur=(TOKEN) lexer->yylex();
		return CHAR;
	}
	else if(strcmp(lexer->YYText(),"DOUBLE")==0){
		cur=(TOKEN) lexer->yylex();
		return DOUBLE;
	}else
		Error("unknown type");

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
	    if(vdtype==INTEGER)
	    	cout << *it << ":\t.quad 0"<<endl;
		else if(vdtype==DOUBLE)
			cout << *it << ":\t.double 0.0"<<endl;
		else if(vdtype==CHAR)
			cout << *it << ":\t.byte 0"<<endl;
		else
			Error("type inconnu");
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

string *var = new string[15000];
TYPES *typ = new TYPES[15000];
int cptt = 0; 


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
	if(type1==CHAR){
		cout << "\tpop %rax"<<endl;
		cout << "\tmovb %al,"<<variable<<endl;
	}
	else
		cout << "\tpop "<<variable<<endl;

}




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
		else if(type==DOUBLE){
			cout << "\tmovsd	(%rsp), %xmm0\t\t# &stack top -> %xmm0"<<endl;
			cout << "\tsubq	$16, %rsp\t\t# allocation for 3 additional doubles"<<endl;
			cout << "\tmovsd %xmm0, 8(%rsp)"<<endl;
			cout << "\tmovq $FormatString2, %rdi\t# \"%lf\\n\""<<endl;
			cout << "\tmovq	$1, %rax"<<endl;
			cout << "\tcall	printf"<<endl;
			cout << "nop"<<endl;
			cout << "\taddq $24, %rsp\t\t\t# pop nothing"<<endl;
		}
		else if(type==CHAR){
			cout<<"\tpop %rsi\t\t\t# get character in the 8 lowest bits of %si"<<endl;
			cout << "\tmovq $FormatString3, %rdi\t# \"%c\\n\""<<endl;
			cout << "\tmovl	$0, %eax"<<endl;
			cout << "\tcall	printf@PLT"<<endl;
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


//<case label list> ::= <constant> {, <constant> }
TYPES CaseLabelList(){
	TYPES t1,t2;
	cur=(TOKEN) lexer->yylex();
	t1=Expression();
	cout << "\tpop %rbx" <<endl;
	cout << "\tcmpq %rcx, %rbx" <<endl;
	cout << "\tje CASEstate" << TagNumber<<endl;
	while(cur==COMMA){
		cur=(TOKEN) lexer->yylex();
		t2=Expression();
		cout << "\tpop %rbx" <<endl;
		cout << "\tcmpq %rcx, %rbx" <<endl;
		cout << "\tje Casestate" << TagNumber <<endl;
		if(t2!=t1) Error("CaseLabelList-> erreur typage;");
	}
	cout << "\tjmp FinCASEstate"<<TagNumber<<"\t\t# PAS EGALES - on va au prochain CaseListElement"<<endl;
	return t1;
}
//<case list element> ::= <case label list> : <statement>
TYPES CaseListElement(){
	enum TYPES type = CaseLabelList();
	if(cur != COLON)
		Error("Caractère ':' attendu");
	cur = (TOKEN)lexer->yylex();
	cout<< "Casestate" << TagNumber << " : " <<endl;
	Statement();
	cout<< "\tjmp FinCase" << TagNumber <<endl;
	cout<< "FinCASEstate" << TagNumber << " : " <<endl;
	return type;

}

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
	unsigned long long tag=TagNumber++;
	cur=(TOKEN) lexer->yylex();
	if(Expression()!=BOOLEAN)
		Error("le type de l'expression doit être BOOLEAN");
	cout<<"\tpop %rax\t# Get the result of expression"<<endl;
	cout<<"\tcmpq $0, %rax"<<endl;
	cout<<"\tje Else"<<tag<<"\t# if FALSE, jump to Else"<<tag<<endl;
	if(cur!=MOTCLEF||strcmp(lexer->YYText(),"THEN")!=0)
		Error("mot-clé 'THEN' attendu");
	cur=(TOKEN) lexer->yylex();
	Statement();
	cout<<"\tjmp Next"<<tag<<"\t# Do not execute the else statement"<<endl;
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
	TYPES t1, t2;
	unsigned long long tag=TagNumber++;
	cout<<"ForInit"<<tag<<":"<<endl;

	if(!strcmp(lexer->YYText(),"FOR")){
		cur=(TOKEN) lexer->yylex();
		AssignementStatement();

		if(!strcmp(lexer->YYText(),"To")){
			cout<<"For"<<tag<<":"<<endl;
			cur=(TOKEN) lexer->yylex();
			t1=Expression();
			t2=typ[cptt-1];

			if(t1!=t2) Error("probleme de variable entre l'initialisation et l'expression");
			cout<<"\tpop %rax\t"<<endl;
			cout<<"\tcmpq "<<var[cptt-1]<<", %rax\t"<<endl; // comparation variable/resultat
			cout<<"\tjnae ForEND"<<tag<<"\t"<<endl;

			if(!strcmp(lexer->YYText(),"DO")){

				if(t2==DOUBLE){

					cout<<"\tpush "<<var[cptt-1]<<endl;
					cout<<"\tpush ForDoubleIncrementation"<<endl;
					cout<<"\tfldl	8(%rsp)\t"<<endl;
					cout<<"\tfldl	(%rsp)\t# first operand -> %st(0) ; second operand -> %st(1)"<<endl;
					cout<<"\tfaddp	%st(0),%st(1)\t# %st(0) <- op1 + op2 ; %st(1)=null"<<endl;
					cout<<"\tfstpl 8(%rsp)"<<endl;
					cout<<"\taddq	$8, %rsp\t"<<endl;
					cout<<"\tpop "<<var[cptt-1]<<endl;
				}
				else cout<<"\taddq $1,"<<var[cptt-1]<<endl;

				cur=(TOKEN) lexer->yylex();
				Statement();
				cptt--;
				cout<<"\tjmp For"<<tag<<endl;
				cout<<"ForEND"<<tag<<":"<<endl;

			}
			else Error("DO attendu");
		}
		else Error("To attendu");
	}
	else Error("FOR attendu");
}

// BlockStatement
void BlockStatement(void){
	if(!strcmp(lexer->YYText(),"BEGIN")){
		cur=(TOKEN) lexer->yylex();
		Statement();
		while(cur==SEMICOLON){
			cur=(TOKEN) lexer->yylex();
			Statement();
		}
		if(cur!=MOTCLEF||strcmp(lexer->YYText(), "END")!=0)
			Error("END expected");
		cur=(TOKEN) lexer->yylex();
	}
	else Error("BEGIN expected");
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

string *fon = new string[15000];
int ind=0;
// ProcedureDeclaration := <NomFonction> ";" BlockStatement
void ProcedureDeclaration(void){
	string nom;
	cur=(TOKEN) lexer->yylex();
	if(cur!=ID) Error("ID expected.");
	nom=lexer->YYText();
	fon[ind]=nom;
	cout<<fon[ind]<<":"<<endl;
	ind++;
	cur=(TOKEN) lexer->yylex();
	if(cur!=SEMICOLON) Error("';' expected.");
	cur=(TOKEN) lexer->yylex();
	BlockStatement();
	cout<<"\tret"<<endl;
}

void ProcedureCall(int i){
	cout<<"\tcall "<<fon[i]<<endl;
	cur=(TOKEN) lexer->yylex();
}

// Program := [VARDeclarationPart] StatementPart
void Program(void){
	if(cur==MOTCLEF && strcmp(lexer->YYText(), "VAR")==0)
		VarDeclarationPart();
	while((!strcmp(lexer->YYText(),"PROCEDURE"))){
		ProcedureDeclaration();
		cur=(TOKEN) lexer->yylex();
	}
	StatementPart();	
}

int main(void){	// First version : Source code on standard input and assembly code on standard output
	// Header for gcc assembler / linker
	cout << "\t\t\t# This code was produced by the Mattia La Mura"<<endl;
	cout << ".data"<<endl;
	cout << "FormatString1:\t.string \"%llu\"\t# used by printf to display 64-bit unsigned integers"<<endl; 
	cout << "FormatString2:\t.string \"%lf\"\t# used by printf to display 64-bit floating point numbers"<<endl; 
	cout << "FormatString3:\t.string \"%c\"\t# used by printf to display a 8-bit single character"<<endl; 
	cout << "TrueString:\t.string \"TRUE\"\t# used by printf to display the boolean value TRUE"<<endl; 
	cout << "FalseString:\t.string \"FALSE\"\t# used by printf to display the boolean value FALSE"<<endl; 
	cout << "ForDoubleIncrementation: .double 1.0"<<endl;
	cout << "SINorCOS: .double 0.0"<<endl;
	cout << "ForRadiant: .double 0.01745392\t\t#=(pigreque/180)"<<endl;
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
		
			




