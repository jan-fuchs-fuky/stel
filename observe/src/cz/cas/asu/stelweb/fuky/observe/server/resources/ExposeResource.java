/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *   $URL$
 *
 *   Copyright (C) 2010-2011 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

import cz.cas.asu.stelweb.fuky.observe.xml.ExposeInfoXML;
import cz.cas.asu.stelweb.fuky.observe.xml.ExposeExecuteXML;

import java.net.URL;
import java.io.StringWriter;
import java.util.logging.Level;
import java.util.HashMap;
import java.util.Iterator;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.ServletContext;

import javax.ws.rs.GET;
import javax.ws.rs.PUT;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.Consumes;
import javax.ws.rs.PathParam;

import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.SecurityContext;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.Response.Status;
import javax.ws.rs.WebApplicationException;

import javax.xml.bind.JAXBElement;
import javax.xml.bind.JAXBContext;
import javax.xml.bind.JAXBException;
import javax.xml.bind.Marshaller;

import org.hibernate.SessionFactory;
import org.apache.xmlrpc.client.XmlRpcClient;
import org.apache.xmlrpc.client.XmlRpcClientConfigImpl;

abstract class ExposeResource {
    protected SecurityContext securityCtx;
    protected ServletContext servletCtx;
    protected String role;
    protected XmlRpcClient client;
    protected StringWriter response = new StringWriter();

    public ExposeResource(SecurityContext securityCtx,
                          ServletContext servletCtx,
                          HttpServletRequest httpServletRequest,
                          SessionFactory sessionFactory,
                          String hostPar,
                          String portPar) {

        this.securityCtx = securityCtx;
        this.servletCtx = servletCtx;

        if (securityCtx.isUserInRole("none")) {
            throw new WebApplicationException(Status.FORBIDDEN);
        }
        else if (securityCtx.getUserPrincipal().getName().equals("tcsuser") &&
                 (!ObserveCommon.allowAddr(httpServletRequest, servletCtx, sessionFactory))) {
            throw new WebApplicationException(Status.FORBIDDEN);
        }

        try {
            XmlRpcClientConfigImpl config = new XmlRpcClientConfigImpl();

            config.setServerURL(new URL(
                String.format("http://%s:%s/RPC2",
                    servletCtx.getInitParameter(hostPar),
                    servletCtx.getInitParameter(portPar)
                ))
            );

            this.client = new XmlRpcClient();
            this.client.setConfig(config);
        }
        catch (Exception e) {
            servletCtx.log("ExposeResource()", e);
            throw new WebApplicationException(e);
        }
    }

    public abstract String get();

    public String exposeInfo() {
        Object untypedResult = null;

        try {
            untypedResult = this.client.execute("expose_info", new Object[]{});
        }
        catch (Exception e) {
            servletCtx.log("ExposeResource.get()", e);
            throw new WebApplicationException(Status.SERVICE_UNAVAILABLE);
        }

        if (!(untypedResult instanceof HashMap)) {
            servletCtx.log("Expected return of type Hashtable, but got " + untypedResult.getClass());
            throw new WebApplicationException(Status.INTERNAL_SERVER_ERROR);
        }

        try {
            ExposeInfoXML exposeInfoXML = new ExposeInfoXML((HashMap)untypedResult);

            return object2xml(exposeInfoXML, ExposeInfoXML.class);
        }
        catch (Exception e) {
            servletCtx.log("ExposeResource.get()", e);
            throw new WebApplicationException(Status.INTERNAL_SERVER_ERROR);
        }
    }

    public abstract String put(JAXBElement<ExposeExecuteXML> jaxbExposeExecuteXML);

    public String execute(JAXBElement<ExposeExecuteXML> jaxbExposeExecuteXML) {
        if ((!securityCtx.isUserInRole("admin")) && (!securityCtx.isUserInRole("control"))) {
            throw new WebApplicationException(Status.FORBIDDEN);
        }

        ExposeExecuteXML exposeExecuteXML = jaxbExposeExecuteXML.getValue();

        try {
            String result = (String)this.client.execute(
                exposeExecuteXML.getFunctionName(), exposeExecuteXML.getFunctionParams());
            exposeExecuteXML.setResult(result);
        }
        catch (Exception e) {
            throw new WebApplicationException(Status.SERVICE_UNAVAILABLE);
        }

        return object2xml(exposeExecuteXML, ExposeExecuteXML.class);
    }

    private String object2xml(Object object, Class... classesToBeBound) {
        try {
            JAXBContext context = JAXBContext.newInstance(classesToBeBound);
            Marshaller marshaller = context.createMarshaller();

            marshaller.setProperty(Marshaller.JAXB_FORMATTED_OUTPUT, true);
            marshaller.marshal(object, this.response);
            this.response.flush();
        }
        catch (JAXBException e) {
            servletCtx.log("ExposeResource.object2xml()", e);
            throw new WebApplicationException(e);
        }

        return this.response.toString();
    }
}
