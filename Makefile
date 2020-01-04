build: homework
homework: tema3APD.cpp
	mpicxx -o tema3 tema3APD.cpp -O2
clean:
	rm -f tema3