#!/bin/bash

dbus-send --system --dest=me.burnaway.BorderGate --type=method_call --print-reply /me/burnaway/BorderGate me.burnaway.BorderGate.invokeModuleCommand string:"C" string:"initiate 172.16.0.20:20000"
