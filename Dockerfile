FROM tobiaslue/taint-impact:latest

COPY entrypoint.sh /entrypoint.sh


ENTRYPOINT [ "/entrypoint.sh" ]