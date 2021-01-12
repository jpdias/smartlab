FROM nodered/node-red:latest

USER root


USER node-red

RUN npm i node-red-contrib-influxdb
RUN npm i node-red-node-ping
RUN npm i node-red-contrib-os
RUN npm i node-red-contrib-assert
RUN npm i node-red-contrib-self-healing

EXPOSE 1880
