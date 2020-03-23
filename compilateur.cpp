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

using namespace std;

char current;				// Current car	
unsigned long TagNumber=0;
int cmpNextChar = 0;
enum OPREL {equ, diff, infe, supe, inf, sup, unknown};
char NextCar;       		// Next car



void ReadChar(void){
	if (cmpNextChar > 0){
			current = NextCar;
			cmpNextChar -- ;
	}
	else{
																								// Read character and skip spaces until 
		while(cin.get(current) && (current==' '||current=='\t'||current=='\n'));				// non space character is read
	}
}

void NextChar(void){
	while(cin.get(NextCar) && (NextCar==' '||NextCar=='\t'||NextCar=='\n')){
		cmpNextChar++;
	}
}



void Error(string s){
	cerr<< s << endl;
	exit(-1);
}

// ArithmeticExpression := Term {AdditiveOperator Term}
// Term := Digit | "(" ArithmeticExpression ")"
// AdditiveOperator := "+" | "-"
// Digit := "0"|"1"|"2"|"3"|"4"|"5"|"6"|"7"|"8"|"9"

	
void AdditiveOperator(void){
	if(current=='+'||current=='-')
		ReadChar();
	else
		Error("Opérateur additif attendu");	   // Additive operator expected
}
		
void Digit(void){
	if((current<'0')||(current>'9'))
		Error("Chiffre attendu");		   // Digit expected
	else{
		cout << "\tpush $"<<current<<endl;
		ReadChar();
	}
}

void ArithmeticExpression(void);			// Called by Term() and calls Term()

OPREL RelationalOperator(void){
	if(current != '=' && current != '!' && current != '<' && current != '>'){
		return unknown;
	}
	NextChar();
	if (NextCar == '='){
			switch(current){
					case '=' : 
						ReadChar(); ReadChar(); 
						return equ;
					case '!': 
						ReadChar(); ReadChar(); 
						return diff;
					case '<': 
						ReadChar(); ReadChar(); 
						return infe;
					case '>': 
						ReadChar(); ReadChar(); 
						return supe;
			}
	}	
	
	switch(current){
			case '=' : 
				Error("utilisez '==' comme opérateur d'égalité");
			case '<' : 
				ReadChar(); ReadChar(); 
				return inf;
			case '>' : 
				ReadChar(); ReadChar(); 
				return sup;
			default:	Error("opérateur relationnel inconnu");
	}
	return unknown;	
}

void Expression (void){
	OPREL opcomp;
	ArithmeticExpression();
	if (current == '=' || current == '!' ||current == '<' || current == '>'){
		opcomp = RelationalOperator();
		ArithmeticExpression();
		cout << "\tpop %rbx"<< " #resultat de l'expression 2" << endl;  	// get the result of the first Arethmetic Expression
		cout << "\tpop %rax"<< " #resultat de l'expression 1" << endl;		// get the result of the first Arethmetic Expression
		cout <<"\t cmpq %rax,%rbx" << endl;       // compare the resulsts
		switch(opcomp){
			case equ:
				cout << "\tje Vrai"<<++TagNumber<<"\t# If equal"<<endl;
				break;
			case diff:
				cout << "\tjne Vrai"<<++TagNumber<<"\t# If different"<<endl;
				break;
			case supe:
				cout << "\tjae Vrai"<<++TagNumber<<"\t# If above or equal"<<endl;
				break;
			case infe:
				cout << "\tjbe Vrai"<<++TagNumber<<"\t# If below or equal"<<endl;
				break;
			case inf:
				cout << "\tjb Vrai"<<++TagNumber<<"\t# If below"<<endl;
				break;
			case sup:
				cout << "\tja Vrai"<<++TagNumber<<"\t# If above"<<endl;
				break;
			default:
				Error("Opérateur de comparaison inconnu");

		}
		cout << "\tpush $0\t\t# False"<<endl;
		cout << "\tjmp Suite"<<TagNumber<<endl;
		cout << "Vrai"<<TagNumber<<":\tpush $0xFFFFFFFFFFFFFFFF\t\t# True"<<endl;	
		cout << "Suite"<<TagNumber<<":"<<endl;
	}
}


void Term(void){
	if(current=='('){
		ReadChar();
		ArithmeticExpression();
		if(current!=')')
			Error("')' était attendu");		// ")" expected
		else
			ReadChar();
	}
	else 
		if (current>='0' && current <='9')
			Digit();
	     	else
			Error("'(' ou chiffre attendu");
}

void ArithmeticExpression(void){
	char adop;
	Term();
	while(current=='+'||current=='-'){
		adop=current;		// Save operator in local variable
		AdditiveOperator();
		Term();
		cout << "\tpop %rbx"<<endl;	// get first operand
		cout << "\tpop %rax"<<endl;	// get second operand
		if(adop=='+')
			cout << "\taddq	%rbx, %rax"<<endl;	// add both operands
		else
			cout << "\tsubq	%rbx, %rax"<<endl;	// substract both operands
		cout << "\tpush %rax"<<endl;			// store result
	}

}

i/*nt main(void){	// First version : Source code on standard input and assembly code on standard output
	// Header for gcc assembler / linker
	cout << "\t\t\t# This code was produced by the CERI Compiler"<<endl;
	cout << "\t.text\t\t# The following lines contain the program"<<endl;
	cout << "\t.globl main\t# The main function must be visible from outside"<<endl;
	cout << "main:\t\t\t# The main function body :"<<endl;
	cout << "\tmovq %rsp, %rbp\t# Save the position of the stack's top"<<endl;

	// Let's proceed to the analysis and code production
	ReadChar();
	ArithmeticExpression();
	ReadChar();
	// Trailer for the gcc assembler / linker
	cout << "\tmovq %rbp, %rsp\t\t# Restore the position of the stack's top"<<endl;
	cout << "\tret\t\t\t# Return from main function"<<endl;
	if(cin.get(current)){
		cerr <<"Caractères en trop à la fin du programme : ["<<current<<"]";
		Error("."); // unexpected characters at the end of program
	}

}*/
		
int main(void){	// First version : Source code on standard input and assembly code on standard output
	// Header for gcc assembler / linker
	cout << "\t\t\t# This code was produced by the CERI Compiler"<<endl;
	cout << "\t.text\t\t# The following lines contain the program"<<endl;
	cout << "\t.globl main\t# The main function must be visible from outside"<<endl;
	cout << "main:\t\t\t# The main function body :"<<endl;
	cout << "\tmovq %rsp, %rbp\t# Save the position of the stack's top"<<endl;

	// Let's proceed to the analysis and code production
	//ReadChar();
	//ArithmeticExpression();
	//ReadChar();
	
	ReadChar();
	Expression();
	ReadChar();
	
	// Trailer for the gcc assembler / linker
	cout << "\tmovq %rbp, %rsp\t\t# Restore the position of the stack's top"<<endl;
	cout << "\tret\t\t\t# Return from main function"<<endl;
	if(cin.get(current)){
		cerr <<"Caractères en trop à la fin du programme : ["<<current<<"]";
		Error("."); // unexpected characters at the end of program
	}

}






