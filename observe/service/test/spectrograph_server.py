#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# Copyright (C) 2021 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
#

import xmlrpc.server

class SpectrographServer:

    def __init__(self):
        port = 8888

        self.grating_angle = "0"
        self.spectral_filter = "0"

        self.glst = {
        }

        server = xmlrpc.server.SimpleXMLRPCServer(("127.0.0.1", port))
        server.register_introspection_functions()
        server.register_function(self.rpc_spectrograph_execute, "spectrograph_execute")
        server.register_function(self.rpc_spectrograph_info, "spectrograph_info")

        server.serve_forever()

    def rpc_spectrograph_execute(self, cmd):
        print(cmd)

        if cmd.startswith("SPAP 13"):
            value = cmd.split(" ")[2]
            self.grating_angle = value
        elif cmd.startswith("SPCH 2"):
            value = cmd.split(" ")[2]
            self.spectral_filter = value
        elif cmd.startswith("SPGP 13"):
            return self.grating_angle
        elif cmd.startswith("SPGS 2"):
            return self.spectral_filter

        return "1"

    def rpc_spectrograph_info(self):

        # GLST - GLobal STate:
        #
        #     1. dichroicka zrcatka - SPGS 1
        #     2. spektralni filtr - SPGS 2
        #     3. maska kolimatoru - SPGS 3
        #     4. ostreni 700
        #     5. ostreni 1400/400
        #     6. preklapeni hvezda/kalibrace - SPGS 6
        #     7. preklapeni Coude/Oes - SPGS 7
        #     8. flat field - SPGS 8
        #     9. srovnavaci spektrum - SPGS 9
        #     10. zaverka expozimetru - SPGS 10
        #     11. zaverka kamery 700 - SPGS 11
        #     12. zaverka kamery 1400/400 - SPGS 12
        #     13. mrizka
        #     14. expozimeter
        #     15. sterbinova kamera - SPGS 15
        #     16. korekcni desky ostreni 700 - SPGS 16
        #     17. korekcni desky ostreni 1400/400 - SPGS 17
        #     18. rezerva
        #     19. rezerva
        #     20. rezerva
        #     21. maska kolimatoru Oes - SPGS 21
        #     22. ostreni Oes
        #     23. zaverka expozimetru Oes - SPGS 23
        #     24. expozimeter Oes
        #     25. rezerva
        #     26. jodova banka - SPGS 26

        info = {
            "GLST": (24 * "0 ").strip(), # GLobal STate
            "SPGP_4": "0", # absolutni pozice ostreni 700
            "SPGP_5": "0", # absolutni pozice ostreni 1400/400
            "SPGP_13": self.grating_angle, # absolutni pozice mrizky
            "SPCE_14": "0", # pocet pulzu nacitanych expozimetrem
            "SPFE_14": "0", # frekvence pulsu nacitanych expozimetrem
            "SPCE_24": "0", # pocet pulzu nacitanych expozimetrem Oes
            "SPFE_24": "0", # frekvence pulzu nacitanych expozimetrem Oes
            "SPGP_22": "0", # absolutni pozice ostreni Oes
            "SPGS_19": "0", # teplota v Coude
            "SPGS_20": "0", # teplota v OESu
        }

        return info

def main():
    SpectrographServer()

if __name__ == '__main__':
    main()
