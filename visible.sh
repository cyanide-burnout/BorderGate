#!/bin/bash

if [ -n "$1" ]
then
  dbus-send --system --dest=me.burnaway.BorderGate --type=method_call --print-reply /me/burnaway/BorderGate me.burnaway.BorderGate.setCallVisibility string:"$1" boolean:true
else
  SELF=`readlink -f $0`
  grep -o -E "^[A-Z0-9]{3,8}" | xargs -l $SELF
fi
