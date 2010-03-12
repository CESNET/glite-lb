/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package org.glite.wsdl.services.lb.example;

import java.net.URL;
import org.apache.axis.AxisFault;
import org.apache.axis.AxisProperties;
import org.apache.log4j.Logger;
import org.apache.log4j.ConsoleAppender;

import org.glite.security.trustmanager.ContextWrapper;

import org.glite.wsdl.services.lb.stubs.LoggingAndBookkeeping_ServiceLocator;
import org.glite.wsdl.services.lb.stubs.LoggingAndBookkeepingPortType;
import org.glite.wsdl.services.lb.stubs.JobStatus;

public class LBClientTM {
	private static String proxyFile = null;
	private static String endpoint = "https://localhost:9003";
	private static Logger log = Logger.getLogger(LBClientTM.class);
	
	public static void main(String[] args) throws Exception {
		if (args.length > 0) proxyFile = args[0];
		if (args.length > 1) endpoint = args[1];

		ConsoleAppender ca = new ConsoleAppender(new org.apache.log4j.SimpleLayout());
//		ca.activateOptions();
		log.addAppender(ca);
		
		log.info("endpoint being used "+ endpoint);
		if (proxyFile != null) log.info("proxy location being used " + proxyFile);
		
		URL url = new URL(endpoint);
		int port = url.getPort();
		String protocol = url.getProtocol();
		String path = url.getPath();
		log.info(" port number "+ port + ", protocol ("+ protocol + "), path: " + path);
		
		System.setProperty("sslProtocol", "SSLv3");
		AxisProperties.setProperty("axis.socketSecureFactory","org.glite.security.trustmanager.axis.AXISSocketFactory");
		// certificate based authentication */
//		System.setProperty("sslCertFile","/home/valtri/.cert/hostcert.pem");
//		System.setProperty("sslKey","/home/valtri/.cert/hostkey.pem");
//		System.setProperty("sslKeyPasswd","");
		
		// proxy based authentication
		if (proxyFile != null) System.setProperty("gridProxyFile", proxyFile);
		
		try {
			LoggingAndBookkeeping_ServiceLocator loc = new LoggingAndBookkeeping_ServiceLocator();
			String sn = loc.getLoggingAndBookkeepingWSDDServiceName();
			log.info(" service name " + sn);
			
			LoggingAndBookkeepingPortType stub = loc.getLoggingAndBookkeeping(url);
			log.info(" got endpoint ");
			
			String version = stub.getVersion(null);
			log.info("LB version "+ version);
			JobStatus status = stub.jobStatus("https://scientific.civ.zcu.cz:9000/PnSBRXoIHVzX68H5vAtxmA", null);
			log.info("job status "+ status);
		} catch (AxisFault af) {
			log.error( " AxisFault - " + af.getFaultString());
			af.printStackTrace();
		}
	}
}
