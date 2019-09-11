FROM nodered/node-red-docker:latest

USER root

RUN apt-get update

RUN npm i node-red-contrib-influxdb
RUN npm i node-red-node-ping
RUN npm i node-red-contrib-os
RUN npm i node-red-contrib-assert

COPY start.sh /

RUN chmod +x /start.sh

COPY flows.json /data

ENTRYPOINT ["/start.sh"]

EXPOSE 1880
