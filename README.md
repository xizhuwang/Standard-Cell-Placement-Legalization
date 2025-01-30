Before:
![螢幕擷取畫面 2024-10-20 115945](https://github.com/user-attachments/assets/e86c14be-5c36-40e9-b40f-b5056a3e2dbf)
After:
![螢幕擷取畫面 2024-10-31 002007](https://github.com/user-attachments/assets/81cf49c0-386b-4f7a-ac10-cbf86db08eba)

# Standard-Cell-Placement-Legalization
1. Programming language: C++
2. Compilation environment: gcc version 13.2.0 (Ubuntu 13.2.0-23ubuntu4)

Compiler installation: sudo apt-get install g++
Environment construction: sudo apt-get install build-essential
Editor: VScode
Compile: g++ legalizer.cpp -o legalizer -Wall
Execute: ./legalizer ibm05 output02
 (Note: It will automatically grab the required files, no need to add file extension)
List: ls output.*

3.Visualization: Viewer.py

Program flow:
1. Read the file (wts and nets are not used), aux defines the file/scl defines the row/pl defines the coordinates/nodes defines the length and width
2. Preliminary legal solution: put it in the nearest row from left to right, from top to bottom, from large to small. If the nearest row is full, put it in the next nearest row until it is full.
3. Secondary optimization: calculate the Manhattan distance (x+y) and sort it, switch rows in this Manhattan range to find a closer distance, if not, keep it as it is
4. The maximum number of optimization iterations is 6, and convergence will exit early
5. Calculate the total displacement and maximum displacement and write to the file (6 bits of accuracy)

Actual execution time (Intel(R) i7-1165G7 /16G RAM)
1.toy: <1 Sec (44005.6000 / 2337.6000)
2.ibm01: 17 Sec (36962324.9663 / 35641.2000)
3.ibm05: 127 Sec (2224209.8687 / 525.5000)
