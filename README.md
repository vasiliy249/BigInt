# BigInt
file mapping and multithreading test

find the largest 4byte-integer number in binary file by means of memory file mapping by multiple threads

- *help*: 

    BigInt.exe -h

- *generate* 100 Mb file with filename test.dat with random integer numbers from 0 to 100: 

    BigInt.exe -f test.dat -c 100 -t 4

- *find max* integer number in filen with filename test.dat using 300 Mb chunk and 4 threads: 
  
    BigInt.exe -f test.dat -c 300 -t 4 
