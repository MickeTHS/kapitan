@echo off

kptest1.exe < in0.txt > test0.txt
kptest2.exe < in1.txt > test1.txt
kptest3.exe < in2.txt > test2.txt
kptest4.exe < in3.txt > test3.txt
kptest5.exe < in4.txt > test4.txt
kptest6.exe < in5.txt > test5.txt
kptest7.exe < in6.txt > test6.txt

@fc sol0.txt test0.txt > nul 2>&1

IF %ERRORLEVEL% NEQ 0 Echo Test 0 Not OK
IF %ERRORLEVEL% EQU 0 Echo Test 0 OK


@fc sol1.txt test1.txt > nul 2>&1

IF %ERRORLEVEL% NEQ 0 Echo Test 1 Not OK
IF %ERRORLEVEL% EQU 0 Echo Test 1 OK


@fc sol2.txt test2.txt > nul 2>&1

IF %ERRORLEVEL% NEQ 0 Echo Test 2 Not OK
IF %ERRORLEVEL% EQU 0 Echo Test 2 OK


@fc sol3.txt test3.txt > nul 2>&1

IF %ERRORLEVEL% NEQ 0 Echo Test 3 Not OK
IF %ERRORLEVEL% EQU 0 Echo Test 3 OK


@fc sol4.txt test4.txt > nul 2>&1

IF %ERRORLEVEL% NEQ 0 Echo Test 4 Not OK
IF %ERRORLEVEL% EQU 0 Echo Test 4 OK


@fc sol5.txt test5.txt > nul 2>&1

IF %ERRORLEVEL% NEQ 0 Echo Test 5 Not OK
IF %ERRORLEVEL% EQU 0 Echo Test 5 OK


@fc sol6.txt test6.txt > nul 2>&1

IF %ERRORLEVEL% NEQ 0 Echo Test 6 Not OK
IF %ERRORLEVEL% EQU 0 Echo Test 6 OK


@fc sol7.txt test7.txt > nul 2>&1

IF %ERRORLEVEL% NEQ 0 Echo Test 7 Not OK
IF %ERRORLEVEL% EQU 0 Echo Test 7 OK

