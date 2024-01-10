/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *   $URL$
 *
 *   Copyright (C) 2011 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

import com.sun.jersey.spi.resource.Singleton;
import com.sun.jersey.spi.template.ViewProcessor;
import com.sun.jersey.api.view.Viewable;

import javax.servlet.ServletContext;
import javax.ws.rs.core.Context;
import javax.ws.rs.ext.Provider;
import java.net.MalformedURLException;

import java.io.IOException;
import java.io.Writer;
import java.io.StringWriter;
import java.io.File;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintStream;

import java.util.Map;
import java.util.HashMap;
import java.util.logging.Level;

import freemarker.template.Configuration;
import freemarker.template.Template;
import freemarker.template.TemplateException;
import freemarker.template.DefaultObjectWrapper;
import freemarker.cache.WebappTemplateLoader;

import javax.ws.rs.WebApplicationException;

//@Singleton
@Provider
public class FreeMarkerViewProcessor implements ViewProcessor<Template>
{
    private static Configuration fmConfig;

    private String rootPath;

    @Context
    protected ServletContext servletContext;

    public FreeMarkerViewProcessor() {
    }

    @Context
    public void setServletContext(final ServletContext context) {
        fmConfig = new Configuration();

        rootPath = servletContext.getInitParameter("freemarker.template.path");
        rootPath = rootPath.replaceAll( "/$", "" );

        fmConfig.setTemplateLoader(new WebappTemplateLoader(servletContext, rootPath));

        fmConfig.setNumberFormat("0");
        fmConfig.setLocalizedLookup(false);
        fmConfig.setTemplateUpdateDelay(0);
    }

    public Template resolve(final String path) {
        try {
            final String fullPath = rootPath + path;
            final boolean templateFound = servletContext.getResource(fullPath) != null;

            return templateFound ? fmConfig.getTemplate(path) : null;
        }
        catch (Throwable t) {
            servletContext.log("FreeMarkerViewProcessor.resolve() threw exception", t);
            return null;
        }
    }

    //@SuppressWarnings("unchecked")
    public void writeTo(Template template, Viewable viewable, OutputStream out) throws IOException {
        out.flush(); // send status + headers

        final OutputStreamWriter writer = new OutputStreamWriter(out);

        //final Map<String,Object> vars = new HashMap<String, Object>((Map<String, Object>)model);

        try {
            template.process(viewable.getModel(), writer);
        }
        catch (Throwable t) {
            out.write( "<pre>".getBytes() );
            t.printStackTrace(new PrintStream(out));
            out.write( "</pre>".getBytes() );
        }
    }
}
