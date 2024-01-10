#!/bin/bash

/etc/init.d/tomcat6 stop

rsync --delete --exclude .svn -a ./class/ /var/lib/tomcat6/webapps/observe/WEB-INF/classes
rsync --delete --exclude .svn -a ./lib/java/ /var/lib/tomcat6/webapps/observe/WEB-INF/lib
rsync --delete --exclude .svn -a ./templates/ /var/lib/tomcat6/webapps/observe/WEB-INF/templates
rsync --delete --exclude .svn -a ./www/css /var/lib/tomcat6/webapps/observe
rsync --delete --exclude .svn -a ./www/setup /var/lib/tomcat6/webapps/observe

cp ./properties/web.xml /var/lib/tomcat6/webapps/observe/WEB-INF/
cp /data/etc/hibernate.cfg.xml /var/lib/tomcat6/webapps/observe/WEB-INF/classes

chown -R tomcat6.tomcat6 /var/lib/tomcat6/webapps/observe

/etc/init.d/tomcat6 start
