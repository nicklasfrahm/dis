.PHONY: http ws

http:
	gcc -o http.elf c/http.c && ./http.elf

ws:
	gcc -o ws.elf -lcrypto c/ws.c && ./ws.elf