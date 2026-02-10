#!/bin/bash

./cert.py -s api.telegram.org -n api_telegram >certs_tg.h 
./cert.py -s raw.githubusercontent.com -n rawgithub >certs_rg.h 
./cert.py -s api.github.com -n github >certs.h 


