VAR 	a,b : INTEGER;
       	cht: CHAR;    
       	tdbl: DOUBLE.
a:=1;
b:=6;

cht:='a';

WHILE a<12 DO
BEGIN
	IF a%2==0 THEN 
		DISPLAY a
	ELSE
		DISPLAY a*2;
	a:=a+1;
	c:=(a<=b);
	DISPLAY c
END



DISPLAY '\n';
FOR tdbl:=0.0 To 13.0 DO
BEGIN
	DISPLAY cht;
	DISPLAY tdbl;
	DISPLAY '\n';
END