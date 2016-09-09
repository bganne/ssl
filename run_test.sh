#!/bin/bash

# NetworkManager sucks
cat << EOF
*****************************************************************************
* WARNING: make sure NetworkManager will *not* mess-up with stun interfaces *
* WARNING: to do so, you can use something like:                            *
* WARNING: ~# sudo cat >> /etc/NetworkManager/NetworkManager.conf << EOF    *
* WARNING     plugins+=keyfile                                              *
* WARNING:    [keyfile]                                                     *
* WARNING:    unmanaged-devices=interface-name:stun*                        *
* WARNING:    EOF                                                           *
* WARNING: ~# sudo systemctl restart NetworkManager                         *
* WARNING: ... or just stop this piece of crap altogether:                  *
* WARNING: ~# sudo systemctl stop NetworkManager                            *
*****************************************************************************
EOF

PORT=$1
CIPHER=$2
DURATION=$3

[ -z "$PORT" -o -z "$CIPHER" -o -z "$DURATION" ] && exit 1

SERVER=127.0.0.1
IPERFS=10.0.0.1
IPERFC=10.0.1.1
PACKETSZ=300
BW=1G

set -exu

ulimit -c unlimited

# start server
./server $PORT $CIPHER &
sleep 0.5

# start clients
./client 127.0.0.1 $PORT $CIPHER &
./client 127.0.0.1 $PORT $CIPHER &
sleep 0.5

# setup network
ip netns del net0 || true
ip netns del net1 || true
ip netns add net0
ip netns add net1
ip link set dev stun0 netns net0
ip link set dev stun1 netns net1
ip netns exec net0 ip link set dev stun0 up
ip netns exec net1 ip link set dev stun1 up
ip netns exec net0 ip addr add $IPERFS peer $IPERFC scope link dev stun0
ip netns exec net1 ip addr add $IPERFC peer $IPERFS scope link dev stun1

# send some ping to force ssl setup (and check it works)
ip netns exec net1 ping -c1 $IPERFS

# run test
ip netns exec net0 iperf -u -s  &
sleep 0.5
ip netns exec net1 iperf -u -c $IPERFS -d -b$BW -t$DURATION -l$PACKETSZ
sleep 0.5

# cleanup
kill $(jobs -p) || true
sleep 2
kill -9 $(jobs -p) || true
ip netns del net0 || true
ip netns del net1 || true
