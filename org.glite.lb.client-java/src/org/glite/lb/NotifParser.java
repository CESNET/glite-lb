/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package org.glite.lb;

import java.io.IOException;
import java.io.StringReader;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;
import org.apache.commons.lang.StringEscapeUtils;
import org.glite.wsdl.types.lb.JobStatus;
import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

/**
 * this class parses the received message into a readable format
 *
 * @author Kopac
 */
public class NotifParser {

    Document doc = null;
    String header = null;


    /**
     * constructor takes a notification in String format and parses it into a String
     * containing the header and an XML doc
     *
     * @param notif the string with notification in it
     * @throws javax.xml.parsers.ParserConfigurationException
     * @throws org.xml.sax.SAXException
     * @throws java.io.IOException
     */
    public NotifParser(String notif) throws ParserConfigurationException, SAXException, IOException {
        String[] splitString = notif.split("DG.NOTIFICATION.JOBSTAT=\"", 2);
        header = splitString[0];
        doc = createXML(splitString[1]);
    }

    /**
     * this method reads through an XML document using XPath expressions and
     * fills an instance of JobStatus, which it then returns
     * this is done using automatically generated code
     *
     * @param notification an array of bytes containing the raw notification
     * @return a Jobstatus instance
     */
    public JobStatus getJobInfo()
        throws ParserConfigurationException, SAXException, IOException {
        JobStatus status = new JobStatus();
        //TODO: insert automated code creation
        status.setJobId(evaluateXPath("//jobId").item(0).getTextContent());
        status.setOwner(evaluateXPath("//owner").item(0).getTextContent());
        return status;
    }

     /**
     * this method returns id of the notification
     *
     * @return notif id
     */
    public String getNotifId() {
        String halfHeader = header.split("DG.NOTIFICATION.NOTIFID=\"")[1];
        return halfHeader.substring(0, halfHeader.indexOf("\""));
    }

    /**
     * a method for handling xpath queries
     *
     * @param xpathString xpath expression
     * @return the result nodelist
     */
    private NodeList evaluateXPath(String xpathString) {
        try {
            XPathFactory xfactory = XPathFactory.newInstance();
            XPath xpath = xfactory.newXPath();
            XPathExpression expr = xpath.compile(xpathString);
            return (NodeList) expr.evaluate(doc, XPathConstants.NODESET);
        } catch (XPathExpressionException ex) {
            ex.printStackTrace();
            return null;
        }
    }

    /**
     * this method creates an XML document out of a provided String
     * note that the string has to be a valid escaped XML document representation,
     * otherwise the process will fail
     *
     * @param body a String containing an XML document
     * @return an XML document in the Document format
     * @throws javax.xml.parsers.ParserConfigurationException
     * @throws org.xml.sax.SAXException
     * @throws java.io.IOException
     */
    private Document createXML(String body) throws ParserConfigurationException, SAXException, IOException {
        String removed = body.substring(0, body.length()-1);
        String parsed = StringEscapeUtils.unescapeXml(removed);
        //this code removes hexadecimal references from the string (couldn't find
        //a suitable parser for this)
        StringBuilder build = new StringBuilder(parsed);
        int j = build.indexOf("%");
        while(j > 0) {
            if(build.charAt(j-1) == '>') {
                build.delete(j, j+3);
                if(build.charAt(j) == '<') {
                    j = build.indexOf("%");
                }
            } else {
                j = build.indexOf("%", j+1);
            }
        }
        parsed = build.toString();
        //ends here
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        return factory.newDocumentBuilder().parse(new InputSource(new StringReader(parsed)));
    }
}
