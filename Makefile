all:
	gcc -o src/port_scan src/port_scan.c
install:
	cp src/port_scan /usr/local/bin/port_scan
	chmod 777 /usr/local/bin/port_scan
clean:
	rm -rf src/port_scan
remove:
	rm -rf /usr/local/bin/port_scan
