#!/bin/bash

gxmessage -buttons "GTK_STOCK_YES:0,GTK_STOCK_NO:1" "Run restart X on PC Primula?"

if [ $? -eq 0 ]; then
    ssh tcsuser@primula "killall fvwm2"
    logger -p local0.err "primula: killall fvwm2 => $?"
fi

exit 0
