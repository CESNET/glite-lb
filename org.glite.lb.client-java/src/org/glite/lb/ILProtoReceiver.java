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
public class ILProtoReceiver {

    private Socket socket = null;
    private InputStream inStream = null;
    private OutputStream outStream = null;
    private static final String magicWord = "6 michal";

    /**
     * construcor initializes the class' socket, inStream and outStream attributes
     *
     * @param socket an SSLSocket
     * @throws java.io.IOException
     */
    public ILProtoReceiver(Socket socket) throws IOException {
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
     * @throws IOException
     */
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
                j = inStream.read(notification, i, length);
                i=i+j;
            }
            String retString = checkWord(notification);
            if(retString == null) return null;
            else
            //return
            return retString.split(" ", 2)[1];
        }
    }

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
        if(!word.equals(magicWord)) {
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
     * @throws IOException
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
}
