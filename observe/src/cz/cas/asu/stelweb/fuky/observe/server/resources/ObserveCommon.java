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

import java.util.List;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.ServletContext;

import org.hibernate.Session;
import org.hibernate.SessionFactory;

public class ObserveCommon {

    public static boolean allowAddr(HttpServletRequest httpServletRequest,
                                    ServletContext servletContext,
                                    SessionFactory sessionFactory) {
        Session session = null;
        String remoteAddr = httpServletRequest.getRemoteAddr();
        String hqlSelect = String.format("from Addresses where address like '%s'", remoteAddr);

        try {
            session = sessionFactory.openSession();
            session.beginTransaction();
            List address = session.createQuery(hqlSelect).list();
            session.getTransaction().commit();

            if (address.size() > 0) {
                return true;
            }
        }
        catch (Exception e) {
            servletContext.log("UsersResource.allowAddr()", e);
        }
        finally {
            if (session != null) {
                session.close();
            }
        }

        return false;
    }
}
