VAR a,b : INTEGER;
       c: BOOLEAN.
a:=1;
b:=6;
WHILE a<12 DO
BEGIN
	IF a%2==0 THEN 
		DISPLAY a
	ELSE
		DISPLAY a*2;
	a:=a+1;
<<<<<<< HEAD
	DISPLAY a;
END;



DISPLAY '\n';
FOR tdbl:=0.0 To 13.0 DO
BEGIN
	DISPLAY cht;
	DISPLAY tdbl;
	DISPLAY '\n';
END.
=======
	c:=(a<=b);
	DISPLAY c
END.
>>>>>>> parent of d651349... Version final
