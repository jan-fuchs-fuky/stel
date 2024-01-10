/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date: 2011-10-29 23:28:57 +0200 (Sat, 29 Oct 2011) $
 *   $Rev$
 *   $URL$
 *
 *   Copyright (C) 2010-2012 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

import cz.cas.asu.stelweb.fuky.observe.xml.ExposeExecuteXML;

import javax.ws.rs.Path;
import javax.ws.rs.GET;
import javax.ws.rs.PUT;
import javax.ws.rs.Produces;
import javax.ws.rs.Consumes;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.SecurityContext;

import javax.xml.bind.JAXBElement;

import javax.servlet.ServletContext;
import javax.servlet.http.HttpServletRequest;

import org.hibernate.SessionFactory;

@Path("/ccd700")
public class CCD700Resource extends ExposeResource {

    public CCD700Resource(@Context SecurityContext securityCtx,
                          @Context ServletContext servletCtx,
                          @Context HttpServletRequest httpServletRequest,
                          @Context SessionFactory sessionFactory) {

        super(securityCtx, servletCtx, httpServletRequest, sessionFactory, "observe.xmlrpc.ccd700.host", "observe.xmlrpc.ccd700.port");
    }

    @GET 
    @Produces(MediaType.APPLICATION_XML)
    public String get() {
        return exposeInfo();
    }

    @PUT
    @Produces(MediaType.APPLICATION_XML)
    @Consumes({MediaType.APPLICATION_XML, MediaType.TEXT_XML})
    public String put(JAXBElement<ExposeExecuteXML> jaxbExposeExecuteXML) {
        return execute(jaxbExposeExecuteXML);
    }
}
