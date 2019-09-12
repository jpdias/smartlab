FROM nodered/node-red-docker:latest

USER root

RUN apt install libnss-mdns libavahi-compat-libdnssd-dev

USER node-red

RUN npm i node-red-contrib-influxdb
RUN npm i node-red-node-ping
RUN npm i node-red-contrib-os
RUN npm i node-red-contrib-assert
RUN npm i node-red-node-discovery

COPY flows.json /data/
COPY settings.js /data/

EXPOSE 1880
