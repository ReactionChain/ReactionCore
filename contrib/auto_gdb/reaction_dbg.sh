#!/bin/bash
# use testnet settings,  if you need mainnet,  use ~/.reaction/reactiond.pid file instead
reaction_pid=$(<~/.reaction/testnet3/reactiond.pid)
sudo gdb -batch -ex "source debug.gdb" reactiond ${reaction_pid}
