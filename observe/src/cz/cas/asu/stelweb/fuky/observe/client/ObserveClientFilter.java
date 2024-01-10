/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *
 *   Copyright (C) 2010 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
 *
 *   This file is part of Observe (Observing System for Ondrejov).
 *
 *   Observe is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Observe is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Observe.  If not, see <http://www.gnu.org/licenses/>.
 */

package cz.cas.asu.stelweb.fuky.observe.client;

import com.sun.jersey.api.client.ClientResponse;
import com.sun.jersey.api.client.ClientRequest;
import com.sun.jersey.api.client.config.ClientConfig;
import com.sun.jersey.api.client.filter.ClientFilter;

class ObserveClientFilter extends ClientFilter {
    private int connectTimeout;
    private int readTimeout;

    public ObserveClientFilter(int connectTimeout, int readTimeout) {
        this.connectTimeout = connectTimeout;
        this.readTimeout = readTimeout;
    }

    public ClientResponse handle(ClientRequest clientRequest) {
        clientRequest.getProperties().put(ClientConfig.PROPERTY_CONNECT_TIMEOUT, this.connectTimeout);
        clientRequest.getProperties().put(ClientConfig.PROPERTY_READ_TIMEOUT, this.readTimeout);
        
        return getNext().handle(clientRequest);
    }
}
