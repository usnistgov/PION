FROM debian:buster
RUN apt-get update && \
    apt-get install -qq ca-certificates jq && \
    echo "deb [trusted=yes] https://nfd-nightly-apt.ndn.today/debian buster main" > /etc/apt/sources.list.d/nfd-nightly.list && \
    apt-get update && \
    apt-get install -qq infoedit ndn-autoconfig-client ndnpeek ndnping ndnsec nfd && \
    apt-get clean
COPY start.sh connect.sh /
RUN chmod +x /start.sh /connect.sh && \
    cp /etc/ndn/nfd.conf.sample /nfd.conf.sample
VOLUME /etc/ndn /var/lib/ndn/nfd /run/ndn
ENV HOME=/var/lib/ndn/nfd
EXPOSE 6363/udp
CMD /start.sh
