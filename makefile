all: run_tld


run_tld: run_tld.o TLD.o FerNNClassifier.o
	g++ -o run_tld run_tld.o TLD.o FerNNClassifier.o -g
run_tld.o: run_tld.cpp TLD.h
	g++ -c run_tld.cpp -o run_tld.o -g
TLD.o: TLD.cpp TLD.h
	g++ -c TLD.cpp -o TLD.o -g
FerNNClassifier.o: FerNNClassifier.cpp FerNNClassifier.h
	g++ -c FerNNClassifier.cpp -o FerNNClassifier.o -g


clean:
	-rm run_tld run_tld.o TLD.o FerNNClassifier.o
