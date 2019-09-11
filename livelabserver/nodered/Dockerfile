FROM nodered/node-red-docker:latest

USER root

RUN npm i node-red-contrib-influxdb
RUN npm i node-red-node-ping
RUN npm i node-red-contrib-os
RUN npm i node-red-contrib-assert

COPY flows.json /data

EXPOSE 1880
