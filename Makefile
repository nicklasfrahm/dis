.PHONY: http ws

http:
	gcc c/http.c -o http.elf && ./http.elf

ws:
	gcc c/ws.c -o ws.elf -lcrypto && ./ws.elf