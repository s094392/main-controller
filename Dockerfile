FROM ubuntu

RUN apt update
RUN DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC apt -y install libhiredis-dev g++ make python3 curl

CMD ["/main-controller/controller"]
