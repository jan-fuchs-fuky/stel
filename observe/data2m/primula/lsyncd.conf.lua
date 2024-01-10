----
-- User configuration file for lsyncd.
--
settings {
    statusInterval = 1,
    logfile    = "/var/log/lsyncd/lsyncd.log",
    statusFile = "/var/log/lsyncd/lsyncd.status",
    insist = true,
--    nodaemon   = true,
}

sync {
    default.rsync,
    delay=5,
    source="/data/pointing/INCOMING/",
    target="data2m@sirius.stel:pointing",
    delete=false,

    rsync = {
       protect_args=false,
        _extra = {
          "-a",
        }
    }
}

sync {
    default.rsync,
    delay=5,
    source="/data/fotometrie/INCOMING/",
    target="data2m@sirius.stel:fotometrie",
    delete=false,

    rsync = {
       protect_args=false,
        _extra = {
          "-a",
        }
    }
}
