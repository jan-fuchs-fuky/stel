/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *   $URL$
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

package cz.cas.asu.stelweb.fuky.observe.server.resources;

import java.util.Map;
import java.util.HashMap;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.Status;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.ext.ExceptionMapper;
import javax.ws.rs.ext.Provider;
import javax.ws.rs.WebApplicationException;
import com.sun.jersey.api.view.Viewable;

@Provider
public class WebApplicationExceptionMapper implements ExceptionMapper<WebApplicationException> {
    static final int FORBIDDEN = Status.FORBIDDEN.getStatusCode();
    static final int BAD_REQUEST = Status.BAD_REQUEST.getStatusCode();
    static final int INTERNAL_SERVER_ERROR = Status.INTERNAL_SERVER_ERROR.getStatusCode();

    public Response toResponse(WebApplicationException e) {
        Response response = e.getResponse();
        Map<String, String> rootMap = new HashMap<String, String>();

        if (response.getStatus() == FORBIDDEN) {
            rootMap.put("message", "User does not have permission to perform this action.");
        }
        else if (response.getStatus() == INTERNAL_SERVER_ERROR) {
            rootMap.put("message", "Internal server error.");
        }
        else if (response.getStatus() == BAD_REQUEST) {
            rootMap.put("message", "Bad request.");
        }
        else {
            if (e.getMessage() == null) {
                rootMap.put("message", "Unknown error.");
            }
            else {
                String[] message = e.getMessage().split(":", 2);

                if (message.length >= 2) {
                    rootMap.put("message", message[1].trim());
                }
                else {
                    rootMap.put("message", message[0]);
                }
            }
        }

        return Response.status(response.getStatus()).
            entity(new Viewable("/error.ftl", rootMap)).
            type(MediaType.TEXT_HTML).
            build();
    }
}
