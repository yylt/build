FROM docker.io/library/debian:11-slim as builder

ARG http_proxy
ARG https_proxy

ENV http_proxy=$http_proxy
ENV https_proxy=$https_proxy

COPY . /
RUN /build.sh

# Use a multi-stage build
FROM docker.io/library/debian:11-slim

ARG http_proxy
ARG https_proxy

ENV http_proxy=$http_proxy
ENV https_proxy=$https_proxy

ENV PATH=$PATH:/usr/local/luajit/bin:/usr/local/nginx/sbin:/usr/local/nginx/bin

ENV LUA_PATH="/usr/local/share/luajit-2.1.0-beta3/?.lua;/usr/local/share/lua/5.1/?.lua;/usr/local/lib/lua/?.lua;;"
ENV LUA_CPATH="/usr/local/lib/lua/?/?.so;/usr/local/lib/lua/?.so;;"

COPY --from=builder /usr/local /usr/local
COPY --from=builder /opt /opt
COPY --from=builder /etc/nginx /etc/nginx

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        bash \
        util-linux \
        libpcre3 \
        libpcre3-dev \
        zlib1g \
        zlib1g-dev \
        libmaxminddb-dev \
        libxslt1-dev \
        libssl-dev \
        libssl3 \
        libgeoip-dev \
        ca-certificates \
        patch \
        libaio-dev \
        lmdb-utils \
        libcurlpp-dev \
        libprotobuf-dev \
        libyajl-dev \
        libxml2 libxml2-dev \
        libmaxminddb-dev \
        libyaml-cpp-dev \
        coreutils \
        tzdata dumb-init diffutils libcap2-bin \
  && rm -rf /var/lib/apt/lists/* \
  && ln -s /usr/local/nginx/sbin/nginx /sbin/nginx \
  && bash -eu -c ' \
  writeDirs=( \
  /var/log/nginx \
  /var/lib/nginx/body \
  /var/lib/nginx/fastcgi \
  /var/lib/nginx/proxy \
  /var/lib/nginx/scgi \
  /var/lib/nginx/uwsgi \
  /var/log/audit \
  ); \
  for dir in "${writeDirs[@]}"; do \
  mkdir -p ${dir}; \
  chown -R www-data.www-data ${dir}; \
  done'

EXPOSE 80 443

CMD ["nginx", "-g", "daemon off;"]
