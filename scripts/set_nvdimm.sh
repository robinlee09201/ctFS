#!/bin/bash
sudo ndctl disable-namespace all
sudo ndctl destroy-namespace all --force
sudo ndctl create-namespace -m devdax -a 1G
