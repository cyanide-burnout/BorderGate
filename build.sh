#!/bin/bash

make
sudo make install config
make clean

sudo service bordergate restart
./initiate.sh

sudo tail -f /var/log/syslog
