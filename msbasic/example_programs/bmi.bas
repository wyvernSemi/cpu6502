10 REM A Simple Body Mass Index Calculator.
20 REM Written by Tim Dwyer on 5 July 2011.
30 REM 
40 DIM HEIGHT
50 DIM WEIGHT
60 DIM BMICALC
70 DIM AGAIN$
80 PRINT "***************************"
90 PRINT "*                         *"
100 PRINT "*  Simple BMI Calculator  *"
110 PRINT "*                         *"
120 PRINT "***************************"
130 PRINT ""
140 PRINT ""
150 INPUT "Input your height (inches): "; HEIGHT
160 INPUT "Input your weight (lbs): ";WEIGHT
170 BMICALC = (WEIGHT/(HEIGHT*HEIGHT))*703
180 PRINT ""
190 PRINT "Your BMI is ";BMICALC
200 PRINT ""
210 PRINT ""
220 INPUT "Calculate another? (y/n): ";AGAIN$
230 IF AGAIN$ = "y" OR AGAIN$ = "Y" GOTO 130
240 IF AGAIN$ = "n" OR AGAIN$ = "N" GOTO 300
250 IF AGAIN$ <> "y" OR AGAIN$ <> "Y" GOTO 270
260 IF AGAIN$ <> "n" OR AGAIN$ <> "N" GOTO 270
270 PRINT ""
280 PRINT "That is not a valid selection." 
290 GOTO 200
300 PRINT ""
310 PRINT "Goodbye!"
320 END

