.PHONY: http

http:
	g++ -o http c/http.c
	chmod +x http
	./http