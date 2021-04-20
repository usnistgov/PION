FROM debian:buster
RUN apt-get -y update && \
    apt-get -y install -qq --no-install-recommends ca-certificates jq && \
    echo "deb [trusted=yes] https://nfd-nightly-apt.ndn.today/debian buster main" > /etc/apt/sources.list.d/nfd-nightly.list && \
    apt-get -y update && \
    apt-get -y install -qq --no-install-recommends infoedit ndnpeek ndnping ndnsec nfd && \
    rm -rf /var/lib/apt/lists/*
COPY start.sh connect.sh /
RUN chmod +x /start.sh /connect.sh && \
    setcap -r /usr/bin/nfd || true && \
    echo 'transport=unix:///run/ndn/nfd.sock' > /etc/ndn/client.conf
VOLUME /var/lib/ndn/nfd /run/ndn
EXPOSE 6363/udp
CMD ["/start.sh"]
