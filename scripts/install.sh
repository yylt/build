#!/bin/sh

set -u -e

# Inspired by: https://github.com/intel/multus-cni/blob/83556f49bd6706a885eda847210b542669279cd0/images/entrypoint.sh#L161-L222
#
# Copyright (c) 2018 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
#
#SPDX-License-Identifier: Apache-2.0

CNI_BIN_DIR=${CNI_BIN_DIR:-"/host/opt/cni/bin/"}
WHEREABOUTS_KUBECONFIG_FILE_HOST=${WHEREABOUTS_KUBECONFIG_FILE_HOST:-"/etc/cni/net.d/whereabouts.d/whereabouts.kubeconfig"}
CNI_CONF_DIR=${CNI_CONF_DIR:-"/host/etc/cni/net.d"}
SKIP_TLS_VERIFY=${SKIP_TLS_VERIFY:-true}

INSTALL_CONFIG=${INSTALL_CONFIG:-"true"}
SLEEP_LOOP=${SLEEP_LOOP:-"true"}
INSTALL_CNI=${INSTALL_CNI:-"true"}
WHEREABOUTS_LOOP=${WHEREABOUTS_LOOP:-"true"}
INSTALL_CNI_CONFLIST=${INSTALL_CNI_CONFLIST:-"false"}

# Make a whereabouts.d directory (for our kubeconfig)
mkdir -p $CNI_CONF_DIR/whereabouts.d

WHEREABOUTS_KUBECONFIG=$CNI_CONF_DIR/whereabouts.d/whereabouts.kubeconfig
WHEREABOUTS_FLATFILE=$CNI_CONF_DIR/whereabouts.d/whereabouts.conf
WHEREABOUTS_KUBECONFIG_LITERAL=$(echo "$WHEREABOUTS_KUBECONFIG" | sed -e s'|/host||')

INSTALL_CNI_CONFLISTPATH=$CNI_CONF_DIR/99-simple.conflist

# ------------------------------- Generate a "kube-config"
SERVICE_ACCOUNT_PATH=/var/run/secrets/kubernetes.io/serviceaccount
KUBE_CA_FILE=${KUBE_CA_FILE:-$SERVICE_ACCOUNT_PATH/ca.crt}
SERVICEACCOUNT_TOKEN=$(cat $SERVICE_ACCOUNT_PATH/token)

# Setup our logging routines

function log()
{
    echo "$(date) ${1}"
}

function error()
{
    log "ERR:  {$1}"
}

function warn()
{
    log "WARN: {$1}"
}

function installwhereaboutsconfig()
{
    if [ "$INSTALL_CONFIG" != "true"  ]; then
        warn "not install whereabouts config"
        return
    fi
     log "start install whereabouts configuring."
# Check if we're running as a k8s pod.
if [ -f "$SERVICE_ACCOUNT_PATH/token" ]; then
  # We're running as a k8d pod - expect some variables.
  if [ -z ${KUBERNETES_SERVICE_HOST} ]; then
    error "KUBERNETES_SERVICE_HOST not set"; exit 1;
  fi
  if [ -z ${KUBERNETES_SERVICE_PORT} ]; then
    error "KUBERNETES_SERVICE_PORT not set"; exit 1;
  fi

  if [ "$SKIP_TLS_VERIFY" == "true" ]; then
    TLS_CFG="insecure-skip-tls-verify: true"
  elif [ -f "$KUBE_CA_FILE" ]; then
    TLS_CFG="certificate-authority-data: $(cat $KUBE_CA_FILE | base64 | tr -d '\n')"
  fi

  # Kubernetes service address must be wrapped if it is IPv6 address
  KUBERNETES_SERVICE_HOST_WRAP=$KUBERNETES_SERVICE_HOST
  if [ "$KUBERNETES_SERVICE_HOST_WRAP" != "${KUBERNETES_SERVICE_HOST_WRAP#*:[0-9a-fA-F]}" ]; then
    KUBERNETES_SERVICE_HOST_WRAP=\[$KUBERNETES_SERVICE_HOST_WRAP\]
  fi

  # Write a kubeconfig file for the CNI plugin.  Do this
  # to skip TLS verification for now.  We should eventually support
  # writing more complete kubeconfig files. This is only used
  # if the provided CNI network config references it.
  touch $WHEREABOUTS_KUBECONFIG
  chmod ${KUBECONFIG_MODE:-600} $WHEREABOUTS_KUBECONFIG
  cat > $WHEREABOUTS_KUBECONFIG <<EOF
# Kubeconfig file for Multus CNI plugin.
apiVersion: v1
kind: Config
clusters:
- name: local
  cluster:
    server: ${KUBERNETES_SERVICE_PROTOCOL:-https}://${KUBERNETES_SERVICE_HOST_WRAP}:${KUBERNETES_SERVICE_PORT}
    $TLS_CFG
users:
- name: whereabouts
  user:
    token: "${SERVICEACCOUNT_TOKEN}"
contexts:
- name: whereabouts-context
  context:
    cluster: local
    user: whereabouts
    namespace: ${WHEREABOUTS_NAMESPACE}
current-context: whereabouts-context
EOF

  touch $WHEREABOUTS_FLATFILE
  chmod ${KUBECONFIG_MODE:-600} $WHEREABOUTS_FLATFILE
  cat > $WHEREABOUTS_FLATFILE <<EOF
{
  "datastore": "kubernetes",
  "kubernetes": {
    "kubeconfig": "${WHEREABOUTS_KUBECONFIG_LITERAL}"
  },
  "reconciler_cron_expression": "30 4 * * *"
}
EOF

else
  warn "Doesn't look like we're running in a kubernetes environment (no serviceaccount token)"
fi
}
# ---------------------- end Generate a "kube-config".

function installcni()
{
    if [ "$INSTALL_CNI" == "true"  ]; then
        log "install cni plugins"
        find /plugins -type f -perm -111 -exec cp {} $CNI_BIN_DIR/ \;
    fi 
}

function installcniconf()
{
  if [ "$INSTALL_CNI_CONFLIST" == "true"  ]; then
  cat > $INSTALL_CNI_CONFLISTPATH <<-'EOF'
{
  "cniVersion": "0.4.0",
  "name": "simple",
  "plugins": [
    {
      "type": "bridge",
      "bridge": "cni-simple",
      "isGateway": true,
      "ipMasq": true,
      "hairpinMode": true,
      "ipam": {
        "type": "host-local",
        "routes": [{ "dst": "0.0.0.0/0" }],
        "ranges": [
          [
            {
              "subnet": "10.42.0.0/16",
              "gateway": "10.42.0.1"
            }
          ]
        ]
      }
    },
    {
      "type": "portmap",
      "capabilities": {
        "portMappings": true
      }
    }
  ]
}

EOF

        
  fi 
}


function whereaboutsloop()
{
    while [ "$WHEREABOUTS_LOOP" == "true"  ]; do
        log "execute whereabouts controller loop"
        /ip-control-loop
    done   
}

function sleeploop()
{
    while [ "$SLEEP_LOOP" == "true"  ]; do
      log "Entering sleep... (success)"
      # Sleep forever. 
      # sleep infinity is not available in alpine; instead lets go sleep for ~68 years. Hopefully that's enough sleep
      sleep 2147483647
    done
}

installcni
installcniconf
installwhereaboutsconfig
whereaboutsloop
sleeploop
