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

import cz.cas.asu.stelweb.fuky.observe.xml.SpectrographExecuteXML;
import cz.cas.asu.stelweb.fuky.observe.xml.SpectrographInfoXML;
import cz.cas.asu.stelweb.fuky.observe.client.ObserveProperties;

import java.net.URL;
import java.io.StringWriter;
import java.util.logging.Level;
import java.util.HashMap;
import java.util.Iterator;
import javax.servlet.ServletContext;
import javax.servlet.http.HttpServletRequest;

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

// TODO: odstranit duplicitu s TelescopeResource
@Path("/spectrograph")
public class SpectrographResource {
    private SecurityContext sc;
    private ServletContext servletContext;
    private String role;
    private XmlRpcClient client;
    private StringWriter response = new StringWriter();
    private ObserveProperties props = ObserveProperties.getInstance();

    public SpectrographResource(@Context SecurityContext sc,
                                @Context HttpServletRequest httpServletRequest,
                                @Context SessionFactory sessionFactory,
                                @Context ServletContext servletContext) {
        this.sc = sc;
        this.servletContext = servletContext;

        if (sc.isUserInRole("none")) {
            throw new WebApplicationException(Status.FORBIDDEN);
        }
        else if (sc.getUserPrincipal().getName().equals("tcsuser") &&
                 (!ObserveCommon.allowAddr(httpServletRequest, servletContext, sessionFactory))) {
            throw new WebApplicationException(Status.FORBIDDEN);
        }

        try {
            XmlRpcClientConfigImpl config = new XmlRpcClientConfigImpl();

            config.setServerURL(new URL(
                String.format("http://%s:%s/RPC2",
                    servletContext.getInitParameter("observe.xmlrpc.spectrograph.host"),
                    servletContext.getInitParameter("observe.xmlrpc.spectrograph.port")
                ))
            );

            this.client = new XmlRpcClient();
            this.client.setConfig(config);
        }
        catch (Exception e) {
            servletContext.log("SpectrographResource()", e);
            throw new WebApplicationException(e);
        }
    }

    @GET 
    @Produces(MediaType.APPLICATION_XML)
    public String get() {
        Object untypedResult = null;

        try {
            untypedResult = this.client.execute("spectrograph_info", new Object[]{});
        }
        catch (Exception e) {
            servletContext.log("SpectrographResource.get()", e);
            throw new WebApplicationException(Status.SERVICE_UNAVAILABLE);
        }

        if (!(untypedResult instanceof HashMap)) {
            servletContext.log("Expected return of type Hashtable, but got " + untypedResult.getClass());
            throw new WebApplicationException(Status.INTERNAL_SERVER_ERROR);
        }

        try {
            SpectrographInfoXML spectrographInfoXML = new SpectrographInfoXML((HashMap)untypedResult);

            return object2xml(spectrographInfoXML, SpectrographInfoXML.class);
        }
        catch (Exception e) {
            servletContext.log("SpectrographResource.get()", e);
            throw new WebApplicationException(Status.INTERNAL_SERVER_ERROR);
        }
    }

    @PUT
    @Produces(MediaType.APPLICATION_XML)
    @Consumes({MediaType.APPLICATION_XML, MediaType.TEXT_XML})
    public String execute(JAXBElement<SpectrographExecuteXML> jaxbSpectrographExecuteXML) {
        if ((!sc.isUserInRole("admin")) && (!sc.isUserInRole("control"))) {
            throw new WebApplicationException(Status.FORBIDDEN);
        }

        SpectrographExecuteXML spectrographExecuteXML = jaxbSpectrographExecuteXML.getValue();

        try {
            String result = (String)this.client.execute(
                spectrographExecuteXML.getFunctionName(), spectrographExecuteXML.getFunctionParams());
            spectrographExecuteXML.setResult(result);
        }
        catch (Exception e) {
            servletContext.log("SpectrographResource.execute()", e);
            throw new WebApplicationException(Status.SERVICE_UNAVAILABLE);
        }

        return object2xml(spectrographExecuteXML, SpectrographExecuteXML.class);
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
            servletContext.log("SpectrographResource.execute()", e);
            throw new WebApplicationException(e);
        }

        return this.response.toString();
    }
}
