10 REM TEXT : HOME
20 PRINT "Prime Numbers"
30 PRINT " "; 
40 FOR X = 1 TO 1000
50 GOSUB 1000
60 IF P = 1 THEN PRINT X;" ";
70 NEXT
80 PRINT
90 END
1000 REM ** SUBROUTINE TO CHECK FOR PRIME **
1010 REM NUMBER TO BE CHECKED IS STORED IN X
1020 REM D IS USED FOR DIVISOR
1030 REM Q IS USED FOR QUOTIENT
1040 REM P IS USED FOR RETURN VALUE, IF X IS PRIME, P WILL BE 1, ELSE 0
1050 P = 0
1060 IF X < 2 OR X <> INT(X) GOTO 1180
1070 IF X = 2 OR X = 3 OR X = 5 THEN P=1 : GOTO 1180
1080 IF X/2 = INT(X/2) GOTO 1180
1090 IF X/3 = INT(X/3) GOTO 1180
1100 D = 5
1110 Q = X/D : IF Q = INT(Q) GOTO 1180
1120 D = D + 2
1130 IF D*D> X GOTO 1170
1140 Q = X/D : IF Q = INT(Q) GOTO 1180
1150 D = D + 4
1160 IF D*D <= X GOTO 1110
1170 P = 1
1180 RETURN
1190 REM ** END OF SUBROUTINE TO CHECK FOR PRIME **