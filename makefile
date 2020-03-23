EXECUTABLE=digicom_r.exe
 
CC="C:\\mingw-w64\\x86_64-8.1.0-win32-seh-rt_v6-rev0\\mingw64\\bin\\g++.exe"

$(EXECUTABLE) : maingw.o digimain.o serial.o
	$(CC) -o $(EXECUTABLE) maingw.o digimain.o serial.o

maingw.o : maingw.cpp hardware.h digimain.h
	$(CC) -c maingw.cpp

digimain.o : digimain.cpp hardware.h digimain.h
	$(CC) -c digimain.cpp

serial.o : serial.cpp hardware.h serial.h
	$(CC) -c serial.cpp

clean : 
	rm ./*.o ./$(EXECUTABLE)
