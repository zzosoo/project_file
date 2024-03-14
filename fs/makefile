fs: main.o fs.o
	gcc -o myfs_shell main.o fs.o
main.o : fs.h
	gcc -c main.c
fs.o : fs.h
	gcc -c fs.c
clean : 
	rm main.o fs.o myfs_shell myfs
