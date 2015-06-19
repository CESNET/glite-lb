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

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package org.glite.lb;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;

/**
 * this class handles communication with LB server (reads messages it sends)
 *
 * @author Kopac
 */
public class ILProto {

    private Socket socket = null;
    private InputStream inStream = null;
    private OutputStream outStream = null;
    private static final String magicWordXXX = "6 michal";
    private static final String magicWord = "michal";
    private int min,maj;
    private String err;
    private byte[] buf;
    private int bufptr,bufsiz;

    /**
     * construcor initializes the class' socket, inStream and outStream attributes
     *
     * @param socket an SSLSocket
     * @throws IOException if any I/O operation fails
     */
    public ILProto(Socket socket) throws IOException {
        this.socket = socket;
        inStream = this.socket.getInputStream();
        outStream = this.socket.getOutputStream();
    }

    /**
     * this method reads from the inpuStream of the provided socket, checks for
     * the magic word and returns relevant info
     *
     * @return a String containing third line of the inputStream data, without
     * the info about its length
     * @throws IOException if any I/O operation fails
     */
    /* XXX: weird implementation, should follow C */
    public String receiveMessage() throws IOException{
        byte[] b = new byte[17];
        int i = 0;
        //read in and convert size of the message
        if(inStream.read(b, 0, 17) == -1) {
            return null;
        } else {
            String test = new String(b);
            test.trim();
            int length = Integer.parseInt(test);
            byte[] notification = new byte[length];
            //read in the rest of the message
            int j = 0;
            while(i != length || j == -1) {
                j = inStream.read(notification, i, length-i);
                i=i+j;
            }
            String retString = checkWord(notification);
            if(retString == null) return null;
            else
            //return
            return retString.split(" ", 2)[1];
        }
    }

    public void sendMessage(String msg) throws IOException
    {
	    newbuf(magicWord.length() + msg.length() + 100);
	    put_string(magicWord);
	    put_string(msg);
	    String hdr = String.format("%16d\n",bufptr);
	    outStream.write(hdr.getBytes());

	    writebuf();
	    outStream.flush();
    }

    public int receiveReply() throws IOException,LBException
    {
	    newbuf(17);

	    if (readbuf(17) != 17) {
		    throw new LBException("reading IL proto header");
	    }
	    int		len = Integer.parseInt((new String(buf)).trim());

	    newbuf(len);
	    if (readbuf(len) < len) {
		    throw new LBException("incomplete IL message");
	    }

	    rewind();
	    this.maj = get_int();
	    this.min = get_int();
	    this.err = get_string();

	    return this.maj;
    }

    public int errMin() { return min; }
    public String errMsg() { return err; }

    /**
     * private method that checks, if the magic word is present in the notification
     *
     * @param notification a notification without the line specifying its length
     * @return null, if the word is not there, the last line of the notification,
     * again without its length specification, otherwise
     */
    private String checkWord(byte[] notification) {
        int i = 0;
        while(notification[i] != '\n') {
            i++;
        }
        String word = new String(notification, 0, i+1);
        word.trim();
        if(!word.equals(magicWordXXX)) {
            return null;
        } else {
            return new String(notification, i+1, notification.length - i + 1);
        }
    }

    /**
     * this method encodes and sends a reply to the interlogger via the socket's
     * outputStream
     *
     * @param errCode errCode of the calling
     * @param minErrCode minimum available errcode
     * @param message message for the interlogger - could be any String
     * @throws IOException if any I/O operation fails
     */
    public void sendReply(int errCode, int minErrCode, String message) throws IOException {
        byte[] errByte = (new String()+errCode).getBytes();
        byte[] minErrByte = (new String()+minErrCode).getBytes();
        byte[] msgByte = message.getBytes();
        int length = errByte.length + minErrByte.length + msgByte.length;
        byte[] lengthByte = (new String()+length).getBytes();
        int numberOfSpaces = 17 - lengthByte.length;
        byte[] returnByte = new byte[length+17];
        int i = 0;
        while(i < numberOfSpaces-1) {
            returnByte[i] = ' ';
            i++;
        }
        returnByte = putByte(returnByte, lengthByte, numberOfSpaces);
        returnByte[16] = '\n';
        returnByte = putByte(returnByte, errByte, 17);
        returnByte = putByte(returnByte, minErrByte, 16 + errByte.length);
        returnByte = putByte(returnByte, msgByte, 16 + errByte.length + minErrByte.length);
        outStream.write(returnByte);
    }

    /**
     * appends a byte array to the end of an existing byte array
     *
     * @param arrayToFill array to be filled
     * @param filler array to be appended
     * @param start starting offset of the first array, from which the second
     * array should be appended
     * @return the resulting byte array
     */
    private byte[] putByte(byte[] arrayToFill, byte[] filler, int start) {
        for(int i = start; i < filler.length + start; i++) {
            int j = 0;
            arrayToFill[i] = filler[j];
            j++;
        }
        return arrayToFill;
    }

    private void newbuf(int size) {
	    buf = new byte[size];
	    bufptr = 0;
	    bufsiz = size;
    }

    private void rewind() { bufptr = 0; }

    private int readbuf(int size) throws IOException {
	    int r,total = 0;
	   
	    while (size > 0 && (r = inStream.read(buf,bufptr,size)) > 0) {
		    bufptr += r;
		    size -= r;
		    total += r;
	    }
	    return total;
    }

    private void writebuf() throws IOException {
	    outStream.write(buf,0,bufptr);
    }

    private void _put_int(int ii)
    {
    	String s = new String() + ii;
	byte[] b = s.getBytes();
	int i;

	for (i=0; i<b.length; i++) buf[bufptr++] = b[i];
    }
    private void put_int(int i) { _put_int(i); buf[bufptr++] = '\n'; }
    private void put_string(String s) {
	   int	i;
	   byte b[] = s.getBytes();
    	   _put_int(b.length);
	   buf[bufptr++] = ' ';
	   for (i=0; i<b.length; i++) buf[bufptr++] = b[i];
	   buf[bufptr++] = '\n';
    }

/* FIXME: sanity checks */
    private int get_int() {
	    int	i,o;

	    for (i=0; Character.isDigit(buf[bufptr+i]); i++);
	    o = Integer.parseInt(new String(buf,bufptr,i));
	    bufptr += i+1;
	    return o;
    }
    private String get_string() {
	    int len = get_int();
	    String out = new String(buf,bufptr,len);
	    bufptr += len+1;
	    return out;
    }
}
