SET PATH=C:\msys64\mingw64\bin;%PATH%
SET CC=g++.exe
SET CXXFLAGS=-std=c++17 -O3 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
SET LDFLAGS=-O3 -static -s

%CC% -c %CXXFLAGS% stats.cpp -o stats.o

%CC% -o word_stats.exe stats.o %LDFLAGS%