#!/bin/bash

if [ -n "$1" ]
then
  dbus-send --system --dest=me.burnaway.BorderGate --type=method_call --print-reply /me/burnaway/BorderGate me.burnaway.BorderGate.invokeModuleCommand string:"$1" string:"reconnect"
fi

