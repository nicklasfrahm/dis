.PHONY: http ws

mqttc:
	gcc c/mqttc.c -o mqttc.elf

http:
	gcc c/http.c -o http.elf && ./http.elf

ws:
	gcc c/ws.c -o ws.elf -lcrypto && ./ws.elf
